#include "Game/GGCourse.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogGGCourse, Log, All);

namespace
{
constexpr double PenaltyPause = 1.5;

FVector ToUnreal(GG::FVec3 V)
{
	return FVector(V.X, V.Y, V.Z);
}

GG::FVec3 ToSim(const FVector& V)
{
	return {V.X, V.Y, V.Z};
}

FString CoursePath(const FString& CourseId)
{
	return FPaths::ProjectContentDir() / TEXT("Courses") / (CourseId + TEXT(".json"));
}

GG::FVec3 ReadVec(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, GG::FVec3 Default = {})
{
	const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
	if (Object->TryGetArrayField(Field, Values) && Values->Num() == 3)
	{
		return {(*Values)[0]->AsNumber(), (*Values)[1]->AsNumber(), (*Values)[2]->AsNumber()};
	}
	return Default;
}

GG::ESurface ParseSurface(const FString& Surface)
{
	if (Surface == TEXT("grass"))
	{
		return GG::ESurface::Grass;
	}
	if (Surface == TEXT("green"))
	{
		return GG::ESurface::Green;
	}
	if (Surface == TEXT("sand"))
	{
		return GG::ESurface::Sand;
	}
	if (Surface == TEXT("ice"))
	{
		return GG::ESurface::Ice;
	}
	return GG::ESurface::Stone;
}

FString ScoreName(int32 Strokes, int32 Par)
{
	if (Strokes == 1)
	{
		return TEXT("Hole in one!");
	}
	switch (Strokes - Par)
	{
	case -2:
		return TEXT("Eagle!");
	case -1:
		return TEXT("Birdie");
	case 0:
		return TEXT("Par");
	case 1:
		return TEXT("Bogey");
	default:
		return Strokes < Par ? TEXT("Albatross!") : FString::Printf(TEXT("+%d"), Strokes - Par);
	}
}
} // namespace

AGGCourse::AGGCourse()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

bool AGGCourse::Exists(const FString& CourseId)
{
	return FPaths::FileExists(CoursePath(CourseId));
}

bool AGGCourse::Load(const FString& CourseId)
{
	FString Text;
	TSharedPtr<FJsonObject> Json;
	if (!FFileHelper::LoadFileToString(Text, *CoursePath(CourseId)) ||
		!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) || !Json.IsValid())
	{
		UE_LOG(LogGGCourse, Error, TEXT("Can't load course %s"), *CoursePath(CourseId));
		return false;
	}

	Course = GG::FCourse();
	Name = Json->GetStringField(TEXT("name"));
	Course.Par = Json->GetIntegerField(TEXT("par"));
	Course.BallRadius = Json->GetNumberField(TEXT("ballRadius"));
	Course.KillZ = Json->GetNumberField(TEXT("killZ"));
	Course.Gravity = ReadVec(Json, TEXT("gravity"), {0, 0, -980});

	const TSharedPtr<FJsonObject> Tee = Json->GetObjectField(TEXT("tee"));
	Course.Tee = ReadVec(Tee, TEXT("position"));
	TeeYawDegrees = Tee->GetNumberField(TEXT("yaw"));

	const TSharedPtr<FJsonObject> Cup = Json->GetObjectField(TEXT("cup"));
	Course.Cup = ReadVec(Cup, TEXT("position"));
	Course.CupRadius = Cup->GetNumberField(TEXT("radius"));

	for (const TSharedPtr<FJsonValue>& Value : Json->GetArrayField(TEXT("boxes")))
	{
		const TSharedPtr<FJsonObject> Box = Value->AsObject();
		const GG::FVec3 Center = ReadVec(Box, TEXT("center"));
		const GG::FVec3 Half = ReadVec(Box, TEXT("size")) * 0.5;
		Course.Boxes.push_back({Center - Half, Center + Half, ParseSurface(Box->GetStringField(TEXT("surface")))});
	}

	Attractors.clear();
	Movable.Reset();
	for (const TSharedPtr<FJsonValue>& Value : Json->GetArrayField(TEXT("attractors")))
	{
		const TSharedPtr<FJsonObject> A = Value->AsObject();
		Attractors.push_back({ReadVec(A, TEXT("position")), A->GetNumberField(TEXT("strength")), A->GetNumberField(TEXT("range"))});
		Movable.Add(A->GetBoolField(TEXT("movable")));
	}
	InitialAttractors = Attractors;

	BuildActors();
	ResetHole();
	UE_LOG(LogGGCourse, Log, TEXT("Loaded course %s: %d boxes, %d attractors"), *Name,
		static_cast<int32>(Course.Boxes.size()), static_cast<int32>(Attractors.size()));
	return true;
}

UStaticMeshComponent* AGGCourse::AddMesh(UStaticMesh* Mesh, UMaterialInterface* Material, const FTransform& Transform)
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

UMaterialInstanceDynamic* AGGCourse::Glow(const FLinearColor& Color, float Strength)
{
	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(GlowBase, this);
	Material->SetVectorParameterValue(TEXT("Color"), Color);
	Material->SetScalarParameterValue(TEXT("Strength"), Strength);
	return Material;
}

void AGGCourse::BuildActors()
{
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	GlowBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_GG_Glow.M_GG_Glow"));
	UMaterialInterface* BallMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_GG_Ball.M_GG_Ball"));

	BallMesh = AddMesh(Sphere, BallMaterial,
		FTransform(FRotator::ZeroRotator, ToUnreal(Course.Tee), FVector(Course.BallRadius / 50.0)));

	// The flag: a glowing pennant so the cup reads from across the course.
	const FVector CupTop = ToUnreal(Course.Cup) + FVector(0, 0, 225);
	AddMesh(Plane, Glow(FLinearColor(1.f, 0.16f, 0.1f), 4.f),
		FTransform(FRotator(0.f, 0.f, 90.f), CupTop + FVector(0, 32, -22), FVector(0.01, 0.6, 0.4)));

	for (const GG::FAttractor& Attractor : Attractors)
	{
		const bool bPull = Attractor.Strength > 0.0;
		const FLinearColor Color = bPull ? FLinearColor(0.2f, 0.55f, 1.f) : FLinearColor(1.f, 0.5f, 0.12f);
		AttractorMeshes.Add(AddMesh(Sphere, Glow(Color, 60.f),
			FTransform(FRotator::ZeroRotator, ToUnreal(Attractor.Position), FVector(0.55))));

		UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetupAttachment(Root);
		Light->SetWorldLocation(ToUnreal(Attractor.Position));
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetIntensity(900.f);
		Light->SetLightColor(Color);
		Light->SetAttenuationRadius(1400.f);
		Light->SetSourceRadius(26.f);
		Light->RegisterComponent();
		AttractorLights.Add(Light);
	}
}

void AGGCourse::ResetHole()
{
	Lie = Course.Tee;
	PreviousPosition = Lie;
	StrokeCount = 0;
	Status.Reset();
	Trail.Reset();
	CurrentPhase = EGGPhase::Ready;
	Attractors = InitialAttractors;
	for (int32 I = 0; I < AttractorMeshes.Num(); ++I)
	{
		MoveAttractor(I, ToUnreal(Attractors[I].Position));
	}
}

int32 AGGCourse::FindAttractor(const FVector& Near, double Reach) const
{
	int32 Best = INDEX_NONE;
	double BestDistance = Reach;
	for (int32 I = 0; I < Movable.Num(); ++I)
	{
		const double Distance = FVector::Dist(Near, ToUnreal(Attractors[I].Position));
		if (Movable[I] && Distance < BestDistance)
		{
			Best = I;
			BestDistance = Distance;
		}
	}
	return Best;
}

void AGGCourse::MoveAttractor(int32 Index, const FVector& Location)
{
	if (!AttractorMeshes.IsValidIndex(Index))
	{
		return;
	}
	// Whole centimeters keep the simulation's inputs identical on every machine.
	const FVector Snapped(FMath::RoundToDouble(Location.X), FMath::RoundToDouble(Location.Y), FMath::RoundToDouble(Location.Z));
	Attractors[Index].Position = ToSim(Snapped);
	AttractorMeshes[Index]->SetWorldLocation(Snapped);
	AttractorLights[Index]->SetWorldLocation(Snapped);
}

void AGGCourse::Swing(const FVector& Direction, double Power)
{
	if (CurrentPhase != EGGPhase::Ready)
	{
		return;
	}
	const FVector Velocity = Direction.GetSafeNormal() * (FMath::Clamp(Power, 0.02, 1.0) * Physics.MaxLaunchSpeed);
	++StrokeCount;
	Status.Reset();
	Sim.Begin(Course, Attractors, Lie, ToSim(Velocity), Physics);
	PreviousPosition = Lie;
	Accumulator = 0.0;
	Trail.Reset();
	CurrentPhase = EGGPhase::Flying;
	PhaseTime = 0.0;
	UE_LOG(LogGGCourse, Log, TEXT("Stroke %d: launch %s"), StrokeCount, *Velocity.ToCompactString());
}

void AGGCourse::PreviewSwing(const FVector& Direction, double Power, double Seconds, TArray<FVector>& OutPoints) const
{
	OutPoints.Reset();
	GG::FSim Preview;
	const FVector Velocity = Direction.GetSafeNormal() * (FMath::Clamp(Power, 0.02, 1.0) * Physics.MaxLaunchSpeed);
	Preview.Begin(Course, Attractors, Lie, ToSim(Velocity), Physics);
	const int32 Steps = FMath::RoundToInt32(Seconds / GG::FSim::Dt);
	for (int32 Step = 0; Step < Steps && !Preview.Finished(); ++Step)
	{
		Preview.Step();
		if (Step % 6 == 0)
		{
			OutPoints.Add(ToUnreal(Preview.Ball().Position));
		}
	}
}

void AGGCourse::FinishStroke(GG::EBallState State)
{
	PhaseTime = 0.0;
	switch (State)
	{
	case GG::EBallState::Sunk:
		CurrentPhase = EGGPhase::Sunk;
		Status = ScoreName(StrokeCount, Course.Par);
		break;
	case GG::EBallState::AtRest:
		Lie = Sim.Ball().Position;
		CurrentPhase = EGGPhase::Ready;
		break;
	default:
		++StrokeCount;
		Status = TEXT("Lost ball  +1");
		CurrentPhase = EGGPhase::Penalty;
		break;
	}
	UE_LOG(LogGGCourse, Log, TEXT("Stroke %d finished: state %d %s at %s"), StrokeCount, static_cast<int32>(State),
		*Status, *ToUnreal(Sim.Ball().Position).ToCompactString());
}

void AGGCourse::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	PhaseTime += DeltaSeconds;

	if (CurrentPhase == EGGPhase::Flying)
	{
		Accumulator += DeltaSeconds * TimeScale;
		int32 Steps = 0;
		while (Accumulator >= GG::FSim::Dt && Steps < 480)
		{
			PreviousPosition = Sim.Ball().Position;
			Sim.Step();
			Accumulator -= GG::FSim::Dt;
			++Steps;
			if (Sim.Finished())
			{
				FinishStroke(Sim.Ball().State);
				break;
			}
		}
		Trail.Add(BallLocation());
		if (Trail.Num() > 240)
		{
			Trail.RemoveAt(0);
		}
	}
	else if (CurrentPhase == EGGPhase::Penalty && PhaseTime > PenaltyPause)
	{
		CurrentPhase = EGGPhase::Ready;
		Trail.Reset();
	}

	BallMesh->SetWorldLocation(BallLocation());

	// Machines breathe.
	const float Pulse = 1.f + 0.06f * FMath::Sin(GetWorld()->GetTimeSeconds() * 2.2f);
	for (UStaticMeshComponent* Mesh : AttractorMeshes)
	{
		Mesh->SetWorldScale3D(FVector(0.55f * Pulse));
	}

	for (int32 I = 1; I < Trail.Num(); ++I)
	{
		const float Fade = static_cast<float>(I) / Trail.Num();
		DrawDebugLine(GetWorld(), Trail[I - 1], Trail[I], FLinearColor(0.5f * Fade, 0.85f * Fade, 1.f * Fade).ToFColor(true),
			false, -1.f, 0, 1.5f * Fade + 0.3f);
	}
}

FVector AGGCourse::BallLocation() const
{
	if (CurrentPhase == EGGPhase::Flying)
	{
		const double Alpha = FMath::Clamp(Accumulator / GG::FSim::Dt, 0.0, 1.0);
		return ToUnreal(PreviousPosition + (Sim.Ball().Position - PreviousPosition) * Alpha);
	}
	if (CurrentPhase == EGGPhase::Sunk)
	{
		return ToUnreal(Sim.Ball().Position);
	}
	return ToUnreal(Lie);
}

FVector AGGCourse::TeeStart() const
{
	// Stand a little behind and to the left of the ball, like a golfer addressing it.
	const FRotator Yaw(0.f, TeeYawDegrees, 0.f);
	return ToUnreal(Course.Tee) + Yaw.RotateVector(FVector(-160.0, -45.0, 100.0));
}

FVector AGGCourse::CupLocation() const
{
	return ToUnreal(Course.Cup);
}
