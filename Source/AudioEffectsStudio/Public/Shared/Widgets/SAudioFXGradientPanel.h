// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Brushes/SlateColorBrush.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

/**
 * SAudioFXGradientPanel
 *
 * A VST-style background panel that draws a subtle vertical gradient
 * (slightly lighter at top, darker at bottom) behind its child content.
 * Provides depth and a "lit from above" feel typical of professional
 * audio plugins, replacing flat solid-color backgrounds.
 *
 * The gradient is drawn via FSlateDrawElement::MakeGradient in OnPaint,
 * then child content is composited on top by the SCompoundWidget slot.
 */
class AUDIOEFFECTSSTUDIO_API SAudioFXGradientPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAudioFXGradientPanel)
		: _TopColor(FLinearColor(FColor(0x1E, 0x1E, 0x1E)))     // Surface.Raised #1E1E1E — lit from above
		, _BottomColor(FLinearColor(FColor(0x0D, 0x0D, 0x0D)))  // Surface.Deep #0D0D0D — darker at base (P6 fix)
		, _CornerRadius(8.0f)
		, _AccentLineColor(FLinearColor::Transparent)
		, _AccentLineHeight(2.0f)
	{}
		/** Color at the top of the gradient (lighter — "light source from above") */
		SLATE_ARGUMENT(FLinearColor, TopColor)

		/** Color at the bottom of the gradient (darker) */
		SLATE_ARGUMENT(FLinearColor, BottomColor)

		/** Corner radius for the gradient box (matches PanelRadius) */
		SLATE_ARGUMENT(float, CornerRadius)

		/** Optional accent line color drawn at the very top of the panel */
		SLATE_ARGUMENT(FLinearColor, AccentLineColor)

		/** Height of the accent line in pixels (default 2px) */
		SLATE_ARGUMENT(float, AccentLineHeight)

		/** Child content rendered on top of the gradient */
		SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// SWidget overrides
	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	FLinearColor TopColor = FLinearColor(0.078f, 0.078f, 0.078f, 1.0f);
	FLinearColor BottomColor = FLinearColor(0.078f, 0.078f, 0.078f, 1.0f);
	float CornerRadius = 8.0f;
	FLinearColor AccentLineColor = FLinearColor::Transparent;
	float AccentLineHeight = 2.0f;

	// Brush built in Construct, not as static local in OnPaint
	FSlateColorBrush AccentBrush{ FLinearColor::White };
};
