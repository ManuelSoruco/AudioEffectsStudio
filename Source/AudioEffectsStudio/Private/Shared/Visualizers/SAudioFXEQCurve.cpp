// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Visualizers/SAudioFXEQCurve.h"
#include "Shared/Core/AudioFXTheme.h"
#include "Rendering/DrawElementTypes.h"
#include "Brushes/SlateRoundedBoxBrush.h"

namespace AudioFXEQCurve
{
	static constexpr float MinRenderableFrequencyHz = 20.f;
	static constexpr float MaxRenderableFrequencyHz = 20000.f;
	static constexpr float MinRenderableBandwidthOctaves = 0.01f;
	static constexpr float MaxRenderableBandwidthOctaves = 8.f;
	static constexpr float MinRenderableGainDb = -60.f;
	static constexpr float MaxRenderableGainDb = 60.f;
}

// ============================================================
// Lifecycle
// ============================================================

void SAudioFXEQCurve::Construct(const FArguments& InArgs)
{
	Settings   = InArgs._Settings;
	BandColors = InArgs._BandColors;
	AssetColor = InArgs._AssetColor;

	// Force the widget to repaint every frame so the curve tracks live settings changes.
	// SLeafWidget with TAttribute lambdas doesn't auto-invalidate when external state changes.
	RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateLambda(
		[](double, float) -> EActiveTimerReturnType
		{
			return EActiveTimerReturnType::Continue;
		}));

	// Pre-build the log-spaced evaluation frequency array once — avoids
	// 256-element heap allocation every frame in OnPaint.
	CachedEvalFreqs.SetNumUninitialized(NumCurvePoints);
	for (int32 Index = 0; Index < NumCurvePoints; ++Index)
	{
		const float Norm = (float)Index / (NumCurvePoints - 1);
		CachedEvalFreqs[Index] = 20.f * FMath::Pow(1000.f, Norm);  // 20..20000 Hz log-spaced
	}
}

FVector2D SAudioFXEQCurve::ComputeDesiredSize(float) const
{
	return FVector2D(620.f, 220.f);
}

// ============================================================
// Band math — peaking EQ biquad magnitude response
// (Audio EQ Cookbook, R. Bristow-Johnson)
// ============================================================

float SAudioFXEQCurve::BandMagnitudeDb(
	float FreqHz, float CenterHz, float BandwidthOctaves, float GainDb, float SampleRate)
{
	if (FMath::IsNearlyZero(GainDb, 0.01f)) return 0.f;
	if (!FMath::IsFinite(SampleRate) || SampleRate <= 0.f) return 0.f;
	if (!FMath::IsFinite(FreqHz) || !FMath::IsFinite(CenterHz) || !FMath::IsFinite(BandwidthOctaves)) return 0.f;

	static constexpr double MinBandwidthOctaves = 0.01;
	static constexpr double MaxBandwidthOctaves = 8.0;
	static constexpr double MinSinW0 = 1e-6;
	static constexpr double MagnitudeEpsilon = 1e-20;
	// Use local high-precision natural-log constants to keep this helper self-contained
	// and avoid dependency on optional engine constant macros across UE versions.
	static constexpr double Ln2 = 0.69314718055994530942;
	static constexpr double Ln10 = 2.30258509299404568402;
	static constexpr double InvLn10 = 1.0 / Ln10;

	// Audio EQ Cookbook bandwidth-in-octaves form (matches Unreal EQ band Bandwidth parameter).
	const double NyquistSafe = static_cast<double>(SampleRate) * 0.499;
	const double SafeCenterHz = FMath::Clamp(static_cast<double>(CenterHz), 20.0, NyquistSafe);
	const double SafeFreqHz = FMath::Clamp(static_cast<double>(FreqHz), 20.0, NyquistSafe);
	const double SafeBandwidth = FMath::Clamp(static_cast<double>(BandwidthOctaves), MinBandwidthOctaves, MaxBandwidthOctaves);

	// Angular frequencies
	const double w0 = 2.0 * PI * SafeCenterHz / static_cast<double>(SampleRate);
	const double w = 2.0 * PI * SafeFreqHz / static_cast<double>(SampleRate);
	const double A = FMath::Pow(10.0, static_cast<double>(GainDb) / 40.0); // sqrt(10^(GainDb/20))
	const double SinW0 = FMath::Sin(w0);
	const double SafeSinW0 = FMath::Max(FMath::Abs(SinW0), MinSinW0);
	// alpha = sin(w0) * sinh((ln(2)/2) * BW * (w0 / sin(w0)))  (RBJ Audio EQ Cookbook, BW in octaves)
	const double AlphaArg = (Ln2 * 0.5) * SafeBandwidth * (w0 / SafeSinW0);
	const double alpha = SinW0 * FMath::Sinh(AlphaArg);

	// Biquad peaking coefficients
	const double b0 =  1.0 + alpha * A;
	const double b1 = -2.0 * FMath::Cos(w0);
	const double b2 =  1.0 - alpha * A;
	const double a0 =  1.0 + alpha / A;
	const double a1 = -2.0 * FMath::Cos(w0);
	const double a2 =  1.0 - alpha / A;

	// H(e^jw) magnitude squared using the frequency-domain evaluation
	const double cosW  = FMath::Cos(w);
	const double cos2W = FMath::Cos(2.0 * w);

	const double NumSq = b0*b0 + b1*b1 + b2*b2
		+ 2.0*(b0*b1 + b1*b2)*cosW
		+ 2.0*b0*b2*cos2W;

	const double DenSq = a0*a0 + a1*a1 + a2*a2
		+ 2.0*(a0*a1 + a1*a2)*cosW
		+ 2.0*a0*a2*cos2W;

	if (!FMath::IsFinite(NumSq) || !FMath::IsFinite(DenSq)
		|| DenSq < MagnitudeEpsilon || NumSq < MagnitudeEpsilon) return 0.f;

	const double MagSq = NumSq / DenSq;
	if (!FMath::IsFinite(MagSq)) return 0.f;
	if (MagSq < MagnitudeEpsilon) return 0.f;

	return static_cast<float>(10.0 * FMath::Loge(MagSq) * InvLn10); // 10*log10(MagSq) = 20*log10(Mag)
}

// ═══════════════════════════════════════════════════════════════════════════
//  Private math helpers
// ═══════════════════════════════════════════════════════════════════════════

float SAudioFXEQCurve::EQDbToY(float Db, float Height)
{
	// Center dB origin at mid-height; Slate Y increases downward, so
	// positive dB (boost) maps to a smaller pixel Y (upward direction).
	const float Norm = FMath::Clamp(Db / DisplayDbRange, -1.f, 1.f);
	return Height * 0.5f - Norm * (Height * 0.5f);
}

float SAudioFXEQCurve::SampleBandDb(const FSourceEffectEQBand& Band, float EvalHz)
{
	// Clamp all band parameters to the renderable range before evaluating
	// the biquad — prevents edge cases (e.g. zero bandwidth, Nyquist overrun).
	const float SafeFreq      = FMath::Clamp(Band.Frequency,  AudioFXEQCurve::MinRenderableFrequencyHz,      AudioFXEQCurve::MaxRenderableFrequencyHz);
	const float SafeBandwidth = FMath::Clamp(Band.Bandwidth,  AudioFXEQCurve::MinRenderableBandwidthOctaves, AudioFXEQCurve::MaxRenderableBandwidthOctaves);
	const float SafeGainDb    = FMath::Clamp(Band.GainDb,     AudioFXEQCurve::MinRenderableGainDb,           AudioFXEQCurve::MaxRenderableGainDb);
	const float Db = BandMagnitudeDb(EvalHz, SafeFreq, SafeBandwidth, SafeGainDb);
	return FMath::IsFinite(Db) ? Db : 0.f;
}

// ═══════════════════════════════════════════════════════════════════════════
//  PaintBackground
// ═══════════════════════════════════════════════════════════════════════════

int32 SAudioFXEQCurve::PaintBackground(
	const FGeometry& Geometry,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId) const
{
	// Dark inset look — FLinearColor::White would render the box bright-white
	// because FSlateRoundedBoxBrush uses a white texture modulated by tint.
	FSlateDrawElement::MakeBox(
		OutDrawElements, LayerId,
		Geometry.ToPaintGeometry(),
		&FAudioFXTheme::GetInsetBrush(),
		ESlateDrawEffect::None,
		FAudioFXTheme::GetWindowBackground());
	return LayerId + 1;
}

// ═══════════════════════════════════════════════════════════════════════════
//  PaintFrequencyGrid — vertical freq lines + horizontal dB lines + 0 dB line
// ═══════════════════════════════════════════════════════════════════════════

int32 SAudioFXEQCurve::PaintFrequencyGrid(
	const FGeometry& Geometry,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	float Width, float Height) const
{
	// ── Vertical frequency grid lines (20 Hz … 20 kHz, log-spaced) ───────
	{
		static const float GridFreqs[] = {
			20.f, 50.f, 100.f, 200.f, 500.f,
			1000.f, 2000.f, 5000.f, 10000.f, 20000.f
		};
		const FLinearColor GridColor = FAudioFXTheme::GetGridMinorColor();
		for (float Freq : GridFreqs)
		{
			const float X = FreqToNorm(Freq) * Width;
			TArray<FVector2f> VLine = { FVector2f(X, 0.f), FVector2f(X, Height) };
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
				Geometry.ToPaintGeometry(),
				VLine, ESlateDrawEffect::None, GridColor, false, 0.5f);
		}

		// Horizontal dB grid lines at ±6, ±12, ±18 dB (0 dB drawn separately below)
		static const float DbGridLines[] = { -18.f, -12.f, -6.f, 6.f, 12.f, 18.f };
		const FLinearColor DbGridColor = FAudioFXTheme::GetGridMinorColor();
		for (float Db : DbGridLines)
		{
			const float Y = EQDbToY(Db, Height);
			TArray<FVector2f> HLine = { FVector2f(0.f, Y), FVector2f(Width, Y) };
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
				Geometry.ToPaintGeometry(),
				HLine, ESlateDrawEffect::None, DbGridColor, false, 0.5f);
		}
	}

	// ── 0 dB center line (brighter than the minor lines — visual anchor) ──
	{
		const float CenterY = Height * 0.5f;
		TArray<FVector2f> ZeroLine = { FVector2f(0.f, CenterY), FVector2f(Width, CenterY) };
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
			Geometry.ToPaintGeometry(),
			ZeroLine, ESlateDrawEffect::None, FAudioFXTheme::GetGridMajorColor(), false, 1.0f);
	}
	return LayerId + 1;
}

// ═══════════════════════════════════════════════════════════════════════════
//  OnPaint — orchestrator
// ═══════════════════════════════════════════════════════════════════════════

int32 SAudioFXEQCurve::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const FSourceEffectEQSettings EQSettings = Settings.Get();
	const TArray<FLinearColor> Colors = BandColors.Get(TArray<FLinearColor>());
	const FVector2f Size = UE::Slate::CastToVector2f(AllottedGeometry.GetLocalSize());
	const float Width  = Size.X;
	const float Height = Size.Y;

	// ── Background ────────────────────────────────────────────────────────
	LayerId = PaintBackground(AllottedGeometry, OutDrawElements, LayerId);

	// ── Frequency + dB grid + 0 dB center line ────────────────────────────
	// CachedEvalFreqs is pre-built in Construct — no heap allocation per frame.
	LayerId = PaintFrequencyGrid(AllottedGeometry, OutDrawElements, LayerId, Width, Height);

	// ── Per-band curves and combined sum curve (skip when there are no bands) ──
	if (EQSettings.EQBands.Num() > 0)
	{
		LayerId = PaintBandCurves(AllottedGeometry, OutDrawElements, LayerId, Width, Height, EQSettings, Colors);
		LayerId = PaintSumCurve(AllottedGeometry, OutDrawElements, LayerId, Width, Height, EQSettings);
	}

	return LayerId;
}

// ============================================================
// Private paint helpers
// ============================================================

int32 SAudioFXEQCurve::PaintBandCurves(
	const FGeometry& Geometry,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	float Width, float Height,
	const FSourceEffectEQSettings& EQSettings,
	const TArray<FLinearColor>& Colors) const
{
	const int32 NumBands    = EQSettings.EQBands.Num();
	const int32 PaletteSize = FAudioFXTheme::NumBandPaletteColors;

	// Use pre-built CachedEvalFreqs — no heap allocation per frame.
	const TArray<float>& EvalFreqs = CachedEvalFreqs;

	for (int32 BandIdx = 0; BandIdx < NumBands; ++BandIdx)
	{
		const FSourceEffectEQBand& Band = EQSettings.EQBands[BandIdx];
		if (!Band.bEnabled || FMath::IsNearlyZero(Band.GainDb, 0.05f)) continue;

		// Use NumBandPaletteColors instead of hardcoded 8
		const int32 ColorIdx = BandIdx % PaletteSize;
		const FLinearColor BandCol = Colors.IsValidIndex(ColorIdx)
			? FLinearColor(Colors[ColorIdx].R, Colors[ColorIdx].G, Colors[ColorIdx].B, 0.55f)
			: FLinearColor(0.5f, 0.5f, 0.7f, 0.55f);

		// Glow pass first (wider, more transparent)
		{
			TArray<FVector2f> GlowPts;
			GlowPts.Reserve(NumCurvePoints);
			for (int32 PointIndex = 0; PointIndex < NumCurvePoints; ++PointIndex)
			{
				const float Db = SampleBandDb(Band, EvalFreqs[PointIndex]);
				GlowPts.Add(FVector2f(FreqToNorm(EvalFreqs[PointIndex]) * Width,
					EQDbToY(FMath::Clamp(Db, -DisplayDbHard, DisplayDbHard), Height)));
			}
			const FLinearColor GlowCol(BandCol.R, BandCol.G, BandCol.B, 0.15f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
				Geometry.ToPaintGeometry(),
				GlowPts, ESlateDrawEffect::None, GlowCol, true, 6.f);
		}

		// Main per-band curve
		{
			TArray<FVector2f> BandPts;
			BandPts.Reserve(NumCurvePoints);
			for (int32 PointIndex = 0; PointIndex < NumCurvePoints; ++PointIndex)
			{
				const float Db = SampleBandDb(Band, EvalFreqs[PointIndex]);
				BandPts.Add(FVector2f(FreqToNorm(EvalFreqs[PointIndex]) * Width,
					EQDbToY(FMath::Clamp(Db, -DisplayDbHard, DisplayDbHard), Height)));
			}
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
				Geometry.ToPaintGeometry(),
				BandPts, ESlateDrawEffect::None, BandCol, true, 1.8f);
		}
	}

	return LayerId + 1;
}

int32 SAudioFXEQCurve::PaintSumCurve(
	const FGeometry& Geometry,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	float Width, float Height,
	const FSourceEffectEQSettings& EQSettings) const
{
	const int32 NumBands       = EQSettings.EQBands.Num();
	const TArray<float>& EvalFreqs = CachedEvalFreqs;

	// Sum curve glow (wide soft pass)
	{
		TArray<FVector2f> SumGlowPts;
		SumGlowPts.Reserve(NumCurvePoints);
		for (int32 PointIndex = 0; PointIndex < NumCurvePoints; ++PointIndex)
		{
			float SumDb = 0.f;
			for (int32 BandIdx = 0; BandIdx < NumBands; ++BandIdx)
			{
				const FSourceEffectEQBand& Band = EQSettings.EQBands[BandIdx];
				if (Band.bEnabled)
					SumDb += SampleBandDb(Band, EvalFreqs[PointIndex]);
			}
			SumGlowPts.Add(FVector2f(FreqToNorm(EvalFreqs[PointIndex]) * Width,
				EQDbToY(FMath::Clamp(SumDb, -DisplayDbHard, DisplayDbHard), Height)));
		}
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
			Geometry.ToPaintGeometry(),
			SumGlowPts, ESlateDrawEffect::None, FAudioFXTheme::GetVisualizationGlowColor(), true, 10.f);
	}
	LayerId++;

	// Sum curve solid (elegant gray, 2px antialiased)
	{
		TArray<FVector2f> SumPts;
		SumPts.Reserve(NumCurvePoints);
		for (int32 PointIndex = 0; PointIndex < NumCurvePoints; ++PointIndex)
		{
			float SumDb = 0.f;
			for (int32 BandIdx = 0; BandIdx < NumBands; ++BandIdx)
			{
				const FSourceEffectEQBand& Band = EQSettings.EQBands[BandIdx];
				if (Band.bEnabled)
					SumDb += SampleBandDb(Band, EvalFreqs[PointIndex]);
			}
			SumPts.Add(FVector2f(FreqToNorm(EvalFreqs[PointIndex]) * Width,
				EQDbToY(FMath::Clamp(SumDb, -DisplayDbHard, DisplayDbHard), Height)));
		}
		// Theme gray (#A8A8B2) for an elegant sum curve
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
			Geometry.ToPaintGeometry(),
			SumPts, ESlateDrawEffect::None,
			FAudioFXTheme::GetVisualizationCurveColor().CopyWithNewOpacity(0.90f), true, 2.5f);
	}

	return LayerId + 1;
}
