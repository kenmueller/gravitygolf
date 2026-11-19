#include "Game/GGSave.h"

#include "Engine/GameInstance.h"
#include "HAL/IConsoleManager.h"

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
		{TEXT("Brief"), TEXT("brief"), TEXT("Second Wind"), TEXT("The court, later that night"), 1},
		{TEXT("Colonnade"), TEXT("colonnade"), TEXT("Dead Weight"), TEXT("A hall of columns at dawn"), 1},
		{TEXT("Stones"), TEXT("stones"), TEXT("Stepping Stones"), TEXT("The salt at noon"), 2},
		{TEXT("Glass"), TEXT("glass"), TEXT("The Looking Glass"), TEXT("The canyon in the morning"), 2},
		{TEXT("Periscope"), TEXT("periscope"), TEXT("Periscope"), TEXT("The salt under the moon"), 2},
		{TEXT("Leapfrog"), TEXT("leapfrog"), TEXT("Leapfrog"), TEXT("The court before dawn"), 1},
		{TEXT("AfterYou"), TEXT("afteryou"), TEXT("After You"), TEXT("The hall of columns in the evening"), 2},
		{TEXT("WaitHere"), TEXT("waithere"), TEXT("Wait Here"), TEXT("The canyon at night"), 1},
		{TEXT("Wires"), TEXT("wires"), TEXT("Crossed Wires"), TEXT("The hall of columns at night"), 2},
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

namespace
{
// Automated play uses a scratch save: empty to begin with, never written. It lasts as long as the game it was asked
// for (one play session in the editor), so the player's own save is back the next time they play.
bool bScratchSave = false;
TWeakObjectPtr<UGameInstance> ScratchOwner;
} // namespace

void UGGSave::UseScratch(const UObject* WorldContext)
{
	if (Loaded)
	{
		Loaded->RemoveFromRoot();
	}
	Loaded = Cast<UGGSave>(UGameplayStatics::CreateSaveGameObject(UGGSave::StaticClass()));
	Loaded->AddToRoot();
	bScratchSave = true;
	ScratchOwner = UGameplayStatics::GetGameInstance(WorldContext);
}

UGGSave& UGGSave::Get()
{
	if (bScratchSave && !ScratchOwner.IsValid() && Loaded)
	{
		// The automated session is over: forget its scratch save and read the real one again.
		Loaded->RemoveFromRoot();
		Loaded = nullptr;
		bScratchSave = false;
	}
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
	UGGSave& Save = Get();
	if (!bScratchSave)
	{
		UGameplayStatics::SaveGameToSlot(&Save, Slot, 0);
	}
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
#if PLATFORM_WASM || PLATFORM_IOS || PLATFORM_ANDROID
	// On the web and on phones the device's own profile sets the quality. Nothing is touched until the player asks.
	if (!bQualityChosen)
	{
		return;
	}
#endif
	// Each setting is set on its own, by name, rather than by applying the engine's stored settings wholesale: those
	// carry a rendering resolution of their own, and nothing here should change what it does not mean to.
	auto Set = [](const TCHAR* Name, int32 Value) {
		if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			Variable->Set(Value, ECVF_SetByGameSetting);
		}
	};
	const int32 Level = FMath::Clamp(Quality, 0, 3);
	if (bQualityChosen)
	{
		// Until the player picks a quality, the platform's own defaults stand.
		for (const TCHAR* Group : {TEXT("sg.ViewDistanceQuality"), TEXT("sg.AntiAliasingQuality"), TEXT("sg.ShadowQuality"),
				 TEXT("sg.ReflectionQuality"), TEXT("sg.PostProcessQuality"), TEXT("sg.TextureQuality"), TEXT("sg.EffectsQuality"),
				 TEXT("sg.FoliageQuality"), TEXT("sg.ShadingQuality")})
		{
			Set(Group, Level);
		}
	}
	// Bounced light never goes above the engine's "High". Its top setting cost 2 ms on hole 1 and a split-screen
	// comparison showed no difference anyone could point at.
	Set(TEXT("sg.GlobalIlluminationQuality"), FMath::Min(Level, 2));
#if !(PLATFORM_WASM || PLATFORM_IOS || PLATFORM_ANDROID)
	// The picture is drawn smaller than the window and sharpened up to it (the engine's temporal upscaler). Left
	// alone, a packaged game draws at 100%: about twice the pixels the editor's play window uses by default, which is
	// what every hole was looked at and measured at. So the size is set here: about two thirds at the highest quality.
	static const int32 Percent[] = {50, 58, 64, 70};
	Set(TEXT("r.ScreenPercentage"), Percent[Level]);
#endif
}
