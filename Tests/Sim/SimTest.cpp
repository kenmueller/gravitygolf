// Standalone tests for the deterministic simulation. No Unreal needed:
//   Tests/Sim/run.sh
#include "Sim/GGSim.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace GG;

static int Failures = 0;

#define CHECK(Cond)                                                                                        \
	do                                                                                                     \
	{                                                                                                      \
		if (!(Cond))                                                                                       \
		{                                                                                                  \
			std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #Cond);                                    \
			++Failures;                                                                                    \
		}                                                                                                  \
	} while (0)

static FVec2 Dir(double Degrees)
{
	const double R = Degrees * 3.14159265358979323846 / 180.0;
	return {std::cos(R), std::sin(R)};
}

static FPlanet Planet(double X, double Y, double R, double G, ESurface S = ESurface::Grass)
{
	FPlanet P;
	P.Position = {X, Y};
	P.Radius = R;
	P.SurfaceGravity = G;
	P.Surface = S;
	return P;
}

/** The bake-off hole, mirrored in Content/Holes/ringside.json. */
static FHole Ringside()
{
	FHole H;
	H.Planets.push_back(Planet(-2600, -700, 320, 700)); // 0 tee
	FPlanet Giant = Planet(0, 250, 1000, 650, ESurface::Hazard);
	Giant.AtmosphereRadius = 1350;
	Giant.AtmosphereDrag = 0.8;
	H.Planets.push_back(Giant); // 1 gas giant
	FPlanet Green = Planet(2650, 450, 360, 700);
	Green.bHasCup = true;
	Green.CupDirection = Dir(150);
	Green.CupCos = std::cos(7.0 * 3.14159265358979323846 / 180.0);
	H.Planets.push_back(Green); // 2 green
	H.Planets.push_back(Planet(1000, -1650, 220, 520, ESurface::Ice)); // 3 ice moon
	H.Stardust = {{{-900, 1700}, 45}, {{400, 2050}, 45}, {{1700, 1750}, 45}};
	H.TeePlanet = 0;
	H.TeeDirection = Dir(40);
	H.BoundsCenter = {0, 0};
	H.BoundsRadius = 7500;
	H.BallRadius = 30;
	H.Par = 3;
	H.MaxGravityWells = 1;
	return H;
}

static void TestRestsOnPlanet()
{
	FHole H;
	H.Planets.push_back(Planet(0, 0, 300, 700));
	H.BallRadius = 30;
	FSim Sim;
	Sim.Begin(H, {}, SurfacePoint(H, 0, Dir(90)), {0, 0});
	CHECK(Sim.Run() == EBallState::AtRest);
	CHECK(std::fabs(Length(Sim.Ball().Position) - 330) < 1e-6);
}

static void TestPuttIntoCup()
{
	FHole H;
	FPlanet P = Planet(0, 0, 300, 700);
	P.bHasCup = true;
	P.CupDirection = Dir(60);
	P.CupCos = std::cos(6.0 * 3.14159265358979323846 / 180.0);
	H.Planets.push_back(P);
	H.BallRadius = 30;
	FSim Sim;
	// Roll counter-clockwise along the surface toward the cup, 30 degrees away.
	const FVec2 Start = SurfacePoint(H, 0, Dir(90));
	Sim.Begin(H, {}, Start, Dir(180) * 240.0);
	std::vector<FEvent> Events;
	const EBallState End = Sim.Run(&Events);
	CHECK(End != EBallState::Sunk); // rolling the wrong way (away from the cup at 60 degrees)

	Sim.Begin(H, {}, Start, Dir(0) * 240.0);
	CHECK(Sim.Run() == EBallState::Sunk);
}

static void TestHazardAndBounds()
{
	FHole H;
	H.Planets.push_back(Planet(0, 0, 300, 700));
	H.Planets.push_back(Planet(1500, 0, 400, 500, ESurface::Hazard));
	H.BallRadius = 30;
	H.BoundsRadius = 5000;
	FSim Sim;
	Sim.Begin(H, {}, SurfacePoint(H, 0, Dir(0)), Dir(0) * 1500.0);
	CHECK(Sim.Run() == EBallState::Hazard);

	Sim.Begin(H, {}, SurfacePoint(H, 0, Dir(180)), Dir(180) * 2600.0);
	CHECK(Sim.Run() == EBallState::OutOfBounds);
}

static void TestDeterminism()
{
	const FHole H = Ringside();
	const std::vector<FWell> Wells = {{{600, 1900}, 3.0e8}};
	const FVec2 Start = SurfacePoint(H, 0, H.TeeDirection);
	FSim A;
	FSim B;
	A.Begin(H, Wells, Start, Dir(52.5) * 1900.0);
	B.Begin(H, Wells, Start, Dir(52.5) * 1900.0);
	A.Run();
	B.Run();
	CHECK(A.Hash() == B.Hash());
	std::printf("determinism hash %016llx\n", static_cast<unsigned long long>(A.Hash()));
}

struct FSweep
{
	int Shots = 0;
	int Sunk = 0;
	int Rest = 0;
	int Hazard = 0;
	int Out = 0;
	int Timeout = 0;
	int RestOnGreen = 0;
	double BestAngle = 0;
	double BestSpeed = 0;
};

static FSweep Sweep(const FHole& H, const std::vector<FWell>& Wells)
{
	FSweep S;
	const FVec2 Start = SurfacePoint(H, H.TeePlanet, H.TeeDirection);
	for (double Angle = 0; Angle < 360; Angle += 0.5)
	{
		for (double Power = 0.2; Power <= 1.0001; Power += 0.025)
		{
			FSim Sim;
			Sim.Begin(H, Wells, Start, Dir(Angle) * (Power * 2600.0));
			const EBallState End = Sim.Run();
			++S.Shots;
			switch (End)
			{
			case EBallState::Sunk:
				++S.Sunk;
				S.BestAngle = Angle;
				S.BestSpeed = Power * 2600.0;
				break;
			case EBallState::AtRest:
				++S.Rest;
				if (Sim.Ball().Contact == 2)
				{
					++S.RestOnGreen;
				}
				break;
			case EBallState::Hazard:
				++S.Hazard;
				break;
			case EBallState::OutOfBounds:
				++S.Out;
				break;
			default:
				++S.Timeout;
				break;
			}
		}
	}
	return S;
}

static void Report(const char* Label, const FSweep& S)
{
	std::printf("%-28s shots %5d  sunk %4d  restOnGreen %4d  rest %5d  hazard %5d  out %5d  timeout %4d", Label,
		S.Shots, S.Sunk, S.RestOnGreen, S.Rest, S.Hazard, S.Out, S.Timeout);
	if (S.Sunk)
	{
		std::printf("  e.g. angle %.1f speed %.0f", S.BestAngle, S.BestSpeed);
	}
	std::printf("\n");
}

int main(int Argc, char** Argv)
{
	TestRestsOnPlanet();
	TestPuttIntoCup();
	TestHazardAndBounds();
	TestDeterminism();

	if (Argc > 1 && std::string_view(Argv[1]) == "--sweep")
	{
		const FHole H = Ringside();
		Report("ringside, no wells", Sweep(H, {}));
		for (const FVec2 W : {FVec2{0, 1900}, FVec2{600, 1900}, FVec2{1200, 1600}, FVec2{1700, 1100}})
		{
			char Label[64];
			std::snprintf(Label, sizeof(Label), "well at (%.0f,%.0f)", W.X, W.Y);
			Report(Label, Sweep(H, {{W, 3.0e8}}));
		}
	}

	std::printf(Failures ? "%d FAILED\n" : "all passed\n", Failures);
	return Failures ? 1 : 0;
}
