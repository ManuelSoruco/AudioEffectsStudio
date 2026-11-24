// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

enum class EAudioFXFilterCurveType : uint8
{
	LowPass,
	HighPass,
	BandPass,
	Notch
};

/**
 * SAudioFXFilterCurve
 *
 * Real-time biquad filter magnitude response display for LP/HP/BP/Notch filters.
 * Drawn entirely in Slate OnPaint and styled to match SAudioFXEQCurve.
 *
 * Layout:
 *   - Dark inset background
 *   - Log-spaced frequency grid (20 Hz … 20 kHz)
 *   - Horizontal ±6, ±12, ±18 dB grid lines
 *   - Bright 0 dB center line
 *   - Wide glow pass beneath the curve
 *   - Solid curve line (CurveColor, 2.25px)
 *
 * CachedEvalFreqs is built once in Construct to avoid per-frame heap allocation.
 */
class AUDIOEFFECTSSTUDIO_API SAudioFXFilterCurve : public SLeafWidget
{
public:

	// ============================================================
	// Slate arguments
	// ============================================================

	SLATE_BEGIN_ARGS(SAudioFXFilterCurve)
		: _FilterType(EAudioFXFilterCurveType::LowPass)
		, _CutoffHz(1000.f)
		, _Q(0.707f)
		, _CurveColor(FLinearColor::White)
	{}
		/** Filter topology to display (LP/HP/BP/Notch) */
		SLATE_ATTRIBUTE(EAudioFXFilterCurveType, FilterType)

		/** Filter cutoff (or center) frequency in Hz */
		SLATE_ATTRIBUTE(float, CutoffHz)

		/** Filter Q (resonance). 0.707 = Butterworth / maximally-flat. */
		SLATE_ATTRIBUTE(float, Q)

		/** Curve color — typically the effect asset color */
		SLATE_ARGUMENT(FLinearColor, CurveColor)
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
	 * Map a frequency in Hz to a normalized [0,1] x position (log scale).
	 * Uses precomputed log2 constants for efficiency.
	 */
	static float FreqToNorm(float Hz)
	{
		return (FMath::Log2(FMath::Max(Hz, 20.f)) - MinLogFrequency) / LogFrequencyRange;
	}

	/**
	 * Compute biquad filter magnitude response in dB at FreqHz.
	 * Uses the RBJ Audio EQ Cookbook single-section biquad for each filter type.
	 * @param FilterType  LP, HP, BP, or Notch
	 * @param FreqHz      Evaluation frequency in Hz
	 * @param CutoffHz    Filter cutoff/center frequency in Hz
	 * @param Q           Filter Q (resonance factor)
	 * @param SampleRate  Sample rate (default 44100 Hz)
	 * @return Magnitude in dB; returns -DisplayDbHard on degenerate input
	 */
	static float MagnitudeDb(
		EAudioFXFilterCurveType FilterType,
		float FreqHz,
		float CutoffHz,
		float Q,
		float SampleRate = 44100.f);

private:

	// ============================================================
	// Members
	// ============================================================

	TAttribute<EAudioFXFilterCurveType> FilterType;
	TAttribute<float> CutoffHz;
	TAttribute<float> Q;
	FLinearColor CurveColor = FLinearColor::White;

	// Display range: ±DisplayDbRange visible, ±DisplayDbHard hard clamp on curve
	static constexpr float DisplayDbRange = 24.f;
	static constexpr float DisplayDbHard  = 30.f;

	// Frequency evaluation point count (pre-allocated in Construct)
	static constexpr int32 NumCurvePoints = 256;

	// Precomputed log2 constants for FreqToNorm (log2(20) and log2(20000)-log2(20))
	static constexpr float MinLogFrequency  = 4.321928095f;  // log2(20)
	static constexpr float LogFrequencyRange = 9.965784073f; // log2(20000) - log2(20)

	// Pre-allocated log-spaced frequency evaluation array — avoids per-frame heap allocation.
	mutable TArray<float> CachedEvalFreqs;
};
