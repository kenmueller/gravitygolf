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
#include "Game/GGWorkshop.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
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

	// The first-person club is for its own player only.
	for (UStaticMeshComponent* Part : {ClubShaft.Get(), ClubGrip.Get(), ClubHead.Get()})
	{
		Part->SetOnlyOwnerSee(true);
	}

	// The body other players see. It hangs from the capsule's center with its feet (the air under it) on the ground.
	AvatarRoot = CreateDefaultSubobject<USceneComponent>(TEXT("AvatarRoot"));
	AvatarRoot->SetupAttachment(GetCapsuleComponent());
	AvatarRoot->SetRelativeLocation(FVector(0.f, 0.f, -88.f));
	AvatarBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AvatarBody"));
	AvatarBody->SetupAttachment(AvatarRoot);
	AvatarHead = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AvatarHead"));
	AvatarHead->SetupAttachment(AvatarRoot);
	AvatarHead->SetRelativeLocation(FVector(0.f, 0.f, 152.f));
	AvatarClub = CreateDefaultSubobject<USceneComponent>(TEXT("AvatarClub"));
	AvatarClub->SetupAttachment(AvatarRoot);
	// Held between the hands, pointing forward and down to the ground.
	AvatarClub->SetRelativeLocationAndRotation(FVector(30.f, 0.f, 93.f), FRotator(-62.f, 0.f, 0.f));
	for (UStaticMeshComponent* Part : {AvatarBody.Get(), AvatarHead.Get()})
	{
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetOwnerNoSee(true);
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

	// The avatar: ivory shell, bronze trim, and a light in the player's colour.
	UStaticMesh* Body = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Meshes/SM_Avatar_Body.SM_Avatar_Body"));
	UStaticMesh* Face = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Meshes/SM_Avatar_Head.SM_Avatar_Head"));
	UMaterialInterface* GlowBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_GG_Glow.M_GG_Glow"));
	if (Body && Face && GlowBase)
	{
		AvatarBody->SetStaticMesh(Body);
		AvatarHead->SetStaticMesh(Face);
		AvatarLight = UMaterialInstanceDynamic::Create(GlowBase, this);
		AvatarLight->SetScalarParameterValue(TEXT("Strength"), 2.2f);
		for (UStaticMeshComponent* Part : {AvatarBody.Get(), AvatarHead.Get()})
		{
			Paint(Part, 0, FLinearColor(0.72f, 0.69f, 0.62f), 0.f, 0.32f);
			Paint(Part, 1, FLinearColor(0.42f, 0.24f, 0.09f), 1.f, 0.3f);
			Part->SetMaterial(2, AvatarLight);
		}
		// Its putter: the same three meshes as the first-person club.
		if (Grip && Shaft && Head)
		{
			for (UStaticMesh* Mesh : {Grip, Shaft, Head})
			{
				UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
				Part->SetStaticMesh(Mesh);
				Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Part->SetOwnerNoSee(true);
				Part->SetupAttachment(AvatarClub);
				Part->RegisterComponent();
				AvatarClubParts.Add(Part);
			}
			Paint(AvatarClubParts[0], 0, FLinearColor(0.012f, 0.012f, 0.014f), 0.f, 0.82f);
			Paint(AvatarClubParts[1], 0, FLinearColor(0.62f, 0.63f, 0.65f), 1.f, 0.18f);
			Paint(AvatarClubParts[2], 0, FLinearColor(0.46f, 0.47f, 0.49f), 1.f, 0.3f);
			Paint(AvatarClubParts[2], 1, FLinearColor(0.92f, 0.92f, 0.9f), 0.f, 0.5f);
			Paint(AvatarClubParts[2], 2, FLinearColor(0.75f, 0.42f, 0.18f), 1.f, 0.26f);
		}
		OnRep_Seat();
	}
}

void AGGGolfer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGGGolfer, Seat);
}

void AGGGolfer::SetSeat(int32 InSeat)
{
	Seat = InSeat;
	OnRep_Seat();
}

void AGGGolfer::OnRep_Seat()
{
	// Blue, amber, green, rose: one each for the first four players.
	static const FLinearColor Colours[] = {FLinearColor(0.2f, 0.6f, 1.f), FLinearColor(1.f, 0.55f, 0.12f), FLinearColor(0.35f, 1.f, 0.45f),
		FLinearColor(1.f, 0.35f, 0.65f)};
	if (AvatarLight)
	{
		AvatarLight->SetVectorParameterValue(TEXT("Color"), Colours[FMath::Abs(Seat) % UE_ARRAY_COUNT(Colours)]);
	}
}

void AGGGolfer::UpdateAvatar(float DeltaSeconds)
{
	if (!AvatarBody->GetStaticMesh())
	{
		return;
	}
	const float Time = GetWorld()->GetTimeSeconds() + Seat * 1.7f;
	const FVector Velocity = GetVelocity();
	const float Speed = static_cast<float>(Velocity.Size2D());
	// It hovers: a slow bob, and it leans into the way it is going.
	const FVector Local = GetActorRotation().UnrotateVector(Velocity);
	const float Lean = FMath::Clamp(static_cast<float>(Local.X) / 950.f, -1.f, 1.f) * 11.f;
	const float Bank = FMath::Clamp(static_cast<float>(Local.Y) / 950.f, -1.f, 1.f) * 9.f;
	AvatarRoot->SetRelativeLocation(FVector(0.f, 0.f, -88.f + 5.f * FMath::Sin(Time * 2.1f) + FMath::Min(Speed, 600.f) * 0.004f));
	AvatarRoot->SetRelativeRotation(FMath::RInterpTo(AvatarRoot->GetRelativeRotation(), FRotator(-Lean, 0.f, Bank), DeltaSeconds, 6.f));
	// The head looks where its player looks.
	const float Pitch = FMath::Clamp(FRotator::NormalizeAxis(GetBaseAimRotation().Pitch), -50.f, 45.f);
	AvatarHead->SetRelativeRotation(FMath::RInterpTo(AvatarHead->GetRelativeRotation(), FRotator(Pitch, 0.f, 0.f), DeltaSeconds, 10.f));

	// The putter: put away while carrying a machine; drawn back and through when its player swings.
	float Swing = 0.f;
	if (AvatarSwing >= 0.f)
	{
		const float T = AvatarSwing / 0.8f;
		Swing = T < 0.25f ? FMath::Lerp(-55.f, 40.f, T / 0.25f) : FMath::Lerp(40.f, 0.f, FMath::SmoothStep(0.f, 1.f, (T - 0.25f) / 0.75f));
	}
	AvatarClub->SetRelativeRotation(FRotator(-62.f + Swing * 0.5f, Swing * 0.35f, Swing));
	AvatarClub->SetVisibility(!IsCarrying(), true);
}

void AGGGolfer::SetupPlayerInputComponent(UInputComponent* Input)
{
	Super::SetupPlayerInputComponent(Input);

	Input->BindAxisKey(EKeys::MouseX, this, &AGGGolfer::Turn);
	Input->BindAxisKey(EKeys::MouseY, this, &AGGGolfer::LookUp);
	Input->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AGGGolfer::JumpKey);
	Input->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AGGGolfer::SwingPressed);
	Input->BindKey(EKeys::LeftMouseButton, IE_Released, this, &AGGGolfer::SwingReleased);
	Input->BindKey(EKeys::E, IE_Pressed, this, &AGGGolfer::Interact);
	Input->BindKey(EKeys::R, IE_Pressed, this, &AGGGolfer::ReturnKey);
	Input->BindKey(EKeys::BackSpace, IE_Pressed, this, &AGGGolfer::ResetKey);
	Input->BindKey(EKeys::B, IE_Pressed, this, &AGGGolfer::BallKey);
	Input->BindKey(EKeys::T, IE_Pressed, this, &AGGGolfer::BuildKey);
	Input->BindKey(EKeys::F, IE_Pressed, this, &AGGGolfer::TurnKey);
	Input->BindKey(EKeys::Gamepad_RightShoulder, IE_Pressed, this, &AGGGolfer::TurnKey);
	Input->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AGGGolfer::WatchPressed);
	Input->BindKey(EKeys::RightMouseButton, IE_Released, this, &AGGGolfer::WatchReleased);
	// The menu: Tab or Escape opens and closes it; arrows and Enter work it. (In the editor, Escape ends play.)
	Input->BindKey(EKeys::Tab, IE_Pressed, this, &AGGGolfer::MenuToggle);
	Input->BindKey(EKeys::Escape, IE_Pressed, this, &AGGGolfer::MenuToggle);
	Input->BindKey(EKeys::Up, IE_Pressed, this, &AGGGolfer::MenuUp);
	Input->BindKey(EKeys::Down, IE_Pressed, this, &AGGGolfer::MenuDown);
	Input->BindKey(EKeys::Left, IE_Pressed, this, &AGGGolfer::MenuLeft);
	Input->BindKey(EKeys::Right, IE_Pressed, this, &AGGGolfer::MenuRight);
	Input->BindKey(EKeys::Enter, IE_Pressed, this, &AGGGolfer::AcceptOrJump);
	Input->BindKey(EKeys::AnyKey, IE_Pressed, this, &AGGGolfer::AnyKey);
	// Testing aids: watch the solution, or jump straight to a hole.
	Input->BindKey(EKeys::F1, IE_Pressed, this, &AGGGolfer::ShowSolution);
	Input->BindKey(EKeys::One, IE_Pressed, this, &AGGGolfer::Hole1);
	Input->BindKey(EKeys::Two, IE_Pressed, this, &AGGGolfer::Hole2);
	Input->BindKey(EKeys::Three, IE_Pressed, this, &AGGGolfer::Hole3);
	Input->BindKey(EKeys::Four, IE_Pressed, this, &AGGGolfer::Hole4);
	Input->BindKey(EKeys::Five, IE_Pressed, this, &AGGGolfer::Hole5);
	Input->BindKey(EKeys::Six, IE_Pressed, this, &AGGGolfer::Hole6);
	Input->BindKey(EKeys::Seven, IE_Pressed, this, &AGGGolfer::Hole7);
	Input->BindKey(EKeys::Eight, IE_Pressed, this, &AGGGolfer::Hole8);
	Input->BindKey(EKeys::Nine, IE_Pressed, this, &AGGGolfer::Hole9);
	Input->BindKey(EKeys::Zero, IE_Pressed, this, &AGGGolfer::Hole10);
	Input->BindKey(EKeys::Hyphen, IE_Pressed, this, &AGGGolfer::Hole11);
	Input->BindKey(EKeys::Equals, IE_Pressed, this, &AGGGolfer::Hole12);
	Input->BindKey(EKeys::LeftBracket, IE_Pressed, this, &AGGGolfer::Hole13);
	Input->BindKey(EKeys::RightBracket, IE_Pressed, this, &AGGGolfer::Hole14);

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

bool AGGGolfer::Building() const
{
	// In the Workshop, while the hole is being built, the player is a pointer over the table, not a golfer.
	const AGGWorkshop* Workshop = AGGWorkshop::Get(this);
	return Workshop && Workshop->Building();
}

bool AGGGolfer::Busy() const
{
	return MenuOpen() || Building();
}

void AGGGolfer::BuildKey()
{
	// T: between building a hole and testing it.
	AGGWorkshop* Workshop = AGGWorkshop::Get(this);
	if (Workshop && !MenuOpen() && IsLocallyControlled())
	{
		bCharging = false;
		SwingTime = -1.f;
		Workshop->Toggle();
	}
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

// The keys do nothing while a menu is open (a menu may be taking typing).
void AGGGolfer::ReturnKey()
{
	if (!Busy())
	{
		ReturnBall();
	}
}

void AGGGolfer::ResetKey()
{
	if (!Busy())
	{
		ResetHole();
	}
}

void AGGGolfer::BallKey()
{
	if (!Busy())
	{
		GoToBall();
	}
}

void AGGGolfer::AnyKey(FKey Key)
{
	if (AGGHUD* Screen = Hud())
	{
		Screen->MenuType(Key);
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
		JumpKey();
	}
}

void AGGGolfer::JumpKey()
{
	// While the ball is rolling the same key runs it fast instead (see Tick).
	const AGGCourse* C = Course();
	if (!Busy() && !(C && C->Phase() == EGGPhase::Flying))
	{
		Jump();
	}
}

void AGGGolfer::AddLook(float YawDegrees, float PitchDegrees)
{
	if (Busy())
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
	if (!Busy())
	{
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AGGGolfer::PadRight(float Value)
{
	if (!Busy())
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
		Add(TEXT("Swing"), 17.f, 19.f, 10.5f, C->HasNext() ? TEXT("NEXT") : C->Drawn() || GetNetMode() != NM_Standalone ? TEXT("AGAIN") : TEXT("BUILD"));
	}
	else if (Phase == EGGPhase::Flying)
	{
		Add(TEXT("Watch"), 17.f, 19.f, 9.f, TEXT("WATCH"));
		Add(TEXT("Fast"), 39.f, 12.f, 7.5f, TEXT("FAST"));
	}
	else if (CanSwing() || bCharging)
	{
		Add(TEXT("Swing"), 17.f, 19.f, 10.5f, TEXT("SWING"));
	}
	if (IsCarrying() || CanPickUp())
	{
		Add(TEXT("Grab"), 39.f, 12.f, 7.5f, IsCarrying() ? TEXT("PLACE") : TEXT("GRAB"));
	}
	if (MirrorAtHand() != INDEX_NONE)
	{
		Add(TEXT("Turn"), 39.f, 29.f, 6.5f, TEXT("TURN"));
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
	if (AGGWorkshop::Get(this))
	{
		// Testing a hole of the player's own: the way back to the table.
		Add(TEXT("Build"), 42.5f, 100.f - 8.5f, 4.8f, TEXT("BUILD"));
	}
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
	else if (Id == TEXT("Build"))
	{
		BuildKey();
	}
	else if (Id == TEXT("Turn"))
	{
		TurnKey();
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
	if (AGGWorkshop* Workshop = AGGWorkshop::Get(this); Workshop && Workshop->Building())
	{
		// Building: fingers work the tools and the view.
		MoveFinger = LookFinger = INDEX_NONE;
		Workshop->HandleTouch(Finger, Phase, At);
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

bool AGGGolfer::MouseLookSteady(float Value)
{
	// When the window takes hold of the mouse (at launch, or as a menu closes) the first movement it reports is the
	// distance the pointer happened to be from the middle of the window, as if the player had flicked it there. The
	// packaged game opened with the player staring at their feet. So the mouse is not listened to for the first
	// moments of the hole or just after a menu, and no single frame may turn the view by more than a flick could.
	const double Now = GetWorld()->GetRealTimeSeconds();
	if (MenuOpen())
	{
		LookQuietUntil = Now + 0.25;
		return false;
	}
	// The limit is in the engine's mouse units, about eleven to a pixel's worth of movement on this project's
	// settings: 45 is some 500 pixels in one frame. A hard flick at thirty frames a second is about 300. (The first
	// limit, 600, was written as if the units were pixels, and a test that jumped the pointer 1400 pixels turned
	// the view 120 degrees.)
	return GetGameTimeSinceCreation() > 0.75f && Now >= LookQuietUntil && FMath::Abs(Value) < 45.f;
}

void AGGGolfer::Turn(float Value)
{
	// The engine turns mouse counts into degrees at 2.5 per unit of yaw input; AddLook works in degrees.
	if (MouseLookSteady(Value))
	{
		AddLook(Value * 0.55f * 2.5f, 0.f);
	}
}

void AGGGolfer::LookUp(float Value)
{
	if (MouseLookSteady(Value))
	{
		AddLook(0.f, Value * 0.55f * 2.5f);
	}
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
	if (IsLocallyControlled())
	{
		return Camera->GetComponentLocation() + GetControlRotation().Vector() * HoldDistance;
	}
	// Someone else's golfer: where they stand and where their view points, as far as this machine knows.
	return GetPawnViewLocation() + GetBaseAimRotation().Vector() * HoldDistance;
}

void AGGGolfer::GGNet(const FString& Command, const FString& Argument)
{
	AGGCourse* C = Course();
	if (!C)
	{
		UE_LOG(LogTemp, Display, TEXT("Net[%s]: no course on this machine yet"), HasAuthority() ? TEXT("server") : TEXT("client"));
		return;
	}
	TArray<FString> Parts;
	Argument.ParseIntoArray(Parts, TEXT(","));
	TArray<double> N;
	for (const FString& Part : Parts)
	{
		N.Add(FCString::Atod(*Part));
	}
	if (Command == TEXT("turn") && N.Num() == 1)
	{
		if (HasAuthority())
		{
			C->TurnMachine(static_cast<int32>(N[0]));
		}
		else
		{
			ServerTurn(static_cast<int32>(N[0]));
		}
	}
	else if (Command == TEXT("carry") && N.Num() == 1)
	{
		if (HasAuthority())
		{
			C->CarryMachine(static_cast<int32>(N[0]), this);
		}
		else
		{
			ServerCarry(static_cast<int32>(N[0]));
		}
	}
	else if (Command == TEXT("pick") && N.Num() == 1)
	{
		// As the E key does it, rules and all, but at any distance.
		const int32 Index = static_cast<int32>(N[0]);
		if (const TCHAR* Why = C->CarryBar(GetActorLocation(), C->MachineBase(Index), true))
		{
			Tell(Why);
		}
		else if (HasAuthority())
		{
			C->CarryMachine(Index, this);
		}
		else
		{
			ServerCarry(Index);
		}
	}
	else if (Command == TEXT("place") && N.Num() == 3)
	{
		PlaceCarried(FVector(N[0], N[1], N[2]));
	}
	else if (Command == TEXT("swing") && N.Num() == 2)
	{
		const FVector Direction = FRotator(0.0, N[0], 0.0).Vector();
		if (HasAuthority())
		{
			C->Swing(Direction, N[1], this);
		}
		else
		{
			ServerSwing(Direction, static_cast<float>(N[1]));
		}
	}
	else if (Command == TEXT("return"))
	{
		ReturnBall();
	}
	else if (Command == TEXT("reset"))
	{
		ResetHole();
	}
	else if (Command == TEXT("goto"))
	{
		GoToBall();
	}
	else if (Command == TEXT("tp") && N.Num() == 3)
	{
		const FVector Spot(N[0], N[1], N[2]);
		SetActorLocation(Spot, false, nullptr, ETeleportType::TeleportPhysics);
		if (!HasAuthority())
		{
			ServerTeleport(Spot);
		}
	}
	const FVector At = GetActorLocation();
	UE_LOG(LogTemp, Display, TEXT("Net[%s] %s: me (%.0f,%.0f) carrying %d | %s"), HasAuthority() ? TEXT("server") : TEXT("client"), *Command,
		At.X, At.Y, Carried, *C->Describe());
}

void AGGGolfer::Tell(const FString& Message)
{
	if (IsLocallyControlled())
	{
		if (AGGCourse* C = Course())
		{
			C->Notify(Message);
		}
	}
	else if (HasAuthority())
	{
		ClientTell(Message);
	}
}

void AGGGolfer::ClientTell_Implementation(const FString& Message)
{
	if (AGGCourse* C = Course())
	{
		C->Notify(Message);
	}
}

void AGGGolfer::ServerSwing_Implementation(FVector Direction, float Power)
{
	AGGCourse* C = Course();
	// The server takes a swing only from someone standing at the ball with empty hands.
	if (C && !IsCarrying() && DistanceToBall() < SwingReach * 1.6)
	{
		C->Swing(Direction, Power, this);
	}
}

int32 AGGGolfer::MirrorAtHand() const
{
	const AGGCourse* C = Course();
	if (!C || C->Phase() == EGGPhase::Flying)
	{
		return INDEX_NONE;
	}
	if (IsCarrying())
	{
		return C->IsMirror(Carried) ? Carried : INDEX_NONE;
	}
	const int32 Index = C->FindMachine(HoldPoint(), PickUpReach);
	return Index != INDEX_NONE && C->IsMirror(Index) && !C->CarrierOf(Index) ? Index : INDEX_NONE;
}

bool AGGGolfer::HandlingMirror() const
{
	const AGGCourse* C = Course();
	if (!C)
	{
		return false;
	}
	return C->IsMirror(IsCarrying() ? Carried : C->FindMachine(HoldPoint(), PickUpReach));
}

void AGGGolfer::TurnKey()
{
	// F: turn a mirror to its other diagonal, in the hands or where it stands.
	AGGCourse* C = Course();
	const int32 Index = MirrorAtHand();
	if (!C || Busy() || Index == INDEX_NONE)
	{
		return;
	}
	if (HasAuthority())
	{
		C->TurnMachine(Index);
	}
	else
	{
		ServerTurn(Index);
	}
}

void AGGGolfer::ServerTurn_Implementation(int32 Index)
{
	AGGCourse* C = Course();
	if (C && (C->CarrierOf(Index) == this || FVector::Dist(GetActorLocation(), C->MachineBase(Index)) < PickUpReach * 2.5))
	{
		C->TurnMachine(Index);
	}
}

void AGGGolfer::ServerCarry_Implementation(int32 Index)
{
	AGGCourse* C = Course();
	if (C && FVector::Dist(GetActorLocation(), C->MachineBase(Index)) < PickUpReach * 2.5)
	{
		C->CarryMachine(Index, this);
	}
}

void AGGGolfer::ServerPlace_Implementation(FVector Ground)
{
	AGGCourse* C = Course();
	if (C && IsCarrying() && FVector::Dist(GetActorLocation(), Ground) < PickUpReach * 2.5)
	{
		C->PlaceMachine(Carried, Ground, this);
	}
}

void AGGGolfer::ServerReturnBall_Implementation()
{
	if (AGGCourse* C = Course())
	{
		C->ReturnBall();
	}
}

void AGGGolfer::ServerResetHole_Implementation()
{
	if (AGGCourse* C = Course())
	{
		C->ResetHole();
	}
}

void AGGGolfer::ServerAdvance_Implementation()
{
	AGGCourse* C = Course();
	if (C && C->Phase() == EGGPhase::Sunk)
	{
		C->Advance();
	}
}

void AGGGolfer::ServerTeleport_Implementation(FVector Location)
{
	SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
}

void AGGGolfer::ServerSetRunning_Implementation(bool bRun)
{
	GetCharacterMovement()->MaxWalkSpeed = bRun ? SprintSpeed : WalkSpeed;
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
	if (Building())
	{
		return;
	}
	AGGCourse* C = Course();
	if (AGGWorkshop* Workshop = AGGWorkshop::Get(this); Workshop && C && C->Phase() == EGGPhase::Sunk)
	{
		// A hole of the player's own, solved: back to the table.
		Workshop->StartBuilding();
		return;
	}
	if (C && C->Phase() == EGGPhase::Sunk)
	{
		if (HasAuthority())
		{
			C->Advance();
		}
		else
		{
			ServerAdvance();
		}
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
	if (HasAuthority())
	{
		return C->PlaceMachine(Carried, Ground, this);
	}
	ServerPlace(Ground);
	return true;
}

void AGGGolfer::Interact()
{
	AGGCourse* C = Course();
	if (!C || C->Phase() == EGGPhase::Flying || Busy())
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
		const int32 Index = C->FindMachine(HoldPoint(), PickUpReach);
		const TCHAR* Why = Index != INDEX_NONE && !C->CarrierOf(Index) ? C->CarryBar(GetActorLocation(), C->MachineBase(Index), true) : nullptr;
		if (Why)
		{
			// No reaching through the bars for it either (or through a wall, where carrying is the puzzle).
			Tell(Why);
		}
		else if (Index != INDEX_NONE && !C->CarrierOf(Index))
		{
			if (HasAuthority())
			{
				C->CarryMachine(Index, this);
			}
			else
			{
				ServerCarry(Index);
			}
		}
	}
}

void AGGGolfer::ReturnBall()
{
	if (Building())
	{
		return;
	}
	if (AGGCourse* C = Course())
	{
		bCharging = false;
		SwingTime = -1.f;
		C->ShowGuide({});
		if (HasAuthority())
		{
			C->ReturnBall();
		}
		else
		{
			ServerReturnBall();
		}
	}
}

void AGGGolfer::ResetHole()
{
	if (Building())
	{
		return;
	}
	if (AGGCourse* C = Course())
	{
		bCharging = false;
		SwingTime = -1.f;
		C->ShowGuide({});
		if (HasAuthority())
		{
			C->ResetHole();
			GoToBall();
		}
		else
		{
			// The reset happens on the server; walk to the ball once it has come back to this machine.
			ServerResetHole();
			GoToBallIn = 0.4f;
		}
	}
}

void AGGGolfer::GoToBall()
{
	const AGGCourse* C = Course();
	if (!C || C->Phase() == EGGPhase::Flying || Building())
	{
		return;
	}
	// Stand behind the ball on the line to the cup.
	FVector ToCup = C->CupLocation() - C->BallLocation();
	ToCup.Z = 0.0;
	const FVector Forward = ToCup.GetSafeNormal();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);
	const FVector Spot = C->BallLocation() - Forward * 160.0 - Right * 45.0 + FVector(0, 0, 100.0);
	SetActorLocation(Spot, false, nullptr, ETeleportType::TeleportPhysics);
	if (!HasAuthority())
	{
		// The server owns where everyone stands; without telling it, it would pull this player back.
		ServerTeleport(Spot);
	}
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
		C->CarryMachine(C->FindMachine(GetActorLocation(), 1.0e9), this);
	}
	return PlaceCarried(Ground);
}

void AGGGolfer::TestStandAt(const FVector& Ground)
{
	SetActorLocation(Ground + FVector(-70.0, 0.0, 100.0), false, nullptr, ETeleportType::TeleportPhysics);
	LastCarryStand = GetActorLocation();
}

bool AGGGolfer::TestPlaceMachine(int32 Index, const FVector& Ground)
{
	AGGCourse* C = Course();
	if (!C)
	{
		return false;
	}
	if (Carried != Index)
	{
		C->CarryMachine(Index, this);
	}
	return Carried == Index && PlaceCarried(Ground);
}

void AGGGolfer::ShowSolution()
{
	if (Building())
	{
		return;
	}
	if (AGGGameMode* Mode = GetWorld()->GetAuthGameMode<AGGGameMode>())
	{
		Mode->GGDemo();
	}
}

void AGGGolfer::JumpToHole(int32 Number)
{
	if (Busy())
	{
		return;
	}
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
				if (HasAuthority())
				{
					C->Swing(SwingAim, SwingPower, this);
				}
				else
				{
					ServerSwing(SwingAim, SwingPower);
				}
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

	// A shut grille stops whoever carries a machine: they are put back where they stood before the step that crossed
	// it. (So does the top of a wall on a hole with grilles, and the edge of the ground on a hole drawn in the
	// Workshop.) Each machine does this for its own player and the server for everyone, so they agree.
	const TCHAR* Barred = IsCarrying() && (IsLocallyControlled() || HasAuthority()) ? C->CarryBar(LastCarryStand, GetActorLocation()) : nullptr;
	if (Barred)
	{
		SetActorLocation(LastCarryStand, false, nullptr, ETeleportType::TeleportPhysics);
		GetCharacterMovement()->Velocity = FVector::ZeroVector;
		if (IsLocallyControlled())
		{
			Tell(Barred);
		}
	}
	else
	{
		LastCarryStand = GetActorLocation();
	}
	// Every machine draws every golfer's carried machine at that golfer's hip, a little ahead.
	if (IsCarrying())
	{
		C->MoveMachine(Carried, HoldPoint() - FVector(0, 0, 95.0), true);
	}
	if (AvatarSwing >= 0.f)
	{
		AvatarSwing += DeltaSeconds;
		if (AvatarSwing > 0.8f)
		{
			AvatarSwing = -1.f;
		}
	}
	UpdateAvatar(DeltaSeconds);
	if (!IsLocallyControlled())
	{
		return;
	}
	// From here on: only the player at this machine.
	if (Building())
	{
		// Over the Workshop's table there is no golfer to move, and no club in the picture.
		bCharging = false;
		ClubPivot->SetVisibility(false, true);
		return;
	}
	if (GoToBallIn > 0.f)
	{
		GoToBallIn -= DeltaSeconds;
		if (GoToBallIn <= 0.f)
		{
			GoToBall();
		}
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
	if (Player && !Busy())
	{
		// W A S D walk. (Read here rather than bound: they are keys, not axes.)
		AddMovementInput(GetActorForwardVector(), (Player->IsInputKeyDown(EKeys::W) ? 1.f : 0.f) - (Player->IsInputKeyDown(EKeys::S) ? 1.f : 0.f));
		AddMovementInput(GetActorRightVector(), (Player->IsInputKeyDown(EKeys::D) ? 1.f : 0.f) - (Player->IsInputKeyDown(EKeys::A) ? 1.f : 0.f));
	}
	const bool bRunKey = Player && (Player->IsInputKeyDown(EKeys::LeftShift) || Player->IsInputKeyDown(EKeys::Gamepad_LeftThumbstick));
	const bool bSprint = (bRunKey || Stick.SizeSquared() > 0.9) && !bCharging;
	GetCharacterMovement()->MaxWalkSpeed = bSprint ? SprintSpeed : WalkSpeed;
	if (bSprint != bRunning)
	{
		// The server moves this golfer too, and has to use the same speed.
		bRunning = bSprint;
		if (!HasAuthority())
		{
			ServerSetRunning(bSprint);
		}
	}

	// Hold Space (the gamepad's bottom button, or FAST on a touch screen) to run a rolling ball at four times its
	// speed: some answers draw a nearly dead ball along for ten seconds, and nobody should have to sit through that
	// twice. Only this machine's picture of the swing runs fast; where the ball ends up is the same for everyone.
	bool bFast = false;
	if (C->Phase() == EGGPhase::Flying && Player && !MenuOpen())
	{
		bFast = Player->IsInputKeyDown(EKeys::SpaceBar) || Player->IsInputKeyDown(EKeys::Gamepad_FaceButton_Bottom);
		for (const TPair<int32, FName>& Held : HeldButtons)
		{
			bFast |= Held.Value == TEXT("Fast");
		}
	}
	C->TimeScale = bFast ? 4.f : 1.f;

	// Fell off the world: back to the ball.
	if (GetActorLocation().Z < C->KillHeight())
	{
		GetCharacterMovement()->StopMovementImmediately();
		GoToBall();
	}

	UpdateClub(DeltaSeconds);
	UpdateFootsteps();

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
