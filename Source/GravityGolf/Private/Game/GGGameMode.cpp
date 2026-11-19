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
#include "EngineUtils.h"
#include "Game/GGCourse.h"
#include "Game/GGGolfer.h"
#include "Game/GGHUD.h"
#include "Game/GGRival.h"
#include "Game/GGSave.h"
#include "Game/GGWorkshop.h"
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

AGGGameMode::AGGGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = AGGGolfer::StaticClass();
	HUDClass = AGGHUD::StaticClass();
}

void AGGGameMode::GGTestSave()
{
	UGGSave::UseScratch(this);
	UE_LOG(LogGGMode, Display, TEXT("Automated play: using a scratch save; the player's own is untouched"));
}

void AGGGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (FParse::Param(FCommandLine::Get(), TEXT("GGTestSave")))
	{
		UGGSave::UseScratch(this);
	}
	// The picture quality the player chose last time (it was only ever applied when they changed it).
	UGGSave::Get().ApplyQuality();

	const FString CourseId = UGameplayStatics::GetCurrentLevelName(this, true).ToLower();
	if (CourseId == TEXT("workshop"))
	{
		// The Workshop has no hole of its own: the player builds one, and the Workshop hands it to the course.
		Course = GetWorld()->SpawnActor<AGGCourse>();
		GetWorld()->SpawnActor<AGGWorkshop>()->Begin(Course);
		return;
	}
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

	// A bot to race, if the player has asked for one. Alone only: it plays its own ghost copy of the hole.
	int32 RivalSkill = UGGSave::Get().Rival;
	FParse::Value(FCommandLine::Get(), TEXT("GGRival="), RivalSkill);
	if (RivalSkill > 0 && GetNetMode() == NM_Standalone)
	{
		GetWorld()->SpawnActor<AGGRival>()->Begin(Course, RivalSkill);
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

void AGGGameMode::GGWorkshop(const FString& Command, const FString& Argument)
{
	// Works the Workshop without hands, through the same calls the pointer and the buttons make.
	AGGWorkshop* Workshop = AGGWorkshop::Get(this);
	if (!Workshop)
	{
		UE_LOG(LogGGMode, Warning, TEXT("GGWorkshop: this level is not the Workshop"));
		return;
	}
	const TArray<double> N = Numbers(Argument);
	if (Command == TEXT("tool"))
	{
		for (int32 I = 0; I < static_cast<int32>(EGGTool::Count); ++I)
		{
			if (FString(AGGWorkshop::ToolName(static_cast<EGGTool>(I))).Replace(TEXT(" "), TEXT("")).Equals(Argument, ESearchCase::IgnoreCase))
			{
				Workshop->SetTool(static_cast<EGGTool>(I));
			}
		}
	}
	else if (Command == TEXT("stroke") && N.Num() >= 2)
	{
		// One stroke of the chosen tool, in world centimetres: x,y for a click, x0,y0,x1,y1 for a drag.
		Workshop->Press(FVector2D(N[0], N[1]));
		if (N.Num() >= 4)
		{
			Workshop->Drag(FVector2D(N[2], N[3]));
		}
		Workshop->Release();
	}
	else if (Command == TEXT("click") && N.Num() >= 2)
	{
		// A press on the screen, in fractions of it: a button if one is there.
		if (AGGGolfer* Golfer = Cast<AGGGolfer>(UGameplayStatics::GetPlayerPawn(this, 0)))
		{
			const FVector2D At = Golfer->ViewportSize() * FVector2D(N[0], N[1]);
			UE_LOG(LogGGMode, Display, TEXT("Workshop: click %s"), Golfer->Hud() && Golfer->Hud()->WorkshopPress(At) ? TEXT("hit a button") : TEXT("missed"));
		}
	}
	else if (Command == TEXT("test"))
	{
		Workshop->StartTesting();
	}
	else if (Command == TEXT("build"))
	{
		Workshop->StartBuilding();
	}
	else if (Command == TEXT("undo"))
	{
		Workshop->Undo();
	}
	else if (Command == TEXT("new"))
	{
		Workshop->NewHole();
	}
	else if (Command == TEXT("name"))
	{
		Workshop->NameHole(Argument);
	}
	else if (Command == TEXT("keep"))
	{
		Workshop->SaveHole();
	}
	else if (Command == TEXT("open"))
	{
		Workshop->LoadHole(Argument);
	}
	else if (Command == TEXT("code"))
	{
		if (Argument.IsEmpty())
		{
			UE_LOG(LogGGMode, Display, TEXT("Workshop: code %s"), *Workshop->ShareCode());
		}
		else
		{
			Workshop->ImportCode(Argument);
		}
	}
	else if (Command == TEXT("check"))
	{
		Workshop->StartProof();
	}
	else if (Command == TEXT("json") && !Argument.IsEmpty())
	{
		// The design as a course file, for the standalone solver (Tests/Sim/design.sh).
		FFileHelper::SaveStringToFile(Workshop->Design().ToJson(), *Argument);
	}
	else if (Command == TEXT("view") && N.Num() >= 3)
	{
		Workshop->LookAt(FVector2D(N[0], N[1]), N[2]);
	}
	else if (Command == TEXT("frame"))
	{
		Workshop->FrameDesign();
	}
	const FGGDesign& Design = Workshop->Design();
	UE_LOG(LogGGMode, Display, TEXT("Workshop: %s \"%s\" squares %d machines %d plates %d gates %d pads %d solved %d necessary %d luck %d proving %d (%.0f%%) | %s"),
		Workshop->Building() ? TEXT("building") : TEXT("testing"), *Design.Name, Design.Cells.Num(), Design.Machines.Num(), Design.Plates.Num(),
		Design.Gates.Num(), Design.Pads.Num(), Design.Solved() ? 1 : 0, Design.Necessary, Design.Luck, Workshop->Proving() ? 1 : 0, Workshop->ProofProgress() * 100.f,
		*Workshop->Message());
}

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
		Course->NetPlace(N.Num() >= 4 ? static_cast<int32>(N[3]) : 0, FVector(N[0], N[1], N[2]));
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
			: Screen == TEXT("holes") ? EGGMenu::Holes : Screen == TEXT("settings") ? EGGMenu::Settings
			: Screen == TEXT("together") ? EGGMenu::Together : Screen == TEXT("workshop") ? EGGMenu::Workshop
			: Screen == TEXT("designs") ? EGGMenu::Designs : EGGMenu::None);
	}
}

void AGGGameMode::GGRival(int32 Skill)
{
	for (TActorIterator<AGGRival> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}
	if (Course && Skill > 0)
	{
		GetWorld()->SpawnActor<AGGRival>()->Begin(Course, Skill);
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
		UE_LOG(LogGGMode, Display, TEXT("Demo: playing the solution for %s"), *Course->CourseName());
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
		UE_LOG(LogGGMode, Display, TEXT("Demo: PASS %s solved in %d swings"), *Course->CourseName(), Course->Strokes());
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
	if (!Course)
	{
		return;
	}
	// Every player, as they arrive, stands at the tee: the first behind the ball, the others in a row to its right.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		if (!Pawn || PlacedPawns.Contains(Pawn))
		{
			continue;
		}
		const int32 Seat = PlacedPawns.Num();
		PlacedPawns.Add(Pawn);
		if (AGGGolfer* Golfer = Cast<AGGGolfer>(Pawn))
		{
			Golfer->SetSeat(Seat);
		}
		const FRotator Facing(-6.0, Course->TeeYaw(), 0.0);
		const FVector Right = FRotator(0.0, Course->TeeYaw(), 0.0).RotateVector(FVector::RightVector);
		FVector Location = Course->TeeStart() + Right * (150.0 * Seat);
		FRotator Look = Facing;

		if (PC->IsLocalController())
		{
			PC->SetInputMode(FInputModeGameOnly());
			PC->bShowMouseCursor = false;

			// A real launch (not Play-In-Editor) opens on the title screen, once per run of the game.
			static bool bTitleShown = false;
			if (!bTitleShown && GetWorld()->WorldType == EWorldType::Game && CapturePath.IsEmpty() && !bHasShot
				&& !FParse::Param(FCommandLine::Get(), TEXT("GGNoTitle")))
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
				Location = FVector(View[0], View[1], View[2]);
				Look = FRotator(View[3], View[4], 0.0);
			}
		}
		Pawn->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
		PC->SetControlRotation(Look);
		PC->ClientSetRotation(Look);
	}
}

void AGGGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Elapsed += DeltaSeconds;

	PlacePlayer();
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
