// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Effects/AudioFXSubmixGainReductionCapture.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(AudioFXSubmixGainReductionCapture)

#include "SubmixEffects/AudioMixerSubmixEffectDynamicsProcessor.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Preset boilerplate
// ─────────────────────────────────────────────────────────────────────────────

FText UAudioFXSubmixGainReductionCapturePreset::GetAssetActionName() const
{
	return FText::FromString(TEXT("AudioFXGainReductionCapture"));
}

UClass* UAudioFXSubmixGainReductionCapturePreset::GetSupportedClass() const
{
	return UAudioFXSubmixGainReductionCapturePreset::StaticClass();
}

FSoundEffectBase* UAudioFXSubmixGainReductionCapturePreset::CreateNewEffect() const
{
	return new FAudioFXGainReductionCapture;
}

USoundEffectPreset* UAudioFXSubmixGainReductionCapturePreset::CreateNewPreset(
	UObject* InParent, FName Name, EObjectFlags Flags) const
{
	USoundEffectPreset* NewPreset = NewObject<UAudioFXSubmixGainReductionCapturePreset>(
		InParent, GetSupportedClass(), Name, Flags);
	NewPreset->Init();
	return NewPreset;
}

void UAudioFXSubmixGainReductionCapturePreset::Init()
{
	FScopeLock ScopeLock(&SettingsCritSect);
	SettingsCopy = Settings;
}

void UAudioFXSubmixGainReductionCapturePreset::UpdateSettings(
	const FAudioFXSubmixGainReductionCaptureSettings& InSettings)
{
	FScopeLock ScopeLock(&SettingsCritSect);
	SettingsCopy = InSettings;
	Update();
}

FAudioFXSubmixGainReductionCaptureSettings UAudioFXSubmixGainReductionCapturePreset::GetSettings()
{
	FScopeLock ScopeLock(&SettingsCritSect);
	return SettingsCopy;
}

void UAudioFXSubmixGainReductionCapturePreset::SetLatestGainReductionDb(float InGainReductionDb)
{
	float Old = LatestGainReductionDb.load(std::memory_order_relaxed);
	while (InGainReductionDb > Old &&
	       !LatestGainReductionDb.compare_exchange_weak(Old, InGainReductionDb, std::memory_order_relaxed)) {}
}

// ─────────────────────────────────────────────────────────────────────────────
//  Private helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace AudioFXGainReductionCaptureSubmixPrivate
{
// Maps the UE submix-effect enum to the Audio DSP enum.
static Audio::EDynamicsProcessingMode::Type ToAudioDynamicsMode(const ESubmixEffectDynamicsProcessorType InType)
{
	switch (InType)
	{
	default:
	case ESubmixEffectDynamicsProcessorType::Compressor:        return Audio::EDynamicsProcessingMode::Compressor;
	case ESubmixEffectDynamicsProcessorType::Limiter:           return Audio::EDynamicsProcessingMode::Limiter;
	case ESubmixEffectDynamicsProcessorType::Expander:          return Audio::EDynamicsProcessingMode::Expander;
	case ESubmixEffectDynamicsProcessorType::Gate:              return Audio::EDynamicsProcessingMode::Gate;
	case ESubmixEffectDynamicsProcessorType::UpwardsCompressor: return Audio::EDynamicsProcessingMode::UpwardsCompressor;
	}
}

// Maps the UE submix-effect peak-mode enum to the Audio DSP enum.
static Audio::EPeakMode::Type ToAudioPeakMode(const ESubmixEffectDynamicsPeakMode InMode)
{
	switch (InMode)
	{
	default:
	case ESubmixEffectDynamicsPeakMode::MeanSquared:     return Audio::EPeakMode::MeanSquared;
	case ESubmixEffectDynamicsPeakMode::RootMeanSquared: return Audio::EPeakMode::RootMeanSquared;
	case ESubmixEffectDynamicsPeakMode::Peak:            return Audio::EPeakMode::Peak;
	}
}

// Converts the per-sample gain envelope to a single peak gain-reduction value in dB.
// Returns 0 dB for an empty buffer and clamps to -60 dB for near-zero gain.
static float ComputeGainReductionDb(const Audio::FAlignedFloatBuffer& InEnvelope)
{
	if (InEnvelope.Num() == 0) { return 0.0f; }
	float MinGain = 1.0f;
	for (const float Gain : InEnvelope) { MinGain = FMath::Min(MinGain, Gain); }
	if (MinGain <= KINDA_SMALL_NUMBER) { return -60.0f; }
	return 20.0f * FMath::LogX(10.0f, MinGain);
}
} // namespace

using namespace AudioFXGainReductionCaptureSubmixPrivate;

// ─────────────────────────────────────────────────────────────────────────────
//  FAudioFXGainReductionCapture — DSP
// ─────────────────────────────────────────────────────────────────────────────

FAudioFXGainReductionCapture::~FAudioFXGainReductionCapture()
{
	// Unregister so the preset never holds a dangling raw pointer.
	if (UAudioFXSubmixGainReductionCapturePreset* P =
		Cast<UAudioFXSubmixGainReductionCapturePreset>(Preset.Get()))
	{
		P->UnregisterDSPInstance(this);
	}
}

void FAudioFXGainReductionCapture::Init(const FSoundEffectSubmixInitData& InInitData)
{
	DynamicsProcessor.Init(InInitData.SampleRate, 2);
	GainReductionDb.store(0.0f, std::memory_order_relaxed);

	// Register with the preset so SyncFromDynamicsPreset can push directly to us.
	if (UAudioFXSubmixGainReductionCapturePreset* P =
		Cast<UAudioFXSubmixGainReductionCapturePreset>(Preset.Get()))
	{
		P->RegisterDSPInstance(this);

		// Apply settings immediately — SyncFromDynamicsPreset() called Update()
		// before this DSP instance existed, so OnPresetChanged() was lost.
		// Without this, the DynamicsProcessor runs with default (no-op) settings.
		ApplySettings(P->GetSettings().DynamicsSettings);
	}
}

void FAudioFXGainReductionCapture::OnPresetChanged()
{
	UAudioFXSubmixGainReductionCapturePreset* EffectPreset =
		Cast<UAudioFXSubmixGainReductionCapturePreset>(Preset.Get());
	if (!EffectPreset) { return; }
	// Use the thread-safe copy, not the game-thread UPROPERTY .Settings directly.
	ApplySettings(EffectPreset->GetSettings().DynamicsSettings);
}

void FAudioFXGainReductionCapture::OnProcessAudio(
	const FSoundEffectSubmixInputData& InData, FSoundEffectSubmixOutputData& OutData)
{
	const int32 NumSamples = InData.NumFrames * InData.NumChannels;
	if (NumSamples <= 0 || !InData.AudioBuffer || !OutData.AudioBuffer) { return; }

	// Re-initialise channel count if the submix topology changed since last block.
	if (InData.NumChannels != DynamicsProcessor.GetNumChannels())
	{
		DynamicsProcessor.SetNumChannels(InData.NumChannels);
		DynamicsProcessor.SetKeyNumChannels(InData.NumChannels);
	}

	EnvelopeBuffer.Reset();
	EnvelopeBuffer.AddZeroed(NumSamples);

	if (bBypass)
	{
		FMemory::Memcpy(OutData.AudioBuffer->GetData(), InData.AudioBuffer->GetData(), NumSamples * sizeof(float));
		PublishGainReductionDb(0.0f);
		return;
	}

	DynamicsProcessor.ProcessAudio(
		InData.AudioBuffer->GetData(), NumSamples, OutData.AudioBuffer->GetData(), nullptr, EnvelopeBuffer.GetData());
	PublishGainReductionDb(AudioFXGainReductionCaptureSubmixPrivate::ComputeGainReductionDb(EnvelopeBuffer));
}

void FAudioFXGainReductionCapture::ApplySettings(const FSubmixEffectDynamicsProcessorSettings& InSettings)
{
	bBypass = InSettings.bBypass;
	DynamicsProcessor.SetProcessingMode(ToAudioDynamicsMode(InSettings.DynamicsProcessorType));
	DynamicsProcessor.SetPeakMode(ToAudioPeakMode(InSettings.PeakMode));
	DynamicsProcessor.SetLookaheadMsec(InSettings.LookAheadMsec);
	DynamicsProcessor.SetAttackTime(InSettings.AttackTimeMsec);
	DynamicsProcessor.SetReleaseTime(InSettings.ReleaseTimeMsec);
	DynamicsProcessor.SetThreshold(InSettings.ThresholdDb);
	DynamicsProcessor.SetRatio(InSettings.Ratio);
	DynamicsProcessor.SetKneeBandwidth(InSettings.KneeBandwidthDb);
	DynamicsProcessor.SetInputGain(InSettings.InputGainDb);
	DynamicsProcessor.SetOutputGain(InSettings.OutputGainDb);
	DynamicsProcessor.SetAnalogMode(InSettings.bAnalogMode);

	// Key-sidechain controls — audition, gain, and shelf EQ filters.
	DynamicsProcessor.SetKeyAudition(InSettings.bKeyAudition);
	DynamicsProcessor.SetKeyGain(InSettings.KeyGainDb);
	DynamicsProcessor.SetKeyHighshelfCutoffFrequency(InSettings.KeyHighshelf.Cutoff);
	DynamicsProcessor.SetKeyHighshelfEnabled(InSettings.KeyHighshelf.bEnabled);
	DynamicsProcessor.SetKeyHighshelfGain(InSettings.KeyHighshelf.GainDb);
	DynamicsProcessor.SetKeyLowshelfCutoffFrequency(InSettings.KeyLowshelf.Cutoff);
	DynamicsProcessor.SetKeyLowshelfEnabled(InSettings.KeyLowshelf.bEnabled);
	DynamicsProcessor.SetKeyLowshelfGain(InSettings.KeyLowshelf.GainDb);
	DynamicsProcessor.SetChannelLinkMode(
		static_cast<Audio::EDynamicsProcessorChannelLinkMode>(InSettings.LinkMode));
}

void FAudioFXGainReductionCapture::PublishGainReductionDb(float InGainReductionDb)
{
	GainReductionDb.store(InGainReductionDb, std::memory_order_relaxed);
	if (UAudioFXSubmixGainReductionCapturePreset* CapturePreset =
		Cast<UAudioFXSubmixGainReductionCapturePreset>(Preset.Get()))
	{
		CapturePreset->SetLatestGainReductionDb(InGainReductionDb);
	}
}

// ============================================================
//  UAudioFXSubmixGainReductionCapturePreset — UAsset
// ============================================================

void UAudioFXSubmixGainReductionCapturePreset::SetSettings(
	const FAudioFXSubmixGainReductionCaptureSettings& InSettings)
{
	Settings = InSettings;
	UpdateSettings(InSettings);
}

void UAudioFXSubmixGainReductionCapturePreset::RegisterDSPInstance(FAudioFXGainReductionCapture* InDSP)
{
	FScopeLock Lock(&DSPCritSect);
	LiveDSP = InDSP;
}

void UAudioFXSubmixGainReductionCapturePreset::UnregisterDSPInstance(FAudioFXGainReductionCapture* InDSP)
{
	FScopeLock Lock(&DSPCritSect);
	if (LiveDSP == InDSP) { LiveDSP = nullptr; }
}

void UAudioFXSubmixGainReductionCapturePreset::SyncFromDynamicsPreset(
	const USubmixEffectDynamicsProcessorPreset* InPreset)
{
	if (!InPreset) { return; }
	FAudioFXSubmixGainReductionCaptureSettings NewSettings;
	NewSettings.DynamicsSettings = InPreset->Settings;

	// 1. Update the preset's thread-safe copy so OnPresetChanged() reads it correctly.
	SetSettings(NewSettings);

	// 2. Directly push to the live DSP instance — bypasses UE's preset subscription
	//    chain which may not reliably deliver updates to a freshly swapped-in
	//    transient preset. This is the authoritative parameter update path.
	FScopeLock Lock(&DSPCritSect);
	if (LiveDSP)
	{
		LiveDSP->ApplySettings(NewSettings.DynamicsSettings);
	}
}
