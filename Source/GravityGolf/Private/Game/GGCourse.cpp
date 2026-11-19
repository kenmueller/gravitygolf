#include "Game/GGCourse.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Animation/SkeletalMeshActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SkyLightComponent.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Game/GGAudio.h"
#include "Kismet/GameplayStatics.h"
#include "RHIGlobals.h"
#include "Game/GGGolfer.h"
#include "Net/UnrealNetwork.h"
#include "Game/GGSave.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
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

// Field preview look, tunable live from the console while playing.
TAutoConsoleVariable<float> CVarFieldGrid(TEXT("gg.Field.Grid"), 190.f, TEXT("Field preview: distance between arrows (cm)"));
TAutoConsoleVariable<float> CVarFieldSize(TEXT("gg.Field.Size"), 0.75f, TEXT("Field preview: arrow size multiplier"));
TAutoConsoleVariable<float> CVarFieldGlow(TEXT("gg.Field.Glow"), 1.1f, TEXT("Field preview: brightness when emphasized"));
TAutoConsoleVariable<float> CVarMachineLight(TEXT("gg.Machine.Light"), 6.f,
	TEXT("Machine: brightness of the light it casts (candelas). The scenes are exposed for a dim sun, so this is low."));
TAutoConsoleVariable<float> CVarMachineCore(TEXT("gg.Machine.Core"), 3.f, TEXT("Machine: glow of its core"));
TAutoConsoleVariable<float> CVarFieldSpeed(TEXT("gg.Field.Speed"), 0.12f, TEXT("Field preview: flow speed"));

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

namespace
{
TAutoConsoleVariable<int32> CVarPlantStandIns(TEXT("gg.Plants.StandIns"), 0,
	TEXT("1 spawns the simple stand-in plants even where the scanned ones could be drawn (to preview the web look)"));
}

void AGGCourse::SpawnPlants(const FString& LevelName)
{
	FString Text;
	TSharedPtr<FJsonObject> Json;
	const FString Path = FPaths::ProjectContentDir() / TEXT("Courses") / (LevelName.ToLower() + TEXT(".plants.json"));
	if (!FFileHelper::LoadFileToString(Text, *Path) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) || !Json.IsValid())
	{
		return;
	}
	// The scans are Nanite foliage on swaying skeletons: desktop renderers only.
	const bool bScans = GMaxRHIFeatureLevel >= ERHIFeatureLevel::SM5 && CVarPlantStandIns.GetValueOnGameThread() == 0;

	// Where there is no bounced light (the mobile renderer: the web, phones) the sky light is the only fill, and at
	// its desktop strength everything in shadow goes black. Each level says how much to raise it and what colour its
	// ground would throw back.
	const TSharedPtr<FJsonObject>* Web = nullptr;
	if (GMaxRHIFeatureLevel < ERHIFeatureLevel::SM5 && Json->TryGetObjectField(TEXT("web"), Web))
	{
		const double Ambient = (*Web)->GetNumberField(TEXT("ambient"));
		const TArray<TSharedPtr<FJsonValue>>& Tint = (*Web)->GetArrayField(TEXT("tint"));
		for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It)
		{
			USkyLightComponent* Sky = It->GetLightComponent();
			Sky->SetIntensity(Sky->Intensity * static_cast<float>(Ambient));
			Sky->SetLightColor(FLinearColor(Tint[0]->AsNumber(), Tint[1]->AsNumber(), Tint[2]->AsNumber()));
		}
	}
	UStaticMesh* Pine = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Meshes/SM_Web_Pine.SM_Web_Pine"));
	UStaticMesh* Shrub = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Meshes/SM_Web_Shrub.SM_Web_Shrub"));
	UMaterialInterface* Surface = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_GG_Surface.M_GG_Surface"));
	auto Paint = [&](const FLinearColor& Color) -> UMaterialInstanceDynamic* {
		UMaterialInstanceDynamic* Material = Surface ? UMaterialInstanceDynamic::Create(Surface, this) : nullptr;
		if (Material)
		{
			Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
			Material->SetScalarParameterValue(TEXT("Roughness"), 0.9f);
			Material->SetScalarParameterValue(TEXT("Metallic"), 0.f);
		}
		return Material;
	};
	UMaterialInstanceDynamic* Bark = Paint(FLinearColor(0.07f, 0.05f, 0.035f));
	UMaterialInstanceDynamic* Needles = Paint(FLinearColor(0.035f, 0.085f, 0.03f));
	UMaterialInstanceDynamic* Heather = Paint(FLinearColor(0.2f, 0.1f, 0.17f));
	UMaterialInstanceDynamic* Rose = Paint(FLinearColor(0.05f, 0.11f, 0.04f));

	int32 Scans = 0;
	int32 StandIns = 0;
	const TArray<TSharedPtr<FJsonValue>>* Plants = nullptr;
	if (!Json->TryGetArrayField(TEXT("plants"), Plants))
	{
		return;
	}
	for (const TSharedPtr<FJsonValue>& Value : *Plants)
	{
		const TSharedPtr<FJsonObject> Plant = Value->AsObject();
		const TArray<TSharedPtr<FJsonValue>>& At = Plant->GetArrayField(TEXT("position"));
		const TArray<TSharedPtr<FJsonValue>>& Extent = Plant->GetArrayField(TEXT("extent"));
		const FVector Location(At[0]->AsNumber(), At[1]->AsNumber(), At[2]->AsNumber());
		const FRotator Rotation(0.0, Plant->GetNumberField(TEXT("yaw")), 0.0);
		const double Scale = Plant->GetNumberField(TEXT("scale"));
		const FString Kind = Plant->GetStringField(TEXT("kind"));
		const bool bShadow = Plant->GetBoolField(TEXT("shadow"));

		USkeletalMesh* Scan = bScans ? LoadObject<USkeletalMesh>(nullptr, *Plant->GetStringField(TEXT("asset"))) : nullptr;
		if (Scan)
		{
			ASkeletalMeshActor* Actor = GetWorld()->SpawnActor<ASkeletalMeshActor>(Location, Rotation);
			Actor->GetSkeletalMeshComponent()->SetSkeletalMeshAsset(Scan);
			Actor->GetSkeletalMeshComponent()->SetCastShadow(bShadow);
			Actor->SetActorScale3D(FVector(Scale));
			++Scans;
			continue;
		}
		UStaticMesh* Mesh = Kind == TEXT("pine") ? Pine : Shrub;
		if (!Mesh)
		{
			continue;
		}
		// The stand-in is built 200 across and 200 tall; stretch it to this plant's real size.
		AStaticMeshActor* Actor = GetWorld()->SpawnActor<AStaticMeshActor>(Location, Rotation);
		UStaticMeshComponent* Component = Actor->GetStaticMeshComponent();
		Component->SetMobility(EComponentMobility::Movable);
		Component->SetStaticMesh(Mesh);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetCastShadow(bShadow);
		Component->SetMaterial(0, Bark);
		Component->SetMaterial(1, Kind == TEXT("pine") ? Needles : Kind == TEXT("heather") ? Heather : Rose);
		Actor->SetActorScale3D(FVector(Extent[0]->AsNumber(), Extent[1]->AsNumber(), Extent[2]->AsNumber()) * (Scale / 100.0));
		++StandIns;
	}
	UE_LOG(LogGGCourse, Display, TEXT("Plants: %d scanned, %d stand-ins"), Scans, StandIns);
}

AGGCourse::AGGCourse()
{
	PrimaryActorTick.bCanEverTick = true;
	// Every player's machine has its own copy of the hole, kept in step by the server's commands.
	bReplicates = true;
	bAlwaysRelevant = true;
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
	if (!FFileHelper::LoadFileToString(Text, *CoursePath(CourseId)))
	{
		UE_LOG(LogGGCourse, Error, TEXT("Can't load course %s"), *CoursePath(CourseId));
		return false;
	}
	return LoadText(Text, CourseId);
}

void AGGCourse::Unload()
{
	TInlineComponentArray<UActorComponent*> Built(this);
	for (UActorComponent* Component : Built)
	{
		if (Component != Root)
		{
			Component->DestroyComponent();
		}
	}
	Course = GG::FCourse();
	Attractors.clear();
	MachineBases.Reset();
	InitialBases.Reset();
	Movable.Reset();
	Carriers.Reset();
	Machines.Reset();
	MachineCores.Reset();
	MachineLights.Reset();
	SwitchMaterials.Reset();
	GateBars.Reset();
	GateLevel.Reset();
	Solutions.Reset();
	DrawnBoxes.Reset();
	Played.Reset();
	bPendingValid = false;
	bDrawn = false;
	NoMachineZones.Reset();
	Trail.Reset();
	BallMesh = nullptr;
	TrailDots = GuideDots = FieldDots = FieldRing = FieldPushDots = FieldPushRing = nullptr;
	Id.Reset();
}

bool AGGCourse::LoadText(const FString& Text, const FString& CourseId)
{
	TSharedPtr<FJsonObject> Json;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) || !Json.IsValid())
	{
		UE_LOG(LogGGCourse, Error, TEXT("Course %s is not valid JSON"), *CourseId);
		return false;
	}
	if (!Id.IsEmpty())
	{
		Unload();
	}

	Course = GG::FCourse();
	Id = CourseId.ToLower();
	if (HasAuthority())
	{
		NetCourseId = Id;
	}
	Name = Json->GetStringField(TEXT("name"));
	bDrawn = false;
	Json->TryGetBoolField(TEXT("drawn"), bDrawn);
	NextLevel.Reset();
	Json->TryGetStringField(TEXT("next"), NextLevel);
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

	const TArray<TSharedPtr<FJsonValue>>* Switches = nullptr;
	if (Json->TryGetArrayField(TEXT("switches"), Switches))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Switches)
		{
			bool bWeight = false;
			Value->AsObject()->TryGetBoolField(TEXT("weight"), bWeight);
			Course.Switches.push_back({ReadVec(Value->AsObject(), TEXT("position")), Value->AsObject()->GetNumberField(TEXT("radius")), bWeight});
		}
	}
	const TArray<TSharedPtr<FJsonValue>>* Gates = nullptr;
	if (Json->TryGetArrayField(TEXT("gates"), Gates))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Gates)
		{
			const TSharedPtr<FJsonObject> G = Value->AsObject();
			const GG::FVec3 Center = ReadVec(G, TEXT("center"));
			const GG::FVec3 Half = ReadVec(G, TEXT("size")) * 0.5;
			GG::FGate Gate;
			Gate.Box = {Center - Half, Center + Half, ParseSurface(G->GetStringField(TEXT("surface")))};
			Gate.Switch = G->GetIntegerField(TEXT("switch"));
			Gate.OpenSeconds = G->GetNumberField(TEXT("openSeconds"));
			Course.Gates.push_back(Gate);
		}
	}
	HeldSwitches.assign(Course.Switches.size(), false);

	Solutions.Reset();
	const TArray<TSharedPtr<FJsonValue>>* Answers = nullptr;
	if (Json->TryGetArrayField(TEXT("solutions"), Answers))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Answers)
		{
			const TSharedPtr<FJsonObject> A = Value->AsObject();
			FGGSolution Solution;
			Solution.Leg = A->GetIntegerField(TEXT("leg"));
			Solution.Yaw = A->GetNumberField(TEXT("yaw"));
			Solution.Power = A->GetNumberField(TEXT("power"));
			for (const TSharedPtr<FJsonValue>& Spot : A->GetArrayField(TEXT("machines")))
			{
				const TArray<TSharedPtr<FJsonValue>>& XY = Spot->AsArray();
				Solution.Machines.Add(FVector(XY[0]->AsNumber(), XY[1]->AsNumber(), 0.0));
			}
			Solutions.Add(Solution);
		}
	}

	for (const TSharedPtr<FJsonValue>& Value : Json->GetArrayField(TEXT("boxes")))
	{
		const TSharedPtr<FJsonObject> Box = Value->AsObject();
		const GG::FVec3 Center = ReadVec(Box, TEXT("center"));
		const GG::FVec3 Half = ReadVec(Box, TEXT("size")) * 0.5;
		Course.Boxes.push_back({Center - Half, Center + Half, ParseSurface(Box->GetStringField(TEXT("surface")))});
		if (bDrawn)
		{
			FString Look;
			Box->TryGetStringField(TEXT("look"), Look);
			DrawnBoxes.Add({ToUnreal(Center), ToUnreal(Half) * 2.0, Look});
		}
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
	Carriers.Init(nullptr, MachineBases.Num());

	BuildActors();
	// Set this machine's copy to the start of the hole. (Not a request to the server: every machine loads its own.)
	NetResetHole_Implementation();
	UE_LOG(LogGGCourse, Log, TEXT("Loaded course %s: %d boxes, %d attractors"), *Name,
		static_cast<int32>(Course.Boxes.size()), static_cast<int32>(Attractors.size()));
	if (!bPlantsSpawned)
	{
		bPlantsSpawned = true;
		SpawnPlants(UGameplayStatics::GetCurrentLevelName(this, true));
	}
	return true;
}

GG::FSetup AGGCourse::MakeSetup() const
{
	GG::FSetup Setup;
	Setup.Course = Course;
	bool bAny = false;
	for (const GG::FBox& Box : Course.Boxes)
	{
		// The ground is every box whose top is about where the ball rolls.
		if (Box.Max.Z < 20.0)
		{
			Setup.MinX = bAny ? FMath::Min(Setup.MinX, Box.Min.X) : Box.Min.X;
			Setup.MinY = bAny ? FMath::Min(Setup.MinY, Box.Min.Y) : Box.Min.Y;
			Setup.MaxX = bAny ? FMath::Max(Setup.MaxX, Box.Max.X) : Box.Max.X;
			Setup.MaxY = bAny ? FMath::Max(Setup.MaxY, Box.Max.Y) : Box.Max.Y;
			bAny = true;
		}
	}
	for (int32 I = 0; I < MachineBases.Num(); ++I)
	{
		Setup.Machines.push_back({Attractors[I].Strength, Attractors[I].Range, CoreHeight, InitialBases[I].X, InitialBases[I].Y});
	}
	for (const FVector& Zone : NoMachineZones)
	{
		Setup.NoMachine.push_back({Zone.X, Zone.Y, Zone.Z});
	}
	Setup.ZoneNeeds.assign(Course.SafeZones.size(), -1);
	for (const FGGSolution& Answer : Solutions)
	{
		GG::FHint Hint;
		Hint.Leg = Answer.Leg;
		Hint.Yaw = Answer.Yaw;
		Hint.Power = Answer.Power;
		for (const FVector& Spot : Answer.Machines)
		{
			Hint.Spots.push_back({Spot.X, Spot.Y, 0.0});
		}
		Setup.Hints.push_back(Hint);
	}
	return Setup;
}

void AGGCourse::BuildDrawn()
{
	// A hole from the Workshop has no level dressed around it: it draws its own ground, walls and markers, and gives
	// them collision so the player can walk the hole.
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UMaterialInterface* SurfaceBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_GG_Surface.M_GG_Surface"));
	TMap<FString, UMaterialInterface*> Looks;
	auto LookFor = [&](const FString& Look) -> UMaterialInterface* {
		if (UMaterialInterface** Found = Looks.Find(Look))
		{
			return *Found;
		}
		// The Workshop's level script makes one material per look (Tools/levels/workshop.py). Plain colours stand in
		// when it hasn't been run.
		UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr,
			*FString::Printf(TEXT("/Game/Materials/Instances/MI_Workshop_%s.MI_Workshop_%s"), *Look, *Look));
		if (!Material)
		{
			UMaterialInstanceDynamic* Plain = UMaterialInstanceDynamic::Create(SurfaceBase, this);
			const FLinearColor Color = Look == TEXT("floor") ? FLinearColor(0.42f, 0.44f, 0.4f)
				: Look == TEXT("sand") || Look == TEXT("pad") ? FLinearColor(0.55f, 0.42f, 0.25f)
				: Look == TEXT("soft") ? FLinearColor(0.5f, 0.36f, 0.22f)
				: FLinearColor(0.6f, 0.6f, 0.62f);
			Plain->SetVectorParameterValue(TEXT("BaseColor"), Color);
			Plain->SetScalarParameterValue(TEXT("Roughness"), 0.8f);
			Material = Plain;
		}
		Looks.Add(Look, Material);
		return Material;
	};
	auto Solid = [](UStaticMeshComponent* Component) {
		Component->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Component->SetCollisionResponseToAllChannels(ECR_Block);
	};
	for (const FDrawnBox& Box : DrawnBoxes)
	{
		FVector Center = Box.Center;
		FVector Size = Box.Size;
		if (Box.Look == TEXT("pad"))
		{
			// A safe pad's soft ground is a square in the simulation and a round patch to the eye (drawn below).
			continue;
		}
		if (Box.Look == TEXT("sand"))
		{
			// Sand lies in the floor. Drawn a hair proud of it so the two surfaces don't flicker.
			Center.Z += 0.6;
		}
		UStaticMeshComponent* Mesh = AddMesh(Cube, LookFor(Box.Look), FTransform(FRotator::ZeroRotator, Center, Size / 100.0));
		Solid(Mesh);
		if (Box.Look == TEXT("wall"))
		{
			// A dark top, so a white wall on white ground reads at once from above.
			AddMesh(Cube, LookFor(TEXT("cap")), FTransform(FRotator::ZeroRotator, FVector(Center.X, Center.Y, Center.Z + Size.Z * 0.5 + 1.5),
				FVector(Size.X, Size.Y, 3.0) / 100.0));
		}
	}

	UMaterialInstanceDynamic* Black = UMaterialInstanceDynamic::Create(SurfaceBase, this);
	Black->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.004f, 0.004f, 0.004f));
	Black->SetScalarParameterValue(TEXT("Roughness"), 1.f);
	UMaterialInterface* Stone = LookFor(TEXT("cap"));
	const FVector Tee = ToUnreal(Course.Tee);
	// The tee: a small dark disc in a ring of light.
	AddMesh(Cylinder, Glow(FLinearColor(1.f, 0.9f, 0.7f), 3.f), FTransform(FRotator::ZeroRotator, FVector(Tee.X, Tee.Y, 0.5), FVector(0.72, 0.72, 0.008)))
		->SetCastShadow(false);
	AddMesh(Cylinder, Stone, FTransform(FRotator::ZeroRotator, FVector(Tee.X, Tee.Y, 0.7), FVector(0.62, 0.62, 0.008)))->SetCastShadow(false);
	const FVector Cup = ToUnreal(Course.Cup);
	const double Mouth = Course.CupRadius / 50.0;
	AddMesh(Cylinder, Glow(FLinearColor(1.f, 0.55f, 0.25f), 6.f), FTransform(FRotator::ZeroRotator, Cup + FVector(0, 0, 0.9), FVector(Mouth + 0.1, Mouth + 0.1, 0.01)))
		->SetCastShadow(false);
	AddMesh(Cylinder, Black, FTransform(FRotator::ZeroRotator, Cup + FVector(0, 0, 1.0), FVector(Mouth, Mouth, 0.01)))->SetCastShadow(false);
	AddMesh(Cylinder, Stone, FTransform(FRotator::ZeroRotator, Cup + FVector(0, 0, 130), FVector(0.025, 0.025, 2.6)));
	// Safe pads: a round patch of dark felt, a little smaller than the disc of light the course lays under it, so
	// the light shows as a ring around it.
	for (const GG::FSafeZone& Zone : Course.SafeZones)
	{
		const double Scale = (Zone.Radius - 9.0) / 50.0;
		AddMesh(Cylinder, LookFor(TEXT("pad")), FTransform(FRotator::ZeroRotator, ToUnreal(Zone.Center) + FVector(0, 0, 0.8), FVector(Scale, Scale, 0.004)))
			->SetCastShadow(false);
	}
	// The court around the cup, where machines can't stand: a thin ring of light.
	for (const FVector& Zone : NoMachineZones)
	{
		if (FVector::DistSquared2D(Zone, Cup) < 1.0)
		{
			UInstancedStaticMeshComponent* Edge = NewObject<UInstancedStaticMeshComponent>(this);
			Edge->SetStaticMesh(Cube);
			Edge->SetMaterial(0, Glow(FLinearColor(1.f, 0.55f, 0.25f), 3.f));
			Edge->SetMobility(EComponentMobility::Movable);
			Edge->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Edge->SetCastShadow(false);
			Edge->SetupAttachment(Root);
			Edge->RegisterComponent();
			constexpr int32 Marks = 72;
			for (int32 I = 0; I < Marks; ++I)
			{
				const double Angle = 2.0 * UE_DOUBLE_PI * I / Marks;
				const FVector At(Zone.X + FMath::Cos(Angle) * Zone.Z, Zone.Y + FMath::Sin(Angle) * Zone.Z, 1.2);
				Edge->AddInstance(FTransform(FRotator(0.0, FMath::RadiansToDegrees(Angle) + 90.0, 0.0), At,
					FVector(Zone.Z * 2.0 * UE_DOUBLE_PI / Marks * 0.55 / 100.0, 0.04, 0.012)), true);
			}
		}
	}
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
	if (bDrawn)
	{
		BuildDrawn();
	}

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

	// Switch plates. A light plate is a disc set into the ground that lights when the ball rolls over it. A weight
	// plate is a heavy square slab with a lit rim, plainly something to stand a machine on.
	UStaticMesh* Slab = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	for (const GG::FSwitch& Switch : Course.Switches)
	{
		const double Scale = Switch.Radius / 50.0;
		UMaterialInstanceDynamic* Material = Glow(FLinearColor(1.f, 0.6f, 0.15f), 1.5f);
		SwitchMaterials.Add(Material);
		if (Switch.bWeight)
		{
			UMaterialInstanceDynamic* Iron = UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_GG_Surface.M_GG_Surface")), this);
			Iron->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.05f, 0.05f, 0.055f));
			Iron->SetScalarParameterValue(TEXT("Metallic"), 1.f);
			Iron->SetScalarParameterValue(TEXT("Roughness"), 0.45f);
			const double Side = Switch.Radius * 1.7 / 100.0;
			AddMesh(Slab, Material, FTransform(FRotator::ZeroRotator, ToUnreal(Switch.Center) + FVector(0, 0, 0.6), FVector(Side + 0.14, Side + 0.14, 0.008)))
				->SetCastShadow(false);
			AddMesh(Slab, Iron, FTransform(FRotator::ZeroRotator, ToUnreal(Switch.Center) + FVector(0, 0, 1.0), FVector(Side, Side, 0.016)))
				->SetCastShadow(false);
			continue;
		}
		AddMesh(Cylinder, Material, FTransform(FRotator::ZeroRotator, ToUnreal(Switch.Center) + FVector(0, 0, 0.5), FVector(Scale, Scale, 0.006)))
			->SetCastShadow(false);
	}

	// Gates: bars of light across an opening. They drop into the ground while the gate is open.
	for (const GG::FGate& Gate : Course.Gates)
	{
		USceneComponent* Bars = NewObject<USceneComponent>(this);
		Bars->SetMobility(EComponentMobility::Movable);
		Bars->SetupAttachment(Root);
		Bars->RegisterComponent();
		const FVector Min = ToUnreal(Gate.Box.Min);
		const FVector Max = ToUnreal(Gate.Box.Max);
		const bool bAlongY = (Max.Y - Min.Y) > (Max.X - Min.X);
		const double Length = bAlongY ? Max.Y - Min.Y : Max.X - Min.X;
		const double Height = Max.Z - Min.Z;
		const FVector Center = (Min + Max) * 0.5;
		Bars->SetWorldLocation(FVector(Center.X, Center.Y, Min.Z));
		UMaterialInstanceDynamic* Light = Glow(FLinearColor(1.f, 0.5f, 0.12f), 14.f);
		const int32 Count = FMath::Max(3, FMath::RoundToInt32(Length / 42.0));
		for (int32 Bar = 0; Bar < Count; ++Bar)
		{
			const double Along = (Bar + 0.5) / Count * Length - Length * 0.5;
			const FVector At = bAlongY ? FVector(0, Along, Height * 0.5) : FVector(Along, 0, Height * 0.5);
			AddMesh(Cylinder, Light, FTransform(FRotator::ZeroRotator, At, FVector(0.035, 0.035, Height / 100.0)), Bars)->SetCastShadow(false);
		}
		GateBars.Add(Bars);
		GateLevel.Add(1.f);
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
		// These markers are rebuilt every frame. Left in the lighting scene, Lumen re-captures every one of them each
		// frame (it cost 6 to 14 ms of GPU time); they are tiny lights that the scene doesn't need to bounce.
		Dots->bAffectDynamicIndirectLighting = false;
		Dots->bAffectDistanceFieldLighting = false;
		Dots->bVisibleInRayTracing = false;
		Dots->SetupAttachment(Root);
		Dots->RegisterComponent();
		return Dots;
	};
	TrailDots = MakeDots(FLinearColor(0.55f, 0.85f, 1.f), 6.f);
	GuideDots = MakeDots(FLinearColor(1.f, 0.92f, 0.7f), 8.f);
	FieldRing = MakeDots(FLinearColor(0.25f, 0.6f, 1.f), 4.f);
	FieldDots = MakeDots(FLinearColor(0.25f, 0.6f, 1.f), 4.f);
	FieldDots->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone")));
	FieldPushDots = MakeDots(FLinearColor(1.f, 0.5f, 0.12f), 4.f);
	FieldPushDots->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone")));
	FieldPushRing = MakeDots(FLinearColor(1.f, 0.5f, 0.12f), 4.f);
	MachineFlash.Init(0.f, MachineBases.Num());
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

	// The machine's real mesh is generated by Tools/build_meshes.py (slots: iron, bronze, light), built for a core
	// 45 cm up. Without it, fall back to basic shapes.
	if (UStaticMesh* Body = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Meshes/SM_Machine.SM_Machine")))
	{
		UStaticMeshComponent* Part = AddMesh(Body, Iron, FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(1.0, 1.0, CoreHeight / 45.0)), Machine);
		Part->SetMaterial(1, Bronze);
		Part->SetMaterial(2, Glow(Color, 6.f));
	}
	else
	{
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
	Light->SetIntensity(CVarMachineLight.GetValueOnGameThread());
	Light->SetLightColor(Color);
	Light->SetAttenuationRadius(900.f);
	Light->SetSourceRadius(10.f);
	Light->RegisterComponent();
	MachineLights.Add(Light);
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

void AGGCourse::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGGCourse, NetCourseId);
}

void AGGCourse::OnRep_CourseId()
{
	// A joining player's copy of the hole: load the same course the server did.
	if (Id.IsEmpty() && !NetCourseId.IsEmpty())
	{
		Load(NetCourseId);
	}
}

void AGGCourse::Flush()
{
	if (CurrentPhase == EGGPhase::Flying)
	{
		// The server's ball has already stopped; let ours catch up at once.
		FinishStroke(Sim.Run());
	}
	if (CurrentPhase == EGGPhase::Penalty)
	{
		Status.Reset();
		Lie = LastSafe;
		CurrentPhase = EGGPhase::Ready;
		Trail.Reset();
	}
}

std::vector<GG::FAttractor> AGGCourse::ActiveMachines() const
{
	std::vector<GG::FAttractor> Active = Attractors;
	for (int32 I = 0; I < Carriers.Num() && I < static_cast<int32>(Active.size()); ++I)
	{
		if (Carriers[I].IsValid())
		{
			// In someone's hands: off, and not standing on anything.
			Active[I].Strength = 0.0;
			Active[I].Position = GG::FVec3{1.0e9, 1.0e9, 1.0e9};
		}
	}
	return Active;
}

FString AGGCourse::Describe() const
{
	FString Where;
	for (int32 I = 0; I < MachineBases.Num(); ++I)
	{
		Where += Carriers[I].IsValid() ? FString(TEXT(" carried")) : FString::Printf(TEXT(" (%.0f,%.0f)"), MachineBases[I].X, MachineBases[I].Y);
	}
	FString Held;
	for (const bool bHeld : HeldSwitches)
	{
		Held += bHeld ? TEXT("1") : TEXT("0");
	}
	const FVector Ball = BallLocation();
	return FString::Printf(TEXT("phase %d swings %d leg %d ball (%.1f,%.1f) machines%s held [%s]"), static_cast<int32>(CurrentPhase),
		StrokeCount, CurrentLeg, Ball.X, Ball.Y, *Where, *Held);
}

AGGGolfer* AGGCourse::CarrierOf(int32 Index) const
{
	return Carriers.IsValidIndex(Index) ? Carriers[Index].Get() : nullptr;
}

void AGGCourse::MarkAssisted()
{
	if (HasAuthority())
	{
		NetAssisted();
	}
}

void AGGCourse::NetAssisted_Implementation()
{
	bAssisted = true;
}

void AGGCourse::ReturnBall()
{
	// Call the ball back to the last safe spot, even mid-roll. Machines stay where they are.
	if (HasAuthority() && CurrentPhase != EGGPhase::Sunk)
	{
		NetReturnBall(CurrentPhase == EGGPhase::Flying);
	}
}

void AGGCourse::NetReturnBall_Implementation(bool bAbort)
{
	if (!bAbort)
	{
		Flush();
	}
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
	if (HasAuthority())
	{
		NetResetHole();
	}
}

void AGGCourse::NetResetHole_Implementation()
{
	bAssisted = false;
	Clock = 0.0;
	Lie = Course.Tee;
	LastSafe = Course.Tee;
	HeldSwitches.assign(Course.Switches.size(), false);
	CurrentLeg = 0;
	PreviousPosition = Lie;
	StrokeCount = 0;
	Status.Reset();
	Trail.Reset();
	CurrentPhase = EGGPhase::Ready;
	for (int32 I = 0; I < Machines.Num(); ++I)
	{
		if (AGGGolfer* Carrier = CarrierOf(I))
		{
			Carrier->SetCarried(INDEX_NONE);
		}
		Carriers[I] = nullptr;
		MoveMachine(I, InitialBases[I], false);
	}
}

void AGGCourse::CarryMachine(int32 Index, AGGGolfer* Carrier)
{
	if (HasAuthority() && Machines.IsValidIndex(Index) && Movable[Index] && Carrier && !CarrierOf(Index) && !Carrier->IsCarrying()
		&& CurrentPhase != EGGPhase::Flying)
	{
		NetCarry(Index, Carrier);
	}
}

void AGGCourse::NetCarry_Implementation(int32 Index, AGGGolfer* Carrier)
{
	if (!Machines.IsValidIndex(Index))
	{
		return;
	}
	Carriers[Index] = Carrier;
	if (Carrier)
	{
		Carrier->SetCarried(Index);
	}
	GGAudio::MachinePickUp(this, MachineBases[Index]);
}

bool AGGCourse::PlaceMachine(int32 Index, const FVector& Base, AGGGolfer* Carrier)
{
	if (!HasAuthority() || !Machines.IsValidIndex(Index) || (Carrier && CarrierOf(Index) != Carrier))
	{
		return false;
	}
	if (!CanPlaceMachine(Base))
	{
		if (Carrier)
		{
			Carrier->Tell(TEXT("The machine won't stand on the court."));
		}
		return false;
	}
	// Whole centimeters keep the simulation's inputs identical on every machine.
	NetPlace(Index, FVector(FMath::RoundToDouble(Base.X), FMath::RoundToDouble(Base.Y), FMath::RoundToDouble(Base.Z)));
	return true;
}

void AGGCourse::NetPlace_Implementation(int32 Index, FVector Base)
{
	if (!Machines.IsValidIndex(Index))
	{
		return;
	}
	if (AGGGolfer* Carrier = CarrierOf(Index))
	{
		Carrier->SetCarried(INDEX_NONE);
	}
	Carriers[Index] = nullptr;
	MoveMachine(Index, Base, false);
	GGAudio::MachinePlace(this, Base);
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

void AGGCourse::Swing(const FVector& Direction, double Power, AGGGolfer* Swinger)
{
	if (!HasAuthority() || CurrentPhase != EGGPhase::Ready)
	{
		return;
	}
	// The server works out the launch once, and everyone simulates from exactly that.
	NetSwing(Direction.GetSafeNormal() * (FMath::Clamp(Power, 0.02, 1.0) * Physics.MaxLaunchSpeed), Swinger);
}

void AGGCourse::NetSwing_Implementation(FVector Velocity, AGGGolfer* Swinger)
{
	Flush();
	if (CurrentPhase != EGGPhase::Ready)
	{
		return;
	}
	++StrokeCount;
	Status.Reset();
	// Presses banked on earlier legs, plus every weight plate a machine is standing on.
	const std::vector<GG::FAttractor> Active = ActiveMachines();
	std::vector<bool> Held = HeldSwitches;
	GG::HoldWeightPlates(Course, Active, Held);
	Sim.Begin(Course, Active, Lie, ToSim(Velocity), Physics, &Held);
	// Remember the swing: if it finishes the leg, it is an answer to it. (Not while a machine is being carried: a
	// carried machine is off, and an answer has to say where every machine stands.)
	Pending = FGGSolution();
	Pending.Leg = CurrentLeg;
	Pending.Yaw = FMath::RadiansToDegrees(FMath::Atan2(Velocity.Y, Velocity.X));
	Pending.Power = Velocity.Size2D() / Physics.MaxLaunchSpeed;
	Pending.Machines = MachineBases;
	for (const bool bHeld : HeldSwitches)
	{
		Pending.Held.Add(bHeld);
	}
	bPendingValid = true;
	for (int32 I = 0; I < Carriers.Num(); ++I)
	{
		bPendingValid &= CarrierOf(I) == nullptr;
	}
	PreviousPosition = Lie;
	Accumulator = 0.0;
	Trail.Reset();
	CurrentPhase = EGGPhase::Flying;
	PhaseTime = 0.0;
	GGAudio::Strike(this, ToUnreal(Lie), static_cast<float>(Velocity.Size() / Physics.MaxLaunchSpeed));
	if (Swinger)
	{
		Swinger->ShowSwing();
	}
	UE_LOG(LogGGCourse, Log, TEXT("Stroke %d: launch %s"), StrokeCount, *Velocity.ToCompactString());
}

void AGGCourse::PreviewSwing(const FVector& Direction, double Power, double Seconds, TArray<FVector>& OutPoints) const
{
	OutPoints.Reset();
	GG::FSim Preview;
	const FVector Velocity = Direction.GetSafeNormal() * (FMath::Clamp(Power, 0.02, 1.0) * Physics.MaxLaunchSpeed);
	const std::vector<GG::FAttractor> Active = ActiveMachines();
	std::vector<bool> Held = HeldSwitches;
	GG::HoldWeightPlates(Course, Active, Held);
	Preview.Begin(Course, Active, Lie, ToSim(Velocity), Physics, &Held);
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
	// A swing that finished its leg (reached the next pad in order, or the cup from the last pad) is kept as an answer.
	const int32 Zones = static_cast<int32>(Course.SafeZones.size());
	const bool bLegDone = State == GG::EBallState::Sunk ? Pending.Leg == Zones
		: State == GG::EBallState::AtRest && Sim.Ball().SafeZone == Pending.Leg;
	if (bPendingValid && bLegDone)
	{
		Played.RemoveAll([&](const FGGSolution& Old) { return Old.Leg == Pending.Leg; });
		Played.Add(Pending);
		++PlayedCount;
	}
	bPendingValid = false;
	switch (State)
	{
	case GG::EBallState::Sunk:
		CurrentPhase = EGGPhase::Sunk;
		{
			// A perfect solve takes one swing per leg.
			const int32 Legs = static_cast<int32>(Course.SafeZones.size()) + 1;
			Status = StrokeCount <= Legs ? FString::Printf(TEXT("Solved. Perfect: %d swing%s"), StrokeCount, StrokeCount == 1 ? TEXT("") : TEXT("s"))
										 : FString::Printf(TEXT("Solved in %d swings (perfect is %d)"), StrokeCount, Legs);
			if (!bAssisted && !bDrawn)
			{
				UGGSave::Get().RecordSolve(Id, StrokeCount);
			}
		}
		break;
	case GG::EBallState::AtRest:
		if (Sim.Ball().SafeZone >= 0)
		{
			// Resting in a safe zone banks the progress. The pad settles the ball at its center, so the next
			// swing always starts from the same known spot (which is also what the solver proves from).
			for (size_t I = 0; I < HeldSwitches.size(); ++I)
			{
				// A plate pressed on the way to a pad stays pressed for the rest of the hole.
				// A weight plate is never banked: it is down only while a machine stands on it.
				HeldSwitches[I] = HeldSwitches[I] || (!Course.Switches[I].bWeight && Sim.SwitchPressed(static_cast<int>(I)));
			}
			const GG::FSafeZone& Zone = Course.SafeZones[Sim.Ball().SafeZone];
			Lie = Zone.Center + GG::FVec3{0.0, 0.0, Course.BallRadius};
			CurrentLeg = Sim.Ball().SafeZone + 1;
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
	if (CurrentPhase != EGGPhase::Sunk)
	{
		Clock += DeltaSeconds;
	}
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
				else if (Event.Type == GG::EEventType::SwitchPressed)
				{
					GGAudio::SwitchPress(this, ToUnreal(Event.Position));
				}
				else if (Event.Type == GG::EEventType::GateClosed)
				{
					GGAudio::GateShut(this, BallLocation());
				}
				else if (Event.Type == GG::EEventType::Engaged || Event.Type == GG::EEventType::Released)
				{
					const bool bRelease = Event.Type == GG::EEventType::Released;
					if (MachineFlash.IsValidIndex(Event.Index))
					{
						MachineFlash[Event.Index] = 1.f;
						GGAudio::MachineField(this, MachineBases[Event.Index], bRelease);
					}
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

	UpdateFieldPreview(DeltaSeconds);

	// The ball draws its path in light, fading behind it.
	TrailDots->ClearInstances();
	for (int32 I = 0; I < Trail.Num(); I += 2)
	{
		const float Fade = static_cast<float>(I) / Trail.Num();
		TrailDots->AddInstance(FTransform(FRotator::ZeroRotator, Trail[I], FVector(0.012f + 0.035f * Fade)), true);
	}
}

void AGGCourse::SetFieldEmphasis(bool bEmphasize)
{
	bFieldEmphasis = bEmphasize;
}

void AGGCourse::UpdateFieldPreview(float DeltaSeconds)
{
	// Each machine draws its field on the ground. Always: a dashed ring at the dome's edge. While it matters
	// (carrying a machine, standing over the ball, a shot in motion): arrowheads that stream along the pull,
	// larger and faster where it is stronger, so the player can read a placement before swinging.
	FieldLevel = FMath::FInterpTo(FieldLevel, bFieldEmphasis ? 1.f : 0.f, DeltaSeconds, 4.f);
	if (UMaterialInstanceDynamic* Ring = Cast<UMaterialInstanceDynamic>(FieldRing->GetMaterial(0)))
	{
		Ring->SetScalarParameterValue(TEXT("Strength"), FMath::Lerp(0.8f, CVarFieldGlow.GetValueOnGameThread(), FieldLevel));
	}
	for (UInstancedStaticMeshComponent* Set : {FieldPushRing.Get(), FieldPushDots.Get()})
	{
		if (UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(Set->GetMaterial(0)))
		{
			const float Idle = Set == FieldPushRing ? 0.8f : 0.f;
			Material->SetScalarParameterValue(TEXT("Strength"), FMath::Lerp(Idle, CVarFieldGlow.GetValueOnGameThread(), FieldLevel));
		}
		Set->ClearInstances();
	}
	if (UMaterialInstanceDynamic* Arrows = Cast<UMaterialInstanceDynamic>(FieldDots->GetMaterial(0)))
	{
		Arrows->SetScalarParameterValue(TEXT("Strength"), FMath::Lerp(0.f, CVarFieldGlow.GetValueOnGameThread(), FieldLevel));
	}

	FieldRing->ClearInstances();
	FieldDots->ClearInstances();
	if (bFieldsHidden)
	{
		return;
	}
	const double Time = GetWorld()->GetTimeSeconds();
	constexpr int32 RingDashes = 96;
	const double SizeScale = CVarFieldSize.GetValueOnGameThread();
	for (int32 Index = 0; Index < MachineBases.Num(); ++Index)
	{
		const FVector Base = MachineBases[Index];
		const double Range = Attractors[Index].Range;
		const bool bPull = Attractors[Index].Strength > 0.0;

		for (int32 I = 0; I < RingDashes; ++I)
		{
			// Dashes laid along the circle, slowly turning, so the dome's edge reads as a boundary.
			const double Turn = 2.0 * PI * I / RingDashes + Time * 0.05;
			const FVector Out(FMath::Cos(Turn), FMath::Sin(Turn), 0.0);
			const FVector Along(-Out.Y, Out.X, 0.0);
			(bPull ? FieldRing : FieldPushRing)->AddInstance(
				FTransform(Along.Rotation(), Base + Out * Range + FVector(0, 0, 3.0), FVector(0.6, 0.06, 0.03)), true);
		}
	}

	// One field, not one per machine. On a grid over the ground an arrow points the way the machines together push a
	// ball there, sized by how hard, and drifts that way. Where two domes overlap the arrows show the sum instead of
	// two tangled sets, and where the field is weak there are none.
	if (FieldLevel >= 0.02f && MachineBases.Num() > 0)
	{
		const double Spacing = FMath::Clamp(static_cast<double>(CVarFieldGrid.GetValueOnGameThread()), 80.0, 600.0);
		const double Soft = GG::FPhysics().AttractorSoftening;
		FBox2D Bounds(ForceInit);
		for (int32 Index = 0; Index < MachineBases.Num(); ++Index)
		{
			const FVector2D Base(MachineBases[Index]);
			Bounds += Base - FVector2D(Attractors[Index].Range);
			Bounds += Base + FVector2D(Attractors[Index].Range);
		}
		const int32 X0 = FMath::FloorToInt32(Bounds.Min.X / Spacing), X1 = FMath::CeilToInt32(Bounds.Max.X / Spacing);
		const int32 Y0 = FMath::FloorToInt32(Bounds.Min.Y / Spacing), Y1 = FMath::CeilToInt32(Bounds.Max.Y / Spacing);
		for (int32 GY = Y0; GY <= Y1; ++GY)
		{
			for (int32 GX = X0; GX <= X1; ++GX)
			{
				// Alternate rows are offset by half a cell, so the grid doesn't read as rows and columns.
				const FVector2D Cell((GX + 0.5 * (GY & 1)) * Spacing, GY * Spacing);
				FVector2D Sum = FVector2D::ZeroVector;
				double Pull = 0.0, Push = 0.0, Ground = MachineBases[0].Z;
				bool bTooClose = false;
				for (int32 Index = 0; Index < MachineBases.Num(); ++Index)
				{
					const FVector2D To = FVector2D(MachineBases[Index]) - Cell;
					const double R2 = To.SizeSquared();
					const double Range = Attractors[Index].Range;
					if (R2 >= Range * Range)
					{
						continue;
					}
					bTooClose |= R2 < 110.0 * 110.0;
					const double Window = 1.0 - R2 / (Range * Range);
					const double Each = Attractors[Index].Strength * Window * Window / FMath::Sqrt(R2 + Soft * Soft);
					Sum += To * Each;
					(Each > 0.0 ? Pull : Push) += FMath::Abs(Each) * FMath::Sqrt(R2);
					Ground = MachineBases[Index].Z;
				}
				const double Strength = Sum.Size();
				if (bTooClose || Strength < 14.0)
				{
					continue;
				}
				const FVector2D Way = Sum / Strength;
				const double Seed = FMath::Frac(FMath::Sin(GX * 12.9898 + GY * 78.233) * 43758.5453);
				const double Phase = FMath::Frac(Time * CVarFieldSpeed.GetValueOnGameThread() * 3.0 + Seed);
				// Each arrow swells as it appears and shrinks as it goes, so the drift never pops.
				const double Swell = FMath::Sin(Phase * PI);
				const double Size = (0.16 + 0.42 * FMath::Min(Strength / 300.0, 1.0)) * Swell * FieldLevel * SizeScale;
				const FVector2D At = Cell + Way * ((Phase - 0.5) * Spacing * 0.8);
				const FVector Tip(Way.X, Way.Y, 0.0);
				(Pull >= Push ? FieldDots : FieldPushDots)->AddInstance(
					FTransform(FRotationMatrix::MakeFromZX(Tip, FVector::UpVector).Rotator(), FVector(At.X, At.Y, Ground + 3.0),
						FVector(0.03, Size * 0.3, Size)), true);
			}
		}
	}

	// Plates glow green once pressed; gate bars sink while their gate is open.
	const bool bLive = CurrentPhase == EGGPhase::Flying;
	std::vector<bool> Down = HeldSwitches;
	GG::HoldWeightPlates(Course, ActiveMachines(), Down);
	for (int32 I = 0; I < SwitchMaterials.Num(); ++I)
	{
		const bool bPressed = Down[I] || (bLive && Sim.SwitchPressed(I));
		SwitchMaterials[I]->SetVectorParameterValue(TEXT("Color"), bPressed ? FLinearColor(0.25f, 1.f, 0.45f) : FLinearColor(1.f, 0.6f, 0.15f));
		SwitchMaterials[I]->SetScalarParameterValue(TEXT("Strength"), bPressed ? 6.f : 1.5f);
	}
	for (int32 I = 0; I < GateBars.Num(); ++I)
	{
		const GG::FGate& Gate = Course.Gates[I];
		const bool bOpen = bLive ? Sim.GateOpen(I) : (Down[Gate.Switch] && Gate.OpenSeconds <= 0.0);
		GateLevel[I] = FMath::FInterpTo(GateLevel[I], bOpen ? 0.f : 1.f, DeltaSeconds, 9.f);
		GateBars[I]->SetRelativeScale3D(FVector(1.f, 1.f, FMath::Max(GateLevel[I], 0.001f)));
		GateBars[I]->SetVisibility(GateLevel[I] > 0.02f, true);
	}

	// A machine flashes when it takes hold of the ball and again when it lets go.
	for (int32 Index = 0; Index < MachineCores.Num(); ++Index)
	{
		MachineFlash[Index] = FMath::Max(0.f, MachineFlash[Index] - DeltaSeconds * 2.2f);
		const float Pulse = 1.f + 0.08f * FMath::Sin(Time * 2.2f) + 1.6f * MachineFlash[Index] * MachineFlash[Index];
		MachineCores[Index]->SetRelativeScale3D(FVector(0.14f * Pulse));
		const float Flash = 1.f + 3.f * MachineFlash[Index] * MachineFlash[Index];
		if (UMaterialInstanceDynamic* CoreGlow = Cast<UMaterialInstanceDynamic>(MachineCores[Index]->GetMaterial(0)))
		{
			CoreGlow->SetScalarParameterValue(TEXT("Strength"), CVarMachineCore.GetValueOnGameThread() * Flash);
		}
		if (MachineLights.IsValidIndex(Index))
		{
			MachineLights[Index]->SetIntensity(CVarMachineLight.GetValueOnGameThread() * Flash);
		}
	}
}

void AGGCourse::Advance()
{
	// After a solve: on to the next hole if there is one, otherwise play this one again.
	const FString Package = TEXT("/Game/Levels/") + NextLevel;
	if (!NextLevel.IsEmpty() && FPackageName::DoesPackageExist(Package))
	{
		if (GetNetMode() == NM_Standalone)
		{
			UGameplayStatics::OpenLevel(this, FName(*Package));
		}
		else if (HasAuthority())
		{
			// Playing together: the server takes everyone to the next hole.
			GetWorld()->ServerTravel(Package);
		}
		return;
	}
	if (!bDrawn && GetNetMode() == NM_Standalone && FPackageName::DoesPackageExist(TEXT("/Game/Levels/Workshop")))
	{
		// The last hole is solved. What is left is to build holes of one's own.
		UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/Levels/Workshop")));
		return;
	}
	ResetHole();
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

const FGGSolution* AGGCourse::SolutionForLeg(int32 Leg) const
{
	return Solutions.FindByPredicate([Leg](const FGGSolution& S) { return S.Leg == Leg; });
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
