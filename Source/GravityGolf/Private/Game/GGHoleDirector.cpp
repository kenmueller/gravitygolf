#include "Game/GGHoleDirector.h"

#include "Components/DirectionalLightComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogGGDirector, Log, All);

using GGHoleLoader::ToSim;
using GGHoleLoader::ToWorld;

namespace
{
constexpr double AimDragForFullPower = 1500.0;
constexpr double WellStrength = 3.0e8;
constexpr double PenaltyPause = 1.2;

GG::FVec2 Direction(double Degrees)
{
	const double Radians = FMath::DegreesToRadians(Degrees);
	return {FMath::Cos(Radians), FMath::Sin(Radians)};
}

struct FPlanetPalette
{
	float Style;
	FLinearColor A;
	FLinearColor B;
	FLinearColor C;
	FLinearColor Air;
	float AirStrength;
};

FPlanetPalette PaletteFor(const FString& Style)
{
	if (Style == TEXT("green"))
	{
		return {1.f, {0.11f, 0.42f, 0.1f}, {0.18f, 0.56f, 0.14f}, {0.3f, 0.27f, 0.22f}, {0.45f, 0.85f, 1.f}, 2.f};
	}
	if (Style == TEXT("gasGiant"))
	{
		return {2.f, {0.86f, 0.72f, 0.52f}, {0.78f, 0.38f, 0.14f}, {0.42f, 0.12f, 0.06f}, {1.f, 0.62f, 0.35f}, 3.f};
	}
	if (Style == TEXT("ice"))
	{
		return {3.f, {0.78f, 0.9f, 1.f}, {0.42f, 0.6f, 0.86f}, {0.25f, 0.85f, 1.f}, {0.6f, 0.85f, 1.f}, 2.5f};
	}
	return {0.f, {0.08f, 0.28f, 0.06f}, {0.2f, 0.44f, 0.1f}, {0.32f, 0.28f, 0.24f}, {0.4f, 0.72f, 1.f}, 2.f};
}

FString ScoreName(int32 Strokes, int32 Par)
{
	if (Strokes == 1)
	{
		return TEXT("Hole in one!");
	}
	switch (Strokes - Par)
	{
	case -3:
		return TEXT("Albatross!");
	case -2:
		return TEXT("Eagle!");
	case -1:
		return TEXT("Birdie");
	case 0:
		return TEXT("Par");
	case 1:
		return TEXT("Bogey");
	case 2:
		return TEXT("Double bogey");
	default:
		return FString::Printf(TEXT("+%d"), Strokes - Par);
	}
}
} // namespace

AGGHoleDirector::AGGHoleDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

bool AGGHoleDirector::LoadHole(const FString& HoleName)
{
	if (!GGHoleLoader::Load(HoleName, Def))
	{
		return false;
	}
	BuildScene();
	ResetHole();
	return true;
}

void AGGHoleDirector::ResetHole()
{
	const GG::FHole& Hole = Def.Sim;
	Lie = GG::SurfacePoint(Hole, Hole.TeePlanet, Hole.TeeDirection);
	PreviousPosition = Lie;
	StrokeCount = 0;
	Status.Reset();
	Trail.Reset();
	bAiming = false;
	CurrentPhase = EGGPhase::Aiming;
	StardustTaken.Init(false, static_cast<int32>(Hole.Stardust.size()));
	for (UStaticMeshComponent* Mesh : StardustMeshes)
	{
		Mesh->SetVisibility(true);
	}
	if (BallMesh)
	{
		BallMesh->SetWorldLocation(ToWorld(Lie));
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Scene

UStaticMeshComponent* AGGHoleDirector::AddMesh(UStaticMesh* Mesh, UMaterialInterface* Material,
	const FTransform& Transform)
{
	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this);
	Component->SetStaticMesh(Mesh);
	Component->SetMobility(EComponentMobility::Movable);
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->SetupAttachment(Root);
	Component->SetWorldTransform(Transform);
	if (Material)
	{
		Component->SetMaterial(0, Material);
	}
	Component->RegisterComponent();
	return Component;
}

UMaterialInstanceDynamic* AGGHoleDirector::SurfaceMaterial(const FLinearColor& Color, float Roughness,
	float Metallic, const FLinearColor& Emissive)
{
	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(SurfaceBase, this);
	Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
	Material->SetVectorParameterValue(TEXT("Color"), Color);
	Material->SetScalarParameterValue(TEXT("Roughness"), Roughness);
	Material->SetScalarParameterValue(TEXT("Metallic"), Metallic);
	Material->SetVectorParameterValue(TEXT("EmissiveColor"), Emissive);
	return Material;
}

UMaterialInstanceDynamic* AGGHoleDirector::GlowMaterial(const FLinearColor& Color, float Strength)
{
	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(GlowBase, this);
	Material->SetVectorParameterValue(TEXT("Color"), Color);
	Material->SetScalarParameterValue(TEXT("Strength"), Strength);
	return Material;
}

void AGGHoleDirector::BuildScene()
{
	SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));

	UMaterialInterface* Fallback =
		LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	SurfaceBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_GG_Surface.M_GG_Surface"));
	GlowBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_GG_Glow.M_GG_Glow"));
	if (!SurfaceBase)
	{
		UE_LOG(LogGGDirector, Warning, TEXT("M_GG_Surface missing; run Tools/build_materials.py. Using fallback."));
		SurfaceBase = Fallback;
	}
	if (!GlowBase)
	{
		GlowBase = Fallback;
	}

	BuildLighting();

	// Starfield and nebula on a huge sphere around everything.
	if (UMaterialInterface* Sky = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_GG_Sky.M_GG_Sky")))
	{
		UStaticMeshComponent* Dome =
			AddMesh(SphereMesh, Sky, FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(4000.0)));
		Dome->SetCastShadow(false);
		Dome->bAffectDistanceFieldLighting = false;
	}

	for (int32 I = 0; I < Def.Looks.Num(); ++I)
	{
		BuildPlanet(I);
	}

	const GG::FHole& Hole = Def.Sim;
	const double BallScale = Hole.BallRadius / 50.0;
	UMaterialInterface* BallMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_GG_Ball.M_GG_Ball"));
	BallMesh = AddMesh(SphereMesh,
		BallMaterial ? BallMaterial : SurfaceMaterial(FLinearColor(0.92f, 0.92f, 0.9f), 0.22f, 0.f),
		FTransform(FRotator::ZeroRotator, ToWorld(Lie), FVector(BallScale)));

	for (const GG::FStardust& Mote : Hole.Stardust)
	{
		StardustMeshes.Add(AddMesh(SphereMesh, GlowMaterial(FLinearColor(1.f, 0.72f, 0.25f), 40.f),
			FTransform(FRotator::ZeroRotator, ToWorld(Mote.Position), FVector(Mote.Radius / 50.0 * 0.6))));
	}

	RefreshWells();
}

void AGGHoleDirector::BuildLighting()
{
	UWorld* World = GetWorld();

	ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(FVector::ZeroVector, Def.SunRotation);
	Sun->GetRootComponent()->SetMobility(EComponentMobility::Movable);
	if (UDirectionalLightComponent* Light = Cast<UDirectionalLightComponent>(Sun->GetLightComponent()))
	{
		Light->SetIntensity(Def.SunIntensity);
		Light->SetLightColor(Def.SunColor);
		Light->SetCastShadows(true);
	}

	// Cool rim light from the opposite side so planet silhouettes read against black space.
	ADirectionalLight* Rim = World->SpawnActor<ADirectionalLight>(FVector::ZeroVector,
		FRotator(-15.f, Def.SunRotation.Yaw + 160.f, 0.f));
	Rim->GetRootComponent()->SetMobility(EComponentMobility::Movable);
	if (UDirectionalLightComponent* Light = Cast<UDirectionalLightComponent>(Rim->GetLightComponent()))
	{
		Light->SetIntensity(Def.SunIntensity * 0.12f);
		Light->SetLightColor(FLinearColor(0.45f, 0.6f, 1.f));
		Light->SetCastShadows(false);
	}

	APostProcessVolume* Post = World->SpawnActor<APostProcessVolume>();
	Post->bUnbound = true;
	FPostProcessSettings& S = Post->Settings;
	S.bOverride_AutoExposureMethod = true;
	S.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	S.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	S.AutoExposureApplyPhysicalCameraExposure = false;
	S.bOverride_AutoExposureBias = true;
	S.AutoExposureBias = 0.f;
	S.bOverride_BloomIntensity = true;
	S.BloomIntensity = 0.8f;
	S.bOverride_VignetteIntensity = true;
	S.VignetteIntensity = 0.35f;
}

void AGGHoleDirector::BuildPlanet(int32 Index)
{
	const GG::FPlanet& Planet = Def.Sim.Planets[Index];
	const FGGPlanetLook& Look = Def.Looks[Index];

	FLinearColor Color(0.3f, 0.3f, 0.35f);
	float Roughness = 0.8f;
	if (Look.Style == TEXT("meadow"))
	{
		Color = FLinearColor(0.16f, 0.42f, 0.14f);
	}
	else if (Look.Style == TEXT("green"))
	{
		Color = FLinearColor(0.2f, 0.62f, 0.2f);
	}
	else if (Look.Style == TEXT("gasGiant"))
	{
		Color = FLinearColor(0.85f, 0.5f, 0.28f);
		Roughness = 0.6f;
	}
	else if (Look.Style == TEXT("ice"))
	{
		Color = FLinearColor(0.7f, 0.86f, 1.f);
		Roughness = 0.15f;
	}

	UMaterialInterface* Surface = SurfaceMaterial(Color, Roughness, 0.f);
	if (UMaterialInterface* PlanetBase =
			LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_GG_Planet.M_GG_Planet")))
	{
		const FPlanetPalette Palette = PaletteFor(Look.Style);
		UMaterialInstanceDynamic* Procedural = UMaterialInstanceDynamic::Create(PlanetBase, this);
		Procedural->SetScalarParameterValue(TEXT("Style"), Palette.Style);
		Procedural->SetVectorParameterValue(TEXT("ColorA"), Palette.A);
		Procedural->SetVectorParameterValue(TEXT("ColorB"), Palette.B);
		Procedural->SetVectorParameterValue(TEXT("ColorC"), Palette.C);
		Procedural->SetVectorParameterValue(TEXT("AtmosphereColor"), Palette.Air);
		Procedural->SetScalarParameterValue(TEXT("AtmosphereStrength"), Palette.AirStrength);
		Surface = Procedural;
	}

	// Planets spin slowly about the axis perpendicular to the play plane.
	AddMesh(SphereMesh, Surface,
		FTransform(FRotator(0.f, 37.f * Index, 0.f), ToWorld(Planet.Position), FVector(Planet.Radius / 50.0)));

	if (Look.RingOuter > Look.RingInner && Look.RingInner > 0.f)
	{
		BuildRings(Index);
	}
	if (Planet.bHasCup)
	{
		BuildCup(Index);
	}
}

void AGGHoleDirector::BuildRings(int32 Index)
{
	const GG::FPlanet& Planet = Def.Sim.Planets[Index];
	const FGGPlanetLook& Look = Def.Looks[Index];
	constexpr int32 Segments = 192;

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	for (int32 I = 0; I <= Segments; ++I)
	{
		const double A = 2.0 * PI * I / Segments;
		const FVector Radial(FMath::Cos(A), FMath::Sin(A), 0.0);
		Vertices.Add(Radial * Look.RingInner);
		Vertices.Add(Radial * Look.RingOuter);
		Normals.Add(FVector::UpVector);
		Normals.Add(FVector::UpVector);
		UVs.Add(FVector2D(0.0, static_cast<double>(I) / Segments));
		UVs.Add(FVector2D(1.0, static_cast<double>(I) / Segments));
		if (I < Segments)
		{
			const int32 B = I * 2;
			// Both windings so the ring shows from above and below.
			Triangles.Append({B, B + 2, B + 1, B + 1, B + 2, B + 3});
			Triangles.Append({B, B + 1, B + 2, B + 1, B + 3, B + 2});
		}
	}

	UProceduralMeshComponent* Ring = NewObject<UProceduralMeshComponent>(this);
	Ring->SetupAttachment(Root);
	Ring->SetMobility(EComponentMobility::Movable);
	Ring->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, {}, {}, false);
	Ring->SetMaterial(0, SurfaceMaterial(FLinearColor(0.75f, 0.62f, 0.48f), 0.7f, 0.f));
	Ring->SetWorldLocationAndRotation(ToWorld(Planet.Position), FRotator(Look.RingTilt, 25.f, 0.f));
	Ring->RegisterComponent();
}

void AGGHoleDirector::BuildCup(int32 Index)
{
	const GG::FPlanet& Planet = Def.Sim.Planets[Index];
	const FVector Center = ToWorld(Planet.Position);
	const FVector Normal = ToWorld(Planet.CupDirection).GetSafeNormal();
	const FVector Tangent = FVector::CrossProduct(FVector::UpVector, Normal);
	const FVector CupPoint = Center + Normal * Planet.Radius;
	const double CupRadius = Def.Sim.BallRadius * 1.7;

	AddMesh(CylinderMesh, SurfaceMaterial(FLinearColor(0.01f, 0.01f, 0.012f), 0.9f, 0.f),
		FTransform(FRotationMatrix::MakeFromZ(Normal).Rotator(), CupPoint - Normal * 2.0,
			FVector(CupRadius / 50.0, CupRadius / 50.0, 0.08)));

	constexpr double PoleLength = 480.0;
	AddMesh(CylinderMesh, SurfaceMaterial(FLinearColor(0.9f, 0.9f, 0.88f), 0.3f, 0.8f),
		FTransform(FRotationMatrix::MakeFromZ(Normal).Rotator(), CupPoint + Normal * (PoleLength * 0.5),
			FVector(0.05, 0.05, PoleLength / 100.0)));

	AddMesh(PlaneMesh, GlowMaterial(FLinearColor(1.f, 0.18f, 0.22f), 6.f),
		FTransform(FRotationMatrix::MakeFromXZ(Normal, FVector::UpVector).Rotator(),
			CupPoint + Normal * (PoleLength - 55.0) + Tangent * 70.0, FVector(1.1, 1.4, 1.0)));
}

void AGGHoleDirector::RefreshWells()
{
	for (UStaticMeshComponent* Mesh : WellMeshes)
	{
		Mesh->DestroyComponent();
	}
	WellMeshes.Reset();

	auto AddWell = [this](const GG::FWell& Well) {
		const bool bPull = Well.Strength > 0.0;
		const FLinearColor Color = bPull ? FLinearColor(0.25f, 0.55f, 1.f) : FLinearColor(1.f, 0.55f, 0.15f);
		WellMeshes.Add(AddMesh(SphereMesh, GlowMaterial(Color, 30.f),
			FTransform(FRotator::ZeroRotator, ToWorld(Well.Position), FVector(1.1))));
	};
	for (const GG::FWell& Well : Def.Sim.FixedWells)
	{
		AddWell(Well);
	}
	for (const GG::FWell& Well : PlacedWells)
	{
		AddWell(Well);
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Play

std::vector<GG::FWell> AGGHoleDirector::ActiveWells() const
{
	std::vector<GG::FWell> Wells = Def.Sim.FixedWells;
	Wells.insert(Wells.end(), PlacedWells.begin(), PlacedWells.end());
	return Wells;
}

int32 AGGHoleDirector::GravityWellsLeft() const
{
	return Def.Sim.MaxGravityWells - PlacedIsAnti.FilterByPredicate([](bool bAnti) { return !bAnti; }).Num();
}

int32 AGGHoleDirector::AntigravityWellsLeft() const
{
	return Def.Sim.MaxAntigravityWells - PlacedIsAnti.FilterByPredicate([](bool bAnti) { return bAnti; }).Num();
}

int32 AGGHoleDirector::StardustCollected() const
{
	return StardustTaken.FilterByPredicate([](bool bTaken) { return bTaken; }).Num();
}

void AGGHoleDirector::PlaceWell(const FVector& PlanePoint, bool bAntigravity)
{
	if (CurrentPhase != EGGPhase::Aiming)
	{
		return;
	}
	const GG::FWell Well{ToSim(PlanePoint), bAntigravity ? -WellStrength : WellStrength};
	if ((bAntigravity ? AntigravityWellsLeft() : GravityWellsLeft()) > 0)
	{
		PlacedWells.push_back(Well);
		PlacedIsAnti.Add(bAntigravity);
	}
	else
	{
		// Out of wells: move the most recent one of this kind instead.
		for (int32 I = PlacedIsAnti.Num() - 1; I >= 0; --I)
		{
			if (PlacedIsAnti[I] == bAntigravity)
			{
				PlacedWells[I] = Well;
				break;
			}
		}
	}
	RefreshWells();
}

GG::FVec2 AGGHoleDirector::AimVelocity(const FVector& PlanePoint) const
{
	const GG::FVec2 Delta = ToSim(PlanePoint) - Lie;
	const double Distance = GG::Length(Delta);
	if (Distance < 1.0)
	{
		return {};
	}
	const double Power = FMath::Min(Distance / AimDragForFullPower, 1.0);
	return Delta * (Power * Physics.MaxLaunchSpeed / Distance);
}

void AGGHoleDirector::BeginAim(const FVector& PlanePoint)
{
	if (CurrentPhase == EGGPhase::Sunk)
	{
		ResetHole();
		return;
	}
	if (CurrentPhase != EGGPhase::Aiming)
	{
		return;
	}
	bAiming = true;
	AimPoint = PlanePoint;
}

void AGGHoleDirector::UpdateAim(const FVector& PlanePoint)
{
	AimPoint = PlanePoint;
}

void AGGHoleDirector::EndAim(const FVector& PlanePoint)
{
	if (!bAiming)
	{
		return;
	}
	bAiming = false;
	const GG::FVec2 Velocity = AimVelocity(PlanePoint);
	if (GG::Length(Velocity) > Physics.MaxLaunchSpeed * 0.04)
	{
		Launch(Velocity);
	}
}

void AGGHoleDirector::CancelAim()
{
	bAiming = false;
}

void AGGHoleDirector::LaunchAt(double AngleDegrees, double Power)
{
	if (CurrentPhase == EGGPhase::Aiming)
	{
		Launch(Direction(AngleDegrees) * (FMath::Clamp(Power, 0.0, 1.0) * Physics.MaxLaunchSpeed));
	}
}

void AGGHoleDirector::Launch(GG::FVec2 Velocity)
{
	++StrokeCount;
	Status.Reset();
	Sim.Begin(Def.Sim, ActiveWells(), Lie, Velocity, Physics);
	PreviousPosition = Lie;
	Accumulator = 0.0;
	Trail.Reset();
	CurrentPhase = EGGPhase::Flying;
	PhaseTime = 0.0;
	UE_LOG(LogGGDirector, Log, TEXT("Stroke %d: launch (%.1f, %.1f)"), StrokeCount, Velocity.X, Velocity.Y);
}

void AGGHoleDirector::HandleEvent(const GG::FEvent& Event)
{
	switch (Event.Type)
	{
	case GG::EEventType::Stardust:
		if (StardustTaken.IsValidIndex(Event.Index))
		{
			StardustTaken[Event.Index] = true;
			StardustMeshes[Event.Index]->SetVisibility(false);
		}
		break;
	case GG::EEventType::LipOut:
		Status = TEXT("Lipped out!");
		break;
	default:
		break;
	}
}

void AGGHoleDirector::FinishStroke(GG::EBallState State)
{
	PhaseTime = 0.0;
	switch (State)
	{
	case GG::EBallState::Sunk:
		CurrentPhase = EGGPhase::Sunk;
		Status = ScoreName(StrokeCount, Def.Sim.Par);
		break;
	case GG::EBallState::AtRest:
		Lie = Sim.Ball().Position;
		CurrentPhase = EGGPhase::Aiming;
		break;
	case GG::EBallState::Hazard:
		++StrokeCount;
		Status = TEXT("Swallowed by the clouds  +1");
		CurrentPhase = EGGPhase::Penalty;
		break;
	default:
		++StrokeCount;
		Status = TEXT("Lost in space  +1");
		CurrentPhase = EGGPhase::Penalty;
		break;
	}
	UE_LOG(LogGGDirector, Log, TEXT("Stroke %d finished: %s"), StrokeCount, *Status);
}

void AGGHoleDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	PhaseTime += DeltaSeconds;

	if (CurrentPhase == EGGPhase::Flying)
	{
		Accumulator += DeltaSeconds * TimeScale;
		std::vector<GG::FEvent> Events;
		int32 Steps = 0;
		while (Accumulator >= GG::FSim::Dt && Steps < 480)
		{
			PreviousPosition = Sim.Ball().Position;
			Events.clear();
			Sim.Step(&Events);
			for (const GG::FEvent& Event : Events)
			{
				HandleEvent(Event);
			}
			Accumulator -= GG::FSim::Dt;
			++Steps;
			if (Sim.Finished())
			{
				FinishStroke(Sim.Ball().State);
				break;
			}
		}
		Trail.Add(BallLocation());
	}
	else if (CurrentPhase == EGGPhase::Penalty && PhaseTime > PenaltyPause)
	{
		CurrentPhase = EGGPhase::Aiming;
		PreviousPosition = Lie;
		Trail.Reset();
	}

	if (BallMesh)
	{
		BallMesh->SetWorldLocation(BallLocation());
	}

	const float Spin = GetWorld()->GetTimeSeconds() * 60.f;
	for (UStaticMeshComponent* Mesh : StardustMeshes)
	{
		Mesh->SetWorldRotation(FRotator(0.f, Spin, 0.f));
	}

	DrawTrail();
	DrawAim();
}

FVector AGGHoleDirector::BallLocation() const
{
	if (CurrentPhase == EGGPhase::Flying)
	{
		const double Alpha = FMath::Clamp(Accumulator / GG::FSim::Dt, 0.0, 1.0);
		const GG::FVec2 Current = Sim.Ball().Position;
		return ToWorld(PreviousPosition + (Current - PreviousPosition) * Alpha);
	}
	if (CurrentPhase == EGGPhase::Sunk)
	{
		return ToWorld(Sim.Ball().Position);
	}
	return ToWorld(Lie);
}

FVector AGGHoleDirector::HoleCenter() const
{
	FBox Box(ForceInit);
	for (const GG::FPlanet& Planet : Def.Sim.Planets)
	{
		Box += FBox::BuildAABB(ToWorld(Planet.Position), FVector(Planet.Radius));
	}
	return Box.GetCenter() * FVector(1, 1, 0);
}

double AGGHoleDirector::HoleExtent() const
{
	FBox Box(ForceInit);
	for (const GG::FPlanet& Planet : Def.Sim.Planets)
	{
		Box += FBox::BuildAABB(ToWorld(Planet.Position), FVector(Planet.Radius));
	}
	return Box.GetExtent().Size2D();
}

void AGGHoleDirector::DrawTrail() const
{
	const int32 Count = Trail.Num();
	for (int32 I = 1; I < Count; ++I)
	{
		const float Fade = static_cast<float>(I) / Count;
		const FColor Color = FLinearColor::LerpUsingHSV(FLinearColor(0.1f, 0.2f, 0.6f), FLinearColor(0.6f, 0.9f, 1.f), Fade)
								 .ToFColor(true);
		DrawDebugLine(GetWorld(), Trail[I - 1], Trail[I], Color, false, -1.f, 0, 6.f * Fade + 1.f);
	}
}

void AGGHoleDirector::DrawAim() const
{
	if (!bAiming || CurrentPhase != EGGPhase::Aiming)
	{
		return;
	}

	const GG::FVec2 Velocity = AimVelocity(AimPoint);
	const double Power = GG::Length(Velocity) / Physics.MaxLaunchSpeed;
	const FColor Color = FLinearColor::LerpUsingHSV(FLinearColor(0.4f, 1.f, 0.6f), FLinearColor(1.f, 0.3f, 0.2f),
		static_cast<float>(Power))
							 .ToFColor(true);

	// Guide line: show the path until the first encounter, then let the player read the rest.
	GG::FSim Preview;
	Preview.Begin(Def.Sim, ActiveWells(), Lie, Velocity, Physics);
	FVector Last = ToWorld(Lie);
	for (int32 Step = 1; Step <= 240 && !Preview.Finished(); ++Step)
	{
		Preview.Step();
		if (Step % 6 == 0)
		{
			const FVector Point = ToWorld(Preview.Ball().Position);
			if ((Step / 6) % 2 == 0)
			{
				DrawDebugLine(GetWorld(), Last, Point, Color, false, -1.f, 0, 5.f);
			}
			Last = Point;
		}
		if (Preview.Ball().Contact >= 0 && Step > 12)
		{
			break;
		}
	}
}
