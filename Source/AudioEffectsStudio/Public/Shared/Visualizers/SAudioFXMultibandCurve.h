// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "SubmixEffects/SubmixEffectMultiBandCompressor.h"

/**
 * SAudioFXMultibandCurve
 *
 * Compact frequency-domain visualization for the multiband compressor.
 * Shows each frequency band as a colored filled region with a gain-envelope
 * line, plus crossover markers and overlap-zone highlights.
 *
 * Layout (typically 400×80):
 *   - Top gradient background (dark visualization → window background)
 *   - Log-spaced frequency grid (20 Hz … 20 kHz) with labels
 *   - Horizontal dB grid lines (0, -6, -12, -18, -24 dB) with labels
 *   - Crossover overlap zones (shaded white, ±HalfOctaves around each crossover)
 *   - Per-band filled area (10% opacity) + glow line + solid main line
 *   - Thin black crossover dividers on top of the overlap zones
 *   - "Add a band to begin" hint when no bands are configured
 *
 * All drawing is pure Slate — no materials, no textures.
 */
class AUDIOEFFECTSSTUDIO_API SAudioFXMultibandCurve : public SLeafWidget
{
public:

	// ============================================================
	// Slate arguments
	// ============================================================

	SLATE_BEGIN_ARGS(SAudioFXMultibandCurve)
		: _AssetColor(FLinearColor(0.620f, 0.300f, 0.300f))
	{}
		/** Live settings read every frame — bind to the preset's Settings struct */
		SLATE_ATTRIBUTE(FSubmixEffectMultibandCompressorSettings, Settings)

		/** Per-band colors, matched to the band cards in the editor UI */
		SLATE_ATTRIBUTE(TArray<FLinearColor>, BandColors)

		/** Effect asset color — used as fallback tint */
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

	// ============================================================
	// Public statics (also used by tests)
	// ============================================================

	/** Delegate log-scale frequency-to-normalized mapping to SAudioFXEQCurve. */
	static float FreqToNorm(float Hz);

	/**
	 * Estimate the crossover overlap half-width in octaves based on the filter order.
	 * Four-pole (24 dB/oct) filters are steeper → narrower visual overlap zone.
	 */
	static float EstimateOverlapHalfOctaves(bool bFourPole);

	/**
	 * Compute a smooth-step blend factor for a crossover edge fade.
	 * @param DistanceOctaves  Frequency distance from the crossover edge in octaves
	 * @param HalfWidthOctaves Half-width of the transition region in octaves
	 * @return 0 at the edge, 1 fully inside the band
	 */
	static float ComputeEdgeBlend(float DistanceOctaves, float HalfWidthOctaves);

private:

	// ============================================================
	// Members
	// ============================================================

	TAttribute<FSubmixEffectMultibandCompressorSettings> Settings;
	TAttribute<TArray<FLinearColor>>                     BandColors;
	FLinearColor                                         AssetColor = FLinearColor(0.620f, 0.300f, 0.300f);

	// Frequency and dB display extents
	static constexpr float MinFreqHz = 20.f;
	static constexpr float MaxFreqHz = 20000.f;
	static constexpr float MinDb     = -24.f;
	static constexpr float MaxDb     =   6.f;

	// Number of horizontal samples used to build each band's envelope polyline
	static constexpr int32 NumEnvelopeSamples = 64;

	// ============================================================
	// Private paint helpers (called by OnPaint)
	// ============================================================

	/** Draw the vertical-gradient background behind the graph area. */
	int32 PaintMBBackground(
		const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, float GraphW, float GraphH, float LeftPad, float TopPad) const;

	/** Draw frequency vertical lines, dB horizontal lines, and axis labels. */
	int32 PaintMBGrid(
		const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, float Width, float Height,
		float GraphW, float GraphH, float LeftPad, float TopPad) const;

	/** Draw translucent crossover overlap zone boxes and their borders. */
	int32 PaintMBOverlapZones(
		const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, float GraphH, float LeftPad, float TopPad,
		const TArray<float>& Crossovers, float OctaveScale) const;

	/** Draw per-band filled areas plus glow and solid envelope curves. */
	int32 PaintMBBandAreas(
		const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, float GraphW, float GraphH, float LeftPad, float TopPad,
		const FSubmixEffectMultibandCompressorSettings& Cfg,
		const TArray<FLinearColor>& Colors,
		const TArray<float>& Crossovers, float OctaveScale, int32 NumBands) const;

	/** Draw thin solid crossover divider lines on top of band areas. */
	int32 PaintMBCrossoverLines(
		const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, float GraphH, float LeftPad, float TopPad,
		const TArray<float>& Crossovers) const;

	/**
	 * Draw equal-power crossfade curves (cos²/sin²) as thin black lines across
	 * each crossover overlap zone. One curve falls 0→1, one rises 1→0 in gain.
	 */
	int32 PaintMBEqualPowerCurves(
		const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, float GraphW, float GraphH, float LeftPad, float TopPad,
		const TArray<float>& Crossovers, float OctaveScale) const;
};
