#include "Game/GGGameMode.h"

#include "Engine/World.h"
#include "Game/GGCameraPawn.h"
#include "Game/GGHUD.h"
#include "Game/GGHoleDirector.h"
#include "Game/GGPlayerController.h"
#include "Hole/GGHoleLoader.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogGGMode, Log, All);

namespace
{
TArray<double> ParseNumbers(const FString& Value)
{
	TArray<FString> Parts;
	Value.ParseIntoArray(Parts, TEXT(","));
	TArray<double> Numbers;
	for (const FString& Part : Parts)
	{
		Numbers.Add(FCString::Atod(*Part));
	}
	return Numbers;
}
} // namespace

AGGGameMode::AGGGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = AGGCameraPawn::StaticClass();
	PlayerControllerClass = AGGPlayerController::StaticClass();
	HUDClass = AGGHUD::StaticClass();
}

void AGGGameMode::BeginPlay()
{
	Super::BeginPlay();

	const TCHAR* Cmd = FCommandLine::Get();
	FString HoleName = TEXT("ringside");
	FParse::Value(Cmd, TEXT("GGHole="), HoleName);

	Director = GetWorld()->SpawnActor<AGGHoleDirector>();
	if (!Director->LoadHole(HoleName))
	{
		UE_LOG(LogGGMode, Error, TEXT("Failed to load hole %s"), *HoleName);
		return;
	}

	float TimeScale = 1.f;
	if (FParse::Value(Cmd, TEXT("GGTimeScale="), TimeScale))
	{
		Director->TimeScale = TimeScale;
	}

	FString Value;
	if (FParse::Value(Cmd, TEXT("GGWell="), Value, false))
	{
		const TArray<double> N = ParseNumbers(Value);
		if (N.Num() >= 2)
		{
			Director->PlaceWell(GGHoleLoader::ToWorld({N[0], N[1]}), N.Num() >= 3 && N[2] != 0.0);
		}
	}

	if (FParse::Value(Cmd, TEXT("GGShot="), Value, false))
	{
		const TArray<double> N = ParseNumbers(Value);
		if (N.Num() >= 2)
		{
			bHasShot = true;
			ShotAngle = N[0];
			ShotPower = N[1];
			FParse::Value(Cmd, TEXT("GGShotDelay="), ShotDelay);
		}
	}

	FParse::Value(Cmd, TEXT("GGCapture="), CapturePath);
	FParse::Value(Cmd, TEXT("GGCaptureDelay="), CaptureDelay);

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (AGGCameraPawn* Camera = Cast<AGGCameraPawn>(PC->GetPawn()))
		{
			Camera->Frame(Director->HoleCenter(), Director->HoleExtent());
			if (FParse::Value(Cmd, TEXT("GGCam="), Value, false))
			{
				const TArray<double> N = ParseNumbers(Value);
				if (N.Num() >= 1)
				{
					Camera->Pitch = N[0];
				}
				if (N.Num() >= 2)
				{
					Camera->Yaw = N[1];
				}
				if (N.Num() >= 3)
				{
					Camera->Distance *= N[2];
				}
			}
		}
	}
}

void AGGGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Elapsed += DeltaSeconds;

	if (bHasShot && !bShotFired && Elapsed >= ShotDelay && Director)
	{
		bShotFired = true;
		Director->LaunchAt(ShotAngle, ShotPower);
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
