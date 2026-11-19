// Reads Saved/SimTest/<id>.course (written by Tools/courses/coursekit.py) for the standalone solver and search.
#pragma once

#include "Sim/GGSim.h"

#include <fstream>
#include <vector>

struct FMachineDef
{
	double Strength = 0, Range = 0, CoreHeight = 0, StartX = 0, StartY = 0;
};

struct FHint
{
	int Leg = 0;
	double Yaw = 0, Power = 0;
	std::vector<GG::FVec3> Spots;
};

struct FSetup
{
	GG::FCourse Course;
	double MinX = 0, MinY = 0, MaxX = 0, MaxY = 0;
	std::vector<FMachineDef> Machines;
	std::vector<GG::FVec3> NoMachine; // x, y, radius
	std::vector<FHint> Hints;
	/** Per safe zone: a switch that must be pressed on the leg that reaches it, or -1. */
	std::vector<int> ZoneNeeds;
};

inline bool LoadCourse(const char* Path, FSetup& S)
{
	using namespace GG;
	std::ifstream In(Path);
	if (!In)
	{
		return false;
	}
	FCourse& C = S.Course;
	In >> C.Tee.X >> C.Tee.Y >> C.Tee.Z >> C.Cup.X >> C.Cup.Y >> C.Cup.Z >> C.CupRadius >> C.CupCaptureSpeed;
	In >> C.BallRadius >> C.KillZ >> S.MinX >> S.MinY >> S.MaxX >> S.MaxY;
	int Count = 0;
	In >> Count;
	for (int I = 0; I < Count; ++I)
	{
		FMachineDef M;
		In >> M.Strength >> M.Range >> M.CoreHeight >> M.StartX >> M.StartY;
		S.Machines.push_back(M);
	}
	In >> Count;
	for (int I = 0; I < Count; ++I)
	{
		FVec3 Zone;
		In >> Zone.X >> Zone.Y >> Zone.Z;
		S.NoMachine.push_back(Zone);
	}
	In >> Count;
	for (int I = 0; I < Count; ++I)
	{
		FSafeZone Zone;
		int Needs = -1;
		In >> Zone.Center.X >> Zone.Center.Y >> Zone.Center.Z >> Zone.Radius >> Needs;
		C.SafeZones.push_back(Zone);
		S.ZoneNeeds.push_back(Needs);
	}
	In >> Count;
	for (int I = 0; I < Count; ++I)
	{
		FBox B;
		int Surface = 0;
		In >> B.Min.X >> B.Min.Y >> B.Min.Z >> B.Max.X >> B.Max.Y >> B.Max.Z >> Surface;
		B.Surface = static_cast<ESurface>(Surface);
		C.Boxes.push_back(B);
	}
	In >> Count;
	for (int I = 0; I < Count; ++I)
	{
		FSwitch Sw;
		int Weight = 0;
		In >> Sw.Center.X >> Sw.Center.Y >> Sw.Center.Z >> Sw.Radius >> Weight;
		Sw.bWeight = Weight != 0;
		C.Switches.push_back(Sw);
	}
	In >> Count;
	for (int I = 0; I < Count; ++I)
	{
		FGate G;
		int Surface = 0;
		In >> G.Box.Min.X >> G.Box.Min.Y >> G.Box.Min.Z >> G.Box.Max.X >> G.Box.Max.Y >> G.Box.Max.Z >> Surface >> G.Switch >> G.OpenSeconds;
		G.Box.Surface = static_cast<ESurface>(Surface);
		C.Gates.push_back(G);
	}
	In >> Count;
	for (int I = 0; I < Count; ++I)
	{
		FHint H;
		In >> H.Leg >> H.Yaw >> H.Power;
		for (size_t M = 0; M < S.Machines.size(); ++M)
		{
			FVec3 Spot;
			In >> Spot.X >> Spot.Y;
			H.Spots.push_back(Spot);
		}
		S.Hints.push_back(H);
	}
	return static_cast<bool>(In);
}

/** A leg runs from Start to a target: safe zone Target, or the cup when Target is -1. */
struct FLeg
{
	GG::FVec3 Start;
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

/** Does this swing complete the leg? */
inline bool LegReached(const FSetup& S, const FLeg& Leg, const std::vector<GG::FAttractor>& Machines, GG::FVec3 Launch)
{
	GG::FSim Sim;
	// Weight plates are down wherever a machine stands on one.
	std::vector<bool> Held = Leg.Held;
	GG::HoldWeightPlates(S.Course, Machines, Held);
	Sim.Begin(S.Course, Machines, Leg.Start, Launch, {}, &Held);
	const GG::EBallState End = Sim.Run();
	if (Leg.Target < 0)
	{
		return End == GG::EBallState::Sunk;
	}
	const int Needs = S.ZoneNeeds[Leg.Target];
	return End == GG::EBallState::AtRest && Sim.Ball().SafeZone == Leg.Target && (Needs < 0 || Sim.SwitchPressed(Needs));
}

/** May a machine stand here? Not inside walls or closed gates, not on courts or pads. */
inline bool MachineAllowed(const FSetup& S, double X, double Y)
{
	auto Blocks = [&](const GG::FBox& B) {
		return B.Max.Z > 20.0 && X > B.Min.X - 40 && X < B.Max.X + 40 && Y > B.Min.Y - 40 && Y < B.Max.Y + 40;
	};
	for (const GG::FBox& B : S.Course.Boxes)
	{
		if (Blocks(B))
		{
			return false;
		}
	}
	for (const GG::FGate& G : S.Course.Gates)
	{
		if (Blocks(G.Box))
		{
			return false;
		}
	}
	for (const GG::FVec3& Zone : S.NoMachine)
	{
		if ((X - Zone.X) * (X - Zone.X) + (Y - Zone.Y) * (Y - Zone.Y) < Zone.Z * Zone.Z)
		{
			return false;
		}
	}
	return true;
}
