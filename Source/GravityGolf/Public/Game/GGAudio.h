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
void SwitchPress(UObject* WorldContext, const FVector& Location);
void GateShut(UObject* WorldContext, const FVector& Location);
/** The air of a place, looping under everything: "wind", "canyon", "night", "water" or "hum". */
void Ambience(UObject* WorldContext, const FString& Kind);
void SetAmbienceVolume(float Volume);
/** Slow music for a place, under everything else: four chords and a glass note now and then. Same names as the air. */
void Music(UObject* WorldContext, const FString& Kind);
void SetMusicVolume(float Volume);
/** Write every sound to a folder as WAV files, as the game would play them, for measuring. */
void Export(const FString& Folder);
} // namespace GGAudio
