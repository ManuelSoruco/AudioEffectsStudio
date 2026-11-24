// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "Shared/Core/AudioFXEffectEditor.h"
#include "Shared/Core/AudioFXTheme.h"
#include "SubmixEffects/SubmixEffectTapDelay.h"

struct FSlateRoundedBoxBrush;

/**
 * FAudioFXTapDelayEditor
 *
 * Submix Tap Delay editor — a 500-series horizontal rack with one card per tap.
 *
 * Layout (top-to-bottom):
 *   1. SAudioFXTapDelayGraph  — waveform view showing all tap positions (≥200px, fills height)
 *   2. GLOBAL section         — MaxDelay + Interp knobs + START/STOP tap-recording transport
 *   3. Tap rack (scrollable)  — one card per FTapDelayInfo, plus a trailing "+" Add button
 *
 * Each tap card contains:
 *   · Tap label (color-coded from the band palette)
 *   · TapLineMode strip (Ch / Pan / Off)
 *   · Delay (Hero) + Gain knobs
 *   · Conditional Pan knob (visible when Mode == Panning)
 *   · Conditional Channel knob (visible when Mode == SendToChannel)
 *   · Remove button (hover-only)
 *
 * Tap recording (START TAPPING / STOP) records timestamps and derives
 * intervals + a descending gain envelope, then writes new FTapDelayInfo entries.
 */
class AUDIOEFFECTSSTUDIO_API FAudioFXTapDelayEditor : public FAudioFXEffectEditor
{
public:
	virtual TSharedRef<SWidget>  CreateEffectWidget(USoundEffectPreset* InPreset) override;
	virtual FVector2D            GetDefaultWindowSize() const override { return FVector2D(920, 760); }
	virtual FText               GetEditorTitle() const override { return FText::FromString(TEXT("Tap Delay")); }
	virtual bool                 ShouldShowMeters() const override { return false; }

	// Dynamic content: taps appear/disappear → VAlign_Top prevents vertical jump
	virtual EVerticalAlignment   GetContentVAlign() const override { return VAlign_Top; }

	/** Build interval list (ms) from absolute tap timestamps (seconds from recording start). */
	static TArray<float> BuildTapIntervalsMs(const TArray<double>& InTapTimestampsSec);

	/** Build descending gain (dB) per interval: first tap loudest, last quietest. */
	static TArray<float> BuildTapGainEnvelopeDb(int32 NumIntervals);

private:
	// ── Card / rack builders ──────────────────────────────────────────

	/** Build one tap card for the given index into Preset->Settings.Taps. */
	TSharedRef<SWidget> BuildTapCard(int32 TapIndex, USubmixEffectTapDelayPreset* Preset,
		const FLinearColor& AssetCol);

	/** Build the Add Tap (+) button shown at the trailing end of the rack. */
	TSharedRef<SWidget> BuildAddTapButton();

	/** Rebuild the entire tap rack after a tap add or remove. */
	void RebuildRack();

	// ── Tap recording transport ───────────────────────────────────────

	/**
	 * Handle a click on the START TAPPING / STOP button.
	 * On START: initializes the recording session (timestamps list, start time).
	 * On STOP:  derives intervals + gain envelope and writes new tap settings.
	 */
	void HandleTapRecordToggle();

	/** Register one raw tap timestamp during an active recording session. */
	void RegisterTapTempo();

	/** Apply the computed tap-tempo delay value to all existing taps. */
	void ApplyTapTempoToAll(float DelayMs);

	// ── State ─────────────────────────────────────────────────────────

	TWeakObjectPtr<USubmixEffectTapDelayPreset>  WeakPreset;
	TSharedPtr<SHorizontalBox>                   TapRack;
	FLinearColor                                 CachedAssetCol = FLinearColor::White;

	/** Absolute timestamps (seconds from session start) recorded by RegisterTapTempo(). */
	TArray<double>                               TapTimestamps;
	double                                       TapRecordStartSeconds = 0.0;
	bool                                         bTapRecordingActive   = false;

	/** Per-tap color-stripe brushes (owned, rebuilt on every RebuildRack call). */
	TArray<TUniquePtr<FSlateRoundedBoxBrush>>    TapStripeBrushes;
};
