#include "Game/GGCourse.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Game/GGAudio.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogGGCourse, Log, All);

namespace
{
/** Seconds the ball lies where it stopped before returning to the tee. */
constexpr double ReturnPause = 1.6;

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
	Course.CupCaptureSpeed = Cup->GetNumberField(TEXT("captureSpeed"));

	NoMachineZones.Reset();
	for (const TSharedPtr<FJsonValue>& Value : Json->GetArrayField(TEXT("noMachine")))
	{
		const GG::FVec3 At = ReadVec(Value->AsObject(), TEXT("position"));
		NoMachineZones.Add(FVector(At.X, At.Y, Value->AsObject()->GetNumberField(TEXT("radius"))));
	}
	for (const TSharedPtr<FJsonValue>& Value : Json->GetArrayField(TEXT("safeZones")))
	{
		Course.SafeZones.push_back({ReadVec(Value->AsObject(), TEXT("position")), Value->AsObject()->GetNumberField(TEXT("radius"))});
	}

	for (const TSharedPtr<FJsonValue>& Value : Json->GetArrayField(TEXT("boxes")))
	{
		const TSharedPtr<FJsonObject> Box = Value->AsObject();
		const GG::FVec3 Center = ReadVec(Box, TEXT("center"));
		const GG::FVec3 Half = ReadVec(Box, TEXT("size")) * 0.5;
		Course.Boxes.push_back({Center - Half, Center + Half, ParseSurface(Box->GetStringField(TEXT("surface")))});
	}

	Attractors.clear();
	Movable.Reset();
	MachineBases.Reset();
	for (const TSharedPtr<FJsonValue>& Value : Json->GetArrayField(TEXT("attractors")))
	{
		const TSharedPtr<FJsonObject> A = Value->AsObject();
		const GG::FVec3 Base = ReadVec(A, TEXT("position"));
		CoreHeight = A->GetNumberField(TEXT("coreHeight"));
		Attractors.push_back({Base + GG::FVec3{0, 0, CoreHeight}, A->GetNumberField(TEXT("strength")), A->GetNumberField(TEXT("range"))});
		Movable.Add(A->GetBoolField(TEXT("movable")));
		MachineBases.Add(ToUnreal(Base));
	}
	InitialBases = MachineBases;

	BuildActors();
	ResetHole();
	UE_LOG(LogGGCourse, Log, TEXT("Loaded course %s: %d boxes, %d attractors"), *Name,
		static_cast<int32>(Course.Boxes.size()), static_cast<int32>(Attractors.size()));
	return true;
}

UStaticMeshComponent* AGGCourse::AddMesh(UStaticMesh* Mesh, UMaterialInterface* Material, const FTransform& Transform,
	USceneComponent* Parent)
{
	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this);
	Component->SetStaticMesh(Mesh);
	Component->SetMobility(EComponentMobility::Movable);
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->SetupAttachment(Parent ? Parent : Root.Get());
	if (Parent)
	{
		Component->SetRelativeTransform(Transform);
	}
	else
	{
		Component->SetWorldTransform(Transform);
	}
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

	// Safe zones: rings of light set into the ground. A ball that rests inside one stays.
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	for (const GG::FSafeZone& Zone : Course.SafeZones)
	{
		const double Scale = Zone.Radius / 50.0;
		AddMesh(Cylinder, Glow(FLinearColor(0.3f, 1.f, 0.55f), 3.f),
			FTransform(FRotator::ZeroRotator, ToUnreal(Zone.Center) + FVector(0, 0, 0.4), FVector(Scale, Scale, 0.004)))
			->SetCastShadow(false);
	}

	for (int32 I = 0; I < MachineBases.Num(); ++I)
	{
		BuildMachine(I);
	}

	auto MakeDots = [&](const FLinearColor& Color, float Strength) {
		UInstancedStaticMeshComponent* Dots = NewObject<UInstancedStaticMeshComponent>(this);
		Dots->SetStaticMesh(Sphere);
		Dots->SetMaterial(0, Glow(Color, Strength));
		Dots->SetMobility(EComponentMobility::Movable);
		Dots->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Dots->SetCastShadow(false);
		Dots->SetupAttachment(Root);
		Dots->RegisterComponent();
		return Dots;
	};
	TrailDots = MakeDots(FLinearColor(0.55f, 0.85f, 1.f), 6.f);
	GuideDots = MakeDots(FLinearColor(1.f, 0.92f, 0.7f), 8.f);
	FieldDots = MakeDots(FLinearColor(0.25f, 0.6f, 1.f), 16.f);
}

void AGGCourse::BuildMachine(int32 Index)
{
	// A tangible device, not an orb: three bronze legs, a housing, a ring, and the field's glowing heart above.
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UMaterialInterface* SurfaceBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_GG_Surface.M_GG_Surface"));

	UMaterialInstanceDynamic* Bronze = UMaterialInstanceDynamic::Create(SurfaceBase, this);
	Bronze->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.42f, 0.26f, 0.11f));
	Bronze->SetScalarParameterValue(TEXT("Metallic"), 1.f);
	Bronze->SetScalarParameterValue(TEXT("Roughness"), 0.38f);
	UMaterialInstanceDynamic* Iron = UMaterialInstanceDynamic::Create(SurfaceBase, this);
	Iron->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.03f, 0.03f, 0.035f));
	Iron->SetScalarParameterValue(TEXT("Metallic"), 1.f);
	Iron->SetScalarParameterValue(TEXT("Roughness"), 0.5f);

	const bool bPull = Attractors[Index].Strength > 0.0;
	const FLinearColor Color = bPull ? FLinearColor(0.2f, 0.55f, 1.f) : FLinearColor(1.f, 0.5f, 0.12f);

	USceneComponent* Machine = NewObject<USceneComponent>(this);
	Machine->SetMobility(EComponentMobility::Movable);
	Machine->SetupAttachment(Root);
	Machine->RegisterComponent();
	Machines.Add(Machine);

	for (int32 Leg = 0; Leg < 3; ++Leg)
	{
		const FRotator Yaw(0.f, 120.f * Leg + 30.f, 0.f);
		// Each leg leans out from the housing to a foot on the ground.
		const FVector Top = Yaw.RotateVector(FVector(7.0, 0.0, CoreHeight - 22.0));
		const FVector Foot = Yaw.RotateVector(FVector(34.0, 0.0, 0.0));
		const FVector Axis = (Top - Foot);
		AddMesh(Cylinder, Iron,
			FTransform(FRotationMatrix::MakeFromZ(Axis).Rotator(), (Top + Foot) * 0.5, FVector(0.035, 0.035, Axis.Size() / 100.0)),
			Machine);
		AddMesh(Sphere, Bronze, FTransform(FRotator::ZeroRotator, Foot + FVector(0, 0, 2.5), FVector(0.07)), Machine);
	}
	AddMesh(Cylinder, Bronze, FTransform(FRotator::ZeroRotator, FVector(0, 0, CoreHeight - 19.0), FVector(0.2, 0.2, 0.1)), Machine);
	AddMesh(Cylinder, Iron, FTransform(FRotator::ZeroRotator, FVector(0, 0, CoreHeight - 12.5), FVector(0.15, 0.15, 0.03)), Machine);
	AddMesh(Cylinder, Glow(Color, 25.f), FTransform(FRotator::ZeroRotator, FVector(0, 0, CoreHeight - 13.5), FVector(0.21, 0.21, 0.01)), Machine);
	for (int32 Prong = 0; Prong < 3; ++Prong)
	{
		const FVector At = FRotator(0.f, 120.f * Prong + 90.f, 0.f).RotateVector(FVector(12.0, 0.0, CoreHeight - 2.0));
		AddMesh(Cylinder, Bronze, FTransform(FRotator::ZeroRotator, At, FVector(0.018, 0.018, 0.2)), Machine);
	}
	UStaticMeshComponent* Core = AddMesh(Sphere, Glow(Color, 70.f),
		FTransform(FRotator::ZeroRotator, FVector(0, 0, CoreHeight), FVector(0.14)), Machine);
	Core->SetCastShadow(false);
	MachineCores.Add(Core);

	UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetupAttachment(Machine);
	Light->SetRelativeLocation(FVector(0, 0, CoreHeight));
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(260.f);
	Light->SetLightColor(Color);
	Light->SetAttenuationRadius(900.f);
	Light->SetSourceRadius(10.f);
	Light->RegisterComponent();
}

bool AGGCourse::CanPlaceMachine(const FVector& Base) const
{
	for (const FVector& Zone : NoMachineZones)
	{
		if (FVector::DistSquared2D(Base, Zone) < Zone.Z * Zone.Z)
		{
			return false;
		}
	}
	return true;
}

void AGGCourse::Notify(const FString& Message)
{
	Status = Message;
	NoticeTime = 2.5;
}

void AGGCourse::ReturnBall()
{
	// Call the ball back to the last safe spot, even mid-roll. Machines stay where they are.
	if (CurrentPhase == EGGPhase::Sunk)
	{
		return;
	}
	Lie = LastSafe;
	Trail.Reset();
	Status.Reset();
	CurrentPhase = EGGPhase::Ready;
}

void AGGCourse::ResetHole()
{
	Lie = Course.Tee;
	LastSafe = Course.Tee;
	PreviousPosition = Lie;
	StrokeCount = 0;
	Status.Reset();
	Trail.Reset();
	CurrentPhase = EGGPhase::Ready;
	for (int32 I = 0; I < Machines.Num(); ++I)
	{
		MoveMachine(I, InitialBases[I], false);
	}
}

int32 AGGCourse::FindMachine(const FVector& Near, double Reach) const
{
	int32 Best = INDEX_NONE;
	double BestDistance = Reach;
	for (int32 I = 0; I < Movable.Num(); ++I)
	{
		const double Distance = FVector::Dist(Near, MachineBases[I] + FVector(0, 0, CoreHeight * 0.6));
		if (Movable[I] && Distance < BestDistance)
		{
			Best = I;
			BestDistance = Distance;
		}
	}
	return Best;
}

void AGGCourse::MoveMachine(int32 Index, const FVector& Base, bool bCarried)
{
	if (!Machines.IsValidIndex(Index))
	{
		return;
	}
	// Whole centimeters keep the simulation's inputs identical on every machine.
	const FVector Snapped = bCarried ? Base
		: FVector(FMath::RoundToDouble(Base.X), FMath::RoundToDouble(Base.Y), FMath::RoundToDouble(Base.Z));
	MachineBases[Index] = Snapped;
	Attractors[Index].Position = ToSim(Snapped + FVector(0, 0, CoreHeight));
	Machines[Index]->SetWorldLocation(Snapped);
}

FVector AGGCourse::MachineBase(int32 Index) const
{
	return MachineBases.IsValidIndex(Index) ? MachineBases[Index] : FVector::ZeroVector;
}

GG::ESurface AGGCourse::SurfaceAt(const FVector& Location) const
{
	GG::ESurface Surface = GG::ESurface::Stone;
	double BestTop = -TNumericLimits<double>::Max();
	for (const GG::FBox& Box : Course.Boxes)
	{
		if (Location.X >= Box.Min.X && Location.X <= Box.Max.X && Location.Y >= Box.Min.Y && Location.Y <= Box.Max.Y &&
			Box.Max.Z <= Location.Z + 30.0 && Box.Max.Z > BestTop)
		{
			BestTop = Box.Max.Z;
			Surface = Box.Surface;
		}
	}
	return Surface;
}

void AGGCourse::ShowGuide(const TArray<FVector>& Points)
{
	GuideDots->ClearInstances();
	for (const FVector& Point : Points)
	{
		GuideDots->AddInstance(FTransform(FRotator::ZeroRotator, Point, FVector(0.035)), true);
	}
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
	GGAudio::Strike(this, ToUnreal(Lie), static_cast<float>(Power));
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
		Status = StrokeCount == 1 ? TEXT("Solved in one attempt!") : FString::Printf(TEXT("Solved in %d attempts"), StrokeCount);
		break;
	case GG::EBallState::AtRest:
		if (Sim.Ball().SafeZone >= 0)
		{
			// Resting in a safe zone banks the progress: the next swing starts here.
			Lie = Sim.Ball().Position;
			LastSafe = Lie;
			CurrentPhase = EGGPhase::Ready;
			Notify(TEXT("Safe."));
			break;
		}
		// One swing per leg: a ball that stops anywhere else goes back to the last safe spot.
		RestPosition = Sim.Ball().Position;
		Status = TEXT("Not in. The ball returns.");
		CurrentPhase = EGGPhase::Penalty;
		break;
	default:
		RestPosition = Sim.Ball().Position;
		Status = TEXT("Lost. The ball returns.");
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
	if (NoticeTime > 0.0)
	{
		NoticeTime -= DeltaSeconds;
		if (NoticeTime <= 0.0 && CurrentPhase == EGGPhase::Ready)
		{
			Status.Reset();
		}
	}

	if (CurrentPhase == EGGPhase::Flying)
	{
		Accumulator += DeltaSeconds * TimeScale;
		int32 Steps = 0;
		std::vector<GG::FEvent> Events;
		while (Accumulator >= GG::FSim::Dt && Steps < 480)
		{
			PreviousPosition = Sim.Ball().Position;
			Events.clear();
			Sim.Step(&Events);
			for (const GG::FEvent& Event : Events)
			{
				if (Event.Type == GG::EEventType::Bounce)
				{
					GGAudio::Bounce(this, ToUnreal(Event.Position), Event.Surface, static_cast<float>(Event.Speed));
				}
				else if (Event.Type == GG::EEventType::Sunk)
				{
					GGAudio::Sunk(this, ToUnreal(Event.Position));
				}
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
		if (Trail.Num() > 240)
		{
			Trail.RemoveAt(0);
		}
	}
	else if (CurrentPhase == EGGPhase::Penalty && PhaseTime > ReturnPause)
	{
		Status.Reset();
		Lie = LastSafe;
		CurrentPhase = EGGPhase::Ready;
		Trail.Reset();
	}

	BallMesh->SetWorldLocation(BallLocation());

	// Machines breathe.
	const float Pulse = 1.f + 0.08f * FMath::Sin(GetWorld()->GetTimeSeconds() * 2.2f);
	for (UStaticMeshComponent* Core : MachineCores)
	{
		Core->SetRelativeScale3D(FVector(0.14f * Pulse));
	}

	UpdateFieldPreview();

	// The ball draws its path in light, fading behind it.
	TrailDots->ClearInstances();
	for (int32 I = 0; I < Trail.Num(); I += 2)
	{
		const float Fade = static_cast<float>(I) / Trail.Num();
		TrailDots->AddInstance(FTransform(FRotator::ZeroRotator, Trail[I], FVector(0.012f + 0.035f * Fade)), true);
	}
}

void AGGCourse::UpdateFieldPreview()
{
	// Each machine shows its field on the ground: a ring at the dome's edge, and motes that stream inward along
	// the pull. Faster, larger motes mean a stronger pull, so the player can read a placement before swinging.
	FieldDots->ClearInstances();
	const double Time = GetWorld()->GetTimeSeconds();
	constexpr int32 RingDots = 96;
	constexpr int32 Spokes = 30;
	constexpr int32 MotesPerSpoke = 5;
	for (int32 Index = 0; Index < MachineBases.Num(); ++Index)
	{
		const FVector Base = MachineBases[Index];
		const double Range = Attractors[Index].Range;
		const bool bPull = Attractors[Index].Strength > 0.0;

		for (int32 I = 0; I < RingDots; ++I)
		{
			const double Angle = 2.0 * PI * I / RingDots;
			// Dashes laid along the circle, slowly turning, so the dome's edge reads as a boundary.
			const double Turn = Angle + Time * 0.05;
			const FVector Out(FMath::Cos(Turn), FMath::Sin(Turn), 0.0);
			const FVector Along(-Out.Y, Out.X, 0.0);
			FieldDots->AddInstance(FTransform(Along.Rotation(), Base + Out * Range + FVector(0, 0, 4.0),
				FVector(0.75, 0.1, 0.05)), true);
		}

		for (int32 Spoke = 0; Spoke < Spokes; ++Spoke)
		{
			const double Angle = 2.0 * PI * (Spoke + 0.5 * (Index % 2)) / Spokes;
			const FVector Radial(FMath::Cos(Angle), FMath::Sin(Angle), 0.0);
			for (int32 Mote = 0; Mote < MotesPerSpoke; ++Mote)
			{
				// Travel from the edge to the center (or outward for a repulsor), staggered per spoke.
				double Phase = FMath::Frac(Time * 0.22 + static_cast<double>(Mote) / MotesPerSpoke + Spoke * 0.381);
				if (!bPull)
				{
					Phase = 1.0 - Phase;
				}
				// Ease in, so motes speed up as the pull strengthens.
				const double R = Range * (1.0 - Phase * Phase);
				if (R < 90.0)
				{
					continue;
				}
				const double Window = 1.0 - (R * R) / (Range * Range);
				const double Size = 0.1 + 0.34 * Window;
				// Stretch each mote along the pull so it reads as a direction.
				FieldDots->AddInstance(FTransform(Radial.Rotation(), Base + Radial * R + FVector(0, 0, 4.0),
					FVector(Size * 3.0, Size * 0.45, 0.05)), true);
			}
		}
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
	if (CurrentPhase == EGGPhase::Penalty)
	{
		return ToUnreal(RestPosition);
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
