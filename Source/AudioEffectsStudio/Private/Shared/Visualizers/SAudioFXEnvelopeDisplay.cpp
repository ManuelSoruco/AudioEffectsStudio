// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Visualizers/SAudioFXEnvelopeDisplay.h"
#include "Shared/Core/AudioFXTheme.h"
#include "Rendering/DrawElementTypes.h"
#include "Brushes/SlateRoundedBoxBrush.h"

void SAudioFXEnvelopeDisplay::Construct(const FArguments& InArgs)
{
	AttackTime  = InArgs._AttackTime;
	ReleaseTime = InArgs._ReleaseTime;
	AssetColor  = InArgs._AssetColor;
}

FVector2D SAudioFXEnvelopeDisplay::ComputeDesiredSize(float) const
{
	return FVector2D(260.f, 100.f);
}

float SAudioFXEnvelopeDisplay::ClampAttackMs(float InAttackMs)
{
	if (!FMath::IsFinite(InAttackMs))
	{
		return 10.f;
	}

	return FMath::Max(InAttackMs, 0.1f);
}

float SAudioFXEnvelopeDisplay::ClampReleaseMs(float InReleaseMs)
{
	if (!FMath::IsFinite(InReleaseMs))
	{
		return 200.f;
	}

	return FMath::Max(InReleaseMs, 0.1f);
}

float SAudioFXEnvelopeDisplay::ComputeAttackFraction(float InAttackMs, float InReleaseMs)
{
	const float AttMs = ClampAttackMs(InAttackMs);
	const float RelMs = ClampReleaseMs(InReleaseMs);

	// Use log-scale proportioning so both attack and release are always
	// visually represented even when their absolute times differ greatly.
	// log(t+1) compresses large values and expands small ones.
	const float LogAtt = FMath::Loge(AttMs + 1.f);
	const float LogRel = FMath::Loge(RelMs + 1.f);
	const float LogTotal = FMath::Max(LogAtt + LogRel, SMALL_NUMBER);

	const float ActiveFraction = 1.f - PreFrac - PeakFrac - TailFrac;
	return FMath::Clamp(ActiveFraction * (LogAtt / LogTotal), 0.f, ActiveFraction);
}

float SAudioFXEnvelopeDisplay::EvaluateEnvelopeNormalized(float NormalizedX, float InAttackMs, float InReleaseMs)
{
	const float X = FMath::Clamp(FMath::IsFinite(NormalizedX) ? NormalizedX : 0.f, 0.f, 1.f);
	const float AttackFraction = ComputeAttackFraction(InAttackMs, InReleaseMs);
	const float AttackEnd = PreFrac + AttackFraction;
	const float HoldEnd = AttackEnd + PeakFrac;
	const float TailStart = 1.f - TailFrac;

	if (X <= PreFrac)
	{
		return 0.f;
	}

	if (X <= AttackEnd)
	{
		const float Denominator = AttackEnd - PreFrac;
		const float AttackT = FMath::IsNearlyZero(Denominator) ? 0.f : (X - PreFrac) / Denominator;
		return 1.f - FMath::Pow(1.f - AttackT, 2.2f);
	}

	if (X <= HoldEnd)
	{
		return 1.f;
	}

	if (X <= TailStart)
	{
		const float Denominator = TailStart - HoldEnd;
		const float ReleaseT = FMath::IsNearlyZero(Denominator) ? 0.f : (X - HoldEnd) / Denominator;
		return FMath::Pow(1.f - ReleaseT, 1.8f);
	}

	return 0.f;
}

int32 SAudioFXEnvelopeDisplay::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const float AttMs  = ClampAttackMs(AttackTime.Get(10.f));
	const float RelMs  = ClampReleaseMs(ReleaseTime.Get(200.f));
	const FVector2f Size = UE::Slate::CastToVector2f(AllottedGeometry.GetLocalSize());
	const float Width = Size.X;
	const float Height = Size.Y;

	// Total timeline span (ms) — always show at least a bit of "pre" and "post" space
	// Normalized proportions: 8% pre-silence | attack portion | peak hold 12% | release portion | 8% tail
	const float AttFrac = ComputeAttackFraction(AttMs, RelMs);
	// Release fraction is implied by HoldEnd/TailStart in EvaluateEnvelopeNormalized.

	// X positions of key timeline events
	const float XPeak    = (PreFrac + AttFrac) * Width;
	const float XHoldEnd = (PreFrac + AttFrac + PeakFrac) * Width;

	// Y positions — bottom = silence, top = peak
	const float YSilence = Height - 4.f;
	const float YPeak    = 4.f;

	// ── Layer 0: inset background ────────────────────────────────
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(),
		&FAudioFXTheme::GetInsetBrush(),
		ESlateDrawEffect::None, FAudioFXTheme::GetWindowBackground());  // #0A0A0A — was White = bright white
	++LayerId;

	// ── Layer 1: vertical time grid (4 divisions) ────────────────
	{
		const FLinearColor GridColor = FAudioFXTheme::GetGridMinorColor();
		for (int32 GridIndex = 1; GridIndex <= 3; ++GridIndex)
		{
			const float X = (float)GridIndex / 4.f * Width;
			TArray<FVector2f> VLine = { FVector2f(X, 0.f), FVector2f(X, Height) };
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
				AllottedGeometry.ToPaintGeometry(),
				VLine, ESlateDrawEffect::None, GridColor, false, 0.5f);
		}
		// 0-baseline
		TArray<FVector2f> Base = { FVector2f(0.f, YSilence), FVector2f(Width, YSilence) };
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(),
			Base, ESlateDrawEffect::None, FAudioFXTheme::GetGridMajorColor(), false, 1.f);
	}
	++LayerId;

	// ── Build envelope curve points ──────────────────────────────
	// Phase 0: pre-silence (flat at bottom)
	// Phase 1: attack (exponential rise — concave: y = 1-(1-t)^2 )
	// Phase 2: peak hold (flat at top)
	// Phase 3: release (exponential decay — y = (1-t)^2 )
	// Phase 4: tail-silence (flat at bottom)

	TArray<FVector2f> EnvPoints;
	EnvPoints.Reserve(NumCurvePoints + 1);

	const int32 Steps = NumCurvePoints;
	for (int32 PointIndex = 0; PointIndex <= Steps; ++PointIndex)
	{
		const float TimeNorm = (float)PointIndex / Steps;
		const float X = TimeNorm * Width;
		const float Envelope = EvaluateEnvelopeNormalized(TimeNorm, AttMs, RelMs);
		const float Y = YSilence + (YPeak - YSilence) * Envelope;

		EnvPoints.Add(FVector2f(X, Y));
	}

	// ── Layer 2: fill — filled area under the curve (dark tinted) ─
	{
		// Draw a thin filled region by rendering the curve as a wide line at 15% opacity
		const FLinearColor FillColor(AssetColor.R, AssetColor.G, AssetColor.B, 0.10f);
		// Draw from curve down to baseline by duplicating points mirrored
		TArray<FVector2f> FillPts = EnvPoints;
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(),
			FillPts, ESlateDrawEffect::None, FillColor, true, 12.f);
	}
	++LayerId;

	// ── Layer 3: glow pass ────────────────────────────────────────
	{
		const FLinearColor GlowColor(AssetColor.R, AssetColor.G, AssetColor.B, 0.14f);
		TArray<FVector2f> GlowPts = EnvPoints;
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(),
			GlowPts, ESlateDrawEffect::None, GlowColor, true, 5.f);
	}
	++LayerId;

	// ── Layer 4: main curve (2px, asset color) ────────────────────
	{
		const FLinearColor CurveColor(
			FMath::Min(AssetColor.R * 1.15f + 0.05f, 1.f),
			FMath::Min(AssetColor.G * 1.15f + 0.05f, 1.f),
			FMath::Min(AssetColor.B * 1.15f + 0.05f, 1.f),
			1.f);
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(),
			EnvPoints, ESlateDrawEffect::None, CurveColor, true, 2.5f);
	}
	++LayerId;

	// ── Layer 5: attack/release marker ticks ─────────────────────
	{
		const FLinearColor TickColor(AssetColor.R, AssetColor.G, AssetColor.B, 0.5f);
		// Attack tick at XPeak — marks where attack phase ends
		TArray<FVector2f> ATick = { FVector2f(XPeak, YPeak - 4.f), FVector2f(XPeak, YSilence) };
		FSlateDrawElement::MakeDashedLines(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(),
			MoveTemp(ATick), ESlateDrawEffect::None, TickColor, 0.7f, 4.f);
		// Release-start tick at XHoldEnd — marks where peak hold ends and release begins
		// (previously was at XTail which is the release END, not the start — that was wrong)
		TArray<FVector2f> RTick = { FVector2f(XHoldEnd, YPeak - 4.f), FVector2f(XHoldEnd, YSilence) };
		FSlateDrawElement::MakeDashedLines(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(),
			MoveTemp(RTick), ESlateDrawEffect::None, TickColor, 0.7f, 4.f);
	}
	++LayerId;

	return LayerId;
}
