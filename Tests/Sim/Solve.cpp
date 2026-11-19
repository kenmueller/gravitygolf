// Proves the puzzle law for a course with the real physics. For every leg (tee -> safe zones -> cup):
//   1. NECESSARY: the leg can't be done with no machine, nor (when a hole has several) with any single machine.
//   2. SOLVABLE:  with the machines placed well, a comfortable range of swings does it, and most placements don't.
// Usage: Tests/Sim/solve.sh <course id>      (reads Saved/SimTest/<id>.course, written by Tools/courses/<id>.py)
#include "CourseFile.h"
#include "Sim/GGSim.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

using namespace GG;

/** Set by a second argument "all": list every machine spot that works, not only the best five (a design aid). */
static bool GListAll = false;

static FVec3 Launch(double YawDegrees, double Power)
{
	const double R = YawDegrees * 3.14159265358979323846 / 180.0;
	return {std::cos(R) * Power * 3000.0, std::sin(R) * Power * 3000.0, 0.0};
}

static bool Reaches(const FSetup& S, const FLeg& Leg, const std::vector<FAttractor>& Machines, double Yaw, double Power)
{
	return LegReached(S, Leg, Machines, Launch(Yaw, Power));
}

/** Counts the swings that complete the leg over a yaw/power grid. */
static int CountSolutions(const FSetup& S, const FLeg& Leg, const std::vector<FAttractor>& Machines, double YawStep,
	double PowerStep, double* BestYaw = nullptr, double* BestPower = nullptr)
{
	int Count = 0;
	for (double Yaw = -180.0; Yaw < 180.0; Yaw += YawStep)
	{
		for (double Power = 0.05; Power <= 1.0001; Power += PowerStep)
		{
			if (Reaches(S, Leg, Machines, Yaw, Power))
			{
				++Count;
				if (BestYaw)
				{
					*BestYaw = Yaw;
					*BestPower = Power;
				}
			}
		}
	}
	return Count;
}

struct FSpot
{
	double X, Y;
	int Solutions;
	double Yaw, Power;
};

/** Tries one machine on a grid of spots; fills in how many swings work at each. Returns the sorted spots. */
static std::vector<FSpot> SweepSpots(const FSetup& S, const FLeg& Leg, const FMachineDef& M, double YawStep, double PowerStep)
{
	std::vector<FSpot> Spots;
	for (double X = S.MinX; X <= S.MaxX; X += 150.0)
	{
		for (double Y = S.MinY; Y <= S.MaxY; Y += 150.0)
		{
			if (MachineAllowed(S, X, Y))
			{
				Spots.push_back({X, Y, 0, 0, 0});
			}
		}
	}
	// Where a hole has plinths, those are the only spots.
	if (!S.Plinths.empty())
	{
		Spots.clear();
		for (const FVec3& Plinth : S.Plinths)
		{
			if (MachineAllowed(S, Plinth.X, Plinth.Y))
			{
				Spots.push_back({Plinth.X, Plinth.Y, 0, 0, 0});
			}
		}
	}
	// A machine can also stand on a weight plate, which the grid may step over.
	for (const FSwitch& Switch : S.Course.Switches)
	{
		// (Where a hole has plinths, a plate a machine can stand on has a plinth of its own, already listed.)
		if (Switch.bWeight && S.Plinths.empty() && MachineAllowed(S, Switch.Center.X, Switch.Center.Y))
		{
			Spots.push_back({Switch.Center.X, Switch.Center.Y, 0, 0, 0});
		}
	}
	std::atomic<size_t> Next{0};
	std::vector<std::thread> Workers;
	for (unsigned T = 0; T < std::max(1u, std::thread::hardware_concurrency()); ++T)
	{
		Workers.emplace_back([&]() {
			for (size_t I = Next++; I < Spots.size(); I = Next++)
			{
				FSpot& Spot = Spots[I];
				const std::vector<FAttractor> Machines = {{{Spot.X, Spot.Y, M.CoreHeight}, M.Strength, M.Range}};
				Spot.Solutions = CountSolutions(S, Leg, Machines, YawStep, PowerStep, &Spot.Yaw, &Spot.Power);
			}
		});
	}
	for (std::thread& Worker : Workers)
	{
		Worker.join();
	}
	std::sort(Spots.begin(), Spots.end(), [](const FSpot& A, const FSpot& B) { return A.Solutions > B.Solutions; });
	return Spots;
}

static bool SolveLeg(const FSetup& S, const FLeg& Leg, int LegIndex, const char* Name)
{
	std::printf("LEG %s\n", Name);
	bool bPass = true;

	const int Bare = CountSolutions(S, Leg, {}, 0.25, 0.0125);
	std::printf("  NECESSARY: %d of %d swings succeed with no machine  ->  %s\n", Bare, 1440 * 77,
		Bare == 0 ? "ok" : "FAIL (solvable without any machine)");
	bPass &= Bare == 0;

	if (S.Machines.size() == 1)
	{
		const std::vector<FSpot> Spots = SweepSpots(S, Leg, S.Machines[0], 1.0, 0.025);
		int Working = 0;
		for (const FSpot& Spot : Spots)
		{
			Working += Spot.Solutions > 0;
		}
		std::printf("  SOLVABLE: %d of %zu machine spots work\n", Working, Spots.size());
		for (size_t I = 0; I < Spots.size() && I < (GListAll ? Spots.size() : 5) && Spots[I].Solutions > 0; ++I)
		{
			std::printf("    machine at (%5.0f, %5.0f): %3d swings work, e.g. yaw %6.1f power %.3f\n", Spots[I].X, Spots[I].Y,
				Spots[I].Solutions, Spots[I].Yaw, Spots[I].Power);
		}
		// A good puzzle: some spots work, most don't, and the best spot forgives imperfect swings.
		const int Best = Spots.empty() ? 0 : Spots[0].Solutions;
		const double Share = Spots.empty() ? 0.0 : static_cast<double>(Working) / Spots.size();
		std::printf("    best spot tolerance: %d swings (need 12+)  ->  %s\n", Best, Best >= 12 ? "ok" : "FAIL (too fiddly)");
		std::printf("    share of spots that work: %.0f%% (need under 35%%)  ->  %s\n", Share * 100.0,
			Share < 0.35 ? "ok" : "FAIL (placement doesn't matter)");
		return bPass && Working > 0 && Best >= 12 && Share < 0.35;
	}

	// Several machines: each one alone must fail, and the designer's intended answer must work with room to spare.
	for (size_t M = 0; M < S.Machines.size(); ++M)
	{
		const std::vector<FSpot> Spots = SweepSpots(S, Leg, S.Machines[M], 2.0, 0.05);
		int Working = 0;
		for (const FSpot& Spot : Spots)
		{
			Working += Spot.Solutions > 0;
		}
		std::printf("  NECESSARY: machine %zu alone works from %d of %zu spots  ->  %s\n", M + 1, Working, Spots.size(),
			Working == 0 ? "ok" : "FAIL (one machine is enough)");
		if (Working > 0)
		{
			std::printf("    e.g. at (%.0f, %.0f) yaw %.1f power %.3f\n", Spots[0].X, Spots[0].Y, Spots[0].Yaw, Spots[0].Power);
		}
		bPass &= Working == 0;
	}

	const FHint* Hint = nullptr;
	for (const FHint& H : S.Hints)
	{
		if (H.Leg == LegIndex)
		{
			Hint = &H;
		}
	}
	if (!Hint)
	{
		std::printf("  SOLVABLE: FAIL (no intended solution given for this leg)\n");
		return false;
	}
	std::vector<FAttractor> Machines;
	for (size_t M = 0; M < S.Machines.size(); ++M)
	{
		Machines.push_back({{Hint->Spots[M].X, Hint->Spots[M].Y, S.Machines[M].CoreHeight}, S.Machines[M].Strength, S.Machines[M].Range});
	}
	const bool bExact = Reaches(S, Leg, Machines, Hint->Yaw, Hint->Power);
	double Yaw = 0, Power = 0;
	const int Swings = CountSolutions(S, Leg, Machines, 1.0, 0.025, &Yaw, &Power);
	// Nudge each machine a pace in four directions: the answer shouldn't depend on centimeters.
	int Robust = 0;
	int Trials = 0;
	for (size_t M = 0; M < Machines.size(); ++M)
	{
		for (int Dir = 0; Dir < 4; ++Dir)
		{
			std::vector<FAttractor> Nudged = Machines;
			Nudged[M].Position.X += (Dir == 0 ? 75.0 : Dir == 1 ? -75.0 : 0.0);
			Nudged[M].Position.Y += (Dir == 2 ? 75.0 : Dir == 3 ? -75.0 : 0.0);
			++Trials;
			Robust += CountSolutions(S, Leg, Nudged, 1.0, 0.025) > 0;
		}
	}
	std::printf("  SOLVABLE: intended answer %s; %d swings work there (need 12+); %d of %d nudged placements still work (need half)\n",
		bExact ? "works" : "DOES NOT WORK", Swings, Robust, Trials);
	if (!bExact && Swings > 0)
	{
		std::printf("    a swing that does work: yaw %.1f power %.3f\n", Yaw, Power);
	}
	return bPass && Swings >= 12 && Robust * 2 >= Trials;
}

int main(int Argc, char** Argv)
{
	FSetup S;
	if (Argc < 2 || !LoadCourse(Argv[1], S))
	{
		std::printf("usage: solve <file.course>\n");
		return 2;
	}

	GListAll = Argc > 2 && std::string(Argv[2]) == "all";
	// Legs run tee -> safe zone 0 -> safe zone 1 -> ... -> cup. Each must obey the puzzle law on its own.
	bool bPass = true;
	const int Zones = static_cast<int>(S.Course.SafeZones.size());
	for (int Leg = 0; Leg <= Zones; ++Leg)
	{
		char Name[96];
		if (Leg < Zones)
		{
			std::snprintf(Name, sizeof(Name), "%d: to safe pad %d%s", Leg + 1, Leg + 1,
				S.ZoneNeeds[Leg] >= 0 ? " (pressing its switch on the way)" : "");
		}
		else
		{
			std::snprintf(Name, sizeof(Name), "%d: to the cup", Leg + 1);
		}
		bPass &= SolveLeg(S, MakeLeg(S, Leg), Leg, Name);
	}
	std::printf("%s\n", bPass ? "PASS" : "FAIL");
	return bPass ? 0 : 1;
}
