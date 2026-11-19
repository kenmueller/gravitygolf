// Design aid: where should the cup go? With the machines standing at given spots, plays every swing from the tee and
// counts, for each 50 cm square of ground, how many different swings roll over it slowly enough to drop into a cup.
// Squares many swings cross are forgiving places for a cup; squares none cross are impossible ones.
// Usage: Tests/Sim/cupscan.sh <course id> <min x> <ax> <ay> <bx> <by>     (only squares east of min x are counted)
#include "CourseFile.h"
#include "Sim/GGSim.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <vector>

using namespace GG;

// One machine, tried at spot A and at spot B: squares that many swings cross with it at A and none cross with it at B.
// That is where a cup makes A the answer and B a dead end.
static std::map<std::pair<int, int>, int> Scan(FSetup S, double X, double Y, double MinX)
{
	const std::vector<FAttractor> Machines = {{{X, Y, S.Machines[0].CoreHeight}, S.Machines[0].Strength, S.Machines[0].Range}};
	const double Capture = S.Course.CupCaptureSpeed;
	// The real cup is taken out of the way so it can't catch anything during the scan.
	S.Course.Cup = {1e7, 1e7, 0};
	const FLeg Leg = MakeLeg(S, 0);
	std::map<std::pair<int, int>, int> Count;
	for (double Yaw = -180.0; Yaw < 180.0; Yaw += 1.0)
	{
		for (double Power = 0.05; Power <= 1.0001; Power += 0.025)
		{
			FSim Sim;
			std::vector<bool> Held = Leg.Held;
			HoldWeightPlates(S.Course, Machines, Held);
			Sim.Begin(S.Course, Machines, Leg.Start, LaunchVelocity(Yaw, Power), {}, &Held);
			std::set<std::pair<int, int>> Seen;
			while (!Sim.Finished())
			{
				Sim.Step();
				const FBall& B = Sim.Ball();
				if (B.Position.X > MinX && B.bOnGround && Length(B.Velocity) <= Capture && Length(B.Velocity) > 20.0)
				{
					Seen.insert({static_cast<int>(std::floor(B.Position.X / 50.0)), static_cast<int>(std::floor(B.Position.Y / 50.0))});
				}
			}
			for (const auto& Cell : Seen)
			{
				++Count[Cell];
			}
		}
	}
	return Count;
}

int main(int Argc, char** Argv)
{
	FSetup S;
	if (Argc < 7 || !LoadCourse(Argv[1], S) || S.Machines.empty())
	{
		std::printf("usage: cupscan <file.course> <min x> <ax> <ay> <bx> <by>\n");
		return 2;
	}
	const double MinX = std::atof(Argv[2]);
	// How many spots to list for each (an optional last argument).
	const size_t Limit = Argc > 7 ? static_cast<size_t>(std::atoi(Argv[7])) : 14;
	const auto A = Scan(S, std::atof(Argv[3]), std::atof(Argv[4]), MinX);
	const auto B = Scan(S, std::atof(Argv[5]), std::atof(Argv[6]), MinX);
	for (int Pass = 0; Pass < 2; ++Pass)
	{
		const auto& Good = Pass == 0 ? A : B;
		const auto& Bad = Pass == 0 ? B : A;
		std::vector<std::pair<int, std::pair<int, int>>> Ranked;
		for (const auto& Entry : Good)
		{
			// Nothing may cross the square or its neighbours from the other spot: a cup is wider than a square.
			bool bClear = true;
			for (int DX = -1; DX <= 1 && bClear; ++DX)
			{
				for (int DY = -1; DY <= 1 && bClear; ++DY)
				{
					bClear = Bad.find({Entry.first.first + DX, Entry.first.second + DY}) == Bad.end();
				}
			}
			if (bClear)
			{
				Ranked.push_back({Entry.second, Entry.first});
			}
		}
		std::sort(Ranked.rbegin(), Ranked.rend());
		std::printf("Cup spots that only the machine at %s serves (of %zu it reaches):\n", Pass == 0 ? "A" : "B", Good.size());
		for (size_t I = 0; I < Ranked.size() && I < Limit; ++I)
		{
			std::printf("  (%5d, %5d): %d swings\n", Ranked[I].second.first * 50 + 25, Ranked[I].second.second * 50 + 25, Ranked[I].first);
		}
	}
	return 0;
}
