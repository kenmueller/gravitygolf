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
 *
 * Console commands while playing take one comma-separated argument (the console drops negative numbers passed
 * separately): GGMachine x,y,z   GGShot yaw,power   GGCarry x,y,z   GGSwing yaw,power   GGView x,y,z,pitch,yaw
 */
UCLASS()
class GRAVITYGOLF_API AGGGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGGGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Console: automated play calls this first, so nothing it does lands in the player's own save. */
	UFUNCTION(Exec)
	void GGTestSave();

	/** Console, in the Workshop: "tool wall", "stroke x0,y0,x1,y1" (one stroke of the tool, in centimetres), "click fx,fy"
	 *  (a press on the screen), "test", "build", "undo", "new", "name N", "keep", "open N", "code" (log it) or "code <text>"
	 *  (open one), "check" (start the proof), "view x,y,distance", "frame". Every call logs the Workshop's state. */
	UFUNCTION(Exec)
	void GGWorkshop(const FString& Command, const FString& Argument);

	/** Console: swing with a yaw in degrees and power 0..1. Lets automated checks replay solver results. */
	UFUNCTION(Exec)
	void GGShot(const FString& YawPower);

	/** Console: stand machine 0 at a ground position. */
	UFUNCTION(Exec)
	void GGMachine(const FString& XYZ);

	/** Console: have the player carry the nearest machine to a ground point and set it down (real placement rules). */
	UFUNCTION(Exec)
	void GGCarry(const FString& XYZ);

	/** Console: have the player walk to the ball and swing through the club animation. */
	UFUNCTION(Exec)
	void GGSwing(const FString& YawPower);

	/** Console: put the player somewhere, looking a given way (for screenshots). */
	UFUNCTION(Exec)
	void GGView(const FString& XYZPitchYaw);

	/** Console or F1: play this hole's verified solution through the real player code, leg by leg. */
	UFUNCTION(Exec)
	void GGDemo();

	/** Console or number keys: open hole 1, 2, 3... */
	UFUNCTION(Exec)
	void GGHole(int32 Number);

	/** Console: one touch event, for testing touch play without a phone. "finger,phase,x,y" with phase 0 down, 1 moved,
	 *  2 up, and x,y as fractions of the screen (0,0 is the top left). */
	UFUNCTION(Exec)
	void GGTouch(const FString& FingerPhaseXY);

	/** Console: open a screen (title, pause, holes, settings) or close it (none). */
	UFUNCTION(Exec)
	void GGMenu(const FString& Screen);

	/** Console: start a rival bot on this hole now (1 easy, 2 medium, 3 hard), or remove it (0). */
	UFUNCTION(Exec)
	void GGRival(int32 Skill);

	/** Console: log where the player is, which way they face and what they are doing. */
	UFUNCTION(Exec)
	void GGWhere();

	/** Console: measure frame times for a number of seconds and log the result (GGPerf 5). */
	UFUNCTION(Exec)
	void GGPerf(float Seconds);

private:
	void PlacePlayer();
	void TickDemo(float DeltaSeconds);

	TArray<float> PerfFrames;
	double PerfLeft = 0.0;
	double PerfGame = 0.0;
	double PerfRender = 0.0;
	double PerfGPU = 0.0;

	bool bDemo = false;
	bool bDemoSwung = false;
	int32 DemoLeg = -1;
	double DemoWait = 0.0;

	UPROPERTY()
	TObjectPtr<AGGCourse> Course;

	double Elapsed = 0.0;
	/** Pawns already stood at the tee. */
	TSet<TWeakObjectPtr<APawn>> PlacedPawns;
	FString CapturePath;
	double CaptureDelay = 6.0;
	bool bCaptured = false;
	double CapturedAt = 0.0;
	bool bHasShot = false;
	FVector Shot = FVector::ZeroVector;
	double ShotDelay = 2.0;
	bool bShotFired = false;
};
