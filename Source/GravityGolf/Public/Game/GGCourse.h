#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sim/GGProof.h"
#include "Sim/GGSim.h"

#include "GGCourse.generated.h"

class AGGGolfer;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UInstancedStaticMeshComponent;
class UPointLightComponent;
class UStaticMesh;
class UStaticMeshComponent;

UENUM()
enum class EGGPhase : uint8
{
	Ready,
	Flying,
	Penalty,
	Sunk,
};

/** A verified answer for one leg of a hole: where each machine stands, and a swing that works. */
struct FGGSolution
{
	int32 Leg = 0;
	TArray<FVector> Machines;
	double Yaw = 0.0;
	double Power = 0.0;
	/** Workshop holes: which plates were already banked when the leg was played. */
	TArray<bool> Held;
	/** Which way each machine is turned (it matters only for the mirrors among them). */
	TArray<int32> Turns;
};

/**
 * One hole's rules and state: loads Content/Courses/<id>.json, runs the deterministic simulation at a fixed
 * step, and owns the ball, the cup's flag and the gravity machines. The level's scenery is separate; the level
 * script builds it from the same JSON, so what you see is what the ball collides with.
 */
UCLASS()
class GRAVITYGOLF_API AGGCourse : public AActor
{
	GENERATED_BODY()

public:
	AGGCourse();

	static bool Exists(const FString& CourseId);
	bool Load(const FString& CourseId);
	/** Build the hole from course JSON held in memory (the Workshop does this after every edit). Replaces whatever
	 *  was loaded before. */
	bool LoadText(const FString& Text, const FString& CourseId);
	/** Take the hole down again: every mesh and light it built, and all of its state. */
	void Unload();

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Playing together. The simulation is deterministic, so players are kept in step by commands, not by copying
	// state: the server decides what happens (a swing, a machine picked up or set down, the ball called back) and
	// tells every machine, and each applies it to its own copy of the hole. The functions below are those requests.
	// They act on the server (or when playing alone); a client asks through its own golfer.

	/** Launch along a unit direction with power in [0, 1]. Swinger is whose swing it was, if anyone's. */
	void Swing(const FVector& Direction, double Power, AGGGolfer* Swinger = nullptr);
	/** Put everything back: ball, machines and the attempt count. */
	void ResetHole();
	/** Bring just the ball back to the last safe spot. */
	void ReturnBall();
	/** A player picks a machine up. While carried it is switched off: its field does nothing to a swing. */
	void CarryMachine(int32 Index, AGGGolfer* Carrier);
	/** A player sets a carried machine down. False (with a message) where machines can't stand. */
	bool PlaceMachine(int32 Index, const FVector& Base, AGGGolfer* Carrier);
	AGGGolfer* CarrierOf(int32 Index) const;
	/** Some machines are mirrors: no field, an upright glass at 45 degrees that the ball bounces off. A mirror can be
	 *  turned to its other diagonal, in a player's hands or where it stands. */
	bool IsMirror(int32 Index) const { return Index >= 0 && Index < static_cast<int32>(Attractors.size()) && Attractors[Index].Turn >= 0; }
	int32 MachineTurn(int32 Index) const { return IsMirror(Index) ? Attractors[Index].Turn : 0; }
	void TurnMachine(int32 Index);
	// For a rival playing its own copy of the hole: the rules, and how the hole starts.
	const GG::FCourse& Rules() const { return Course; }
	const GG::FPhysics& PhysicsRules() const { return Physics; }
	const std::vector<GG::FAttractor>& MachineFields() const { return Attractors; }
	const TArray<FVector>& StartingBases() const { return InitialBases; }
	/** The hole as the puzzle-law proof sees it (see Sim/GGProof.h). */
	GG::FSetup MakeSetup() const;
	/** Legs finished by a real swing since the hole was loaded: where the machines stood, and the swing. The Workshop
	 *  keeps these as its proof that a hole can be solved. */
	const TArray<FGGSolution>& PlayedLegs() const { return Played; }
	/** Goes up each time a leg is added to PlayedLegs. */
	int32 PlayedSerial() const { return PlayedCount; }
	/** A hole built in the Workshop draws itself. */
	bool Drawn() const { return bDrawn; }
	double MachineCoreHeight() const { return CoreHeight; }
	/** Seconds the player has spent on this hole so far (it stops when the ball is sunk). */
	double PlayTime() const { return Clock; }

	/** This machine's copy of the hole in one line, for checking that every player's copy agrees. */
	FString Describe() const;

	UFUNCTION(NetMulticast, Reliable)
	void NetSwing(FVector Velocity, AGGGolfer* Swinger);
	UFUNCTION(NetMulticast, Reliable)
	void NetResetHole();
	/** bAbort: the server called the ball back in mid-roll, so the stroke under way is thrown away. */
	UFUNCTION(NetMulticast, Reliable)
	void NetReturnBall(bool bAbort);
	UFUNCTION(NetMulticast, Reliable)
	void NetCarry(int32 Index, AGGGolfer* Carrier);
	UFUNCTION(NetMulticast, Reliable)
	void NetPlace(int32 Index, FVector Base);
	UFUNCTION(NetMulticast, Reliable)
	void NetAssisted();
	UFUNCTION(NetMulticast, Reliable)
	void NetTurn(int32 Index, int32 Turn);

	/** Which course this is, so a joining player's copy loads the same one. */
	UPROPERTY(ReplicatedUsing = OnRep_CourseId)
	FString NetCourseId;
	UFUNCTION()
	void OnRep_CourseId();

	/** A hole built in the Workshop has no file. Its text travels to every player who joins, and again whenever its
	 *  builder changes it. */
	UPROPERTY(ReplicatedUsing = OnRep_CourseText)
	FString NetCourseText;
	UFUNCTION()
	void OnRep_CourseText();

	/** Samples of the path a swing would take, for the aiming guide. */
	void PreviewSwing(const FVector& Direction, double Power, double Seconds, TArray<FVector>& OutPoints) const;

	/** Nearest movable gravity machine to a point, or INDEX_NONE. */
	int32 FindMachine(const FVector& Near, double Reach) const;
	/** Move a machine. Base is where its feet stand; the field's center sits above it. */
	void MoveMachine(int32 Index, const FVector& Base, bool bCarried);
	FVector MachineBase(int32 Index) const;
	/** Some ground refuses machines (the paved court around a cup, for instance). */
	bool CanPlaceMachine(const FVector& Base) const;
	/** Where a machine set down near Base comes to stand: the middle of the plinth it is on, if the hole has plinths. */
	FVector MachineSpot(const FVector& Base) const;
	bool HasPlinths() const { return Plinths.Num() > 0; }
	/** Show a short message on the HUD. */
	void Notify(const FString& Message);

	/** What the ground is made of under a point. */
	GG::ESurface SurfaceAt(const FVector& Location) const;

	/** The colour a plate and the gates it opens share, by the plate's number. */
	static FLinearColor SwitchColor(int32 Index);
	/** Show the machines' fields boldly (true) or as faint rings only (false). */
	void SetFieldEmphasis(bool bEmphasize);
	/** Draw no fields at all (the Workshop's table, where the rings of a few machines cover the whole design). */
	void HideFields(bool bHide) { bFieldsHidden = bHide; }
	/** After a solve: open the next hole, or replay this one if it's the last. */
	void Advance();
	bool HasNext() const { return !NextLevel.IsEmpty(); }
	double KillHeight() const { return Course.KillZ; }
	/** Which leg the ball is on: 0 from the tee, 1 after the first safe pad, and so on. */
	int32 Leg() const { return CurrentLeg; }
	const FGGSolution* SolutionForLeg(int32 Leg) const;
	int32 MachineCount() const { return MachineBases.Num(); }

	/** Dotted aiming guide; pass an empty array to hide it. */
	void ShowGuide(const TArray<FVector>& Points);

	FVector BallLocation() const;
	FVector TeeStart() const;
	float TeeYaw() const { return TeeYawDegrees; }
	FVector CupLocation() const;

	EGGPhase Phase() const { return CurrentPhase; }
	int32 Strokes() const { return StrokeCount; }
	int32 Par() const { return Course.Par; }
	const FString& CourseName() const { return Name; }
	/** The course file's name, which is also the key its best score is saved under. */
	const FString& CourseId() const { return Id; }
	/** Watching the solution doesn't count as solving: a hole solved after its solution was shown isn't recorded. */
	void MarkAssisted();
	bool Assisted() const { return bAssisted; }
	/** Swings in a perfect solve: one per leg. */
	int32 PerfectSwings() const { return static_cast<int32>(Course.SafeZones.size()) + 1; }
	const FString& StatusText() const { return Status; }

	UPROPERTY(EditAnywhere, Category = "Gravity Golf")
	float TimeScale = 1.f;

private:
	void BuildActors();
	/** Plants are spawned from the level's list: the scans where the renderer can draw them, stand-ins elsewhere. */
	void SpawnPlants(const FString& LevelName);
	/** Bring this machine's copy up to date before a command: finish a stroke still rolling, end a penalty pause. */
	void Flush();
	/** The machines as a swing sees them: a carried machine is switched off and nowhere. */
	std::vector<GG::FAttractor> ActiveMachines() const;
	void FinishStroke(GG::EBallState State);
	void BuildMachine(int32 Index);
	void UpdateFieldPreview(float DeltaSeconds);
	UStaticMeshComponent* AddMesh(UStaticMesh* Mesh, UMaterialInterface* Material, const FTransform& Transform,
		USceneComponent* Parent = nullptr);
	UMaterialInstanceDynamic* Glow(const FLinearColor& Color, float Strength);

	GG::FCourse Course;
	GG::FPhysics Physics;
	GG::FSim Sim;
	std::vector<GG::FAttractor> Attractors;
	TArray<FVector> MachineBases;
	TArray<FVector> InitialBases;
	TArray<int32> InitialTurns;
	void BuildMirror(int32 Index);
	/** Stand a mirror's glass along the diagonal it is turned to. */
	void ApplyTurn(int32 Index);
	TArray<bool> Movable;
	/** Per machine: who is carrying it, if anyone. */
	TArray<TWeakObjectPtr<AGGGolfer>> Carriers;

	FString Name;
	FString Status;
	FString Id;
	bool bAssisted = false;
	float TeeYawDegrees = 0.f;
	EGGPhase CurrentPhase = EGGPhase::Ready;
	GG::FVec3 Lie;
	GG::FVec3 PreviousPosition;
	GG::FVec3 RestPosition;
	GG::FVec3 LastSafe;
	double NoticeTime = 0.0;
	FString NextLevel;
	bool bFieldEmphasis = false;
	bool bFieldsHidden = false;
	float FieldLevel = 0.f;
	TArray<float> MachineFlash;
	/** Switches banked on earlier legs of this hole. */
	std::vector<bool> HeldSwitches;
	int32 CurrentLeg = 0;
	TArray<FGGSolution> Solutions;
	TArray<float> GateLevel;
	TArray<FVector> NoMachineZones; // x, y, radius
	/** Plinths (x, y, radius). A hole that has any lets machines stand only on them. */
	TArray<FVector> Plinths;
	/** A course made in the Workshop has no level dressed around it, so it draws its own floor and walls. */
	struct FDrawnBox
	{
		FVector Center;
		FVector Size;
		FString Look;
	};
	TArray<FDrawnBox> DrawnBoxes;
	bool bDrawn = false;
	bool bPlantsSpawned = false;
	void BuildDrawn();
	TArray<FGGSolution> Played;
	int32 PlayedCount = 0;
	/** The swing under way, kept until it is known whether it finished its leg. */
	FGGSolution Pending;
	bool bPendingValid = false;
	double CoreHeight = 45.0;
	double Accumulator = 0.0;
	double Clock = 0.0;
	double PhaseTime = 0.0;
	int32 StrokeCount = 0;
	TArray<FVector> Trail;

	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> BallMesh;

	UPROPERTY()
	TArray<TObjectPtr<USceneComponent>> Machines;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> MachineCores;
	UPROPERTY()
	TArray<TObjectPtr<class UPointLightComponent>> MachineLights;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> TrailDots;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> GuideDots;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> FieldDots;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> FieldRing;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> FieldPushDots;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> FieldPushRing;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> SwitchMaterials;

	UPROPERTY()
	TArray<TObjectPtr<USceneComponent>> GateBars;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> GlowBase;
};
