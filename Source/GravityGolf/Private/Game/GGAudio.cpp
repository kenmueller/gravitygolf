#include "Game/GGAudio.h"

#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWaveProcedural.h"
#include "TimerManager.h"

namespace
{
constexpr int32 Rate = 44100;

struct FTone
{
	TArray<float> Samples;

	explicit FTone(float Seconds) { Samples.SetNumZeroed(FMath::CeilToInt32(Seconds * Rate)); }

	/** Filtered noise burst with an exponential decay. */
	void Noise(float Start, float Decay, float Cutoff, float Gain, int32 Seed)
	{
		FRandomStream Random(Seed);
		const float Alpha = 1.f - FMath::Exp(-2.f * PI * Cutoff / Rate);
		float State = 0.f;
		for (int32 I = FMath::RoundToInt32(Start * Rate); I < Samples.Num(); ++I)
		{
			const float T = static_cast<float>(I) / Rate - Start;
			State += Alpha * (Random.FRandRange(-1.f, 1.f) - State);
			const float Attack = FMath::Min(T / 0.004f, 1.f);
			Samples[I] += State * Attack * FMath::Exp(-T / Decay) * Gain;
		}
	}

	/** Sine that glides from one pitch to another while decaying. */
	void Sine(float Start, float Decay, float FromHz, float ToHz, float Gain)
	{
		float Phase = 0.f;
		for (int32 I = FMath::RoundToInt32(Start * Rate); I < Samples.Num(); ++I)
		{
			const float T = static_cast<float>(I) / Rate - Start;
			const float Hz = FMath::Lerp(ToHz, FromHz, FMath::Exp(-T / (Decay * 0.5f)));
			Phase += 2.f * PI * Hz / Rate;
			const float Attack = FMath::Min(T / 0.002f, 1.f);
			Samples[I] += FMath::Sin(Phase) * Attack * FMath::Exp(-T / Decay) * Gain;
		}
	}
};

void Play(UObject* WorldContext, const FVector& Location, const FTone& Tone, float Volume)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World || Tone.Samples.IsEmpty())
	{
		return;
	}

	TArray<int16> Pcm;
	Pcm.SetNumUninitialized(Tone.Samples.Num());
	for (int32 I = 0; I < Pcm.Num(); ++I)
	{
		Pcm[I] = static_cast<int16>(FMath::Clamp(Tone.Samples[I], -1.f, 1.f) * 32000.f);
	}

	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(GetTransientPackage());
	Wave->SetSampleRate(Rate);
	Wave->NumChannels = 1;
	Wave->Duration = static_cast<float>(Pcm.Num()) / Rate;
	Wave->SoundGroup = SOUNDGROUP_Effects;
	Wave->bLooping = false;
	Wave->QueueAudio(reinterpret_cast<const uint8*>(Pcm.GetData()), Pcm.Num() * sizeof(int16));

	// A procedural wave never reports that it finished, so stop its voice ourselves.
	if (UAudioComponent* Voice = UGameplayStatics::SpawnSoundAtLocation(World, Wave, Location, FRotator::ZeroRotator, Volume))
	{
		FTimerHandle Handle;
		World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(Voice, [Voice]() { Voice->Stop(); }),
			Wave->Duration + 0.1f, false);
	}
}

int32 NextSeed()
{
	static int32 Seed = 1;
	return Seed++;
}
} // namespace

void GGAudio::Footstep(UObject* WorldContext, const FVector& Location, GG::ESurface Surface)
{
	FTone Tone(0.22f);
	const int32 Seed = NextSeed();
	const float Vary = 0.85f + 0.3f * FMath::Frac(Seed * 0.618f);
	if (Surface == GG::ESurface::Grass || Surface == GG::ESurface::Green || Surface == GG::ESurface::Sand)
	{
		// Soft and brushy: a muffled thump, then blades springing back.
		Tone.Noise(0.f, 0.05f, 700.f * Vary, 0.9f, Seed);
		Tone.Noise(0.03f, 0.07f, 2600.f * Vary, 0.22f, Seed + 101);
		Tone.Sine(0.f, 0.035f, 95.f, 60.f, 0.35f);
	}
	else
	{
		// Hard sole on stone: a sharp tick over a short body.
		Tone.Noise(0.f, 0.012f, 5200.f * Vary, 0.5f, Seed);
		Tone.Noise(0.004f, 0.04f, 1400.f * Vary, 0.45f, Seed + 101);
		Tone.Sine(0.f, 0.03f, 150.f * Vary, 90.f, 0.4f);
	}
	Play(WorldContext, Location, Tone, 0.5f);
}

void GGAudio::Strike(UObject* WorldContext, const FVector& Location, float Power)
{
	FTone Tone(0.25f);
	Tone.Sine(0.f, 0.018f, 2600.f, 1500.f, 0.6f);
	Tone.Sine(0.f, 0.05f, 620.f, 420.f, 0.35f);
	Tone.Noise(0.f, 0.008f, 7000.f, 0.5f, NextSeed());
	Play(WorldContext, Location, Tone, 0.35f + 0.65f * Power);
}

void GGAudio::Bounce(UObject* WorldContext, const FVector& Location, GG::ESurface Surface, float Speed)
{
	const bool bSoft = Surface == GG::ESurface::Grass || Surface == GG::ESurface::Green || Surface == GG::ESurface::Sand;
	FTone Tone(0.2f);
	Tone.Sine(0.f, bSoft ? 0.04f : 0.025f, bSoft ? 170.f : 900.f, bSoft ? 90.f : 600.f, 0.55f);
	Tone.Noise(0.f, bSoft ? 0.03f : 0.01f, bSoft ? 900.f : 5000.f, 0.4f, NextSeed());
	Play(WorldContext, Location, Tone, FMath::Clamp(Speed / 900.f, 0.08f, 1.f));
}

void GGAudio::Sunk(UObject* WorldContext, const FVector& Location)
{
	FTone Tone(1.4f);
	// The ball rattles down the cup, then the course answers with a chord.
	for (int32 I = 0; I < 4; ++I)
	{
		Tone.Sine(0.05f * I * I, 0.02f, 1500.f - 180.f * I, 900.f, 0.4f);
	}
	Tone.Sine(0.28f, 0.5f, 523.25f, 523.25f, 0.2f);
	Tone.Sine(0.34f, 0.5f, 659.25f, 659.25f, 0.18f);
	Tone.Sine(0.40f, 0.6f, 783.99f, 783.99f, 0.18f);
	Play(WorldContext, Location, Tone, 0.9f);
}

void GGAudio::MachinePickUp(UObject* WorldContext, const FVector& Location)
{
	FTone Tone(0.35f);
	Tone.Sine(0.f, 0.12f, 220.f, 440.f, 0.35f);
	Tone.Noise(0.f, 0.02f, 3000.f, 0.25f, NextSeed());
	Play(WorldContext, Location, Tone, 0.6f);
}

void GGAudio::MachinePlace(UObject* WorldContext, const FVector& Location)
{
	FTone Tone(0.7f);
	Tone.Noise(0.f, 0.02f, 1800.f, 0.5f, NextSeed());
	Tone.Sine(0.f, 0.05f, 140.f, 70.f, 0.6f);
	Tone.Sine(0.08f, 0.3f, 440.f, 220.f, 0.22f);
	Tone.Sine(0.08f, 0.3f, 660.f, 330.f, 0.12f);
	Play(WorldContext, Location, Tone, 0.7f);
}

void GGAudio::MachineField(UObject* WorldContext, const FVector& Location, bool bRelease)
{
	FTone Tone(0.6f);
	if (bRelease)
	{
		Tone.Sine(0.f, 0.22f, 660.f, 330.f, 0.3f);
		Tone.Sine(0.f, 0.22f, 990.f, 495.f, 0.14f);
	}
	else
	{
		Tone.Sine(0.f, 0.25f, 220.f, 440.f, 0.28f);
		Tone.Sine(0.f, 0.25f, 330.f, 660.f, 0.12f);
	}
	Play(WorldContext, Location, Tone, 0.55f);
}
