#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "GGPlayerController.generated.h"

class AGGCameraPawn;
class AGGHoleDirector;

UCLASS()
class GRAVITYGOLF_API AGGPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGGPlayerController();

	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

private:
	AGGHoleDirector* Director() const;
	AGGCameraPawn* CameraPawn() const;
	bool CursorOnPlane(FVector& OutPoint) const;

	void PrimaryPressed();
	void PrimaryReleased();
	void PanPressed();
	void PanReleased();
	void ZoomIn();
	void ZoomOut();
	void PlaceGravity();
	void PlaceAntigravity();
	void Reset();
	void OrbitLeft();
	void OrbitRight();

	bool bPrimaryDown = false;
	bool bPanning = false;
	FVector PanAnchor = FVector::ZeroVector;
};
