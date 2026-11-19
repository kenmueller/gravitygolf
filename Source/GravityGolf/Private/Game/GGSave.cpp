#include "Game/GGSave.h"

#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"

namespace
{
const TCHAR* Slot = TEXT("GravityGolf");
UGGSave* Loaded = nullptr;
} // namespace

const TArray<FGGHoleInfo>& UGGSave::Holes()
{
	static const TArray<FGGHoleInfo> All = {
		{TEXT("Terraces"), TEXT("terraces"), TEXT("The Terraces"), TEXT("A garden over the sea"), 1},
		{TEXT("SaltMirror"), TEXT("saltmirror"), TEXT("Salt Mirror"), TEXT("The brine flats at sunset"), 2},
		{TEXT("Canyon"), TEXT("canyon"), TEXT("The Narrows"), TEXT("A slot canyon at noon"), 1},
		{TEXT("Lantern"), TEXT("lantern"), TEXT("Lantern Court"), TEXT("A walled court at night"), 2},
		{TEXT("Returning"), TEXT("returning"), TEXT("The Long Way Round"), TEXT("The canyon in the evening"), 2},
	};
	return All;
}

int32 UGGSave::HoleIndex(const FString& CourseId)
{
	const TArray<FGGHoleInfo>& All = Holes();
	for (int32 I = 0; I < All.Num(); ++I)
	{
		if (CourseId.Equals(All[I].Course, ESearchCase::IgnoreCase))
		{
			return I;
		}
	}
	return INDEX_NONE;
}

UGGSave& UGGSave::Get()
{
	if (!Loaded)
	{
		if (UGameplayStatics::DoesSaveGameExist(Slot, 0))
		{
			Loaded = Cast<UGGSave>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
		}
		if (!Loaded)
		{
			Loaded = Cast<UGGSave>(UGameplayStatics::CreateSaveGameObject(UGGSave::StaticClass()));
		}
		// Lives for the whole session, across level changes.
		Loaded->AddToRoot();
	}
	return *Loaded;
}

void UGGSave::Store()
{
	UGameplayStatics::SaveGameToSlot(&Get(), Slot, 0);
}

bool UGGSave::RecordSolve(const FString& CourseId, int32 Swings)
{
	const int32 Before = Best(CourseId);
	if (Before != 0 && Before <= Swings)
	{
		return false;
	}
	BestSwings.Add(CourseId.ToLower(), Swings);
	Store();
	return true;
}

int32 UGGSave::Best(const FString& CourseId) const
{
	const int32* Found = BestSwings.Find(CourseId.ToLower());
	return Found ? *Found : 0;
}

bool UGGSave::IsUnlocked(int32 Index) const
{
	const TArray<FGGHoleInfo>& All = Holes();
	return Index == 0 || (All.IsValidIndex(Index) && Best(All[Index - 1].Course) > 0) || (All.IsValidIndex(Index) && Best(All[Index].Course) > 0);
}

int32 UGGSave::NextUnsolved() const
{
	const TArray<FGGHoleInfo>& All = Holes();
	for (int32 I = 0; I < All.Num(); ++I)
	{
		if (Best(All[I].Course) == 0)
		{
			return I;
		}
	}
	return All.Num() - 1;
}

void UGGSave::ApplyQuality() const
{
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		Settings->SetOverallScalabilityLevel(FMath::Clamp(Quality, 0, 3));
		Settings->ApplyNonResolutionSettings();
	}
}
