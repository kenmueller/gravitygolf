#pragma once

#include "CoreMinimal.h"
#include "Sim/GGSim.h"

/** How a planet looks. The simulation never reads this. */
struct FGGPlanetLook
{
	FString Id;
	FString Style;
	float RingInner = 0.f;
	float RingOuter = 0.f;
	float RingTilt = 0.f;
};

struct FGGHoleDefinition
{
	FString Id;
	FString Name;
	GG::FHole Sim;
	TArray<FGGPlanetLook> Looks;
	FRotator SunRotation = FRotator(-40.f, -35.f, 0.f);
	float SunIntensity = 10.f;
	FLinearColor SunColor = FLinearColor::White;
};

namespace GGHoleLoader
{
/** Loads Content/Holes/<Name>.json. Returns false and logs on any error. */
bool Load(const FString& Name, FGGHoleDefinition& Out);

/** Sim plane (x right, y up) to Unreal world, with the play plane at Z = 0. */
inline FVector ToWorld(GG::FVec2 P, double Z = 0.0) { return FVector(P.Y, P.X, Z); }
inline GG::FVec2 ToSim(const FVector& W) { return {W.Y, W.X}; }
} // namespace GGHoleLoader
