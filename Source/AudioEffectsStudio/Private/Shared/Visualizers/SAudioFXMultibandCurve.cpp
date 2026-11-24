// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Visualizers/SAudioFXMultibandCurve.h"

#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Visualizers/SAudioFXEQCurve.h"
#include "Rendering/DrawElementTypes.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"

// ============================================================
// Lifecycle
// ============================================================

void SAudioFXMultibandCurve::Construct(const FArguments& InArgs)
{
	Settings = InArgs._Settings;
	BandColors = InArgs._BandColors;
	AssetColor = InArgs._AssetColor;
}

FVector2D SAudioFXMultibandCurve::ComputeDesiredSize(float /*LayoutScaleMultiplier*/) const
{
	return FVector2D(400.f, 80.f);
}

// ============================================================
// Public statics
// ============================================================

float SAudioFXMultibandCurve::FreqToNorm(float Hz)
{
	return SAudioFXEQCurve::FreqToNorm(Hz);
}

float SAudioFXMultibandCurve::EstimateOverlapHalfOctaves(bool bFourPole)
{
	return bFourPole ? 0.35f : 0.5f;
}

float SAudioFXMultibandCurve::ComputeEdgeBlend(float DistanceOctaves, float HalfWidthOctaves)
{
	if (!FMath::IsFinite(DistanceOctaves) || !FMath::IsFinite(HalfWidthOctaves) || HalfWidthOctaves <= SMALL_NUMBER)
	{
		return 1.f;
	}

	const float T = FMath::Clamp(DistanceOctaves / HalfWidthOctaves, 0.f, 1.f);
	return FMath::SmoothStep(0.f, 1.f, T);
}

// ============================================================
// Paint — orchestrator
// ============================================================

int32 SAudioFXMultibandCurve::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const FSubmixEffectMultibandCompressorSettings LocalSettings = Settings.Get();
	const TArray<FLinearColor> Colors = BandColors.Get(TArray<FLinearColor>());
	const int32 NumBands = LocalSettings.Bands.Num();
	// NOTE: Do NOT early-return on NumBands==0. We always draw the background + grid.
	// When no bands exist, show a "No bands" placeholder message instead.

	const FVector2f Size = UE::Slate::CastToVector2f(AllottedGeometry.GetLocalSize());
	const float Width  = Size.X;
	const float Height = Size.Y;

	// Padding: LeftPad/BottomPad leave room for axis labels
	static constexpr float LeftPad   = 30.f;
	static constexpr float RightPad  = 10.f;
	static constexpr float TopPad    = 8.f;
	static constexpr float BottomPad = 18.f;
	const float GraphW = FMath::Max(1.f, Width  - LeftPad - RightPad);
	const float GraphH = FMath::Max(1.f, Height - TopPad  - BottomPad);

	LayerId = PaintMBBackground(AllottedGeometry, OutDrawElements, LayerId, GraphW, GraphH, LeftPad, TopPad);
	LayerId = PaintMBGrid(AllottedGeometry, OutDrawElements, LayerId, Width, Height, GraphW, GraphH, LeftPad, TopPad);

	if (NumBands > 0)
	{
		TArray<float> Crossovers;
		Crossovers.Reserve(FMath::Max(0, NumBands - 1));
		for (int32 BandIndex = 0; BandIndex < NumBands - 1; ++BandIndex)
		{
			Crossovers.Add(FMath::Clamp(LocalSettings.Bands[BandIndex].CrossoverTopFrequency, MinFreqHz, MaxFreqHz));
		}
		const float HalfOctaves = EstimateOverlapHalfOctaves(LocalSettings.bFourPole);
		const float OctaveScale = FMath::Pow(2.f, HalfOctaves);

		LayerId = PaintMBOverlapZones(AllottedGeometry, OutDrawElements, LayerId, GraphH, LeftPad, TopPad, Crossovers, OctaveScale);
		LayerId = PaintMBBandAreas(AllottedGeometry, OutDrawElements, LayerId, GraphW, GraphH, LeftPad, TopPad, LocalSettings, Colors, Crossovers, OctaveScale, NumBands);
		LayerId = PaintMBCrossoverLines(AllottedGeometry, OutDrawElements, LayerId, GraphH, LeftPad, TopPad, Crossovers);
		LayerId = PaintMBEqualPowerCurves(AllottedGeometry, OutDrawElements, LayerId, GraphW, GraphH, LeftPad, TopPad, Crossovers, OctaveScale);
		return LayerId;
	}

	// Empty state — show a subtle centred hint
	const FSlateFontInfo HintFont = FCoreStyle::GetDefaultFontStyle("Italic", 9);
	const FText HintText = FText::FromString(TEXT("Add a band to begin"));
	const FPaintGeometry HintGeo = AllottedGeometry.ToPaintGeometry(
		FVector2f(GraphW, 20.f),
		FSlateLayoutTransform(FVector2f(LeftPad, TopPad + GraphH * 0.5f - 10.f)));
	FSlateDrawElement::MakeText(
		OutDrawElements, LayerId, HintGeo, HintText, HintFont,
		ESlateDrawEffect::None,
		FAudioFXTheme::GetTextDisabledColor().CopyWithNewOpacity(0.45f));

	return LayerId + 1;
}

// ============================================================
// Private paint helpers
// ============================================================

int32 SAudioFXMultibandCurve::PaintMBBackground(
	const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, float GraphW, float GraphH, float LeftPad, float TopPad) const
{
	TArray<FSlateGradientStop> GradientStops;
	GradientStops.Add(FSlateGradientStop(FVector2f(0.f, TopPad),          FAudioFXTheme::GetVisualizationBackground()));
	GradientStops.Add(FSlateGradientStop(FVector2f(0.f, TopPad + GraphH), FAudioFXTheme::GetWindowBackground()));
	FSlateDrawElement::MakeGradient(
		OutDrawElements, LayerId,
		Geometry.ToPaintGeometry(FVector2f(GraphW, GraphH), FSlateLayoutTransform(FVector2f(LeftPad, TopPad))),
		GradientStops, Orient_Vertical);
	return ++LayerId;
}

int32 SAudioFXMultibandCurve::PaintMBGrid(
	const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, float Width, float Height,
	float GraphW, float GraphH, float LeftPad, float TopPad) const
{
	// Coordinate helpers
	auto XFromFreq = [&](float Hz) -> float
	{
		return LeftPad + FreqToNorm(FMath::Clamp(Hz, MinFreqHz, MaxFreqHz)) * GraphW;
	};
	auto YFromDb = [&](float Db) -> float
	{
		// T=0 → bottom of graph (MinDb), T=1 → top (MaxDb); Slate Y is inverted
		const float T = FMath::Clamp((Db - MinDb) / (MaxDb - MinDb), 0.f, 1.f);
		return TopPad + (1.f - T) * GraphH;
	};

	// ── Frequency and dB grid lines with axis labels ────────────
	static const float GridFreqs[] = { 20.f, 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f, 20000.f };
	static const float GridDbs[]   = { 0.f, -6.f, -12.f, -18.f, -24.f };
	const FSlateFontInfo Font       = FCoreStyle::GetDefaultFontStyle("Regular", 7);
	const FLinearColor GridMinor    = FAudioFXTheme::GetGridMinorColor();
	const FLinearColor GridMajor    = FAudioFXTheme::GetGridMajorColor();
	const FLinearColor LabelColor   = FAudioFXTheme::GetTextDisabledColor();

	// Vertical frequency lines + labels
	for (float Freq : GridFreqs)
	{
		const float X = XFromFreq(Freq);
		TArray<FVector2f> Line = { FVector2f(X, TopPad), FVector2f(X, TopPad + GraphH) };
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(), Line, ESlateDrawEffect::None, GridMinor, false, 0.5f);

		// Frequency label below the graph area (kHz format for ≥1000 Hz)
		const FText Label = (Freq >= 1000.f)
			? FText::FromString(FString::Printf(TEXT("%.0fk"), Freq / 1000.f))
			: FText::AsNumber((int32)Freq);
		FSlateDrawElement::MakeText(
			OutDrawElements, LayerId + 1,
			Geometry.ToPaintGeometry(FVector2f(24.f, 10.f), FSlateLayoutTransform(FVector2f(X - 12.f, TopPad + GraphH + 2.f))),
			Label, Font, ESlateDrawEffect::None, LabelColor);
	}

	// Horizontal dB lines + labels
	for (float Db : GridDbs)
	{
		const float Y = YFromDb(Db);
		TArray<FVector2f> Line = { FVector2f(LeftPad, Y), FVector2f(LeftPad + GraphW, Y) };
		// 0 dB line is slightly brighter (major grid) to serve as a visual anchor
		FSlateDrawElement::MakeLines(
			OutDrawElements, LayerId, Geometry.ToPaintGeometry(), Line, ESlateDrawEffect::None,
			(FMath::IsNearlyZero(Db) ? GridMajor : GridMinor), false,
			FMath::IsNearlyZero(Db) ? 1.f : 0.5f);

		FSlateDrawElement::MakeText(
			OutDrawElements, LayerId + 1,
			Geometry.ToPaintGeometry(FVector2f(28.f, 10.f), FSlateLayoutTransform(FVector2f(0.f, Y - 5.f))),
			FText::FromString(FString::Printf(TEXT("%.0f"), Db)),
			Font, ESlateDrawEffect::None, LabelColor);
	}
	LayerId += 2;

	return LayerId;
}

int32 SAudioFXMultibandCurve::PaintMBOverlapZones(
	const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, float GraphH, float LeftPad, float TopPad,
	const TArray<float>& Crossovers, float OctaveScale) const
{
	// Recalculate GraphW from the geometry width since it's not passed here
	const float GraphW = FMath::Max(1.f, (float)Geometry.GetLocalSize().X - LeftPad - 10.f);
	auto XFromFreqFull = [&](float Hz) -> float
	{
		return LeftPad + FreqToNorm(FMath::Clamp(Hz, MinFreqHz, MaxFreqHz)) * GraphW;
	};

	// Overlap zone spans ±HalfOctaves around the crossover frequency
	for (float Crossover : Crossovers)
	{
		const float Low  = FMath::Clamp(Crossover / OctaveScale, MinFreqHz, MaxFreqHz);
		const float High = FMath::Clamp(Crossover * OctaveScale, MinFreqHz, MaxFreqHz);
		const float ZoneX = XFromFreqFull(Low);
		const float ZoneW = XFromFreqFull(High) - ZoneX;

		FSlateDrawElement::MakeBox(
			OutDrawElements, LayerId,
			Geometry.ToPaintGeometry(FVector2f(ZoneW, GraphH), FSlateLayoutTransform(FVector2f(ZoneX, TopPad))),
			FAppStyle::Get().GetBrush("WhiteBrush"),
			ESlateDrawEffect::None,
			FLinearColor(1.f, 1.f, 1.f, 0.12f));

		// Subtle border around the overlap zone for extra delineation
		TArray<FVector2f> Border = {
			FVector2f(ZoneX,         TopPad),
			FVector2f(ZoneX + ZoneW, TopPad),
			FVector2f(ZoneX + ZoneW, TopPad + GraphH),
			FVector2f(ZoneX,         TopPad + GraphH),
			FVector2f(ZoneX,         TopPad)
		};
		FSlateDrawElement::MakeLines(
			OutDrawElements, LayerId + 1, Geometry.ToPaintGeometry(), Border,
			ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, 0.08f), true, 1.f);
	}
	LayerId += 2;

	return LayerId;
}

int32 SAudioFXMultibandCurve::PaintMBBandAreas(
	const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, float GraphW, float GraphH, float LeftPad, float TopPad,
	const FSubmixEffectMultibandCompressorSettings& Cfg,
	const TArray<FLinearColor>& Colors,
	const TArray<float>& Crossovers, float OctaveScale, int32 NumBands) const
{
	auto YFromDb = [&](float Db) -> float
	{
		const float T = FMath::Clamp((Db - MinDb) / (MaxDb - MinDb), 0.f, 1.f);
		return TopPad + (1.f - T) * GraphH;
	};

	const float BottomY = YFromDb(MinDb);

	for (int32 BandIndex = 0; BandIndex < NumBands; ++BandIndex)
	{
		const FDynamicsBandSettings& Band = Cfg.Bands[BandIndex];

		// Determine this band's frequency extent from its neighbors' crossovers.
		// Extend into adjacent crossover regions so lines appear flat and unbroken
		// through the overlap zone instead of stopping abruptly at the crossover edge.
		const float RawStartHz = (BandIndex == 0)          ? MinFreqHz : FMath::Clamp(Cfg.Bands[BandIndex - 1].CrossoverTopFrequency, MinFreqHz, MaxFreqHz);
		const float RawEndHz   = (BandIndex == NumBands-1) ? MaxFreqHz : FMath::Clamp(Band.CrossoverTopFrequency, MinFreqHz, MaxFreqHz);
		const float StartHz    = (BandIndex == 0)          ? RawStartHz : FMath::Clamp(RawStartHz / OctaveScale, MinFreqHz, MaxFreqHz);
		const float EndHz      = (BandIndex == NumBands-1) ? RawEndHz   : FMath::Clamp(RawEndHz   * OctaveScale, MinFreqHz, MaxFreqHz);
		if (EndHz <= StartHz + SMALL_NUMBER) continue;

		const float StartNorm = FreqToNorm(StartHz);
		const float EndNorm   = FreqToNorm(EndHz);
		const FLinearColor BaseColor = Colors.IsValidIndex(BandIndex)
			? Colors[BandIndex]
			: FAudioFXTheme::GetBandPalette()[BandIndex % FAudioFXTheme::NumBandPaletteColors];

		// Build the top-edge polyline for this band (OutputGainDb is constant across its range)
		TArray<FVector2f> EnvelopePoints;
		EnvelopePoints.Reserve(NumEnvelopeSamples + 1);
		for (int32 Step = 0; Step <= NumEnvelopeSamples; ++Step)
		{
			const float T    = (float)Step / (float)NumEnvelopeSamples;
			const float Norm = FMath::Lerp(StartNorm, EndNorm, T);
			const float EnvelopeDb = FMath::Clamp(Band.OutputGainDb, MinDb, MaxDb);
			EnvelopePoints.Add(FVector2f(LeftPad + Norm * GraphW, YFromDb(EnvelopeDb)));
		}

		// Custom triangle-strip geometry to fill the area under the envelope
		TArray<FSlateVertex> Verts;
		TArray<SlateIndex>   Indices;
		Verts.Reserve(EnvelopePoints.Num() * 2);
		Indices.Reserve((EnvelopePoints.Num() - 1) * 6);
		const FColor FillColor = BaseColor.CopyWithNewOpacity(0.10f).ToFColor(true);

		for (int32 PointIndex = 0; PointIndex < EnvelopePoints.Num(); ++PointIndex)
		{
			const FVector2f& Pt = EnvelopePoints[PointIndex];
			// Top vertex at the envelope Y, bottom vertex at the MinDb baseline
			Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
				Geometry.ToPaintGeometry().GetAccumulatedRenderTransform(), Pt,               FVector2f::ZeroVector, FillColor));
			Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
				Geometry.ToPaintGeometry().GetAccumulatedRenderTransform(), FVector2f(Pt.X, BottomY), FVector2f::ZeroVector, FillColor));

			if (PointIndex > 0)
			{
				// Two triangles per quad (top-left, bottom-left, top-right, bottom-right)
				const int32 BaseIdx = (PointIndex - 1) * 2;
				Indices.Add(BaseIdx);     Indices.Add(BaseIdx + 1); Indices.Add(BaseIdx + 2);
				Indices.Add(BaseIdx + 1); Indices.Add(BaseIdx + 3); Indices.Add(BaseIdx + 2);
			}
		}
		FSlateDrawElement::MakeCustomVerts(OutDrawElements, LayerId, FSlateResourceHandle(), Verts, Indices, nullptr, 0, 0);

		// Glow pass — fat line at low opacity to give a soft bloom effect
		FSlateDrawElement::MakeLines(
			OutDrawElements, LayerId + 1, Geometry.ToPaintGeometry(), EnvelopePoints,
			ESlateDrawEffect::None, BaseColor.CopyWithNewOpacity(0.15f), true, 4.f);

		// Main envelope line — solid, 2px
		FSlateDrawElement::MakeLines(
			OutDrawElements, LayerId + 2, Geometry.ToPaintGeometry(), EnvelopePoints,
			ESlateDrawEffect::None, BaseColor, true, 2.f);
	}
	LayerId += 3;

	return LayerId;
}

int32 SAudioFXMultibandCurve::PaintMBCrossoverLines(
	const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, float GraphH, float LeftPad, float TopPad,
	const TArray<float>& Crossovers) const
{
	// Recalculate GraphW from geometry
	const float GraphW = FMath::Max(1.f, (float)Geometry.GetLocalSize().X - LeftPad - 10.f);
	auto XFromFreq = [&](float Hz) -> float
	{
		return LeftPad + FreqToNorm(FMath::Clamp(Hz, MinFreqHz, MaxFreqHz)) * GraphW;
	};

	// Thin crossover divider lines (drawn over overlap zones) — semi-transparent black
	// so they remain visible against any band color
	for (float Crossover : Crossovers)
	{
		const float X = XFromFreq(Crossover);
		// Start from Y=0 (absolute top of widget) so the line goes through the entire
		// white overlap region, not just from where the band color starts.
		TArray<FVector2f> Line = { FVector2f(X, 0.f), FVector2f(X, TopPad + GraphH) };
		FSlateDrawElement::MakeLines(
			OutDrawElements, LayerId, Geometry.ToPaintGeometry(), Line,
			ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, 0.60f), true, 1.f);
	}

	return ++LayerId;
}

int32 SAudioFXMultibandCurve::PaintMBEqualPowerCurves(
	const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, float GraphW, float GraphH, float LeftPad, float TopPad,
	const TArray<float>& Crossovers, float OctaveScale) const
{
	// Each crossover has two equal-power curves drawn across its overlap zone:
	//   Left band  (falling): gain = cos( T * π/2 )   T goes 0→1 across the zone
	//   Right band (rising):  gain = sin( T * π/2 )
	// gain 0..1 mapped to graph height via dB (gain=1 → 0 dB top, gain=0 → MinDb bottom).

	auto XFromFreq = [&](float Hz) -> float
	{
		return LeftPad + FreqToNorm(FMath::Clamp(Hz, MinFreqHz, MaxFreqHz)) * GraphW;
	};
	auto YFromGain = [&](float Gain) -> float
	{
		const float Db = (Gain > 0.f)
			? FMath::Clamp(20.f * FMath::LogX(10.f, Gain), MinDb, MaxDb)
			: MinDb;
		const float T = (Db - MinDb) / (MaxDb - MinDb);
		return TopPad + (1.f - T) * GraphH;
	};

	static constexpr int32 CurveSamples = 48;
	const FLinearColor CurveColor(0.f, 0.f, 0.f, 0.65f);

	for (float Crossover : Crossovers)
	{
		const float LowHz  = FMath::Clamp(Crossover / OctaveScale, MinFreqHz, MaxFreqHz);
		const float HighHz = FMath::Clamp(Crossover * OctaveScale, MinFreqHz, MaxFreqHz);
		const float X0 = XFromFreq(LowHz);
		const float X1 = XFromFreq(HighHz);

		TArray<FVector2f> FallCurve; FallCurve.Reserve(CurveSamples + 1);
		TArray<FVector2f> RiseCurve; RiseCurve.Reserve(CurveSamples + 1);

		for (int32 Step = 0; Step <= CurveSamples; ++Step)
		{
			const float T     = (float)Step / (float)CurveSamples;
			const float Angle = T * (PI / 2.f);
			const float X     = FMath::Lerp(X0, X1, T);

			FallCurve.Add(FVector2f(X, YFromGain(FMath::Cos(Angle))));  // 1 → 0
			RiseCurve.Add(FVector2f(X, YFromGain(FMath::Sin(Angle))));  // 0 → 1
		}

		FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
			Geometry.ToPaintGeometry(), FallCurve,
			ESlateDrawEffect::None, CurveColor, true, 1.5f);

		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1,
			Geometry.ToPaintGeometry(), RiseCurve,
			ESlateDrawEffect::None, CurveColor, true, 1.5f);
	}

	LayerId += 2;
	return LayerId;
}
