// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundEffectPreset.h"
#include "Sound/SoundSubmix.h"
#include "Sound/SoundEffectSource.h"
#include "Shared/Effects/AudioFXSubmixGainReductionCapture.h"
#include "Shared/Effects/AudioFXSourceGainReductionCapture.h"

class USoundWave;
class USoundEffectSourcePresetChain;

/**
 * FAudioFXGRSwapGuard
 *
 * RAII guard that swaps the real dynamics preset in a source chain or submix
 * chain with a transient GR capture proxy, then restores it on destruction.
 *
 * Usage:
 *   GRSwap = MakeUnique<FAudioFXGRSwapGuard>(Preset, Sound, TargetSubmixPaths, bPendingBypass);
 *   // ... play ...
 *   GRSwap.Reset();  // Restore happens here automatically.
 *
 * Only activates if Preset is a USourceEffectDynamicsProcessorPreset or
 * USubmixEffectDynamicsProcessorPreset — otherwise it's a no-op guard.
 */
struct AUDIOEFFECTSSTUDIO_API FAudioFXGRSwapGuard
{
	// Non-copyable, non-movable
	FAudioFXGRSwapGuard(const FAudioFXGRSwapGuard&) = delete;
	FAudioFXGRSwapGuard& operator=(const FAudioFXGRSwapGuard&) = delete;

	/**
	 * Construct and install the GR capture swap.
	 * @param InPreset         The preset being edited (may or may not be a dynamics type).
	 * @param InSound          Sound whose source chain to inspect (source effects only).
	 * @param InSubmixPaths    Direct submix paths from pool discovery (submix effects only).
	 * @param bInPendingBypass Bypass state to apply to the swapped source chain slot.
	 * @param OutActiveSubmix  Receives the loaded submix if a submix swap was performed.
	 */
	FAudioFXGRSwapGuard(
		USoundEffectPreset* InPreset,
		USoundWave* InSound,
		const TArray<FSoftObjectPath>& InSubmixPaths,
		bool bInPendingBypass,
		TSoftObjectPtr<USoundSubmix>& OutActiveSubmix);

	/** Restore the swapped slots on destruction. Idempotent. */
	~FAudioFXGRSwapGuard();

	// ── Queries ──────────────────────────────────────────────────────

	/** Gain reduction in dB from the active capture preset, or 0. */
	float GetGainReductionDb() const;

	/** True if a GR capture is active. */
	bool HasGainReductionData() const;

	/**
	 * The slot index where the source GR capture was placed in the chain
	 * (before meter insertion). INDEX_NONE if no source swap.
	 * Needed by FAudioFXMeterGuard to place meters around the right slot.
	 */
	int32 GetSourceSlotIndex() const { return SourceSlotIndex; }

	/** The slot index where the submix GR capture was placed. INDEX_NONE if no submix swap. */
	int32 GetSubmixSlotIndex() const { return SubmixSlotIndex; }

	/**
	 * Update the bypass flag on the swapped source chain slot.
	 * Also stores the value so subsequent Play() cycles re-apply it.
	 */
	void SetSourceBypass(bool bBypass);

	/**
	 * Adjust the stored source slot index when the meter guard inserts
	 * entries before it. Needed to keep restoration index correct.
	 */
	void AdjustSourceSlotIndex(int32 Delta) { SourceSlotIndex += Delta; }

	/**
	 * Adjust the stored submix slot index when the meter guard inserts
	 * entries before it. Needed to keep restoration index correct.
	 */
	void AdjustSubmixSlotIndex(int32 Delta) { SubmixSlotIndex += Delta; }

	/**
	 * Sync capture parameters from the live preset (called when preset
	 * properties change during playback so GR follows parameter edits).
	 */
	void SyncCaptureFromPreset(USoundEffectPreset* InPreset);

private:
	void InstallSourceSwap(USoundEffectPreset* InPreset, USoundWave* InSound, bool bInPendingBypass);
	void InstallSubmixSwap(USoundEffectPreset* InPreset, const TArray<FSoftObjectPath>& InSubmixPaths,
		TSoftObjectPtr<USoundSubmix>& OutActiveSubmix);
	void RestoreSourceSwap();
	void RestoreSubmixSwap();

	// -- Source swap state ------------------------------------------------
	/** Transient GR capture preset placed in the source chain slot. */
	TStrongObjectPtr<UAudioFXSourceGainReductionCapturePreset> CaptureSource;
	/** The chain whose slot we modified -- used to restore on destruction. */
	TWeakObjectPtr<USoundEffectSourcePresetChain>               OriginalChain;
	/** The original preset that occupied the source slot before the swap. */
	TWeakObjectPtr<USoundEffectSourcePreset>                    OriginalSourcePreset;
	/** Index of the slot we swapped in the source chain. INDEX_NONE = no swap. */
	int32 SourceSlotIndex = INDEX_NONE;

	// -- Submix swap state ------------------------------------------------
	/** Transient GR capture preset placed in the submix effect chain. */
	TStrongObjectPtr<UAudioFXSubmixGainReductionCapturePreset>  CaptureSingle;
	/** The submix whose effect chain we modified. */
	TWeakObjectPtr<USoundSubmix>                                SwapSubmix;
	/** The original submix preset that was replaced. */
	TWeakObjectPtr<USoundEffectSubmixPreset>                    OriginalSubmixPreset;
	/** Index of the slot we swapped in the submix chain. INDEX_NONE = no swap. */
	int32 SubmixSlotIndex = INDEX_NONE;

	/** True after RestoreSourceSwap() / RestoreSubmixSwap() have run -- prevents double-restore. */
	bool bRestored = false;
};
