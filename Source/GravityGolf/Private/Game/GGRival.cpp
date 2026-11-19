#include "Game/GGRival.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Game/GGAudio.h"
#include "Game/GGCourse.h"
#include "Materials/MaterialInstanceDynamic.h"

DEFINE_LOG_CATEGORY_STATIC(LogGGRival, Log, All);

namespace
{
// Per skill (index 1 easy, 2 medium, 3 hard): how wrong the first attempt is, how much of that survives each miss,
// how long it stands thinking, and how fast it travels.
struct FSkill
{
	double PlaceError;   // cm
	double YawError;     // degrees
	double PowerError;   // fraction of full power
	double Keep;         // error multiplier per miss
	double ThinkMin;
	double ThinkMax;
	double Speed;        // cm/s
};
const FSkill Skills[] = {
	{0, 0, 0, 0, 0, 0, 0},
	{520.0, 16.0, 0.22, 0.82, 7.0, 13.0, 420.0},
	{320.0, 10.0, 0.14, 0.68, 4.0, 7.0, 620.0},
	{150.0, 5.0, 0.07, 0.5, 1.5, 3.0, 900.0},
};
const FLinearColor Rose(1.f, 0.3f, 0.55f);

FVector ToUnreal(const GG::FVec3& V)
{
	return FVector(V.X, V.Y, V.Z);
}

GG::FVec3 ToSim(const FVector& V)
{
	return {V.X, V.Y, V.Z};
}
} // namespace

AGGRival::AGGRival()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

UStaticMeshComponent* AGGRival::Part(UStaticMesh* Mesh, USceneComponent* Parent, const FVector& Offset)
{
	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this);
	Component->SetStaticMesh(Mesh);
	Component->SetMobility(EComponentMobility::Movable);
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// It moves every frame: keep it out of the bounced-light scene (see the notes on Lumen in CLAUDE.md).
	Component->bAffectDynamicIndirectLighting = false;
	Component->bAffectDistanceFieldLighting = false;
	Component->bVisibleInRayTracing = false;
	Component->SetupAttachment(Parent);
	Component->SetRelativeLocation(Offset);
	Component->RegisterComponent();
	return Component;
}

void AGGRival::Begin(AGGCourse* InCourse, int32 InSkill)
{
	Course = InCourse;
	Skill = FMath::Clamp(InSkill, 1, 3);
	Random.Initialize(GetTypeHash(Course->CourseId()) * 31 + Skill);

	UMaterialInterface* Surface = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_GG_Surface.M_GG_Surface"));
	UMaterialInterface* Glow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_GG_Glow.M_GG_Glow"));
	UStaticMesh* BodyMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Meshes/SM_Avatar_Body.SM_Avatar_Body"));
	UStaticMesh* HeadMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Meshes/SM_Avatar_Head.SM_Avatar_Head"));
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (!Surface || !Glow || !BodyMesh || !HeadMesh)
	{
		return;
	}
	Light = UMaterialInstanceDynamic::Create(Glow, this);
	Light->SetVectorParameterValue(TEXT("Color"), Rose);
	Light->SetScalarParameterValue(TEXT("Strength"), 2.2f);
	// A darker shell than a player's, so nobody mistakes the rival for a friend.
	UMaterialInstanceDynamic* Shell = UMaterialInstanceDynamic::Create(Surface, this);
	Shell->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.05f, 0.05f, 0.06f));
	Shell->SetScalarParameterValue(TEXT("Metallic"), 0.f);
	Shell->SetScalarParameterValue(TEXT("Roughness"), 0.3f);
	UMaterialInstanceDynamic* Trim = UMaterialInstanceDynamic::Create(Surface, this);
	Trim->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.42f, 0.24f, 0.09f));
	Trim->SetScalarParameterValue(TEXT("Metallic"), 1.f);
	Trim->SetScalarParameterValue(TEXT("Roughness"), 0.3f);

	Body = NewObject<USceneComponent>(this);
	Body->SetMobility(EComponentMobility::Movable);
	Body->SetupAttachment(Root);
	Body->RegisterComponent();
	UStaticMeshComponent* Torso = Part(BodyMesh, Body, FVector::ZeroVector);
	Head = Part(HeadMesh, Body, FVector(0.0, 0.0, 152.0));
	for (UStaticMeshComponent* Piece : {Torso, Head.Get()})
	{
		Piece->SetMaterial(0, Shell);
		Piece->SetMaterial(1, Trim);
		Piece->SetMaterial(2, Light);
	}

	// Its ball and its machines are light only: ghosts that share the place without touching anything.
	Ball = Part(Sphere, Root, FVector::ZeroVector);
	Ball->SetRelativeScale3D(FVector(0.13));
	Ball->SetMaterial(0, Light);
	Ball->SetCastShadow(false);
	const double Core = Course->MachineCoreHeight();
	for (const FVector& Base : Course->StartingBases())
	{
		USceneComponent* Machine = NewObject<USceneComponent>(this);
		Machine->SetMobility(EComponentMobility::Movable);
		Machine->SetupAttachment(Root);
		Machine->RegisterComponent();
		UStaticMeshComponent* Ring = Part(Cylinder, Machine, FVector(0.0, 0.0, 1.5));
		Ring->SetRelativeScale3D(FVector(0.62, 0.62, 0.006));
		UStaticMeshComponent* Stem = Part(Cylinder, Machine, FVector(0.0, 0.0, Core * 0.5));
		Stem->SetRelativeScale3D(FVector(0.012, 0.012, Core / 100.0));
		UStaticMeshComponent* Orb = Part(Sphere, Machine, FVector(0.0, 0.0, Core));
		Orb->SetRelativeScale3D(FVector(0.1));
		for (UStaticMeshComponent* Piece : {Ring, Stem, Orb})
		{
			Piece->SetMaterial(0, Light);
			Piece->SetCastShadow(false);
		}
		Machines.Add(Machine);
		Bases.Add(Base);
	}

	Lie = Course->Rules().Tee;
	Banked.assign(Course->Rules().Switches.size(), false);
	// It starts beside the player, on the other side of the ball.
	const FVector Tee = ToUnreal(Lie);
	Position = FVector(Tee.X - 220.0, Tee.Y + 260.0, 0.0);
	Leg = 0;
	Error = 1.0;
	PlanAttempt();
}

std::vector<GG::FAttractor> AGGRival::Fields() const
{
	// The player's machines' strengths and reaches, standing where the rival has put its own.
	std::vector<GG::FAttractor> Out = Course->MachineFields();
	for (int32 I = 0; I < Bases.Num() && I < static_cast<int32>(Out.size()); ++I)
	{
		Out[I].Position = ToSim(Bases[I] + FVector(0.0, 0.0, Course->MachineCoreHeight()));
	}
	return Out;
}

void AGGRival::PlanAttempt()
{
	const FSkill& S = Skills[Skill];
	const FGGSolution* Answer = Course->SolutionForLeg(Leg);
	Targets = Bases;
	Yaw = 0.0;
	Power = 0.3;
	if (Answer)
	{
		// What the right answer presses on the way: an attempt that reaches a pad without it has not done the leg.
		std::vector<GG::FAttractor> Exact = Course->MachineFields();
		for (int32 I = 0; I < Answer->Machines.Num() && I < static_cast<int32>(Exact.size()); ++I)
		{
			Exact[I].Position = ToSim(Answer->Machines[I] + FVector(0.0, 0.0, Course->MachineCoreHeight()));
		}
		std::vector<bool> Held = Banked;
		GG::HoldWeightPlates(Course->Rules(), Exact, Held);
		GG::FSim Check;
		const FVector Direction = FRotator(0.0, Answer->Yaw, 0.0).Vector() * (Answer->Power * Course->PhysicsRules().MaxLaunchSpeed);
		Check.Begin(Course->Rules(), Exact, Lie, ToSim(Direction), Course->PhysicsRules(), &Held);
		Check.Run();
		Needed.assign(Banked.size(), false);
		for (size_t I = 0; I < Needed.size(); ++I)
		{
			Needed[I] = !Course->Rules().Switches[I].bWeight && Check.SwitchPressed(static_cast<int>(I));
		}

		// The answer, plus this attempt's error. A machine that belongs on a weight plate is easy to see, so the
		// rival always gets that one right.
		for (int32 I = 0; I < Targets.Num() && I < Answer->Machines.Num(); ++I)
		{
			// (The same goes for a hole with plinths: the rival's mistakes are in its swing, not in which plinth.)
			bool bOnPlate = Course->HasPlinths();
			for (const GG::FSwitch& Switch : Course->Rules().Switches)
			{
				bOnPlate |= Switch.bWeight && FVector::Dist2D(Answer->Machines[I], ToUnreal(Switch.Center)) <= Switch.Radius;
			}
			const double Radius = bOnPlate ? 0.0 : S.PlaceError * Error * FMath::Sqrt(Random.FRand());
			const double Angle = Random.FRandRange(0.f, 2.f * PI);
			Targets[I] = Answer->Machines[I] + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0) * Radius;
			Targets[I].X = FMath::RoundToDouble(Targets[I].X);
			Targets[I].Y = FMath::RoundToDouble(Targets[I].Y);
		}
		Yaw = Answer->Yaw + S.YawError * Error * Random.FRandRange(-1.f, 1.f);
		Power = FMath::Clamp(Answer->Power + S.PowerError * Error * Random.FRandRange(-1.f, 1.f), 0.05, 1.0);
	}
	NextMachine = 0;
	Carrying = INDEX_NONE;
	Step = EStep::Think;
	StepTime = Random.FRandRange(static_cast<float>(S.ThinkMin), static_cast<float>(S.ThinkMax)) * (Misses == 0 && Leg == 0 ? 0.5 : 1.0);
}

bool AGGRival::Travel(const FVector& Ground, float DeltaSeconds)
{
	if (TravelLength <= 0.0)
	{
		TravelFrom = Position;
		TravelLength = FMath::Max(FVector::Dist2D(Position, Ground), 1.0);
		TravelDone = 0.0;
	}
	TravelDone = FMath::Min(TravelDone + Skills[Skill].Speed * DeltaSeconds, TravelLength);
	const double T = TravelDone / TravelLength;
	Position = FMath::Lerp(TravelFrom, FVector(Ground.X, Ground.Y, 0.0), T);
	// It hovers, so it goes over walls instead of round them: higher the further it has to go.
	Position.Z = FMath::Sin(T * PI) * FMath::Min(60.0 + TravelLength * 0.12, 420.0);
	const FVector Way = Ground - TravelFrom;
	if (!Way.IsNearlyZero(1.0))
	{
		Facing = static_cast<float>(Way.Rotation().Yaw);
	}
	if (T >= 1.0)
	{
		TravelLength = 0.0;
		return true;
	}
	return false;
}

void AGGRival::Launch()
{
	++SwingsTaken;
	std::vector<bool> Held = Banked;
	const std::vector<GG::FAttractor> Machines2 = Fields();
	GG::HoldWeightPlates(Course->Rules(), Machines2, Held);
	const FVector Velocity = FRotator(0.0, Yaw, 0.0).Vector() * (Power * Course->PhysicsRules().MaxLaunchSpeed);
	Sim.Begin(Course->Rules(), Machines2, Lie, ToSim(Velocity), Course->PhysicsRules(), &Held);
	Accumulator = 0.0;
	Step = EStep::Watch;
}

void AGGRival::Finish(GG::EBallState State)
{
	const int32 Zones = static_cast<int32>(Course->Rules().SafeZones.size());
	bool bLegDone = false;
	if (State == GG::EBallState::Sunk)
	{
		bSolved = true;
		Step = EStep::Done;
		UE_LOG(LogGGRival, Display, TEXT("Rival: skill %d solved %s in %.0f s, %d swings"), Skill, *Course->CourseName(), Elapsed, SwingsTaken);
		return;
	}
	if (State == GG::EBallState::AtRest && Sim.Ball().SafeZone >= 0 && Sim.Ball().SafeZone < Zones)
	{
		bLegDone = true;
		for (size_t I = 0; I < Needed.size(); ++I)
		{
			bLegDone &= !Needed[I] || Banked[I] || Sim.SwitchPressed(static_cast<int>(I));
		}
	}
	UE_LOG(LogGGRival, Log, TEXT("Rival: leg %d swing %d %s (error %.2f) at %.0f s"), Leg + 1, SwingsTaken, bLegDone ? TEXT("reached the pad") : TEXT("missed"),
		Error, Elapsed);
	if (bLegDone)
	{
		for (size_t I = 0; I < Banked.size(); ++I)
		{
			Banked[I] = Banked[I] || (!Course->Rules().Switches[I].bWeight && Sim.SwitchPressed(static_cast<int>(I)));
		}
		const GG::FSafeZone& Zone = Course->Rules().SafeZones[Sim.Ball().SafeZone];
		Lie = Zone.Center + GG::FVec3{0.0, 0.0, Course->Rules().BallRadius};
		Leg = Sim.Ball().SafeZone + 1;
		Error = 1.0;
		Misses = 0;
	}
	else
	{
		// A miss teaches it something: the next try is closer.
		Error *= Skills[Skill].Keep;
		++Misses;
		if (Misses > 14)
		{
			Error = 0.0;
		}
	}
	PlanAttempt();
}

void AGGRival::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Course || !Body)
	{
		return;
	}
	if (!bSolved)
	{
		Elapsed += DeltaSeconds;
	}
	FVector BallAt = ToUnreal(Lie);

	switch (Step)
	{
	case EStep::Think:
		StepTime -= DeltaSeconds;
		if (StepTime <= 0.0)
		{
			Step = EStep::FetchMachine;
		}
		break;
	case EStep::FetchMachine:
		// Skip machines that already stand where this attempt wants them.
		while (NextMachine < Bases.Num() && FVector::Dist2D(Bases[NextMachine], Targets[NextMachine]) < 1.0)
		{
			++NextMachine;
		}
		if (NextMachine >= Bases.Num())
		{
			Step = EStep::GoToBall;
		}
		else if (Travel(Bases[NextMachine], DeltaSeconds))
		{
			Carrying = NextMachine;
			Step = EStep::CarryMachine;
		}
		break;
	case EStep::CarryMachine:
		if (Travel(Targets[Carrying], DeltaSeconds))
		{
			Bases[Carrying] = Targets[Carrying];
			Carrying = INDEX_NONE;
			++NextMachine;
			Step = EStep::FetchMachine;
		}
		break;
	case EStep::GoToBall:
	{
		const FVector Aim = FRotator(0.0, Yaw, 0.0).Vector();
		if (Travel(BallAt - Aim * 150.0, DeltaSeconds))
		{
			Facing = static_cast<float>(Yaw);
			Step = EStep::Address;
			StepTime = 0.9;
		}
		break;
	}
	case EStep::Address:
		StepTime -= DeltaSeconds;
		if (StepTime <= 0.0)
		{
			Launch();
		}
		break;
	case EStep::Watch:
	{
		Accumulator += DeltaSeconds;
		int32 Steps = 0;
		while (Accumulator >= GG::FSim::Dt && Steps < 480 && !Sim.Finished())
		{
			Sim.Step();
			Accumulator -= GG::FSim::Dt;
			++Steps;
		}
		BallAt = ToUnreal(Sim.Ball().Position);
		// It watches its ball go.
		const FVector Look = BallAt - Position;
		if (!Look.IsNearlyZero(10.0))
		{
			Facing = static_cast<float>(Look.Rotation().Yaw);
		}
		if (Sim.Finished())
		{
			Finish(Sim.Ball().State);
		}
		break;
	}
	case EStep::Done:
		BallAt = ToUnreal(Course->Rules().Cup) - FVector(0.0, 0.0, 6.0);
		break;
	}

	// Put everything where it is.
	const float Time = GetWorld()->GetTimeSeconds();
	Body->SetWorldLocation(Position + FVector(0.0, 0.0, 5.0 * FMath::Sin(Time * 2.3f + 1.3f)));
	Body->SetWorldRotation(FMath::RInterpTo(Body->GetComponentRotation(), FRotator(0.f, Facing, 0.f), DeltaSeconds, 6.f));
	Ball->SetWorldLocation(BallAt);
	for (int32 I = 0; I < Machines.Num(); ++I)
	{
		const FVector At = I == Carrying ? Position + FRotator(0.f, Facing, 0.f).Vector() * 70.0 + FVector(0.0, 0.0, 60.0) : Bases[I];
		Machines[I]->SetWorldLocation(At);
	}
}

FString AGGRival::StatusLine() const
{
	static const TCHAR* Names[] = {TEXT(""), TEXT("Easy"), TEXT("Medium"), TEXT("Hard")};
	const int32 Whole = FMath::FloorToInt32(Elapsed);
	if (bSolved)
	{
		return FString::Printf(TEXT("Rival (%s) solved it in %d:%02d, %d swing%s"), Names[Skill], Whole / 60, Whole % 60, SwingsTaken,
			SwingsTaken == 1 ? TEXT("") : TEXT("s"));
	}
	const TCHAR* Doing = Step == EStep::Think ? TEXT("thinking") : Step == EStep::Watch ? TEXT("watching its shot")
		: Step == EStep::Address || Step == EStep::GoToBall ? TEXT("lining up") : TEXT("moving a machine");
	return FString::Printf(TEXT("Rival (%s)   %d swing%s   %s"), Names[Skill], SwingsTaken, SwingsTaken == 1 ? TEXT("") : TEXT("s"), Doing);
}
