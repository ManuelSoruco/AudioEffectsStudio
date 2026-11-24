// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

/**
 * SAudioFXIconButton
 *
 * A small icon button with hover glow for the preset bar and toolbar areas.
 * On hover: icon brightens and a subtle rounded background appears.
 * On rest: icon is dim (LabelTextColor), no visible background.
 *
 * Supports an optional IsDirty attribute that draws a small orange dot
 * in the bottom-right corner (e.g. for the Save button when preset is unsaved).
 */
class AUDIOEFFECTSSTUDIO_API SAudioFXIconButton : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAudioFXIconButton)
		: _IconSize(10.0f, 10.0f)
		, _IsDirty(false)
	{}
		/** Brush for the icon image */
		SLATE_ARGUMENT(const FSlateBrush*, IconBrush)

		/** Icon size override */
		SLATE_ARGUMENT(FVector2D, IconSize)

		/** Tooltip text */
		SLATE_ARGUMENT(FText, ToolTipText)

		/** When true, draws a small orange "unsaved" dot in the bottom-right corner */
		SLATE_ATTRIBUTE(bool, IsDirty)

		/**
		 * When the button is in an "active" state, the icon glows this color
		 * (e.g. teal for Play-while-playing, orange for Loop-when-enabled).
		 * Pass FLinearColor::Transparent (default) for normal behavior.
		 */
		SLATE_ATTRIBUTE(FLinearColor, ActiveColor)

		/** Click handler */
		SLATE_EVENT(FOnClicked, OnClicked)
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

	/** Tick to update icon tint from ActiveColor attribute */
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	TSharedPtr<class SBorder> HoverBorder;
	TSharedPtr<class SImage> IconImage;

	TAttribute<bool> IsDirty;
	TAttribute<FLinearColor> ActiveColor;

	FLinearColor NormalIconColor = FLinearColor::Gray;
	FLinearColor HoverIconColor = FLinearColor::White;

	/** Last known active color — used to detect changes in Tick */
	mutable FLinearColor LastActiveColor = FLinearColor::Transparent;
};
