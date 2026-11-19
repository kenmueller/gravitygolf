// Standalone tests for the deterministic 3D simulation. No Unreal needed:  Tests/Sim/run.sh
#include "Sim/GGSim.h"

#include <cmath>
#include <cstdio>
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

/** A 40 m by 10 m green slab with its top at Z = 0, a wall at the far end and the cup 20 m out. */
static FCourse Fairway(ESurface Surface)
{
	FCourse C;
	C.Boxes.push_back({{-500, -500, -100}, {4000, 500, 0}, Surface});
	C.Boxes.push_back({{4000, -500, -100}, {4100, 500, 300}, ESurface::Stone});
	C.Tee = {0, 0, 6};
	C.Cup = {2000, 0, 0};
	return C;
}

static void TestRestsOnGround()
{
	const FCourse C = Fairway(ESurface::Green);
	FSim Sim;
	Sim.Begin(C, {}, {0, 0, 50}, {});
	CHECK(Sim.Run() == EBallState::AtRest);
	CHECK(std::fabs(Sim.Ball().Position.Z - 6.0) < 1e-6);
}

static void TestPuttDistanceBySurface()
{
	double Rolled[3];
	const ESurface Surfaces[3] = {ESurface::Grass, ESurface::Green, ESurface::Ice};
	for (int I = 0; I < 3; ++I)
	{
		FCourse C = Fairway(Surfaces[I]);
		C.Cup = {0, 400, 0}; // out of the way
		FSim Sim;
		Sim.Begin(C, {}, C.Tee, {400, 0, 0});
		Sim.Run();
		Rolled[I] = Sim.Ball().Position.X;
	}
	std::printf("4 m/s putt rolls: grass %.0f cm, green %.0f cm, ice %.0f cm\n", Rolled[0], Rolled[1], Rolled[2]);
	CHECK(Rolled[0] < Rolled[1] && Rolled[1] < Rolled[2]);
	// v^2 / (2 * mu * g) = 160000 / (2 * 0.09 * 980) = 907 cm on the green.
	CHECK(std::fabs(Rolled[1] - 907.0) < 15.0);
}

static void TestCupCapture()
{
	const FCourse C = Fairway(ESurface::Green);
	FSim Sim;
	Sim.Begin(C, {}, C.Tee, {640, 0, 0}); // arrives at the cup at about 2.4 m/s
	CHECK(Sim.Run() == EBallState::Sunk);

	std::vector<FEvent> Events;
	Sim.Begin(C, {}, C.Tee, {1400, 0, 0}); // far too fast: skips over the cup on the way out
	Sim.Run(&Events);
	bool bLipOut = false;
	for (const FEvent& E : Events)
	{
		bLipOut |= E.Type == EEventType::LipOut;
	}
	CHECK(bLipOut);
}

static void TestChipBouncesAndStops()
{
	FCourse C = Fairway(ESurface::Grass);
	C.Cup = {0, 400, 0};
	std::vector<FEvent> Events;
	FSim Sim;
	Sim.Begin(C, {}, C.Tee, {900, 0, 900});
	CHECK(Sim.Run(&Events) == EBallState::AtRest);
	int Bounces = 0;
	for (const FEvent& E : Events)
	{
		Bounces += E.Type == EEventType::Bounce;
	}
	CHECK(Bounces >= 2);
	CHECK(Sim.Ball().Position.X > 1600 && Sim.Ball().Position.X < 4000);
}

static void TestWallAndLoss()
{
	FCourse C = Fairway(ESurface::Stone);
	C.Cup = {0, 400, 0};
	FSim Sim;
	Sim.Begin(C, {}, C.Tee, {2990, 0, 100}); // fast and low: must hit the wall, not tunnel through it
	Sim.Run();
	CHECK(Sim.Ball().Position.X < 4000);

	Sim.Begin(C, {}, C.Tee, {0, 1500, 300}); // off the side
	CHECK(Sim.Run() == EBallState::Lost);
}

static void TestAttractorBendsShot()
{
	FCourse C = Fairway(ESurface::Green);
	C.Cup = {0, 400, 0};
	FSim Straight;
	Straight.Begin(C, {}, C.Tee, {600, 0, 0});
	Straight.Run();

	FSim Bent;
	Bent.Begin(C, {{{800, 300, 60}, 420.0, 1300}}, C.Tee, {600, 0, 0});
	Bent.Run();
	std::printf("attractor pulled the putt %.0f cm sideways\n", Bent.Ball().Position.Y - Straight.Ball().Position.Y);
	CHECK(std::fabs(Straight.Ball().Position.Y) < 1e-9);
	CHECK(Bent.Ball().Position.Y > 30.0);
}

static void TestDeterminism()
{
	const FCourse C = Fairway(ESurface::Grass);
	const std::vector<FAttractor> A = {{{900, -200, 150}, 420.0, 1300}};
	FSim One;
	FSim Two;
	One.Begin(C, A, C.Tee, {1700, 120, 1300});
	Two.Begin(C, A, C.Tee, {1700, 120, 1300});
	One.Run();
	Two.Run();
	CHECK(One.Hash() == Two.Hash());
	std::printf("determinism hash %016llx\n", static_cast<unsigned long long>(One.Hash()));
}

int main()
{
	TestRestsOnGround();
	TestPuttDistanceBySurface();
	TestCupCapture();
	TestChipBouncesAndStops();
	TestWallAndLoss();
	TestAttractorBendsShot();
	TestDeterminism();
	std::printf(Failures ? "%d FAILED\n" : "all passed\n", Failures);
	return Failures ? 1 : 0;
}
