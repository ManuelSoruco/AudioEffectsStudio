// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundEffectSource.h"
#include "SourceEffects/SourceEffectDynamicsProcessor.h"
#include <atomic> // std::atomic: no UE CAS equivalent for float

#include "AudioFXSourceGainReductionCapture.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Settings
// ─────────────────────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct FAudioFXSourceGainReductionCaptureSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AudioFX")
	FSourceEffectDynamicsProcessorSettings DynamicsSettings;
};

// ─────────────────────────────────────────────────────────────────────────────
//  DSP Processor  (lives on the audio thread)
// ─────────────────────────────────────────────────────────────────────────────

class AUDIOEFFECTSSTUDIO_API FAudioFXSourceGainReductionCapture : public FSoundEffectSource
{
public:

	virtual ~FAudioFXSourceGainReductionCapture() override;

	virtual void Init(const FSoundEffectSourceInitData& InInitData) override;
	virtual void OnPresetChanged() override;
	virtual void ProcessAudio(const FSoundEffectSourceInputData& InData, float* OutAudioBufferData) override;

	/** Peak gain reduction (dB) from the last audio block. Thread-safe read. */
	float GetGainReductionDb() const { return GainReductionDb.load(std::memory_order_relaxed); }

	/** Public so UAudioFXSourceGainReductionCapturePreset::SyncFromDynamicsPreset can call directly. */
	void ApplySettings(const FSourceEffectDynamicsProcessorSettings& InSettings);

private:

	void PublishGainReductionDb(float InGainReductionDb);

	Audio::FDynamicsProcessor  DynamicsProcessor;
	Audio::FAlignedFloatBuffer EnvelopeBuffer;

	// std::atomic: no UE CAS equivalent for float
	std::atomic<float> GainReductionDb{ 0.0f };
};

// ...existing code...

UCLASS(Transient, MinimalAPI)
class UAudioFXSourceGainReductionCapturePreset : public USoundEffectSourcePreset
{
	GENERATED_BODY()

public:

	EFFECT_PRESET_METHODS(AudioFXSourceGainReductionCapture)

	UFUNCTION(BlueprintCallable, Category = "AudioFX|GainReduction")
	void SetSettings(const FAudioFXSourceGainReductionCaptureSettings& InSettings);

	void SyncFromDynamicsPreset(const USourceEffectDynamicsProcessorPreset* InPreset);

	void RegisterDSPInstance(FAudioFXSourceGainReductionCapture* InDSP);
	void UnregisterDSPInstance(FAudioFXSourceGainReductionCapture* InDSP);

	UPROPERTY(BlueprintReadOnly, Category = "SourceEffectPreset", meta = (ShowOnlyInnerProperties))
	FAudioFXSourceGainReductionCaptureSettings Settings;

	UPROPERTY(VisibleAnywhere, Category = "SourceEffectPreset", meta = (ShowOnlyInnerProperties))
	FString Note = "This effect is managed internally by Audio Effects Studio. Do not add it to effect chains manually -- it is created and removed automatically by the plugin's dynamics editors.";

	// ── GR level reads ────────────────────────────────────────────────────
	float GetGainReductionDb() const { return LatestGainReductionDb.load(std::memory_order_relaxed); }
	float ExchangeGainReductionDb() const { return LatestGainReductionDb.exchange(0.f, std::memory_order_relaxed); }
	void SetLatestGainReductionDb(float InGainReductionDb);

private:

	FCriticalSection DSPCritSect;
	FAudioFXSourceGainReductionCapture* LiveDSP = nullptr;

	mutable std::atomic<float> LatestGainReductionDb{ 0.0f };
};
