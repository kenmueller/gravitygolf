#include "Game/GGHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Game/GGCourse.h"
#include "Game/GGGolfer.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void AGGHUD::DrawHUD()
{
	Super::DrawHUD();

	const AGGGolfer* Golfer = Cast<AGGGolfer>(GetOwningPawn());
	const AGGCourse* Course = Golfer ? Golfer->Course() : nullptr;
	if (!Course || !Canvas || FParse::Param(FCommandLine::Get(), TEXT("GGNoHUD")))
	{
		return;
	}

	UFont* Font = GEngine->GetLargeFont();
	const float S = Canvas->ClipY / 1080.f;
	const FLinearColor Text(0.95f, 0.95f, 0.92f);
	const FLinearColor Dim(0.95f, 0.95f, 0.92f, 0.7f);
	const float CX = Canvas->ClipX * 0.5f;
	const float CY = Canvas->ClipY * 0.5f;

	// Every line gets a soft dark shadow, so it reads over bright rock, sky or salt.
	auto Shadowed = [&](const FString& Line, const FLinearColor& Color, float X, float Y, float Scale) {
		const FLinearColor Shade(0.f, 0.f, 0.f, 0.75f * Color.A);
		DrawText(Line, Shade, X + 1.5f * S, Y + 1.5f * S, Font, Scale);
		DrawText(Line, Shade, X - 1.f * S, Y + 1.f * S, Font, Scale);
		DrawText(Line, Color, X, Y, Font, Scale);
	};
	auto Centered = [&](const FString& Line, float Y, float Scale, const FLinearColor& Color) {
		float W = 0.f;
		float H = 0.f;
		GetTextSize(Line, W, H, Font, Scale);
		Shadowed(Line, Color, CX - W * 0.5f, Y, Scale);
	};

	DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.85f), CX - 1.5f * S, CY - 1.5f * S, 3.f * S, 3.f * S);

	Shadowed(Course->CourseName().ToUpper(), Text, 40.f * S, 32.f * S, 1.5f * S);
	Shadowed(FString::Printf(TEXT("Swings %d"), Course->Strokes()), Dim, 40.f * S, 66.f * S, 1.1f * S);

	if (!Course->StatusText().IsEmpty())
	{
		Centered(Course->StatusText(), Canvas->ClipY * 0.2f, 2.4f * S, Text);
	}

	FString Prompt;
	if (Course->Phase() == EGGPhase::Sunk)
	{
		Prompt = Course->HasNext() ? TEXT("Click to continue") : TEXT("Click to play again");
	}
	else if (Golfer->IsCarrying())
	{
		Prompt = TEXT("E: set down the gravity machine");
	}
	else if (Golfer->IsCharging())
	{
		const float W = 320.f * S;
		const float Y = Canvas->ClipY * 0.8f;
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.45f), CX - W * 0.5f, Y, W, 10.f * S);
		DrawRect(FLinearColor::LerpUsingHSV(FLinearColor(0.5f, 1.f, 0.6f), FLinearColor(1.f, 0.35f, 0.2f), Golfer->Charge()),
			CX - W * 0.5f, Y, W * Golfer->Charge(), 10.f * S);
	}
	else if (Golfer->CanPickUp())
	{
		Prompt = TEXT("E: pick up the gravity machine");
	}
	else if (Golfer->CanSwing())
	{
		Prompt = TEXT("Hold click to swing    Right click: watch the ball");
	}
	else if (Course->Phase() == EGGPhase::Ready)
	{
		Prompt = FString::Printf(TEXT("Ball: %d m    B: go to it    Shift: run    R: return ball    Backspace: reset hole"),
			FMath::RoundToInt32(Golfer->DistanceToBall() / 100.0));
	}
	if (!Prompt.IsEmpty())
	{
		Centered(Prompt, Canvas->ClipY * 0.86f, 1.2f * S, Dim);
	}
}
