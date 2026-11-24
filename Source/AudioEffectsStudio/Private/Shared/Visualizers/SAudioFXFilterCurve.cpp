// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Visualizers/SAudioFXFilterCurve.h"
#include "Shared/Core/AudioFXTheme.h"

#include "Rendering/DrawElementTypes.h"

// ============================================================
// Lifecycle
// ============================================================

void SAudioFXFilterCurve::Construct(const FArguments& InArgs)
{
	FilterType  = InArgs._FilterType;
	CutoffHz    = InArgs._CutoffHz;
	Q           = InArgs._Q;
	CurveColor  = InArgs._CurveColor;

	// Pre-build log-spaced evaluation frequencies once — avoids per-frame allocation.
	// Range: 20 Hz … 20 kHz (256 points).
	CachedEvalFreqs.SetNumUninitialized(NumCurvePoints);
	for (int32 Index = 0; Index < NumCurvePoints; ++Index)
	{
		const float Norm = static_cast<float>(Index) / static_cast<float>(NumCurvePoints - 1);
		CachedEvalFreqs[Index] = 20.f * FMath::Pow(1000.f, Norm);
	}
}

FVector2D SAudioFXFilterCurve::ComputeDesiredSize(float) const
{
	return FVector2D(620.f, 160.f);
}

// ═══════════════════════════════════════════════════════════════════════════
//  MagnitudeDb — RBJ biquad filter frequency response
// ═══════════════════════════════════════════════════════════════════════════

float SAudioFXFilterCurve::MagnitudeDb(
	EAudioFXFilterCurveType InFilterType,
	float FreqHz,
	float InCutoffHz,
	float InQ,
	float SampleRate)
{
	// log2(10) — used to convert log2 magnitude to dB: 10*log2(MagSq)/log2(10)
	static constexpr float DecibelDenominator = 3.321928095f;
	static constexpr float MagnitudeEpsilon   = 1e-12f;
	if (!FMath::IsFinite(SampleRate) || SampleRate <= 0.f)
	{
		return -DisplayDbHard;
	}

	// Clamp all inputs to safe, numerically-stable ranges
	const float SafeCutoff = FMath::Clamp(InCutoffHz, 20.f, SampleRate * 0.499f);
	const float SafeFreq   = FMath::Clamp(FreqHz,     20.f, SampleRate * 0.499f);
	const float SafeQ      = FMath::Max(InQ, 0.05f);  // avoid division by zero

	// Pre-compute shared biquad terms (RBJ Audio EQ Cookbook notation)
	const float W0    = 2.f * PI * SafeCutoff / SampleRate;  // cutoff angular frequency
	const float W     = 2.f * PI * SafeFreq   / SampleRate;  // eval angular frequency
	const float Alpha = FMath::Sin(W0) / (2.f * SafeQ);      // bandwidth factor
	const float CosW0 = FMath::Cos(W0);

	// Default biquad denominator (shared by all filter types)
	float B0 = 1.f, B1 = 0.f, B2 = 0.f;
	float A0 = 1.f + Alpha;
	float A1 = -2.f * CosW0;
	float A2 = 1.f - Alpha;

	// Numerator coefficients differ per filter type
	switch (InFilterType)
	{
	case EAudioFXFilterCurveType::LowPass:
		B0 = (1.f - CosW0) * 0.5f;
		B1 = 1.f - CosW0;
		B2 = (1.f - CosW0) * 0.5f;
		break;

	case EAudioFXFilterCurveType::HighPass:
		B0 = (1.f + CosW0) * 0.5f;
		B1 = -(1.f + CosW0);
		B2 = (1.f + CosW0) * 0.5f;
		break;

	case EAudioFXFilterCurveType::BandPass:
		B0 =  Alpha;
		B1 =  0.f;
		B2 = -Alpha;
		break;

	case EAudioFXFilterCurveType::Notch:
		B0 = 1.f;
		B1 = -2.f * CosW0;
		B2 = 1.f;
		break;
	}

	// Evaluate |H(e^jw)|^2 using the frequency-domain polynomial form:
	// |H|^2 = (b0^2+b1^2+b2^2 + 2*(b0*b1+b1*b2)*cos(w) + 2*b0*b2*cos(2w)) / (same with a coeffs)
	const float CosW  = FMath::Cos(W);
	const float Cos2W = FMath::Cos(2.f * W);
	const float NumSq = B0*B0 + B1*B1 + B2*B2
		+ 2.f*(B0*B1 + B1*B2)*CosW
		+ 2.f*B0*B2*Cos2W;
	const float DenSq = A0*A0 + A1*A1 + A2*A2
		+ 2.f*(A0*A1 + A1*A2)*CosW
		+ 2.f*A0*A2*Cos2W;

	if (!FMath::IsFinite(NumSq) || !FMath::IsFinite(DenSq)
		|| DenSq < MagnitudeEpsilon || NumSq < MagnitudeEpsilon)
	{
		return -DisplayDbHard;
	}

	const float MagSq = NumSq / DenSq;
	if (!FMath::IsFinite(MagSq) || MagSq < MagnitudeEpsilon)
	{
		return -DisplayDbHard;
	}

	// 10 * log2(MagSq) / log2(10) = 10 * log10(MagSq) = 20 * log10(|H|)
	return 10.f * FMath::Log2(MagSq) / DecibelDenominator;
}

// ═══════════════════════════════════════════════════════════════════════════
//  OnPaint
// ═══════════════════════════════════════════════════════════════════════════

int32 SAudioFXFilterCurve::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const FVector2f Size = UE::Slate::CastToVector2f(AllottedGeometry.GetLocalSize());
	const float Width  = Size.X;
	const float Height = Size.Y;

	// Map normalized [0,1] → pixel X; map dB → pixel Y (0 dB = center)
	auto NormToX = [Width](float Norm) { return Norm * Width; };
	auto DbToY   = [Height](float Db)
	{
		// 0 dB → center, positive dB → above center; Slate Y is inverted
		const float Norm = FMath::Clamp(Db / DisplayDbRange, -1.f, 1.f);
		return Height * 0.5f - Norm * (Height * 0.5f);
	};

	// ── Layer 0: dark inset background ───────────────────────────────────
	FSlateDrawElement::MakeBox(
		OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(),
		&FAudioFXTheme::GetInsetBrush(),
		ESlateDrawEffect::None,
		FAudioFXTheme::GetWindowBackground());
	++LayerId;

	// ── Layer 1: frequency + dB grid lines ───────────────────────────────
	{
		static const float GridFreqs[] = { 20.f, 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f, 20000.f };
		for (float GridFreq : GridFreqs)
		{
			const float X = NormToX(FreqToNorm(GridFreq));
			const TArray<FVector2f> VLine = { FVector2f(X, 0.f), FVector2f(X, Height) };
			FSlateDrawElement::MakeLines(
				OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), VLine,
				ESlateDrawEffect::None, FAudioFXTheme::GetGridMinorColor(), false, 0.5f);
		}

		// Horizontal dB lines at ±6, ±12, ±18 (0 dB drawn on a separate brighter layer)
		static const float DbGridLines[] = { -18.f, -12.f, -6.f, 6.f, 12.f, 18.f };
		for (float Db : DbGridLines)
		{
			const float Y = DbToY(Db);
			const TArray<FVector2f> HLine = { FVector2f(0.f, Y), FVector2f(Width, Y) };
			FSlateDrawElement::MakeLines(
				OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), HLine,
				ESlateDrawEffect::None, FAudioFXTheme::GetGridMinorColor(), false, 0.5f);
		}
	}
	++LayerId;

	// ── Layer 2: 0 dB center reference line (brighter) ───────────────────
	{
		const float CenterY = Height * 0.5f;
		const TArray<FVector2f> ZeroLine = { FVector2f(0.f, CenterY), FVector2f(Width, CenterY) };
		FSlateDrawElement::MakeLines(
			OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), ZeroLine,
			ESlateDrawEffect::None, FAudioFXTheme::GetGridMajorColor(), false, 1.0f);
	}
	++LayerId;

	// ── Layers 3–4: curve glow + solid curve ─────────────────────────────
	// Build curve points once and reuse for both the glow and the main line.
	// CachedEvalFreqs is pre-built in Construct — no heap allocation per frame.
	TArray<FVector2f> CurvePoints;
	CurvePoints.Reserve(NumCurvePoints);

	const EAudioFXFilterCurveType CurrentType   = FilterType.Get(EAudioFXFilterCurveType::LowPass);
	const float                   CurrentCutoff = CutoffHz.Get(1000.f);
	const float                   CurrentQ      = Q.Get(0.707f);
	for (int32 Index = 0; Index < NumCurvePoints; ++Index)
	{
		const float Db = MagnitudeDb(CurrentType, CachedEvalFreqs[Index], CurrentCutoff, CurrentQ);
		CurvePoints.Add(FVector2f(
			NormToX(FreqToNorm(CachedEvalFreqs[Index])),
			DbToY(FMath::Clamp(Db, -DisplayDbHard, DisplayDbHard))));
	}

	// Glow pass — wide soft bloom under the curve
	FSlateDrawElement::MakeLines(
		OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), CurvePoints,
		ESlateDrawEffect::None,
		FAudioFXTheme::GetVisualizationGlowColor(),
		true, 8.0f);
	++LayerId;

	// Main curve — solid, 2.25px, asset color
	FSlateDrawElement::MakeLines(
		OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), CurvePoints,
		ESlateDrawEffect::None,
		CurveColor.CopyWithNewOpacity(0.95f),
		true, 2.25f);

	return LayerId;
}
