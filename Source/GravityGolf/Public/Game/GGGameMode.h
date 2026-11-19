#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "GGGameMode.generated.h"

class AGGCourse;

/**
 * Starts the course that matches the open level (Content/Courses/<level name>.json) and runs command-line
 * automation, so the game can be launched, driven and photographed without anyone at the controls:
 *
 *   -GGView=x,y,z,pitch,yaw     place the player and look direction
 *   -GGAttractor=x,y,z          move the first attractor
 *   -GGShot=yaw,loft,power      swing after -GGShotDelay seconds (default 2)
 *   -GGCapture=/abs/file.png    screenshot after -GGCaptureDelay seconds (default 6), then quit
 *   -GGNoHUD                    hide the HUD
 */
UCLASS()
class GRAVITYGOLF_API AGGGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGGGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Console: swing with a yaw in degrees and power 0..1. Lets automated checks replay solver results. */
	UFUNCTION(Exec)
	void GGShot(float Yaw, float Power);

	/** Console: stand machine 0 at a ground position. */
	UFUNCTION(Exec)
	void GGMachine(float X, float Y, float Z);

private:
	void PlacePlayer();

	UPROPERTY()
	TObjectPtr<AGGCourse> Course;

	double Elapsed = 0.0;
	bool bPlayerPlaced = false;
	FString CapturePath;
	double CaptureDelay = 6.0;
	bool bCaptured = false;
	double CapturedAt = 0.0;
	bool bHasShot = false;
	FVector Shot = FVector::ZeroVector;
	double ShotDelay = 2.0;
	bool bShotFired = false;
};
