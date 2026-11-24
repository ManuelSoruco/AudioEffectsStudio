// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Visualizers/SAudioFXTimeDomainGraph.h"
#include "Shared/Core/AudioFXTheme.h"

#include "Rendering/DrawElementTypes.h"
#include "Styling/CoreStyle.h"

// ═══════════════════════════════════════════════════════════════════════════
//  Statics
// ═══════════════════════════════════════════════════════════════════════════

float SAudioFXTimeDomainGraph::ChooseTimeGridInterval(float MaxTimeSec)
{
	if (!FMath::IsFinite(MaxTimeSec) || MaxTimeSec <= 0.f)
	{
		return 0.25f;
	}

	// Pick the smallest interval that yields ≤ 12 grid lines (was 8 — denser horizontal grid).
	static constexpr float Candidates[] = {
		0.025f, 0.05f, 0.1f, 0.25f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 20.0f
	};
	for (float C : Candidates)
	{
		if ((MaxTimeSec / C) <= 12.f)
		{
			return C;
		}
	}
	return 30.0f;
}

float SAudioFXTimeDomainGraph::AmplitudeToDb(float LinearAmp)
{
	if (LinearAmp <= 0.0001f)
	{
		return MinDb;
	}
	return FMath::Clamp(20.f * FMath::LogX(10.f, LinearAmp), MinDb, MaxDb);
}

float SAudioFXTimeDomainGraph::DbToNormY(float Db)
{
	const float Clamped = FMath::Clamp(Db, MinDb, MaxDb);
	// MaxDb (0 dB) → 0.0 (top), MinDb (-40 dB) → 1.0 (bottom)
	return (MaxDb - Clamped) / (MaxDb - MinDb);
}

// ═══════════════════════════════════════════════════════════════════════════
//  Widget lifecycle
// ═══════════════════════════════════════════════════════════════════════════

void SAudioFXTimeDomainGraph::Construct(const FArguments& InArgs)
{
	MaxTimeSec = InArgs._MaxTimeSec;
	Bars       = InArgs._Bars;
	Curves     = InArgs._Curves;
	AssetColor = InArgs._AssetColor;
}

FVector2D SAudioFXTimeDomainGraph::ComputeDesiredSize(float) const
{
	// Enforce a minimum desired height of 200px to prevent graph collapse
	// when layout siblings (e.g. CollapsibleSection expanding) steal space.
	// The width is a hint; height is the critical minimum.
	return FVector2D(400.f, 200.f);
}

// ═══════════════════════════════════════════════════════════════════════════
//  OnPaint — orchestrator
// ═══════════════════════════════════════════════════════════════════════════

int32 SAudioFXTimeDomainGraph::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const FVector2f Size = UE::Slate::CastToVector2f(AllottedGeometry.GetLocalSize());
	const float Width = FMath::Max(Size.X - GraphInset * 2.f, 1.f);
	const float Height = FMath::Max(Size.Y - GraphInset * 2.f, 1.f);
	const float SafeMaxTime = FMath::Max(MaxTimeSec.Get(2.0f), 0.001f);

	// Background
	const FPaintGeometry PlotGeo = AllottedGeometry.ToPaintGeometry(
		FVector2f(Width, Height),
		FSlateLayoutTransform(FVector2f(GraphInset, GraphInset)));

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		PlotGeo,
		&FAudioFXTheme::GetInsetBrush(),
		ESlateDrawEffect::None,
		FAudioFXTheme::GetVisualizationBackground());
	++LayerId;

	// Grid
	LayerId = PaintGrid(AllottedGeometry, OutDrawElements, LayerId, Width, Height, SafeMaxTime);

	// Curves (behind bars)
	const TArray<FAudioFXGraphCurve> LocalCurves = Curves.Get(TArray<FAudioFXGraphCurve>());
	if (LocalCurves.Num() > 0)
	{
		LayerId = PaintCurves(AllottedGeometry, OutDrawElements, LayerId, Width, Height, SafeMaxTime, LocalCurves);
	}

	// Bars (on top)
	const TArray<FAudioFXGraphBar> LocalBars = Bars.Get(TArray<FAudioFXGraphBar>());
	if (LocalBars.Num() > 0)
	{
		LayerId = PaintBars(AllottedGeometry, OutDrawElements, LayerId, Width, Height, SafeMaxTime, LocalBars);
	}

	return LayerId;
}

// ═══════════════════════════════════════════════════════════════════════════
//  PaintGrid — standard dB horizontal + time vertical grid
// ═══════════════════════════════════════════════════════════════════════════

int32 SAudioFXTimeDomainGraph::PaintGrid(
	const FGeometry& Geometry,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	float Width, float Height, float MaxTime) const
{
	const FPaintGeometry PlotGeo = Geometry.ToPaintGeometry(
		FVector2f(Width, Height),
		FSlateLayoutTransform(FVector2f(GraphInset, GraphInset)));

	const FSlateFontInfo GridFont = FCoreStyle::GetDefaultFontStyle("Regular", 7);

	// ── Horizontal dB grid: every 6 dB from -6 down to MinDb (-30) ──────
	{
		constexpr float DbLevels[] = { -6.f, -12.f, -18.f, -24.f, -30.f };
		for (float Db : DbLevels)
		{
			const float Y = DbToNormY(Db) * Height;
			const TArray<FVector2f> HLine = { FVector2f(0.f, Y), FVector2f(Width, Y) };
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId,
				PlotGeo,
				HLine,
				ESlateDrawEffect::None,
				FAudioFXTheme::GetGridMinorColor(),
				false,
				0.5f);

			const FText Label = FText::FromString(FString::Printf(TEXT("%d"), FMath::RoundToInt(Db)));
			const FPaintGeometry LabelGeo = Geometry.ToPaintGeometry(
				FVector2f(24.f, 10.f),
				FSlateLayoutTransform(FVector2f(GraphInset + 2.f, GraphInset + Y - 9.f)));
			FSlateDrawElement::MakeText(
				OutDrawElements,
				LayerId + 1,
				LabelGeo,
				Label,
				GridFont,
				ESlateDrawEffect::None,
				FAudioFXTheme::GetTextDisabledColor());
		}

		// 0 dB reference line (top)
		const TArray<FVector2f> TopLine = { FVector2f(0.f, 0.f), FVector2f(Width, 0.f) };
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId + 1,
			PlotGeo,
			TopLine,
			ESlateDrawEffect::None,
			FMath::Lerp(FAudioFXTheme::GetGridMajorColor(), AssetColor.CopyWithNewOpacity(0.4f), 0.25f),
			false,
			1.0f);

		// Floor line at -30 dB (bottom)
		const TArray<FVector2f> FloorLine = { FVector2f(0.f, Height), FVector2f(Width, Height) };
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId + 1,
			PlotGeo,
			FloorLine,
			ESlateDrawEffect::None,
			FAudioFXTheme::GetGridMajorColor(),
			false,
			1.0f);
	}
	LayerId += 2;

	// ── Vertical time grid (auto-interval) ───────────────────────────────
	{
		const float GridStep = ChooseTimeGridInterval(MaxTime);
		const int32 NumSteps = FMath::CeilToInt(MaxTime / GridStep);
		for (int32 I = 0; I <= NumSteps; ++I)
		{
			const float TimeSec = FMath::Min(static_cast<float>(I) * GridStep, MaxTime);
			const float X = FMath::Clamp(TimeSec / MaxTime, 0.f, 1.f) * Width;

			const TArray<FVector2f> VLine = { FVector2f(X, 0.f), FVector2f(X, Height) };
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId,
				PlotGeo,
				VLine,
				ESlateDrawEffect::None,
				FAudioFXTheme::GetGridMinorColor(),
				false,
				0.5f);

			// Skip the label for the last step — show only first and intermediary labels.
			if (I == NumSteps) { continue; }

			// Time label at bottom
			FText Label;
			if (MaxTime <= 0.5f)
			{
				// Show ms for very short times
				Label = FText::FromString(FString::Printf(TEXT("%.0fms"), TimeSec * 1000.f));
			}
			else if (MaxTime <= 4.f)
			{
				Label = FText::FromString(FString::Printf(TEXT("%.2fs"), TimeSec));
			}
			else
			{
				Label = FText::FromString(FString::Printf(TEXT("%.1fs"), TimeSec));
			}

			const FPaintGeometry LabelGeo = Geometry.ToPaintGeometry(
				FVector2f(36.f, 10.f),
				FSlateLayoutTransform(FVector2f(GraphInset + X + 2.f, GraphInset + Height - 10.f)));
			FSlateDrawElement::MakeText(
				OutDrawElements,
				LayerId + 1,
				LabelGeo,
				Label,
				GridFont,
				ESlateDrawEffect::None,
				FAudioFXTheme::GetTextDisabledColor());
		}
	}
	LayerId += 2;

	return LayerId;
}

// ═══════════════════════════════════════════════════════════════════════════
//  PaintBars — vertical bar elements
// ═══════════════════════════════════════════════════════════════════════════

int32 SAudioFXTimeDomainGraph::PaintBars(
	const FGeometry& Geometry,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	float Width, float Height, float MaxTime,
	const TArray<FAudioFXGraphBar>& InBars) const
{
	const FPaintGeometry PlotGeo = Geometry.ToPaintGeometry(
		FVector2f(Width, Height),
		FSlateLayoutTransform(FVector2f(GraphInset, GraphInset)));

	constexpr float BarHalfWidth = 3.f;
	constexpr float DisabledAlpha = 0.3f;

	for (const FAudioFXGraphBar& Bar : InBars)
	{
		const float X = FMath::Clamp(Bar.TimeSec / FMath::Max(MaxTime, 0.001f), 0.f, 1.f) * Width;
		const float NormY = DbToNormY(FMath::Clamp(Bar.GainDb, MinDb, MaxDb));
		const float TopY = NormY * Height;
		const float Alpha = Bar.bDisabled ? DisabledAlpha : 1.0f;
		const FLinearColor BarColor = Bar.Color;

		// Glow (wider, translucent)
		const TArray<FVector2f> Line = { FVector2f(X, Height), FVector2f(X, TopY) };
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			PlotGeo,
			Line,
			ESlateDrawEffect::None,
			BarColor.CopyWithNewOpacity(0.15f * Alpha),
			true,
			BarHalfWidth * 3.f);

		// Main bar
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId + 1,
			PlotGeo,
			Line,
			ESlateDrawEffect::None,
			BarColor.CopyWithNewOpacity(0.95f * Alpha),
			false,
			BarHalfWidth * 2.f);

		// Cap dot
		const TArray<FVector2f> Cap = {
			FVector2f(X - BarHalfWidth, TopY),
			FVector2f(X + BarHalfWidth, TopY)
		};
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId + 2,
			PlotGeo,
			Cap,
			ESlateDrawEffect::None,
			BarColor.CopyWithNewOpacity(Alpha),
			false,
			2.f);
	}
	LayerId += 3;

	return LayerId;
}

// ═══════════════════════════════════════════════════════════════════════════
//  PaintCurves — continuous polyline overlays
// ═══════════════════════════════════════════════════════════════════════════

int32 SAudioFXTimeDomainGraph::PaintCurves(
	const FGeometry& Geometry,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	float Width, float Height, float MaxTime,
	const TArray<FAudioFXGraphCurve>& InCurves) const
{
	const FPaintGeometry PlotGeo = Geometry.ToPaintGeometry(
		FVector2f(Width, Height),
		FSlateLayoutTransform(FVector2f(GraphInset, GraphInset)));

	for (const FAudioFXGraphCurve& Curve : InCurves)
	{
		if (Curve.Points.Num() < 2)
		{
			continue;
		}

		// Convert (TimeSec, LinearAmplitude) → (pixel X, pixel Y)
		TArray<FVector2f> PixelPoints;
		PixelPoints.Reserve(Curve.Points.Num());
		for (const FVector2f& Pt : Curve.Points)
		{
			const float X = FMath::Clamp(Pt.X / FMath::Max(MaxTime, 0.001f), 0.f, 1.f) * Width;
			const float Y = DbToNormY(AmplitudeToDb(Pt.Y)) * Height;
			PixelPoints.Add(FVector2f(X, Y));
		}

		// Glow pass (optional)
		if (Curve.bGlow)
		{
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId,
				PlotGeo,
				PixelPoints,
				ESlateDrawEffect::None,
				Curve.Color.CopyWithNewOpacity(0.15f),
				true,
				Curve.Thickness * 3.f);
		}

		// Main curve
		if (Curve.bDashed)
		{
			FSlateDrawElement::MakeDashedLines(
				OutDrawElements,
				LayerId + 1,
				PlotGeo,
				CopyTemp(PixelPoints),
				ESlateDrawEffect::None,
				Curve.Color,
				Curve.Thickness,
				6.f);
		}
		else
		{
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId + 1,
				PlotGeo,
				PixelPoints,
				ESlateDrawEffect::None,
				Curve.Color,
				true,
				Curve.Thickness);
		}
	}
	LayerId += 2;

	return LayerId;
}
