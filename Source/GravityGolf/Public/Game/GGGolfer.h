#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "GGGolfer.generated.h"

class AGGCourse;
class UCameraComponent;
class UStaticMeshComponent;

/** A round button drawn on screen for touch play. Center and radius are in viewport pixels. */
struct FGGTouchButton
{
	FName Id;
	FVector2D Center = FVector2D::ZeroVector;
	float Radius = 0.f;
	FString Label;
	bool bDown = false;
};

/** The player on foot: walk the course, carry gravity machines, swing from beside the ball. */
UCLASS()
class GRAVITYGOLF_API AGGGolfer : public ACharacter
{
	GENERATED_BODY()

public:
	AGGGolfer();

	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;

	AGGCourse* Course() const;
	bool CanSwing() const;
	bool IsCharging() const { return bCharging; }
	float Charge() const { return ChargeAmount; }
	bool IsCarrying() const { return Carried != INDEX_NONE; }
	bool CanPickUp() const;
	double DistanceToBall() const;

	// Touch play (phones and tablets): a movement stick under the left thumb, drag anywhere else to look, and round
	// buttons for everything the keys do. It switches on with the first touch.
	bool UsingTouch() const { return bTouchMode; }
	/** The buttons to show right now, for a viewport of this size. */
	void TouchButtons(const FVector2D& Viewport, TArray<FGGTouchButton>& Out) const;
	/** The movement stick: where the thumb went down and where it is now. False while no thumb is on it. */
	bool TouchStick(FVector2D& OutOrigin, FVector2D& OutKnob) const;
	/** One touch event in viewport pixels. Phase 0 is down, 1 is moved, 2 is up. Also the entry point for tests. */
	void HandleTouch(int32 Finger, int32 Phase, const FVector2D& At);
	FVector2D ViewportSize() const;

	// Things the menus and the touch buttons also do.
	void ResetHole();
	void ReturnBall();
	void GoToBall();
	void ShowSolution();
	class AGGHUD* Hud() const;
	bool MenuOpen() const;

	// Hooks for automated play tests. They drive the same code paths as the keys.
	/** Pick up the nearest machine wherever it is and try to set it down at a ground point. */
	bool TestCarryTo(const FVector& Ground);
	/** Carry a particular machine to a ground point and set it down. */
	bool TestPlaceMachine(int32 Index, const FVector& Ground);
	/** Stand at the ball, face a yaw and swing with a power in [0, 1], through the club animation. */
	void TestSwing(float Yaw, float Power);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	/** The club's grip. The club swings by rotating this. */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> ClubPivot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ClubShaft;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ClubGrip;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ClubHead;

private:
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void SwingPressed();
	void SwingReleased();
	void Interact();
	void WatchPressed() { bWatching = true; }
	void WatchReleased() { bWatching = false; }
	void JumpToHole(int32 Number);
	void MenuToggle();
	void MenuUp();
	void MenuDown();
	void MenuLeft();
	void MenuRight();
	/** Enter and the gamepad's bottom button: choose in a menu, jump in the world. */
	void AcceptOrJump();
	void Hole1() { JumpToHole(1); }
	void Hole2() { JumpToHole(2); }
	void Hole3() { JumpToHole(3); }
	void Hole4() { JumpToHole(4); }
	void Hole5() { JumpToHole(5); }
	void Hole6() { JumpToHole(6); }

	void TouchPressed(ETouchIndex::Type Finger, FVector Location) { HandleTouch(static_cast<int32>(Finger), 0, FVector2D(Location)); }
	void TouchMoved(ETouchIndex::Type Finger, FVector Location) { HandleTouch(static_cast<int32>(Finger), 1, FVector2D(Location)); }
	void TouchReleased(ETouchIndex::Type Finger, FVector Location) { HandleTouch(static_cast<int32>(Finger), 2, FVector2D(Location)); }
	void PressButton(FName Id);
	void ReleaseButton(FName Id);
	void PadForward(float Value);
	void PadRight(float Value);
	void PadTurn(float Value);
	void PadLookUp(float Value);
	/** Turn the view by a number of degrees, keeping the pitch within what a neck can do. */
	void AddLook(float YawDegrees, float PitchDegrees);

	FVector SwingDirection() const;
	FVector HoldPoint() const;
	bool GroundBelow(const FVector& From, FVector& OutGround) const;
	bool PlaceCarried(const FVector& Ground);
	void StartSwing();
	void UpdateClub(float DeltaSeconds);
	void UpdateFootsteps();

	bool bCharging = false;
	float ChargeAmount = 0.f;
	int32 Carried = INDEX_NONE;

	/** Seconds since the club was released, or negative when no swing is in progress. */
	float SwingTime = -1.f;
	bool bStruck = false;
	float SwingPower = 0.f;
	FVector SwingAim = FVector::ForwardVector;
	/** -1 at the top of the backswing, 0 at address, +1 at the end of the follow-through. */
	float ClubPhase = 0.f;
	float ClubRaise = 0.f;

	bool bTouchMode = false;
	int32 MoveFinger = INDEX_NONE;
	int32 LookFinger = INDEX_NONE;
	FVector2D MoveOrigin = FVector2D::ZeroVector;
	FVector2D MoveAt = FVector2D::ZeroVector;
	/** Per finger: the button it holds, and where it was last seen. */
	TMap<int32, FName> HeldButtons;
	TMap<int32, FVector2D> LastTouch;

	bool bWatching = false;
	double StepDistance = 0.0;
	FVector LastFeet = FVector::ZeroVector;
};
