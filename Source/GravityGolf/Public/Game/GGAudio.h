#pragma once

#include "CoreMinimal.h"
#include "Sim/GGSim.h"

/**
 * Every sound in the spike is synthesized at runtime from noise and sine waves, so there are no audio assets to
 * source or license yet. Recorded foley replaces these one by one later.
 */
namespace GGAudio
{
void Footstep(UObject* WorldContext, const FVector& Location, GG::ESurface Surface);
void Strike(UObject* WorldContext, const FVector& Location, float Power);
void Bounce(UObject* WorldContext, const FVector& Location, GG::ESurface Surface, float Speed);
void Sunk(UObject* WorldContext, const FVector& Location);
void MachinePickUp(UObject* WorldContext, const FVector& Location);
void MachinePlace(UObject* WorldContext, const FVector& Location);
/** The field taking hold of the ball (rising) or letting it go (falling). */
void MachineField(UObject* WorldContext, const FVector& Location, bool bRelease);
} // namespace GGAudio
