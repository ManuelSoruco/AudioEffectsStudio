// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Visualizers/SAudioFXTapDelayGraph.h"
#include "Shared/Visualizers/SAudioFXTimeDomainGraph.h"
#include "Shared/Core/AudioFXTheme.h"

namespace
{
	static constexpr float GridIntervalsMs[] = { 10.f, 25.f, 50.f, 100.f, 250.f, 500.f, 1000.f, 2000.f, 5000.f };
}

// ═══════════════════════════════════════════════════════════════════════════
//  Static helpers (unchanged — tests depend on these)
// ═══════════════════════════════════════════════════════════════════════════

float SAudioFXTapDelayGraph::ChooseGridIntervalMs(float MaxDelayLengthMs)
{
	if (!FMath::IsFinite(MaxDelayLengthMs) || MaxDelayLengthMs <= 0.f)
	{
		return GridIntervalsMs[0];
	}

	// Find the smallest candidate interval that keeps the number of grid lines ≤ 8
	const float SafeMaxDelay = FMath::Max(MaxDelayLengthMs, 1.f);
	for (float GridInterval : GridIntervalsMs)
	{
		if ((SafeMaxDelay / GridInterval) <= 8.f)
		{
			return GridInterval;
		}
	}

	return 10000.f;  // fallback for extreme MaxDelayLength values
}

float SAudioFXTapDelayGraph::GainDbToNormalizedHeight(float GainDb)
{
	// Linear mapping: -60 dB → 0.0 (no bar), +6 dB → 1.0 (full height)
	return FMath::GetMappedRangeValueClamped(FVector2D(-60.f, 6.f), FVector2D(0.f, 1.f), GainDb);
}

// ═══════════════════════════════════════════════════════════════════════════
//  Construct — embed SAudioFXTimeDomainGraph
// ═══════════════════════════════════════════════════════════════════════════

void SAudioFXTapDelayGraph::Construct(const FArguments& InArgs)
{
	MaxDelayLength = InArgs._MaxDelayLength;
	Taps           = InArgs._Taps;
	AssetColor     = InArgs._AssetColor;

	ChildSlot
	[
		SNew(SAudioFXTimeDomainGraph)
		.AssetColor(AssetColor)
		.MaxTimeSec_Lambda([this]()
		{
			// Find the rightmost tap that is audible (> -40 dB) and enabled.
			// This prevents silent taps at large delay times from wasting horizontal space.
			float RightmostMs = 0.f;
			for (const FTapDelayInfo& T : Taps.Get(TArray<FTapDelayInfo>()))
			{
				if (T.TapLineMode != ETapLineMode::Disabled && T.Gain > -30.f)
				{
					RightmostMs = FMath::Max(RightmostMs, T.DelayLength);
				}
			}
			// Fall back to MaxDelayLength if no audible taps exist
			if (RightmostMs < 1.f)
			{
				RightmostMs = MaxDelayLength.Get(DefaultMaxDelayLengthMs);
			}
			return FMath::Max(RightmostMs, 1.f) / 1000.f;
		})
		.Bars_Lambda([this]() -> TArray<FAudioFXGraphBar>
		{
			return BuildBars();
		})
	];
}

// ═══════════════════════════════════════════════════════════════════════════
//  BuildBars — convert FTapDelayInfo to unified bar data
// ═══════════════════════════════════════════════════════════════════════════

TArray<FAudioFXGraphBar> SAudioFXTapDelayGraph::BuildBars() const
{
	const TArray<FTapDelayInfo> LocalTaps = Taps.Get(TArray<FTapDelayInfo>());
	const FLinearColor* Palette = FAudioFXTheme::GetBandPalette();
	const int32 PaletteSize = FAudioFXTheme::NumBandPaletteColors;

	TArray<FAudioFXGraphBar> Out;
	Out.Reserve(LocalTaps.Num());

	for (int32 I = 0; I < LocalTaps.Num(); ++I)
	{
		const FTapDelayInfo& Tap = LocalTaps[I];
		FAudioFXGraphBar Bar;
		Bar.TimeSec  = Tap.DelayLength / 1000.f;   // ms → sec
		Bar.GainDb   = Tap.Gain;                    // Already in dB
		Bar.Color    = Palette[I % PaletteSize];
		// Disabled: either explicitly disabled OR below the floor threshold (-30 dB = MinDb)
		Bar.bDisabled = (Tap.TapLineMode == ETapLineMode::Disabled) || (Tap.Gain <= -30.f);
		Out.Add(MoveTemp(Bar));
	}
	return Out;
}
