#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sim/GGSim.h"

#include "GGCourse.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
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
	void ResetHole();

	/** Samples of the path a swing would take, for the aiming guide. */
	void PreviewSwing(const FVector& Direction, double Power, double Seconds, TArray<FVector>& OutPoints) const;

	int32 FindAttractor(const FVector& Near, double Reach) const;
	void MoveAttractor(int32 Index, const FVector& Location);

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
	UStaticMeshComponent* AddMesh(UStaticMesh* Mesh, UMaterialInterface* Material, const FTransform& Transform);
	UMaterialInstanceDynamic* Glow(const FLinearColor& Color, float Strength);

	GG::FCourse Course;
	GG::FPhysics Physics;
	GG::FSim Sim;
	std::vector<GG::FAttractor> Attractors;
	std::vector<GG::FAttractor> InitialAttractors;
	TArray<bool> Movable;

	FString Name;
	FString Status;
	float TeeYawDegrees = 0.f;
	EGGPhase CurrentPhase = EGGPhase::Ready;
	GG::FVec3 Lie;
	GG::FVec3 PreviousPosition;
	double Accumulator = 0.0;
	double PhaseTime = 0.0;
	int32 StrokeCount = 0;
	TArray<FVector> Trail;

	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> BallMesh;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> AttractorMeshes;

	UPROPERTY()
	TArray<TObjectPtr<UPointLightComponent>> AttractorLights;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> GlowBase;
};
