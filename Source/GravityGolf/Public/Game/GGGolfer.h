#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "GGGolfer.generated.h"

class AGGCourse;
class UCameraComponent;

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
	void GoToBall();
	FVector SwingDirection() const;
	FVector HoldPoint() const;

	bool bCharging = false;
	float ChargeAmount = 0.f;
	float LoftDegrees = 0.f;
	int32 Carried = INDEX_NONE;
};
