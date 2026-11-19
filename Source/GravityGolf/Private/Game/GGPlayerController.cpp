#include "Game/GGPlayerController.h"

#include "Components/InputComponent.h"
#include "EngineUtils.h"
#include "Game/GGCameraPawn.h"
#include "Game/GGHoleDirector.h"
#include "InputCoreTypes.h"

AGGPlayerController::AGGPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = false;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void AGGPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AGGPlayerController::PrimaryPressed);
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &AGGPlayerController::PrimaryReleased);
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AGGPlayerController::PanPressed);
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &AGGPlayerController::PanReleased);
	InputComponent->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &AGGPlayerController::ZoomIn);
	InputComponent->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &AGGPlayerController::ZoomOut);
	InputComponent->BindKey(EKeys::G, IE_Pressed, this, &AGGPlayerController::PlaceGravity);
	InputComponent->BindKey(EKeys::A, IE_Pressed, this, &AGGPlayerController::PlaceAntigravity);
	InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AGGPlayerController::Reset);
	InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AGGPlayerController::OrbitLeft);
	InputComponent->BindKey(EKeys::E, IE_Pressed, this, &AGGPlayerController::OrbitRight);
}

AGGHoleDirector* AGGPlayerController::Director() const
{
	TActorIterator<AGGHoleDirector> It(GetWorld());
	return It ? *It : nullptr;
}

AGGCameraPawn* AGGPlayerController::CameraPawn() const
{
	return Cast<AGGCameraPawn>(GetPawn());
}

bool AGGPlayerController::CursorOnPlane(FVector& OutPoint) const
{
	FVector Origin;
	FVector Direction;
	if (!DeprojectMousePositionToWorld(Origin, Direction) || FMath::IsNearlyZero(Direction.Z))
	{
		return false;
	}
	const double T = -Origin.Z / Direction.Z;
	if (T <= 0.0)
	{
		return false;
	}
	OutPoint = Origin + Direction * T;
	return true;
}

void AGGPlayerController::PrimaryPressed()
{
	FVector Point;
	if (AGGHoleDirector* D = Director(); D && CursorOnPlane(Point))
	{
		bPrimaryDown = true;
		D->BeginAim(Point);
	}
}

void AGGPlayerController::PrimaryReleased()
{
	FVector Point;
	if (AGGHoleDirector* D = Director(); D && bPrimaryDown)
	{
		if (CursorOnPlane(Point))
		{
			D->EndAim(Point);
		}
		else
		{
			D->CancelAim();
		}
	}
	bPrimaryDown = false;
}

void AGGPlayerController::PanPressed()
{
	bPanning = CursorOnPlane(PanAnchor);
}

void AGGPlayerController::PanReleased()
{
	bPanning = false;
}

void AGGPlayerController::ZoomIn()
{
	if (AGGCameraPawn* C = CameraPawn())
	{
		C->Zoom(1.f);
	}
}

void AGGPlayerController::ZoomOut()
{
	if (AGGCameraPawn* C = CameraPawn())
	{
		C->Zoom(-1.f);
	}
}

void AGGPlayerController::PlaceGravity()
{
	FVector Point;
	if (AGGHoleDirector* D = Director(); D && CursorOnPlane(Point))
	{
		D->PlaceWell(Point, false);
	}
}

void AGGPlayerController::PlaceAntigravity()
{
	FVector Point;
	if (AGGHoleDirector* D = Director(); D && CursorOnPlane(Point))
	{
		D->PlaceWell(Point, true);
	}
}

void AGGPlayerController::Reset()
{
	if (AGGHoleDirector* D = Director())
	{
		D->ResetHole();
	}
}

void AGGPlayerController::OrbitLeft()
{
	if (AGGCameraPawn* C = CameraPawn())
	{
		C->Orbit(-15.f, 0.f);
	}
}

void AGGPlayerController::OrbitRight()
{
	if (AGGCameraPawn* C = CameraPawn())
	{
		C->Orbit(15.f, 0.f);
	}
}

void AGGPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	FVector Point;
	if (!CursorOnPlane(Point))
	{
		return;
	}
	if (bPrimaryDown)
	{
		if (AGGHoleDirector* D = Director())
		{
			D->UpdateAim(Point);
		}
	}
	if (bPanning)
	{
		if (AGGCameraPawn* C = CameraPawn())
		{
			// Keep the grabbed point under the cursor.
			C->Pan(PanAnchor - Point);
		}
	}
}
