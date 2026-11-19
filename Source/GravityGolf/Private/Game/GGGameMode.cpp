#include "Game/GGGameMode.h"

#include "Engine/World.h"
#include "Game/GGCourse.h"
#include "Game/GGGolfer.h"
#include "Game/GGHUD.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
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

void AGGGameMode::BeginPlay()
{
	Super::BeginPlay();

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

	if (bHasShot && !bShotFired && Elapsed >= ShotDelay && Course)
	{
		bShotFired = true;
		Course->Swing(FRotator(Shot.Y, Shot.X, 0.0).Vector(), Shot.Z);
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
