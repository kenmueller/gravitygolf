#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"

#include "GGHUD.generated.h"

/** Placeholder HUD for the spike: score line, wells left and status. */
UCLASS()
class GRAVITYGOLF_API AGGHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
};
