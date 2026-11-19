#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"

#include "GGHUD.generated.h"

class AGGCourse;
class AGGGolfer;
class AGGWorkshop;
class UFont;

/** The screens that can cover the game. */
enum class EGGMenu : uint8
{
	None,
	Title,
	Pause,
	Holes,
	Settings,
	Together,
	/** The Workshop's own pause screen, and its list of kept holes. */
	Workshop,
	Designs,
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
	bool TitleOpen() const { return Menu == EGGMenu::Title; }
	void OpenMenu(EGGMenu Which);
	void CloseMenu();
	/** Move the highlight by rows, or change the highlighted value by columns. */
	void MenuMove(int32 Rows, int32 Columns);
	/** Rows in each column when the open menu is laid out in two columns; zero when it is a single column. */
	int32 MenuPerColumn = 0;
	void MenuActivate();
	/** Back one screen; from the pause screen, back to the game. */
	void MenuBack();
	/** A key pressed while a menu is open. True if the menu used it (typing a host's address). */
	bool MenuType(const FKey& Key);
	/** A mouse or finger at a viewport position. With bPress it also activates what is under it. */
	void MenuPointer(const FVector2D& At, bool bPress);

	// The Workshop's building screen: a row of tools along the bottom and a few buttons at the top.
	/** A press at a viewport position. True if it landed on one of the screen's buttons, which then acts. */
	bool WorkshopPress(const FVector2D& At);
	bool WorkshopOver(const FVector2D& At) const;

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
	void DrawWorkshop(AGGWorkshop* Workshop);
	void DrawTool(int32 Tool, const FVector2D& Center, float Size);
	/** A line on the ground, in world centimetres. */
	void GroundLine(const FVector2D& A, const FVector2D& B, const FLinearColor& Color, float Thickness, float Height = 2.f);
	void GroundRing(const FVector2D& Center, double Radius, const FLinearColor& Color, float Thickness);
	void GroundBox(const FIntRect& Rect, const FLinearColor& Color, float Thickness, float Height = 2.f);
	/** Crisp text at any size. Size is the height of a capital letter in pixels, roughly. */
	void Label(const FString& Line, float X, float Y, float Size, const FLinearColor& Color, bool bCentered = false, bool bBold = false,
		bool bRight = false);
	static bool Adjustable(const FItem& Item);
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
	/** The host address being typed on the Play together screen. */
	FString Address;
	/** The building screen's buttons, where they were last drawn. */
	TArray<FItem> ShopButtons;
};
