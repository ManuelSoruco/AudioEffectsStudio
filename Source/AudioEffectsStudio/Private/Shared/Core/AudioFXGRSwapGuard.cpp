// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

/**
 * @file AudioFXGRSwapGuard.cpp
 * @brief RAII guard that temporarily replaces a dynamics preset with a
 *        gain-reduction capture proxy for live GR metering during preview.
 *
 * Source-effect path:
 *   Locates the preset in the SoundEffectSourcePresetChain of the preview sound,
 *   replaces it with UAudioFXSourceGainReductionCapturePreset, and restores it
 *   on destruction.
 *
 * Submix-effect path:
 *   Locates the preset in the target SoundSubmix's effect chain,
 *   replaces it with UAudioFXSubmixGainReductionCapturePreset, and restores it
 *   on destruction.
 *
 * Only activates for USourceEffectDynamicsProcessorPreset and
 * USubmixEffectDynamicsProcessorPreset -- all other types pass through as no-ops.
 */

#include "Shared/Core/AudioFXGRSwapGuard.h"
#include "Sound/SoundWave.h"
#include "SubmixEffects/AudioMixerSubmixEffectDynamicsProcessor.h"
#include "SourceEffects/SourceEffectDynamicsProcessor.h"
#include "AudioDevice.h"
#include "Engine/Engine.h"
#include "UObject/UObjectGlobals.h"
#include "Shared/Core/AudioFXTheme.h" // LogAudioEffectsStudio

// ─────────────────────────────────────────────────────────────────────────────
//  Construction / Destruction
// ─────────────────────────────────────────────────────────────────────────────

FAudioFXGRSwapGuard::FAudioFXGRSwapGuard(
	USoundEffectPreset* InPreset,
	USoundWave* InSound,
	const TArray<FSoftObjectPath>& InSubmixPaths,
	bool bInPendingBypass,
	TSoftObjectPtr<USoundSubmix>& OutActiveSubmix)
{
	InstallSourceSwap(InPreset, InSound, bInPendingBypass);
	InstallSubmixSwap(InPreset, InSubmixPaths, OutActiveSubmix);
}

FAudioFXGRSwapGuard::~FAudioFXGRSwapGuard()
{
	if (!bRestored)
	{
		RestoreSourceSwap();
		RestoreSubmixSwap();
		bRestored = true;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  Install
// ─────────────────────────────────────────────────────────────────────────────

void FAudioFXGRSwapGuard::InstallSourceSwap(
	USoundEffectPreset* InPreset,
	USoundWave* InSound,
	bool bInPendingBypass)
{
	USourceEffectDynamicsProcessorPreset* SourceDynamics =
		Cast<USourceEffectDynamicsProcessorPreset>(InPreset);
	if (!SourceDynamics) { return; }

	USoundEffectSourcePresetChain* SourceChain = InSound ? InSound->SourceEffectChain : nullptr;
	if (!SourceChain) { return; }

	for (int32 SlotIndex = 0; SlotIndex < SourceChain->Chain.Num(); ++SlotIndex)
	{
		if (SourceChain->Chain[SlotIndex].Preset == InPreset)
		{
			UAudioFXSourceGainReductionCapturePreset* Capture =
				NewObject<UAudioFXSourceGainReductionCapturePreset>(
					GetTransientPackage(), NAME_None, RF_Transient);
			if (Capture)
			{
				Capture->SyncFromDynamicsPreset(SourceDynamics);
				CaptureSource       = TStrongObjectPtr<UAudioFXSourceGainReductionCapturePreset>(Capture);
				OriginalChain       = SourceChain;
				OriginalSourcePreset = SourceChain->Chain[SlotIndex].Preset;
				SourceSlotIndex     = SlotIndex;
				SourceChain->Chain[SlotIndex].Preset  = Capture;
				SourceChain->Chain[SlotIndex].bBypass = bInPendingBypass;
				UE_LOG(LogAudioEffectsStudio, Verbose,
					TEXT("GRSwapGuard: source GR capture installed at slot %d"), SlotIndex);
			}
			break;
		}
	}
}

void FAudioFXGRSwapGuard::InstallSubmixSwap(
	USoundEffectPreset* InPreset,
	const TArray<FSoftObjectPath>& InSubmixPaths,
	TSoftObjectPtr<USoundSubmix>& OutActiveSubmix)
{
	USubmixEffectDynamicsProcessorPreset* SubmixDynamics =
		Cast<USubmixEffectDynamicsProcessorPreset>(InPreset);
	if (!SubmixDynamics || InSubmixPaths.Num() == 0) { return; }

	OutActiveSubmix = TSoftObjectPtr<USoundSubmix>(InSubmixPaths[0]);
	USoundSubmix* Submix = OutActiveSubmix.LoadSynchronous();
	if (!Submix) { return; }

	for (int32 SlotIndex = 0; SlotIndex < Submix->SubmixEffectChain.Num(); ++SlotIndex)
	{
		if (Submix->SubmixEffectChain[SlotIndex] == InPreset)
		{
			UAudioFXSubmixGainReductionCapturePreset* Capture =
				NewObject<UAudioFXSubmixGainReductionCapturePreset>(
					GetTransientPackage(), NAME_None, RF_Transient);
			if (Capture)
			{
				Capture->SyncFromDynamicsPreset(SubmixDynamics);
				CaptureSingle       = TStrongObjectPtr<UAudioFXSubmixGainReductionCapturePreset>(Capture);
				SwapSubmix          = Submix;
				OriginalSubmixPreset = Submix->SubmixEffectChain[SlotIndex];
				SubmixSlotIndex     = SlotIndex;
				Submix->SubmixEffectChain[SlotIndex] = Capture;
				if (GEngine)
				{
					if (FAudioDevice* AudioDevice = GEngine->GetMainAudioDevice().GetAudioDevice())
					{
						AudioDevice->UpdateSubmixProperties(Submix);
					}
				}
				UE_LOG(LogAudioEffectsStudio, Verbose,
					TEXT("GRSwapGuard: submix GR capture installed at slot %d on '%s'"),
					SlotIndex, *Submix->GetName());
			}
			break;
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  Restore
// ─────────────────────────────────────────────────────────────────────────────

void FAudioFXGRSwapGuard::RestoreSourceSwap()
{
	if (SourceSlotIndex == INDEX_NONE) { return; }

	USoundEffectSourcePresetChain* Chain = OriginalChain.Get();
	USoundEffectSourcePreset* Original  = OriginalSourcePreset.Get();
	if (Chain && Original && Chain->Chain.IsValidIndex(SourceSlotIndex))
	{
		if (Chain->Chain[SourceSlotIndex].Preset == Original)
		{
			// Slot already holds the original — another path already restored it
			// (e.g. editor window closed while audio was playing). Safe to skip.
			UE_LOG(LogAudioEffectsStudio, Warning,
				TEXT("GRSwapGuard: source slot %d already restored — skipping double-restore."), SourceSlotIndex);
		}
		else
		{
			Chain->Chain[SourceSlotIndex].Preset = Original;
			UE_LOG(LogAudioEffectsStudio, Verbose,
				TEXT("GRSwapGuard: restored source slot %d"), SourceSlotIndex);
		}
	}
	SourceSlotIndex = INDEX_NONE;
	CaptureSource.Reset();
}

void FAudioFXGRSwapGuard::RestoreSubmixSwap()
{
	if (SubmixSlotIndex == INDEX_NONE) { return; }

	USoundSubmix* Submix              = SwapSubmix.Get();
	USoundEffectSubmixPreset* Original = OriginalSubmixPreset.Get();
	if (Submix && Original && Submix->SubmixEffectChain.IsValidIndex(SubmixSlotIndex))
	{
		if (Submix->SubmixEffectChain[SubmixSlotIndex] == Original)
		{
			// Slot already holds the original — another path already restored it.
			// (e.g. editor window closed while audio was playing). Safe to skip.
			UE_LOG(LogAudioEffectsStudio, Warning,
				TEXT("GRSwapGuard: submix slot %d on '%s' already restored — skipping double-restore."),
				SubmixSlotIndex, *Submix->GetName());
		}
		else
		{
			Submix->SubmixEffectChain[SubmixSlotIndex] = Original;
			if (GEngine)
			{
				if (FAudioDevice* AudioDevice = GEngine->GetMainAudioDevice().GetAudioDevice())
				{
					AudioDevice->UpdateSubmixProperties(Submix);
				}
			}
			UE_LOG(LogAudioEffectsStudio, Verbose,
				TEXT("GRSwapGuard: restored submix slot %d on '%s'"),
				SubmixSlotIndex, *Submix->GetName());
		}
	}
	SubmixSlotIndex = INDEX_NONE;
	CaptureSingle.Reset();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Queries / Controls
// ─────────────────────────────────────────────────────────────────────────────

float FAudioFXGRSwapGuard::GetGainReductionDb() const
{
	if (CaptureSingle.IsValid())   { return CaptureSingle->ExchangeGainReductionDb(); }
	if (CaptureSource.IsValid())   { return CaptureSource->ExchangeGainReductionDb(); }
	return 0.0f;
}

bool FAudioFXGRSwapGuard::HasGainReductionData() const
{
	return CaptureSingle.IsValid() || CaptureSource.IsValid();
}

void FAudioFXGRSwapGuard::SetSourceBypass(bool bBypass)
{
	if (SourceSlotIndex == INDEX_NONE) { return; }
	USoundEffectSourcePresetChain* Chain = OriginalChain.Get();
	if (Chain && Chain->Chain.IsValidIndex(SourceSlotIndex))
	{
		Chain->Chain[SourceSlotIndex].bBypass = bBypass;
	}
}

void FAudioFXGRSwapGuard::SyncCaptureFromPreset(USoundEffectPreset* InPreset)
{
	if (CaptureSingle.IsValid())
	{
		if (const USubmixEffectDynamicsProcessorPreset* D =
			Cast<USubmixEffectDynamicsProcessorPreset>(InPreset))
		{
			CaptureSingle->SyncFromDynamicsPreset(D);
		}
	}
	else if (CaptureSource.IsValid())
	{
		if (const USourceEffectDynamicsProcessorPreset* D =
			Cast<USourceEffectDynamicsProcessorPreset>(InPreset))
		{
			CaptureSource->SyncFromDynamicsPreset(D);
		}
	}
}
