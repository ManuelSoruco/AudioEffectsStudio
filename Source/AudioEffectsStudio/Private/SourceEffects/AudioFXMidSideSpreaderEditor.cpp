// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

/**
 * FAudioFXMidSideSpreaderEditor
 *
 * Custom Slate editor skin for the Mid/Side Spreader source effect.
 * Layout (WaveShaper-style): square Mid/Side visualization on the left,
 * Spread hero knob + Input/Output dropdowns + Equal Power toggle on the right.
 */

#include "SourceEffects/AudioFXMidSideSpreaderEditor.h"
#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Visualizers/SAudioFXMidSideDisplay.h"
#include "Shared/Widgets/SAudioFXDropdown.h"
#include "Shared/Widgets/SAudioFXButton.h"
#include "Shared/Widgets/SAudioFXLabeledKnob.h"

#include "SourceEffects/SourceEffectMidSideSpreader.h"
#include "AudioWidgetsEnums.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AudioFXMidSideSpreader"

TSharedRef<SWidget> FAudioFXMidSideSpreaderEditor::CreateEffectWidget(USoundEffectPreset* InPreset)
{
	USourceEffectMidSideSpreaderPreset* Preset = Cast<USourceEffectMidSideSpreaderPreset>(InPreset);
	if (!Preset)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("AudioEffectsStudio: Cast to preset type failed for '%s'. Check asset registration."),
			InPreset ? *InPreset->GetClass()->GetName() : TEXT("null"));
		return FAudioFXEffectEditor::CreateEffectWidget(InPreset);
	}

	const FLinearColor AssetCol = GetAssetColor();
	TWeakObjectPtr<USourceEffectMidSideSpreaderPreset> WeakPreset = Preset;

	const FAudioMaterialKnobStyle* SpreadStyle = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Mix, AssetCol, EAudioFXKnobSize::Hero));

	TArray<FText> ModeOptions = { LOCTEXT("MidSide", "Mid/Side"), LOCTEXT("LeftRight", "L/R") };

	return SNew(SBox)
		.Padding(FMargin(FAudioFXTheme::WindowPadding, FAudioFXTheme::SectionPadding))
	[
		SNew(SHorizontalBox)

		// ── LEFT: Square Mid/Side visualization ──────────────────
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.f, 0.f, FAudioFXTheme::WindowPadding, 0.f)
		[
			SNew(SBox)
			.WidthOverride(200.f)
			.HeightOverride(200.f)
			[
				SNew(SOverlay)

				+ SOverlay::Slot()
				[
					SNew(SAudioFXMidSideDisplay)
					.MidLevel_Lambda([WeakPreset]() -> float
					{
						if (const auto* P = WeakPreset.Get())
							return 1.f - FMath::Clamp(P->Settings.SpreadAmount, 0.f, 1.f);
						return 0.5f;
					})
					.SideLevel_Lambda([WeakPreset]() -> float
					{
						if (const auto* P = WeakPreset.Get())
							return FMath::Clamp(P->Settings.SpreadAmount, 0.f, 1.f);
						return 0.5f;
					})
					.AssetColor(AssetCol)
				]

				// "M" label top-left
				+ SOverlay::Slot()
				.HAlign(HAlign_Left).VAlign(VAlign_Top)
				.Padding(4.f, 3.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("MLbl", "M"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
					.ColorAndOpacity(FAudioFXTheme::GetTextDisabledColor())
				]

				// "S" label bottom-right
				+ SOverlay::Slot()
				.HAlign(HAlign_Right).VAlign(VAlign_Bottom)
				.Padding(4.f, 3.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SLbl", "S"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
					.ColorAndOpacity(FAudioFXTheme::GetTextDisabledColor())
				]
			]
		]

		// ── RIGHT: Spread hero knob + controls ───────────────────
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)

			// Spread hero knob — centered
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.f, 0.f, 0.f, FAudioFXTheme::SectionGap)
			[
				SNew(SAudioFXLabeledKnob)
				.ParamName(LOCTEXT("SpreadLbl", "Spread"))
				.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
					LOCTEXT("SpreadTipName", "Spread"), 0.f, 1.f, TEXT(""),
					LOCTEXT("SpreadTip",
						"Amount of Mid/Side Spread. 0.0 is no spread, 1.0 is full wide.\n"
						"AudioFactory Note: At 0.0, only the mid (mono) signal is heard. At 1.0, only the side (stereo difference) signal remains.")))
				.Value_Lambda([WeakPreset]() -> float
				{
					if (auto* P = WeakPreset.Get()) return P->Settings.SpreadAmount;
					return 0.5f;
				})
				.Style(SpreadStyle)
				.AudioUnitsValueType(EAudioUnitsValueType::Linear)
				.ValueFormat(EAudioFXValueFormat::Percentage)
				.DefaultValue(0.5f)
				.MinValue(0.f)
				.MaxValue(1.f)
				.OnValueChanged_Lambda([this, WeakPreset](float NewNorm)
				{
					if (auto* P = WeakPreset.Get())
					{
						MarkDirty.ExecuteIfBound();
						P->Settings.SpreadAmount = FMath::Clamp(NewNorm, 0.f, 1.f);
						P->SetSettings(P->Settings);
					}
				})
			]

			// Input/Output dropdowns
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
				[
					SNew(SBox).MinDesiredWidth(44.f)
					[
						SNew(STextBlock).Text(LOCTEXT("InputMode", "Input"))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
						.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
					]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SAudioFXDropdown)
					.Options(ModeOptions).AssetColor(AssetCol)
					.SelectedIndex((int32)Preset->Settings.InputMode)
					.OnSelectionChanged_Lambda([this, WeakPreset](int32 OptionIndex)
					{
						if (auto* P = WeakPreset.Get()) { MarkDirty.ExecuteIfBound(); P->Settings.InputMode = (EStereoChannelMode)OptionIndex; P->SetSettings(P->Settings); }
					})
				]
			]

			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
				[
					SNew(SBox).MinDesiredWidth(44.f)
					[
						SNew(STextBlock).Text(LOCTEXT("OutputMode", "Output"))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
						.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
					]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SAudioFXDropdown)
					.Options(ModeOptions).AssetColor(AssetCol)
					.SelectedIndex((int32)Preset->Settings.OutputMode)
					.OnSelectionChanged_Lambda([this, WeakPreset](int32 OptionIndex)
					{
						if (auto* P = WeakPreset.Get()) { MarkDirty.ExecuteIfBound(); P->Settings.OutputMode = (EStereoChannelMode)OptionIndex; P->SetSettings(P->Settings); }
					})
				]
			]

			// Equal Power toggle
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left)
			[
				SNew(SAudioFXButton)
				.Options(TArray<FText>{ LOCTEXT("EqualPower", "Equal Power") })
				.AssetColor(AssetCol)
				.SelectedIndex(Preset->Settings.bEqualPower ? 0 : -1)
				.OnSelectionChanged_Lambda([this, WeakPreset](int32 OptionIndex)
				{
					if (auto* P = WeakPreset.Get()) { MarkDirty.ExecuteIfBound(); P->Settings.bEqualPower = (OptionIndex == 0); P->SetSettings(P->Settings); }
				})
			]
		]
	];
}

#undef LOCTEXT_NAMESPACE
