// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Visualizers/SAudioFXImpulseResponseCurve.h"
#include "Shared/Visualizers/SAudioFXTimeDomainGraph.h"

#include "UObject/Field.h"
#include "UObject/UnrealType.h"

namespace AudioFXImpulseResponseCurve
{
	/**
	 * Extract impulse-response data from an arbitrary UObject asset via reflection.
	 * Searches for:
	 *   - A float or int32 property named "SampleRate" (case-insensitive) → OutSampleRate
	 *   - The largest TArray<float> property → OutSamples (assumed to be the IR channel data)
	 * Returns true if at least one sample was found.
	 * Works with any SoundWave-derived asset that exposes a TArray<float> samples property
	 * and a SampleRate numeric property.
	 */
	static bool ExtractImpulseResponseData(const UObject* Asset, TArray<float>& OutSamples, float& OutSampleRate)
	{
		OutSamples.Reset();
		OutSampleRate = 0.f;
		if (!IsValid(Asset))
		{
			return false;
		}

		for (TFieldIterator<FProperty> It(Asset->GetClass()); It; ++It)
		{
			const FProperty* Property = *It;
			if (!Property)
			{
				continue;
			}

			const FString PropertyName = Property->GetName();
			if (const FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
			{
				if (PropertyName.Contains(TEXT("SampleRate"), ESearchCase::IgnoreCase))
				{
					OutSampleRate = FloatProp->GetPropertyValue_InContainer(Asset);
				}
				continue;
			}

			if (const FIntProperty* IntProp = CastField<FIntProperty>(Property))
			{
				if (PropertyName.Contains(TEXT("SampleRate"), ESearchCase::IgnoreCase))
				{
					OutSampleRate = static_cast<float>(IntProp->GetPropertyValue_InContainer(Asset));
				}
				continue;
			}

			const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property);
			if (!ArrayProp || !ArrayProp->Inner || !ArrayProp->Inner->IsA(FFloatProperty::StaticClass()))
			{
				continue;
			}

			// Collect all elements of this float array into a candidate buffer
			FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Asset));
			if (Helper.Num() == 0)
			{
				continue;
			}

			TArray<float> Candidate;
			Candidate.Reserve(Helper.Num());
			const FFloatProperty* InnerFloat = CastField<FFloatProperty>(ArrayProp->Inner);
			if (!InnerFloat)
			{
				continue;
			}
			for (int32 Index = 0; Index < Helper.Num(); ++Index)
			{
				Candidate.Add(InnerFloat->GetFloatingPointPropertyValue(Helper.GetRawPtr(Index)));
			}

			if (Candidate.Num() > OutSamples.Num())
			{
				// Keep the largest array — it is most likely to be the full IR buffer
				OutSamples = MoveTemp(Candidate);
			}
		}

		return OutSamples.Num() > 0;
	}
}

// ═══════════════════════════════════════════════════════════════════════════
//  Construct — embed SAudioFXTimeDomainGraph as the child
// ═══════════════════════════════════════════════════════════════════════════

void SAudioFXImpulseResponseCurve::Construct(const FArguments& InArgs)
{
	ImpulseResponseAsset = InArgs._ImpulseResponseAsset;
	AssetColor = InArgs._AssetColor;
	FallbackSampleRate = FMath::Max(InArgs._FallbackSampleRate, 1.f);

	ChildSlot
	[
		SNew(SAudioFXTimeDomainGraph)
		.AssetColor(AssetColor)
		.MaxTimeSec_Lambda([this]() { return ComputeMaxTimeSec(); })
		.Curves_Lambda([this]() { return BuildCurves(); })
	];
}

// ═══════════════════════════════════════════════════════════════════════════
//  Public helpers
// ═══════════════════════════════════════════════════════════════════════════

int32 SAudioFXImpulseResponseCurve::ComputeDownsampleStep(int32 NumSamples, int32 MaxPoints)
{
	const int32 SafePoints = FMath::Max(MaxPoints, 1);
	return FMath::Max(1, FMath::CeilToInt(static_cast<float>(FMath::Max(NumSamples, 0)) / static_cast<float>(SafePoints)));
}

TArray<FVector2f> SAudioFXImpulseResponseCurve::BuildEnvelopePoints(const TArray<float>& Samples, float SampleRate, int32 MaxPoints)
{
	TArray<FVector2f> Out;
	const float SafeSampleRate = FMath::Max(SampleRate, 1.f);
	if (Samples.IsEmpty())
	{
		return Out;
	}

	const int32 Step = ComputeDownsampleStep(Samples.Num(), MaxPoints);
	const int32 NumBlocks = FMath::CeilToInt(static_cast<float>(Samples.Num()) / static_cast<float>(Step));
	Out.Reserve(NumBlocks);

	for (int32 BlockIndex = 0; BlockIndex < NumBlocks; ++BlockIndex)
	{
		const int32 StartIndex = BlockIndex * Step;
		const int32 EndIndex   = FMath::Min(StartIndex + Step, Samples.Num());

		// Peak-pick over each block to form a smooth amplitude envelope
		float Peak = 0.f;
		for (int32 Index = StartIndex; Index < EndIndex; ++Index)
		{
			Peak = FMath::Max(Peak, FMath::Abs(Samples[Index]));
		}

		const float TimeSec = static_cast<float>(StartIndex) / SafeSampleRate;
		Out.Add(FVector2f(TimeSec, FMath::Clamp(Peak, 0.f, 1.f)));
	}

	return Out;
}

// ═══════════════════════════════════════════════════════════════════════════
//  Private builders
// ═══════════════════════════════════════════════════════════════════════════

TArray<FAudioFXGraphCurve> SAudioFXImpulseResponseCurve::BuildCurves() const
{
	TArray<float> Samples;
	float SampleRate = 0.f;
	AudioFXImpulseResponseCurve::ExtractImpulseResponseData(ImpulseResponseAsset.Get(nullptr), Samples, SampleRate);

	FAudioFXGraphCurve Curve;
	Curve.Color = AssetColor.CopyWithNewOpacity(0.9f);
	Curve.Thickness = 1.8f;
	Curve.bGlow = true;
	Curve.Points = BuildEnvelopePoints(Samples, (SampleRate > 1.f) ? SampleRate : FallbackSampleRate);

	// Need at least 2 points for MakeLines to draw anything
	if (Curve.Points.Num() <= 1)
	{
		return TArray<FAudioFXGraphCurve>();
	}

	return TArray<FAudioFXGraphCurve>{ MoveTemp(Curve) };
}

float SAudioFXImpulseResponseCurve::ComputeMaxTimeSec() const
{
	TArray<float> Samples;
	float SampleRate = 0.f;
	AudioFXImpulseResponseCurve::ExtractImpulseResponseData(ImpulseResponseAsset.Get(nullptr), Samples, SampleRate);

	const float EffectiveRate = (SampleRate > 1.f) ? SampleRate : FallbackSampleRate;
	if (Samples.IsEmpty())
	{
		return 1.f;
	}

	// Clip max time to where the IR drops below -40 dB (amplitude threshold 0.01)
	// so we don't render the inaudible tail.
	constexpr float MinAudibleAmplitude = 0.01f;  // = 10^(-40/20)
	int32 LastAudibleIndex = 0;
	for (int32 I = Samples.Num() - 1; I >= 0; --I)
	{
		if (FMath::Abs(Samples[I]) >= MinAudibleAmplitude)
		{
			LastAudibleIndex = I;
			break;
		}
	}

	// Add a small margin (5% of the audible duration) so the curve doesn't clip right at the edge.
	const float AudibleTimeSec = static_cast<float>(LastAudibleIndex + 1) / EffectiveRate;
	return FMath::Max(AudibleTimeSec * 1.05f, 0.05f);
}
