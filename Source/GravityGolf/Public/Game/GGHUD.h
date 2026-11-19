#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"

#include "GGHUD.generated.h"

class AGGCourse;
class AGGGolfer;
class UFont;

/** The screens that can cover the game. */
enum class EGGMenu : uint8
{
	None,
	Title,
	Pause,
	Holes,
	Settings,
};

/**
 * Everything drawn over the world: the play HUD (hole name, swings, prompts, power bar), the touch controls, the
 * solved card, and the menus (title, pause, hole list, settings). Drawn on the canvas, so it is all code.
 */
UCLASS()
class GRAVITYGOLF_API AGGHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	bool MenuOpen() const { return Menu != EGGMenu::None; }
	void OpenMenu(EGGMenu Which);
	void CloseMenu();
	/** Move the highlight by rows, or change the highlighted value by columns. */
	void MenuMove(int32 Rows, int32 Columns);
	void MenuActivate();
	/** Back one screen; from the pause screen, back to the game. */
	void MenuBack();
	/** A mouse or finger at a viewport position. With bPress it also activates what is under it. */
	void MenuPointer(const FVector2D& At, bool bPress);

private:
	struct FItem
	{
		FString Label;
		FString Value;
		FName Action;
		int32 Number = 0;
		bool bEnabled = true;
		FBox2D Rect = FBox2D(ForceInit);
	};

	void BuildMenu();
	void RunAction(const FItem& Item, int32 Direction);
	void OpenHole(int32 Index);
	void DrawPlay(const AGGGolfer* Golfer, const AGGCourse* Course);
	void DrawTouch(const AGGGolfer* Golfer);
	void DrawMenu();
	/** Crisp text at any size. Size is the height of a capital letter in pixels, roughly. */
	void Label(const FString& Line, float X, float Y, float Size, const FLinearColor& Color, bool bCentered = false, bool bBold = false);
	void Ring(const FVector2D& Center, float Radius, const FLinearColor& Color, float Thickness);
	void Disc(const FVector2D& Center, float Radius, const FLinearColor& Color);
	/** One unit of layout: a pixel on a 1080-line screen. */
	float Unit() const;

	UPROPERTY()
	TObjectPtr<UFont> TextFont;

	EGGMenu Menu = EGGMenu::None;
	/** The screen that opened the hole list or the settings, to go back to. */
	EGGMenu Parent = EGGMenu::None;
	TArray<FItem> Items;
	int32 Selected = 0;
	FVector2D LastMouse = FVector2D(-1.0, -1.0);
};
