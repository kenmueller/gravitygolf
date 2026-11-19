#include "Hole/GGHoleLoader.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogGGHole, Log, All);

namespace
{
/**
 * Trig results can differ in the last bit between platforms. Rounding derived values to a fixed grid makes the
 * simulation's inputs identical everywhere, which keeps replays deterministic.
 */
double Quantize(double Value)
{
	return FMath::RoundToDouble(Value * 1e9) / 1e9;
}

GG::FVec2 DirectionDegrees(double Degrees)
{
	const double Radians = FMath::DegreesToRadians(Degrees);
	return {Quantize(FMath::Cos(Radians)), Quantize(FMath::Sin(Radians))};
}

double Number(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, double Default)
{
	double Value = Default;
	Object->TryGetNumberField(Field, Value);
	return Value;
}

GG::ESurface ParseSurface(const FString& Surface)
{
	if (Surface == TEXT("ice"))
	{
		return GG::ESurface::Ice;
	}
	if (Surface == TEXT("dust"))
	{
		return GG::ESurface::Dust;
	}
	if (Surface == TEXT("hazard"))
	{
		return GG::ESurface::Hazard;
	}
	return GG::ESurface::Grass;
}
} // namespace

bool GGHoleLoader::Load(const FString& Name, FGGHoleDefinition& Out)
{
	const FString Path = FPaths::ProjectContentDir() / TEXT("Holes") / (Name + TEXT(".json"));
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path))
	{
		UE_LOG(LogGGHole, Error, TEXT("Can't read hole file %s"), *Path);
		return false;
	}

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogGGHole, Error, TEXT("Invalid JSON in %s"), *Path);
		return false;
	}

	Out = FGGHoleDefinition();
	Out.Id = Root->GetStringField(TEXT("id"));
	Out.Name = Root->GetStringField(TEXT("name"));

	GG::FHole& Hole = Out.Sim;
	Hole.Par = static_cast<int>(Number(Root, TEXT("par"), 3));
	Hole.BallRadius = Number(Root, TEXT("ballRadius"), 30);

	const TSharedPtr<FJsonObject>* Bounds = nullptr;
	if (Root->TryGetObjectField(TEXT("bounds"), Bounds))
	{
		Hole.BoundsCenter = {Number(*Bounds, TEXT("x"), 0), Number(*Bounds, TEXT("y"), 0)};
		Hole.BoundsRadius = Number(*Bounds, TEXT("radius"), 10000);
	}

	const TSharedPtr<FJsonObject>* Inventory = nullptr;
	if (Root->TryGetObjectField(TEXT("inventory"), Inventory))
	{
		Hole.MaxGravityWells = static_cast<int>(Number(*Inventory, TEXT("gravity"), 0));
		Hole.MaxAntigravityWells = static_cast<int>(Number(*Inventory, TEXT("antigravity"), 0));
	}

	const TArray<TSharedPtr<FJsonValue>>* Planets = nullptr;
	if (!Root->TryGetArrayField(TEXT("planets"), Planets) || Planets->Num() == 0)
	{
		UE_LOG(LogGGHole, Error, TEXT("Hole %s has no planets"), *Name);
		return false;
	}

	for (const TSharedPtr<FJsonValue>& Value : *Planets)
	{
		const TSharedPtr<FJsonObject> P = Value->AsObject();
		GG::FPlanet Planet;
		Planet.Position = {Number(P, TEXT("x"), 0), Number(P, TEXT("y"), 0)};
		Planet.Radius = Number(P, TEXT("radius"), 300);
		Planet.SurfaceGravity = Number(P, TEXT("gravity"), 600);
		Planet.Surface = ParseSurface(P->GetStringField(TEXT("surface")));

		const TSharedPtr<FJsonObject>* Atmosphere = nullptr;
		if (P->TryGetObjectField(TEXT("atmosphere"), Atmosphere))
		{
			Planet.AtmosphereRadius = Number(*Atmosphere, TEXT("radius"), 0);
			Planet.AtmosphereDrag = Number(*Atmosphere, TEXT("drag"), 0);
		}

		const TSharedPtr<FJsonObject>* Cup = nullptr;
		if (P->TryGetObjectField(TEXT("cup"), Cup))
		{
			Planet.bHasCup = true;
			Planet.CupDirection = DirectionDegrees(Number(*Cup, TEXT("angle"), 90));
			Planet.CupCos = Quantize(FMath::Cos(FMath::DegreesToRadians(Number(*Cup, TEXT("halfWidth"), 6))));
		}

		FGGPlanetLook Look;
		Look.Id = P->GetStringField(TEXT("id"));
		P->TryGetStringField(TEXT("style"), Look.Style);
		const TSharedPtr<FJsonObject>* Rings = nullptr;
		if (P->TryGetObjectField(TEXT("rings"), Rings))
		{
			Look.RingInner = Number(*Rings, TEXT("inner"), 0);
			Look.RingOuter = Number(*Rings, TEXT("outer"), 0);
			Look.RingTilt = Number(*Rings, TEXT("tilt"), 0);
		}

		Hole.Planets.push_back(Planet);
		Out.Looks.Add(Look);
	}

	const TSharedPtr<FJsonObject>* Tee = nullptr;
	if (Root->TryGetObjectField(TEXT("tee"), Tee))
	{
		const FString TeePlanet = (*Tee)->GetStringField(TEXT("planet"));
		Hole.TeePlanet = FMath::Max(0, Out.Looks.IndexOfByPredicate([&](const FGGPlanetLook& L) { return L.Id == TeePlanet; }));
		Hole.TeeDirection = DirectionDegrees(Number(*Tee, TEXT("angle"), 90));
	}

	const TArray<TSharedPtr<FJsonValue>>* Wells = nullptr;
	if (Root->TryGetArrayField(TEXT("wells"), Wells))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Wells)
		{
			const TSharedPtr<FJsonObject> W = Value->AsObject();
			Hole.FixedWells.push_back({{Number(W, TEXT("x"), 0), Number(W, TEXT("y"), 0)}, Number(W, TEXT("strength"), 3e8)});
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* Stardust = nullptr;
	if (Root->TryGetArrayField(TEXT("stardust"), Stardust))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Stardust)
		{
			const TSharedPtr<FJsonObject> S = Value->AsObject();
			Hole.Stardust.push_back({{Number(S, TEXT("x"), 0), Number(S, TEXT("y"), 0)}, Number(S, TEXT("radius"), 45)});
		}
	}

	const TSharedPtr<FJsonObject>* Sun = nullptr;
	if (Root->TryGetObjectField(TEXT("sun"), Sun))
	{
		Out.SunRotation = FRotator(Number(*Sun, TEXT("pitch"), -40), Number(*Sun, TEXT("yaw"), -35), 0);
		Out.SunIntensity = Number(*Sun, TEXT("intensity"), 10);
		FString Hex;
		if ((*Sun)->TryGetStringField(TEXT("color"), Hex))
		{
			Out.SunColor = FLinearColor::FromSRGBColor(FColor::FromHex(Hex));
		}
	}

	UE_LOG(LogGGHole, Log, TEXT("Loaded hole %s (%d planets)"), *Out.Name, Out.Looks.Num());
	return true;
}
