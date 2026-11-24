// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Widgets/SAudioFXIconButton.h"
#include "Shared/Core/AudioFXTheme.h"

#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Rendering/DrawElementTypes.h"
#include "Styling/AppStyle.h"

void SAudioFXIconButton::Construct(const FArguments& InArgs)
{
	IsDirty = InArgs._IsDirty;
	ActiveColor = InArgs._ActiveColor;
	NormalIconColor = FAudioFXTheme::GetLabelTextColor();
	HoverIconColor = FAudioFXTheme::GetValueTextColor();

	static FSlateRoundedBoxBrush HoverBgBrush(
		FLinearColor(1.0f, 1.0f, 1.0f, 0.06f),
		FAudioFXTheme::ButtonRadius);

	static FSlateRoundedBoxBrush EmptyBrush(
		FLinearColor::Transparent,
		FAudioFXTheme::ButtonRadius);

	const FSlateBrush* HoverBgBrushPtr = &HoverBgBrush;
	const FSlateBrush* EmptyBrushPtr = &EmptyBrush;

	ChildSlot
	[
		SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "NoBorder")
		.OnClicked(InArgs._OnClicked)
		.ToolTipText(InArgs._ToolTipText)
		.ContentPadding(FMargin(0.0f))
		.OnHovered_Lambda([this, HoverBgBrushPtr]()
		{
			if (HoverBorder.IsValid()) HoverBorder->SetBorderImage(HoverBgBrushPtr);
			if (IconImage.IsValid()) IconImage->SetColorAndOpacity(HoverIconColor);
		})
		.OnUnhovered_Lambda([this, EmptyBrushPtr]()
		{
			if (HoverBorder.IsValid()) HoverBorder->SetBorderImage(EmptyBrushPtr);
			const FLinearColor AC = ActiveColor.Get(FLinearColor::Transparent);
			if (IconImage.IsValid())
			{
				IconImage->SetColorAndOpacity(AC.A > 0.01f ? AC : NormalIconColor);
			}
		})
		[
			SAssignNew(HoverBorder, SBorder)
			.BorderImage(EmptyBrushPtr)
			.Padding(FMargin(3.0f, 2.0f))
			[
				SAssignNew(IconImage, SImage)
				.Image(InArgs._IconBrush)
				.ColorAndOpacity(NormalIconColor)
				.DesiredSizeOverride(InArgs._IconSize)
			]
		]
	];
}

void SAudioFXIconButton::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	// Polling the TAttribute each tick is intentional: ActiveColor is typically bound
	// to a lambda that reads engine state (e.g. "is playing?"), so it can change any frame.
	// We cache LastActiveColor to skip redundant SetColorAndOpacity() calls.
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (!IconImage.IsValid())
	{
		return;
	}

	const FLinearColor Current = ActiveColor.Get(FLinearColor::Transparent);

	// Only update the icon color when the active color actually changes
	if (!Current.Equals(LastActiveColor, 0.001f))
	{
		LastActiveColor = Current;
		IconImage->SetColorAndOpacity(Current.A > 0.01f ? Current : NormalIconColor);
	}
}

int32 SAudioFXIconButton::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	int32 MaxLayer = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect,
		OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	// Draw the orange unsaved-dot in the bottom-right corner when dirty
	if (IsDirty.Get(false))
	{
		constexpr float DotRadius = 2.5f;
		constexpr float DotDiameter = DotRadius * 2.0f;

		const FVector2f LocalSize = UE::Slate::CastToVector2f(AllottedGeometry.GetLocalSize());

		// Position: bottom-right, 2px inset from edges
		const float DotX = LocalSize.X - DotDiameter - 2.0f;
		const float DotY = LocalSize.Y - DotDiameter - 2.0f;

		static FSlateRoundedBoxBrush DotBrush(
			FLinearColor(1.0f, 0.5f, 0.0f, 1.0f),  // Accent orange
			DotRadius);

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			MaxLayer + 1,
			AllottedGeometry.ToPaintGeometry(
				FVector2f(DotDiameter, DotDiameter),
				FSlateLayoutTransform(FVector2f(DotX, DotY))),
			&DotBrush,
			ESlateDrawEffect::None,
			FLinearColor(1.0f, 0.5f, 0.0f, 1.0f));  // Orange tint — White would render the box white
	}

	return MaxLayer + (IsDirty.Get(false) ? 2 : 1);
}
