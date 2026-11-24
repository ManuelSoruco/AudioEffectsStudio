// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Widgets/SAudioFXAdvancedSection.h"
#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Core/AudioFXEffectEditor.h"  // full include in .cpp only (avoids circular dep in .h)

#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "SAudioFXAdvancedSection"

void SAudioFXAdvancedSection::Construct(const FArguments& InArgs)
{
	HeaderLabel       = InArgs._Label;
	CachedAssetColor  = InArgs._AssetColor;
	bIsExpanded       = InArgs._bStartExpanded;
	HeightOffset      = InArgs._HeightOffset;
	OwnerEditorWeak   = InArgs._OwnerEditor;
	OnExpandedChangedDelegate = InArgs._OnExpandedChanged;

	// Horizontal divider brush drawn across the full width
	static FSlateRoundedBoxBrush DividerBrush(
		FLinearColor(0.12f, 0.12f, 0.17f, 1.0f),  // Matches section outline color
		0.0f);

	ChildSlot
	[
		SNew(SVerticalBox)

		// Toggle header
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				.HeightOverride(1.0f)
				[
					SNew(SBorder)
					.BorderImage(&DividerBrush)
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.ContentPadding(FMargin(FAudioFXTheme::SectionInnerPadding, 5.0f))
				.OnClicked(this, &SAudioFXAdvancedSection::OnToggleClicked)
				.Cursor(EMouseCursor::Hand)
				[
					SNew(SHorizontalBox)

					// Arrow icon (chevron) — uses FAppStyle brush
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.0f, 0.0f, 5.0f, 0.0f)
					[
						SNew(SBox)
						.WidthOverride(12.f)
						.HeightOverride(12.f)
						[
							SAssignNew(ArrowImage, SImage)
							.Image(this, &SAudioFXAdvancedSection::GetArrowBrush)
							.ColorAndOpacity(this, &SAudioFXAdvancedSection::GetHeaderTextColor)
						]
					]

					// Label text ────────────────────────────────────
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(HeaderLabel)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
						.ColorAndOpacity(this, &SAudioFXAdvancedSection::GetHeaderTextColor)
					]

					// ── Trailing horizontal rule (fills remaining width) ──
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					.Padding(8.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(SBox)
						.HeightOverride(1.0f)
						[
							SNew(SBorder)
							.BorderImage(&DividerBrush)
						]
					]
				]
			]
		]

		// ── Collapsible content area ───────────────────────────────────
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew(ContentContainer, SBorder)
			.BorderImage(&FAudioFXTheme::GetSectionBrush())
			.Padding(FMargin(FAudioFXTheme::SectionInnerPadding))
			.Visibility(bIsExpanded ? EVisibility::Visible : EVisibility::Collapsed)
			[
				InArgs._Content.Widget
			]
		]
	];
}

FReply SAudioFXAdvancedSection::OnToggleClicked()
{
	bIsExpanded = !bIsExpanded;

	// ── Step 1: toggle content container visibility ────────────────────────
	if (ContentContainer.IsValid())
	{
		ContentContainer->SetVisibility(bIsExpanded ? EVisibility::Visible : EVisibility::Collapsed);
	}

	// ── Step 2: auto-resize the owner window if configured ────────────────
	if (HeightOffset != 0.f)
	{
		if (TSharedPtr<FAudioFXEffectEditor> Editor = OwnerEditorWeak.Pin())
		{
			const float Delta = bIsExpanded ? HeightOffset : -HeightOffset;
			Editor->ResizeWindowByDelta(Delta);
		}
	}

	// ── Step 3: fire the legacy delegate for any editor still using it ─────
	OnExpandedChangedDelegate.ExecuteIfBound(bIsExpanded);

	return FReply::Handled();
}

const FSlateBrush* SAudioFXAdvancedSection::GetArrowBrush() const
{
	// Use the same FAppStyle chevron brushes as the preset bar's browse button.
	// "Icons.ChevronDown" = expanded, "Icons.ChevronRight" = collapsed.
	return bIsExpanded
		? FAppStyle::GetBrush("Icons.ChevronDown")
		: FAppStyle::GetBrush("Icons.ChevronRight");
}

FSlateColor SAudioFXAdvancedSection::GetHeaderTextColor() const
{
	if (bIsExpanded)
	{
		const FLinearColor Tinted = FMath::Lerp(
			FAudioFXTheme::GetLabelTextColor(),
			CachedAssetColor,
			0.6f);
		return FSlateColor(Tinted);
	}
	return FSlateColor(FAudioFXTheme::GetLabelTextColor());
}

FSlateColor SAudioFXAdvancedSection::GetHeaderBackgroundColor() const
{
	if (bIsExpanded)
	{
		return FSlateColor(FMath::Lerp(
			FAudioFXTheme::GetSectionBackground(),
			CachedAssetColor,
			0.05f));
	}
	return FSlateColor(FLinearColor::Transparent);
}

#undef LOCTEXT_NAMESPACE
