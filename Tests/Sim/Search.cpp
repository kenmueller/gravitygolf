// Design aid: random search for placements of every machine plus a swing that completes a leg.
// Usage: Tests/Sim/search.sh <course id> [leg index, default 0] [samples in millions, default 3]
// Prints the placements it found, ranked by how many different swings work from each.
#include "CourseFile.h"
#include "Sim/GGSim.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

using namespace GG;

static bool Reaches(const FSetup& S, const FLeg& Leg, const std::vector<FAttractor>& Machines, double Yaw, double Power)
{
	const double R = Yaw * 3.14159265358979323846 / 180.0;
	return LegReached(S, Leg, Machines, {std::cos(R) * Power * 3000.0, std::sin(R) * Power * 3000.0, 0.0});
}

struct FFound
{
	std::vector<FVec3> Spots;
	double Yaw, Power;
	int Tolerance = 0;
};

int main(int Argc, char** Argv)
{
	FSetup S;
	if (Argc < 2 || !LoadCourse(Argv[1], S)) { std::printf("usage: search <file.course> [leg] [millions]\n"); return 2; }
	const FLeg Leg = MakeLeg(S, Argc > 2 ? std::atoi(Argv[2]) : 0);
	const long Samples = static_cast<long>((Argc > 3 ? std::atof(Argv[3]) : 3.0) * 1e6);

	std::vector<FFound> Found;
	std::mutex Lock;
	std::atomic<long> Done{0};
	const unsigned Threads = std::max(1u, std::thread::hardware_concurrency());
	std::vector<std::thread> Workers;
	for (unsigned T = 0; T < Threads; ++T)
	{
		Workers.emplace_back([&, T]() {
			std::mt19937_64 Rng(1234 + T);
			std::uniform_real_distribution<double> U(0.0, 1.0);
			while (Done++ < Samples)
			{
				std::vector<FAttractor> Machines;
				std::vector<FVec3> Spots;
				for (const FMachineDef& M : S.Machines)
				{
					double X, Y;
					do
					{
						X = std::round((S.MinX + U(Rng) * (S.MaxX - S.MinX)) / 50.0) * 50.0;
						Y = std::round((S.MinY + U(Rng) * (S.MaxY - S.MinY)) / 50.0) * 50.0;
					} while (!MachineAllowed(S, X, Y));
					Machines.push_back({{X, Y, M.CoreHeight}, M.Strength, M.Range});
					Spots.push_back({X, Y, 0});
				}
				const double Yaw = std::round(-180.0 + U(Rng) * 360.0);
				const double Power = std::round((0.1 + U(Rng) * 0.9) * 40.0) / 40.0;
				if (Reaches(S, Leg, Machines, Yaw, Power))
				{
					std::lock_guard<std::mutex> Guard(Lock);
					if (Found.size() < 400) { Found.push_back({Spots, Yaw, Power, 0}); }
				}
			}
		});
	}
	for (std::thread& W : Workers) { W.join(); }
	std::printf("%zu hits in %ld samples\n", Found.size(), Samples);

	// Rank what was found by how many swings work from each placement.
	std::atomic<size_t> Next{0};
	Workers.clear();
	for (unsigned T = 0; T < Threads; ++T)
	{
		Workers.emplace_back([&]() {
			for (size_t I = Next++; I < Found.size(); I = Next++)
			{
				std::vector<FAttractor> Machines;
				for (size_t M = 0; M < S.Machines.size(); ++M)
				{
					Machines.push_back({{Found[I].Spots[M].X, Found[I].Spots[M].Y, S.Machines[M].CoreHeight}, S.Machines[M].Strength, S.Machines[M].Range});
				}
				for (double Yaw = -180.0; Yaw < 180.0; Yaw += 1.0)
				{
					for (double Power = 0.05; Power <= 1.0001; Power += 0.025)
					{
						Found[I].Tolerance += Reaches(S, Leg, Machines, Yaw, Power);
					}
				}
			}
		});
	}
	for (std::thread& W : Workers) { W.join(); }
	std::sort(Found.begin(), Found.end(), [](const FFound& A, const FFound& B) { return A.Tolerance > B.Tolerance; });
	for (size_t I = 0; I < Found.size() && I < 12; ++I)
	{
		std::printf("tolerance %3d  swing yaw %6.1f power %.3f  machines:", Found[I].Tolerance, Found[I].Yaw, Found[I].Power);
		for (const FVec3& Spot : Found[I].Spots) { std::printf(" (%.0f, %.0f)", Spot.X, Spot.Y); }
		std::printf("\n");
	}
	return 0;
}
