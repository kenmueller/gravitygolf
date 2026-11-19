#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "GGGameMode.generated.h"

class AGGHoleDirector;

/**
 * Builds the hole at startup and runs command-line automation, so the game can be launched, driven and
 * photographed without the editor UI:
 *
 *   -GGHole=ringside            hole file in Content/Holes
 *   -GGCam=pitch,yaw,zoom       camera override (zoom multiplies the framed distance)
 *   -GGWell=x,y[,anti]          place a well (sim coordinates)
 *   -GGShot=angle,power         launch after -GGShotDelay seconds (default 1)
 *   -GGCapture=/abs/file.png    screenshot after -GGCaptureDelay seconds (default 3), then quit
 *   -GGTimeScale=1              simulation speed
 */
UCLASS()
class GRAVITYGOLF_API AGGGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGGGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY()
	TObjectPtr<AGGHoleDirector> Director;

	double Elapsed = 0.0;
	FString CapturePath;
	double CaptureDelay = 3.0;
	bool bCaptured = false;
	double CapturedAt = 0.0;
	bool bHasShot = false;
	double ShotAngle = 0.0;
	double ShotPower = 0.0;
	double ShotDelay = 1.0;
	bool bShotFired = false;
};
