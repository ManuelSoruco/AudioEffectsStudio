// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "SubmixEffects/SubmixEffectTapDelay.h"

struct FAudioFXGraphBar;

/**
 * SAudioFXTapDelayGraph
 *
 * Tap-delay visualization. Thin wrapper — delegates all painting
 * to SAudioFXTimeDomainGraph. Converts FTapDelayInfo array to
 * FAudioFXGraphBar array with per-tap colors from the band palette.
 */
class AUDIOEFFECTSSTUDIO_API SAudioFXTapDelayGraph : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAudioFXTapDelayGraph)
		: _MaxDelayLength(4000.f)
		, _AssetColor(FLinearColor::White)
	{}
		SLATE_ATTRIBUTE(float, MaxDelayLength)
		SLATE_ATTRIBUTE(TArray<FTapDelayInfo>, Taps)
		SLATE_ARGUMENT(FLinearColor, AssetColor)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// ============================================================
	// Public statics (also used by tests)
	// ============================================================

	static constexpr float DefaultMaxDelayLengthMs = 4000.f;

	/** Pick a round-number grid interval (ms) so the graph shows ≤ 8 vertical lines. */
	static float ChooseGridIntervalMs(float MaxDelayLengthMs);

	/** Map a tap's gain dB value to a normalized bar height in [0, 1]. */
	static float GainDbToNormalizedHeight(float GainDb);

private:

	// ============================================================
	// Private helpers
	// ============================================================

	/** Build bar data from taps. */
	TArray<FAudioFXGraphBar> BuildBars() const;

	// ============================================================
	// Members
	// ============================================================

	TAttribute<float> MaxDelayLength;
	TAttribute<TArray<FTapDelayInfo>> Taps;
	FLinearColor AssetColor = FLinearColor::White;
};
