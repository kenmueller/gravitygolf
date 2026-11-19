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

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;

	AGGCourse* Course() const;
	bool CanSwing() const;
	bool IsCharging() const { return bCharging; }
	float Charge() const { return ChargeAmount; }
	float Loft() const { return LoftDegrees; }
	bool IsCarrying() const { return Carried != INDEX_NONE; }
	bool CanPickUp() const;
	double DistanceToBall() const;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	/** Pivot of the club at the hands; the club swings by pitching this. */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> ClubPivot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ClubShaft;

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
	void LoftUp();
	void LoftDown();
	void ResetHole();
	void ReturnBall();
	void GoToBall();
	FVector SwingDirection() const;
	FVector HoldPoint() const;

	void WatchPressed() { bWatching = true; }
	void WatchReleased() { bWatching = false; }
	void UpdateClub(float DeltaSeconds);
	void UpdateFootsteps();
	bool GroundBelow(const FVector& From, FVector& OutGround) const;

	/** Seconds since the club was released, or negative when no swing is in progress. */
	float SwingTime = -1.f;
	bool bStruck = false;
	float SwingPower = 0.f;
	FVector SwingAim = FVector::ForwardVector;
	float ClubPitch = 0.f;
	bool bWatching = false;
	double StepDistance = 0.0;
	FVector LastFeet = FVector::ZeroVector;

	bool bCharging = false;
	float ChargeAmount = 0.f;
	float LoftDegrees = 0.f;
	int32 Carried = INDEX_NONE;
};
