// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundEffectSource.h"
#include "Sound/SoundEffectPreset.h"
#include "DSP/EnvelopeFollower.h"
#include <atomic> // std::atomic<float>: no UE CAS equivalent for float
#include "AudioFXSourceMeterCapture.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Settings
// ─────────────────────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct FAudioFXSourceMeterCaptureSettings
{
	GENERATED_BODY()
};

// ─────────────────────────────────────────────────────────────────────────────
//  DSP Wrapper  (one per active preset, lives on the audio thread)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Wraps one source effect and measures input/output levels.
 * Peak uses a fast-attack follower (1 ms / 300 ms). RMS uses a slow-release
 * follower (5 ms / 1500 ms) for a stable background bar.
 */
class AUDIOEFFECTSSTUDIO_API FAudioFXSourceMeterWrapper : public FSoundEffectSource
{
public:

	using FSourceEffectPtr = TSharedPtr<FSoundEffectSource, ESPMode::ThreadSafe>;

	virtual ~FAudioFXSourceMeterWrapper() override = default;

	void             SetInnerEffect(FSourceEffectPtr In) { InnerEffect = MoveTemp(In); }
	FSourceEffectPtr GetInnerEffect() const              { return InnerEffect; }

	virtual void ProcessAudio(const FSoundEffectSourceInputData& InData, float* OutAudioBufferData) override;

private:

	virtual void Init(const FSoundEffectSourceInitData& InInitData) override;

	struct FLevels { float PeakL, PeakR, RmsL, RmsR; };

	FLevels MeasureBuffer(const float* Buf, int32 NumFrames, int32 NumChannels,
	                      Audio::FEnvelopeFollower& PeakL, Audio::FEnvelopeFollower& PeakR,
	                      Audio::FEnvelopeFollower& RmsL,  Audio::FEnvelopeFollower& RmsR);

	FSourceEffectPtr InnerEffect;
	int32            NumChannels = 0;

	// Input followers
	Audio::FEnvelopeFollower InPeakFollowerL,  InPeakFollowerR;
	Audio::FEnvelopeFollower InRmsL,           InRmsR;

	// Output followers
	Audio::FEnvelopeFollower OutPeakFollowerL, OutPeakFollowerR;
	Audio::FEnvelopeFollower OutRmsL,          OutRmsR;

	TArray<float> MonoScratch;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Preset  (transient UObject, lives for the duration of one preview session)
// ─────────────────────────────────────────────────────────────────────────────

UCLASS(Transient, MinimalAPI)
class UAudioFXSourceMeterCapturePreset : public USoundEffectSourcePreset
{
	GENERATED_BODY()

public:

	// ── UE preset boilerplate ─────────────────────────────────────────────
	virtual bool        CanFilter()       const override { return false; }
	virtual bool        HasAssetActions() const override { return true; }
	virtual FText       GetAssetActionName() const override;
	virtual UClass*     GetSupportedClass() const override;
	virtual FSoundEffectBase*   CreateNewEffect()  const override;
	virtual USoundEffectPreset* CreateNewPreset(UObject* InParent, FName Name, EObjectFlags Flags) const override;
	virtual void Init() override;

	UFUNCTION(BlueprintCallable, Category = "AudioFX|Meter")
	void SetSettings(const FAudioFXSourceMeterCaptureSettings& InSettings);

	void UpdateSettings(const FAudioFXSourceMeterCaptureSettings& InSettings);

	UPROPERTY(BlueprintReadOnly, Category = "SourceEffectPreset", meta = (ShowOnlyInnerProperties))
	FAudioFXSourceMeterCaptureSettings Settings;

	// ── Input level reads (pre-effect) ────────────────────────────────────
	float GetInputPeakL() const { return InPeakL.load(std::memory_order_relaxed); }
	float GetInputPeakR() const { return InPeakR.load(std::memory_order_relaxed); }
	float GetInputRmsL()  const { return InRmsL.load(std::memory_order_relaxed); }
	float GetInputRmsR()  const { return InRmsR.load(std::memory_order_relaxed); }

	// ── Output level reads (post-effect) ──────────────────────────────────
	float GetPeakL() const { return OutPeakL.load(std::memory_order_relaxed); }
	float GetPeakR() const { return OutPeakR.load(std::memory_order_relaxed); }
	float GetRmsL()  const { return OutRmsL.load(std::memory_order_relaxed); }
	float GetRmsR()  const { return OutRmsR.load(std::memory_order_relaxed); }

	// ── Exchange reads — atomic read-and-reset for Slate Tick ─────────────
	// Returns the highest value written since the last call, then resets to 0.
	// Prevents audio-to-Slate aliasing where transients are overwritten before Tick reads them.
	float ExchangeOutputPeakL() const { return OutPeakL.exchange(0.f, std::memory_order_relaxed); }
	float ExchangeOutputPeakR() const { return OutPeakR.exchange(0.f, std::memory_order_relaxed); }
	float ExchangeInputPeakL()  const { return InPeakL.exchange(0.f,  std::memory_order_relaxed); }
	float ExchangeInputPeakR()  const { return InPeakR.exchange(0.f,  std::memory_order_relaxed); }

	// ── Audio-thread writes (called from FAudioFXSourceMeterWrapper) ───────
	void SetInputLevels(float PeakL, float PeakR, float RmsL, float RmsR);
	void SetLatestLevels(float PeakL, float PeakR, float RmsL, float RmsR);

	void SetPendingInnerPreset(USoundEffectSourcePreset* InPreset) { PendingInnerPreset = InPreset; }
	USoundEffectSourcePreset* GetPendingInnerPreset() const        { return PendingInnerPreset; }

private:

	// CAS-based max: stores the larger of current and NewVal atomically.
	static void AtomicMax(std::atomic<float>& Target, float NewVal);

	USoundEffectSourcePreset* PendingInnerPreset = nullptr;

	FCriticalSection SettingsCritSect;
	FAudioFXSourceMeterCaptureSettings SettingsCopy;

	mutable std::atomic<float> InPeakL{0.f},  InPeakR{0.f},  InRmsL{0.f},  InRmsR{0.f};
	mutable std::atomic<float> OutPeakL{0.f}, OutPeakR{0.f}, OutRmsL{0.f}, OutRmsR{0.f};
};
