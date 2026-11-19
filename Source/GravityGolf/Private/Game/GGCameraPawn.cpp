#include "Game/GGCameraPawn.h"

#include "Camera/CameraComponent.h"

AGGCameraPawn::AGGCameraPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetFieldOfView(38.f);
	RootComponent = Camera;
}

void AGGCameraPawn::Frame(const FVector& Center, double Extent)
{
	Target = Center;
	const double HalfFov = FMath::DegreesToRadians(Camera->FieldOfView * 0.5);
	Distance = Extent * 1.15 / FMath::Tan(HalfFov);
	bSnapped = false;
}

void AGGCameraPawn::Zoom(float Steps)
{
	Distance = FMath::Clamp(Distance * FMath::Pow(0.9, Steps), 1500.0, 40000.0);
}

void AGGCameraPawn::Pan(const FVector& WorldDelta)
{
	Target += FVector(WorldDelta.X, WorldDelta.Y, 0.0);
}

void AGGCameraPawn::Orbit(float YawDelta, float PitchDelta)
{
	Yaw += YawDelta;
	Pitch = FMath::Clamp(Pitch + PitchDelta, -89.0, -15.0);
}

void AGGCameraPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bSnapped)
	{
		SmoothTarget = Target;
		SmoothDistance = Distance;
		bSnapped = true;
	}
	const double Blend = 1.0 - FMath::Exp(-10.0 * DeltaSeconds);
	SmoothTarget = FMath::Lerp(SmoothTarget, Target, Blend);
	SmoothDistance = FMath::Lerp(SmoothDistance, Distance, Blend);

	const FRotator Rotation(Pitch, Yaw, 0.0);
	SetActorLocationAndRotation(SmoothTarget - Rotation.Vector() * SmoothDistance, Rotation);
}
