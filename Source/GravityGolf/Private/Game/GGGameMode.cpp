#include "Game/GGGameMode.h"

#include "Animation/SkeletalMeshActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RHIGlobals.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Game/GGCourse.h"
#include "Game/GGGolfer.h"
#include "Game/GGHUD.h"
#include "Game/GGSave.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/App.h"
#include "Misc/Parse.h"
#include "DynamicRHI.h"
#include "RenderTimer.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogGGMode, Log, All);

namespace
{
TArray<double> ParseNumbers(const TCHAR* Key)
{
	FString Value;
	TArray<double> Numbers;
	if (FParse::Value(FCommandLine::Get(), Key, Value, false))
	{
		TArray<FString> Parts;
		Value.ParseIntoArray(Parts, TEXT(","));
		for (const FString& Part : Parts)
		{
			Numbers.Add(FCString::Atod(*Part));
		}
	}
	return Numbers;
}
} // namespace

namespace
{
TAutoConsoleVariable<int32> CVarPlantStandIns(TEXT("gg.Plants.StandIns"), 0,
	TEXT("1 spawns the simple stand-in plants even where the scanned ones could be drawn (to preview the web look)"));
}

void AGGGameMode::SpawnPlants(const FString& LevelName)
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
	UE_LOG(LogGGMode, Log, TEXT("Plants: %d scanned, %d stand-ins"), Scans, StandIns);
}

AGGGameMode::AGGGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = AGGGolfer::StaticClass();
	HUDClass = AGGHUD::StaticClass();
}

void AGGGameMode::BeginPlay()
{
	Super::BeginPlay();

	SpawnPlants(UGameplayStatics::GetCurrentLevelName(this, true));

	const FString CourseId = UGameplayStatics::GetCurrentLevelName(this, true).ToLower();
	if (!AGGCourse::Exists(CourseId))
	{
		UE_LOG(LogGGMode, Warning, TEXT("No course file for level '%s'"), *CourseId);
		return;
	}

	Course = GetWorld()->SpawnActor<AGGCourse>();
	if (!Course->Load(CourseId))
	{
		Course->Destroy();
		Course = nullptr;
		return;
	}

	const TArray<double> Attractor = ParseNumbers(TEXT("GGAttractor="));
	if (Attractor.Num() == 3)
	{
		Course->MoveMachine(0, FVector(Attractor[0], Attractor[1], Attractor[2]), false);
	}

	const TArray<double> ShotValues = ParseNumbers(TEXT("GGShot="));
	if (ShotValues.Num() == 3)
	{
		bHasShot = true;
		Shot = FVector(ShotValues[0], ShotValues[1], ShotValues[2]);
		FParse::Value(FCommandLine::Get(), TEXT("GGShotDelay="), ShotDelay);
	}

	FParse::Value(FCommandLine::Get(), TEXT("GGCapture="), CapturePath);
	FParse::Value(FCommandLine::Get(), TEXT("GGCaptureDelay="), CaptureDelay);
}

namespace
{
TArray<double> Numbers(const FString& Text)
{
	TArray<FString> Parts;
	Text.ParseIntoArray(Parts, TEXT(","));
	TArray<double> Out;
	for (const FString& Part : Parts)
	{
		Out.Add(FCString::Atod(*Part));
	}
	return Out;
}
} // namespace

void AGGGameMode::GGShot(const FString& YawPower)
{
	const TArray<double> N = Numbers(YawPower);
	if (Course && N.Num() == 2)
	{
		Course->Swing(FRotator(0.0, N[0], 0.0).Vector(), N[1]);
	}
}

void AGGGameMode::GGMachine(const FString& XYZ)
{
	const TArray<double> N = Numbers(XYZ);
	if (Course && N.Num() >= 3)
	{
		Course->MoveMachine(N.Num() >= 4 ? static_cast<int32>(N[3]) : 0, FVector(N[0], N[1], N[2]), false);
	}
}

void AGGGameMode::GGCarry(const FString& XYZ)
{
	const TArray<double> N = Numbers(XYZ);
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	AGGGolfer* Golfer = PC ? Cast<AGGGolfer>(PC->GetPawn()) : nullptr;
	if (Golfer && N.Num() == 3)
	{
		const bool bPlaced = Golfer->TestCarryTo(FVector(N[0], N[1], N[2]));
		UE_LOG(LogGGMode, Log, TEXT("GGCarry (%s): %s"), *XYZ, bPlaced ? TEXT("placed") : TEXT("refused"));
	}
}

void AGGGameMode::GGSwing(const FString& YawPower)
{
	const TArray<double> N = Numbers(YawPower);
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	AGGGolfer* Golfer = PC ? Cast<AGGGolfer>(PC->GetPawn()) : nullptr;
	if (Golfer && N.Num() == 2)
	{
		Golfer->TestSwing(N[0], N[1]);
	}
}

void AGGGameMode::GGView(const FString& XYZPitchYaw)
{
	const TArray<double> N = Numbers(XYZPitchYaw);
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (Pawn && N.Num() == 5)
	{
		Pawn->SetActorLocation(FVector(N[0], N[1], N[2]), false, nullptr, ETeleportType::TeleportPhysics);
		PC->SetControlRotation(FRotator(N[3], N[4], 0.0));
	}
}

void AGGGameMode::GGHole(int32 Number)
{
	const TArray<FGGHoleInfo>& Holes = UGGSave::Holes();
	if (Holes.IsValidIndex(Number - 1))
	{
		UGameplayStatics::OpenLevel(this, FName(*(FString(TEXT("/Game/Levels/")) + Holes[Number - 1].Level)));
	}
}

void AGGGameMode::GGTouch(const FString& FingerPhaseXY)
{
	const TArray<double> N = Numbers(FingerPhaseXY);
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	AGGGolfer* Golfer = PC ? Cast<AGGGolfer>(PC->GetPawn()) : nullptr;
	if (Golfer && N.Num() == 4)
	{
		const FVector2D View = Golfer->ViewportSize();
		Golfer->HandleTouch(static_cast<int32>(N[0]), static_cast<int32>(N[1]), FVector2D(N[2] * View.X, N[3] * View.Y));
	}
}

void AGGGameMode::GGMenu(const FString& Screen)
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (AGGHUD* Hud = PC ? Cast<AGGHUD>(PC->GetHUD()) : nullptr)
	{
		Hud->OpenMenu(Screen == TEXT("title") ? EGGMenu::Title : Screen == TEXT("pause") ? EGGMenu::Pause
			: Screen == TEXT("holes") ? EGGMenu::Holes : Screen == TEXT("settings") ? EGGMenu::Settings : EGGMenu::None);
	}
}

void AGGGameMode::GGWhere()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const AGGGolfer* Golfer = PC ? Cast<AGGGolfer>(PC->GetPawn()) : nullptr;
	if (Golfer && Course)
	{
		const FVector At = Golfer->GetActorLocation();
		const FRotator Look = PC->GetControlRotation().GetNormalized();
		UE_LOG(LogGGMode, Log, TEXT("Where: at %.0f,%.0f,%.0f yaw %.1f pitch %.1f phase %d swings %d carrying %d charging %d touch %d"),
			At.X, At.Y, At.Z, Look.Yaw, Look.Pitch, static_cast<int32>(Course->Phase()), Course->Strokes(), Golfer->IsCarrying() ? 1 : 0,
			Golfer->IsCharging() ? 1 : 0, Golfer->UsingTouch() ? 1 : 0);
	}
}

void AGGGameMode::GGPerf(float Seconds)
{
	PerfFrames.Reset();
	PerfLeft = FMath::Max(1.f, Seconds);
	PerfGame = PerfRender = PerfGPU = 0.0;
}

void AGGGameMode::GGDemo()
{
	if (Course && Course->Phase() == EGGPhase::Ready)
	{
		bDemo = true;
		Course->MarkAssisted();
		bDemoSwung = false;
		DemoLeg = -1;
		DemoWait = 0.6;
		UE_LOG(LogGGMode, Log, TEXT("Demo: playing the solution for %s"), *Course->CourseName());
	}
}

void AGGGameMode::TickDemo(float DeltaSeconds)
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	AGGGolfer* Golfer = PC ? Cast<AGGGolfer>(PC->GetPawn()) : nullptr;
	if (!bDemo || !Course || !Golfer)
	{
		return;
	}
	if (Course->Phase() == EGGPhase::Sunk)
	{
		UE_LOG(LogGGMode, Log, TEXT("Demo: PASS %s solved in %d swings"), *Course->CourseName(), Course->Strokes());
		bDemo = false;
		return;
	}
	if (Course->Phase() != EGGPhase::Ready)
	{
		return;
	}
	DemoWait -= DeltaSeconds;
	if (DemoWait > 0.0)
	{
		return;
	}

	const int32 Leg = Course->Leg();
	if (bDemoSwung && Leg == DemoLeg)
	{
		// The swing for this leg has been played and the ball is back where it started: the solution failed.
		UE_LOG(LogGGMode, Error, TEXT("Demo: FAIL %s leg %d did not complete"), *Course->CourseName(), Leg + 1);
		bDemo = false;
		return;
	}
	const FGGSolution* Solution = Course->SolutionForLeg(Leg);
	if (!Solution)
	{
		UE_LOG(LogGGMode, Error, TEXT("Demo: FAIL %s has no solution recorded for leg %d"), *Course->CourseName(), Leg + 1);
		bDemo = false;
		return;
	}
	for (int32 I = 0; I < Solution->Machines.Num() && I < Course->MachineCount(); ++I)
	{
		if (!Golfer->TestPlaceMachine(I, Solution->Machines[I]))
		{
			UE_LOG(LogGGMode, Error, TEXT("Demo: FAIL %s machine %d could not be placed"), *Course->CourseName(), I + 1);
			bDemo = false;
			return;
		}
	}
	Golfer->TestSwing(Solution->Yaw, Solution->Power);
	DemoLeg = Leg;
	bDemoSwung = true;
	DemoWait = 1.0;
}

void AGGGameMode::PlacePlayer()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn || !Course)
	{
		return;
	}
	bPlayerPlaced = true;
	PC->SetInputMode(FInputModeGameOnly());
	PC->bShowMouseCursor = false;

	// A real launch (not Play-In-Editor) opens on the title screen, once per run of the game.
	static bool bTitleShown = false;
	if (!bTitleShown && GetWorld()->WorldType == EWorldType::Game && CapturePath.IsEmpty() && !bHasShot)
	{
		bTitleShown = true;
		if (AGGHUD* Hud = Cast<AGGHUD>(PC->GetHUD()))
		{
			Hud->OpenMenu(EGGMenu::Title);
		}
	}

	const TArray<double> View = ParseNumbers(TEXT("GGView="));
	if (View.Num() == 5)
	{
		Pawn->SetActorLocation(FVector(View[0], View[1], View[2]), false, nullptr, ETeleportType::TeleportPhysics);
		PC->SetControlRotation(FRotator(View[3], View[4], 0.0));
	}
	else
	{
		Pawn->SetActorLocation(Course->TeeStart(), false, nullptr, ETeleportType::TeleportPhysics);
		PC->SetControlRotation(FRotator(-6.0, Course->TeeYaw(), 0.0));
	}
}

void AGGGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Elapsed += DeltaSeconds;

	if (!bPlayerPlaced)
	{
		PlacePlayer();
	}
	TickDemo(DeltaSeconds);

	if (PerfLeft > 0.0)
	{
		const double Frame = FApp::GetDeltaTime();
		PerfFrames.Add(static_cast<float>(Frame * 1000.0));
		PerfGame += FPlatformTime::ToMilliseconds(GGameThreadTime);
		PerfRender += FPlatformTime::ToMilliseconds(GRenderThreadTime);
		PerfGPU += FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
		PerfLeft -= Frame;
		if (PerfLeft <= 0.0 && PerfFrames.Num() > 0)
		{
			PerfFrames.Sort();
			const int32 N = PerfFrames.Num();
			double Sum = 0.0;
			for (const float Ms : PerfFrames)
			{
				Sum += Ms;
			}
			const FIntPoint Size = GEngine && GEngine->GameViewport ? GEngine->GameViewport->Viewport->GetSizeXY() : FIntPoint::ZeroValue;
			UE_LOG(LogGGMode, Log,
				TEXT("Perf: %s at %dx%d: %.1f fps average (%.1f ms), slowest 1%% of frames %.1f ms, worst %.1f ms; game %.1f ms, render %.1f ms, GPU %.1f ms; %d frames"),
				*UGameplayStatics::GetCurrentLevelName(this, true), Size.X, Size.Y, 1000.0 * N / Sum, Sum / N,
				PerfFrames[FMath::Min(N - 1, N * 99 / 100)], PerfFrames[N - 1], PerfGame / N, PerfRender / N, PerfGPU / N, N);
		}
	}

	if (bHasShot && !bShotFired && Elapsed >= ShotDelay && Course)
	{
		bShotFired = true;
		Course->Swing(FRotator(0.0, Shot.X, 0.0).Vector(), Shot.Z);
	}

	if (!CapturePath.IsEmpty())
	{
		if (!bCaptured && Elapsed >= CaptureDelay)
		{
			bCaptured = true;
			CapturedAt = Elapsed;
			FScreenshotRequest::RequestScreenshot(CapturePath, true, false);
			UE_LOG(LogGGMode, Log, TEXT("Capturing %s"), *CapturePath);
		}
		else if (bCaptured && Elapsed - CapturedAt > 1.5)
		{
			FPlatformMisc::RequestExit(false, TEXT("GGCapture"));
		}
	}
}
