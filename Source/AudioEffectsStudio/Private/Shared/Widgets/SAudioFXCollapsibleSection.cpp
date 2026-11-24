// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Widgets/SAudioFXCollapsibleSection.h"
#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Core/AudioFXEffectEditor.h"   // full include in .cpp only — no circular dependency

#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "SAudioFXCollapsibleSection"

void SAudioFXCollapsibleSection::Construct(const FArguments& InArgs)
{
	bExpanded          = InArgs._bDefaultExpanded;
	OnToggledDelegate  = InArgs._OnToggled;
	OwnerEditorWeak    = InArgs._OwnerEditor;
	HeightOffset       = InArgs._HeightOffset;

	// Same card visuals as SAudioFXSectionBox: #111115 fill, #1A1A22 border, 6px radius
	CardBrush = MakeUnique<FSlateRoundedBoxBrush>(
		FLinearColor(FColor(0x11, 0x11, 0x15)),
		FAudioFXTheme::CardRadius,
		FLinearColor(FColor(0x1A, 0x1A, 0x22)),
		FAudioFXTheme::OutlineWidth);

	ContentWidget = InArgs._Content.Widget;

	ChildSlot
	[
		SNew(SBox)
		.Padding(FMargin(0.0f, FAudioFXTheme::SectionGap * 0.5f, 0.0f, 0.0f))
		[
			SNew(SBorder)
			.BorderImage(CardBrush.Get())
			.Padding(FMargin(12.0f, 8.0f, 12.0f, 12.0f))
			[
				SNew(SVerticalBox)

				// ── Header row: label (left) + arrow toggle (right) ──
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, bExpanded ? 8.0f : 0.0f)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "NoBorder")
					.ContentPadding(FMargin(0.f))
					.Cursor(EMouseCursor::Hand)
					.OnClicked(this, &SAudioFXCollapsibleSection::OnHeaderClicked)
					[
						SNew(SHorizontalBox)

						// Section label
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(InArgs._SectionLabel)
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
							.ColorAndOpacity(FSlateColor(FLinearColor(FColor(0x70, 0x70, 0x80))))
						]

						// Collapse arrow — image icon (ChevronDown/Right)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SBox)
							.WidthOverride(10.f)
							.HeightOverride(10.f)
							[
								SNew(SImage)
								.Image_Lambda([this]() -> const FSlateBrush*
								{
									return bExpanded
										? FAppStyle::GetBrush("Icons.ChevronDown")
										: FAppStyle::GetBrush("Icons.ChevronRight");
								})
								.ColorAndOpacity(FSlateColor(FLinearColor(FColor(0x70, 0x70, 0x80))))
							]
						]
					]
				]

				// ── Collapsible content ────────────────────────────
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBox)
					.Visibility_Lambda([this]() -> EVisibility
					{
						return bExpanded ? EVisibility::Visible : EVisibility::Collapsed;
					})
					[
						ContentWidget.ToSharedRef()
					]
				]
			]
		]
	];
}

FReply SAudioFXCollapsibleSection::OnHeaderClicked()
{
	bExpanded = !bExpanded;

	// ── Step 1: invalidate layout so the Visibility_Lambda on the content SBox fires ──
	Invalidate(EInvalidateWidgetReason::Layout);

	// ── Step 2: auto-resize the owner window if configured ────────────────
	if (HeightOffset > 0.f)
	{
		if (TSharedPtr<FAudioFXEffectEditor> Editor = OwnerEditorWeak.Pin())
		{
			Editor->ResizeWindowByDelta(bExpanded ? HeightOffset : -HeightOffset);
		}
	}

	// ── Step 3: fire the legacy callback for any code still using OnToggled ──
	OnToggledDelegate.ExecuteIfBound();

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
