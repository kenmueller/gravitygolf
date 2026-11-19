#include "Game/GGGolfer.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Game/GGAudio.h"
#include "Game/GGCourse.h"
#include "Game/GGGameMode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
constexpr double SwingReach = 320.0;
constexpr double PickUpReach = 380.0;
constexpr double HoldDistance = 170.0;
constexpr float ChargeSeconds = 1.4f;
constexpr float DownswingSeconds = 0.14f;
constexpr float RecoverSeconds = 0.6f;
constexpr double StrideLength = 150.0;
constexpr float WalkSpeed = 480.f;
constexpr float SprintSpeed = 950.f;
constexpr float ClubLength = 72.f;

// The club's pose in view space, tunable live from the console while playing.
TAutoConsoleVariable<float> CVarClubX(TEXT("gg.Club.X"), 34.f, TEXT("Club grip: forward of the eyes (cm)"));
TAutoConsoleVariable<float> CVarClubY(TEXT("gg.Club.Y"), 21.f, TEXT("Club grip: right of the eyes (cm)"));
TAutoConsoleVariable<float> CVarClubZ(TEXT("gg.Club.Z"), -19.f, TEXT("Club grip: above the eyes (cm)"));
TAutoConsoleVariable<float> CVarClubYaw(TEXT("gg.Club.Yaw"), -17.f, TEXT("Club at address: yaw (degrees)"));
TAutoConsoleVariable<float> CVarClubPitch(TEXT("gg.Club.Pitch"), -15.f, TEXT("Club at address: pitch (degrees)"));
TAutoConsoleVariable<float> CVarClubBackYaw(TEXT("gg.Club.BackYaw"), 38.f, TEXT("Backswing: extra yaw at full power"));
TAutoConsoleVariable<float> CVarClubBackPitch(TEXT("gg.Club.BackPitch"), 16.f, TEXT("Backswing: extra pitch at full power"));
TAutoConsoleVariable<float> CVarClubThroughYaw(TEXT("gg.Club.ThroughYaw"), -46.f, TEXT("Follow-through: extra yaw"));
TAutoConsoleVariable<float> CVarClubThroughPitch(TEXT("gg.Club.ThroughPitch"), -6.f, TEXT("Follow-through: extra pitch"));
TAutoConsoleVariable<float> CVarClubCarryYaw(TEXT("gg.Club.CarryYaw"), 12.f, TEXT("Walking: extra yaw"));
TAutoConsoleVariable<float> CVarClubCarryPitch(TEXT("gg.Club.CarryPitch"), -34.f, TEXT("Walking: extra pitch (negative points the head down)"));
TAutoConsoleVariable<float> CVarClubCarryDrop(TEXT("gg.Club.CarryDrop"), 10.f, TEXT("Walking: how far the grip drops (cm)"));
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

	// The club is held like a first-person tool: grip low on the right, head out ahead where you can see it.
	// Its parts lie along the pivot's forward axis.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	ClubPivot = CreateDefaultSubobject<USceneComponent>(TEXT("ClubPivot"));
	ClubPivot->SetupAttachment(Camera);

	const FRotator AlongForward(90.f, 0.f, 0.f); // the cylinder's long axis is Z
	ClubShaft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ClubShaft"));
	ClubShaft->SetupAttachment(ClubPivot);
	ClubShaft->SetStaticMesh(Cylinder.Object);
	ClubShaft->SetRelativeLocationAndRotation(FVector(ClubLength * 0.5f, 0.f, 0.f), AlongForward);
	ClubShaft->SetRelativeScale3D(FVector(0.011f, 0.011f, ClubLength / 100.f));

	ClubGrip = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ClubGrip"));
	ClubGrip->SetupAttachment(ClubPivot);
	ClubGrip->SetStaticMesh(Cylinder.Object);
	ClubGrip->SetRelativeLocationAndRotation(FVector(9.f, 0.f, 0.f), AlongForward);
	ClubGrip->SetRelativeScale3D(FVector(0.022f, 0.022f, 0.24f));

	// A blade putter head: long across, short along the shaft, hanging just below it.
	ClubHead = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ClubHead"));
	ClubHead->SetupAttachment(ClubPivot);
	ClubHead->SetStaticMesh(Cube.Object);
	ClubHead->SetRelativeLocation(FVector(ClubLength, -3.5f, -2.2f));
	ClubHead->SetRelativeScale3D(FVector(0.034f, 0.1f, 0.05f));

	for (UStaticMeshComponent* Part : {ClubShaft.Get(), ClubGrip.Get(), ClubHead.Get()})
	{
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCastShadow(false);
	}

	bUseControllerRotationYaw = true;
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->MaxWalkSpeed = WalkSpeed;
	Movement->JumpZVelocity = 480.f;
	Movement->AirControl = 0.4f;
	Movement->BrakingDecelerationWalking = 1800.f;
}

void AGGGolfer::BeginPlay()
{
	Super::BeginPlay();

	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_GG_Surface.M_GG_Surface"));
	if (!Base)
	{
		return;
	}
	auto Metal = [&](UStaticMeshComponent* Part, const FLinearColor& Color, float Metallic, float Roughness) {
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, this);
		Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
		Material->SetScalarParameterValue(TEXT("Metallic"), Metallic);
		Material->SetScalarParameterValue(TEXT("Roughness"), Roughness);
		Part->SetMaterial(0, Material);
	};
	Metal(ClubShaft, FLinearColor(0.55f, 0.56f, 0.58f), 1.f, 0.22f);
	Metal(ClubGrip, FLinearColor(0.015f, 0.015f, 0.017f), 0.f, 0.75f);
	Metal(ClubHead, FLinearColor(0.3f, 0.19f, 0.08f), 1.f, 0.34f);
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
	Input->BindKey(EKeys::R, IE_Pressed, this, &AGGGolfer::ReturnBall);
	Input->BindKey(EKeys::BackSpace, IE_Pressed, this, &AGGGolfer::ResetHole);
	Input->BindKey(EKeys::B, IE_Pressed, this, &AGGGolfer::GoToBall);
	Input->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AGGGolfer::WatchPressed);
	Input->BindKey(EKeys::RightMouseButton, IE_Released, this, &AGGGolfer::WatchReleased);
	// Testing aids until there is a menu: watch the solution, or jump between holes.
	Input->BindKey(EKeys::F1, IE_Pressed, this, &AGGGolfer::ShowSolution);
	Input->BindKey(EKeys::One, IE_Pressed, this, &AGGGolfer::Hole1);
	Input->BindKey(EKeys::Two, IE_Pressed, this, &AGGGolfer::Hole2);
	Input->BindKey(EKeys::Three, IE_Pressed, this, &AGGGolfer::Hole3);
	Input->BindKey(EKeys::Four, IE_Pressed, this, &AGGGolfer::Hole4);
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
	// A putter: the ball leaves along the ground in the direction you face.
	return FRotator(0.f, GetControlRotation().Yaw, 0.f).Vector();
}

void AGGGolfer::SwingPressed()
{
	AGGCourse* C = Course();
	if (C && C->Phase() == EGGPhase::Sunk)
	{
		C->Advance();
		return;
	}
	if (CanSwing())
	{
		bCharging = true;
		ChargeAmount = 0.f;
	}
}

void AGGGolfer::StartSwing()
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

void AGGGolfer::SwingReleased()
{
	if (bCharging)
	{
		StartSwing();
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

bool AGGGolfer::PlaceCarried(const FVector& Ground)
{
	AGGCourse* C = Course();
	if (!C || !IsCarrying())
	{
		return false;
	}
	if (!C->CanPlaceMachine(Ground))
	{
		C->Notify(TEXT("The machine won't stand on the court."));
		return false;
	}
	C->MoveMachine(Carried, Ground, false);
	GGAudio::MachinePlace(this, Ground);
	Carried = INDEX_NONE;
	return true;
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
			PlaceCarried(Ground);
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

void AGGGolfer::ReturnBall()
{
	if (AGGCourse* C = Course())
	{
		bCharging = false;
		SwingTime = -1.f;
		C->ShowGuide({});
		C->ReturnBall();
	}
}

void AGGGolfer::ResetHole()
{
	if (AGGCourse* C = Course())
	{
		Carried = INDEX_NONE;
		bCharging = false;
		SwingTime = -1.f;
		C->ShowGuide({});
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

bool AGGGolfer::TestCarryTo(const FVector& Ground)
{
	AGGCourse* C = Course();
	if (!C)
	{
		return false;
	}
	if (!IsCarrying())
	{
		Carried = C->FindMachine(GetActorLocation(), 1.0e9);
	}
	return PlaceCarried(Ground);
}

bool AGGGolfer::TestPlaceMachine(int32 Index, const FVector& Ground)
{
	Carried = Index;
	return PlaceCarried(Ground);
}

void AGGGolfer::ShowSolution()
{
	if (AGGGameMode* Mode = GetWorld()->GetAuthGameMode<AGGGameMode>())
	{
		Mode->GGDemo();
	}
}

void AGGGolfer::JumpToHole(int32 Number)
{
	if (AGGGameMode* Mode = GetWorld()->GetAuthGameMode<AGGGameMode>())
	{
		Mode->GGHole(Number);
	}
}

void AGGGolfer::TestSwing(float Yaw, float Power)
{
	GoToBall();
	if (AController* PC = GetController())
	{
		PC->SetControlRotation(FRotator(-8.f, Yaw, 0.f));
	}
	if (CanSwing())
	{
		ChargeAmount = FMath::Clamp(Power, 0.f, 1.f);
		StartSwing();
	}
}

void AGGGolfer::UpdateClub(float DeltaSeconds)
{
	// ClubPhase runs -1 (top of the backswing) .. 0 (address) .. +1 (end of the follow-through).
	float Target = 0.f;
	if (bCharging)
	{
		Target = -ChargeAmount;
	}
	else if (SwingTime >= 0.f)
	{
		SwingTime += DeltaSeconds;
		const float Back = -SwingPower;
		const float Through = 0.35f + 0.65f * SwingPower;
		if (SwingTime < DownswingSeconds)
		{
			// Accelerate through the ball.
			const float T = SwingTime / DownswingSeconds;
			Target = FMath::Lerp(Back, Through, T * T);
		}
		else
		{
			const float T = (SwingTime - DownswingSeconds) / RecoverSeconds;
			Target = FMath::Lerp(Through, 0.f, FMath::SmoothStep(0.3f, 1.f, T));
			if (T >= 1.f)
			{
				SwingTime = -1.f;
			}
		}
		if (!bStruck && (Target >= 0.f || SwingTime >= DownswingSeconds || SwingTime < 0.f))
		{
			bStruck = true;
			if (AGGCourse* C = Course())
			{
				C->Swing(SwingAim, SwingPower);
			}
		}
	}
	ClubPhase = (bCharging || SwingTime >= 0.f) ? Target : FMath::FInterpTo(ClubPhase, 0.f, DeltaSeconds, 10.f);

	// Away from the ball the club is carried low, head down, mostly out of view; at the ball it comes up to address.
	const bool bAddressing = CanSwing() || bCharging || SwingTime >= 0.f;
	ClubRaise = FMath::FInterpTo(ClubRaise, bAddressing ? 0.f : 1.f, DeltaSeconds, 7.f);

	const float Back = FMath::Max(-ClubPhase, 0.f);
	const float Through = FMath::Max(ClubPhase, 0.f);
	const float Yaw = CVarClubYaw.GetValueOnGameThread() + Back * CVarClubBackYaw.GetValueOnGameThread() +
		Through * CVarClubThroughYaw.GetValueOnGameThread() + ClubRaise * CVarClubCarryYaw.GetValueOnGameThread();
	const float Pitch = CVarClubPitch.GetValueOnGameThread() + Back * CVarClubBackPitch.GetValueOnGameThread() +
		Through * CVarClubThroughPitch.GetValueOnGameThread() + ClubRaise * CVarClubCarryPitch.GetValueOnGameThread();
	ClubPivot->SetRelativeLocation(FVector(CVarClubX.GetValueOnGameThread(), CVarClubY.GetValueOnGameThread() + ClubRaise * 6.f,
		CVarClubZ.GetValueOnGameThread() - ClubRaise * CVarClubCarryDrop.GetValueOnGameThread()));
	ClubPivot->SetRelativeRotation(FRotator(Pitch, Yaw, Back * 14.f - Through * 10.f));
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

	// Hold Shift to run.
	const APlayerController* Player = Cast<APlayerController>(GetController());
	const bool bSprint = Player && Player->IsInputKeyDown(EKeys::LeftShift) && !bCharging;
	GetCharacterMovement()->MaxWalkSpeed = bSprint ? SprintSpeed : WalkSpeed;

	// Fell off the world: back to the ball.
	if (GetActorLocation().Z < C->KillHeight())
	{
		GetCharacterMovement()->StopMovementImmediately();
		GoToBall();
	}

	UpdateClub(DeltaSeconds);
	UpdateFootsteps();

	if (IsCarrying())
	{
		// Carried at the hip, a little ahead.
		C->MoveMachine(Carried, HoldPoint() - FVector(0, 0, 95.0), true);
	}

	// The field shows strongly while it matters: carrying a machine, standing over the ball, or watching a shot.
	C->SetFieldEmphasis(IsCarrying() || CanSwing() || bCharging || SwingTime >= 0.f || C->Phase() == EGGPhase::Flying);

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
		if (C->Phase() != EGGPhase::Ready || DistanceToBall() >= SwingReach)
		{
			bCharging = false;
			C->ShowGuide({});
		}
		else
		{
			ChargeAmount = FMath::Min(ChargeAmount + DeltaSeconds / ChargeSeconds, 1.f);
			// Guide: the start of the path only. Reading the rest is the puzzle.
			TArray<FVector> Points;
			C->PreviewSwing(SwingDirection(), ChargeAmount, 1.2, Points);
			C->ShowGuide(Points);
		}
	}
}
