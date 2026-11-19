// Design aid: random search for placements of every machine plus a swing that completes a leg.
// Usage: Tests/Sim/search.sh <course id> [leg index, default 0] [samples in millions, default 3]
// Prints the placements it found, ranked by how many different swings work from each.
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

struct FMachineDef { double Strength, Range, CoreHeight, StartX, StartY; };
struct FSetup
{
	FCourse Course;
	double MinX, MinY, MaxX, MaxY;
	std::vector<FMachineDef> Machines;
	std::vector<FVec3> NoMachine;
};

static bool Load(const char* Path, FSetup& S)
{
	std::ifstream In(Path);
	if (!In) { return false; }
	FCourse& C = S.Course;
	In >> C.Tee.X >> C.Tee.Y >> C.Tee.Z >> C.Cup.X >> C.Cup.Y >> C.Cup.Z >> C.CupRadius >> C.CupCaptureSpeed;
	In >> C.BallRadius >> C.KillZ >> S.MinX >> S.MinY >> S.MaxX >> S.MaxY;
	int N = 0;
	In >> N;
	for (int I = 0; I < N; ++I) { FMachineDef M; In >> M.Strength >> M.Range >> M.CoreHeight >> M.StartX >> M.StartY; S.Machines.push_back(M); }
	In >> N;
	for (int I = 0; I < N; ++I) { FVec3 Z; In >> Z.X >> Z.Y >> Z.Z; S.NoMachine.push_back(Z); }
	In >> N;
	for (int I = 0; I < N; ++I) { FSafeZone Z; In >> Z.Center.X >> Z.Center.Y >> Z.Center.Z >> Z.Radius; C.SafeZones.push_back(Z); }
	In >> N;
	for (int I = 0; I < N; ++I) { FBox B; int Surface; In >> B.Min.X >> B.Min.Y >> B.Min.Z >> B.Max.X >> B.Max.Y >> B.Max.Z >> Surface; B.Surface = static_cast<ESurface>(Surface); C.Boxes.push_back(B); }
	return static_cast<bool>(In);
}

static bool Allowed(const FSetup& S, double X, double Y)
{
	for (const FBox& B : S.Course.Boxes)
	{
		if (B.Max.Z > 20.0 && X > B.Min.X - 40 && X < B.Max.X + 40 && Y > B.Min.Y - 40 && Y < B.Max.Y + 40) { return false; }
	}
	for (const FVec3& Z : S.NoMachine)
	{
		if ((X - Z.X) * (X - Z.X) + (Y - Z.Y) * (Y - Z.Y) < Z.Z * Z.Z) { return false; }
	}
	return true;
}

static bool Reaches(const FSetup& S, FVec3 Start, int Target, const std::vector<FAttractor>& Machines, double Yaw, double Power)
{
	const double R = Yaw * 3.14159265358979323846 / 180.0;
	FSim Sim;
	Sim.Begin(S.Course, Machines, Start, {std::cos(R) * Power * 3000.0, std::sin(R) * Power * 3000.0, 0.0});
	const EBallState End = Sim.Run();
	return Target < 0 ? End == EBallState::Sunk : (End == EBallState::AtRest && Sim.Ball().SafeZone == Target);
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
	if (Argc < 2 || !Load(Argv[1], S)) { std::printf("usage: search <file.course> [leg] [millions]\n"); return 2; }
	const int Leg = Argc > 2 ? std::atoi(Argv[2]) : 0;
	const long Samples = static_cast<long>((Argc > 3 ? std::atof(Argv[3]) : 3.0) * 1e6);
	const int Zones = static_cast<int>(S.Course.SafeZones.size());
	FVec3 Start = S.Course.Tee;
	if (Leg > 0) { Start = S.Course.SafeZones[Leg - 1].Center; Start.Z += S.Course.BallRadius; }
	const int Target = Leg < Zones ? Leg : -1;

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
					} while (!Allowed(S, X, Y));
					Machines.push_back({{X, Y, M.CoreHeight}, M.Strength, M.Range});
					Spots.push_back({X, Y, 0});
				}
				const double Yaw = std::round(-180.0 + U(Rng) * 360.0);
				const double Power = std::round((0.1 + U(Rng) * 0.9) * 40.0) / 40.0;
				if (Reaches(S, Start, Target, Machines, Yaw, Power))
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
						Found[I].Tolerance += Reaches(S, Start, Target, Machines, Yaw, Power);
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
