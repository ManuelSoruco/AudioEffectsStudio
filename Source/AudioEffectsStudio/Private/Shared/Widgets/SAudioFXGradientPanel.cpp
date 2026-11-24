// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Widgets/SAudioFXGradientPanel.h"

#include "Rendering/DrawElementTypes.h"
#include "Brushes/SlateColorBrush.h"

void SAudioFXGradientPanel::Construct(const FArguments& InArgs)
{
	TopColor = InArgs._TopColor;
	BottomColor = InArgs._BottomColor;
	CornerRadius = InArgs._CornerRadius;
	AccentLineColor = InArgs._AccentLineColor;
	AccentLineHeight = InArgs._AccentLineHeight;

	ChildSlot
	[
		InArgs._Content.Widget
	];
}

int32 SAudioFXGradientPanel::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const FVector2f LocalSize = UE::Slate::CastToVector2f(AllottedGeometry.GetLocalSize());
	const float CornerRadiusValue = CornerRadius;

	// ── Layer 0: Vertical gradient (top lighter → bottom darker) ─────────
	TArray<FSlateGradientStop> GradientStops;
	GradientStops.Emplace(FVector2f(0.0f, 0.0f), TopColor);
	GradientStops.Emplace(FVector2f(LocalSize.X, LocalSize.Y), BottomColor);

	FSlateDrawElement::MakeGradient(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(),
		MoveTemp(GradientStops),
		Orient_Vertical,
		ESlateDrawEffect::None,
		FVector4f(CornerRadiusValue, CornerRadiusValue, CornerRadiusValue, CornerRadiusValue));

	// ── Layer 1: Optional accent line at top ─────────────────────────
	// AccentBrush is white; the AccentLineColor tint modulates it (standard Slate pattern)
	if (AccentLineColor.A > 0.0f)
	{
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId + 1,
			AllottedGeometry.ToPaintGeometry(
				FVector2f(LocalSize.X, AccentLineHeight),
				FSlateLayoutTransform(FVector2f(0.0f, 0.0f))),
			&AccentBrush,
			ESlateDrawEffect::None,
			AccentLineColor);
	}

	// Paint children above all background layers
	return SCompoundWidget::OnPaint(
		Args,
		AllottedGeometry,
		MyCullingRect,
		OutDrawElements,
		LayerId + 2,
		InWidgetStyle,
		bParentEnabled);
}
