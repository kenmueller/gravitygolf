// Reads Saved/SimTest/<id>.course (written by Tools/courses/coursekit.py) for the standalone solver and search.
#pragma once

#include "Sim/GGProof.h"
#include "Sim/GGSim.h"

#include <fstream>
#include <vector>

using GG::FHint;
using GG::FLeg;
using GG::FMachineDef;
using GG::FSetup;
using GG::LegReached;
using GG::MachineAllowed;
using GG::MakeLeg;
using GG::Stand;

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
		int Mirror = 0;
		In >> M.Strength >> M.Range >> M.CoreHeight >> M.StartX >> M.StartY >> Mirror;
		M.bMirror = Mirror != 0;
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
		int Surface = 0, Grille = 0;
		In >> G.Box.Min.X >> G.Box.Min.Y >> G.Box.Min.Z >> G.Box.Max.X >> G.Box.Max.Y >> G.Box.Max.Z >> Surface >> G.Switch >> G.OpenSeconds >> Grille;
		G.Box.Surface = static_cast<ESurface>(Surface);
		G.bGrille = Grille != 0;
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
		for (size_t M = 0; M < S.Machines.size(); ++M)
		{
			int Turn = 0;
			In >> Turn;
			H.Turns.push_back(Turn);
		}
		S.Hints.push_back(H);
	}
	const bool bRead = static_cast<bool>(In);
	// Plinths come last, and older files have none.
	if (In >> Count)
	{
		for (int I = 0; I < Count; ++I)
		{
			FVec3 Plinth;
			In >> Plinth.X >> Plinth.Y >> Plinth.Z;
			S.Plinths.push_back(Plinth);
		}
		// Then whether the hole was drawn in the Workshop (a carried machine stays on its ground).
		int Drawn = 0;
		if (In >> Drawn)
		{
			S.bGroundOnly = Drawn != 0;
		}
	}
	return bRead;
}
