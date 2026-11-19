#include "Game/GGHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "EngineUtils.h"
#include "Game/GGHoleDirector.h"

void AGGHUD::DrawHUD()
{
	Super::DrawHUD();

	TActorIterator<AGGHoleDirector> It(GetWorld());
	AGGHoleDirector* Director = It ? *It : nullptr;
	if (!Director || !Canvas)
	{
		return;
	}

	UFont* Font = GEngine->GetLargeFont();
	const float Scale = Canvas->ClipY / 1080.f * 1.4f;
	const FLinearColor Text(0.92f, 0.95f, 1.f);
	const FLinearColor Dim(0.6f, 0.68f, 0.8f);

	const FGGHoleDefinition& Def = Director->Definition();
	DrawText(Def.Name.ToUpper(), Text, 40.f * Scale / 1.4f, 32.f * Scale / 1.4f, Font, Scale);
	DrawText(FString::Printf(TEXT("Par %d    Strokes %d    Stardust %d/%d"), Def.Sim.Par, Director->Strokes(),
				 Director->StardustCollected(), static_cast<int32>(Def.Sim.Stardust.size())),
		Dim, 40.f * Scale / 1.4f, 62.f * Scale / 1.4f, Font, Scale * 0.8f);

	if (Def.Sim.MaxGravityWells + Def.Sim.MaxAntigravityWells > 0)
	{
		DrawText(FString::Printf(TEXT("Wells:  gravity %d  (G)    antigravity %d  (A)"), Director->GravityWellsLeft(),
					 Director->AntigravityWellsLeft()),
			Dim, 40.f * Scale / 1.4f, 88.f * Scale / 1.4f, Font, Scale * 0.8f);
	}

	const FString Status = Director->StatusText();
	if (!Status.IsEmpty())
	{
		float W = 0.f;
		float H = 0.f;
		GetTextSize(Status, W, H, Font, Scale * 1.6f);
		DrawText(Status, Text, (Canvas->ClipX - W) * 0.5f, Canvas->ClipY * 0.16f, Font, Scale * 1.6f);
	}

	const FString Hint = Director->Phase() == EGGPhase::Sunk
		? TEXT("Click to play again")
		: TEXT("Drag toward where the ball should go  ·  G: gravity well  ·  R: restart  ·  Right-drag: pan  ·  Wheel: zoom");
	float W = 0.f;
	float H = 0.f;
	GetTextSize(Hint, W, H, Font, Scale * 0.7f);
	DrawText(Hint, Dim, (Canvas->ClipX - W) * 0.5f, Canvas->ClipY - 50.f * Scale / 1.4f, Font, Scale * 0.7f);
}
