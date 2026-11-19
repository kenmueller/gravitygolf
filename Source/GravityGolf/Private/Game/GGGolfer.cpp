#include "Game/GGGolfer.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Game/GGAudio.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"
#include "Game/GGCourse.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputCoreTypes.h"

namespace
{
constexpr double SwingReach = 320.0;
constexpr double PickUpReach = 380.0;
constexpr double HoldDistance = 170.0;
constexpr float ChargeSeconds = 1.4f;
constexpr float BackswingDegrees = 62.f;
constexpr float DownswingSeconds = 0.13f;
constexpr float FollowThroughDegrees = 48.f;
constexpr float RecoverSeconds = 0.55f;
constexpr double StrideLength = 150.0;
/** The club rests angled ahead of the hands so it stays in view, as first-person tools do. */
constexpr float ClubRestPitch = 64.f;
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

	// The club: a steel shaft and a bronze head, held low on the right and swung like a pendulum.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	ClubPivot = CreateDefaultSubobject<USceneComponent>(TEXT("ClubPivot"));
	ClubPivot->SetupAttachment(Camera);
	ClubPivot->SetRelativeLocation(FVector(52.f, 20.f, -24.f));
	ClubShaft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ClubShaft"));
	ClubShaft->SetupAttachment(ClubPivot);
	ClubShaft->SetStaticMesh(Cylinder.Object);
	ClubShaft->SetRelativeLocation(FVector(0.f, 0.f, -46.f));
	ClubShaft->SetRelativeScale3D(FVector(0.014f, 0.014f, 0.92f));
	ClubHead = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ClubHead"));
	ClubHead->SetupAttachment(ClubPivot);
	ClubHead->SetStaticMesh(Cube.Object);
	ClubHead->SetRelativeLocation(FVector(1.f, -5.f, -93.f));
	ClubHead->SetRelativeScale3D(FVector(0.03f, 0.13f, 0.035f));
	for (UStaticMeshComponent* Part : {ClubShaft.Get(), ClubHead.Get()})
	{
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCastShadow(false);
	}

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
	Input->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AGGGolfer::WatchPressed);
	Input->BindKey(EKeys::RightMouseButton, IE_Released, this, &AGGGolfer::WatchReleased);
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
	return C && C->Phase() == EGGPhase::Ready && !IsCarrying() && SwingTime < 0.f && DistanceToBall() < SwingReach;
}

FVector AGGGolfer::HoldPoint() const
{
	return Camera->GetComponentLocation() + GetControlRotation().Vector() * HoldDistance;
}

bool AGGGolfer::CanPickUp() const
{
	const AGGCourse* C = Course();
	return C && !IsCarrying() && C->Phase() != EGGPhase::Flying && C->FindMachine(HoldPoint(), PickUpReach) != INDEX_NONE;
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
		// Release starts the downswing; the ball leaves when the head reaches it.
		bCharging = false;
		SwingTime = 0.f;
		bStruck = false;
		SwingPower = ChargeAmount;
		SwingAim = SwingDirection();
		if (AGGCourse* C = Course())
		{
			C->ShowGuide({});
		}
	}
}

bool AGGGolfer::GroundBelow(const FVector& From, FVector& OutGround) const
{
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, From + FVector(0, 0, 60), From - FVector(0, 0, 600), ECC_Visibility, Params))
	{
		OutGround = Hit.ImpactPoint;
		return true;
	}
	return false;
}

void AGGGolfer::Interact()
{
	AGGCourse* C = Course();
	if (!C || C->Phase() == EGGPhase::Flying)
	{
		return;
	}
	if (IsCarrying())
	{
		// Set the machine down on its feet where you're looking. No ground there, no placement.
		FVector Ground;
		if (GroundBelow(HoldPoint(), Ground))
		{
			C->MoveMachine(Carried, Ground, false);
			GGAudio::MachinePlace(this, Ground);
			Carried = INDEX_NONE;
		}
	}
	else
	{
		Carried = C->FindMachine(HoldPoint(), PickUpReach);
		if (IsCarrying())
		{
			GGAudio::MachinePickUp(this, C->MachineBase(Carried));
		}
	}
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

void AGGGolfer::UpdateClub(float DeltaSeconds)
{
	float Target = 0.f;
	if (bCharging)
	{
		Target = -BackswingDegrees * ChargeAmount;
	}
	else if (SwingTime >= 0.f)
	{
		SwingTime += DeltaSeconds;
		const float Back = -BackswingDegrees * SwingPower;
		const float Through = FollowThroughDegrees * (0.35f + 0.65f * SwingPower);
		if (SwingTime < DownswingSeconds)
		{
			// Accelerate through the ball.
			const float T = SwingTime / DownswingSeconds;
			Target = FMath::Lerp(Back, Through, T * T);
			if (!bStruck && Target >= 0.f)
			{
				bStruck = true;
				if (AGGCourse* C = Course())
				{
					C->Swing(SwingAim, SwingPower);
				}
			}
		}
		else
		{
			if (!bStruck)
			{
				bStruck = true;
				if (AGGCourse* C = Course())
				{
					C->Swing(SwingAim, SwingPower);
				}
			}
			const float T = (SwingTime - DownswingSeconds) / RecoverSeconds;
			Target = FMath::Lerp(Through, 0.f, FMath::SmoothStep(0.25f, 1.f, T));
			if (T >= 1.f)
			{
				SwingTime = -1.f;
			}
		}
	}
	ClubPitch = (bCharging || SwingTime >= 0.f) ? Target : FMath::FInterpTo(ClubPitch, 0.f, DeltaSeconds, 10.f);

	// Lower the club out of the way while carrying a machine or away from the ball.
	const bool bAddressing = CanSwing() || bCharging || SwingTime >= 0.f;
	const FVector Rest = bAddressing ? FVector(52.f, 20.f, -24.f) : FVector(40.f, 30.f, -34.f);
	ClubPivot->SetRelativeLocation(FMath::VInterpTo(ClubPivot->GetRelativeLocation(), Rest, DeltaSeconds, 8.f));
	ClubPivot->SetRelativeRotation(FRotator(ClubRestPitch + ClubPitch, 0.f, bAddressing ? 0.f : 24.f));
	ClubPivot->SetVisibility(!IsCarrying(), true);
}

void AGGGolfer::UpdateFootsteps()
{
	const FVector Feet = GetActorLocation() - FVector(0, 0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	if (GetCharacterMovement()->IsMovingOnGround())
	{
		StepDistance += FVector::Dist2D(Feet, LastFeet);
		if (StepDistance >= StrideLength)
		{
			StepDistance = 0.0;
			if (const AGGCourse* C = Course())
			{
				GGAudio::Footstep(this, Feet, C->SurfaceAt(Feet));
			}
		}
	}
	LastFeet = Feet;
}

void AGGGolfer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AGGCourse* C = Course();
	if (!C)
	{
		return;
	}

	UpdateClub(DeltaSeconds);
	UpdateFootsteps();

	if (IsCarrying())
	{
		// Carried at the hip, a little ahead.
		C->MoveMachine(Carried, HoldPoint() - FVector(0, 0, 150.0), true);
	}

	// Hold right mouse to watch the ball through a long lens.
	const bool bZoom = bWatching && C->Phase() == EGGPhase::Flying;
	Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView, bZoom ? 28.f : 80.f, DeltaSeconds, 6.f));
	if (bZoom)
	{
		if (AController* PC = GetController())
		{
			const FRotator Look = (C->BallLocation() - Camera->GetComponentLocation()).Rotation();
			PC->SetControlRotation(FMath::RInterpTo(PC->GetControlRotation(), Look, DeltaSeconds, 9.f));
		}
	}

	if (bCharging)
	{
		if (!CanSwing() && SwingTime < 0.f && (C->Phase() != EGGPhase::Ready || DistanceToBall() >= SwingReach))
		{
			bCharging = false;
			C->ShowGuide({});
		}
		else
		{
			ChargeAmount = FMath::Min(ChargeAmount + DeltaSeconds / ChargeSeconds, 1.f);
			// Guide: the start of the path only. Reading the rest is the puzzle.
			TArray<FVector> Points;
			C->PreviewSwing(SwingDirection(), ChargeAmount, 0.7, Points);
			C->ShowGuide(Points);
		}
	}
}
