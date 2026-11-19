#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Hole/GGHoleLoader.h"
#include "Sim/GGSim.h"

#include "GGHoleDirector.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UProceduralMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

UENUM()
enum class EGGPhase : uint8
{
	Aiming,
	Flying,
	Penalty,
	Sunk,
};

/**
 * Owns one hole: builds its scene from the hole definition, runs the deterministic simulation at a fixed
 * step, and turns player input into wells and launches.
 */
UCLASS()
class GRAVITYGOLF_API AGGHoleDirector : public AActor
{
	GENERATED_BODY()

public:
	AGGHoleDirector();

	bool LoadHole(const FString& HoleName);

	virtual void Tick(float DeltaSeconds) override;

	// Input, in world space on the play plane (Z = 0).
	void BeginAim(const FVector& PlanePoint);
	void UpdateAim(const FVector& PlanePoint);
	void EndAim(const FVector& PlanePoint);
	void CancelAim();
	void PlaceWell(const FVector& PlanePoint, bool bAntigravity);
	void ResetHole();

	/** Launch toward a sim-space angle with power in [0, 1]. Used by automated captures. */
	void LaunchAt(double AngleDegrees, double Power);

	FVector BallLocation() const;
	FVector HoleCenter() const;
	double HoleExtent() const;

	const FGGHoleDefinition& Definition() const { return Def; }
	EGGPhase Phase() const { return CurrentPhase; }
	int32 Strokes() const { return StrokeCount; }
	int32 StardustCollected() const;
	FString StatusText() const { return Status; }
	int32 GravityWellsLeft() const;
	int32 AntigravityWellsLeft() const;

	UPROPERTY(EditAnywhere, Category = "Gravity Golf")
	float TimeScale = 1.f;

private:
	void BuildScene();
	void BuildLighting();
	void BuildPlanet(int32 Index);
	void BuildRings(int32 Index);
	void BuildCup(int32 Index);
	void RefreshWells();
	void Launch(GG::FVec2 Velocity);
	void HandleEvent(const GG::FEvent& Event);
	void FinishStroke(GG::EBallState State);
	void DrawAim() const;
	void DrawTrail() const;
	std::vector<GG::FWell> ActiveWells() const;
	GG::FVec2 AimVelocity(const FVector& PlanePoint) const;

	UStaticMeshComponent* AddMesh(UStaticMesh* Mesh, UMaterialInterface* Material, const FTransform& Transform);
	UMaterialInstanceDynamic* SurfaceMaterial(const FLinearColor& Color, float Roughness, float Metallic,
		const FLinearColor& Emissive = FLinearColor::Black);
	UMaterialInstanceDynamic* GlowMaterial(const FLinearColor& Color, float Strength);

	FGGHoleDefinition Def;
	GG::FSim Sim;
	GG::FPhysics Physics;
	std::vector<GG::FWell> PlacedWells;
	TArray<bool> PlacedIsAnti;

	EGGPhase CurrentPhase = EGGPhase::Aiming;
	GG::FVec2 Lie;
	GG::FVec2 PreviousPosition;
	double Accumulator = 0.0;
	double PhaseTime = 0.0;
	int32 StrokeCount = 0;
	FString Status;
	TArray<FVector> Trail;
	TArray<bool> StardustTaken;

	bool bAiming = false;
	FVector AimPoint = FVector::ZeroVector;

	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> BallMesh;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> StardustMeshes;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> WellMeshes;

	UPROPERTY()
	TObjectPtr<UStaticMesh> SphereMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CylinderMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> PlaneMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> SurfaceBase;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> GlowBase;
};
