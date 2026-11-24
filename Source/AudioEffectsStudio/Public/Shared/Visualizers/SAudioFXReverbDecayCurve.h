// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class SAudioFXTimeDomainGraph;
struct FAudioFXGraphBar;
struct FAudioFXGraphCurve;

/**
 * SAudioFXReverbDecayCurve
 *
 * Time-domain visualization for reverb decay or delay echoes.
 * Thin wrapper — delegates all painting to SAudioFXTimeDomainGraph.
 *
 * Reverb mode (default): smooth exponential decay envelope with early
 *   reflections spike and HF-decay overlay.
 *
 * Delay mode (bDelayMode = true): vertical bars representing discrete
 *   echoes at multiples of DelayTime, with amplitude decaying by
 *   Feedback per repeat.
 */
class AUDIOEFFECTSSTUDIO_API SAudioFXReverbDecayCurve : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAudioFXReverbDecayCurve)
		: _DecayTime(1.2f)
		, _DecayHFRatio(0.83f)
		, _ReflectionsDelay(0.01f)
		, _LateDelay(0.01f)
		, _Density(0.85f)
		, _AssetColor(FLinearColor::White)
		, _bDelayMode(false)
		, _DelayFeedback(0.5f)
	{}
		SLATE_ATTRIBUTE(float, DecayTime)
		SLATE_ATTRIBUTE(float, DecayHFRatio)
		SLATE_ATTRIBUTE(float, ReflectionsDelay)
		SLATE_ATTRIBUTE(float, LateDelay)
		SLATE_ATTRIBUTE(float, Density)
		SLATE_ARGUMENT(FLinearColor, AssetColor)
		SLATE_ATTRIBUTE(bool,  bDelayMode)
		SLATE_ATTRIBUTE(float, DelayFeedback)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// ============================================================
	// Reverb helpers (public for testing)
	// ============================================================

	static float ClampDecayTime(float DecayTimeSec);
	static float ClampDecayHFRatio(float DecayHFRatioValue);
	static float ClampReflectionsDelay(float ReflectionsDelaySec);
	static float ClampLateDelay(float LateDelaySec);
	static float ClampDensity(float DensityValue);
	static float ComputeSafeHFDecay(float DecayTimeSec, float DecayHFRatioValue);
	static float ComputeMaxTime(float DecayTimeSec);
	static float ComputeDecayEnvelopeAmplitude(float TimeSec, float DecayTimeSec);
	static float ComputeLateEnvelopeAmplitude(float TimeSec, float LateStartTimeSec, float DecayTimeSec, float DensityValue);

	// -- Delay helpers (public for testing) --
	static float ClampDelayFeedback(float Feedback);
	static int32 ComputeDelayEchoCount(float DelayTimeSec, float Feedback);
	static float ComputeDelayEchoAmplitude(int32 EchoIndex, float Feedback);

private:

	// ============================================================
	// Private data builders
	// ============================================================

	/** Build bar data for delay mode. */
	TArray<FAudioFXGraphBar> BuildDelayBars() const;

	/** Build curve data for reverb mode. */
	TArray<FAudioFXGraphCurve> BuildReverbCurves() const;

	/** Compute the dynamic max time for either mode. */
	float ComputeMaxTimeSec() const;

	// ============================================================
	// Members
	// ============================================================

	TAttribute<float> DecayTime;
	TAttribute<float> DecayHFRatio;
	TAttribute<float> ReflectionsDelay;
	TAttribute<float> LateDelay;
	TAttribute<float> Density;
	FLinearColor AssetColor = FLinearColor::White;
	TAttribute<bool>  bDelayMode;
	TAttribute<float> DelayFeedback;
};
