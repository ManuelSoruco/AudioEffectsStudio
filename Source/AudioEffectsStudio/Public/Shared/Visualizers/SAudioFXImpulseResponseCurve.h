// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class UObject;
struct FAudioFXGraphCurve;

/**
 * SAudioFXImpulseResponseCurve
 *
 * Impulse-response envelope visualizer. Uses reflection to extract the
 * largest float array property (the IR samples) and an optional SampleRate
 * property from an arbitrary UObject asset. Falls back to FallbackSampleRate
 * when the asset doesn't expose its own sample rate.
 *
 * The raw samples are peak-envelope-downsampled to MaxPoints points for
 * rendering, then displayed using SAudioFXTimeDomainGraph (amplitude → dB,
 * with glow pass). This keeps the render cost bounded regardless of IR size.
 *
 * Public helpers (ComputeDownsampleStep, BuildEnvelopePoints) are also
 * used by automated tests.
 */
class AUDIOEFFECTSSTUDIO_API SAudioFXImpulseResponseCurve : public SCompoundWidget
{
public:

	// ── Slate arguments ───────────────────────────────────────────────────

	SLATE_BEGIN_ARGS(SAudioFXImpulseResponseCurve)
		: _AssetColor(FLinearColor::White)
		, _FallbackSampleRate(48000.f)
	{}
		SLATE_ATTRIBUTE(const UObject*, ImpulseResponseAsset)
		SLATE_ARGUMENT(FLinearColor, AssetColor)
		SLATE_ARGUMENT(float, FallbackSampleRate)
	SLATE_END_ARGS()

	// ── Public interface ──────────────────────────────────────────────────

	void Construct(const FArguments& InArgs);

	/**
	 * Compute the integer step size used when downsampling IR samples to MaxPoints blocks.
	 * Step = ceil(NumSamples / MaxPoints), minimum 1.
	 */
	static int32 ComputeDownsampleStep(int32 NumSamples, int32 MaxPoints);

	/**
	 * Build a peak-envelope polyline from raw audio samples for rendering.
	 * Divides Samples into blocks of ComputeDownsampleStep() and emits the
	 * peak absolute value of each block as a (TimeSec, Peak) point.
	 * @param Samples     Raw audio samples (mono)
	 * @param SampleRate  Sample rate used to compute time axis
	 * @param MaxPoints   Maximum number of output points (default 256)
	 */
	static TArray<FVector2f> BuildEnvelopePoints(const TArray<float>& Samples, float SampleRate, int32 MaxPoints = 256);

private:

	// ── Private helpers ───────────────────────────────────────────────────

	/** Build curve data for the current IR asset. */
	TArray<FAudioFXGraphCurve> BuildCurves() const;

	/** Compute the total duration of the IR in seconds. */
	float ComputeMaxTimeSec() const;

	// ── Members ───────────────────────────────────────────────────────────

	TAttribute<const UObject*> ImpulseResponseAsset;
	FLinearColor AssetColor = FLinearColor::White;
	float FallbackSampleRate = 48000.f;
};
