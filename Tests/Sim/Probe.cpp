// Design aid: play one swing and print what happened.
// Usage: Tests/Sim/probe.sh <course id> <leg> <yaw> <power> [x y]...   (one x y pair per machine; omitted machines stay at their start)
#include "CourseFile.h"
#include "Sim/GGSim.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdlib>

using namespace GG;

int main(int Argc, char** Argv)
{
	FSetup S;
	if (Argc < 5 || !LoadCourse(Argv[1], S))
	{
		std::printf("usage: probe <file.course> <leg> <yaw> <power> [x y]...\n");
		return 2;
	}
	const FLeg Leg = MakeLeg(S, std::atoi(Argv[2]));
	const double Yaw = std::atof(Argv[3]) * 3.14159265358979323846 / 180.0;
	const double Power = std::atof(Argv[4]);
	std::vector<FAttractor> Machines;
	for (size_t M = 0; M < S.Machines.size(); ++M)
	{
		const int Arg = 5 + static_cast<int>(M) * 2;
		const double X = Arg + 1 < Argc ? std::atof(Argv[Arg]) : S.Machines[M].StartX;
		const double Y = Arg + 1 < Argc ? std::atof(Argv[Arg + 1]) : S.Machines[M].StartY;
		// Mirrors are turned as GG_TURNS says ("0,1": one number a machine), or all "/" without it.
		int Turn = 0;
		if (const char* Turns = std::getenv("GG_TURNS"))
		{
			Turn = static_cast<size_t>(2 * M) < std::strlen(Turns) && Turns[2 * M] == '1' ? 1 : 0;
		}
		Machines.push_back(Stand(S.Machines[M], X, Y, Turn));
	}

	FSim Sim;
	// Weight plates are down wherever a machine stands on one.
	std::vector<bool> Held = Leg.Held;
	GG::HoldWeightPlates(S.Course, Machines, Held);
	Sim.Begin(S.Course, Machines, Leg.Start, {std::cos(Yaw) * Power * 3000.0, std::sin(Yaw) * Power * 3000.0, 0.0}, {}, &Held);
	std::vector<FEvent> Events;
	const char* Names[] = {"engaged", "released", "switch", "gate closed", "bounce", "land", "lip out", "SUNK", "rest", "lost", "timed out"};
	double NextPrint = 0.0;
	while (!Sim.Finished())
	{
		Events.clear();
		Sim.Step(&Events);
		const FBall& B = Sim.Ball();
		for (const FEvent& E : Events)
		{
			std::printf("  t=%5.2f  %-11s index %d at (%.0f, %.0f) speed %.0f\n", B.Time, Names[static_cast<int>(E.Type)], E.Index,
				E.Position.X, E.Position.Y, E.Speed);
		}
		if (B.Time >= NextPrint)
		{
			std::printf("t=%5.2f  (%6.0f, %6.0f)  speed %5.0f\n", B.Time, B.Position.X, B.Position.Y, Length(B.Velocity));
			NextPrint += 0.5;
		}
	}
	std::printf("end: state %d at (%.0f, %.0f) after %.2f s, safe zone %d\n", static_cast<int>(Sim.Ball().State), Sim.Ball().Position.X,
		Sim.Ball().Position.Y, Sim.Ball().Time, Sim.Ball().SafeZone);
	return 0;
}
