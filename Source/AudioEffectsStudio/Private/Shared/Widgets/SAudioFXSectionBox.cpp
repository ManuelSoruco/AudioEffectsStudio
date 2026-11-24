// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Widgets/SAudioFXSectionBox.h"
#include "Shared/Core/AudioFXTheme.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Brushes/SlateRoundedBoxBrush.h"

#define LOCTEXT_NAMESPACE "SAudioFXSectionBox"

void SAudioFXSectionBox::Construct(const FArguments& InArgs)
{
	SetVisibility(InArgs._Visibility);

	// Panel Card: #111115 fill, #1A1A22 border (1px), 6px corner radius
	// Group label at top-left inside card (8pt, #707080), 12px inner padding
	CardBrush = MakeUnique<FSlateRoundedBoxBrush>(
		FLinearColor(FColor(0x11, 0x11, 0x15)),   // #111115 — section card fill
		FAudioFXTheme::CardRadius,
		FLinearColor(FColor(0x1A, 0x1A, 0x22)),   // #1A1A22 — card border
		FAudioFXTheme::OutlineWidth);

	ChildSlot
	[
		// Outer spacing between section cards
		SNew(SBox)
		.Padding(FMargin(0.0f, FAudioFXTheme::SectionGap * 0.5f, 0.0f, 0.0f))
		[
			SNew(SBorder)
			.BorderImage(CardBrush.Get())
			.Padding(FMargin(12.0f))
			[
				SNew(SVerticalBox)

				// ── Section header label — top-left inside card ───
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 6.0f)
				[
					SNew(STextBlock)
					.Text(InArgs._SectionLabel)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(FSlateColor(FLinearColor(FColor(0x70, 0x70, 0x80))))  // #707080
				]

				// ── Content ───────────────────────────────────────
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					InArgs._Content.Widget
				]
			]
		]
	];
}

#undef LOCTEXT_NAMESPACE
