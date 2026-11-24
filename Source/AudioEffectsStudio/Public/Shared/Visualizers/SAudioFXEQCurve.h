// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "SourceEffects/SourceEffectEQ.h"

/**
 * SAudioFXEQCurve
 *
 * Real-time parametric EQ frequency response display drawn entirely in OnPaint.
 *
 * Layout:
 *   - Dark inset background
 *   - Subtle frequency grid (20, 50, 100, 200, 500, 1k, 2k, 5k, 10k, 20k Hz)
 *   - Horizontal 0 dB center line
 *   - Per-band magnitude curves, each in the band's color, at 60% opacity
 *   - White sum curve (all bands combined), 2px antialiased
 *   - ±12 dB scale (anything beyond ±24 dB clamped visually)
 *
 * All drawing is pure Slate — no materials, no textures.
 * Updates every frame (live bound to preset settings).
 */
class AUDIOEFFECTSSTUDIO_API SAudioFXEQCurve : public SLeafWidget
{
public:

	// ============================================================
	// Slate arguments
	// ============================================================

	SLATE_BEGIN_ARGS(SAudioFXEQCurve)
		: _AssetColor(FLinearColor(0.212f, 0.620f, 0.600f))   // Teal-green, matching EQ default
	{}
		/** Live settings read each frame */
		SLATE_ATTRIBUTE(FSourceEffectEQSettings, Settings)

		/** Band palette — per-band curve colors, matched to the card colors */
		SLATE_ATTRIBUTE(TArray<FLinearColor>, BandColors)

		/** Effect asset color — used for grid tint */
		SLATE_ARGUMENT(FLinearColor, AssetColor)
	SLATE_END_ARGS()

	// ============================================================
	// Public interface
	// ============================================================

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;

	/**
	 * Compute magnitude response in dB for a single peaking EQ band at frequency Hz.
	 * Uses the standard Audio EQ Cookbook peaking filter formula.
	 * Public: pure math function, also used by automated tests.
	 * @param FreqHz      Evaluation frequency in Hz
	 * @param CenterHz    Band center frequency in Hz
	 * @param BandwidthOctaves Band bandwidth in octaves (matches FSourceEffectEQBand::Bandwidth)
	 * @param GainDb      Peak gain in dB
	 * @param SampleRate  Sample rate (44100 Hz assumed)
	 * @return Magnitude response in dB at FreqHz
	 */
	static float BandMagnitudeDb(float FreqHz, float CenterHz, float BandwidthOctaves,
		float GainDb, float SampleRate = 44100.f);

	/**
	 * Map a frequency in Hz to a normalized [0,1] x position (log scale).
	 * Public: pure math function, also used by automated tests.
	 */
	static float FreqToNorm(float Hz)
	{
		const float ClampedHz = FMath::Clamp(Hz, 20.f, 20000.f);
		return (FMath::Log2(ClampedHz) - FMath::Log2(20.f))
		     / (FMath::Log2(20000.f) - FMath::Log2(20.f));
	}

private:
	// ── Private math helpers ─────────────────────────────────────────────────

	/**
	 * Map a dB value to a Slate pixel Y coordinate.
	 * 0 dB → center of Height, positive dB → above center (smaller Y, Slate Y is down).
	 */
	static float EQDbToY(float Db, float Height);

	/** Evaluate a single band's magnitude at EvalHz with clamped inputs and NaN guard. */
	static float SampleBandDb(const FSourceEffectEQBand& Band, float EvalHz);

	// ── Members ──────────────────────────────────────────────────────────────

	TAttribute<FSourceEffectEQSettings>    Settings;
	TAttribute<TArray<FLinearColor>>       BandColors;
	FLinearColor                           AssetColor = FLinearColor(0.212f, 0.620f, 0.600f);

	// dB display range — ±DisplayDbRange centered on 0
	static constexpr float DisplayDbRange = 24.f;   // ±24 dB visible
	static constexpr float DisplayDbHard  = 30.f;   // hard clamp on curve

	// Number of frequency evaluation points for the curve
	static constexpr int32 NumCurvePoints = 1024;

	// Pre-allocated frequency evaluation array — avoids per-frame heap allocation.
	// Built once in Construct; mutable so OnPaint (const) can use it.
	mutable TArray<float> CachedEvalFreqs;

	// ── Private paint helpers (called by OnPaint) ─────────────────────────────

	/** Draw the dark inset background box. */
	int32 PaintBackground(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId) const;

	/** Draw frequency vertical grid lines and dB horizontal grid lines, plus the 0 dB center line. */
	int32 PaintFrequencyGrid(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		float Width, float Height) const;

	/** Draw per-band glow + main curves for all enabled, non-zero bands. */
	int32 PaintBandCurves(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		float Width, float Height,
		const FSourceEffectEQSettings& EQSettings,
		const TArray<FLinearColor>& Colors) const;

	/** Draw the combined-sum glow pass and solid sum curve. */
	int32 PaintSumCurve(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		float Width, float Height,
		const FSourceEffectEQSettings& EQSettings) const;
};
