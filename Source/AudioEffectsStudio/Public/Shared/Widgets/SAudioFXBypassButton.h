// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class SBorder;
class STextBlock;

DECLARE_DELEGATE_OneParam(FOnAudioFXBypassChanged, bool /* bIsBypassed */);

/**
 * SAudioFXBypassButton
 *
 * A VST-style bypass toggle with an LED indicator dot.
 * When bypassed: glowing red LED dot + bright red "BYPASS" text.
 * When processing: dim grey LED dot + muted text.
 * The LED dot and optional glow halo are drawn in OnPaint.
 */
class AUDIOEFFECTSSTUDIO_API SAudioFXBypassButton : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAudioFXBypassButton)
		: _IsBypassed(false)
	{}
		/** Current bypass state */
		SLATE_ATTRIBUTE(bool, IsBypassed)

		/** Fired when bypass state changes */
		SLATE_EVENT(FOnAudioFXBypassChanged, OnBypassChanged)
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
	FReply OnClicked();
	void UpdateVisuals();

	TAttribute<bool> IsBypassed;
	FOnAudioFXBypassChanged OnBypassChanged;

	TSharedPtr<SBorder> ButtonBorder;
	TSharedPtr<STextBlock> ButtonLabel;

	/** LED dot constants */
	static constexpr float LEDRadius = 2.5f;
	static constexpr float LEDLeftPadding = 10.0f;  // From left edge of button to LED center

	// Brush built in Construct, not as static local in OnPaint
	mutable FSlateRoundedBoxBrush LEDOnBrush{ FLinearColor::White, LEDRadius };
};
