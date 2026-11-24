// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Visualizers/SAudioFXReverbDecayCurve.h"
#include "Shared/Visualizers/SAudioFXTimeDomainGraph.h"
#include "Shared/Core/AudioFXTheme.h"

// ═══════════════════════════════════════════════════════════════════════════
//  Static helpers (unchanged — tests depend on these)
// ═══════════════════════════════════════════════════════════════════════════

float SAudioFXReverbDecayCurve::ClampDecayTime(float DecayTimeSec)
{
	return FMath::Clamp(DecayTimeSec, 0.1f, 20.f);
}

float SAudioFXReverbDecayCurve::ClampDecayHFRatio(float DecayHFRatioValue)
{
	return FMath::Clamp(DecayHFRatioValue, 0.1f, 2.f);
}

float SAudioFXReverbDecayCurve::ClampReflectionsDelay(float ReflectionsDelaySec)
{
	return FMath::Clamp(ReflectionsDelaySec, 0.f, 0.3f);
}

float SAudioFXReverbDecayCurve::ClampLateDelay(float LateDelaySec)
{
	return FMath::Clamp(LateDelaySec, 0.f, 0.1f);
}

float SAudioFXReverbDecayCurve::ClampDensity(float DensityValue)
{
	return FMath::Clamp(DensityValue, 0.f, 1.f);
}

float SAudioFXReverbDecayCurve::ComputeSafeHFDecay(float DecayTimeSec, float DecayHFRatioValue)
{
	return FMath::Clamp(ClampDecayTime(DecayTimeSec) * ClampDecayHFRatio(DecayHFRatioValue), 0.1f, 40.f);
}

float SAudioFXReverbDecayCurve::ComputeMaxTime(float DecayTimeSec)
{
	return FMath::Max(0.25f, ClampDecayTime(DecayTimeSec) * 1.2f);
}

float SAudioFXReverbDecayCurve::ComputeDecayEnvelopeAmplitude(float TimeSec, float DecayTimeSec)
{
	constexpr float Decay60dBConstant = 6.91f;
	return FMath::Exp(-Decay60dBConstant * FMath::Max(TimeSec, 0.f) / ClampDecayTime(DecayTimeSec));
}

float SAudioFXReverbDecayCurve::ComputeLateEnvelopeAmplitude(float TimeSec, float LateStartTimeSec, float DecayTimeSec, float DensityValue)
{
	const float LocalT = FMath::Max(TimeSec - LateStartTimeSec, 0.f);
	const float LateScale = FMath::Lerp(0.35f, 0.75f, ClampDensity(DensityValue));
	return ComputeDecayEnvelopeAmplitude(LocalT, DecayTimeSec) * LateScale * ((TimeSec >= LateStartTimeSec) ? 1.f : 0.f);
}

float SAudioFXReverbDecayCurve::ClampDelayFeedback(float Feedback)
{
	return FMath::Clamp(Feedback, 0.f, 0.99f);
}

int32 SAudioFXReverbDecayCurve::ComputeDelayEchoCount(float DelayTimeSec, float Feedback)
{
	const float SafeFb = ClampDelayFeedback(Feedback);
	if (SafeFb < 0.01f)
	{
		return 1;
	}
	const int32 Count = FMath::CeilToInt(FMath::Loge(0.001f) / FMath::Loge(SafeFb));
	return FMath::Clamp(Count, 1, 32);
}

float SAudioFXReverbDecayCurve::ComputeDelayEchoAmplitude(int32 EchoIndex, float Feedback)
{
	return FMath::Pow(ClampDelayFeedback(Feedback), static_cast<float>(FMath::Max(EchoIndex, 0)));
}

// ═══════════════════════════════════════════════════════════════════════════
//  Construct — embed a SAudioFXTimeDomainGraph as the child
// ═══════════════════════════════════════════════════════════════════════════

void SAudioFXReverbDecayCurve::Construct(const FArguments& InArgs)
{
	DecayTime        = InArgs._DecayTime;
	DecayHFRatio     = InArgs._DecayHFRatio;
	ReflectionsDelay = InArgs._ReflectionsDelay;
	LateDelay        = InArgs._LateDelay;
	Density          = InArgs._Density;
	AssetColor       = InArgs._AssetColor;
	bDelayMode       = InArgs._bDelayMode;
	DelayFeedback    = InArgs._DelayFeedback;

	ChildSlot
	[
		SNew(SAudioFXTimeDomainGraph)
		.AssetColor(AssetColor)
		.MaxTimeSec_Lambda([this]() { return ComputeMaxTimeSec(); })
		.Bars_Lambda([this]() -> TArray<FAudioFXGraphBar>
		{
			if (bDelayMode.Get(false))
			{
				return BuildDelayBars();
			}
			return TArray<FAudioFXGraphBar>();
		})
		.Curves_Lambda([this]() -> TArray<FAudioFXGraphCurve>
		{
			if (!bDelayMode.Get(false))
			{
				return BuildReverbCurves();
			}
			return TArray<FAudioFXGraphCurve>();
		})
	];
}

// ═══════════════════════════════════════════════════════════════════════════
//  Data builders
// ═══════════════════════════════════════════════════════════════════════════

float SAudioFXReverbDecayCurve::ComputeMaxTimeSec() const
{
	if (bDelayMode.Get(false))
	{
		const float SafeDelayTime = FMath::Max(DecayTime.Get(0.5f), 0.001f);
		const float SafeFeedback  = ClampDelayFeedback(DelayFeedback.Get(0.5f));
		const int32 EchoCount     = ComputeDelayEchoCount(SafeDelayTime, SafeFeedback);

		// Find the last echo still above the -30 dB visible floor.
		constexpr float MinAudibleDb = -30.f;
		int32 LastAudibleIndex = 0;
		for (int32 i = 1; i <= EchoCount; ++i)
		{
			const float Amp    = ComputeDelayEchoAmplitude(i, SafeFeedback);
			const float GainDb = SAudioFXTimeDomainGraph::AmplitudeToDb(Amp);
			if (GainDb < MinAudibleDb) { break; }
			LastAudibleIndex = i;
		}
		// Add a small margin (one extra delay period) so the last bar isn't at the right edge.
		return SafeDelayTime * static_cast<float>(LastAudibleIndex + 1);
	}
	return ComputeMaxTime(DecayTime.Get(1.2f));
}

TArray<FAudioFXGraphBar> SAudioFXReverbDecayCurve::BuildDelayBars() const
{
	const float SafeDelayTime = FMath::Max(DecayTime.Get(0.5f), 0.001f);
	const float SafeFeedback  = ClampDelayFeedback(DelayFeedback.Get(0.5f));
	const int32 EchoCount     = ComputeDelayEchoCount(SafeDelayTime, SafeFeedback);

	constexpr float MinAudibleDb = -30.f;

	TArray<FAudioFXGraphBar> Out;
	Out.Reserve(EchoCount + 1);

	for (int32 EchoIndex = 0; EchoIndex <= EchoCount; ++EchoIndex)
	{
		// EchoIndex 0 = dry/input (amplitude 1.0); subsequent echoes decay by Feedback
		const float Amp   = (EchoIndex == 0) ? 1.f : ComputeDelayEchoAmplitude(EchoIndex, SafeFeedback);
		const float GainDb = SAudioFXTimeDomainGraph::AmplitudeToDb(Amp);

		// Skip echoes that are below the audible threshold — they add no visual value.
		if (GainDb < MinAudibleDb)
		{
			break;
		}

		FAudioFXGraphBar Bar;
		Bar.TimeSec = SafeDelayTime * static_cast<float>(EchoIndex);
		Bar.GainDb  = GainDb;
		Bar.Color   = AssetColor;
		Out.Add(MoveTemp(Bar));
	}
	return Out;
}

TArray<FAudioFXGraphCurve> SAudioFXReverbDecayCurve::BuildReverbCurves() const
{
	constexpr int32 NumPoints = 80;

	const float SafeDecay            = ClampDecayTime(DecayTime.Get(1.2f));
	const float SafeHFDecay          = ComputeSafeHFDecay(SafeDecay, DecayHFRatio.Get(0.83f));
	const float SafeReflectionsDelay = ClampReflectionsDelay(ReflectionsDelay.Get(0.01f));
	const float SafeLateDelay        = ClampLateDelay(LateDelay.Get(0.01f));
	const float SafeDensity          = ClampDensity(Density.Get(0.85f));
	const float LateStartTime        = SafeReflectionsDelay + SafeLateDelay;
	const float MaxTime              = ComputeMaxTime(SafeDecay);

	const FLinearColor MainColor = AssetColor.CopyWithNewOpacity(0.95f);

	TArray<FAudioFXGraphCurve> Out;

	// 1. Main decay envelope (glow + solid)
	{
		FAudioFXGraphCurve Curve;
		Curve.Color     = MainColor;
		Curve.Thickness = 2.2f;
		Curve.bGlow     = true;
		Curve.Points.Reserve(NumPoints);
		for (int32 I = 0; I < NumPoints; ++I)
		{
			const float T = MaxTime * (static_cast<float>(I) / static_cast<float>(NumPoints - 1));
			const float A = ComputeDecayEnvelopeAmplitude(T, SafeDecay);
			Curve.Points.Add(FVector2f(T, A));
		}
		Out.Add(MoveTemp(Curve));
	}

	// 2. Late reflections envelope
	{
		FAudioFXGraphCurve Curve;
		Curve.Color     = MainColor.CopyWithNewOpacity(0.65f);
		Curve.Thickness = 1.5f;
		Curve.bGlow     = true;
		Curve.Points.Reserve(NumPoints);
		for (int32 I = 0; I < NumPoints; ++I)
		{
			const float T = MaxTime * (static_cast<float>(I) / static_cast<float>(NumPoints - 1));
			const float A = ComputeLateEnvelopeAmplitude(T, LateStartTime, SafeDecay, SafeDensity);
			Curve.Points.Add(FVector2f(T, A));
		}
		Out.Add(MoveTemp(Curve));
	}

	// 3. HF decay (dashed)
	{
		FAudioFXGraphCurve Curve;
		Curve.Color     = MainColor.CopyWithNewOpacity(0.55f);
		Curve.Thickness = 1.2f;
		Curve.bDashed   = true;
		Curve.Points.Reserve(NumPoints);
		for (int32 I = 0; I < NumPoints; ++I)
		{
			const float T = MaxTime * (static_cast<float>(I) / static_cast<float>(NumPoints - 1));
			const float A = ComputeDecayEnvelopeAmplitude(T, SafeHFDecay);
			Curve.Points.Add(FVector2f(T, A));
		}
		Out.Add(MoveTemp(Curve));
	}

	return Out;
}
