// The Workshop's building screen, drawn by the HUD: the tools, the buttons, and what is marked on the ground.

#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Game/GGCourse.h"
#include "Game/GGGolfer.h"
#include "Game/GGHUD.h"
#include "Game/GGWorkshop.h"
#include "GameFramework/PlayerController.h"

namespace
{
const FLinearColor ShopInk(0.96f, 0.95f, 0.91f);
const FLinearColor ShopDim(0.96f, 0.95f, 0.91f, 0.68f);
const FLinearColor ShopFaint(0.96f, 0.95f, 0.91f, 0.4f);
const FLinearColor ShopGold(1.f, 0.78f, 0.36f);
const FLinearColor ShopGood(0.45f, 1.f, 0.62f);
const FLinearColor ShopBad(1.f, 0.42f, 0.36f);
const FLinearColor ShopPull(0.3f, 0.62f, 1.f);
const FLinearColor ShopPush(1.f, 0.55f, 0.18f);
const FLinearColor ShopPlinth(0.7f, 0.85f, 1.f);
const FLinearColor ShopPanel(0.015f, 0.02f, 0.035f, 0.72f);
} // namespace

void AGGHUD::GroundLine(const FVector2D& A, const FVector2D& B, const FLinearColor& Color, float Thickness, float Height)
{
	const FVector From = Project(FVector(A.X, A.Y, Height), false);
	const FVector To = Project(FVector(B.X, B.Y, Height), false);
	if (From.Z > 0.0 && To.Z > 0.0)
	{
		Canvas->K2_DrawLine(FVector2D(From), FVector2D(To), Thickness, Color);
	}
}

void AGGHUD::GroundRing(const FVector2D& Center, double Radius, const FLinearColor& Color, float Thickness)
{
	constexpr int32 Sides = 56;
	FVector2D Previous = Center + FVector2D(Radius, 0.0);
	for (int32 I = 1; I <= Sides; ++I)
	{
		const double Angle = 2.0 * UE_DOUBLE_PI * I / Sides;
		const FVector2D Next = Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius;
		GroundLine(Previous, Next, Color, Thickness);
		Previous = Next;
	}
}

void AGGHUD::GroundBox(const FIntRect& Rect, const FLinearColor& Color, float Thickness, float Height)
{
	const FVector2D A(Rect.Min.X, Rect.Min.Y);
	const FVector2D B(Rect.Max.X, Rect.Min.Y);
	const FVector2D C(Rect.Max.X, Rect.Max.Y);
	const FVector2D D(Rect.Min.X, Rect.Max.Y);
	GroundLine(A, B, Color, Thickness, Height);
	GroundLine(B, C, Color, Thickness, Height);
	GroundLine(C, D, Color, Thickness, Height);
	GroundLine(D, A, Color, Thickness, Height);
}

void AGGHUD::DrawTool(int32 Tool, const FVector2D& Center, float Size)
{
	// Each tool's picture is what it makes, seen from above, in the colours it has in the world.
	const float H = Size * 0.5f;
	auto Block = [&](const FLinearColor& Color, float X0, float Y0, float X1, float Y1) {
		DrawRect(Color, Center.X + X0 * H, Center.Y + Y0 * H, (X1 - X0) * H, (Y1 - Y0) * H);
	};
	auto Line = [&](float X0, float Y0, float X1, float Y1, const FLinearColor& Color, float Thickness) {
		Canvas->K2_DrawLine(Center + FVector2D(X0, Y0) * H, Center + FVector2D(X1, Y1) * H, Thickness, Color);
	};
	const float Stroke = FMath::Max(2.f, Size * 0.07f);
	switch (static_cast<EGGTool>(Tool))
	{
	case EGGTool::Move:
		Line(-0.8f, 0.f, 0.8f, 0.f, ShopInk, Stroke);
		Line(0.f, -0.8f, 0.f, 0.8f, ShopInk, Stroke);
		Line(0.8f, 0.f, 0.5f, -0.25f, ShopInk, Stroke);
		Line(0.8f, 0.f, 0.5f, 0.25f, ShopInk, Stroke);
		Line(-0.8f, 0.f, -0.5f, -0.25f, ShopInk, Stroke);
		Line(-0.8f, 0.f, -0.5f, 0.25f, ShopInk, Stroke);
		Line(0.f, -0.8f, -0.25f, -0.5f, ShopInk, Stroke);
		Line(0.f, -0.8f, 0.25f, -0.5f, ShopInk, Stroke);
		Line(0.f, 0.8f, -0.25f, 0.5f, ShopInk, Stroke);
		Line(0.f, 0.8f, 0.25f, 0.5f, ShopInk, Stroke);
		break;
	case EGGTool::Floor:
		Block(FLinearColor(0.62f, 0.66f, 0.6f), -0.8f, -0.6f, 0.8f, 0.6f);
		break;
	case EGGTool::Sand:
		Block(FLinearColor(0.85f, 0.68f, 0.42f), -0.8f, -0.6f, 0.8f, 0.6f);
		for (int32 I = 0; I < 6; ++I)
		{
			Disc(Center + FVector2D(-0.5f + 0.5f * (I % 3), -0.25f + 0.5f * (I / 3)) * H, Size * 0.035f, FLinearColor(0.5f, 0.36f, 0.2f));
		}
		break;
	case EGGTool::Wall:
		Block(FLinearColor(0.86f, 0.86f, 0.9f), -0.85f, -0.1f, 0.85f, 0.45f);
		Block(FLinearColor(0.2f, 0.27f, 0.38f), -0.85f, -0.45f, 0.85f, -0.1f);
		break;
	case EGGTool::Soft:
		Block(FLinearColor(0.12f, 0.36f, 0.16f), -0.85f, -0.1f, 0.85f, 0.45f);
		Block(FLinearColor(0.35f, 0.75f, 0.42f), -0.85f, -0.45f, 0.85f, -0.1f);
		break;
	case EGGTool::Pad:
		Disc(Center, H * 0.75f, FLinearColor(0.3f, 1.f, 0.55f, 0.28f));
		Ring(Center, H * 0.75f, ShopGood, Stroke);
		break;
	case EGGTool::Plate:
		Disc(Center, H * 0.55f, ShopPush);
		Ring(Center, H * 0.75f, FLinearColor(1.f, 0.6f, 0.15f, 0.5f), Stroke * 0.6f);
		break;
	case EGGTool::Weight:
		Block(ShopPush, -0.72f, -0.72f, 0.72f, 0.72f);
		Block(FLinearColor(0.07f, 0.07f, 0.08f), -0.56f, -0.56f, 0.56f, 0.56f);
		break;
	case EGGTool::Gate:
		for (int32 I = 0; I < 5; ++I)
		{
			Line(-0.7f + 0.35f * I, -0.7f, -0.7f + 0.35f * I, 0.7f, ShopPush, Stroke);
		}
		break;
	case EGGTool::Pull:
	case EGGTool::Push:
	{
		const bool bPull = static_cast<EGGTool>(Tool) == EGGTool::Pull;
		const FLinearColor Color = bPull ? ShopPull : ShopPush;
		Ring(Center, H * 0.8f, FLinearColor(Color.R, Color.G, Color.B, 0.55f), Stroke * 0.6f);
		Disc(Center, H * 0.28f, Color);
		// Arrows in for the attractor, out for the repulsor.
		for (int32 I = 0; I < 4; ++I)
		{
			const float Angle = PI * 0.5f * I + PI * 0.25f;
			const FVector2D Out(FMath::Cos(Angle), FMath::Sin(Angle));
			const FVector2D Side(-Out.Y, Out.X);
			const FVector2D Tip = Center + Out * H * (bPull ? 0.42f : 0.72f);
			const FVector2D Tail = Center + Out * H * (bPull ? 0.72f : 0.42f);
			const FVector2D Back = Tip + (Tail - Tip).GetSafeNormal() * H * 0.16f;
			Canvas->K2_DrawLine(Tail, Tip, Stroke * 0.7f, Color);
			Canvas->K2_DrawLine(Tip, Back + Side * H * 0.1f, Stroke * 0.7f, Color);
			Canvas->K2_DrawLine(Tip, Back - Side * H * 0.1f, Stroke * 0.7f, Color);
		}
		break;
	}
	case EGGTool::Mirror:
		// A glass on its diagonal, and a ball's path turning a right angle off it.
		Line(-0.62f, 0.62f, 0.62f, -0.62f, FLinearColor(0.82f, 0.9f, 1.f), Stroke * 1.5f);
		Line(-0.8f, 0.05f, -0.08f, 0.05f, ShopDim, Stroke * 0.6f);
		Line(-0.05f, 0.08f, -0.05f, 0.8f, ShopDim, Stroke * 0.6f);
		break;
	case EGGTool::Plinth:
		// A dark disc in a thin ring of light, as it lies in the ground.
		Disc(Center, H * 0.6f, FLinearColor(0.13f, 0.17f, 0.25f));
		Ring(Center, H * 0.76f, ShopPlinth, Stroke * 0.7f);
		break;
	case EGGTool::Grille:
		// The gate's bars, with rails across them.
		for (int32 I = 0; I < 5; ++I)
		{
			Line(-0.7f + 0.35f * I, -0.7f, -0.7f + 0.35f * I, 0.7f, ShopPush, Stroke);
		}
		Line(-0.85f, -0.32f, 0.85f, -0.32f, ShopInk, Stroke);
		Line(-0.85f, 0.32f, 0.85f, 0.32f, ShopInk, Stroke);
		break;
	case EGGTool::Erase:
		Line(-0.6f, -0.6f, 0.6f, 0.6f, ShopBad, Stroke * 1.3f);
		Line(-0.6f, 0.6f, 0.6f, -0.6f, ShopBad, Stroke * 1.3f);
		break;
	default:
		break;
	}
}

void AGGHUD::DrawWorkshop(AGGWorkshop* Workshop)
{
	const AGGGolfer* Golfer = Cast<AGGGolfer>(GetOwningPawn());
	const bool bTouch = Golfer && Golfer->UsingTouch();
	const float S = Unit();
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	const FGGDesign& Design = Workshop->Design();
	ShopButtons.Reset();
	// Words are sized for a desk screen; on a phone they are scaled up to stay readable.
	const float T = bTouch ? FMath::Max(S, H / 620.f) : S;

	// ---- on the ground: what the tools can't show by building it.
	const float Thin = FMath::Max(1.5f, 2.f * S);
	for (int32 I = 0; I < Design.Gates.Num(); ++I)
	{
		// A gate and the plate that opens it are tied by a line, and share a number.
		const FGGDesign::FGate& Gate = Design.Gates[I];
		if (!Design.Plates.IsValidIndex(Gate.Plate))
		{
			continue;
		}
		const FVector2D Middle((Gate.Rect.Min.X + Gate.Rect.Max.X) * 0.5, (Gate.Rect.Min.Y + Gate.Rect.Max.Y) * 0.5);
		const FIntPoint Plate = Design.Plates[Gate.Plate].At;
		FLinearColor Tie = AGGCourse::SwitchColor(Gate.Plate);
		const FVector At = Project(FVector(Middle.X, Middle.Y, 160.0), false);
		Label(FString::FromInt(Gate.Plate + 1), At.X, At.Y - 14.f * S, 20.f * T, Tie, true, true);
		if (Gate.bGrille)
		{
			// Rails, drawn over the gate's top from above: two lines along it, so a grille reads at a glance.
			const bool bAlongY = Gate.Rect.Height() >= Gate.Rect.Width();
			for (const double Side : {0.3, 0.7})
			{
				const FVector2D From = bAlongY ? FVector2D(FMath::Lerp(double(Gate.Rect.Min.X), double(Gate.Rect.Max.X), Side), Gate.Rect.Min.Y)
					: FVector2D(Gate.Rect.Min.X, FMath::Lerp(double(Gate.Rect.Min.Y), double(Gate.Rect.Max.Y), Side));
				const FVector2D To = bAlongY ? FVector2D(From.X, Gate.Rect.Max.Y) : FVector2D(Gate.Rect.Max.X, From.Y);
				GroundLine(From, To, ShopInk, 3.f * S, 152.f);
			}
		}
		Tie.A = 0.55f;
		GroundLine(Middle, FVector2D(Plate.X, Plate.Y), Tie, Thin);
	}
	if (Design.PlinthsOnly())
	{
		// Every place a machine may stand, the ones the hole gives for nothing included (starts and weight plates).
		for (const FIntPoint& Station : Design.Stations())
		{
			GroundRing(FVector2D(Station.X, Station.Y), 92.0, FLinearColor(ShopPlinth.R, ShopPlinth.G, ShopPlinth.B, 0.75f), Thin);
		}
	}
	for (int32 I = 0; I < Design.Plates.Num(); ++I)
	{
		const FVector At = Project(FVector(Design.Plates[I].At.X, Design.Plates[I].At.Y, 10.0), false);
		// Dark on a light plate's bright disc, bright on a weight plate's iron.
		Label(FString::FromInt(I + 1), At.X, At.Y - 14.f * S, 20.f * T, Design.Plates[I].bWeight ? AGGCourse::SwitchColor(I) : FLinearColor(0.1f, 0.05f, 0.f), true, true);
	}
	const FVector TeeAt = Project(FVector(Design.Tee.X, Design.Tee.Y, 0.0), false);
	Label(TEXT("TEE"), TeeAt.X, TeeAt.Y + 16.f * S, 15.f * T, ShopDim, true, true);
	const FVector CupAt = Project(FVector(Design.Cup.X, Design.Cup.Y, 270.0), false);
	Label(TEXT("CUP"), CupAt.X, CupAt.Y - 26.f * T, 15.f * T, ShopDim, true, true);
	for (int32 I = 0; I < Design.Pads.Num(); ++I)
	{
		const FVector At = Project(FVector(Design.Pads[I].X, Design.Pads[I].Y, 0.0), false);
		Label(FString::Printf(TEXT("PAD %d"), I + 1), At.X, At.Y - 10.f * S, 15.f * S, ShopGood, true, true);
	}

	// The plain shot that breaks the hole, when the check found one.
	FVector2D FlawFrom;
	double FlawYaw = 0.0;
	if (Workshop->ProofFlaw(FlawFrom, FlawYaw))
	{
		const FVector2D Along(FMath::Cos(FMath::DegreesToRadians(FlawYaw)), FMath::Sin(FMath::DegreesToRadians(FlawYaw)));
		for (int32 Dash = 0; Dash < 14; ++Dash)
		{
			GroundLine(FlawFrom + Along * (Dash * 120.0), FlawFrom + Along * (Dash * 120.0 + 70.0), ShopBad, 3.f * S, 8.f);
		}
	}

	// The pointer: the square it is over, the rectangle being dragged, or the reach of the machine about to be placed.
	const EGGTool Tool = Workshop->Tool();
	FVector2D Hover;
	FIntRect Rect;
	if (Workshop->Dragging(Rect))
	{
		GroundBox(Rect, Tool == EGGTool::Erase ? ShopBad : ShopGold, 3.f * S, 4.f);
		const FVector At = Project(FVector((Rect.Min.X + Rect.Max.X) * 0.5, (Rect.Min.Y + Rect.Max.Y) * 0.5, 4.0), false);
		Label(FString::Printf(TEXT("%.1f x %.1f m"), Rect.Width() / 100.f, Rect.Height() / 100.f), At.X, At.Y - 10.f * S, 17.f * S, ShopInk, true, true);
	}
	else if (Workshop->Hover(Hover))
	{
		const int32 Cell = FGGDesign::Cell;
		const FIntPoint Square(FMath::FloorToInt32(Hover.X / Cell) * Cell, FMath::FloorToInt32(Hover.Y / Cell) * Cell);
		const FVector2D Middle(Square.X + Cell * 0.5, Square.Y + Cell * 0.5);
		GroundBox(FIntRect(Square.X, Square.Y, Square.X + Cell, Square.Y + Cell), FLinearColor(1.f, 1.f, 1.f, 0.8f), Thin, 3.f);
		if (Tool == EGGTool::Pull)
		{
			GroundRing(Middle, 1900.0, FLinearColor(ShopPull.R, ShopPull.G, ShopPull.B, 0.5f), Thin);
		}
		else if (Tool == EGGTool::Push)
		{
			GroundRing(Middle, 1500.0, FLinearColor(ShopPush.R, ShopPush.G, ShopPush.B, 0.5f), Thin);
		}
		else if (Tool == EGGTool::Pad)
		{
			GroundRing(Middle, 240.0, FLinearColor(ShopGood.R, ShopGood.G, ShopGood.B, 0.7f), Thin);
		}
		else if (Tool == EGGTool::Plinth)
		{
			GroundRing(Middle, 92.0, ShopPlinth, Thin);
		}
	}

	// ---- top left: the hole's name, and how far along its proof is.
	const float Pad = 28.f * S;
	Label(Design.Name.ToUpper(), Pad + 12.f * S, 26.f * S, 26.f * T, ShopInk, false, true);
	const bool bSolved = Design.Solved();
	const bool bProven = bSolved && Design.Necessary == 1;
	auto Step = [&](float Y, const TCHAR* Text, int32 State) {
		// State: 1 done, 0 open, -1 failed.
		const FLinearColor Color = State > 0 ? ShopGood : State < 0 ? ShopBad : ShopDim;
		const FVector2D Dot(Pad + 22.f * T, Y + 12.f * T);
		Ring(Dot, 8.f * T, Color, 2.f * T);
		if (State != 0)
		{
			Disc(Dot, 5.f * T, Color);
		}
		Label(Text, Pad + 42.f * T, Y, 18.f * T, Color);
	};
	Step(26.f * S + 40.f * T, bSolved ? TEXT("You have solved it") : TEXT("Test it and solve it yourself"), bSolved ? 1 : 0);
	Step(26.f * S + 70.f * T, Workshop->Rating() ? TEXT("Trying the machines in random places...")
		: Workshop->Proving() ? TEXT("Checking that no swing works without the machines...")
		: Design.Necessary == 1 ? TEXT("No swing works without the machines")
		: Design.Necessary == -1 ? TEXT("A plain swing does it: not a puzzle yet") : TEXT("Check that it can't be done without the machines"),
		Workshop->Proving() ? 0 : Design.Necessary);
	if (Workshop->Proving())
	{
		const float BarW = 300.f * T;
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.5f), Pad + 42.f * T, 26.f * S + 98.f * T, BarW, 6.f * T);
		DrawRect(ShopGold, Pad + 42.f * T, 26.f * S + 98.f * T, BarW * Workshop->ProofProgress(), 6.f * T);
	}
	else
	{
		float Next = 26.f * S + 100.f * T;
		bool bEasy = false;
		if (Design.Necessary == 1 && Design.Luck >= 0)
		{
			// How hard it is: how often a random placement of the machines can be made to work.
			bEasy = Design.Luck > 35;
			if (Design.Luck == 0 && !bSolved)
			{
				// Nothing worked at random and nobody has solved it: it may not be possible at all. Say so.
				Step(Next, TEXT("No random placement works: very hard, or impossible"), 0);
			}
			else
			{
				Step(Next, *FString::Printf(TEXT("%s: %d in 100 random placements work"), FGGDesign::RatingName(Design.Luck), Design.Luck), bEasy ? -1 : 1);
			}
			Next += 30.f * T;
		}
		if (Design.Carries >= 0 && Design.PlainCarries >= 0)
		{
			// A solved hole with rails: do the rails matter? The carries its answer takes, with them and without.
			const bool bIdle = Design.Carries <= Design.PlainCarries;
			Step(Next, bIdle ? *FString::Printf(TEXT("The rails change nothing: %d %s with them or without"), Design.Carries, Design.Carries == 1 ? TEXT("carry") : TEXT("carries"))
				: *FString::Printf(TEXT("The rails matter: %d carries, %d without them"), Design.Carries, Design.PlainCarries), bIdle ? -1 : 1);
			Next += 30.f * T;
		}
		if (bProven)
		{
			Label(bEasy ? TEXT("PROVEN, BUT EASY") : TEXT("PROVEN PUZZLE"), Pad + 12.f * S, Next + 4.f * T, 20.f * T, ShopGold, false, true);
		}
	}

	// ---- top right: the buttons.
	const float Scale = bTouch ? FMath::Max(S, H / 620.f) : S;
	auto Button = [&](const TCHAR* Text, FName Action, float Right, float Width, bool bStrong, bool bEnabled = true) {
		FItem Item;
		Item.Label = Text;
		Item.Action = Action;
		Item.bEnabled = bEnabled;
		const float Height = 46.f * Scale;
		Item.Rect = FBox2D(FVector2D(Right - Width, 22.f * S), FVector2D(Right, 22.f * S + Height));
		DrawRect(bStrong ? FLinearColor(1.f, 0.78f, 0.36f, 0.92f) : ShopPanel, Item.Rect.Min.X, Item.Rect.Min.Y, Width, Height);
		if (!bStrong)
		{
			DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.22f), Item.Rect.Min.X, Item.Rect.Max.Y - 2.f * S, Width, 2.f * S);
		}
		const float Size = 19.f * Scale;
		Label(Text, Right - Width * 0.5f, Item.Rect.Min.Y + Height * 0.5f - Size * 0.72f, Size,
			bStrong ? FLinearColor(0.05f, 0.04f, 0.02f) : bEnabled ? ShopInk : ShopFaint, true, true);
		ShopButtons.Add(Item);
		return Right - Width - 10.f * Scale;
	};
	float Right = W - Pad;
	Right = Button(TEXT("MENU"), TEXT("Menu"), Right, 96.f * Scale, false);
	Right = Button(TEXT("UNDO"), TEXT("Undo"), Right, 96.f * Scale, false, Workshop->CanUndo());
	Right = Button(TEXT("CHECK"), TEXT("Check"), Right, 104.f * Scale, false, !Workshop->Proving());
	Right = Button(TEXT("TEST"), TEXT("Test"), Right, 112.f * Scale, true);

	// ---- bottom: the tools.
	const int32 Count = static_cast<int32>(EGGTool::Count);
	const float Tile = FMath::Min(bTouch ? FMath::Max(84.f * S, H * 0.135f) : 84.f * S, (W - 2.f * Pad) / Count - 6.f * S);
	const float Gap = 6.f * S;
	const float Total = Count * Tile + (Count - 1) * Gap;
	const float Left = (W - Total) * 0.5f;
	const float Top = H - Tile - 30.f * S;
	DrawRect(ShopPanel, Left - 12.f * S, Top - 12.f * S, Total + 24.f * S, Tile + 24.f * S);
	for (int32 I = 0; I < Count; ++I)
	{
		FItem Item;
		Item.Action = TEXT("Tool");
		Item.Number = I;
		const float X = Left + I * (Tile + Gap);
		Item.Rect = FBox2D(FVector2D(X, Top), FVector2D(X + Tile, Top + Tile));
		const bool bChosen = static_cast<EGGTool>(I) == Tool;
		DrawRect(FLinearColor(1.f, 1.f, 1.f, bChosen ? 0.2f : 0.05f), X, Top, Tile, Tile);
		if (bChosen)
		{
			DrawRect(ShopGold, X, Top + Tile - 4.f * S, Tile, 4.f * S);
		}
		DrawTool(I, FVector2D(X + Tile * 0.5f, Top + Tile * 0.5f), Tile * 0.62f);
		ShopButtons.Add(Item);
	}
	// The chosen tool's name, and either what the Workshop last said or what the tool does.
	const FString& Said = Workshop->Message();
	Label(FString(AGGWorkshop::ToolName(Tool)).ToUpper(), W * 0.5f, Top - 14.f * S - 60.f * T, 20.f * T, ShopGold, true, true);
	Label(Said.IsEmpty() ? FString(AGGWorkshop::ToolHelp(Tool)) : Said, W * 0.5f, Top - 14.f * S - 30.f * T, 19.f * T, ShopInk, true);

	if (!bTouch)
	{
		Label(TEXT("W A S D  slide the view      Wheel  zoom      Right drag  pan      Q E  next tool      Z  undo      F  see it all      T  test      Tab  menu"),
			W * 0.5f, H - 25.f * S, 15.f * S, ShopDim, true);
	}
	else
	{
		Label(TEXT("Two fingers slide and zoom the view"), W * 0.5f, H - 27.f * S, 13.f * T, ShopDim, true);
	}
}

bool AGGHUD::WorkshopOver(const FVector2D& At) const
{
	for (const FItem& Item : ShopButtons)
	{
		if (Item.Rect.bIsValid && Item.Rect.ExpandBy(4.0).IsInside(At))
		{
			return true;
		}
	}
	return false;
}

bool AGGHUD::WorkshopPress(const FVector2D& At)
{
	AGGWorkshop* Workshop = AGGWorkshop::Get(this);
	if (!Workshop || !Workshop->Building())
	{
		return false;
	}
	for (const FItem& Item : ShopButtons)
	{
		if (!Item.Rect.bIsValid || !Item.Rect.ExpandBy(4.0).IsInside(At))
		{
			continue;
		}
		if (!Item.bEnabled)
		{
			return true;
		}
		if (Item.Action == TEXT("Tool"))
		{
			Workshop->SetTool(static_cast<EGGTool>(Item.Number));
		}
		else if (Item.Action == TEXT("Test"))
		{
			Workshop->StartTesting();
		}
		else if (Item.Action == TEXT("Check"))
		{
			Workshop->StartProof();
		}
		else if (Item.Action == TEXT("Undo"))
		{
			Workshop->Undo();
		}
		else if (Item.Action == TEXT("Menu"))
		{
			OpenMenu(EGGMenu::Workshop);
		}
		return true;
	}
	return false;
}
