// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class FAudioFXPreviewEngine;

/**
 * SAudioFXGainReductionMeter
 *
 * Gain reduction meter strip (0 dB at top → MaxDisplayDb at bottom).
 *
 * Reads gain reduction data from FAudioFXPreviewEngine each Tick, applies
 * analogue-style peak-hold and decay, then draws a filled bar in OnPaint.
 *
 * Meter behavior:
 *   - Instantaneous fill: tracks current GR, decays at DecayRateDbPerSec
 *   - Peak hold: held for PeakHoldTime seconds, then also decays
 *   - Color: accent-primary below 20 dB reduction; accent-warn above
 *   - Dims when no active preview data is available
 */
class AUDIOEFFECTSSTUDIO_API SAudioFXGainReductionMeter : public SCompoundWidget
{
public:

	// ============================================================
	// Slate arguments
	// ============================================================

	SLATE_BEGIN_ARGS(SAudioFXGainReductionMeter)
		: _BandIndex(-1)
	{}
		SLATE_ARGUMENT(TSharedPtr<FAudioFXPreviewEngine>, Engine)
		SLATE_ARGUMENT(int32, BandIndex)
	SLATE_END_ARGS()

	// ============================================================
	// Public interface
	// ============================================================

	void Construct(const FArguments& InArgs);

	/** Updates meter levels with decay/hold ballistics each frame. */
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:

	// ============================================================
	// Members
	// ============================================================

	TSharedPtr<FAudioFXPreviewEngine> Engine;
	int32 BandIndex = -1;  // Reserved for future per-band GR support; currently unused

	// Meter state — mutable so OnPaint (const) can update them
	mutable float DisplayReductionDb = 0.0f; // current smoothed GR in dB [0..MaxDisplayDb]
	mutable float PeakReductionDb    = 0.0f; // peak-hold GR in dB [0..MaxDisplayDb]
	mutable float PeakHoldTimer      = 0.0f; // time since peak was last set
	mutable bool  bHasActiveData     = false; // true when the engine is providing live data

	// Meter timing / range constants
	static constexpr float DecayRateDbPerSec = 3.0f;   // fall rate for both fill and peak
	static constexpr float PeakHoldTime      = 1.5f;   // seconds before peak begins to decay
	static constexpr float MaxDisplayDb      = 40.0f;  // full-scale reduction (bottom of bar)
};
