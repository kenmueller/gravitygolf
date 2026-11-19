#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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

	/** Which course this is, so a joining player's copy loads the same one. */
	UPROPERTY(ReplicatedUsing = OnRep_CourseId)
	FString NetCourseId;
	UFUNCTION()
	void OnRep_CourseId();

	/** Samples of the path a swing would take, for the aiming guide. */
	void PreviewSwing(const FVector& Direction, double Power, double Seconds, TArray<FVector>& OutPoints) const;

	/** Nearest movable gravity machine to a point, or INDEX_NONE. */
	int32 FindMachine(const FVector& Near, double Reach) const;
	/** Move a machine. Base is where its feet stand; the field's center sits above it. */
	void MoveMachine(int32 Index, const FVector& Base, bool bCarried);
	FVector MachineBase(int32 Index) const;
	/** Some ground refuses machines (the paved court around a cup, for instance). */
	bool CanPlaceMachine(const FVector& Base) const;
	/** Show a short message on the HUD. */
	void Notify(const FString& Message);

	/** What the ground is made of under a point. */
	GG::ESurface SurfaceAt(const FVector& Location) const;

	/** Show the machines' fields boldly (true) or as faint rings only (false). */
	void SetFieldEmphasis(bool bEmphasize);
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
	float FieldLevel = 0.f;
	TArray<float> MachineFlash;
	/** Switches banked on earlier legs of this hole. */
	std::vector<bool> HeldSwitches;
	int32 CurrentLeg = 0;
	TArray<FGGSolution> Solutions;
	TArray<float> GateLevel;
	TArray<FVector> NoMachineZones; // x, y, radius
	double CoreHeight = 45.0;
	double Accumulator = 0.0;
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
