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
	/** A mirror's face: the ball leaves it at the angle it came in at, a little slower. */
	Mirror,
};

/** Axis-aligned collision box. */
struct FBox
{
	FVec3 Min;
	FVec3 Max;
	ESurface Surface = ESurface::Stone;
};

/**
 * A gravity machine. Strength is its peak pull in cm/s^2; the field is a dome of radius Range that fades smoothly
 * to zero at its edge.
 *  - Attractor (positive): pulls only while the ball approaches, then releases it for the rest of the swing.
 *  - Repulsor (negative): pushes the ball away whenever it is inside the dome.
 */
struct FAttractor
{
	FVec3 Position;
	double Strength = 420.0;
	double Range = 1800.0;
	/**
	 * A machine can instead be a mirror: an upright slab standing on its spot at 45 degrees, which the ball bounces
	 * off like light. -1 is not a mirror. 0 is a mirror lying along +X+Y ("/" seen from above with +Y up), 1 along
	 * +X-Y ("\"). A mirror has no field: its strength and range are zero.
	 */
	int Turn = -1;
};

/** A pad where the ball may come to rest and stay: a checkpoint. Optional, per level. */
struct FSafeZone
{
	FVec3 Center;
	double Radius = 150.0;
};

/**
 * A plate in the ground that opens its gates.
 *  - A light plate is pressed by the ball rolling over it.
 *  - A weight plate ignores the ball: it is down only while a gravity machine stands on it (see HoldWeightPlates).
 */
struct FSwitch
{
	FVec3 Center;
	double Radius = 60.0;
	bool bWeight = false;
};

/** A barrier that stops the ball until its switch is pressed. */
struct FGate
{
	FBox Box;
	int Switch = 0;
	/** Seconds the gate stays open after the switch is pressed. Zero means it stays open for the rest of the swing. */
	double OpenSeconds = 0.0;
	/** A grille: while shut it also stops a machine being carried (see Sim/GGLogistics.h). The ball cannot tell. */
	bool bGrille = false;
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
	std::vector<FSwitch> Switches;
	std::vector<FGate> Gates;
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
	/** A mirror: half its length, half its thickness, and how far it stands above and below its machine's center. */
	double MirrorHalfLength = 90.0;
	double MirrorHalfThickness = 8.0;
	double MirrorHalfHeight = 60.0;
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
	/** A machine took hold of the ball / let it go. Index is the machine. */
	Engaged,
	Released,
	/** The ball pressed a switch. Index is the switch. */
	SwitchPressed,
	/** A timed gate ran out and closed. Index is the gate. */
	GateClosed,
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
	int Index = -1;
};

/**
 * Marks every weight plate that has a machine standing on it as held. (A field machine: a mirror is too light.) Held is one flag per switch, the same list
 * FSim::Begin takes; flags already set (presses banked on earlier legs) are left alone.
 */
void HoldWeightPlates(const FCourse& Course, const std::vector<FAttractor>& Machines, std::vector<bool>& Held);

class FSim
{
public:
	static constexpr double Dt = 1.0 / 240.0;

	/**
	 * Start a swing. HeldSwitches lists switches already pressed on earlier legs of the hole (banked by reaching a
	 * safe pad); they stay pressed for this swing.
	 */
	void Begin(const FCourse& InCourse, const std::vector<FAttractor>& InAttractors, FVec3 Start, FVec3 Launch,
		const FPhysics& InPhysics = {}, const std::vector<bool>* HeldSwitches = nullptr);

	/** Advance one fixed step (internally sub-stepped at high speed so thin walls can't be tunneled). */
	void Step(std::vector<FEvent>* Events = nullptr);

	EBallState Run(std::vector<FEvent>* Events = nullptr, int MaxSteps = 1 << 20);

	const FBall& Ball() const { return BallState; }

	bool Finished() const { return BallState.State != EBallState::Moving; }
	/** Is this gate letting the ball through right now? */
	bool GateOpen(int Index) const;
	/** Has this switch been pressed (on this swing, or held from an earlier leg)? */
	bool SwitchPressed(int Index) const { return PressedAt[Index] >= 0.0; }

	/** Order-sensitive hash of the full state, for determinism checks. */
	uint64_t Hash() const;

private:
	void SubStep(double H, std::vector<FEvent>* Events);
	FVec3 Acceleration(FVec3 Position, FVec3 Velocity, std::vector<FEvent>* Events);

	const FCourse* Course = nullptr;
	std::vector<FAttractor> Attractors;
	FPhysics Physics;
	FBall BallState;
	bool bInCupZone = false;
	/** Per machine: has the ball started approaching it, and has it since been let go? */
	std::vector<bool> Engaged;
	std::vector<bool> Released;
	/** Per switch: the time it was last pressed, or a negative number if never. */
	std::vector<double> PressedAt;
	std::vector<bool> GateWasOpen;

	void Collide(const FBox& Box, FVec3 A, double H, bool bWasOnGround, ESurface& Ground, std::vector<FEvent>* Events);
	void CollideMirror(const FAttractor& Mirror, FVec3 A, double H, bool bWasOnGround, ESurface& Ground, std::vector<FEvent>* Events);
};
} // namespace GG
