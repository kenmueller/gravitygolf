#include "Game/GGHUD.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Fonts/SlateFontInfo.h"
#include "Game/GGCourse.h"
#include "Game/GGGameMode.h"
#include "EngineUtils.h"
#include "Game/GGGolfer.h"
#include "Game/GGRival.h"
#include "Game/GGSave.h"
#include "Game/GGWorkshop.h"
#include "HAL/PlatformApplicationMisc.h"
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

void AGGHUD::Label(const FString& Line, float X, float Y, float Size, const FLinearColor& Color, bool bCentered, bool bBold, bool bRight)
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
	if (bRight)
	{
		// Right-aligned: measure the line and start it that far to the left.
		// (Drawing a text item is what measures it, so an invisible copy is drawn first.)
		FCanvasTextItem Measure(FVector2D(-4000.0, -4000.0), FText::FromString(Line), Info, FLinearColor(0.f, 0.f, 0.f, 0.f));
		Measure.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Measure);
		X -= static_cast<float>(Measure.DrawnSize.X);
	}
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
	AGGWorkshop* Workshop = AGGWorkshop::Get(this);
	if (Workshop && Workshop->Building())
	{
		DrawWorkshop(Workshop);
		return;
	}
	DrawPlay(Golfer, Course);
	if (Workshop && !Golfer->UsingTouch())
	{
		Label(TEXT("T   back to building"), Canvas->ClipX - 40.f * Unit(), 34.f * Unit(), 19.f * Unit(), Dim, false, false, true);
	}
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
	// A rival, when one is playing: what it is up to, under the swing count.
	if (TActorIterator<AGGRival> Rival(GetWorld()); Rival)
	{
		Label(Rival->StatusLine(), 40.f * S, 100.f * S, 19.f * S, Rival->Solved() ? FLinearColor(1.f, 0.45f, 0.65f) : Dim);
	}

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
		if (const AGGWorkshop* Shop = AGGWorkshop::Get(this))
		{
			// A hole of the player's own: solving it is half of proving it.
			Label(Shop->Design().Solved() ? TEXT("Your answer is kept as this hole's solution") : TEXT("Solve every swing in one go to keep it as the answer"),
				CX, Top + 152.f * S, 20.f * S, Dim, true);
			Label(bTouch ? TEXT("BUILD takes you back to the table") : TEXT("T  back to building        Backspace  play it again"), CX, Top + 198.f * S, 20.f * S, Dim, true);
			return;
		}
		if (Course->Assisted())
		{
			Label(TEXT("The solution was shown, so this one isn't recorded"), CX, Top + 152.f * S, 20.f * S, Dim, true);
		}
		else if (Best > 0)
		{
			Label(FString::Printf(TEXT("Your best: %d"), Best), CX, Top + 152.f * S, 20.f * S, Dim, true);
		}
		const bool bAlone = GetNetMode() == NM_Standalone;
		if (!bTouch)
		{
			Label(Course->HasNext() ? TEXT("Click for the next hole") : bAlone ? TEXT("Click to go on to the Workshop") : TEXT("Click to play it again"),
				CX, Top + 198.f * S, 20.f * S, Dim, true);
		}
		if (!Course->HasNext())
		{
			// The last hole: the course is finished. Say so, with the player's record, and point at what comes next.
			const TArray<FGGHoleInfo>& All = UGGSave::Holes();
			int32 Solved = 0;
			int32 Flawless = 0;
			for (const FGGHoleInfo& Hole : All)
			{
				const int32 Fewest = Save.Best(Hole.Course);
				Solved += Fewest > 0 ? 1 : 0;
				Flawless += Fewest > 0 && Fewest <= Hole.Perfect ? 1 : 0;
			}
			const float EndTop = Top + H + 26.f * S;
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.45f), CX - W * 0.5f, EndTop, W, 150.f * S);
			DrawRect(Gold, CX - W * 0.5f, EndTop, W, 3.f * S);
			Label(TEXT("THE COURSE IS COMPLETE"), CX, EndTop + 20.f * S, 30.f * S, Gold, true, true);
			Label(FString::Printf(TEXT("%d of %d holes solved        %d in a perfect round"), Solved, All.Num(), Flawless), CX, EndTop + 68.f * S, 20.f * S, Ink, true);
			Label(TEXT("Every hole was made with the tools in the Workshop. Build one that stumps a friend."), CX, EndTop + 104.f * S, 18.f * S, Dim, true);
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
	const bool bWorkshop = AGGWorkshop::Get(this) != nullptr;
	if (Which == EGGMenu::Pause && bWorkshop)
	{
		Which = EGGMenu::Workshop;
	}
	if (Which == EGGMenu::Holes || Which == EGGMenu::Settings || Which == EGGMenu::Together || Which == EGGMenu::Designs)
	{
		Parent = Menu == EGGMenu::Title ? EGGMenu::Title : bWorkshop ? EGGMenu::Workshop : EGGMenu::Pause;
	}
	if (Which == EGGMenu::Together && Address.IsEmpty())
	{
		Address = UGGSave::Get().LastHost;
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
		// Building in the Workshop is done with the pointer, so it stays.
		const AGGWorkshop* Workshop = AGGWorkshop::Get(this);
		if (Workshop && Workshop->Building())
		{
			return;
		}
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
		Add(TEXT("Workshop"), TEXT("Workshop"));
#if !PLATFORM_WASM
		if (!AGGWorkshop::Get(this))
		{
			Add(TEXT("Play together"), TEXT("Together"));
		}
#endif
		Add(TEXT("Settings"), TEXT("Settings"));
#if !PLATFORM_WASM
		Add(TEXT("Quit"), TEXT("Quit"));
#endif
		break;
	case EGGMenu::Workshop:
	{
		const AGGWorkshop* Workshop = AGGWorkshop::Get(this);
		const bool bBuilding = Workshop && Workshop->Building();
		Add(bBuilding ? TEXT("Back to building") : TEXT("Back to the hole"), TEXT("Resume"));
		Add(bBuilding ? TEXT("Test this hole") : TEXT("Back to building"), TEXT("ToggleBuild"));
		Add(TEXT("Name"), TEXT("Name"), Workshop ? Workshop->Design().Name + (bBuilding ? TEXT("_") : TEXT("")) : FString());
		Add(TEXT("Keep this hole"), TEXT("Keep"));
		Add(TEXT("My holes"), TEXT("Designs"), FString::Printf(TEXT("%d kept"), Save.Designs.Num()));
		Add(TEXT("Start a new hole"), TEXT("NewHole"));
#if !PLATFORM_WASM
		Add(TEXT("Copy this hole's code"), TEXT("CopyCode"));
		Add(TEXT("Open a hole from a copied code"), TEXT("PasteCode"));
		Add(TEXT("Play together"), TEXT("Together"));
#endif
		Add(TEXT("Settings"), TEXT("Settings"));
		Add(TEXT("Leave the Workshop"), TEXT("Title"));
		break;
	}
	case EGGMenu::Designs:
	{
		TArray<FString> Names;
		Save.Designs.GetKeys(Names);
		Names.Sort();
		for (int32 I = 0; I < Names.Num(); ++I)
		{
			Add(*Names[I], TEXT("OpenDesign"), FString(TEXT("open")), I);
		}
		if (Names.Num() == 0)
		{
			Add(TEXT("Nothing kept yet"), TEXT("None"), FString(), 0, false);
		}
		const AGGWorkshop* Workshop = AGGWorkshop::Get(this);
		if (Workshop && Save.Designs.Contains(Workshop->Design().Name))
		{
			Add(*FString::Printf(TEXT("Delete \"%s\""), *Workshop->Design().Name), TEXT("DeleteDesign"));
		}
		Add(TEXT("Back"), TEXT("Back"));
		break;
	}
	case EGGMenu::Pause:
		Add(TEXT("Resume"), TEXT("Resume"));
		Add(TEXT("Start this hole again"), TEXT("Restart"));
		Add(TEXT("Show the solution"), TEXT("Hint"));
		Add(TEXT("Holes"), TEXT("Holes"));
		Add(TEXT("Workshop"), TEXT("Workshop"));
#if !PLATFORM_WASM
		Add(TEXT("Play together"), TEXT("Together"));
#endif
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
	case EGGMenu::Together:
	{
		const ENetMode Mode = GetNetMode();
		if (Mode == NM_Standalone)
		{
			Add(TEXT("Host this hole"), TEXT("Host"));
			Add(TEXT("Join a host at"), TEXT("Join"), Address.IsEmpty() ? FString(TEXT("type an address")) : Address);
		}
		else
		{
			Add(Mode == NM_Client ? TEXT("You have joined a host") : TEXT("You are hosting"), TEXT("None"), FString(), 0, false);
			Add(TEXT("Leave and play alone"), TEXT("Leave"));
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
		static const TCHAR* Rivals[] = {TEXT("None"), TEXT("Easy"), TEXT("Medium"), TEXT("Hard")};
		Add(TEXT("Rival to race"), TEXT("Rival"), Rivals[FMath::Clamp(Save.Rival, 0, 3)]);
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
	if (Columns != 0 && Items.IsValidIndex(Selected) && Adjustable(Items[Selected]))
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

bool AGGHUD::Adjustable(const FItem& Item)
{
	// Rows whose value steps left and right. The others only show a value beside their name.
	static const FName Steps[] = {TEXT("Sensitivity"), TEXT("Invert"), TEXT("Volume"), TEXT("Quality"), TEXT("Rival")};
	return !Item.Value.IsEmpty() && Algo::Find(Steps, Item.Action) != nullptr;
}

bool AGGHUD::MenuType(const FKey& Key)
{
	if (Menu == EGGMenu::Workshop)
	{
		// In the Workshop's menu, typing names the hole.
		AGGWorkshop* Workshop = AGGWorkshop::Get(this);
		const APlayerController* PC = GetOwningPlayerController();
		if (!Workshop || !PC)
		{
			return false;
		}
		FString Name = Workshop->Design().Name;
		const FString KeyName = Key.GetFName().ToString();
		static const TPair<FKey, TCHAR> Digits[] = {
			{EKeys::Zero, TEXT('0')}, {EKeys::One, TEXT('1')}, {EKeys::Two, TEXT('2')}, {EKeys::Three, TEXT('3')}, {EKeys::Four, TEXT('4')},
			{EKeys::Five, TEXT('5')}, {EKeys::Six, TEXT('6')}, {EKeys::Seven, TEXT('7')}, {EKeys::Eight, TEXT('8')}, {EKeys::Nine, TEXT('9')},
			{EKeys::SpaceBar, TEXT(' ')}, {EKeys::Hyphen, TEXT('-')},
		};
		TCHAR Typed = 0;
		for (const TPair<FKey, TCHAR>& Pair : Digits)
		{
			if (Key == Pair.Key)
			{
				Typed = Pair.Value;
			}
		}
		if (!Typed && KeyName.Len() == 1 && FChar::IsAlpha(KeyName[0]))
		{
			// Capitals with Shift, and to start the name.
			const bool bUpper = PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift) || Name.IsEmpty();
			Typed = bUpper ? FChar::ToUpper(KeyName[0]) : FChar::ToLower(KeyName[0]);
		}
		if (Key == EKeys::BackSpace)
		{
			Name.LeftChopInline(1);
		}
		else if (Typed && Name.Len() < 28)
		{
			Name.AppendChar(Typed);
		}
		else
		{
			return false;
		}
		Workshop->NameHole(Name);
		BuildMenu();
		for (int32 I = 0; I < Items.Num(); ++I)
		{
			if (Items[I].Action == TEXT("Name"))
			{
				Selected = I;
			}
		}
		return true;
	}
	// Otherwise typing only happens on the Play together screen, into the address.
	if (Menu != EGGMenu::Together || GetNetMode() != NM_Standalone)
	{
		return false;
	}
	static const TPair<FKey, TCHAR> Keys[] = {
		{EKeys::Zero, TEXT('0')}, {EKeys::One, TEXT('1')}, {EKeys::Two, TEXT('2')}, {EKeys::Three, TEXT('3')}, {EKeys::Four, TEXT('4')},
		{EKeys::Five, TEXT('5')}, {EKeys::Six, TEXT('6')}, {EKeys::Seven, TEXT('7')}, {EKeys::Eight, TEXT('8')}, {EKeys::Nine, TEXT('9')},
		{EKeys::NumPadZero, TEXT('0')}, {EKeys::NumPadOne, TEXT('1')}, {EKeys::NumPadTwo, TEXT('2')}, {EKeys::NumPadThree, TEXT('3')},
		{EKeys::NumPadFour, TEXT('4')}, {EKeys::NumPadFive, TEXT('5')}, {EKeys::NumPadSix, TEXT('6')}, {EKeys::NumPadSeven, TEXT('7')},
		{EKeys::NumPadEight, TEXT('8')}, {EKeys::NumPadNine, TEXT('9')}, {EKeys::Period, TEXT('.')}, {EKeys::Decimal, TEXT('.')},
		{EKeys::Hyphen, TEXT('-')}, {EKeys::Semicolon, TEXT(':')}, {EKeys::Colon, TEXT(':')},
	};
	bool bUsed = false;
	if (Key == EKeys::BackSpace)
	{
		Address.LeftChopInline(1);
		bUsed = true;
	}
	for (const TPair<FKey, TCHAR>& Pair : Keys)
	{
		if (Key == Pair.Key && Address.Len() < 40)
		{
			Address.AppendChar(Pair.Value);
			bUsed = true;
		}
	}
	// Letters, for host names.
	const FString Name = Key.GetFName().ToString();
	if (!bUsed && Name.Len() == 1 && FChar::IsAlpha(Name[0]) && Address.Len() < 40)
	{
		Address.AppendChar(FChar::ToLower(Name[0]));
		bUsed = true;
	}
	if (bUsed)
	{
		// Typing selects the address row.
		BuildMenu();
		for (int32 I = 0; I < Items.Num(); ++I)
		{
			if (Items[I].Action == TEXT("Join"))
			{
				Selected = I;
			}
		}
	}
	return bUsed;
}

void AGGHUD::MenuBack()
{
	if (Menu == EGGMenu::Holes || Menu == EGGMenu::Settings || Menu == EGGMenu::Together || Menu == EGGMenu::Designs)
	{
		Menu = Parent;
		Selected = 0;
		BuildMenu();
	}
	else if (Menu == EGGMenu::Pause || Menu == EGGMenu::Workshop)
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
				RunAction(Item, Adjustable(Item) ? (At.X < Item.Rect.GetCenter().X ? -1 : 1) : 0);
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
			// Already there: put the player back behind the ball (the title screen let the view drift).
			CloseMenu();
			Golfer->GoToBall();
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
	else if (Action == TEXT("Together"))
	{
		OpenMenu(EGGMenu::Together);
	}
	else if (Action == TEXT("Host"))
	{
		// Reopen this hole as a host: friends join with this machine's address.
		CloseMenu();
		UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)), true, TEXT("listen"));
	}
	else if (Action == TEXT("Join"))
	{
		if (!Address.IsEmpty())
		{
			Save.LastHost = Address;
			UGGSave::Store();
			CloseMenu();
			if (APlayerController* PC = GetOwningPlayerController())
			{
				PC->ClientTravel(Address, TRAVEL_Absolute);
			}
		}
	}
	else if (Action == TEXT("Leave"))
	{
		CloseMenu();
		UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
	}
	else if (Action == TEXT("Title"))
	{
		OpenMenu(EGGMenu::Title);
	}
	else if (Action == TEXT("Workshop"))
	{
		CloseMenu();
		UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/Levels/Workshop")));
	}
	else if (Action == TEXT("Designs"))
	{
		OpenMenu(EGGMenu::Designs);
	}
	else if (AGGWorkshop* Workshop = AGGWorkshop::Get(this); Workshop && (Action == TEXT("ToggleBuild") || Action == TEXT("Keep") || Action == TEXT("NewHole")
		|| Action == TEXT("CopyCode") || Action == TEXT("PasteCode") || Action == TEXT("OpenDesign") || Action == TEXT("DeleteDesign") || Action == TEXT("Name")))
	{
		if (Action == TEXT("Name"))
		{
			return;
		}
		if (Action == TEXT("ToggleBuild"))
		{
			Menu = EGGMenu::None;
			Items.Reset();
			Workshop->Toggle();
			CloseMenu();
			return;
		}
		if (Action == TEXT("Keep"))
		{
			Workshop->SaveHole();
		}
		else if (Action == TEXT("NewHole"))
		{
			Workshop->NewHole();
		}
		else if (Action == TEXT("CopyCode"))
		{
			FPlatformApplicationMisc::ClipboardCopy(*Workshop->ShareCode());
			Workshop->Say(TEXT("This hole's code is copied. Paste it to a friend."));
		}
		else if (Action == TEXT("PasteCode"))
		{
			FString Code;
			FPlatformApplicationMisc::ClipboardPaste(Code);
			Workshop->ImportCode(Code);
		}
		else if (Action == TEXT("OpenDesign"))
		{
			Workshop->LoadHole(Item.Label);
		}
		else if (Action == TEXT("DeleteDesign"))
		{
			Workshop->DeleteHole(Workshop->Design().Name);
			BuildMenu();
			return;
		}
		Menu = EGGMenu::None;
		Items.Reset();
		if (!Workshop->Building())
		{
			Workshop->StartBuilding();
		}
		CloseMenu();
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
			Save.bQualityChosen = true;
			Save.ApplyQuality();
		}
		else if (Action == TEXT("Rival"))
		{
			// Takes effect on the next hole, or when this one is started again from the hole list.
			Save.Rival = (Save.Rival + Step + 4) % 4;
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
		const TCHAR* Heading = Menu == EGGMenu::Pause ? TEXT("PAUSED") : Menu == EGGMenu::Holes ? TEXT("HOLES")
			: Menu == EGGMenu::Together ? TEXT("PLAY TOGETHER") : Menu == EGGMenu::Workshop ? TEXT("WORKSHOP")
			: Menu == EGGMenu::Designs ? TEXT("MY HOLES") : TEXT("SETTINGS");
		Label(Heading, CX, Canvas->ClipY * 0.13f, 54.f * S, Ink, true, true);
		Top = Canvas->ClipY * 0.27f;
	}

	// Long lists (the Workshop's menu) take smaller rows so they fit the screen.
	const float Room = Canvas->ClipY * 0.9f - Top;
	const float Row = FMath::Min((bTouch ? 84.f : 66.f) * S, Room / FMath::Max(Items.Num(), 1));
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
		const float TextSize = FMath::Min(30.f * S, Row * 0.46f);
		const float TextY = Y + (Row - 6.f * S) * 0.5f - TextSize * 0.7f;
		if (Item.Value.IsEmpty())
		{
			Label(Item.Label, CX, TextY, TextSize, Color, true, bChosen);
		}
		else
		{
			Label(Item.Label, Left + 28.f * S, TextY, TextSize, Color, false, bChosen);
			// Values sit in a right-hand column; an arrow each side says they can be changed.
			const bool bChange = Adjustable(Item);
			const FString Shown = bChange && bChosen ? FString::Printf(TEXT("<   %s   >"), *Item.Value) : Item.Value;
			Label(Shown, Left + Width * 0.74f, TextY + TextSize * 0.1f, TextSize * 0.8f, bChosen ? Gold : Color, true);
		}
	}

	if (!bTouch)
	{
		const TCHAR* Hint = Menu == EGGMenu::Workshop ? TEXT("Type to name the hole        Arrows or mouse to choose        Enter to select        Tab to go back")
			: Menu == EGGMenu::Together ? TEXT("Type the host's address, then Enter to join        A host's friends need its address and port 7777 open")
			: Menu == EGGMenu::Settings ? TEXT("Arrows or mouse to choose        Left and right to change        Tab to go back")
			: Menu == EGGMenu::Title ? TEXT("Arrows or mouse to choose        Enter to select")
			: TEXT("Arrows or mouse to choose        Enter to select        Tab to go back");
		Label(Hint, CX, Canvas->ClipY * 0.93f, 17.f * S, Faint, true);
	}
}
