#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "GGGolfer.generated.h"

class AGGCourse;
class UCameraComponent;
class UStaticMeshComponent;

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
	void ResetHole();
	void ReturnBall();
	void GoToBall();
	void WatchPressed() { bWatching = true; }
	void WatchReleased() { bWatching = false; }
	void ShowSolution();
	void JumpToHole(int32 Number);
	void Hole1() { JumpToHole(1); }
	void Hole2() { JumpToHole(2); }
	void Hole3() { JumpToHole(3); }
	void Hole4() { JumpToHole(4); }
	void Hole5() { JumpToHole(5); }

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

	bool bWatching = false;
	double StepDistance = 0.0;
	FVector LastFeet = FVector::ZeroVector;
};
