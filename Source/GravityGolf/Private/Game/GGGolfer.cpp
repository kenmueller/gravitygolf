#include "Game/GGGolfer.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "Game/GGCourse.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputCoreTypes.h"

namespace
{
constexpr double SwingReach = 320.0;
constexpr double PickUpReach = 380.0;
constexpr double HoldDistance = 230.0;
constexpr float ChargeSeconds = 1.4f;
} // namespace

AGGGolfer::AGGGolfer()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
	Camera->bUsePawnControlRotation = true;
	Camera->SetFieldOfView(80.f);

	bUseControllerRotationYaw = true;
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->MaxWalkSpeed = 520.f;
	Movement->JumpZVelocity = 480.f;
	Movement->AirControl = 0.4f;
	Movement->BrakingDecelerationWalking = 1800.f;
}

void AGGGolfer::SetupPlayerInputComponent(UInputComponent* Input)
{
	Super::SetupPlayerInputComponent(Input);

	Input->BindAxisKey(EKeys::W, this, &AGGGolfer::MoveForward);
	Input->BindAxisKey(EKeys::D, this, &AGGGolfer::MoveRight);
	Input->BindAxisKey(EKeys::MouseX, this, &AGGGolfer::Turn);
	Input->BindAxisKey(EKeys::MouseY, this, &AGGGolfer::LookUp);
	Input->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ACharacter::Jump);
	Input->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AGGGolfer::SwingPressed);
	Input->BindKey(EKeys::LeftMouseButton, IE_Released, this, &AGGGolfer::SwingReleased);
	Input->BindKey(EKeys::E, IE_Pressed, this, &AGGGolfer::Interact);
	Input->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &AGGGolfer::LoftUp);
	Input->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &AGGGolfer::LoftDown);
	Input->BindKey(EKeys::R, IE_Pressed, this, &AGGGolfer::ResetHole);
	Input->BindKey(EKeys::B, IE_Pressed, this, &AGGGolfer::GoToBall);
}

AGGCourse* AGGGolfer::Course() const
{
	TActorIterator<AGGCourse> It(GetWorld());
	return It ? *It : nullptr;
}

void AGGGolfer::MoveForward(float Value)
{
	// W is bound as an axis key (1 while held); S is read directly so one binding covers both.
	const APlayerController* PC = Cast<APlayerController>(GetController());
	const float Back = PC && PC->IsInputKeyDown(EKeys::S) ? 1.f : 0.f;
	AddMovementInput(GetActorForwardVector(), Value - Back);
}

void AGGGolfer::MoveRight(float Value)
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	const float Left = PC && PC->IsInputKeyDown(EKeys::A) ? 1.f : 0.f;
	AddMovementInput(GetActorRightVector(), Value - Left);
}

void AGGGolfer::Turn(float Value)
{
	AddControllerYawInput(Value * 0.55f);
}

void AGGGolfer::LookUp(float Value)
{
	AddControllerPitchInput(-Value * 0.55f);
}

double AGGGolfer::DistanceToBall() const
{
	const AGGCourse* C = Course();
	return C ? FVector::Dist(GetActorLocation(), C->BallLocation()) : TNumericLimits<double>::Max();
}

bool AGGGolfer::CanSwing() const
{
	const AGGCourse* C = Course();
	return C && C->Phase() == EGGPhase::Ready && !IsCarrying() && DistanceToBall() < SwingReach;
}

FVector AGGGolfer::HoldPoint() const
{
	return Camera->GetComponentLocation() + GetControlRotation().Vector() * HoldDistance;
}

bool AGGGolfer::CanPickUp() const
{
	const AGGCourse* C = Course();
	return C && !IsCarrying() && C->Phase() != EGGPhase::Flying && C->FindAttractor(HoldPoint(), PickUpReach) != INDEX_NONE;
}

FVector AGGGolfer::SwingDirection() const
{
	// Aim with where you look; loft comes from the selected club.
	const FRotator Aim(LoftDegrees, GetControlRotation().Yaw, 0.f);
	return Aim.Vector();
}

void AGGGolfer::SwingPressed()
{
	AGGCourse* C = Course();
	if (C && C->Phase() == EGGPhase::Sunk)
	{
		C->ResetHole();
		GoToBall();
		return;
	}
	if (CanSwing())
	{
		bCharging = true;
		ChargeAmount = 0.f;
	}
}

void AGGGolfer::SwingReleased()
{
	if (bCharging)
	{
		bCharging = false;
		if (AGGCourse* C = Course())
		{
			C->Swing(SwingDirection(), ChargeAmount);
		}
	}
}

void AGGGolfer::Interact()
{
	AGGCourse* C = Course();
	if (!C || C->Phase() == EGGPhase::Flying)
	{
		return;
	}
	Carried = IsCarrying() ? INDEX_NONE : C->FindAttractor(HoldPoint(), PickUpReach);
}

void AGGGolfer::LoftUp()
{
	LoftDegrees = FMath::Min(LoftDegrees + 15.f, 60.f);
}

void AGGGolfer::LoftDown()
{
	LoftDegrees = FMath::Max(LoftDegrees - 15.f, 0.f);
}

void AGGGolfer::ResetHole()
{
	if (AGGCourse* C = Course())
	{
		Carried = INDEX_NONE;
		C->ResetHole();
		GoToBall();
	}
}

void AGGGolfer::GoToBall()
{
	const AGGCourse* C = Course();
	if (!C || C->Phase() == EGGPhase::Flying)
	{
		return;
	}
	// Stand behind the ball on the line to the cup.
	FVector ToCup = C->CupLocation() - C->BallLocation();
	ToCup.Z = 0.0;
	const FVector Forward = ToCup.GetSafeNormal();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);
	SetActorLocation(C->BallLocation() - Forward * 160.0 - Right * 45.0 + FVector(0, 0, 100.0), false, nullptr,
		ETeleportType::TeleportPhysics);
	if (AController* PC = GetController())
	{
		PC->SetControlRotation(FRotator(-8.f, Forward.Rotation().Yaw, 0.f));
	}
}

void AGGGolfer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AGGCourse* C = Course();
	if (!C)
	{
		return;
	}

	if (IsCarrying())
	{
		C->MoveAttractor(Carried, HoldPoint());
	}

	if (bCharging)
	{
		if (!CanSwing())
		{
			bCharging = false;
		}
		else
		{
			ChargeAmount = FMath::Min(ChargeAmount + DeltaSeconds / ChargeSeconds, 1.f);
			// Guide: the start of the path only. Reading the rest is the puzzle.
			TArray<FVector> Points;
			C->PreviewSwing(SwingDirection(), ChargeAmount, 0.7, Points);
			for (int32 I = 1; I < Points.Num(); I += 2)
			{
				DrawDebugLine(GetWorld(), Points[I - 1], Points[I], FColor(255, 240, 200), false, -1.f, 0, 1.2f);
			}
		}
	}
}
