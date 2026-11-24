// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "Shared/Core/AudioFXEffectEditor.h"
#include "Shared/Core/AudioFXTheme.h"
#include "SubmixEffects/SubmixEffectMultiBandCompressor.h"

struct FSlateRoundedBoxBrush;

/**
 * FAudioFXSubmixMultibandEditor
 *
 * Multiband Compressor editor for USubmixEffectMultibandCompressorPreset.
 *
 * Layout (top to bottom):
 *   1. PresetBar (base class)
 *   2. GLOBAL BAR  — Type / Peak / Link / LookAhead / Analog / 4-Pole
 *   3. CURVE       — SAudioFXMultibandCurve (combined freq + transfer view)
 *   4. BAND RACK   — horizontal scroll with alternating cards and crossover knobs
 *
 * Design rules:
 *   · Minimum 2 bands enforced at open time (modal dialog if fewer).
 *   · Crossover[i] = Bands[i].CrossoverTopFrequency; last band's crossover is never shown.
 *   · Monotonic ordering enforced via ClampCrossoverHz.
 *   · Threshold is the hero knob in each card.
 *   · Remove button disabled when Bands.Num() <= 2.
 */
class AUDIOEFFECTSSTUDIO_API FAudioFXSubmixMultibandEditor : public FAudioFXEffectEditor
{
public:
	virtual TSharedRef<SWidget>  CreateEffectWidget(USoundEffectPreset* InPreset) override;
	virtual FVector2D            GetDefaultWindowSize() const override { return FVector2D(900, 620); }
	virtual FText               GetEditorTitle() const override { return FText::FromString(TEXT("Multiband Dynamics")); }
	virtual bool                 ShouldShowMeters() const override { return false; }
	virtual bool                 ShouldShowGRMeter() const override { return false; }
	virtual bool                 ShouldShowBypass() const override { return true; }
	virtual bool                 GetBypassState() const override;
	virtual void                 SetBypassState(bool bBypassed) override;

	// Dynamic content: bands appear/disappear → VAlign_Top prevents vertical jump
	virtual EVerticalAlignment   GetContentVAlign() const override { return VAlign_Top; }

private:
	// ── Validation ───────────────────────────────────────────────────

	/**
	 * Guards against degenerate presets (< 2 bands).
	 * Shows a modal dialog offering to create default bands or open the native editor.
	 * Returns true if the custom editor should be built, false to fall back.
	 */
	bool EnforceMinimumBands(USubmixEffectMultibandCompressorPreset* Preset);

protected:
	/**
	 * When true, EnforceMinimumBands() silently auto-creates default bands without
	 * showing the blocking modal dialog. Set this on capture/headless subclasses.
	 */
	bool bSuppressMinBandsDialog = false;

private:

	// ── Builders ─────────────────────────────────────────────────────

	/** Build one compressor band card for BandIndex. No crossover knob inside. */
	TSharedRef<SWidget> BuildBandCard(int32 BandIndex, USubmixEffectMultibandCompressorPreset* Preset,
		const FLinearColor& AssetCol, const FLinearColor& BandCol,
		TSharedPtr<FAudioFXPreviewEngine> InEngine);

	/** Build the trailing Add Band (+) card at the end of the rack. */
	TSharedRef<SWidget> BuildAddBandButton();

	/** Build a single crossover label+knob for the transition at CrossIdx (0-based). */
	TSharedRef<SWidget> BuildCrossoverControl(int32 CrossIdx, USubmixEffectMultibandCompressorPreset* Preset);

	// ── Rack management ───────────────────────────────────────────────

	/** Rebuild the dedicated crossover bar (N-1 knobs). */
	void RebuildCrossoverBar();

	/** Rebuild the band rack + inline crossover knobs after add/remove. */
	void RebuildRack();

	/**
	 * Clamp a desired crossover frequency so all crossovers stay monotonically ordered.
	 * Uses MinCrossoverGapHz as the minimum separation between adjacent values.
	 */
	static float ClampCrossoverHz(const TArray<FDynamicsBandSettings>& Bands, int32 CrossIdx, float DesiredHz);

	/** Minimum frequency gap (Hz) enforced between adjacent crossovers. */
	static constexpr float MinCrossoverGapHz = 50.f;

	// ── State ─────────────────────────────────────────────────────────

	TWeakObjectPtr<USubmixEffectMultibandCompressorPreset> WeakPreset;
	TSharedPtr<SHorizontalBox>                             BandRack;
	TSharedPtr<SHorizontalBox>                             CrossoverBar;
	FLinearColor                                           CachedAssetCol = FLinearColor::White;

	/** Tracks collapsed/expanded state of the RESPONSE section across card rebuilds. */
	bool bResponseSectionExpanded = false;

	/** Per-band color-stripe brushes (owned, rebuilt on every RebuildRack call). */
	TArray<TUniquePtr<FSlateRoundedBoxBrush>>              BandStripeBrushes;
};
