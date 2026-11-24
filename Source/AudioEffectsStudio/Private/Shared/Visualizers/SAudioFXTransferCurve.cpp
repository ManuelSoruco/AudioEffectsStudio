// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Visualizers/SAudioFXTransferCurve.h"
#include "Shared/Core/AudioFXTheme.h"

#include "Rendering/DrawElementTypes.h"
#include "Brushes/SlateRoundedBoxBrush.h"

// ═══════════════════════════════════════════════════════════════════════════
//  Widget lifecycle
// ═══════════════════════════════════════════════════════════════════════════

void SAudioFXTransferCurve::Construct(const FArguments& InArgs)
{
	Settings   = InArgs._Settings;
	AssetColor = InArgs._AssetColor;
}

FVector2D SAudioFXTransferCurve::ComputeDesiredSize(float) const
{
	return FVector2D(300.f, 300.f);
}

// ═══════════════════════════════════════════════════════════════════════════
//  ComputeOutputDb — dynamics transfer function math
//  Converts InputDb → OutputDb using the current DynSettings.
//  Applies input gain, threshold, ratio, knee and output gain.
// ═══════════════════════════════════════════════════════════════════════════
float SAudioFXTransferCurve::ComputeOutputDb(
	float InputDb,
	const FSourceEffectDynamicsProcessorSettings& DynSettings)
{
	// Apply input gain first
	const float GainedDb = InputDb + DynSettings.InputGainDb;

	const float ThreshDb   = DynSettings.ThresholdDb;
	const float Ratio      = FMath::Max(DynSettings.Ratio, 1.0f);
	const float HalfKnee   = DynSettings.KneeBandwidthDb * 0.5f;

	float OutputDb = GainedDb;  // default: 1:1

	switch (DynSettings.DynamicsProcessorType)
	{
	case ESourceEffectDynamicsProcessorType::Compressor:
	case ESourceEffectDynamicsProcessorType::Limiter:
	{
		// Limiter uses an extreme ratio (100:1) to approximate a hard ceiling
		const float LimRatio = (DynSettings.DynamicsProcessorType ==
			ESourceEffectDynamicsProcessorType::Limiter) ? 100.f : Ratio;

		if (HalfKnee > 0.01f && GainedDb > ThreshDb - HalfKnee && GainedDb < ThreshDb + HalfKnee)
		{
			// Soft knee region — quadratic blend from 1:1 to the full ratio
			const float t = (GainedDb - (ThreshDb - HalfKnee)) / (2.f * HalfKnee);
			const float KneeRatio = FMath::Lerp(1.f, LimRatio, t * t);
			const float Over = GainedDb - ThreshDb;
			OutputDb = GainedDb + (Over / KneeRatio - Over);
		}
		else if (GainedDb >= ThreshDb + HalfKnee)
		{
			// Above threshold — apply ratio normally
			const float Over = GainedDb - ThreshDb;
			OutputDb = ThreshDb + Over / LimRatio;
		}
		// Below threshold — 1:1 (OutputDb already initialized to GainedDb)
		break;
	}

	case ESourceEffectDynamicsProcessorType::Expander:
	case ESourceEffectDynamicsProcessorType::Gate:
	{
		// Gate uses an extreme ratio (100:1) to approximate a hard noise gate
		const float ExpRatio = (DynSettings.DynamicsProcessorType ==
			ESourceEffectDynamicsProcessorType::Gate) ? 100.f : Ratio;

		if (GainedDb < ThreshDb - HalfKnee)
		{
			// Below threshold — expand (attenuate) the signal
			const float Under = GainedDb - ThreshDb;
			OutputDb = ThreshDb + Under * ExpRatio;
		}
		else if (HalfKnee > 0.01f && GainedDb >= ThreshDb - HalfKnee && GainedDb < ThreshDb + HalfKnee)
		{
			// Soft knee — quadratic blend from full expansion back to 1:1
			const float t = 1.f - (GainedDb - (ThreshDb - HalfKnee)) / (2.f * HalfKnee);
			const float KneeRatio = FMath::Lerp(1.f, ExpRatio, t * t);
			const float Under = GainedDb - ThreshDb;
			OutputDb = ThreshDb + Under * KneeRatio;
		}
		// Above threshold — 1:1 (OutputDb already initialized to GainedDb)
		break;
	}

	case ESourceEffectDynamicsProcessorType::UpwardsCompressor:
	{
		// Upward compression — boosts signals below the threshold toward it
		if (GainedDb < ThreshDb)
		{
			const float Under = ThreshDb - GainedDb;
			OutputDb = GainedDb + Under * (1.f - 1.f / Ratio);
		}
		break;
	}

	default:
		break;
	}

	// Apply output gain
	return OutputDb + DynSettings.OutputGainDb;
}

// ═══════════════════════════════════════════════════════════════════════════
//  OnPaint
// ═══════════════════════════════════════════════════════════════════════════

int32 SAudioFXTransferCurve::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const FSourceEffectDynamicsProcessorSettings DynamicsSettings = Settings.Get();
	const FVector2f Size = UE::Slate::CastToVector2f(AllottedGeometry.GetLocalSize());
	const float Width  = Size.X;
	const float Height = Size.Y;

	// Coordinate helpers — both axes span [DisplayDbMin, DisplayDbMax]
	// DbToFrac: maps a dB value to a [0,1] normalized fraction along an axis
	auto DbToFrac = [](float Db) -> float
	{
		return (Db - DisplayDbMin) / DisplayDbRange;
	};

	// DbToPixel: converts (InputDb, OutputDb) to a Slate pixel coordinate.
	// X axis = input dB (left=min, right=max); Y axis = output dB (bottom=min, top=max, Slate-inverted).
	auto DbToPixel = [&](float InDb, float OutDb) -> FVector2f
	{
		const float NormX = DbToFrac(InDb);
		const float NormY = 1.f - DbToFrac(OutDb);  // invert: higher dB → smaller pixel Y
		return FVector2f(NormX * Width, NormY * Height);
	};

	// ── Layer 0: inset background ────────────────────────────────
	// Use the surface-deep color as tint — FLinearColor::White would render
	// the box white because FSlateRoundedBoxBrush uses a white texture modulated by tint.
	FSlateDrawElement::MakeBox(
		OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(),
		&FAudioFXTheme::GetInsetBrush(),
		ESlateDrawEffect::None,
		FAudioFXTheme::GetWindowBackground());  // #0A0A0A — matches inset dark look
	++LayerId;

	// ── Layer 1: dB grid lines (every 12 dB, subtle) ───────────────
	{
		const FLinearColor GridColor = FAudioFXTheme::GetGridMinorColor();
		static const float GridSteps[] = { -60.f, -48.f, -36.f, -24.f, -12.f, 0.f, 12.f };

		for (float Db : GridSteps)
		{
			if (Db < DisplayDbMin || Db > DisplayDbMax) continue;
			const float Frac = DbToFrac(Db);
			const float PixelX = Frac * Width;
			const float PixelY = (1.f - Frac) * Height;

			// Vertical line (input axis)
			TArray<FVector2f> VLine = {
				FVector2f(PixelX, 0.f),
				FVector2f(PixelX, Height)
			};
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
				AllottedGeometry.ToPaintGeometry(),
				VLine, ESlateDrawEffect::None, GridColor, false, 0.5f);

			// Horizontal line (output axis)
			TArray<FVector2f> HLine = {
				FVector2f(0.f, PixelY),
				FVector2f(Width,   PixelY)
			};
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
				AllottedGeometry.ToPaintGeometry(),
				HLine, ESlateDrawEffect::None, GridColor, false, 0.5f);
		}
	}
	++LayerId;

	// ── Layer 2: 1:1 diagonal reference (dashed, dim) ──────────────
	{
		const FLinearColor RefColor = FAudioFXTheme::GetGridMajorColor();
		TArray<FVector2f> DiagPoints;
		const int32 DiagSteps = 64;
		for (int32 PointIndex = 0; PointIndex <= DiagSteps; ++PointIndex)
		{
			const float Db = FMath::Lerp(DisplayDbMin, DisplayDbMax, (float)PointIndex / DiagSteps);
			DiagPoints.Add(DbToPixel(Db, Db));
		}
		FSlateDrawElement::MakeDashedLines(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(),
			MoveTemp(DiagPoints),
			ESlateDrawEffect::None, RefColor, 1.2f, 8.f);
	}
	++LayerId;

	// ── Layer 3: knee region highlight (subtle fill above/around threshold) ──
	{
		const float ThreshDb   = DynamicsSettings.ThresholdDb;
		const float HalfKnee   = DynamicsSettings.KneeBandwidthDb * 0.5f;
		const float KneeStartDb = ThreshDb - FMath::Max(HalfKnee, 1.5f);
		const float KneeEndDb   = ThreshDb + FMath::Max(HalfKnee, 1.5f);

		if (KneeStartDb >= DisplayDbMin && KneeEndDb <= DisplayDbMax)
		{
			const float X0 = DbToFrac(KneeStartDb) * Width;
			const float X1 = DbToFrac(KneeEndDb) * Width;
			const FLinearColor KneeColor(AssetColor.R, AssetColor.G, AssetColor.B, 0.07f);

			static FSlateRoundedBoxBrush KneeBrush(FLinearColor::White, 0.0f);
			FSlateDrawElement::MakeBox(
				OutDrawElements, LayerId,
				AllottedGeometry.ToPaintGeometry(
					FVector2f(X1 - X0, Height),
					FSlateLayoutTransform(FVector2f(X0, 0.f))),
				&KneeBrush,
				ESlateDrawEffect::None,
				KneeColor);
		}
	}
	++LayerId;

	// ── Layer 4: threshold crossover marker (vertical dashed, asset color) ──
	{
		const float ThreshFrac = DbToFrac(DynamicsSettings.ThresholdDb);
		if (ThreshFrac >= 0.f && ThreshFrac <= 1.f)
		{
			const float ThreshX = ThreshFrac * Width;
			const FLinearColor ThreshColor(AssetColor.R, AssetColor.G, AssetColor.B, 0.45f);

			TArray<FVector2f> ThreshLine = {
				FVector2f(ThreshX, 0.f),
				FVector2f(ThreshX, Height)
			};
			FSlateDrawElement::MakeDashedLines(OutDrawElements, LayerId,
				AllottedGeometry.ToPaintGeometry(),
				MoveTemp(ThreshLine),
				ESlateDrawEffect::None, ThreshColor, 1.0f, 5.f);
		}
	}
	++LayerId;

	// ── Layer 5: glow pass — wide soft copy of the curve at low opacity ──
	// Drawn before the main curve so the main sits cleanly on top.
	{
		const FLinearColor GlowColor(AssetColor.R, AssetColor.G, AssetColor.B, 0.15f);
		const int32 NumPoints = 256;
		TArray<FVector2f> GlowPoints;
		GlowPoints.Reserve(NumPoints + 1);
		for (int32 PointIndex = 0; PointIndex <= NumPoints; ++PointIndex)
		{
			const float InDb = FMath::Lerp(DisplayDbMin, DisplayDbMax, (float)PointIndex / NumPoints);
			const float OutDb = ComputeOutputDb(InDb, DynamicsSettings);
			const float ClampedOut = FMath::Clamp(OutDb, DisplayDbMin, DisplayDbMax);
			GlowPoints.Add(DbToPixel(InDb, ClampedOut));
		}
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(),
			GlowPoints, ESlateDrawEffect::None, GlowColor, true, 8.f);
	}
	++LayerId;

	// ── Layer 6: main transfer curve (solid, 2.5px, asset color brightened) ──
	// Slightly boost brightness to ensure the curve reads over the dark background.
	{
		const FLinearColor CurveColor(
			FMath::Min(AssetColor.R * 1.2f + 0.05f, 1.f),
			FMath::Min(AssetColor.G * 1.2f + 0.05f, 1.f),
			FMath::Min(AssetColor.B * 1.2f + 0.05f, 1.f),
			1.f);
		const int32 NumPoints = 256;
		TArray<FVector2f> CurvePoints;
		CurvePoints.Reserve(NumPoints + 1);
		for (int32 PointIndex = 0; PointIndex <= NumPoints; ++PointIndex)
		{
			const float InDb = FMath::Lerp(DisplayDbMin, DisplayDbMax, (float)PointIndex / NumPoints);
			const float OutDb = ComputeOutputDb(InDb, DynamicsSettings);
			const float ClampedOut = FMath::Clamp(OutDb, DisplayDbMin, DisplayDbMax);
			CurvePoints.Add(DbToPixel(InDb, ClampedOut));
		}
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(),
			CurvePoints, ESlateDrawEffect::None, CurveColor, true, 2.5f);
	}
	++LayerId;

	return LayerId;
}
