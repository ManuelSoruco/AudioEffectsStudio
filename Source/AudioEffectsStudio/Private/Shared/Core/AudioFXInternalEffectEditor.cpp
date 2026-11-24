// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Core/AudioFXInternalEffectEditor.h"
#include "Shared/Core/AudioFXTheme.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AudioFXInternalEffect"

// ============================================================
// Effect Widget
// ============================================================

TSharedRef<SWidget> FAudioFXInternalEffectEditor::CreateEffectWidget(USoundEffectPreset* InPreset)
{
	// ── Static brushes (created once, reused for every window open) ──────────
	// Rounded card brush for the outer message panel
	static FSlateRoundedBoxBrush CardBrush(
		FAudioFXTheme::GetSectionBackground(),
		8.0f);

	// Thin accent bar brush at the top of the card (warn color = amber/orange)
	static FSlateRoundedBoxBrush AccentBrush(
		FAudioFXTheme::GetAccentWarn(),
		4.0f);

	// ── Root: centered, padded container ─────────────────────────────────────
	return SNew(SBox)
		.Padding(FMargin(FAudioFXTheme::WindowPadding * 2.f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			// ── Outer card — max 400 px wide, rounded corners ─────────────────
			SNew(SBox)
			.MaxDesiredWidth(400.f)
			[
				SNew(SBorder)
				.BorderImage(&CardBrush)
				.Padding(FMargin(0.f))
				[
					SNew(SVerticalBox)

					// ── Top accent bar (4 px warn-color stripe) ───────────────
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SBox)
						.HeightOverride(4.f)
						[
							SNew(SBorder)
							.BorderImage(&AccentBrush)
						]
					]

					// ── Card body (icon + title + divider + message + footer) ─
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(24.f, 20.f, 24.f, 24.f)
					[
						SNew(SVerticalBox)

						// ── Icon + title row ──────────────────────────────────
						// ⚠ Unicode warning sign (U+26A0) as text block for icon-free setups
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.f, 0.f, 0.f, 12.f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(0.f, 0.f, 10.f, 0.f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(FString(TEXT("\u26A0"))))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
								.ColorAndOpacity(FAudioFXTheme::GetAccentWarn())
							]
							+ SHorizontalBox::Slot()
							.FillWidth(1.f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("Title", "Internal Effect"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
								.ColorAndOpacity(FAudioFXTheme::GetHeaderTextColor())
							]
						]

						// ── Horizontal divider (1 px) ─────────────────────────
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.f, 0.f, 0.f, 14.f)
						[
							SNew(SBox)
							.HeightOverride(1.f)
							[
								SNew(SBorder)
								.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
								.BorderBackgroundColor(FAudioFXTheme::GetDividerColor())
							]
						]

						// ── Explanatory body text (auto-wrapped) ──────────────
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.f, 0.f, 0.f, 16.f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("Body",
								"This effect is managed internally by Audio Effects Studio.\n\n"
								"It is created and removed automatically by the plugin's dynamics editors "
								"to capture gain-reduction data for metering. Do not add it to "
								"effect chains manually."))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
							.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
							.AutoWrapText(true)
						]

						// ── Footer hint (centered, italic, dim) ───────────────
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(LOCTEXT("Footer", "You can safely close this window."))
							.Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
							.ColorAndOpacity(FAudioFXTheme::GetTextDisabledColor())
							.Justification(ETextJustify::Center)
						]
					]
				]
			]
		];
}

#undef LOCTEXT_NAMESPACE
