// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundEffectSubmix.h"
#include "SubmixEffects/AudioMixerSubmixEffectDynamicsProcessor.h"
#include <atomic> // std::atomic: no UE CAS equivalent for float
#include "AudioFXSubmixGainReductionCapture.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Settings
// ─────────────────────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct FAudioFXSubmixGainReductionCaptureSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AudioFX")
	FSubmixEffectDynamicsProcessorSettings DynamicsSettings;
};

// ─────────────────────────────────────────────────────────────────────────────
//  DSP Processor  (lives on the audio thread)
// ─────────────────────────────────────────────────────────────────────────────

class AUDIOEFFECTSSTUDIO_API FAudioFXGainReductionCapture : public FSoundEffectSubmix
{
public:

	virtual ~FAudioFXGainReductionCapture() override;

	virtual void Init(const FSoundEffectSubmixInitData& InInitData) override;
	virtual void OnPresetChanged() override;
	virtual void OnProcessAudio(const FSoundEffectSubmixInputData& InData,
	                            FSoundEffectSubmixOutputData& OutData) override;

	/** Peak gain reduction (dB) from the last audio block. Thread-safe read. */
	float GetGainReductionDb() const { return GainReductionDb.load(std::memory_order_relaxed); }

	/**
	 * Directly apply dynamics settings to the running DSP processor.
	 * Called from UAudioFXSubmixGainReductionCapturePreset::SyncFromDynamicsPreset()
	 * under DSPCritSect to ensure knob changes reach the audio thread immediately.
	 * Must only be called while DSPCritSect is held by the caller.
	 */
	void ApplySettings(const FSubmixEffectDynamicsProcessorSettings& InSettings);
	void PublishGainReductionDb(float InGainReductionDb);

	Audio::FDynamicsProcessor   DynamicsProcessor;
	Audio::FAlignedFloatBuffer  EnvelopeBuffer;
	bool                        bBypass = false;

	// std::atomic: no UE CAS equivalent for float
	std::atomic<float> GainReductionDb{ 0.0f };
};

// ─────────────────────────────────────────────────────────────────────────────
//  Preset  (transient UObject, lives for the duration of one preview session)
// ─────────────────────────────────────────────────────────────────────────────

UCLASS(Transient, MinimalAPI)
class UAudioFXSubmixGainReductionCapturePreset : public USoundEffectSubmixPreset
{
	GENERATED_BODY()

public:

	// ── UE preset boilerplate ─────────────────────────────────────────────
	// Manually expanded — macro would look for class name matching the DSP class,
	// but our DSP class (FAudioFXGainReductionCapture) no longer matches the preset name.
	virtual bool        CanFilter()       const override { return false; }
	virtual bool        HasAssetActions() const override { return true; }
	virtual FText       GetAssetActionName() const override;
	virtual UClass*     GetSupportedClass() const override;
	virtual FSoundEffectBase*   CreateNewEffect()  const override;
	virtual USoundEffectPreset* CreateNewPreset(UObject* InParent, FName Name, EObjectFlags Flags) const override;
	virtual void Init() override;

	UFUNCTION(BlueprintCallable, Category = "AudioFX|GainReduction")
	void SetSettings(const FAudioFXSubmixGainReductionCaptureSettings& InSettings);

	void UpdateSettings(const FAudioFXSubmixGainReductionCaptureSettings& InSettings);
	FAudioFXSubmixGainReductionCaptureSettings GetSettings();

	void SyncFromDynamicsPreset(const USubmixEffectDynamicsProcessorPreset* InPreset);

	/**
	 * Called by FAudioFXGainReductionCapture::Init() to register itself so that
	 * SyncFromDynamicsPreset can directly push settings to the running DSP
	 * without relying on UE's preset-subscription chain (which may not deliver
	 * updates to a freshly swapped-in transient preset).
	 */
	void RegisterDSPInstance(FAudioFXGainReductionCapture* InDSP);
	void UnregisterDSPInstance(FAudioFXGainReductionCapture* InDSP);

	UPROPERTY(BlueprintReadOnly, Category = "SubmixEffectPreset", meta = (ShowOnlyInnerProperties))
	FAudioFXSubmixGainReductionCaptureSettings Settings;

	UPROPERTY(VisibleAnywhere, Category = "SubmixEffectPreset", meta = (ShowOnlyInnerProperties))
	FString Note = "This effect is managed internally by Audio Effects Studio. Do not add it to effect chains manually -- it is created and removed automatically by the plugin's dynamics editors.";

	// ── GR level reads ────────────────────────────────────────────────────
	/** Most recent gain-reduction value (dB). Use ExchangeGainReductionDb() from Slate Tick. */
	float GetGainReductionDb() const { return LatestGainReductionDb.load(std::memory_order_relaxed); }

	/** Atomically read-and-reset to 0. Returns the highest GR since the last call. */
	float ExchangeGainReductionDb() const { return LatestGainReductionDb.exchange(0.f, std::memory_order_relaxed); }

	/** CAS-max write — called by the DSP processor on the audio thread. */
	void SetLatestGainReductionDb(float InGainReductionDb);

private:

	FCriticalSection SettingsCritSect;
	FAudioFXSubmixGainReductionCaptureSettings SettingsCopy;

	// Raw pointer — only accessed under DSPCritSect. The DSP unregisters itself
	// in its destructor so this never dangles.
	FCriticalSection DSPCritSect;
	FAudioFXGainReductionCapture* LiveDSP = nullptr;

	// std::atomic: no UE CAS equivalent for float
	mutable std::atomic<float> LatestGainReductionDb{ 0.0f };
};
