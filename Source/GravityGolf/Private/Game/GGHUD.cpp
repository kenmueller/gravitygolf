#include "Game/GGHUD.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Fonts/SlateFontInfo.h"
#include "Game/GGCourse.h"
#include "Game/GGGameMode.h"
#include "Game/GGGolfer.h"
#include "Game/GGSave.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
const FLinearColor Ink(0.96f, 0.95f, 0.91f);
const FLinearColor Dim(0.96f, 0.95f, 0.91f, 0.68f);
const FLinearColor Faint(0.96f, 0.95f, 0.91f, 0.4f);
const FLinearColor Gold(1.f, 0.78f, 0.36f);
} // namespace

float AGGHUD::Unit() const
{
	return Canvas ? Canvas->ClipY / 1080.f : 1.f;
}

void AGGHUD::Label(const FString& Line, float X, float Y, float Size, const FLinearColor& Color, bool bCentered, bool bBold)
{
	if (!TextFont)
	{
		TextFont = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
	}
	if (!TextFont || !Canvas)
	{
		return;
	}
	// Slate sizes fonts in points; three quarters of a pixel height gives capitals about that tall.
	const FSlateFontInfo Info(TextFont, FMath::Max(6.f, Size * 0.75f), bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular")));
	FCanvasTextItem Item(FVector2D(X, Y), FText::FromString(Line), Info, Color);
	// A soft dark shadow, so every line reads over bright rock, sky or salt.
	Item.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.7f * Color.A), FVector2D(1.5f, 1.5f) * FMath::Max(Unit(), 0.75f));
	Item.bCentreX = bCentered;
	Item.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Item);
}

void AGGHUD::Ring(const FVector2D& Center, float Radius, const FLinearColor& Color, float Thickness)
{
	constexpr int32 Sides = 48;
	FVector2D Previous = Center + FVector2D(Radius, 0.f);
	for (int32 I = 1; I <= Sides; ++I)
	{
		const float Angle = 2.f * PI * I / Sides;
		const FVector2D Next = Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius;
		Canvas->K2_DrawLine(Previous, Next, Thickness, Color);
		Previous = Next;
	}
}

void AGGHUD::Disc(const FVector2D& Center, float Radius, const FLinearColor& Color)
{
	FCanvasNGonItem Item(Center, FVector2D(Radius, Radius), 40, Color);
	Item.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Item);
}

void AGGHUD::DrawHUD()
{
	Super::DrawHUD();

	const AGGGolfer* Golfer = Cast<AGGGolfer>(GetOwningPawn());
	const AGGCourse* Course = Golfer ? Golfer->Course() : nullptr;
	if (!Course || !Canvas || FParse::Param(FCommandLine::Get(), TEXT("GGNoHUD")))
	{
		return;
	}
	if (MenuOpen())
	{
		// The mouse highlights what it is over, but only when it moves, so it doesn't fight the arrow keys.
		if (const APlayerController* PC = GetOwningPlayerController())
		{
			float X = 0.f;
			float Y = 0.f;
			if (PC->GetMousePosition(X, Y) && !FVector2D(X, Y).Equals(LastMouse, 0.5))
			{
				LastMouse = FVector2D(X, Y);
				MenuPointer(LastMouse, false);
			}
		}
		DrawMenu();
		return;
	}
	DrawPlay(Golfer, Course);
	if (Golfer->UsingTouch())
	{
		DrawTouch(Golfer);
	}
}

void AGGHUD::DrawPlay(const AGGGolfer* Golfer, const AGGCourse* Course)
{
	const float S = Unit();
	const float CX = Canvas->ClipX * 0.5f;
	const float CY = Canvas->ClipY * 0.5f;
	const bool bTouch = Golfer->UsingTouch();

	DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.85f), CX - 1.5f * S, CY - 1.5f * S, 3.f * S, 3.f * S);

	Label(Course->CourseName().ToUpper(), 40.f * S, 30.f * S, 26.f * S, Ink, false, true);
	Label(FString::Printf(TEXT("Swings %d"), Course->Strokes()), 40.f * S, 68.f * S, 19.f * S, Dim);

	if (Course->Phase() == EGGPhase::Sunk)
	{
		// The solved card.
		const UGGSave& Save = UGGSave::Get();
		const int32 Perfect = Course->PerfectSwings();
		const int32 Best = Save.Best(Course->CourseId());
		const bool bPerfect = Course->Strokes() <= Perfect && !Course->Assisted();
		const float W = 640.f * S;
		const float H = 250.f * S;
		const float Top = Canvas->ClipY * 0.2f;
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.45f), CX - W * 0.5f, Top, W, H);
		DrawRect(bPerfect ? Gold : Ink, CX - W * 0.5f, Top, W, 3.f * S);
		Label(bPerfect ? TEXT("PERFECT") : TEXT("SOLVED"), CX, Top + 28.f * S, 54.f * S, bPerfect ? Gold : Ink, true, true);
		Label(FString::Printf(TEXT("%d swing%s    perfect is %d"), Course->Strokes(), Course->Strokes() == 1 ? TEXT("") : TEXT("s"), Perfect),
			CX, Top + 112.f * S, 24.f * S, Ink, true);
		if (Course->Assisted())
		{
			Label(TEXT("The solution was shown, so this one isn't recorded"), CX, Top + 152.f * S, 20.f * S, Dim, true);
		}
		else if (Best > 0)
		{
			Label(FString::Printf(TEXT("Your best: %d"), Best), CX, Top + 152.f * S, 20.f * S, Dim, true);
		}
		if (!bTouch)
		{
			Label(Course->HasNext() ? TEXT("Click for the next hole") : TEXT("Click to play it again"), CX, Top + 198.f * S, 20.f * S, Dim, true);
		}
		return;
	}

	if (!Course->StatusText().IsEmpty())
	{
		Label(Course->StatusText(), CX, Canvas->ClipY * 0.2f, 40.f * S, Ink, true, true);
	}

	if (Golfer->IsCharging())
	{
		const float W = 320.f * S;
		const float Y = Canvas->ClipY * 0.8f;
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.45f), CX - W * 0.5f, Y, W, 10.f * S);
		DrawRect(FLinearColor::LerpUsingHSV(FLinearColor(0.5f, 1.f, 0.6f), FLinearColor(1.f, 0.35f, 0.2f), Golfer->Charge()),
			CX - W * 0.5f, Y, W * Golfer->Charge(), 10.f * S);
		return;
	}

	FString Prompt;
	if (bTouch)
	{
		// The buttons say what can be done; the only words needed are how far away the ball is.
		if (Course->Phase() == EGGPhase::Ready && !Golfer->CanSwing() && !Golfer->IsCarrying())
		{
			Prompt = FString::Printf(TEXT("Ball: %d m"), FMath::RoundToInt32(Golfer->DistanceToBall() / 100.0));
		}
	}
	else if (Golfer->IsCarrying())
	{
		Prompt = TEXT("E   set the machine down");
	}
	else if (Golfer->CanPickUp())
	{
		Prompt = TEXT("E   pick up the gravity machine");
	}
	else if (Golfer->CanSwing())
	{
		Prompt = TEXT("Hold click to swing        Right click to watch the ball");
	}
	else if (Course->Phase() == EGGPhase::Ready)
	{
		Prompt = FString::Printf(TEXT("Ball %d m        B  go to it        R  return ball        Tab  menu"),
			FMath::RoundToInt32(Golfer->DistanceToBall() / 100.0));
	}
	if (!Prompt.IsEmpty())
	{
		Label(Prompt, CX, Canvas->ClipY * 0.87f, 20.f * S, Dim, true);
	}
}

void AGGHUD::DrawTouch(const AGGGolfer* Golfer)
{
	const float S = Unit();
	const FVector2D View(Canvas->ClipX, Canvas->ClipY);
	TArray<FGGTouchButton> Buttons;
	Golfer->TouchButtons(View, Buttons);
	for (const FGGTouchButton& Button : Buttons)
	{
		Disc(Button.Center, Button.Radius, FLinearColor(0.02f, 0.03f, 0.05f, Button.bDown ? 0.6f : 0.32f));
		Ring(Button.Center, Button.Radius, FLinearColor(1.f, 1.f, 1.f, Button.bDown ? 0.95f : 0.6f), 2.5f * S);
		const float Size = FMath::Clamp(Button.Radius * 0.36f, 13.f * S, 30.f * S);
		Label(Button.Label, Button.Center.X, Button.Center.Y - Size * 0.62f, Size, Ink, true, true);
	}
	FVector2D Origin;
	FVector2D Knob;
	const float Reach = View.Y * 0.09f;
	if (Golfer->TouchStick(Origin, Knob))
	{
		FVector2D Offset = Knob - Origin;
		if (Offset.Size() > Reach)
		{
			Offset = Offset.GetSafeNormal() * Reach;
		}
		Ring(Origin, Reach, FLinearColor(1.f, 1.f, 1.f, 0.5f), 2.5f * S);
		Disc(Origin + Offset, Reach * 0.42f, FLinearColor(1.f, 1.f, 1.f, 0.5f));
	}
	else
	{
		// A faint ring where the stick usually sits, so a new player knows the left thumb goes there.
		Ring(FVector2D(View.Y * 0.2f, View.Y * 0.78f), Reach, FLinearColor(1.f, 1.f, 1.f, 0.22f), 2.f * S);
	}
}

// ---------------------------------------------------------------------------------------------------- menus

void AGGHUD::OpenMenu(EGGMenu Which)
{
	if (Which == EGGMenu::None)
	{
		CloseMenu();
		return;
	}
	if (Which == EGGMenu::Holes || Which == EGGMenu::Settings)
	{
		Parent = Menu == EGGMenu::Title ? EGGMenu::Title : EGGMenu::Pause;
	}
	Menu = Which;
	Selected = 0;
	BuildMenu();
	if (APlayerController* PC = GetOwningPlayerController())
	{
		PC->bShowMouseCursor = true;
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
	}
}

void AGGHUD::CloseMenu()
{
	Menu = EGGMenu::None;
	Items.Reset();
	if (APlayerController* PC = GetOwningPlayerController())
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void AGGHUD::BuildMenu()
{
	const UGGSave& Save = UGGSave::Get();
	const int32 Keep = Selected;
	Items.Reset();
	auto Add = [&](const TCHAR* Text, FName Action, const FString& Value = FString(), int32 Number = 0, bool bEnabled = true) {
		FItem Item;
		Item.Label = Text;
		Item.Value = Value;
		Item.Action = Action;
		Item.Number = Number;
		Item.bEnabled = bEnabled;
		Items.Add(Item);
	};
	switch (Menu)
	{
	case EGGMenu::Title:
		Add(Save.BestSwings.Num() > 0 ? TEXT("Continue") : TEXT("Play"), TEXT("Play"));
		Add(TEXT("Holes"), TEXT("Holes"));
		Add(TEXT("Settings"), TEXT("Settings"));
#if !PLATFORM_WASM
		Add(TEXT("Quit"), TEXT("Quit"));
#endif
		break;
	case EGGMenu::Pause:
		Add(TEXT("Resume"), TEXT("Resume"));
		Add(TEXT("Start this hole again"), TEXT("Restart"));
		Add(TEXT("Show the solution"), TEXT("Hint"));
		Add(TEXT("Holes"), TEXT("Holes"));
		Add(TEXT("Settings"), TEXT("Settings"));
		Add(TEXT("Title screen"), TEXT("Title"));
		break;
	case EGGMenu::Holes:
	{
		const TArray<FGGHoleInfo>& All = UGGSave::Holes();
		for (int32 I = 0; I < All.Num(); ++I)
		{
			const int32 Best = Save.Best(All[I].Course);
			const bool bOpen = Save.IsUnlocked(I);
			const FString Value = !bOpen ? FString(TEXT("Locked"))
				: Best == 0 ? FString(TEXT("Not solved"))
				: Best <= All[I].Perfect ? FString::Printf(TEXT("Perfect, %d"), Best)
				: FString::Printf(TEXT("Best %d, perfect is %d"), Best, All[I].Perfect);
			Add(*FString::Printf(TEXT("%d   %s"), I + 1, All[I].Name), TEXT("Hole"), Value, I, bOpen);
		}
		Add(TEXT("Back"), TEXT("Back"));
		break;
	}
	case EGGMenu::Settings:
	{
		static const TCHAR* Qualities[] = {TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Highest")};
		Add(TEXT("Look speed"), TEXT("Sensitivity"), FString::Printf(TEXT("%d%%"), FMath::RoundToInt32(Save.LookSensitivity * 100.f)));
		Add(TEXT("Look up and down"), TEXT("Invert"), Save.bInvertLook ? TEXT("Inverted") : TEXT("Normal"));
		Add(TEXT("Sound"), TEXT("Volume"), FString::Printf(TEXT("%d%%"), FMath::RoundToInt32(Save.Volume * 100.f)));
		Add(TEXT("Picture quality"), TEXT("Quality"), Qualities[FMath::Clamp(Save.Quality, 0, 3)]);
		Add(TEXT("Back"), TEXT("Back"));
		break;
	}
	default:
		break;
	}
	Selected = Items.IsValidIndex(Keep) ? Keep : 0;
}

void AGGHUD::MenuMove(int32 Rows, int32 Columns)
{
	if (!MenuOpen() || Items.Num() == 0)
	{
		return;
	}
	if (Rows != 0)
	{
		// Step over rows that can't be chosen (locked holes).
		for (int32 Try = 0; Try < Items.Num(); ++Try)
		{
			Selected = (Selected + Rows + Items.Num()) % Items.Num();
			if (Items[Selected].bEnabled)
			{
				break;
			}
		}
	}
	if (Columns != 0 && Items.IsValidIndex(Selected) && !Items[Selected].Value.IsEmpty() && Items[Selected].Action != TEXT("Hole"))
	{
		RunAction(Items[Selected], Columns);
	}
}

void AGGHUD::MenuActivate()
{
	if (MenuOpen() && Items.IsValidIndex(Selected) && Items[Selected].bEnabled)
	{
		const FItem Item = Items[Selected];
		RunAction(Item, 0);
	}
}

void AGGHUD::MenuBack()
{
	if (Menu == EGGMenu::Holes || Menu == EGGMenu::Settings)
	{
		Menu = Parent;
		Selected = 0;
		BuildMenu();
	}
	else if (Menu == EGGMenu::Pause)
	{
		CloseMenu();
	}
}

void AGGHUD::MenuPointer(const FVector2D& At, bool bPress)
{
	if (!MenuOpen())
	{
		return;
	}
	for (int32 I = 0; I < Items.Num(); ++I)
	{
		if (Items[I].bEnabled && Items[I].Rect.bIsValid && Items[I].Rect.IsInside(At))
		{
			Selected = I;
			if (bPress)
			{
				const FItem Item = Items[I];
				// On a value row, the left half steps down and the right half steps up.
				const bool bValue = !Item.Value.IsEmpty() && Item.Action != TEXT("Hole");
				RunAction(Item, bValue ? (At.X < Item.Rect.GetCenter().X ? -1 : 1) : 0);
			}
			return;
		}
	}
}

void AGGHUD::OpenHole(int32 Index)
{
	const TArray<FGGHoleInfo>& All = UGGSave::Holes();
	if (All.IsValidIndex(Index))
	{
		CloseMenu();
		UGameplayStatics::OpenLevel(this, FName(*(FString(TEXT("/Game/Levels/")) + All[Index].Level)));
	}
}

void AGGHUD::RunAction(const FItem& Item, int32 Direction)
{
	UGGSave& Save = UGGSave::Get();
	AGGGolfer* Golfer = Cast<AGGGolfer>(GetOwningPawn());
	const AGGCourse* Course = Golfer ? Golfer->Course() : nullptr;
	const FName Action = Item.Action;
	const int32 Step = Direction == 0 ? 1 : Direction;
	if (Action == TEXT("Play"))
	{
		const int32 Next = Save.NextUnsolved();
		if (Course && UGGSave::HoleIndex(Course->CourseId()) == Next)
		{
			CloseMenu();
		}
		else
		{
			OpenHole(Next);
		}
	}
	else if (Action == TEXT("Resume"))
	{
		CloseMenu();
	}
	else if (Action == TEXT("Restart"))
	{
		CloseMenu();
		if (Golfer)
		{
			Golfer->ResetHole();
		}
	}
	else if (Action == TEXT("Hint"))
	{
		CloseMenu();
		if (Golfer)
		{
			Golfer->ShowSolution();
		}
	}
	else if (Action == TEXT("Holes"))
	{
		OpenMenu(EGGMenu::Holes);
	}
	else if (Action == TEXT("Settings"))
	{
		OpenMenu(EGGMenu::Settings);
	}
	else if (Action == TEXT("Title"))
	{
		OpenMenu(EGGMenu::Title);
	}
	else if (Action == TEXT("Back"))
	{
		MenuBack();
	}
	else if (Action == TEXT("Hole"))
	{
		OpenHole(Item.Number);
	}
	else if (Action == TEXT("Quit"))
	{
		if (APlayerController* PC = GetOwningPlayerController())
		{
			PC->ConsoleCommand(TEXT("quit"));
		}
	}
	else
	{
		if (Action == TEXT("Sensitivity"))
		{
			Save.LookSensitivity = FMath::Clamp(FMath::RoundToFloat((Save.LookSensitivity + 0.1f * Step) * 10.f) / 10.f, 0.3f, 2.5f);
		}
		else if (Action == TEXT("Invert"))
		{
			Save.bInvertLook = !Save.bInvertLook;
		}
		else if (Action == TEXT("Volume"))
		{
			Save.Volume = FMath::Clamp(FMath::RoundToFloat((Save.Volume + 0.1f * Step) * 10.f) / 10.f, 0.f, 1.f);
		}
		else if (Action == TEXT("Quality"))
		{
			Save.Quality = (Save.Quality + Step + 4) % 4;
			Save.ApplyQuality();
		}
		UGGSave::Store();
		BuildMenu();
	}
}

void AGGHUD::DrawMenu()
{
	const float S = Unit();
	const float CX = Canvas->ClipX * 0.5f;
	const AGGGolfer* Golfer = Cast<AGGGolfer>(GetOwningPawn());
	const bool bTouch = Golfer && Golfer->UsingTouch();

	// The world stays visible behind, dimmed. The title screen dims less: the place is the backdrop.
	DrawRect(FLinearColor(0.f, 0.f, 0.f, Menu == EGGMenu::Title ? 0.42f : 0.62f), 0.f, 0.f, Canvas->ClipX, Canvas->ClipY);

	float Top = Canvas->ClipY * 0.3f;
	if (Menu == EGGMenu::Title)
	{
		Label(TEXT("GRAVITY GOLF"), CX, Canvas->ClipY * 0.14f, 92.f * S, Ink, true, true);
		Label(TEXT("Think first. Swing once."), CX, Canvas->ClipY * 0.14f + 120.f * S, 26.f * S, Dim, true);
		Top = Canvas->ClipY * 0.42f;
	}
	else
	{
		const TCHAR* Heading = Menu == EGGMenu::Pause ? TEXT("PAUSED") : Menu == EGGMenu::Holes ? TEXT("HOLES") : TEXT("SETTINGS");
		Label(Heading, CX, Canvas->ClipY * 0.13f, 54.f * S, Ink, true, true);
		Top = Canvas->ClipY * 0.27f;
	}

	const float Row = (bTouch ? 84.f : 66.f) * S;
	const float Width = FMath::Min(760.f * S, Canvas->ClipX * 0.92f);
	const float Left = CX - Width * 0.5f;
	for (int32 I = 0; I < Items.Num(); ++I)
	{
		FItem& Item = Items[I];
		const float Y = Top + Row * I;
		Item.Rect = FBox2D(FVector2D(Left, Y), FVector2D(Left + Width, Y + Row - 6.f * S));
		const bool bChosen = I == Selected && Item.bEnabled;
		if (bChosen)
		{
			DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.13f), Left, Y, Width, Row - 6.f * S);
			DrawRect(Gold, Left, Y, 4.f * S, Row - 6.f * S);
		}
		const FLinearColor Color = !Item.bEnabled ? Faint : bChosen ? Ink : Dim;
		const float TextY = Y + (Row - 6.f * S) * 0.5f - 21.f * S;
		if (Item.Value.IsEmpty())
		{
			Label(Item.Label, CX, TextY, 30.f * S, Color, true, bChosen);
		}
		else
		{
			Label(Item.Label, Left + 28.f * S, TextY, 30.f * S, Color, false, bChosen);
			// Values sit in a right-hand column; an arrow each side says they can be changed.
			const bool bChange = Item.Action != TEXT("Hole");
			const FString Shown = bChange && bChosen ? FString::Printf(TEXT("<   %s   >"), *Item.Value) : Item.Value;
			Label(Shown, Left + Width * 0.74f, TextY + 3.f * S, 24.f * S, bChosen ? Gold : Color, true);
		}
	}

	if (!bTouch)
	{
		const TCHAR* Hint = Menu == EGGMenu::Settings ? TEXT("Arrows or mouse to choose        Left and right to change        Tab to go back")
			: Menu == EGGMenu::Title ? TEXT("Arrows or mouse to choose        Enter to select")
			: TEXT("Arrows or mouse to choose        Enter to select        Tab to go back");
		Label(Hint, CX, Canvas->ClipY * 0.93f, 17.f * S, Faint, true);
	}
}
