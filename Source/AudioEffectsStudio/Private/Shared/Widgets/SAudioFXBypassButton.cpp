// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Widgets/SAudioFXBypassButton.h"
#include "Shared/Core/AudioFXTheme.h"

#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Rendering/DrawElementTypes.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "SAudioFXBypassButton"

void SAudioFXBypassButton::Construct(const FArguments& InArgs)
{
	IsBypassed = InArgs._IsBypassed;
	OnBypassChanged = InArgs._OnBypassChanged;

	ChildSlot
	[
		SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "NoBorder")
		.OnClicked(FOnClicked::CreateRaw(this, &SAudioFXBypassButton::OnClicked))
		.ToolTipText(LOCTEXT("BypassTip", "Toggle effect bypass in its current sound chain"))
		.ContentPadding(FMargin(0.0f))
		[
			SAssignNew(ButtonBorder, SBorder)
			.BorderImage(&FAudioFXTheme::GetBypassBrush())
			.Padding(FMargin(20.0f, 4.0f, 10.0f, 4.0f))  // Extra left padding for LED dot
			[
				SAssignNew(ButtonLabel, STextBlock)
				.Text(LOCTEXT("BypassLabel", "BYPASS"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
			]
		]
	];

	UpdateVisuals();
}

int32 SAudioFXBypassButton::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	// ── Layer 0: paint child widgets (border + "BYPASS" label) ────────────
	int32 MaxLayer = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect,
		OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	// ── Layer 1: LED dot drawn on top of children ─────────────────────────
	const bool bBypassed = IsBypassed.Get(false);
	const bool bHovered = IsHovered();

	// LED position: vertically centered, LEDLeftPadding from the left edge
	const FVector2f LocalSize = UE::Slate::CastToVector2f(AllottedGeometry.GetLocalSize());
	const float CenterX = LEDLeftPadding;
	const float CenterY = LocalSize.Y * 0.5f;

	{
		const float LEDDiameter = LEDRadius * 2.0f;

		// Choose LED color based on bypass state + hover:
		// Bypassed = dim red (glows brighter on hover); Active = dark grey (lifts on hover)
		FLinearColor LEDColor;
		if (bBypassed)
		{
			LEDColor = bHovered
				? FLinearColor(0.8f, 0.2f, 0.2f, 1.0f)                  // Slightly brighter on hover
				: FAudioFXTheme::GetBypassActiveColor();                  // Dim red dot
		}
		else
		{
			LEDColor = bHovered
				? FLinearColor(0.35f, 0.35f, 0.4f, 1.0f)                // Slightly brighter on hover
				: FLinearColor(0.22f, 0.22f, 0.28f, 1.0f);               // Dark grey
		}

		// LEDOnBrush is now a member — no static local

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			MaxLayer + 1,
			AllottedGeometry.ToPaintGeometry(
				FVector2f(LEDDiameter, LEDDiameter),
				FSlateLayoutTransform(FVector2f(CenterX - LEDRadius, CenterY - LEDRadius))),
			&LEDOnBrush,
			ESlateDrawEffect::None,
			LEDColor);
	}

	return MaxLayer + 2;
}

FReply SAudioFXBypassButton::OnClicked()
{
	const bool bNewState = !IsBypassed.Get(false);
	IsBypassed = TAttribute<bool>(bNewState);
	UpdateVisuals();
	OnBypassChanged.ExecuteIfBound(bNewState);
	return FReply::Handled();
}

void SAudioFXBypassButton::UpdateVisuals()
{
	const bool bBypassed = IsBypassed.Get(false);

	if (ButtonBorder.IsValid())
	{
		ButtonBorder->SetBorderImage(
			bBypassed
				? &FAudioFXTheme::GetBypassActiveBrush()
				: &FAudioFXTheme::GetBypassBrush()
		);
	}

	if (ButtonLabel.IsValid())
	{
		ButtonLabel->SetColorAndOpacity(
			bBypassed
				? FAudioFXTheme::GetBypassActiveColor()
				: FAudioFXTheme::GetBypassInactiveColor()
		);
	}
}

#undef LOCTEXT_NAMESPACE
