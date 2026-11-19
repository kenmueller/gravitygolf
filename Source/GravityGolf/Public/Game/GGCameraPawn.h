#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

#include "GGCameraPawn.generated.h"

class UCameraComponent;

/** Orbit-style camera looking down at the play plane. */
UCLASS()
class GRAVITYGOLF_API AGGCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	AGGCameraPawn();

	virtual void Tick(float DeltaSeconds) override;

	void Frame(const FVector& Center, double Extent);
	void Zoom(float Steps);
	void Pan(const FVector& WorldDelta);
	void Orbit(float YawDelta, float PitchDelta);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	FVector Target = FVector::ZeroVector;
	double Distance = 9000.0;
	double Pitch = -62.0;
	double Yaw = 0.0;

private:
	FVector SmoothTarget = FVector::ZeroVector;
	double SmoothDistance = 9000.0;
	bool bSnapped = false;
};
