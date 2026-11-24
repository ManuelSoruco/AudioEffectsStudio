// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Widgets/SAudioFXDropdown.h"
#include "Shared/Core/AudioFXTheme.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SMenuAnchor.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

// ─── Layout constants ────────────────────────────────────────────────────────
namespace
{
	constexpr float  kButtonPadH    = 7.f;   // horizontal padding inside trigger button text
	constexpr float  kButtonPadV    = 4.f;   // vertical   padding inside trigger button text
	constexpr float  kRowPadH       = 10.f;  // horizontal padding inside each popup row
	constexpr float  kRowPadV       = 5.f;   // vertical   padding inside each popup row
	constexpr float  kChevronGap    = 4.f;   // gap between label and chevron ▾
	constexpr float  kPopupRadius   = 6.f;   // popup panel corner radius
	constexpr float  kRowRadius     = 4.f;   // individual row corner radius
	constexpr float  kPopupOutline  = 1.f;   // popup border width
	constexpr float  kMinPopupWidth = 80.f;  // popup won't be narrower than this
}

// ─── Construct ───────────────────────────────────────────────────────────────
void SAudioFXDropdown::Construct(const FArguments& InArgs)
{
	Options       = InArgs._Options;
	AssetColor    = InArgs._AssetColor;
	SelectedIndex = InArgs._SelectedIndex;
	OnSelectionChanged = InArgs._OnSelectionChanged;
	MinWidth      = InArgs._MinWidth;

	// Trigger button body brush — same dark base as ButtonStrip inactive
	const FLinearColor TriggerBg(FColor(0x0E, 0x0E, 0x0E));
	const FLinearColor TriggerHoverBg(FColor(0x1A, 0x1A, 0x1A));
	const FLinearColor TriggerOutline(FColor(0x2A, 0x2A, 0x2A));
	const FLinearColor TriggerHoverOutline(FColor(0x3A, 0x3A, 0x3A));

	BodyBrush = MakeUnique<FSlateRoundedBoxBrush>(
		TriggerBg,
		FAudioFXTheme::ButtonRadius,
		TriggerOutline,
		FAudioFXTheme::OutlineWidth);

	BodyHoverBrush = MakeUnique<FSlateRoundedBoxBrush>(
		TriggerHoverBg,
		FAudioFXTheme::ButtonRadius,
		TriggerHoverOutline,
		FAudioFXTheme::OutlineWidth);

	// Popup panel body brush
	PopupBrush = MakeUnique<FSlateRoundedBoxBrush>(
		FAudioFXTheme::GetPanelBackground(),
		kPopupRadius,
		FMath::Lerp(FAudioFXTheme::GetDividerColor(), AssetColor, 0.25f),
		kPopupOutline);

	const FSlateBrush* NormalBrushPtr = BodyBrush.Get();
	const FSlateBrush* HoverBrushPtr  = BodyHoverBrush.Get();

	TSharedRef<SMenuAnchor> Anchor = SAssignNew(MenuAnchor, SMenuAnchor)
		.Placement(MenuPlacement_ComboBox)
		.OnGetMenuContent(this, &SAudioFXDropdown::BuildPopupContent)
		[
			// ── Trigger — NoBorder SButton wraps a styled SBorder ──────────
			SAssignNew(TriggerBorder, SBorder)
			.BorderImage(NormalBrushPtr)
			.Padding(FMargin(0.f))
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.ContentPadding(FMargin(kButtonPadH, kButtonPadV))
				.OnClicked_Lambda([this]() -> FReply
				{
					TogglePopup();
					return FReply::Handled();
				})
				.OnHovered_Lambda([this, HoverBrushPtr]()
				{
					if (TriggerBorder.IsValid()) TriggerBorder->SetBorderImage(HoverBrushPtr);
				})
				.OnUnhovered_Lambda([this, NormalBrushPtr]()
				{
					if (TriggerBorder.IsValid()) TriggerBorder->SetBorderImage(NormalBrushPtr);
				})
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(this, &SAudioFXDropdown::GetSelectedText)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						.ColorAndOpacity(FAudioFXTheme::GetValueTextColor())
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(kChevronGap, 0.f, 0.f, 0.f)
					[
						SNew(SBox)
						.WidthOverride(8.f)
						.HeightOverride(8.f)
						[
							SNew(SImage)
							.Image(FAppStyle::GetBrush("Icons.ChevronDown"))
							.ColorAndOpacity(FAudioFXTheme::GetTextDisabledColor())
						]
					]
				]
			]
		];

	// Wrap in SBox with MinDesiredWidth if requested (prevents layout
	// reshuffle when selected text length changes between short and long options).
	if (MinWidth > 0.f)
	{
		ChildSlot
		[
			SNew(SBox)
			.MinDesiredWidth(MinWidth)
			[ Anchor ]
		];
	}
	else
	{
		ChildSlot
		[ Anchor ];
	}
}

// ─── Popup content ───────────────────────────────────────────────────────────
TSharedRef<SWidget> SAudioFXDropdown::BuildPopupContent()
{
	// Clear and rebuild brush storage for this popup invocation
	PopupRowBrushes.Empty(Options.Num());

	TSharedRef<SVerticalBox> RowBox = SNew(SVerticalBox);

	const int32 CurrentSel = GetSelectedIndex();

	for (int32 Index = 0; Index < Options.Num(); ++Index)
	{
		const bool bIsSelected = (Index == CurrentSel);
		const FLinearColor RowFill = bIsSelected
			? FMath::Lerp(FAudioFXTheme::GetSectionBackground(), AssetColor, 0.18f)
			: FAudioFXTheme::GetSectionBackground();
		const FLinearColor TextCol = bIsSelected
			? FMath::Lerp(FAudioFXTheme::GetValueTextColor(), AssetColor, 0.4f)
			: FAudioFXTheme::GetValueTextColor();

		// Store brush in member array to keep it alive for the popup lifetime
		TSharedPtr<FSlateRoundedBoxBrush> RowBrush = MakeShared<FSlateRoundedBoxBrush>(RowFill, kRowRadius);
		PopupRowBrushes.Add(RowBrush);
		const FSlateBrush* RowBrushPtr = RowBrush.Get();
		const int32 CapturedIndex = Index;

		RowBox->AddSlot()
		.AutoHeight()
		.Padding(3.f, 2.f)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.ContentPadding(FMargin(0.f))
			.OnClicked_Lambda([this, CapturedIndex]() -> FReply
			{
				SelectOption(CapturedIndex);
				return FReply::Handled();
			})
			[
				SNew(SBorder)
				.BorderImage(RowBrushPtr)
				.Padding(FMargin(kRowPadH, kRowPadV))
				[
					SNew(STextBlock)
					.Text(Options[Index])
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
					.ColorAndOpacity(TextCol)
				]
			]
		];
	}

	return SNew(SBorder)
		.BorderImage(PopupBrush.Get())
		.Padding(FMargin(0.f, 2.f))
		[
			SNew(SBox)
			.MinDesiredWidth(kMinPopupWidth)
			[ RowBox ]
		];
}

// ─── Popup toggle ─────────────────────────────────────────────────────────
void SAudioFXDropdown::TogglePopup()
{
	if (MenuAnchor.IsValid())
	{
		if (MenuAnchor->IsOpen())
		{
			MenuAnchor->SetIsOpen(false);
		}
		else
		{
			MenuAnchor->SetIsOpen(true, false);
		}
	}
}

// ─── Select an option ─────────────────────────────────────────────────────
void SAudioFXDropdown::SelectOption(int32 Index)
{
	if (MenuAnchor.IsValid())
	{
		MenuAnchor->SetIsOpen(false);
	}
	if (Index != GetSelectedIndex())
	{
		SelectedIndex = TAttribute<int32>(Index);
		OnSelectionChanged.ExecuteIfBound(Index);
	}
}

// ─── Public helpers ────────────────────────────────────────────────────────
void SAudioFXDropdown::SetSelectedIndex(int32 NewIndex)
{
	SelectedIndex = TAttribute<int32>(NewIndex);
}

int32 SAudioFXDropdown::GetSelectedIndex() const
{
	return SelectedIndex.Get(INDEX_NONE);
}

FText SAudioFXDropdown::GetSelectedText() const
{
	const int32 Idx = GetSelectedIndex();
	return Options.IsValidIndex(Idx) ? Options[Idx] : FText::GetEmpty();
}
