// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Effects/AudioFXSourceGainReductionCapture.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(AudioFXSourceGainReductionCapture)

#include "SourceEffects/SourceEffectDynamicsProcessor.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Preset — SetLatestGainReductionDb
// ─────────────────────────────────────────────────────────────────────────────

void UAudioFXSourceGainReductionCapturePreset::SetLatestGainReductionDb(float InGainReductionDb)
{
	float Old = LatestGainReductionDb.load(std::memory_order_relaxed);
	while (InGainReductionDb > Old &&
	       !LatestGainReductionDb.compare_exchange_weak(Old, InGainReductionDb, std::memory_order_relaxed)) {}
}

// ─────────────────────────────────────────────────────────────────────────────
//  Private helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace AudioFXGainReductionCaptureSourcePrivate
{
// Maps the UE source-effect enum to the Audio DSP enum.
static Audio::EDynamicsProcessingMode::Type ToAudioDynamicsMode(const ESourceEffectDynamicsProcessorType InType)
{
	switch (InType)
	{
	default:
	case ESourceEffectDynamicsProcessorType::Compressor:        return Audio::EDynamicsProcessingMode::Compressor;
	case ESourceEffectDynamicsProcessorType::Limiter:           return Audio::EDynamicsProcessingMode::Limiter;
	case ESourceEffectDynamicsProcessorType::Expander:          return Audio::EDynamicsProcessingMode::Expander;
	case ESourceEffectDynamicsProcessorType::Gate:              return Audio::EDynamicsProcessingMode::Gate;
	case ESourceEffectDynamicsProcessorType::UpwardsCompressor: return Audio::EDynamicsProcessingMode::UpwardsCompressor;
	}
}

// Maps the UE source-effect peak-mode enum to the Audio DSP enum.
static Audio::EPeakMode::Type ToAudioPeakMode(const ESourceEffectDynamicsPeakMode InMode)
{
	switch (InMode)
	{
	default:
	case ESourceEffectDynamicsPeakMode::MeanSquared:     return Audio::EPeakMode::MeanSquared;
	case ESourceEffectDynamicsPeakMode::RootMeanSquared: return Audio::EPeakMode::RootMeanSquared;
	case ESourceEffectDynamicsPeakMode::Peak:            return Audio::EPeakMode::Peak;
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

using namespace AudioFXGainReductionCaptureSourcePrivate;

// ─────────────────────────────────────────────────────────────────────────────
//  FAudioFXSourceGainReductionCapture — DSP
// ─────────────────────────────────────────────────────────────────────────────

FAudioFXSourceGainReductionCapture::~FAudioFXSourceGainReductionCapture()
{
	if (UAudioFXSourceGainReductionCapturePreset* P =
		Cast<UAudioFXSourceGainReductionCapturePreset>(Preset.Get()))
	{
		P->UnregisterDSPInstance(this);
	}
}

void FAudioFXSourceGainReductionCapture::Init(const FSoundEffectSourceInitData& InInitData)
{
	DynamicsProcessor.Init(InInitData.SampleRate, InInitData.NumSourceChannels);
	GainReductionDb.store(0.0f, std::memory_order_relaxed);

	// Register with the preset for direct parameter updates.
	if (UAudioFXSourceGainReductionCapturePreset* P =
		Cast<UAudioFXSourceGainReductionCapturePreset>(Preset.Get()))
	{
		P->RegisterDSPInstance(this);

		// Apply settings immediately — SyncFromDynamicsPreset() called Update()
		// before this DSP instance existed, so OnPresetChanged() was lost.
		// Without this, the DynamicsProcessor runs with default (no-op) settings.
		ApplySettings(P->Settings.DynamicsSettings);
	}
}

void FAudioFXSourceGainReductionCapture::OnPresetChanged()
{
	GET_EFFECT_SETTINGS(AudioFXSourceGainReductionCapture);
	ApplySettings(Settings.DynamicsSettings);
}

void FAudioFXSourceGainReductionCapture::ProcessAudio(
	const FSoundEffectSourceInputData& InData, float* OutAudioBufferData)
{
	if (!InData.InputSourceEffectBufferPtr || !OutAudioBufferData || InData.NumSamples <= 0) { return; }

	EnvelopeBuffer.Reset();
	EnvelopeBuffer.AddZeroed(InData.NumSamples);

	DynamicsProcessor.ProcessAudio(
		InData.InputSourceEffectBufferPtr, InData.NumSamples, OutAudioBufferData, nullptr, EnvelopeBuffer.GetData());
	PublishGainReductionDb(AudioFXGainReductionCaptureSourcePrivate::ComputeGainReductionDb(EnvelopeBuffer));
}

void FAudioFXSourceGainReductionCapture::ApplySettings(
	const FSourceEffectDynamicsProcessorSettings& InSettings)
{
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
	DynamicsProcessor.SetChannelLinkMode(
		InSettings.bStereoLinked
		? Audio::EDynamicsProcessorChannelLinkMode::Average
		: Audio::EDynamicsProcessorChannelLinkMode::Disabled);
}

void FAudioFXSourceGainReductionCapture::PublishGainReductionDb(float InGainReductionDb)
{
	GainReductionDb.store(InGainReductionDb, std::memory_order_relaxed);
	if (UAudioFXSourceGainReductionCapturePreset* CapturePreset =
		Cast<UAudioFXSourceGainReductionCapturePreset>(Preset.Get()))
	{
		CapturePreset->SetLatestGainReductionDb(InGainReductionDb);
	}
}

// ============================================================
//  UAudioFXSourceGainReductionCapturePreset — UAsset
// ============================================================

void UAudioFXSourceGainReductionCapturePreset::SetSettings(
	const FAudioFXSourceGainReductionCaptureSettings& InSettings)
{
	Settings = InSettings;
	UpdateSettings(InSettings);
}

void UAudioFXSourceGainReductionCapturePreset::RegisterDSPInstance(FAudioFXSourceGainReductionCapture* InDSP)
{
	FScopeLock Lock(&DSPCritSect);
	LiveDSP = InDSP;
}

void UAudioFXSourceGainReductionCapturePreset::UnregisterDSPInstance(FAudioFXSourceGainReductionCapture* InDSP)
{
	FScopeLock Lock(&DSPCritSect);
	if (LiveDSP == InDSP) { LiveDSP = nullptr; }
}

void UAudioFXSourceGainReductionCapturePreset::SyncFromDynamicsPreset(
	const USourceEffectDynamicsProcessorPreset* InPreset)
{
	if (!InPreset) { return; }
	FAudioFXSourceGainReductionCaptureSettings NewSettings;
	NewSettings.DynamicsSettings = InPreset->Settings;

	// 1. Update preset settings so OnPresetChanged() is also consistent.
	SetSettings(NewSettings);

	// 2. Directly push to the live DSP — bypasses UE's subscription chain.
	FScopeLock Lock(&DSPCritSect);
	if (LiveDSP)
	{
		LiveDSP->ApplySettings(NewSettings.DynamicsSettings);
	}
}
