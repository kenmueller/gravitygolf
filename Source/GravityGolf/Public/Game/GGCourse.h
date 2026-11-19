#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sim/GGSim.h"

#include "GGCourse.generated.h"

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

	/** Launch along a unit direction with power in [0, 1]. */
	void Swing(const FVector& Direction, double Power);
	/** Put everything back: ball, machines and the attempt count. */
	void ResetHole();
	/** Bring just the ball back to the last safe spot. */
	void ReturnBall();

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
	const FString& StatusText() const { return Status; }

	UPROPERTY(EditAnywhere, Category = "Gravity Golf")
	float TimeScale = 1.f;

private:
	void BuildActors();
	void FinishStroke(GG::EBallState State);
	void BuildMachine(int32 Index);
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

	FString Name;
	FString Status;
	float TeeYawDegrees = 0.f;
	EGGPhase CurrentPhase = EGGPhase::Ready;
	GG::FVec3 Lie;
	GG::FVec3 PreviousPosition;
	GG::FVec3 RestPosition;
	GG::FVec3 LastSafe;
	double NoticeTime = 0.0;
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
	TObjectPtr<UInstancedStaticMeshComponent> TrailDots;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> GuideDots;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> GlowBase;
};
