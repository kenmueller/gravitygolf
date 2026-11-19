// Deterministic 2D simulation for Gravity Golf.
//
// Plain C++ with no Unreal types, so the exact same code runs in the game, in replays, in the level solver
// and (compiled to WebAssembly) in the server referee. Only + - * / and sqrt touch doubles, and the .cpp
// disables floating-point contraction, so a shot produces bit-identical results on every platform.
//
// Units: centimeters and seconds, in the plane of play (Unreal's X/Y plane).

#pragma once

#include <cstdint>
#include <vector>

namespace GG
{
struct FVec2
{
	double X = 0.0;
	double Y = 0.0;
};

inline FVec2 operator+(FVec2 A, FVec2 B) { return {A.X + B.X, A.Y + B.Y}; }
inline FVec2 operator-(FVec2 A, FVec2 B) { return {A.X - B.X, A.Y - B.Y}; }
inline FVec2 operator*(FVec2 A, double S) { return {A.X * S, A.Y * S}; }
inline double Dot(FVec2 A, FVec2 B) { return A.X * B.X + A.Y * B.Y; }
double Length(FVec2 A);

enum class ESurface : uint8_t
{
	Grass,
	Ice,
	Dust,
	Hazard,
};

struct FPlanet
{
	FVec2 Position;
	double Radius = 300.0;
	/** Gravitational acceleration at the surface, cm/s^2. Mass follows as SurfaceGravity * Radius^2. */
	double SurfaceGravity = 600.0;
	ESurface Surface = ESurface::Grass;
	/** Drag applies inside this radius. Zero means no atmosphere. */
	double AtmosphereRadius = 0.0;
	/** Fraction of velocity lost per second inside the atmosphere. */
	double AtmosphereDrag = 0.0;
	bool bHasCup = false;
	/** Unit vector from the planet center to the cup. Quantized by the loader so it's identical everywhere. */
	FVec2 CupDirection{1.0, 0.0};
	/** Cosine of the cup's angular half-width; the ball drops in when its contact normal is within it. */
	double CupCos = 0.999;
};

struct FWell
{
	FVec2 Position;
	/** Signed strength in cm^3/s^2. Positive pulls (gravity), negative pushes (antigravity). */
	double Strength = 0.0;
};

struct FStardust
{
	FVec2 Position;
	double Radius = 40.0;
};

struct FHole
{
	std::vector<FPlanet> Planets;
	std::vector<FWell> FixedWells;
	std::vector<FStardust> Stardust;
	int TeePlanet = 0;
	FVec2 TeeDirection{0.0, 1.0};
	FVec2 BoundsCenter;
	double BoundsRadius = 12000.0;
	double BallRadius = 25.0;
	int Par = 3;
	int MaxGravityWells = 0;
	int MaxAntigravityWells = 0;
};

struct FPhysics
{
	double MaxLaunchSpeed = 2600.0;
	double WellSoftening = 150.0;
	double WellMaxAccel = 4000.0;
	/** Normal impact speed below which the ball settles instead of bouncing. */
	double BounceThreshold = 70.0;
	double RestSpeed = 10.0;
	double RestTime = 0.3;
	double CupCaptureSpeed = 340.0;
	double MaxFlightTime = 20.0;
};

enum class EBallState : uint8_t
{
	Moving,
	AtRest,
	Sunk,
	OutOfBounds,
	Hazard,
	TimedOut,
};

struct FBall
{
	FVec2 Position;
	FVec2 Velocity;
	double Radius = 25.0;
	/** Planet the ball touched this step, or -1 while airborne. */
	int Contact = -1;
	double SlowTime = 0.0;
	double Time = 0.0;
	EBallState State = EBallState::Moving;
};

enum class EEventType : uint8_t
{
	Launch,
	Bounce,
	Land,
	Stardust,
	LipOut,
	Sunk,
	Rest,
	OutOfBounds,
	Hazard,
	TimedOut,
};

struct FEvent
{
	EEventType Type = EEventType::Bounce;
	/** Planet or stardust index, when relevant. */
	int Index = -1;
	double Speed = 0.0;
	FVec2 Position;
};

/** Where a ball rests on a planet's surface in a given direction from its center. */
FVec2 SurfacePoint(const FHole& Hole, int Planet, FVec2 Direction);

class FSim
{
public:
	static constexpr double Dt = 1.0 / 240.0;

	/**
	 * Start a stroke. Wells are everything active this stroke (fixed + placed). Launch is the initial velocity;
	 * callers clamp it to FPhysics::MaxLaunchSpeed.
	 */
	void Begin(const FHole& InHole, const std::vector<FWell>& InWells, FVec2 Start, FVec2 Launch,
		const FPhysics& InPhysics = {});

	/** Advance one fixed step. Appends anything notable to Events when it's provided. */
	void Step(std::vector<FEvent>* Events = nullptr);

	/** Step until the stroke ends or MaxSteps pass. Returns the final state. */
	EBallState Run(std::vector<FEvent>* Events = nullptr, int MaxSteps = 1 << 20);

	const FBall& Ball() const { return BallState; }
	const std::vector<bool>& StardustCollected() const { return Collected; }
	FVec2 Acceleration(FVec2 Position) const;
	bool Finished() const { return BallState.State != EBallState::Moving; }

	/** Order-sensitive hash of the full state, for determinism checks. */
	uint64_t Hash() const;

private:
	const FHole* Hole = nullptr;
	std::vector<FWell> Wells;
	FPhysics Physics;
	FBall BallState;
	std::vector<bool> Collected;
	bool bAirborne = true;
	bool bInCupZone = false;
};
} // namespace GG
