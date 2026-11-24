// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

/**
 * SAudioFXPanDisplay
 *
 * Real-time stereo pan position indicator drawn entirely in OnPaint.
 *
 * Shows:
 *   - Dark inset background
 *   - "L" and "R" labels at the far left/right
 *   - A horizontal track bar (the stereo field)
 *   - Center tick mark
 *   - A glowing "puck" dot at the current Pan position (-1 = full L, +1 = full R)
 *   - Spread arc — a highlighted region around center showing the Spread width
 *
 * Both Pan and Spread update live via TAttribute bindings.
 * All drawing is pure Slate — no materials.
 */
class AUDIOEFFECTSSTUDIO_API SAudioFXPanDisplay : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SAudioFXPanDisplay)
		: _Pan(0.f)
		, _Spread(0.f)
		, _AssetColor(FLinearColor(0.498f, 0.608f, 0.396f))
	{}
		/** Pan position: -1 = full left, 0 = center, +1 = full right */
		SLATE_ATTRIBUTE(float, Pan)

		/** Spread width: 0 = mono, 1 = full stereo */
		SLATE_ATTRIBUTE(float, Spread)

		/** Effect asset color — declared last to match member init order */
		SLATE_ARGUMENT(FLinearColor, AssetColor)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;

	static float ClampPanValue(float InPan);
	static float ClampSpreadValue(float InSpread);
	static float PanToNormalizedX(float InPan);
	static void ComputeSpreadBoundsNormalized(float InPan, float InSpread, float& OutLeft, float& OutRight);

private:
	TAttribute<float> Pan;
	TAttribute<float> Spread;
	FLinearColor AssetColor = FLinearColor(0.498f, 0.608f, 0.396f);

	// Brushes built in Construct, not as static locals in OnPaint
	FSlateRoundedBoxBrush SpreadBrush{ FLinearColor::White, 2.f };
	// PuckBrush radius is set in Construct based on desired puck size
	mutable FSlateRoundedBoxBrush PuckBrush{ FLinearColor::White, 5.f };
};
