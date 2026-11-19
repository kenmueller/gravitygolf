#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"

#include "GGSave.generated.h"

/** One hole of the game, in play order. */
struct FGGHoleInfo
{
	/** The level under /Game/Levels and the course file under Content/Courses. */
	const TCHAR* Level;
	const TCHAR* Course;
	const TCHAR* Name;
	const TCHAR* Place;
	/** Swings in a perfect solve: one per leg. */
	int32 Perfect;
};

/** What the player keeps between sessions: which holes are solved and in how many swings, and their settings. */
UCLASS()
class GRAVITYGOLF_API UGGSave : public USaveGame
{
	GENERATED_BODY()

public:
	/** Fewest swings per solved course, keyed by course id. A course that isn't here hasn't been solved. */
	UPROPERTY()
	TMap<FString, int32> BestSwings;

	UPROPERTY()
	float LookSensitivity = 1.f;

	UPROPERTY()
	bool bInvertLook = false;

	UPROPERTY()
	float Volume = 1.f;

	/** 0 low to 3 highest. */
	UPROPERTY()
	int32 Quality = 3;

	/** A bot to race on every hole: 0 none, 1 easy, 2 medium, 3 hard. */
	UPROPERTY()
	int32 Rival = 0;

	/** The one save, loaded on first use. */
	static UGGSave& Get();
	/** Write it to disk. */
	static void Store();

	static const TArray<FGGHoleInfo>& Holes();
	static int32 HoleIndex(const FString& CourseId);

	/** Record a solve. True when it beats the player's best. */
	bool RecordSolve(const FString& CourseId, int32 Swings);
	/** Fewest swings on a course, or 0 if it has never been solved. */
	int32 Best(const FString& CourseId) const;
	/** A hole opens when the one before it is solved. */
	bool IsUnlocked(int32 Index) const;
	/** The first hole not solved yet, or the last one when everything is. */
	int32 NextUnsolved() const;
	/** Apply the quality setting to the renderer. */
	void ApplyQuality() const;
};
