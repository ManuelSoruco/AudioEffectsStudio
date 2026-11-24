// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SLeafWidget.h"

/**
 * FAudioFXGraphBar
 *
 * A single vertical bar in the time-domain graph.
 * TimeSec is the X position (in seconds), GainDb is the Y position (in dB).
 * Color is per-bar (allows palette-based coloring for tap delay).
 * bDisabled dims the bar to 30% opacity.
 */
struct FAudioFXGraphBar
{
	float TimeSec = 0.f;
	float GainDb  = 0.f;   // 0 dB = full height, -40 dB = floor
	FLinearColor Color = FLinearColor::White;
	bool bDisabled = false;
};

/**
 * FAudioFXGraphCurve
 *
 * A continuous polyline drawn over the grid.
 * Points are in (TimeSec, LinearAmplitude 0..1) space.
 * The graph converts amplitude to dB internally for Y placement.
 */
struct FAudioFXGraphCurve
{
	TArray<FVector2f> Points;     // (TimeSec, LinearAmplitude 0..1)
	FLinearColor Color = FLinearColor::White;
	float Thickness    = 2.0f;
	bool bDashed       = false;
	bool bGlow         = false;   // If true, draw a fat translucent pass underneath
};

/**
 * SAudioFXTimeDomainGraph
 *
 * Unified time-domain visualization used by ALL reverb, delay, and tap-delay
 * editors. Draws a standardized grid:
 *
 *  - Vertical axis: dB scale, 0 dB at top, -40 dB at bottom
 *  - Horizontal dB grid lines at -6, -12, -18, -24, -30 dB (with labels)
 *  - Vertical time grid lines, auto-scaled to MaxTimeSec (with labels)
 *  - Floor line at -40 dB, ceiling line at 0 dB
 *
 * Content is provided via two delegate attributes:
 *  - Bars: vertical bar elements (delay echoes, tap delay taps)
 *  - Curves: continuous polylines (reverb decay envelopes)
 *
 * MaxTimeSec is dynamically settable and the grid auto-adapts.
 */
class AUDIOEFFECTSSTUDIO_API SAudioFXTimeDomainGraph : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SAudioFXTimeDomainGraph)
		: _MaxTimeSec(2.0f)
		, _AssetColor(FLinearColor::White)
	{}
		/** Maximum time extent of the X axis in seconds. Grid auto-scales. */
		SLATE_ATTRIBUTE(float, MaxTimeSec)

		/** Vertical bars to draw (delay echoes, tap delay taps, etc.) */
		SLATE_ATTRIBUTE(TArray<FAudioFXGraphBar>, Bars)

		/** Continuous curves to draw over the grid (reverb envelopes, etc.) */
		SLATE_ATTRIBUTE(TArray<FAudioFXGraphCurve>, Curves)

		/** Asset color — used for 0 dB reference line tinting */
		SLATE_ARGUMENT(FLinearColor, AssetColor)
	SLATE_END_ARGS()

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

	// ── Public statics (testable) ────────────────────────────────────────

	/** dB range constants.
	 *  MinDb = -30: floor is -30 dB. Content above this floor fills the full
	 *  graph height — no wasted dead space for typical reverb/delay signals.
	 *  Every 6 dB grid line (-6, -12, -18, -24, -30) is evenly spaced. */
	static constexpr float MinDb = -30.f;
	static constexpr float MaxDb =   0.f;
	static constexpr float GraphInset = 6.f;

	/** Choose a nice time-grid interval (in seconds) for the given max time. */
	static float ChooseTimeGridInterval(float MaxTimeSec);

	/** Convert linear amplitude (0..1) to dB, clamped to [MinDb, MaxDb]. */
	static float AmplitudeToDb(float LinearAmp);

	/** Map a dB value to normalised Y (0 = top/0dB, 1 = bottom/MinDb). */
	static float DbToNormY(float Db);

private:

	// ============================================================
	// Private paint helpers (called by OnPaint)
	// ============================================================

	int32 PaintGrid(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		float Width, float Height, float MaxTime) const;

	int32 PaintBars(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		float Width, float Height, float MaxTime,
		const TArray<FAudioFXGraphBar>& InBars) const;

	int32 PaintCurves(
		const FGeometry& Geometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		float Width, float Height, float MaxTime,
		const TArray<FAudioFXGraphCurve>& InCurves) const;

	TAttribute<float> MaxTimeSec;
	TAttribute<TArray<FAudioFXGraphBar>> Bars;
	TAttribute<TArray<FAudioFXGraphCurve>> Curves;
	FLinearColor AssetColor = FLinearColor::White;
};
