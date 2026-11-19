// The puzzle law's building blocks, shared by the standalone solver (Tests/Sim) and the game's Workshop: a hole as
// the proof sees it, its legs, and "does this swing complete this leg?". Plain C++ like the simulation itself.

#pragma once

#include "Sim/GGSim.h"

#include <cmath>
#include <vector>

namespace GG
{
struct FMachineDef
{
	double Strength = 0, Range = 0, CoreHeight = 0, StartX = 0, StartY = 0;
	/** A mirror instead of a field: it can be turned two ways (see FAttractor::Turn). */
	bool bMirror = false;
};

/** A machine standing somewhere. Turn only matters to a mirror. */
inline FAttractor Stand(const FMachineDef& M, double X, double Y, int Turn = 0)
{
	return {{X, Y, M.CoreHeight}, M.Strength, M.Range, M.bMirror ? Turn : -1};
}

/** A recorded answer for one leg: where each machine stands, and one swing. */
struct FHint
{
	int Leg = 0;
	double Yaw = 0, Power = 0;
	std::vector<FVec3> Spots;
	/** Which way each machine is turned, for the mirrors among them. */
	std::vector<int> Turns;
};

struct FSetup
{
	FCourse Course;
	/** Where machines may be tried: the walkable ground. */
	double MinX = 0, MinY = 0, MaxX = 0, MaxY = 0;
	std::vector<FMachineDef> Machines;
	std::vector<FVec3> NoMachine; // x, y, radius
	/** Plinths (x, y, radius). When a hole has any, a machine may stand only on one, at its center. */
	std::vector<FVec3> Plinths;
	std::vector<FHint> Hints;
	/** Per safe zone: a switch that must be pressed on the leg that reaches it, or -1. */
	std::vector<int> ZoneNeeds;
};

/** A leg runs from Start to a target: safe zone Target, or the cup when Target is -1. */
struct FLeg
{
	FVec3 Start;
	int Target = -1;
	/** Switches banked on earlier legs. */
	std::vector<bool> Held;
};

inline FLeg MakeLeg(const FSetup& S, int Index)
{
	FLeg Leg;
	const int Zones = static_cast<int>(S.Course.SafeZones.size());
	Leg.Start = S.Course.Tee;
	Leg.Target = Index < Zones ? Index : -1;
	Leg.Held.assign(S.Course.Switches.size(), false);
	for (int Z = 0; Z < Index && Z < Zones; ++Z)
	{
		if (S.ZoneNeeds[Z] >= 0)
		{
			Leg.Held[S.ZoneNeeds[Z]] = true;
		}
	}
	if (Index > 0)
	{
		Leg.Start = S.Course.SafeZones[Index - 1].Center;
		Leg.Start.Z += S.Course.BallRadius;
	}
	return Leg;
}

/** The launch velocity of a swing. (Trigonometry is fine here: it happens once, outside the simulation's step.) */
inline FVec3 LaunchVelocity(double YawDegrees, double Power)
{
	const double R = YawDegrees * 3.14159265358979323846 / 180.0;
	return {std::cos(R) * Power * 3000.0, std::sin(R) * Power * 3000.0, 0.0};
}

/** Does this swing complete the leg? */
inline bool LegReached(const FSetup& S, const FLeg& Leg, const std::vector<FAttractor>& Machines, FVec3 Launch)
{
	FSim Sim;
	// Weight plates are down wherever a machine stands on one.
	std::vector<bool> Held = Leg.Held;
	HoldWeightPlates(S.Course, Machines, Held);
	Sim.Begin(S.Course, Machines, Leg.Start, Launch, {}, &Held);
	const EBallState End = Sim.Run();
	if (Leg.Target < 0)
	{
		return End == EBallState::Sunk;
	}
	const int Needs = S.ZoneNeeds[Leg.Target];
	return End == EBallState::AtRest && Sim.Ball().SafeZone == Leg.Target && (Needs < 0 || Sim.SwitchPressed(Needs));
}

/** May a machine stand here? Not inside walls or closed gates, not on courts or pads; only on a plinth where a
 *  hole has plinths. */
inline bool MachineAllowed(const FSetup& S, double X, double Y)
{
	if (!S.Plinths.empty())
	{
		bool bOnOne = false;
		for (const FVec3& Plinth : S.Plinths)
		{
			bOnOne = bOnOne || ((X - Plinth.X) * (X - Plinth.X) + (Y - Plinth.Y) * (Y - Plinth.Y) <= Plinth.Z * Plinth.Z);
		}
		if (!bOnOne)
		{
			return false;
		}
	}
	auto Blocks = [&](const FBox& B) {
		return B.Max.Z > 20.0 && X > B.Min.X - 40 && X < B.Max.X + 40 && Y > B.Min.Y - 40 && Y < B.Max.Y + 40;
	};
	for (const FBox& B : S.Course.Boxes)
	{
		if (Blocks(B))
		{
			return false;
		}
	}
	for (const FGate& G : S.Course.Gates)
	{
		if (Blocks(G.Box))
		{
			return false;
		}
	}
	for (const FVec3& Zone : S.NoMachine)
	{
		if ((X - Zone.X) * (X - Zone.X) + (Y - Zone.Y) * (Y - Zone.Y) < Zone.Z * Zone.Z)
		{
			return false;
		}
	}
	return true;
}
} // namespace GG
