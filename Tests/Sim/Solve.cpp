// Proves the puzzle law for a course with the real physics:
//   1. NECESSARY: with no machine, no swing sinks the ball.
//   2. SOLVABLE:  with the machine placed well, a comfortable range of swings sinks it.
// Usage: Tests/Sim/solve.sh <course id>      (reads Saved/SimTest/<id>.course, written by Tools/courses/<id>.py)
#include "Sim/GGSim.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <thread>
#include <vector>

using namespace GG;

struct FSetup
{
	FCourse Course;
	double Strength = 0, Range = 0, CoreHeight = 0;
	double MinX = 0, MinY = 0, MaxX = 0, MaxY = 0;
};

static bool Load(const char* Path, FSetup& S)
{
	std::ifstream In(Path);
	if (!In)
	{
		return false;
	}
	FCourse& C = S.Course;
	In >> C.Tee.X >> C.Tee.Y >> C.Tee.Z >> C.Cup.X >> C.Cup.Y >> C.Cup.Z >> C.CupRadius >> C.BallRadius >> C.KillZ;
	In >> S.Strength >> S.Range >> S.CoreHeight >> S.MinX >> S.MinY >> S.MaxX >> S.MaxY;
	int Count = 0;
	In >> Count;
	for (int I = 0; I < Count; ++I)
	{
		FBox B;
		int Surface = 0;
		In >> B.Min.X >> B.Min.Y >> B.Min.Z >> B.Max.X >> B.Max.Y >> B.Max.Z >> Surface;
		B.Surface = static_cast<ESurface>(Surface);
		C.Boxes.push_back(B);
	}
	return static_cast<bool>(In);
}

static FVec3 Launch(double YawDegrees, double Power)
{
	const double R = YawDegrees * 3.14159265358979323846 / 180.0;
	return {std::cos(R) * Power * 3000.0, std::sin(R) * Power * 3000.0, 0.0};
}

/** Counts the swings that sink the ball over a yaw/power grid. */
static int CountSolutions(const FSetup& S, const std::vector<FAttractor>& Machines, double YawStep, double PowerStep,
	double* BestYaw = nullptr, double* BestPower = nullptr)
{
	int Sunk = 0;
	for (double Yaw = -180.0; Yaw < 180.0; Yaw += YawStep)
	{
		for (double Power = 0.05; Power <= 1.0001; Power += PowerStep)
		{
			FSim Sim;
			Sim.Begin(S.Course, Machines, S.Course.Tee, Launch(Yaw, Power));
			if (Sim.Run() == EBallState::Sunk)
			{
				++Sunk;
				if (BestYaw)
				{
					*BestYaw = Yaw;
					*BestPower = Power;
				}
			}
		}
	}
	return Sunk;
}

static bool InsideAnyBox(const FCourse& C, double X, double Y)
{
	for (const FBox& B : C.Boxes)
	{
		if (B.Max.Z > 20.0 && X > B.Min.X - 40 && X < B.Max.X + 40 && Y > B.Min.Y - 40 && Y < B.Max.Y + 40)
		{
			return true;
		}
	}
	return false;
}

int main(int Argc, char** Argv)
{
	FSetup S;
	if (Argc < 2 || !Load(Argv[1], S))
	{
		std::printf("usage: solve <file.course>\n");
		return 2;
	}

	// 1. Necessity: a fine sweep with no machine at all.
	const int Bare = CountSolutions(S, {}, 0.25, 0.0125);
	std::printf("NECESSARY: %d of %d swings sink the ball with no machine  ->  %s\n", Bare, 1440 * 77,
		Bare == 0 ? "ok" : "FAIL (solvable without the machine)");

	// 2. Solvability: try the machine on a grid of spots; for each, count the swings that work.
	struct FSpot
	{
		double X, Y;
		int Solutions;
		double Yaw, Power;
	};
	std::vector<FSpot> Spots;
	for (double X = S.MinX; X <= S.MaxX; X += 150.0)
	{
		for (double Y = S.MinY; Y <= S.MaxY; Y += 150.0)
		{
			if (!InsideAnyBox(S.Course, X, Y))
			{
				Spots.push_back({X, Y, 0, 0, 0});
			}
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
				const std::vector<FAttractor> Machines = {{{Spot.X, Spot.Y, S.CoreHeight}, S.Strength, S.Range}};
				Spot.Solutions = CountSolutions(S, Machines, 1.0, 0.025, &Spot.Yaw, &Spot.Power);
			}
		});
	}
	for (std::thread& Worker : Workers)
	{
		Worker.join();
	}

	std::sort(Spots.begin(), Spots.end(), [](const FSpot& A, const FSpot& B) { return A.Solutions > B.Solutions; });
	int Working = 0;
	for (const FSpot& Spot : Spots)
	{
		Working += Spot.Solutions > 0;
	}
	std::printf("SOLVABLE: %d of %zu machine spots allow a hole-out\n", Working, Spots.size());
	for (size_t I = 0; I < Spots.size() && I < 8 && Spots[I].Solutions > 0; ++I)
	{
		std::printf("  machine at (%5.0f, %5.0f): %3d swings work, e.g. yaw %6.1f power %.3f\n", Spots[I].X, Spots[I].Y,
			Spots[I].Solutions, Spots[I].Yaw, Spots[I].Power);
	}

	// A good puzzle: some spots work, most don't, and the best spot forgives imperfect swings.
	const int Best = Spots.empty() ? 0 : Spots[0].Solutions;
	const double Share = Spots.empty() ? 0.0 : static_cast<double>(Working) / Spots.size();
	const bool bForgiving = Best >= 12;
	const bool bNotEverywhere = Share < 0.35;
	std::printf("  best spot tolerance: %d swings (need 12+)  ->  %s\n", Best, bForgiving ? "ok" : "FAIL (too fiddly)");
	std::printf("  share of spots that work: %.0f%% (need under 35%%)  ->  %s\n", Share * 100.0,
		bNotEverywhere ? "ok" : "FAIL (placement doesn't matter)");

	const bool bPass = Bare == 0 && Working > 0 && bForgiving && bNotEverywhere;
	std::printf("%s\n", bPass ? "PASS" : "FAIL");
	return bPass ? 0 : 1;
}
