// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundEffectSubmix.h"
#include "Shared/Effects/AudioFXSourceMeterCapture.h"
#include "Shared/Effects/AudioFXSubmixMeterCapture.h"

class USoundWave;
class USoundSubmix;
struct FAudioFXGRSwapGuard;

/**
 * FAudioFXMeterGuard
 *
 * RAII guard that inserts transient input/output meter presets around the
 * target effect slot in a source chain or submix chain, then removes them
 * on destruction.
 *
 * Must be constructed AFTER FAudioFXGRSwapGuard so it can read the correct
 * slot index (which may have been modified by the GR swap).
 *
 * Usage:
 *   MeterGuard = MakeUnique<FAudioFXMeterGuard>(bIsSource, Sound, ActiveSubmix, GRSwap);
 *   // ... play ...
 *   MeterGuard.Reset();  // Removal happens automatically.
 */
struct AUDIOEFFECTSSTUDIO_API FAudioFXMeterGuard
{
	FAudioFXMeterGuard(const FAudioFXMeterGuard&) = delete;
	FAudioFXMeterGuard& operator=(const FAudioFXMeterGuard&) = delete;

	/**
	 * Construct and install meter effects.
	 * @param bInIsSourceEffect  True for source effects, false for submix effects.
	 * @param InPreset           The preset being monitored (used to locate the slot
	 *                           when GRSwap is not active).
	 * @param InSound            Sound whose source chain to instrument (source only).
	 * @param InActiveSubmix     Loaded submix to instrument (submix only).
	 * @param InGRSwap           The GR swap guard (may be null); provides slot index
	 *                           and receives slot-offset adjustments.
	 */
	FAudioFXMeterGuard(
		bool bInIsSourceEffect,
		USoundEffectPreset* InPreset,
		USoundWave* InSound,
		USoundSubmix* InActiveSubmix,
		FAudioFXGRSwapGuard* InGRSwap);

	/** Remove meter presets on destruction. Idempotent. */
	~FAudioFXMeterGuard();

	// ── Metering Readout ─────────────────────────────────────────────

	/** Peak input level (linear 0..1) — brighter bar. */
	float GetInputLevel()    const;
	/** RMS input level (linear 0..1) — darker/background bar. */
	float GetInputRmsLevel() const;

	/** Peak output levels — brighter bar. */
	float GetOutputLevelL()  const;
	float GetOutputLevelR()  const;
	/** RMS output levels — darker/background bar. */
	float GetOutputRmsLevelL() const;
	float GetOutputRmsLevelR() const;

	/**
	 * Exchange-based peak readout: atomically reads AND resets the peak to 0.
	 * Returns the highest peak since the previous call. Used by SAudioFXMeterStrip
	 * to avoid missing transients between Slate Tick frames.
	 */
	float ExchangeOutputPeakL() const;
	float ExchangeOutputPeakR() const;
	float ExchangeInputPeak()   const;

private:
	void InstallSourceMeters(USoundEffectPreset* InPreset, USoundWave* InSound,
		FAudioFXGRSwapGuard* InGRSwap);
	void InstallSubmixMeters(USoundEffectPreset* InPreset, USoundSubmix* InActiveSubmix,
		FAudioFXGRSwapGuard* InGRSwap);
	void RemoveSourceMeters();
	void RemoveSubmixMeters();

	/** True for source effects, false for submix effects. */
	bool bIsSourceEffect = false;

	// -- Source meters (wrapper replaces the target slot) ----------------
	/** The meter capture preset placed in the source chain. */
	TStrongObjectPtr<UAudioFXSourceMeterCapturePreset> SourceWrapper;
	/** Slot index of the inserted wrapper in the source chain. */
	int32 SourceWrapperSlot = INDEX_NONE;
	/** Original preset stored inside the wrapper (restored on removal). */
	TWeakObjectPtr<USoundEffectSourcePreset> OriginalSourcePreset;
	/** Weak ref to the source chain we modified. */
	TWeakObjectPtr<USoundEffectSourcePresetChain> SourceChainWeak;

	// -- Submix meters (SetSubmixEffectChainOverride) --------------------
	/** The meter capture preset placed via submix chain override. */
	TStrongObjectPtr<UAudioFXSubmixMeterCapturePreset> SubmixWrapper;
	/** Weak ref to the submix we overrode. */
	TWeakObjectPtr<USoundSubmix> SubmixWeak;

	/** True after removal has run -- prevents double-remove. */
	bool bRemoved = false;
};
