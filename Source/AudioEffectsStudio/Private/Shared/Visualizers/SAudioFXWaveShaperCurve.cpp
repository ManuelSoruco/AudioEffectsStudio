// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Visualizers/SAudioFXWaveShaperCurve.h"
#include "Shared/Core/AudioFXTheme.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SOverlay.h"

void SAudioFXWaveShaperCurve::Construct(const FArguments& InArgs)
{
	Amount    = InArgs._Amount;
	AssetColor = InArgs._AssetColor;
}

FVector2D SAudioFXWaveShaperCurve::ComputeDesiredSize(float) const
{
	return FVector2D(160.f, 160.f);
}

float SAudioFXWaveShaperCurve::ClampAmount(float InAmount)
{
	if (!FMath::IsFinite(InAmount))
	{
		return 0.f;
	}

	return FMath::Max(InAmount, 0.f);
}

float SAudioFXWaveShaperCurve::AmountToDrive(float InAmount)
{
	const float ClampedAmount = ClampAmount(InAmount);
	return FMath::GetMappedRangeValueClamped(FVector2D(0.f, 500.f), FVector2D(0.01f, 20.f), ClampedAmount);
}

float SAudioFXWaveShaperCurve::EvaluateTransfer(float NormalizedInput, float DriveK)
{
	const float X = FMath::Clamp(FMath::IsFinite(NormalizedInput) ? NormalizedInput : 0.f, -1.f, 1.f);
	const float K = FMath::Clamp(FMath::IsFinite(DriveK) ? DriveK : 0.01f, 0.01f, 20.f);
	return FMath::Clamp(FMath::Tanh(X * K), -1.f, 1.f);
}

int32 SAudioFXWaveShaperCurve::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const float Width = Size.X;
	const float Height = Size.Y;

	// ── Background ────────────────────────────────────────────────
	// Must pass the actual dark color as tint — FLinearColor::White renders the box white
	// because FSlateRoundedBoxBrush uses a white texture modulated by the tint parameter.
	FSlateDrawElement::MakeBox(
		OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(),
		&FAudioFXTheme::GetInsetBrush(),
		ESlateDrawEffect::None,
		FAudioFXTheme::GetWindowBackground());  // #0A0A0A — correct dark tint
	++LayerId;

	// ── Grid lines (horizontal & vertical center lines) — UIReview Issue 1 ──
	{
		const FLinearColor GridColor = FAudioFXTheme::GetGridMinorColor();   // #1A1A1A
		TArray<FVector2D> HLine = { FVector2D(0.f, Height * 0.5f), FVector2D(Width, Height * 0.5f) };
		TArray<FVector2D> VLine = { FVector2D(Width * 0.5f, 0.f), FVector2D(Width * 0.5f, Height) };
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), HLine, ESlateDrawEffect::None, GridColor, true, 1.0f);
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), VLine, ESlateDrawEffect::None, GridColor, true, 1.0f);
	}
	++LayerId;

	// ── 1:1 Reference diagonal (linear) — UIReview Issue 1 ──────
	{
		const FLinearColor RefColor = FAudioFXTheme::GetGridMajorColor();   // #222222
		TArray<FVector2D> Diag = { FVector2D(0.f, Height), FVector2D(Width, 0.f) };
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Diag, ESlateDrawEffect::None, RefColor, true, 1.0f);
	}
	++LayerId;

	// ── Transfer curve ────────────────────────────────────────────
	// x in [-1, 1] → y = tanh(x * DriveAmount), DriveAmount derived from Amount (0–500)
	// We clamp DriveAmount to [0.01, 20] for display; DriveAmount=1 = linear, DriveAmount=20 = brick-wall clip
	const float DriveAmount = AmountToDrive(Amount.Get(0.f));

	constexpr int32 NumSteps = 256;
	TArray<FVector2D> Curve;
	Curve.Reserve(NumSteps + 1);

	for (int32 PointIndex = 0; PointIndex <= NumSteps; ++PointIndex)
	{
		const float NormPos = static_cast<float>(PointIndex) / NumSteps;  // 0..1
		const float InputValue = NormPos * 2.f - 1.f;                      // -1..1
		const float OutputValue = EvaluateTransfer(InputValue, DriveAmount);

		// Map to pixel space: x=[0,Width], y=[Height,0] (y flipped: y=-1 → Height, y=+1 → 0)
		Curve.Add(FVector2D(NormPos * Width, (1.f - (OutputValue * 0.5f + 0.5f)) * Height));
	}

	FSlateDrawElement::MakeLines(
		OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(),
		Curve,
		ESlateDrawEffect::None,
		AssetColor.CopyWithNewOpacity(0.90f),
		true, 2.5f);
	++LayerId;

	return LayerId;
}
