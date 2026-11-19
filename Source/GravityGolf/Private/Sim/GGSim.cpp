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
};

FSurfaceProps PropsFor(ESurface Surface)
{
	switch (Surface)
	{
	case ESurface::Green:
		return {0.3, 0.09};
	case ESurface::Stone:
		return {0.55, 0.06};
	case ESurface::Sand:
		return {0.05, 0.9};
	case ESurface::Ice:
		return {0.5, 0.01};
	case ESurface::Grass:
	default:
		return {0.35, 0.25};
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
		Events->push_back({Type, Surface, Speed, Position});
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
	const FPhysics& InPhysics)
{
	Course = &InCourse;
	Attractors = InAttractors;
	Physics = InPhysics;
	BallState = {};
	BallState.Position = Start;
	BallState.Velocity = Launch;
	BallState.Radius = InCourse.BallRadius;
	bInCupZone = false;
}

FVec3 FSim::Acceleration(FVec3 Position) const
{
	FVec3 Total = Course->Gravity;
	const double Soft2 = Physics.AttractorSoftening * Physics.AttractorSoftening;
	for (const FAttractor& A : Attractors)
	{
		const FVec3 D = A.Position - Position;
		const double R2 = Dot(D, D);
		const double Range2 = A.Range * A.Range;
		if (R2 >= Range2)
		{
			continue;
		}
		// Plummer-softened inverse square, windowed so the pull reaches exactly zero at Range.
		const double Window = 1.0 - R2 / Range2;
		const double S2 = R2 + Soft2;
		Total = Total + D * (A.Strength * Window * Window / (S2 * std::sqrt(S2)));
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

void FSim::SubStep(double H, std::vector<FEvent>* Events)
{
	FBall& B = BallState;
	const FVec3 A = Acceleration(B.Position);
	B.Velocity = B.Velocity + A * H;
	B.Position = B.Position + B.Velocity * H;
	B.Time += H;

	const bool bWasOnGround = B.bOnGround;
	B.bOnGround = false;
	ESurface Ground = ESurface::Stone;

	for (const FBox& Box : Course->Boxes)
	{
		const FVec3 Q{Clamp(B.Position.X, Box.Min.X, Box.Max.X), Clamp(B.Position.Y, Box.Min.Y, Box.Max.Y),
			Clamp(B.Position.Z, Box.Min.Z, Box.Max.Z)};
		const FVec3 D = B.Position - Q;
		const double Dist2 = Dot(D, D);
		if (Dist2 >= B.Radius * B.Radius)
		{
			continue;
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
				Tangent = Tangent * Physics.BounceGrip;
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

		B.Velocity = Tangent + N * NewVn;
		if (N.Z > 0.5)
		{
			B.bOnGround = true;
			Ground = Box.Surface;
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
		if (Speed <= Physics.CupCaptureSpeed)
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
