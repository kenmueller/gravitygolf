#include "Game/GGAudio.h"

#include "Game/GGSave.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Sound/SoundWaveProcedural.h"
#include "TimerManager.h"

#include <atomic>

namespace
{
constexpr int32 Rate = 44100;

struct FTone
{
	TArray<float> Samples;

	explicit FTone(float Seconds) { Samples.SetNumZeroed(FMath::CeilToInt32(Seconds * Rate)); }

	/** Filtered noise burst with an exponential decay. The filter is three gentle stages in a row: one stage alone
	 *  lets so much of the top through that a footstep on grass measured as bright as a cymbal. */
	void Noise(float Start, float Decay, float Cutoff, float Gain, int32 Seed)
	{
		FRandomStream Random(Seed);
		const float Alpha = 1.f - FMath::Exp(-2.f * PI * Cutoff / Rate);
		float A = 0.f;
		float B = 0.f;
		float C = 0.f;
		// Each stage takes level off as well as brightness; this puts the level back.
		const float Makeup = 2.6f;
		for (int32 I = FMath::RoundToInt32(Start * Rate); I < Samples.Num(); ++I)
		{
			const float T = static_cast<float>(I) / Rate - Start;
			A += Alpha * (Random.FRandRange(-1.f, 1.f) - A);
			B += Alpha * (A - B);
			C += Alpha * (B - C);
			const float Attack = FMath::Min(T / 0.004f, 1.f);
			Samples[I] += C * Makeup * Attack * FMath::Exp(-T / Decay) * Gain;
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

// For GGAudio::Export: while a folder is set, sounds are written to it as WAV files instead of being played.
FString ExportFolder;
FString ExportName;

void WriteWav(const FString& Path, const TArray<float>& Samples, float Volume)
{
	TArray<uint8> File;
	auto Put32 = [&](uint32 Value) { File.Append(reinterpret_cast<const uint8*>(&Value), 4); };
	auto Put16 = [&](uint16 Value) { File.Append(reinterpret_cast<const uint8*>(&Value), 2); };
	const uint32 Bytes = Samples.Num() * 2;
	File.Append(reinterpret_cast<const uint8*>("RIFF"), 4);
	Put32(36 + Bytes);
	File.Append(reinterpret_cast<const uint8*>("WAVEfmt "), 8);
	Put32(16);
	Put16(1);
	Put16(1);
	Put32(Rate);
	Put32(Rate * 2);
	Put16(2);
	Put16(16);
	File.Append(reinterpret_cast<const uint8*>("data"), 4);
	Put32(Bytes);
	float Peak = 0.f;
	for (const float Sample : Samples)
	{
		// What the game does to a sample: the tone is clipped to full scale first, then played at its volume.
		Peak = FMath::Max(Peak, FMath::Abs(Sample));
		Put16(static_cast<uint16>(static_cast<int16>(FMath::Clamp(Sample, -1.f, 1.f) * Volume * 32000.f)));
	}
	FFileHelper::SaveArrayToFile(File, *Path);
	UE_LOG(LogTemp, Display, TEXT("Sound %s: %.2f s, peak before clipping %.2f, played at volume %.2f"), *FPaths::GetBaseFilename(Path),
		static_cast<float>(Samples.Num()) / Rate, Peak, Volume);
}

void Play(UObject* WorldContext, const FVector& Location, const FTone& Tone, float Volume)
{
	if (!ExportFolder.IsEmpty())
	{
		WriteWav(ExportFolder / (ExportName + TEXT(".wav")), Tone.Samples, Volume);
		return;
	}
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
	Volume *= UGGSave::Get().Volume;
	if (UAudioComponent* Voice = UGameplayStatics::SpawnSoundAtLocation(World, Wave, Location, FRotator::ZeroRotator, Volume))
	{
		FTimerHandle Handle;
		World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(Voice, [Voice]() { Voice->Stop(); }),
			Wave->Duration + 0.1f, false);
	}
}

// ---- ambience: a ten-second bed of air for each kind of place, made once and looped for as long as the hole lasts.

constexpr float BedSeconds = 10.f;

/** Noise shaped by a low-pass (three stages) and a slow swell, added to a bed. Swell 0 is steady. */
void Air(TArray<float>& Bed, float Cutoff, float Gain, float SwellHz, float SwellDepth, int32 Seed)
{
	FRandomStream Random(Seed);
	const float Alpha = 1.f - FMath::Exp(-2.f * PI * Cutoff / Rate);
	float A = 0.f;
	float B = 0.f;
	float C = 0.f;
	const float PhaseA = Random.FRandRange(0.f, 2.f * PI);
	const float PhaseB = Random.FRandRange(0.f, 2.f * PI);
	for (int32 I = 0; I < Bed.Num(); ++I)
	{
		const float T = static_cast<float>(I) / Rate;
		A += Alpha * (Random.FRandRange(-1.f, 1.f) - A);
		B += Alpha * (A - B);
		C += Alpha * (B - C);
		// Two slow waves that both come round exactly at the end of the bed, so the loop has no seam in its swell.
		const float Swell = 1.f + SwellDepth * (0.6f * FMath::Sin(2.f * PI * FMath::RoundToFloat(SwellHz * BedSeconds) * T / BedSeconds + PhaseA)
			+ 0.4f * FMath::Sin(2.f * PI * FMath::RoundToFloat(SwellHz * 2.7f * BedSeconds) * T / BedSeconds + PhaseB));
		Bed[I] += C * 2.6f * Gain * FMath::Max(Swell, 0.f);
	}
}

/** A steady tone with a slow beat, for the Workshop's hum. */
void Drone(TArray<float>& Bed, float Hz, float Gain, float BeatHz)
{
	// Whole numbers of cycles in the bed, so the loop has no click.
	const float Cycles = FMath::RoundToFloat(Hz * BedSeconds);
	const float Beats = FMath::RoundToFloat(BeatHz * BedSeconds);
	for (int32 I = 0; I < Bed.Num(); ++I)
	{
		const float T = static_cast<float>(I) / Rate;
		Bed[I] += FMath::Sin(2.f * PI * Cycles * T / BedSeconds) * Gain * (0.75f + 0.25f * FMath::Sin(2.f * PI * Beats * T / BedSeconds));
	}
}

/** Crickets: bursts of quick, high chirps. */
void Crickets(TArray<float>& Bed, float Hz, float Every, float Offset, float Gain)
{
	for (float Burst = Offset; Burst < BedSeconds - 0.6f; Burst += Every)
	{
		for (int32 Chirp = 0; Chirp < 9; ++Chirp)
		{
			const int32 Start = FMath::RoundToInt32((Burst + Chirp * 0.033f) * Rate);
			const int32 Length = FMath::RoundToInt32(0.018f * Rate);
			for (int32 I = 0; I < Length && Start + I < Bed.Num(); ++I)
			{
				const float T = static_cast<float>(I) / Length;
				Bed[Start + I] += FMath::Sin(2.f * PI * Hz * I / Rate) * FMath::Sin(PI * T) * Gain;
			}
		}
	}
}

/** The bed for one kind of place. Unknown kinds get plain quiet air. */
TArray<float> MakeBed(const FString& Kind)
{
	TArray<float> Bed;
	Bed.SetNumZeroed(FMath::RoundToInt32(BedSeconds * Rate));
	if (Kind == TEXT("wind"))
	{
		// Open ground: a low wind that comes and goes, and a thin hiss of air over it.
		Air(Bed, 420.f, 0.22f, 0.12f, 0.6f, 11);
		Air(Bed, 2400.f, 0.02f, 0.2f, 0.7f, 12);
	}
	else if (Kind == TEXT("canyon"))
	{
		// A slot of rock: deeper and hollower, the wind caught in it.
		Air(Bed, 240.f, 0.3f, 0.1f, 0.7f, 21);
		Air(Bed, 900.f, 0.035f, 0.3f, 0.8f, 22);
	}
	else if (Kind == TEXT("night"))
	{
		// Still air and crickets.
		Air(Bed, 320.f, 0.09f, 0.08f, 0.4f, 31);
		Crickets(Bed, 4300.f, 1.37f, 0.3f, 0.012f);
		Crickets(Bed, 3700.f, 2.11f, 0.9f, 0.008f);
	}
	else if (Kind == TEXT("water"))
	{
		// A flooded hall: air, and water moving against stone.
		Air(Bed, 300.f, 0.12f, 0.08f, 0.4f, 41);
		Air(Bed, 1300.f, 0.045f, 0.4f, 0.95f, 42);
	}
	else if (Kind == TEXT("hum"))
	{
		// The Workshop's table: a low hum, almost below hearing, and a breath of air.
		Drone(Bed, 55.f, 0.05f, 0.2f);
		Drone(Bed, 110.2f, 0.02f, 0.3f);
		Air(Bed, 500.f, 0.05f, 0.1f, 0.3f, 51);
	}
	else
	{
		Air(Bed, 400.f, 0.1f, 0.1f, 0.4f, 61);
	}
	// The noise does not come round on itself as the swells do: fade the last second into the first.
	const int32 Fade = Rate;
	for (int32 I = 0; I < Fade; ++I)
	{
		const float T = static_cast<float>(I) / Fade;
		Bed[I] = Bed[I] * T + Bed[Bed.Num() - Fade + I] * (1.f - T);
	}
	Bed.SetNum(Bed.Num() - Fade);
	// Every bed sits at the same quiet level, about 15 dB under a footstep, whatever it is made of. (As first written
	// the wind measured louder than the ball dropping into the cup.)
	double Sum = 0.0;
	for (const float Sample : Bed)
	{
		Sum += static_cast<double>(Sample) * Sample;
	}
	const float Level = static_cast<float>(FMath::Sqrt(Sum / FMath::Max(Bed.Num(), 1)));
	const float Target = Kind == TEXT("night") ? 0.008f : 0.012f;
	const float Scale = Level > 1e-6f ? Target / Level : 1.f;
	for (float& Sample : Bed)
	{
		Sample *= Scale;
	}
	return Bed;
}

/** How many times the playing bed has been asked for more sound: proof, short of ears, that it is running. */
std::atomic<int32> AmbienceFeeds{0};

TWeakObjectPtr<UAudioComponent> AmbienceVoice;

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
	Tone.Sine(0.f, 0.018f, 2600.f, 1500.f, 0.52f);
	Tone.Sine(0.f, 0.05f, 620.f, 420.f, 0.3f);
	Tone.Noise(0.f, 0.008f, 7000.f, 0.3f, NextSeed());
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

void GGAudio::SwitchPress(UObject* WorldContext, const FVector& Location)
{
	// A stone plate sinking, then the gate answering.
	FTone Tone(0.8f);
	Tone.Noise(0.f, 0.03f, 900.f, 0.5f, NextSeed());
	Tone.Sine(0.f, 0.06f, 110.f, 70.f, 0.5f);
	Tone.Sine(0.1f, 0.4f, 392.f, 392.f, 0.18f);
	Tone.Sine(0.16f, 0.4f, 587.33f, 587.33f, 0.16f);
	Play(WorldContext, Location, Tone, 0.8f);
}

void GGAudio::GateShut(UObject* WorldContext, const FVector& Location)
{
	FTone Tone(0.5f);
	Tone.Sine(0.f, 0.2f, 587.33f, 196.f, 0.25f);
	Tone.Noise(0.12f, 0.03f, 600.f, 0.4f, NextSeed());
	Play(WorldContext, Location, Tone, 0.7f);
}

void GGAudio::Export(const FString& Folder)
{
	if (Folder == TEXT("status"))
	{
		UE_LOG(LogTemp, Display, TEXT("Ambience: fed %d times; voice %s"), AmbienceFeeds.load(),
			AmbienceVoice.IsValid() && AmbienceVoice->IsPlaying() ? TEXT("playing") : TEXT("not playing"));
		return;
	}
	// Every sound the game makes, written out as it would be played, so it can be measured (nobody building this
	// game can hear it).
	ExportFolder = Folder;
	auto One = [](const TCHAR* Name, TFunctionRef<void()> Make) {
		ExportName = Name;
		Make();
	};
	const FVector Here = FVector::ZeroVector;
	One(TEXT("footstep_grass"), [&]() { Footstep(nullptr, Here, GG::ESurface::Green); });
	One(TEXT("footstep_stone"), [&]() { Footstep(nullptr, Here, GG::ESurface::Stone); });
	One(TEXT("strike_soft"), [&]() { Strike(nullptr, Here, 0.15f); });
	One(TEXT("strike_full"), [&]() { Strike(nullptr, Here, 1.f); });
	One(TEXT("bounce_soft"), [&]() { Bounce(nullptr, Here, GG::ESurface::Sand, 500.f); });
	One(TEXT("bounce_stone"), [&]() { Bounce(nullptr, Here, GG::ESurface::Stone, 900.f); });
	One(TEXT("sunk"), [&]() { Sunk(nullptr, Here); });
	One(TEXT("machine_pickup"), [&]() { MachinePickUp(nullptr, Here); });
	One(TEXT("machine_place"), [&]() { MachinePlace(nullptr, Here); });
	One(TEXT("field_take"), [&]() { MachineField(nullptr, Here, false); });
	One(TEXT("field_release"), [&]() { MachineField(nullptr, Here, true); });
	One(TEXT("switch_press"), [&]() { SwitchPress(nullptr, Here); });
	One(TEXT("gate_shut"), [&]() { GateShut(nullptr, Here); });
	for (const TCHAR* Kind : {TEXT("wind"), TEXT("canyon"), TEXT("night"), TEXT("water"), TEXT("hum")})
	{
		WriteWav(Folder / (FString(TEXT("bed_")) + Kind + TEXT(".wav")), MakeBed(Kind), 1.f);
	}
	ExportFolder.Reset();
}

void GGAudio::Ambience(UObject* WorldContext, const FString& Kind)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World || Kind.IsEmpty() || Kind == TEXT("none"))
	{
		return;
	}
	if (UAudioComponent* Old = AmbienceVoice.Get())
	{
		Old->Stop();
	}
	// The bed is handed to the sound a piece at a time, round and round, for as long as it is asked for.
	const TArray<float> Bed = MakeBed(Kind);
	TSharedRef<TArray<int16>> Pcm = MakeShared<TArray<int16>>();
	Pcm->SetNumUninitialized(Bed.Num());
	for (int32 I = 0; I < Bed.Num(); ++I)
	{
		(*Pcm)[I] = static_cast<int16>(FMath::Clamp(Bed[I], -1.f, 1.f) * 32000.f);
	}
	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(GetTransientPackage());
	Wave->SetSampleRate(Rate);
	Wave->NumChannels = 1;
	Wave->Duration = INDEFINITELY_LOOPING_DURATION;
	Wave->SoundGroup = SOUNDGROUP_Effects;
	Wave->bLooping = true;
	int32 Position = 0;
	Wave->OnSoundWaveProceduralUnderflow = FOnSoundWaveProceduralUnderflow::CreateLambda(
		[Pcm, Position](USoundWaveProcedural* Sound, int32 SamplesNeeded) mutable {
			++AmbienceFeeds;
			int32 Left = FMath::Max(SamplesNeeded, 4096);
			while (Left > 0)
			{
				const int32 Count = FMath::Min(Left, Pcm->Num() - Position);
				Sound->QueueAudio(reinterpret_cast<const uint8*>(Pcm->GetData() + Position), Count * sizeof(int16));
				Position = (Position + Count) % Pcm->Num();
				Left -= Count;
			}
		});
	AmbienceVoice = UGameplayStatics::SpawnSound2D(World, Wave, UGGSave::Get().Volume);
	UE_LOG(LogTemp, Display, TEXT("Ambience: %s (%s)"), *Kind, AmbienceVoice.IsValid() ? TEXT("playing") : TEXT("no voice"));
}

void GGAudio::SetAmbienceVolume(float Volume)
{
	if (UAudioComponent* Voice = AmbienceVoice.Get())
	{
		Voice->SetVolumeMultiplier(FMath::Max(Volume, 0.001f));
	}
}
