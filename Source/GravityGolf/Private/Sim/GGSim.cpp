#include "Sim/GGSim.h"

#include <cmath>
#include <cstring>

// Fused multiply-add changes rounding, and compilers fuse differently per platform. Keep every operation
// separately rounded so a shot replays identically on Mac, Windows, phones and the WebAssembly referee.
#if defined(__clang__)
#pragma clang fp contract(off)
#elif defined(_MSC_VER)
#pragma fp_contract(off)
#endif

namespace GG
{
double Length(FVec2 A)
{
	return std::sqrt(A.X * A.X + A.Y * A.Y);
}

namespace
{
struct FSurfaceProps
{
	double Restitution;
	/** Rolling friction as a multiple of the gravity pressing the ball into the surface. */
	double Friction;
};

FSurfaceProps PropsFor(ESurface Surface)
{
	switch (Surface)
	{
	case ESurface::Ice:
		return {0.62, 0.02};
	case ESurface::Dust:
		return {0.08, 1.6};
	case ESurface::Hazard:
		return {0.0, 0.0};
	case ESurface::Grass:
	default:
		return {0.42, 0.32};
	}
}

void Emit(std::vector<FEvent>* Events, EEventType Type, int Index, double Speed, FVec2 Position)
{
	if (Events)
	{
		Events->push_back({Type, Index, Speed, Position});
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

FVec2 SurfacePoint(const FHole& Hole, int Planet, FVec2 Direction)
{
	const FPlanet& P = Hole.Planets[Planet];
	return P.Position + Direction * (P.Radius + Hole.BallRadius);
}

void FSim::Begin(const FHole& InHole, const std::vector<FWell>& InWells, FVec2 Start, FVec2 Launch,
	const FPhysics& InPhysics)
{
	Hole = &InHole;
	Wells = InWells;
	Physics = InPhysics;
	BallState = {};
	BallState.Position = Start;
	BallState.Velocity = Launch;
	BallState.Radius = InHole.BallRadius;
	Collected.assign(InHole.Stardust.size(), false);
	bAirborne = true;
	bInCupZone = false;
}

FVec2 FSim::Acceleration(FVec2 Position) const
{
	FVec2 Total;

	for (const FPlanet& P : Hole->Planets)
	{
		const FVec2 D = P.Position - Position;
		const double R2 = Dot(D, D);
		const double R = std::sqrt(R2);
		if (R <= 0.0)
		{
			continue;
		}
		const double Mu = P.SurfaceGravity * P.Radius * P.Radius;
		const double Magnitude = R > P.Radius ? Mu / R2 : P.SurfaceGravity;
		Total = Total + D * (Magnitude / R);
	}

	const double Soft2 = Physics.WellSoftening * Physics.WellSoftening;
	for (const FWell& W : Wells)
	{
		const FVec2 D = W.Position - Position;
		const double R2 = Dot(D, D) + Soft2;
		// Plummer softening: strength falls to zero at the center instead of blowing up.
		FVec2 A = D * (W.Strength / (R2 * std::sqrt(R2)));
		const double Magnitude = Length(A);
		if (Magnitude > Physics.WellMaxAccel)
		{
			A = A * (Physics.WellMaxAccel / Magnitude);
		}
		Total = Total + A;
	}

	return Total;
}

void FSim::Step(std::vector<FEvent>* Events)
{
	FBall& B = BallState;
	if (B.State != EBallState::Moving)
	{
		return;
	}

	const FVec2 A = Acceleration(B.Position);
	B.Velocity = B.Velocity + A * Dt;

	for (const FPlanet& P : Hole->Planets)
	{
		if (P.AtmosphereRadius > 0.0)
		{
			const FVec2 D = B.Position - P.Position;
			if (Dot(D, D) < P.AtmosphereRadius * P.AtmosphereRadius)
			{
				B.Velocity = B.Velocity * (1.0 - P.AtmosphereDrag * Dt);
			}
		}
	}

	B.Position = B.Position + B.Velocity * Dt;
	B.Time += Dt;

	const bool bWasAirborne = bAirborne;
	bool bCupZoneNow = false;
	B.Contact = -1;

	for (int I = 0; I < static_cast<int>(Hole->Planets.size()); ++I)
	{
		const FPlanet& P = Hole->Planets[I];
		const FVec2 D = B.Position - P.Position;
		const double Dist = Length(D);
		const double MinDist = P.Radius + B.Radius;
		if (Dist >= MinDist || Dist <= 0.0)
		{
			continue;
		}

		const FVec2 N = D * (1.0 / Dist);
		if (P.Surface == ESurface::Hazard)
		{
			B.State = EBallState::Hazard;
			Emit(Events, EEventType::Hazard, I, Length(B.Velocity), B.Position);
			return;
		}

		B.Position = P.Position + N * MinDist;
		const FSurfaceProps Surface = PropsFor(P.Surface);
		const FVec2 T{-N.Y, N.X};
		double Vn = Dot(B.Velocity, N);
		double Vt = Dot(B.Velocity, T);

		if (Vn < 0.0)
		{
			const double Impact = -Vn;
			if (Impact > Physics.BounceThreshold)
			{
				Vn = Impact * Surface.Restitution;
				Emit(Events, EEventType::Bounce, I, Impact, B.Position);
			}
			else
			{
				Vn = 0.0;
				if (bWasAirborne)
				{
					Emit(Events, EEventType::Land, I, Impact, B.Position);
				}
			}
		}

		// Kinetic friction scaled by how hard gravity presses the ball into the surface. Because it can zero
		// the tangential speed outright, it also acts as static friction on gentle slopes.
		double Load = -Dot(A, N);
		if (Load < 0.0)
		{
			Load = 0.0;
		}
		const double Decel = Surface.Friction * Load * Dt;
		if (Vt > Decel)
		{
			Vt -= Decel;
		}
		else if (Vt < -Decel)
		{
			Vt += Decel;
		}
		else
		{
			Vt = 0.0;
		}

		B.Velocity = N * Vn + T * Vt;
		B.Contact = I;

		if (P.bHasCup && Dot(N, P.CupDirection) >= P.CupCos)
		{
			bCupZoneNow = true;
			const double Speed = Length(B.Velocity);
			if (Speed <= Physics.CupCaptureSpeed)
			{
				B.State = EBallState::Sunk;
				B.Position = P.Position + P.CupDirection * P.Radius;
				B.Velocity = {};
				Emit(Events, EEventType::Sunk, I, Speed, B.Position);
				return;
			}
			if (!bInCupZone)
			{
				Emit(Events, EEventType::LipOut, I, Speed, B.Position);
			}
		}
	}

	bInCupZone = bCupZoneNow;
	bAirborne = B.Contact < 0;

	for (int I = 0; I < static_cast<int>(Hole->Stardust.size()); ++I)
	{
		if (Collected[I])
		{
			continue;
		}
		const FStardust& S = Hole->Stardust[I];
		const FVec2 D = B.Position - S.Position;
		const double Reach = S.Radius + B.Radius;
		if (Dot(D, D) <= Reach * Reach)
		{
			Collected[I] = true;
			Emit(Events, EEventType::Stardust, I, Length(B.Velocity), S.Position);
		}
	}

	if (Length(B.Position - Hole->BoundsCenter) > Hole->BoundsRadius)
	{
		B.State = EBallState::OutOfBounds;
		Emit(Events, EEventType::OutOfBounds, -1, Length(B.Velocity), B.Position);
		return;
	}

	if (B.Contact >= 0 && Length(B.Velocity) < Physics.RestSpeed)
	{
		B.SlowTime += Dt;
		if (B.SlowTime >= Physics.RestTime)
		{
			B.State = EBallState::AtRest;
			B.Velocity = {};
			Emit(Events, EEventType::Rest, B.Contact, 0.0, B.Position);
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
		Emit(Events, EEventType::TimedOut, -1, Length(B.Velocity), B.Position);
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
	H = Mix(H, &BallState.Position, sizeof(FVec2));
	H = Mix(H, &BallState.Velocity, sizeof(FVec2));
	H = Mix(H, &BallState.Time, sizeof(double));
	H = Mix(H, &BallState.State, sizeof(EBallState));
	for (bool C : Collected)
	{
		const unsigned char Byte = C ? 1 : 0;
		H = Mix(H, &Byte, 1);
	}
	return H;
}
} // namespace GG
