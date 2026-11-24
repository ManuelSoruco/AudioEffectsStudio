// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class FAudioFXPreviewEngine;

/**
 * SAudioFXMeterStrip
 *
 * Vertical meter strip with three visual layers:
 *   1. RMS fill        — dim background bar (slow, stable)
 *   2. Realtime Peak   — bright ballistic bar (PPM-style attack/decay)
 *   3. Peak Hold tick  — white 2px marker (holds then slowly falls)
 *
 * Bottom readout: single amber numeric dBFS value — historic peak.
 * Click anywhere on strip to reset it.
 */
class AUDIOEFFECTSSTUDIO_API SAudioFXMeterStrip : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAudioFXMeterStrip)
		: _bIsInput(true)
		, _bIsStereo(true)
	{}
		SLATE_ARGUMENT(TSharedPtr<FAudioFXPreviewEngine>, Engine)
		SLATE_ARGUMENT(bool, bIsInput)
		SLATE_ARGUMENT(bool, bIsStereo)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

	/** Click anywhere on strip resets the historic peak hold. */
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	TSharedPtr<FAudioFXPreviewEngine> Engine;
	bool bIsInput  = true;
	bool bIsStereo = true;

	// ── Ballistic display (PPM-style) ─────────────────────────────────────
	mutable float DisplayLevel  = 0.0f;
	mutable float DisplayLevelR = 0.0f;

	// ── Peak hold tick (slow decay after hold) ────────────────────────────
	mutable float PeakLevel  = 0.0f;
	mutable float PeakLevelR = 0.0f;
	mutable float PeakHoldTimer  = 0.0f;
	mutable float PeakHoldTimerR = 0.0f;

	// ── RMS (DSP-smoothed, used for background fill) ──────────────────────
	mutable float RmsLevel  = 0.0f;
	mutable float RmsLevelR = 0.0f;

	// ── Historic peak — never decays, click to reset ──────────────────────
	// Averaged L+R for mono display, tracked separately for stereo.
	mutable float HistoricPeak  = 0.0f; // left / mono
	mutable float HistoricPeakR = 0.0f; // right

	// ── Ballistic constants ───────────────────────────────────────────────
	static constexpr float DecayRate      = 1.7f;  // linear/s — ~1.7s full-scale
	static constexpr float PeakHoldTime   = 2.0f;  // seconds to hold before decay
	static constexpr float PeakDecayRate  = 1.2f;  // linear/s after hold expires

	// ── Numeric readout height (reserved at bottom) ───────────────────────
	static constexpr float ReadoutHeight = 14.0f;  // px for single text line

	// ── Helpers ───────────────────────────────────────────────────────────
	static float LinearToDb(float Linear);
	static FString DbToString(float Db);
};
