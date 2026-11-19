#include "Sim/GGSim.h"

#include <cmath>

// Fused multiply-add changes rounding, and compilers fuse differently per platform. Keep every operation
// separately rounded so a shot replays identically on every device and in the WebAssembly referee.
#if defined(__clang__)
#pragma clang fp contract(off)
#elif defined(_MSC_VER)
#pragma fp_contract(off)
#endif

namespace GG
{
double Length(FVec3 A)
{
	return std::sqrt(A.X * A.X + A.Y * A.Y + A.Z * A.Z);
}

namespace
{
struct FSurfaceProps
{
	double Restitution;
	/** Rolling resistance as a multiple of the force pressing the ball into the surface. */
	double Rolling;
	/** Fraction of tangential speed kept through a bounce. */
	double Grip;
	/** Speed lost per second while touching, whatever the load. Soft surfaces (hedges, sand) grab the ball. */
	double Drag;
};

FSurfaceProps PropsFor(ESurface Surface)
{
	switch (Surface)
	{
	case ESurface::Green:
		return {0.3, 0.09, 0.8, 0.0};
	case ESurface::Stone:
		return {0.55, 0.06, 0.8, 0.0};
	case ESurface::Sand:
		return {0.05, 0.9, 0.1, 14.0};
	case ESurface::Ice:
		return {0.5, 0.01, 0.95, 0.0};
	case ESurface::Grass:
	default:
		return {0.35, 0.25, 0.8, 0.0};
	}
}

double Clamp(double V, double Lo, double Hi)
{
	return V < Lo ? Lo : (V > Hi ? Hi : V);
}

void Emit(std::vector<FEvent>* Events, EEventType Type, ESurface Surface, double Speed, FVec3 Position)
{
	if (Events)
	{
		Events->push_back({Type, Surface, Speed, Position, -1});
	}
}

uint64_t Mix(uint64_t Hash, const void* Data, size_t Size)
{
	const auto* Bytes = static_cast<const unsigned char*>(Data);
	for (size_t I = 0; I < Size; ++I)
	{
		Hash ^= Bytes[I];
		Hash *= 1099511628211ull;
	}
	return Hash;
}
} // namespace

void FSim::Begin(const FCourse& InCourse, const std::vector<FAttractor>& InAttractors, FVec3 Start, FVec3 Launch,
	const FPhysics& InPhysics, const std::vector<bool>* HeldSwitches)
{
	Course = &InCourse;
	Attractors = InAttractors;
	Physics = InPhysics;
	BallState = {};
	BallState.Position = Start;
	BallState.Velocity = Launch;
	BallState.Radius = InCourse.BallRadius;
	bInCupZone = false;
	Engaged.assign(Attractors.size(), false);
	Released.assign(Attractors.size(), false);
	PressedAt.assign(InCourse.Switches.size(), -1.0);
	for (size_t I = 0; HeldSwitches && I < PressedAt.size() && I < HeldSwitches->size(); ++I)
	{
		if ((*HeldSwitches)[I])
		{
			PressedAt[I] = 0.0;
		}
	}
	GateWasOpen.assign(InCourse.Gates.size(), false);
}

FVec3 FSim::Acceleration(FVec3 Position, FVec3 Velocity, std::vector<FEvent>* Events)
{
	FVec3 Total = Course->Gravity;
	const double Soft2 = Physics.AttractorSoftening * Physics.AttractorSoftening;
	for (size_t I = 0; I < Attractors.size(); ++I)
	{
		const FAttractor& A = Attractors[I];
		const FVec3 D = A.Position - Position;
		const double R2 = Dot(D, D);
		const double Range2 = A.Range * A.Range;
		if (R2 >= Range2 || Released[I])
		{
			continue;
		}

		if (A.Strength > 0.0)
		{
			// An attractor bends the ball as it passes, then lets go: it pulls only while the ball is
			// approaching. Once the ball has made its closest pass and starts to move away, the machine
			// releases it for the rest of the swing. So it turns corners; it never traps the ball.
			if (Dot(Velocity, D) <= 0.0)
			{
				if (Engaged[I])
				{
					Released[I] = true;
					if (Events)
					{
						Events->push_back({EEventType::Released, ESurface::Stone, Length(Velocity), Position, static_cast<int>(I)});
					}
				}
				continue;
			}
			if (!Engaged[I])
			{
				Engaged[I] = true;
				if (Events)
				{
					Events->push_back({EEventType::Engaged, ESurface::Stone, Length(Velocity), Position, static_cast<int>(I)});
				}
			}
		}

		// Dome-shaped field: a steady force that eases to zero at the dome's edge and at its very center.
		const double Window = 1.0 - R2 / Range2;
		const double Direction = 1.0 / std::sqrt(R2 + Soft2);
		Total = Total + D * (A.Strength * Window * Window * Direction);
	}
	return Total;
}

void FSim::Step(std::vector<FEvent>* Events)
{
	if (BallState.State != EBallState::Moving)
	{
		return;
	}

	// Never move more than half a radius per sub-step.
	const double Travel = Length(BallState.Velocity) * Dt;
	const double Limit = BallState.Radius * 0.5;
	int Count = 1;
	while (Travel > Limit * Count && Count < 64)
	{
		Count *= 2;
	}
	const double H = Dt / Count;
	for (int I = 0; I < Count && BallState.State == EBallState::Moving; ++I)
	{
		SubStep(H, Events);
	}
}

bool FSim::GateOpen(int Index) const
{
	const FGate& Gate = Course->Gates[Index];
	const double Pressed = PressedAt[Gate.Switch];
	if (Pressed < 0.0)
	{
		return false;
	}
	return Gate.OpenSeconds <= 0.0 || BallState.Time - Pressed <= Gate.OpenSeconds;
}

void FSim::Collide(const FBox& Box, FVec3 A, double H, bool bWasOnGround, ESurface& Ground, std::vector<FEvent>* Events)
{
	FBall& B = BallState;
	const FVec3 Q{Clamp(B.Position.X, Box.Min.X, Box.Max.X), Clamp(B.Position.Y, Box.Min.Y, Box.Max.Y),
		Clamp(B.Position.Z, Box.Min.Z, Box.Max.Z)};
	const FVec3 D = B.Position - Q;
	const double Dist2 = Dot(D, D);
	if (Dist2 >= B.Radius * B.Radius)
	{
		return;
	}

	FVec3 N;
	if (Dist2 > 0.0)
	{
		const double Dist = std::sqrt(Dist2);
		N = D * (1.0 / Dist);
		B.Position = Q + N * B.Radius;
	}
	else
	{
		// Center is inside the box: leave through the nearest face.
		const double Faces[6] = {B.Position.X - Box.Min.X, Box.Max.X - B.Position.X, B.Position.Y - Box.Min.Y,
			Box.Max.Y - B.Position.Y, B.Position.Z - Box.Min.Z, Box.Max.Z - B.Position.Z};
		int Best = 0;
		for (int I = 1; I < 6; ++I)
		{
			if (Faces[I] < Faces[Best])
			{
				Best = I;
			}
		}
		const FVec3 Normals[6] = {{-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}};
		N = Normals[Best];
		B.Position = B.Position + N * (Faces[Best] + B.Radius);
	}

	const FSurfaceProps Surface = PropsFor(Box.Surface);
	const double Vn = Dot(B.Velocity, N);
	FVec3 Tangent = B.Velocity - N * Vn;
	double NewVn = Vn > 0.0 ? Vn : 0.0;

	if (Vn < 0.0)
	{
		const double Impact = -Vn;
		if (Impact > Physics.BounceThreshold)
		{
			NewVn = Impact * Surface.Restitution;
			Tangent = Tangent * Surface.Grip;
			Emit(Events, EEventType::Bounce, Box.Surface, Impact, B.Position);
		}
		else if (!bWasOnGround && N.Z > 0.5)
		{
			Emit(Events, EEventType::Land, Box.Surface, Impact, B.Position);
		}
	}

	// Rolling resistance scaled by the load on the surface. Because it can zero the tangential speed
	// outright, it also holds the ball still on a surface a weak pull couldn't move it across.
	double Load = -Dot(A, N);
	if (Load > 0.0)
	{
		const double Speed = Length(Tangent);
		const double Decel = Surface.Rolling * Load * H;
		Tangent = Speed > Decel ? Tangent * ((Speed - Decel) / Speed) : FVec3{};
	}

	if (Surface.Drag > 0.0)
	{
		const double Keep = 1.0 - Surface.Drag * H;
		Tangent = Tangent * (Keep > 0.0 ? Keep : 0.0);
	}

	B.Velocity = Tangent + N * NewVn;
	if (N.Z > 0.5)
	{
		B.bOnGround = true;
		Ground = Box.Surface;
	}
}

void FSim::SubStep(double H, std::vector<FEvent>* Events)
{
	FBall& B = BallState;
	const FVec3 A = Acceleration(B.Position, B.Velocity, Events);
	B.Velocity = B.Velocity + A * H;
	B.Position = B.Position + B.Velocity * H;
	B.Time += H;

	const bool bWasOnGround = B.bOnGround;
	B.bOnGround = false;
	ESurface Ground = ESurface::Stone;

	for (const FBox& Box : Course->Boxes)
	{
		Collide(Box, A, H, bWasOnGround, Ground, Events);
	}

	// Switches: rolling over one presses it. Gates: solid unless their switch is holding them open.
	for (size_t I = 0; I < Course->Switches.size(); ++I)
	{
		const FSwitch& Switch = Course->Switches[I];
		const double Dx = B.Position.X - Switch.Center.X;
		const double Dy = B.Position.Y - Switch.Center.Y;
		const double Dz = B.Position.Z - Switch.Center.Z;
		if (Dx * Dx + Dy * Dy <= Switch.Radius * Switch.Radius && Dz > -B.Radius && Dz < 3.0 * B.Radius)
		{
			if (PressedAt[I] < 0.0)
			{
				Emit(Events, EEventType::SwitchPressed, ESurface::Stone, Length(B.Velocity), Switch.Center);
				if (Events)
				{
					Events->back().Index = static_cast<int>(I);
				}
			}
			PressedAt[I] = B.Time;
		}
	}
	for (size_t I = 0; I < Course->Gates.size(); ++I)
	{
		const bool bOpen = GateOpen(static_cast<int>(I));
		if (GateWasOpen[I] && !bOpen)
		{
			Emit(Events, EEventType::GateClosed, ESurface::Stone, 0.0, B.Position);
			if (Events)
			{
				Events->back().Index = static_cast<int>(I);
			}
		}
		GateWasOpen[I] = bOpen;
		if (!bOpen)
		{
			Collide(Course->Gates[I].Box, A, H, bWasOnGround, Ground, Events);
		}
	}

	// Cup: the ball drops when it rolls over the mouth slowly enough.
	const double CupDx = B.Position.X - Course->Cup.X;
	const double CupDy = B.Position.Y - Course->Cup.Y;
	const double CupDz = B.Position.Z - (Course->Cup.Z + B.Radius);
	const bool bOverCup = CupDx * CupDx + CupDy * CupDy < Course->CupRadius * Course->CupRadius &&
		CupDz < B.Radius && CupDz > -B.Radius;
	if (bOverCup)
	{
		const double Speed = Length(B.Velocity);
		if (Speed <= Course->CupCaptureSpeed)
		{
			B.State = EBallState::Sunk;
			B.Position = {Course->Cup.X, Course->Cup.Y, Course->Cup.Z - B.Radius};
			B.Velocity = {};
			Emit(Events, EEventType::Sunk, Ground, Speed, B.Position);
			return;
		}
		if (!bInCupZone)
		{
			Emit(Events, EEventType::LipOut, Ground, Speed, B.Position);
		}
	}
	bInCupZone = bOverCup;

	if (B.Position.Z < Course->KillZ)
	{
		B.State = EBallState::Lost;
		Emit(Events, EEventType::Lost, Ground, Length(B.Velocity), B.Position);
		return;
	}

	if (B.bOnGround && Length(B.Velocity) < Physics.RestSpeed)
	{
		B.SlowTime += H;
		if (B.SlowTime >= Physics.RestTime)
		{
			B.State = EBallState::AtRest;
			B.Velocity = {};
			for (int I = 0; I < static_cast<int>(Course->SafeZones.size()); ++I)
			{
				const FSafeZone& Zone = Course->SafeZones[I];
				const double Dx = B.Position.X - Zone.Center.X;
				const double Dy = B.Position.Y - Zone.Center.Y;
				const double Dz = B.Position.Z - Zone.Center.Z;
				if (Dx * Dx + Dy * Dy <= Zone.Radius * Zone.Radius && Dz > -B.Radius && Dz < 3.0 * B.Radius)
				{
					B.SafeZone = I;
					break;
				}
			}
			Emit(Events, EEventType::Rest, Ground, 0.0, B.Position);
			return;
		}
	}
	else
	{
		B.SlowTime = 0.0;
	}

	if (B.Time >= Physics.MaxFlightTime)
	{
		B.State = EBallState::TimedOut;
		Emit(Events, EEventType::TimedOut, Ground, Length(B.Velocity), B.Position);
	}
}

EBallState FSim::Run(std::vector<FEvent>* Events, int MaxSteps)
{
	for (int I = 0; I < MaxSteps && !Finished(); ++I)
	{
		Step(Events);
	}
	return BallState.State;
}

uint64_t FSim::Hash() const
{
	uint64_t H = 14695981039346656037ull;
	H = Mix(H, &BallState.Position, sizeof(FVec3));
	H = Mix(H, &BallState.Velocity, sizeof(FVec3));
	H = Mix(H, &BallState.Time, sizeof(double));
	H = Mix(H, &BallState.State, sizeof(EBallState));
	return H;
}
} // namespace GG
