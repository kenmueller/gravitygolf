// Deterministic 3D golf ball simulation.
//
// Plain C++ with no Unreal types, so the same code runs in the game, in replays, in the level solver and
// (compiled to WebAssembly) in the server referee. Only + - * / and sqrt touch doubles, and the .cpp disables
// floating-point contraction, so a shot produces bit-identical results on every platform.
//
// Units: centimeters and seconds, in Unreal world axes (Z up).

#pragma once

#include <cstdint>
#include <vector>

namespace GG
{
struct FVec3
{
	double X = 0.0;
	double Y = 0.0;
	double Z = 0.0;
};

inline FVec3 operator+(FVec3 A, FVec3 B) { return {A.X + B.X, A.Y + B.Y, A.Z + B.Z}; }
inline FVec3 operator-(FVec3 A, FVec3 B) { return {A.X - B.X, A.Y - B.Y, A.Z - B.Z}; }
inline FVec3 operator*(FVec3 A, double S) { return {A.X * S, A.Y * S, A.Z * S}; }
inline double Dot(FVec3 A, FVec3 B) { return A.X * B.X + A.Y * B.Y + A.Z * B.Z; }
double Length(FVec3 A);

enum class ESurface : uint8_t
{
	Grass,
	Green,
	Stone,
	Sand,
	Ice,
};

/** Axis-aligned collision box. */
struct FBox
{
	FVec3 Min;
	FVec3 Max;
	ESurface Surface = ESurface::Stone;
};

/**
 * A gravity machine. Strength is its peak pull in cm/s^2 (negative pushes); the field is a dome of radius Range
 * that fades smoothly to zero at its edge. It acts only while the ball approaches, then releases it.
 */
struct FAttractor
{
	FVec3 Position;
	double Strength = 420.0;
	double Range = 1800.0;
};

/** A pad where the ball may come to rest and stay: a checkpoint. Optional, per level. */
struct FSafeZone
{
	FVec3 Center;
	double Radius = 150.0;
};

struct FCourse
{
	std::vector<FBox> Boxes;
	FVec3 Gravity{0.0, 0.0, -980.0};
	/** Where the ball's center starts. */
	FVec3 Tee;
	/** Center of the cup's mouth, on the surface. */
	FVec3 Cup;
	double CupRadius = 18.0;
	/** The ball drops when it crosses the cup at or below this speed. */
	double CupCaptureSpeed = 320.0;
	std::vector<FSafeZone> SafeZones;
	double BallRadius = 6.0;
	/** The ball is lost below this height. */
	double KillZ = -1000.0;
	int Par = 3;
};

struct FPhysics
{
	double MaxLaunchSpeed = 3000.0;
	double AttractorSoftening = 80.0;
	/** Normal impact speed below which the ball settles instead of bouncing. */
	double BounceThreshold = 60.0;
	double RestSpeed = 6.0;
	double RestTime = 0.35;
	double MaxFlightTime = 30.0;
};

enum class EBallState : uint8_t
{
	Moving,
	AtRest,
	Sunk,
	Lost,
	TimedOut,
};

struct FBall
{
	FVec3 Position;
	FVec3 Velocity;
	double Radius = 6.0;
	bool bOnGround = false;
	/** Index of the safe zone the ball came to rest in, or -1. */
	int SafeZone = -1;
	double SlowTime = 0.0;
	double Time = 0.0;
	EBallState State = EBallState::Moving;
};

enum class EEventType : uint8_t
{
	Bounce,
	Land,
	LipOut,
	Sunk,
	Rest,
	Lost,
	TimedOut,
};

struct FEvent
{
	EEventType Type = EEventType::Bounce;
	ESurface Surface = ESurface::Stone;
	double Speed = 0.0;
	FVec3 Position;
};

class FSim
{
public:
	static constexpr double Dt = 1.0 / 240.0;

	void Begin(const FCourse& InCourse, const std::vector<FAttractor>& InAttractors, FVec3 Start, FVec3 Launch,
		const FPhysics& InPhysics = {});

	/** Advance one fixed step (internally sub-stepped at high speed so thin walls can't be tunneled). */
	void Step(std::vector<FEvent>* Events = nullptr);

	EBallState Run(std::vector<FEvent>* Events = nullptr, int MaxSteps = 1 << 20);

	const FBall& Ball() const { return BallState; }

	bool Finished() const { return BallState.State != EBallState::Moving; }

	/** Order-sensitive hash of the full state, for determinism checks. */
	uint64_t Hash() const;

private:
	void SubStep(double H, std::vector<FEvent>* Events);
	FVec3 Acceleration(FVec3 Position, FVec3 Velocity);

	const FCourse* Course = nullptr;
	std::vector<FAttractor> Attractors;
	FPhysics Physics;
	FBall BallState;
	bool bInCupZone = false;
	/** Per machine: has the ball started approaching it, and has it since been let go? */
	std::vector<bool> Engaged;
	std::vector<bool> Released;
};
} // namespace GG
