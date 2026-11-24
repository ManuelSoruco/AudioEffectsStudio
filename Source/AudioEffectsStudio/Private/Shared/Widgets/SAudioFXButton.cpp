// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Widgets/SAudioFXButton.h"
#include "Shared/Core/AudioFXTheme.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "SAudioFXButton"

void SAudioFXButton::Construct(const FArguments& InArgs)
{
	Options = InArgs._Options;
	AssetColor = InArgs._AssetColor;
	SelectedIndex = InArgs._SelectedIndex;
	OnSelectionChanged = InArgs._OnSelectionChanged;

	ChildSlot
	[
		SAssignNew(ButtonBox, SHorizontalBox)
	];

	RebuildButtons();
}

void SAudioFXButton::RebuildButtons()
{
	// Rebuild the entire button row from scratch. Called on Construct and whenever
	// SelectedIndex changes so active/inactive styling is always up-to-date.
	ButtonBox->ClearChildren();
	ButtonBrushes.Empty();
	HoverBrushes.Empty();

	const int32 CurrentSelection = SelectedIndex.Get(0);

	for (int32 Index = 0; Index < Options.Num(); ++Index)
	{
		const bool bIsActive = (Index == CurrentSelection);

		// ── Normal state colors ──────────────────────────────────────────
		const FLinearColor BgColor = bIsActive
			? FMath::Lerp(FAudioFXTheme::GetSurfaceRaised(), AssetColor, 0.60f)
			: FLinearColor(FColor(0x0E, 0x0E, 0x0E));

		const FLinearColor OutlineColor = bIsActive
			? AssetColor
			: FLinearColor(FColor(0x1A, 0x1A, 0x1A));

		// ── Hover state colors ───────────────────────────────────────────
		// Brighten background by 30% + 3% lift so the hover is visible on dark buttons
		const FLinearColor HoverBgColor = FLinearColor(
			FMath::Min(BgColor.R * 1.3f + 0.03f, 1.0f),
			FMath::Min(BgColor.G * 1.3f + 0.03f, 1.0f),
			FMath::Min(BgColor.B * 1.3f + 0.03f, 1.0f),
			1.0f);

		const FLinearColor HoverOutlineColor = bIsActive
			? AssetColor
			: FLinearColor(
				FMath::Min(OutlineColor.R * 1.5f + 0.05f, 1.0f),
				FMath::Min(OutlineColor.G * 1.5f + 0.05f, 1.0f),
				FMath::Min(OutlineColor.B * 1.5f + 0.05f, 1.0f),
				1.0f);

		const FLinearColor TextColor = bIsActive
			? FAudioFXTheme::GetValueTextColor()
			: FLinearColor(FColor(0x66, 0x66, 0x66));

		const FLinearColor HoverTextColor = bIsActive
			? FAudioFXTheme::GetValueTextColor()
			: FLinearColor(FColor(0x99, 0x99, 0x99));

		// ── Brush creation ───────────────────────────────────────────────
		// Brushes are stored as TUniquePtr arrays on the widget so they outlive the frame
		TUniquePtr<FSlateRoundedBoxBrush>& NormalBrush = ButtonBrushes.Emplace_GetRef(
			MakeUnique<FSlateRoundedBoxBrush>(
				BgColor,
				FAudioFXTheme::ButtonRadius,
				OutlineColor,
				FAudioFXTheme::OutlineWidth));

		TUniquePtr<FSlateRoundedBoxBrush>& HoverBrush = HoverBrushes.Emplace_GetRef(
			MakeUnique<FSlateRoundedBoxBrush>(
				HoverBgColor,
				FAudioFXTheme::ButtonRadius,
				HoverOutlineColor,
				FAudioFXTheme::OutlineWidth));

		const FSlateBrush* NormalBrushPtr = NormalBrush.Get();
		const FSlateBrush* HoverBrushPtr  = HoverBrush.Get();

		// ── Widget creation ──────────────────────────────────────────────
		TSharedPtr<SBorder>    ItemBorder;
		TSharedPtr<STextBlock> ItemText;

		ButtonBox->AddSlot()
			.AutoWidth()
			// Small gap between adjacent buttons; no left gap for the first button
			.Padding(Index > 0 ? FAudioFXTheme::ButtonGroupSpacing : 0.0f, 0.0f, 0.0f, 0.0f)
			[
				SAssignNew(ItemBorder, SBorder)
				.BorderImage(NormalBrushPtr)
				.Padding(FMargin(0.0f))
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "NoBorder")
					.ContentPadding(FMargin(10.0f, 4.0f))
					.OnClicked(FOnClicked::CreateRaw(this, &SAudioFXButton::OnButtonClicked, Index))
					.OnHovered_Lambda([ItemBorder, HoverBrushPtr, ItemText, HoverTextColor]()
					{
						if (ItemBorder.IsValid()) ItemBorder->SetBorderImage(HoverBrushPtr);
						if (ItemText.IsValid())   ItemText->SetColorAndOpacity(HoverTextColor);
					})
					.OnUnhovered_Lambda([ItemBorder, NormalBrushPtr, ItemText, TextColor]()
					{
						if (ItemBorder.IsValid()) ItemBorder->SetBorderImage(NormalBrushPtr);
						if (ItemText.IsValid())   ItemText->SetColorAndOpacity(TextColor);
					})
					[
						SAssignNew(ItemText, STextBlock)
						.Text(Options[Index])
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
						.ColorAndOpacity(TextColor)
					]
				]
			];
	}
}

void SAudioFXButton::SetSelectedIndex(int32 NewIndex)
{
	SelectedIndex = TAttribute<int32>(NewIndex);
	RebuildButtons();
}

FReply SAudioFXButton::OnButtonClicked(int32 Index)
{
	const int32 Current = SelectedIndex.Get(0);

	if (Options.Num() == 1)
	{
		// Single-option: toggle between active (0) and inactive (-1)
		const int32 NewIndex = (Current == Index) ? -1 : Index;
		SelectedIndex = TAttribute<int32>(NewIndex);
		RebuildButtons();
		OnSelectionChanged.ExecuteIfBound(NewIndex);
	}
	else if (Index != Current)
	{
		// Multi-option: radio behavior — only change if different
		SelectedIndex = TAttribute<int32>(Index);
		RebuildButtons();
		OnSelectionChanged.ExecuteIfBound(Index);
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
