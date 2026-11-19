#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"
#include "Sim/GGSim.h"

#include "GGRival.generated.h"

class AGGCourse;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/**
 * A bot to race: another caddie working the same hole in a ghost copy of its own. It carries its own (ghost)
 * machines to where it thinks they go, takes its own swing at its own (ghost) ball, watches what happens and tries
 * again. Its attempts start from the hole's verified answer with error added; each failure shrinks the error, so it
 * always gets there. Skill sets how big the error starts, how fast it shrinks, how long it thinks and how quickly
 * it moves. It never touches the player's ball or machines.
 */
UCLASS()
class GRAVITYGOLF_API AGGRival : public AActor
{
	GENERATED_BODY()

public:
	AGGRival();

	virtual void Tick(float DeltaSeconds) override;

	/** Start on a course. Skill is 1 (easy), 2 (medium) or 3 (hard). */
	void Begin(AGGCourse* InCourse, int32 InSkill);

	/** A few words for the HUD on what the rival is doing. */
	FString StatusLine() const;
	bool Solved() const { return bSolved; }
	double Seconds() const { return Elapsed; }
	int32 Swings() const { return SwingsTaken; }

private:
	enum class EStep : uint8
	{
		Think,
		FetchMachine,
		CarryMachine,
		GoToBall,
		Address,
		Watch,
		Done,
	};

	void PlanAttempt();
	void Launch();
	void Finish(GG::EBallState State);
	/** Move toward a ground point, arcing up over whatever is in the way. True on arrival. */
	bool Travel(const FVector& Ground, float DeltaSeconds);
	std::vector<GG::FAttractor> Fields() const;
	UStaticMeshComponent* Part(UStaticMesh* Mesh, USceneComponent* Parent, const FVector& Offset);

	UPROPERTY()
	TObjectPtr<AGGCourse> Course;

	UPROPERTY()
	TObjectPtr<USceneComponent> Root;
	UPROPERTY()
	TObjectPtr<USceneComponent> Body;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Head;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Ball;
	UPROPERTY()
	TArray<TObjectPtr<USceneComponent>> Machines;
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> Light;

	FRandomStream Random;
	int32 Skill = 2;
	EStep Step = EStep::Think;
	double StepTime = 0.0;
	double Elapsed = 0.0;
	bool bSolved = false;
	int32 SwingsTaken = 0;

	// Its own copy of the hole.
	GG::FSim Sim;
	GG::FVec3 Lie;
	std::vector<bool> Banked;
	TArray<FVector> Bases;
	int32 Leg = 0;
	/** Switches the verified answer for this leg presses: an attempt that reaches the pad without them doesn't count. */
	std::vector<bool> Needed;
	double Accumulator = 0.0;

	// The attempt under way.
	TArray<FVector> Targets;
	/** Which way the rival has each mirror turned. */
	TArray<int32> Turns;
	double Yaw = 0.0;
	double Power = 0.0;
	int32 Carrying = INDEX_NONE;
	/** This attempt's carries, in order: which machine, and where to. */
	TArray<TPair<int32, FVector>> Moves;
	int32 NextMove = 0;
	/** 1 on the first attempt at a leg, shrinking with every miss. */
	double Error = 1.0;
	int32 Misses = 0;

	// Where it is.
	FVector Position = FVector::ZeroVector;
	FVector TravelFrom = FVector::ZeroVector;
	double TravelLength = 0.0;
	double TravelDone = 0.0;
	float Facing = 0.f;
};
