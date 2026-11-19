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
#include "Game/GGHUD.h"
#include "Game/GGSave.h"
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
		// The club moves with the view every frame; keep it out of the bounced-light scene so it isn't re-captured.
		Part->bAffectDynamicIndirectLighting = false;
		Part->bAffectDistanceFieldLighting = false;
		Part->bVisibleInRayTracing = false;
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
	auto Paint = [&](UStaticMeshComponent* Part, int32 Slot, const FLinearColor& Color, float Metallic, float Roughness) {
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, this);
		Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
		Material->SetScalarParameterValue(TEXT("Metallic"), Metallic);
		Material->SetScalarParameterValue(TEXT("Roughness"), Roughness);
		Part->SetMaterial(Slot, Material);
	};

	// The putter's real meshes are generated by Tools/build_meshes.py in the pivot's own space. Without them the
	// club falls back to the basic shapes set up in the constructor.
	UStaticMesh* Grip = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Meshes/SM_Putter_Grip.SM_Putter_Grip"));
	UStaticMesh* Shaft = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Meshes/SM_Putter_Shaft.SM_Putter_Shaft"));
	UStaticMesh* Head = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Meshes/SM_Putter_Head.SM_Putter_Head"));
	if (Grip && Shaft && Head)
	{
		ClubGrip->SetStaticMesh(Grip);
		ClubShaft->SetStaticMesh(Shaft);
		ClubHead->SetStaticMesh(Head);
		for (UStaticMeshComponent* Part : {ClubShaft.Get(), ClubGrip.Get(), ClubHead.Get()})
		{
			Part->SetRelativeTransform(FTransform::Identity);
		}
	}
	Paint(ClubShaft, 0, FLinearColor(0.62f, 0.63f, 0.65f), 1.f, 0.18f);
	Paint(ClubGrip, 0, FLinearColor(0.012f, 0.012f, 0.014f), 0.f, 0.82f);
	Paint(ClubHead, 0, FLinearColor(0.46f, 0.47f, 0.49f), 1.f, 0.3f);
	if (Head)
	{
		Paint(ClubHead, 1, FLinearColor(0.92f, 0.92f, 0.9f), 0.f, 0.5f);
		Paint(ClubHead, 2, FLinearColor(0.75f, 0.42f, 0.18f), 1.f, 0.26f);
	}
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
	// The menu: Tab, P or Escape opens and closes it; arrows and Enter work it. (In the editor, Escape ends play.)
	Input->BindKey(EKeys::Tab, IE_Pressed, this, &AGGGolfer::MenuToggle);
	Input->BindKey(EKeys::P, IE_Pressed, this, &AGGGolfer::MenuToggle);
	Input->BindKey(EKeys::Escape, IE_Pressed, this, &AGGGolfer::MenuToggle);
	Input->BindKey(EKeys::Up, IE_Pressed, this, &AGGGolfer::MenuUp);
	Input->BindKey(EKeys::Down, IE_Pressed, this, &AGGGolfer::MenuDown);
	Input->BindKey(EKeys::Left, IE_Pressed, this, &AGGGolfer::MenuLeft);
	Input->BindKey(EKeys::Right, IE_Pressed, this, &AGGGolfer::MenuRight);
	Input->BindKey(EKeys::Enter, IE_Pressed, this, &AGGGolfer::AcceptOrJump);
	// Testing aids: watch the solution, or jump straight to a hole.
	Input->BindKey(EKeys::F1, IE_Pressed, this, &AGGGolfer::ShowSolution);
	Input->BindKey(EKeys::One, IE_Pressed, this, &AGGGolfer::Hole1);
	Input->BindKey(EKeys::Two, IE_Pressed, this, &AGGGolfer::Hole2);
	Input->BindKey(EKeys::Three, IE_Pressed, this, &AGGGolfer::Hole3);
	Input->BindKey(EKeys::Four, IE_Pressed, this, &AGGGolfer::Hole4);
	Input->BindKey(EKeys::Five, IE_Pressed, this, &AGGGolfer::Hole5);
	Input->BindKey(EKeys::Six, IE_Pressed, this, &AGGGolfer::Hole6);
	Input->BindKey(EKeys::Seven, IE_Pressed, this, &AGGGolfer::Hole7);

	// Gamepad: left stick walks (click it to run), right stick looks, right trigger swings.
	Input->BindAxisKey(EKeys::Gamepad_LeftY, this, &AGGGolfer::PadForward);
	Input->BindAxisKey(EKeys::Gamepad_LeftX, this, &AGGGolfer::PadRight);
	Input->BindAxisKey(EKeys::Gamepad_RightX, this, &AGGGolfer::PadTurn);
	Input->BindAxisKey(EKeys::Gamepad_RightY, this, &AGGGolfer::PadLookUp);
	Input->BindKey(EKeys::Gamepad_RightTrigger, IE_Pressed, this, &AGGGolfer::SwingPressed);
	Input->BindKey(EKeys::Gamepad_RightTrigger, IE_Released, this, &AGGGolfer::SwingReleased);
	Input->BindKey(EKeys::Gamepad_LeftTrigger, IE_Pressed, this, &AGGGolfer::WatchPressed);
	Input->BindKey(EKeys::Gamepad_LeftTrigger, IE_Released, this, &AGGGolfer::WatchReleased);
	Input->BindKey(EKeys::Gamepad_FaceButton_Bottom, IE_Pressed, this, &AGGGolfer::AcceptOrJump);
	Input->BindKey(EKeys::Gamepad_Special_Right, IE_Pressed, this, &AGGGolfer::MenuToggle);
	Input->BindKey(EKeys::Gamepad_DPad_Up, IE_Pressed, this, &AGGGolfer::MenuUp);
	Input->BindKey(EKeys::Gamepad_DPad_Down, IE_Pressed, this, &AGGGolfer::MenuDown);
	Input->BindKey(EKeys::Gamepad_DPad_Left, IE_Pressed, this, &AGGGolfer::MenuLeft);
	Input->BindKey(EKeys::Gamepad_DPad_Right, IE_Pressed, this, &AGGGolfer::MenuRight);
	Input->BindKey(EKeys::Gamepad_FaceButton_Left, IE_Pressed, this, &AGGGolfer::Interact);
	Input->BindKey(EKeys::Gamepad_FaceButton_Top, IE_Pressed, this, &AGGGolfer::GoToBall);
	Input->BindKey(EKeys::Gamepad_FaceButton_Right, IE_Pressed, this, &AGGGolfer::ReturnBall);
	Input->BindKey(EKeys::Gamepad_Special_Left, IE_Pressed, this, &AGGGolfer::ResetHole);

	// Touch: every finger goes through HandleTouch.
	Input->BindTouch(IE_Pressed, this, &AGGGolfer::TouchPressed);
	Input->BindTouch(IE_Repeat, this, &AGGGolfer::TouchMoved);
	Input->BindTouch(IE_Released, this, &AGGGolfer::TouchReleased);
}

AGGHUD* AGGGolfer::Hud() const
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	return PC ? Cast<AGGHUD>(PC->GetHUD()) : nullptr;
}

bool AGGGolfer::MenuOpen() const
{
	const AGGHUD* Screen = Hud();
	return Screen && Screen->MenuOpen();
}

void AGGGolfer::MenuToggle()
{
	if (AGGHUD* Screen = Hud())
	{
		if (Screen->MenuOpen())
		{
			Screen->MenuBack();
		}
		else
		{
			bCharging = false;
			Screen->OpenMenu(EGGMenu::Pause);
		}
	}
}

void AGGGolfer::MenuUp()
{
	if (AGGHUD* Screen = Hud())
	{
		Screen->MenuMove(-1, 0);
	}
}

void AGGGolfer::MenuDown()
{
	if (AGGHUD* Screen = Hud())
	{
		Screen->MenuMove(1, 0);
	}
}

void AGGGolfer::MenuLeft()
{
	if (AGGHUD* Screen = Hud())
	{
		Screen->MenuMove(0, -1);
	}
}

void AGGGolfer::MenuRight()
{
	if (AGGHUD* Screen = Hud())
	{
		Screen->MenuMove(0, 1);
	}
}

void AGGGolfer::AcceptOrJump()
{
	if (MenuOpen())
	{
		Hud()->MenuActivate();
	}
	else
	{
		Jump();
	}
}

void AGGGolfer::AddLook(float YawDegrees, float PitchDegrees)
{
	if (MenuOpen())
	{
		return;
	}
	// The player's own look speed and up/down preference apply to every way of looking.
	const UGGSave& Save = UGGSave::Get();
	YawDegrees *= Save.LookSensitivity;
	PitchDegrees *= Save.LookSensitivity * (Save.bInvertLook ? -1.f : 1.f);
	if (AController* PC = GetController())
	{
		FRotator Look = PC->GetControlRotation();
		Look.Yaw += YawDegrees;
		Look.Pitch = FMath::ClampAngle(Look.Pitch + PitchDegrees, -80.f, 80.f);
		PC->SetControlRotation(Look);
	}
}

void AGGGolfer::PadForward(float Value)
{
	if (!MenuOpen())
	{
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AGGGolfer::PadRight(float Value)
{
	if (!MenuOpen())
	{
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AGGGolfer::PadTurn(float Value)
{
	AddLook(Value * 150.f * GetWorld()->GetDeltaSeconds(), 0.f);
}

void AGGGolfer::PadLookUp(float Value)
{
	AddLook(0.f, Value * 95.f * GetWorld()->GetDeltaSeconds());
}

FVector2D AGGGolfer::ViewportSize() const
{
	int32 X = 0;
	int32 Y = 0;
	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->GetViewportSize(X, Y);
	}
	return FVector2D(X, Y);
}

void AGGGolfer::TouchButtons(const FVector2D& View, TArray<FGGTouchButton>& Out) const
{
	const AGGCourse* C = Course();
	if (!C)
	{
		return;
	}
	// Everything is sized from the screen's height, so it fits a phone held either way.
	const float U = static_cast<float>(View.Y) * 0.01f;
	auto Add = [&](FName Id, float FromRight, float FromBottom, float Radius, const TCHAR* Label) {
		FGGTouchButton Button;
		Button.Id = Id;
		Button.Center = FVector2D(View.X - FromRight * U, View.Y - FromBottom * U);
		Button.Radius = Radius * U;
		Button.Label = Label;
		for (const TPair<int32, FName>& Held : HeldButtons)
		{
			Button.bDown |= Held.Value == Id;
		}
		Out.Add(Button);
	};
	const EGGPhase Phase = C->Phase();
	// Under the right thumb: the one big thing you can do now.
	if (Phase == EGGPhase::Sunk)
	{
		Add(TEXT("Swing"), 17.f, 19.f, 10.5f, C->HasNext() ? TEXT("NEXT") : TEXT("AGAIN"));
	}
	else if (Phase == EGGPhase::Flying)
	{
		Add(TEXT("Watch"), 17.f, 19.f, 9.f, TEXT("WATCH"));
	}
	else if (CanSwing() || bCharging)
	{
		Add(TEXT("Swing"), 17.f, 19.f, 10.5f, TEXT("SWING"));
	}
	if (IsCarrying() || CanPickUp())
	{
		Add(TEXT("Grab"), 39.f, 12.f, 7.5f, IsCarrying() ? TEXT("PLACE") : TEXT("GRAB"));
	}
	// Along the top, out of the way: the things you do now and then.
	if (Phase == EGGPhase::Ready || Phase == EGGPhase::Penalty)
	{
		const TCHAR* Labels[] = {TEXT("RETURN"), TEXT("BALL")};
		const TCHAR* Ids[] = {TEXT("Return"), TEXT("Ball")};
		for (int32 I = 0; I < 2; ++I)
		{
			Add(Ids[I], 19.5f + 11.5f * I, 100.f - 8.5f, 4.8f, Labels[I]);
		}
	}
	// The menu is always within reach.
	Add(TEXT("Menu"), 8.f, 100.f - 8.5f, 4.8f, TEXT("MENU"));
}

bool AGGGolfer::TouchStick(FVector2D& OutOrigin, FVector2D& OutKnob) const
{
	OutOrigin = MoveOrigin;
	OutKnob = MoveAt;
	return MoveFinger != INDEX_NONE;
}

void AGGGolfer::PressButton(FName Id)
{
	if (Id == TEXT("Swing"))
	{
		SwingPressed();
	}
	else if (Id == TEXT("Grab"))
	{
		Interact();
	}
	else if (Id == TEXT("Watch"))
	{
		bWatching = true;
	}
	else if (Id == TEXT("Ball"))
	{
		GoToBall();
	}
	else if (Id == TEXT("Return"))
	{
		ReturnBall();
	}
	else if (Id == TEXT("Menu"))
	{
		MenuToggle();
	}
}

void AGGGolfer::ReleaseButton(FName Id)
{
	if (Id == TEXT("Swing"))
	{
		SwingReleased();
	}
	else if (Id == TEXT("Watch"))
	{
		bWatching = false;
	}
}

void AGGGolfer::HandleTouch(int32 Finger, int32 Phase, const FVector2D& At)
{
	bTouchMode = true;
	if (MenuOpen())
	{
		// In a menu a tap is a choice.
		if (Phase == 0)
		{
			Hud()->MenuPointer(At, true);
		}
		MoveFinger = LookFinger = INDEX_NONE;
		return;
	}
	const FVector2D View = ViewportSize();
	if (View.Y <= 0.0)
	{
		return;
	}
	const double U = View.Y * 0.01;
	if (Phase == 0)
	{
		LastTouch.Add(Finger, At);
		TArray<FGGTouchButton> Buttons;
		TouchButtons(View, Buttons);
		for (const FGGTouchButton& Button : Buttons)
		{
			// A little slack around each button: thumbs are not precise.
			if (FVector2D::Distance(At, Button.Center) <= Button.Radius * 1.2)
			{
				HeldButtons.Add(Finger, Button.Id);
				PressButton(Button.Id);
				return;
			}
		}
		// The left part of the screen is the movement stick, wherever the thumb lands. Anything else looks around.
		if (MoveFinger == INDEX_NONE && At.X < View.X * 0.42 && At.Y > 16.0 * U)
		{
			MoveFinger = Finger;
			MoveOrigin = MoveAt = At;
		}
		else if (LookFinger == INDEX_NONE)
		{
			LookFinger = Finger;
		}
	}
	else if (Phase == 1)
	{
		const FVector2D Delta = At - LastTouch.FindRef(Finger);
		LastTouch.Add(Finger, At);
		if (Finger == MoveFinger)
		{
			MoveAt = At;
		}
		else if (Finger == LookFinger)
		{
			// A swipe across the screen's height turns half a circle.
			AddLook(static_cast<float>(Delta.X / View.Y * 180.0), static_cast<float>(-Delta.Y / View.Y * 120.0));
		}
		else if (const FName* Held = HeldButtons.Find(Finger))
		{
			// Sliding the thumb that holds SWING trims the aim, so one hand can charge and steer.
			if (*Held == TEXT("Swing"))
			{
				AddLook(static_cast<float>(Delta.X / View.Y * 60.0), 0.f);
			}
		}
	}
	else
	{
		if (const FName* Held = HeldButtons.Find(Finger))
		{
			const FName Id = *Held;
			HeldButtons.Remove(Finger);
			ReleaseButton(Id);
		}
		if (Finger == MoveFinger)
		{
			MoveFinger = INDEX_NONE;
		}
		if (Finger == LookFinger)
		{
			LookFinger = INDEX_NONE;
		}
		LastTouch.Remove(Finger);
	}
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
	if (!MenuOpen())
	{
		AddMovementInput(GetActorForwardVector(), Value - Back);
	}
}

void AGGGolfer::MoveRight(float Value)
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	const float Left = PC && PC->IsInputKeyDown(EKeys::A) ? 1.f : 0.f;
	if (!MenuOpen())
	{
		AddMovementInput(GetActorRightVector(), Value - Left);
	}
}

void AGGGolfer::Turn(float Value)
{
	// The engine turns mouse counts into degrees at 2.5 per unit of yaw input; AddLook works in degrees.
	AddLook(Value * 0.55f * 2.5f, 0.f);
}

void AGGGolfer::LookUp(float Value)
{
	AddLook(0.f, Value * 0.55f * 2.5f);
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
	if (MenuOpen())
	{
		float X = 0.f;
		float Y = 0.f;
		const APlayerController* PC = Cast<APlayerController>(GetController());
		// A click chooses what the mouse is over; a gamepad trigger chooses the highlighted row.
		if (PC && PC->IsInputKeyDown(EKeys::LeftMouseButton) && PC->GetMousePosition(X, Y))
		{
			Hud()->MenuPointer(FVector2D(X, Y), true);
		}
		else
		{
			Hud()->MenuActivate();
		}
		return;
	}
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
	if (!C || C->Phase() == EGGPhase::Flying || MenuOpen())
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

	// The touch stick: how far the thumb is from where it went down, out to a thumb's reach.
	FVector2D Stick = FVector2D::ZeroVector;
	if (MoveFinger != INDEX_NONE && !MenuOpen())
	{
		const double Reach = FMath::Max(ViewportSize().Y * 0.09, 1.0);
		Stick = (MoveAt - MoveOrigin) / Reach;
		if (Stick.SizeSquared() > 1.0)
		{
			Stick.Normalize();
		}
		AddMovementInput(GetActorForwardVector(), static_cast<float>(-Stick.Y));
		AddMovementInput(GetActorRightVector(), static_cast<float>(Stick.X));
	}

	// Run: hold Shift, click the left stick, or push the touch stick all the way.
	const APlayerController* Player = Cast<APlayerController>(GetController());
	const bool bRunKey = Player && (Player->IsInputKeyDown(EKeys::LeftShift) || Player->IsInputKeyDown(EKeys::Gamepad_LeftThumbstick));
	const bool bSprint = (bRunKey || Stick.SizeSquared() > 0.9) && !bCharging;
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
	const AGGHUD* Screen = Hud();
	const bool bMenu = Screen && Screen->MenuOpen();
	C->SetFieldEmphasis(!bMenu && (IsCarrying() || CanSwing() || bCharging || SwingTime >= 0.f || C->Phase() == EGGPhase::Flying));
	if (bMenu)
	{
		// Behind a menu the place is the picture: no club in frame, and on the title screen the view drifts.
		ClubPivot->SetVisibility(false, true);
		if (Screen->TitleOpen())
		{
			if (AController* PC = GetController())
			{
				FRotator Look = PC->GetControlRotation();
				Look.Yaw += 2.5f * DeltaSeconds;
				Look.Pitch = FMath::FInterpTo(FRotator::NormalizeAxis(Look.Pitch), 4.f, DeltaSeconds, 0.6f);
				PC->SetControlRotation(Look);
			}
		}
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
