// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Visualizers/SAudioFXPanDisplay.h"
#include "Shared/Core/AudioFXTheme.h"
#include "Rendering/DrawElementTypes.h"
#include "Brushes/SlateRoundedBoxBrush.h"

void SAudioFXPanDisplay::Construct(const FArguments& InArgs)
{
	Pan        = InArgs._Pan;
	Spread     = InArgs._Spread;
	AssetColor = InArgs._AssetColor;

	// Force repaint every frame so the display tracks live knob changes.
	RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateLambda(
		[](double, float) -> EActiveTimerReturnType
		{
			return EActiveTimerReturnType::Continue;
		}));
}

FVector2D SAudioFXPanDisplay::ComputeDesiredSize(float) const
{
	return FVector2D(200.f, 60.f);
}

float SAudioFXPanDisplay::ClampPanValue(float InPan)
{
	const float PanValue = FMath::IsFinite(InPan) ? InPan : 0.f;
	return FMath::Clamp(PanValue, -1.f, 1.f);
}

float SAudioFXPanDisplay::ClampSpreadValue(float InSpread)
{
	const float SpreadValue = FMath::IsFinite(InSpread) ? InSpread : 0.f;
	return FMath::Clamp(SpreadValue, -1.f, 1.f);
}

float SAudioFXPanDisplay::PanToNormalizedX(float InPan)
{
	return (ClampPanValue(InPan) + 1.f) * 0.5f;
}

void SAudioFXPanDisplay::ComputeSpreadBoundsNormalized(float InPan, float InSpread, float& OutLeft, float& OutRight)
{
	const float Center = PanToNormalizedX(InPan);
	const float HalfSpread = FMath::Abs(ClampSpreadValue(InSpread)) * 0.5f;
	OutLeft = FMath::Clamp(Center - HalfSpread, 0.f, 1.f);
	OutRight = FMath::Clamp(Center + HalfSpread, 0.f, 1.f);
}

int32 SAudioFXPanDisplay::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const float PanVal = ClampPanValue(Pan.Get(0.f));
	const float SpreadVal = ClampSpreadValue(Spread.Get(0.f));

	const FVector2f Size = UE::Slate::CastToVector2f(AllottedGeometry.GetLocalSize());
	const float Width = Size.X;
	const float Height = Size.Y;

	const float TrackMarginX = 20.f;
	const float TrackY       = Height * 0.5f;
	const float TrackLeft    = TrackMarginX;
	const float TrackRight   = Width - TrackMarginX;
	const float TrackLen     = TrackRight - TrackLeft;

	auto PanToX = [&](float PanValue) -> float {
		return TrackLeft + PanToNormalizedX(PanValue) * TrackLen;
	};

	const float PuckX = PanToX(PanVal);

	// ── Layer 0: inset background ────────────────────────────────
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(),
		&FAudioFXTheme::GetInsetBrush(),
		ESlateDrawEffect::None, FAudioFXTheme::GetWindowBackground());  // #0A0A0A — was White = bright white
	++LayerId;

	// ── Layer 1: track bar ───────────────────────────────────────
	{
		const FLinearColor TrackColor(0.22f, 0.22f, 0.30f, 1.f);
		TArray<FVector2f> Track = {
			FVector2f(TrackLeft,  TrackY),
			FVector2f(TrackRight, TrackY)
		};
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(),
			Track, ESlateDrawEffect::None, TrackColor, false, 3.f);
	}
	++LayerId;

	// ── Layer 2: spread region ───────────────────────────────────
	{
		float SpreadLeftNorm = 0.f;
		float SpreadRightNorm = 0.f;
		ComputeSpreadBoundsNormalized(PanVal, SpreadVal, SpreadLeftNorm, SpreadRightNorm);
		const float SpreadLeft = TrackLeft + SpreadLeftNorm * TrackLen;
		const float SpreadRight = TrackLeft + SpreadRightNorm * TrackLen;

		if (SpreadRight > SpreadLeft + 2.f)
		{
			// Use the same tone as the knob arcs — Frequency blue for positive spread,
			// AccentWarn red for negative spread. Full opacity to match knob arc intensity.
			const FLinearColor SpreadColor = (SpreadVal >= 0.f)
				? FAudioFXTheme::GetParamColor(EAudioFXParamColor::Frequency).CopyWithNewOpacity(0.95f)
				: FAudioFXTheme::GetAccentWarn().CopyWithNewOpacity(0.95f);
			const float BarH = 3.5f;  // half the track's visual weight
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
				AllottedGeometry.ToPaintGeometry(
					FVector2f(SpreadRight - SpreadLeft, BarH),
					FSlateLayoutTransform(FVector2f(SpreadLeft, TrackY - BarH * 0.5f))),
				&SpreadBrush, ESlateDrawEffect::None, SpreadColor);
		}
	}
	++LayerId;

	// ── Layer 3: center tick ─────────────────────────────────────
	{
		const float CenterX = TrackLeft + TrackLen * 0.5f;
		TArray<FVector2f> CenterTick = {
			FVector2f(CenterX, TrackY - 7.f),
			FVector2f(CenterX, TrackY + 7.f)
		};
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(),
			CenterTick, ESlateDrawEffect::None,
			FLinearColor(0.30f, 0.30f, 0.42f, 1.f), false, 1.f);
	}
	++LayerId;

	// ── Layer 4: L/R endpoint ticks ──────────────────────────────
	{
		const FLinearColor EndColor(0.25f, 0.25f, 0.35f, 0.8f);
		float Ends[] = { TrackLeft, TrackRight };
		for (float X : Ends)
		{
			TArray<FVector2f> Tick = {
				FVector2f(X, TrackY - 5.f),
				FVector2f(X, TrackY + 5.f)
			};
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
				AllottedGeometry.ToPaintGeometry(),
				Tick, ESlateDrawEffect::None, EndColor, false, 1.f);
		}
	}
	++LayerId;

	// ── Layer 5: puck glow ────────────────────────────────────────
	{
		TArray<FVector2f> GlowPts = {
			FVector2f(PuckX - 0.5f, TrackY),
			FVector2f(PuckX + 0.5f, TrackY)
		};
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(),
			GlowPts, ESlateDrawEffect::None,
			FLinearColor(AssetColor.R, AssetColor.G, AssetColor.B, 0.20f),
			true, 18.f);
	}
	++LayerId;

	// ── Layer 6: puck ─────────────────────────────────────────────
	{
		const FLinearColor PuckColor(
			FMath::Min(AssetColor.R * 1.3f + 0.1f, 1.f),
			FMath::Min(AssetColor.G * 1.3f + 0.1f, 1.f),
			FMath::Min(AssetColor.B * 1.3f + 0.1f, 1.f),
			1.f);
		const float PuckR = 5.f;
		// PuckBrush is now a member — no static local
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(
				FVector2f(PuckR * 2.f, PuckR * 2.f),
				FSlateLayoutTransform(FVector2f(PuckX - PuckR, TrackY - PuckR))),
			&PuckBrush, ESlateDrawEffect::None, PuckColor);
	}
	++LayerId;

	return LayerId;
}
