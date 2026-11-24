// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

/**
 * @file AudioFXMeterGuard.cpp
 * @brief RAII guard that injects transient input/output meter presets around
 *        the target effect slot for live level metering during preview.
 *
 * Source-effect path:
 *   Replaces the target slot in the SoundEffectSourcePresetChain with a
 *   UAudioFXSourceMeterCapturePreset wrapper that routes signal through
 *   the original preset while capturing peak/RMS levels.
 *
 * Submix-effect path:
 *   Uses SetSubmixEffectChainOverride() to temporarily replace the submix
 *   effect chain with a UAudioFXSubmixMeterCapturePreset wrapper.
 *
 * Must be constructed AFTER FAudioFXGRSwapGuard so it reads the correct
 * slot index (which GR swap may have shifted).
 */

#include "Shared/Core/AudioFXMeterGuard.h"
#include "Shared/Core/AudioFXGRSwapGuard.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundSubmix.h"
#include "Sound/SoundEffectSource.h"
#include "Sound/SoundEffectPreset.h"
#include "AudioDevice.h"
#include "Engine/Engine.h"
#include "UObject/UObjectGlobals.h"
#include "Shared/Core/AudioFXTheme.h" // LogAudioEffectsStudio

// ─────────────────────────────────────────────────────────────────────────────
//  Construction / Destruction
// ─────────────────────────────────────────────────────────────────────────────

FAudioFXMeterGuard::FAudioFXMeterGuard(
	bool bInIsSourceEffect,
	USoundEffectPreset* InPreset,
	USoundWave* InSound,
	USoundSubmix* InActiveSubmix,
	FAudioFXGRSwapGuard* InGRSwap)
	: bIsSourceEffect(bInIsSourceEffect)
{
	if (bIsSourceEffect)
	{
		InstallSourceMeters(InPreset, InSound, InGRSwap);
	}
	else
	{
		InstallSubmixMeters(InPreset, InActiveSubmix, InGRSwap);
	}
}

FAudioFXMeterGuard::~FAudioFXMeterGuard()
{
	if (bIsSourceEffect) { RemoveSourceMeters(); }
	else                 { RemoveSubmixMeters(); }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Source — Install
// ─────────────────────────────────────────────────────────────────────────────

void FAudioFXMeterGuard::InstallSourceMeters(
	USoundEffectPreset* InPreset,
	USoundWave* InSound,
	FAudioFXGRSwapGuard* InGRSwap)
{
	USoundEffectSourcePresetChain* Chain = InSound ? InSound->SourceEffectChain : nullptr;

	UE_LOG(LogAudioEffectsStudio, Warning,
		TEXT("InstallSourceMeters: InPreset=%p ('%s'), InSound=%p ('%s'), Chain=%p, GRSwap=%p (slot=%d)"),
		InPreset, InPreset ? *InPreset->GetName() : TEXT("null"),
		InSound, InSound ? *InSound->GetName() : TEXT("null"),
		Chain,
		InGRSwap, InGRSwap ? InGRSwap->GetSourceSlotIndex() : -1);

	if (!Chain)
	{
		UE_LOG(LogAudioEffectsStudio, Error,
			TEXT("InstallSourceMeters: FAILED — Sound '%s' has no SourceEffectChain!"),
			InSound ? *InSound->GetName() : TEXT("null"));
		return;
	}

	UE_LOG(LogAudioEffectsStudio, Warning,
		TEXT("InstallSourceMeters: Chain '%s' has %d entries:"), *Chain->GetName(), Chain->Chain.Num());
	for (int32 i = 0; i < Chain->Chain.Num(); ++i)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("  [%d] Preset=%p ('%s') bBypass=%d match=%d"),
			i,
			Chain->Chain[i].Preset.Get(),
			Chain->Chain[i].Preset ? *Chain->Chain[i].Preset->GetName() : TEXT("null"),
			Chain->Chain[i].bBypass,
			Chain->Chain[i].Preset == InPreset ? 1 : 0);
	}

	int32 TargetSlot = INDEX_NONE;
	if (InGRSwap && InGRSwap->GetSourceSlotIndex() != INDEX_NONE)
	{
		TargetSlot = InGRSwap->GetSourceSlotIndex();
	}
	else
	{
		for (int32 i = 0; i < Chain->Chain.Num(); ++i)
		{
			if (Chain->Chain[i].Preset == InPreset) { TargetSlot = i; break; }
		}
	}

	if (TargetSlot == INDEX_NONE)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("MeterGuard: could not find target slot in source chain '%s'"), *Chain->GetName());
		return;
	}

	USoundEffectSourcePreset* SourcePreset = Cast<USoundEffectSourcePreset>(InPreset);
	if (!SourcePreset) { return; }

	UAudioFXSourceMeterCapturePreset* WrapperPreset = NewObject<UAudioFXSourceMeterCapturePreset>(
		GetTransientPackage(), NAME_None, RF_Transient);
	if (!WrapperPreset) { return; }

	// The wrapper's DSP::Init() will call GetPendingInnerPreset() to create and init
	// the real inner effect automatically when UE instantiates the chain at play-time.
	WrapperPreset->SetPendingInnerPreset(SourcePreset);

	SourceWrapper        = TStrongObjectPtr<UAudioFXSourceMeterCapturePreset>(WrapperPreset);
	SourceChainWeak      = Chain;
	SourceWrapperSlot    = TargetSlot;
	OriginalSourcePreset = SourcePreset;

	// Replace the target slot in-place (no insert = no index shifting needed)
	Chain->Chain[TargetSlot].Preset  = WrapperPreset;
	Chain->Chain[TargetSlot].bBypass = false;

	UE_LOG(LogAudioEffectsStudio, Verbose,
		TEXT("MeterGuard: source wrapper installed at slot %d on '%s'"),
		TargetSlot, *Chain->GetName());
}// ─────────────────────────────────────────────────────────────────────────────
//  Submix — Install  (SetSubmixEffectChainOverride approach)
// ─────────────────────────────────────────────────────────────────────────────

void FAudioFXMeterGuard::InstallSubmixMeters(
	USoundEffectPreset* InPreset,
	USoundSubmix* InActiveSubmix,
	FAudioFXGRSwapGuard* /*InGRSwap*/)
{
	UE_LOG(LogAudioEffectsStudio, Warning,
		TEXT("InstallSubmixMeters: InPreset=%p ('%s'), InActiveSubmix=%p ('%s')"),
		InPreset, InPreset ? *InPreset->GetName() : TEXT("null"),
		InActiveSubmix, InActiveSubmix ? *InActiveSubmix->GetName() : TEXT("null"));

	if (!InActiveSubmix || !InPreset || !GEngine) { return; }

	FAudioDevice* AudioDevice = GEngine->GetMainAudioDevice().GetAudioDevice();
	if (!AudioDevice)
	{
		UE_LOG(LogAudioEffectsStudio, Error, TEXT("InstallSubmixMeters: no AudioDevice"));
		return;
	}

	// ── Build the override chain ──────────────────────────────────────────────
	// Walk the asset's SubmixEffectChain. For every non-null entry:
	//   - If it's the target preset  -> create our MeterWrapper DSP (wrapping the real effect)
	//   - Otherwise                  -> create a fresh DSP instance from the existing preset
	// Null entries are skipped (they don't exist in the live chain either).

	const FSoundEffectSubmixInitData InitData = [&]() {
		FSoundEffectSubmixInitData D;
		D.SampleRate           = AudioDevice->GetSampleRate();
		D.DeviceID             = AudioDevice->DeviceID;
		D.ParentPresetUniqueId = 0; // will be set per-effect below
		return D;
	}();

	// Create the transient wrapper preset (holds level readouts)
	UAudioFXSubmixMeterCapturePreset* WrapperPreset = NewObject<UAudioFXSubmixMeterCapturePreset>(
		GetTransientPackage(), NAME_None, RF_Transient);
	if (!WrapperPreset) { return; }

	SubmixWrapper = TStrongObjectPtr<UAudioFXSubmixMeterCapturePreset>(WrapperPreset);
	SubmixWeak    = InActiveSubmix;

	TArray<FSoundEffectSubmixPtr> OverrideChain;
	bool bFoundTarget = false;

	for (USoundEffectSubmixPreset* ChainEntry : InActiveSubmix->SubmixEffectChain)
	{
		if (!ChainEntry) { continue; } // skip nulls

		if (ChainEntry == InPreset)
		{
			// ── Wrapper for the target effect ──────────────────────────────────
			// Step 1: create the inner effect DSP via CreateInstance — this sets
			//         InnerDSP->Preset = InPreset correctly so param changes propagate.
			FSoundEffectSubmixInitData InnerInit = InitData;
			InnerInit.ParentPresetUniqueId = InPreset->GetUniqueID();
			FSoundEffectSubmixPtr InnerPtr =
				USoundEffectPreset::CreateInstance<FSoundEffectSubmixInitData, FSoundEffectSubmix>(
					InnerInit, *InPreset);
			InnerPtr->SetEnabled(true);

			// Step 2: create the wrapper DSP via CreateInstance on the transient preset —
			//         this sets WrapperDSP->Preset = WrapperPreset so level readouts work.
			FSoundEffectSubmixInitData WrapperInit = InitData;
			WrapperInit.ParentPresetUniqueId = WrapperPreset->GetUniqueID();
			TSharedPtr<FAudioFXSubmixMeterWrapper, ESPMode::ThreadSafe> WrapperTypedPtr =
				USoundEffectPreset::CreateInstance<FSoundEffectSubmixInitData, FAudioFXSubmixMeterWrapper>(
					WrapperInit, *WrapperPreset);
			WrapperTypedPtr->SetEnabled(true);

			// Wire the inner effect into the wrapper BEFORE audio starts
			WrapperTypedPtr->SetInnerEffect(InnerPtr);

			OverrideChain.Add(WrapperTypedPtr);
			bFoundTarget = true;

			UE_LOG(LogAudioEffectsStudio, Warning,
				TEXT("InstallSubmixMeters: target '%s' wrapped at chain position %d"),
				*InPreset->GetName(), OverrideChain.Num() - 1);
		}
		else
		{
			// Non-target effect — create via CreateInstance so Preset ptr is set.
			FSoundEffectSubmixInitData EffInit = InitData;
			EffInit.ParentPresetUniqueId = ChainEntry->GetUniqueID();
			FSoundEffectSubmixPtr EffectPtr =
				USoundEffectPreset::CreateInstance<FSoundEffectSubmixInitData, FSoundEffectSubmix>(
					EffInit, *ChainEntry);
			EffectPtr->SetEnabled(true);
			OverrideChain.Add(EffectPtr);
		}
	}

	if (!bFoundTarget)
	{
		UE_LOG(LogAudioEffectsStudio, Error,
			TEXT("InstallSubmixMeters: FAILED — preset '%s' not found in chain of '%s'"),
			*InPreset->GetName(), *InActiveSubmix->GetName());
		SubmixWrapper.Reset();
		return;
	}

	// Push the override to the live audio mixer (instant, no crossfade)
	AudioDevice->SetSubmixEffectChainOverride(InActiveSubmix, OverrideChain, 0.0f);

	UE_LOG(LogAudioEffectsStudio, Warning,
		TEXT("InstallSubmixMeters: SetSubmixEffectChainOverride pushed (%d effects) on '%s'"),
		OverrideChain.Num(), *InActiveSubmix->GetName());
}

// ─────────────────────────────────────────────────────────────────────────────
//  Remove
// ─────────────────────────────────────────────────────────────────────────────

void FAudioFXMeterGuard::RemoveSourceMeters()
{
	if (!SourceWrapper.IsValid()) { return; }

	USoundEffectSourcePresetChain* Chain = SourceChainWeak.Get();
	if (Chain && Chain->Chain.IsValidIndex(SourceWrapperSlot))
	{
		// Restore the original preset in the slot we swapped
		Chain->Chain[SourceWrapperSlot].Preset = OriginalSourcePreset.Get();
	}

	SourceWrapper.Reset();
	OriginalSourcePreset.Reset();
	SourceWrapperSlot = INDEX_NONE;
	UE_LOG(LogAudioEffectsStudio, Verbose, TEXT("MeterGuard: source wrapper removed"));
}

void FAudioFXMeterGuard::RemoveSubmixMeters()
{
	if (!SubmixWrapper.IsValid()) { return; }

	UE_LOG(LogAudioEffectsStudio, Warning,
		TEXT("RemoveSubmixMeters: clearing override on submix '%s'"),
		SubmixWeak.IsValid() ? *SubmixWeak->GetName() : TEXT("null"));

	if (USoundSubmix* Submix = SubmixWeak.Get())
	{
		if (GEngine)
		{
			if (FAudioDevice* AudioDevice = GEngine->GetMainAudioDevice().GetAudioDevice())
			{
				// Restore the original base chain (instant)
				AudioDevice->ClearSubmixEffectChainOverride(Submix, 0.0f);
			}
		}
	}

	SubmixWrapper.Reset();
	UE_LOG(LogAudioEffectsStudio, Warning, TEXT("RemoveSubmixMeters: done"));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Level Readout
// ─────────────────────────────────────────────────────────────────────────────

float FAudioFXMeterGuard::GetInputLevel() const
{
	if (bIsSourceEffect)
		return SourceWrapper.IsValid() ? SourceWrapper->GetInputPeakL() : 0.f;
	return SubmixWrapper.IsValid()
		? (SubmixWrapper->GetInputPeakL() + SubmixWrapper->GetInputPeakR()) * 0.5f : 0.f;
}

float FAudioFXMeterGuard::GetInputRmsLevel() const
{
	if (bIsSourceEffect)
		return SourceWrapper.IsValid() ? SourceWrapper->GetInputRmsL() : 0.f;
	return SubmixWrapper.IsValid()
		? (SubmixWrapper->GetInputRmsL() + SubmixWrapper->GetInputRmsR()) * 0.5f : 0.f;
}

float FAudioFXMeterGuard::GetOutputLevelL() const
{
	if (bIsSourceEffect) return SourceWrapper.IsValid() ? SourceWrapper->GetPeakL() : 0.f;
	return SubmixWrapper.IsValid() ? SubmixWrapper->GetPeakL() : 0.f;
}

float FAudioFXMeterGuard::GetOutputLevelR() const
{
	if (bIsSourceEffect) return SourceWrapper.IsValid() ? SourceWrapper->GetPeakR() : 0.f;
	return SubmixWrapper.IsValid() ? SubmixWrapper->GetPeakR() : 0.f;
}

float FAudioFXMeterGuard::GetOutputRmsLevelL() const
{
	if (bIsSourceEffect) return SourceWrapper.IsValid() ? SourceWrapper->GetRmsL() : 0.f;
	return SubmixWrapper.IsValid() ? SubmixWrapper->GetRmsL() : 0.f;
}

float FAudioFXMeterGuard::GetOutputRmsLevelR() const
{
	if (bIsSourceEffect) return SourceWrapper.IsValid() ? SourceWrapper->GetRmsR() : 0.f;
	return SubmixWrapper.IsValid() ? SubmixWrapper->GetRmsR() : 0.f;
}

float FAudioFXMeterGuard::ExchangeOutputPeakL() const
{
	if (bIsSourceEffect) return SourceWrapper.IsValid() ? SourceWrapper->ExchangeOutputPeakL() : 0.f;
	return SubmixWrapper.IsValid() ? SubmixWrapper->ExchangeOutputPeakL() : 0.f;
}

float FAudioFXMeterGuard::ExchangeOutputPeakR() const
{
	if (bIsSourceEffect) return SourceWrapper.IsValid() ? SourceWrapper->ExchangeOutputPeakR() : 0.f;
	return SubmixWrapper.IsValid() ? SubmixWrapper->ExchangeOutputPeakR() : 0.f;
}

float FAudioFXMeterGuard::ExchangeInputPeak() const
{
	if (bIsSourceEffect)
		return SourceWrapper.IsValid() ? SourceWrapper->ExchangeInputPeakL() : 0.f;
	if (SubmixWrapper.IsValid())
		return (SubmixWrapper->ExchangeInputPeakL() + SubmixWrapper->ExchangeInputPeakR()) * 0.5f;
	return 0.f;
}
