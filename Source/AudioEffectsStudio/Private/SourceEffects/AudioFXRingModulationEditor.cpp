// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

/**
 * AudioFXRingModulationEditor.cpp
 *
 * Custom Slate skin for the Ring Modulation source effect.
 * Layout: [Depth + Shape below] | [Frequency hero center] | [Wet + Dry stacked right]
 */

#include "SourceEffects/AudioFXRingModulationEditor.h"
#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Widgets/SAudioFXDropdown.h"
#include "Shared/Widgets/SAudioFXLabeledKnob.h"

#include "SourceEffects/SourceEffectRingModulation.h"
#include "AudioWidgetsEnums.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AudioFXRingModulation"

TSharedRef<SWidget> FAudioFXRingModulationEditor::CreateEffectWidget(USoundEffectPreset* InPreset)
{
	USourceEffectRingModulationPreset* Preset = Cast<USourceEffectRingModulationPreset>(InPreset);
	if (!Preset)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("AudioEffectsStudio: Cast to preset type failed for '%s'. Check asset registration."),
			InPreset ? *InPreset->GetClass()->GetName() : TEXT("null"));
		return FAudioFXEffectEditor::CreateEffectWidget(InPreset);
	}

	const FLinearColor AssetCol = GetAssetColor();
	TWeakObjectPtr<USourceEffectRingModulationPreset> WeakPreset = Preset;

	const FAudioMaterialKnobStyle* FreqStyle  = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Frequency, AssetCol, EAudioFXKnobSize::Hero));
	const FAudioMaterialKnobStyle* DepthStyle = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Mix,       AssetCol, EAudioFXKnobSize::Primary));
	const FAudioMaterialKnobStyle* DryStyle   = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain,      AssetCol, EAudioFXKnobSize::Tertiary));
	const FAudioMaterialKnobStyle* WetStyle   = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain,      AssetCol, EAudioFXKnobSize::Tertiary));

	TArray<FText> ModTypeOpts = {
		LOCTEXT("Sine",   "Sine"),
		LOCTEXT("Saw",    "Saw"),
		LOCTEXT("Tri",    "Triangle"),
		LOCTEXT("Square", "Square")
	};

	// Pre-declare Value attributes to avoid MSVC Slate parse ambiguity with -> float lambdas
	TAttribute<float> DepthAttr = TAttribute<float>::Create(
		TAttribute<float>::FGetter::CreateLambda([WeakPreset]() -> float
		{
			if (auto* EffectPreset = WeakPreset.Get()) { return EffectPreset->Settings.Depth; }
			return 0.5f;
		}));
	TAttribute<float> FreqAttr = TAttribute<float>::Create(
		TAttribute<float>::FGetter::CreateLambda([WeakPreset]() -> float
		{
			if (auto* EffectPreset = WeakPreset.Get())
			{
				return FAudioFXTheme::LogFreqToNorm(
					FMath::Clamp(EffectPreset->Settings.Frequency, 10.f, 5000.f), 10.f, 5000.f);
			}
			return FAudioFXTheme::LogFreqToNorm(100.f, 10.f, 5000.f);
		}));
	TAttribute<float> WetAttr = TAttribute<float>::Create(
		TAttribute<float>::FGetter::CreateLambda([WeakPreset]() -> float
		{
			if (auto* EffectPreset = WeakPreset.Get()) { return EffectPreset->Settings.WetLevel; }
			return 0.5f;
		}));
	TAttribute<float> DryAttr = TAttribute<float>::Create(
		TAttribute<float>::FGetter::CreateLambda([WeakPreset]() -> float
		{
			if (auto* EffectPreset = WeakPreset.Get()) { return EffectPreset->Settings.DryLevel; }
			return 1.f;
		}));


	return SNew(SBox)
		.Padding(FMargin(FAudioFXTheme::WindowPadding, FAudioFXTheme::SectionPadding))
	[
		SNew(SHorizontalBox)

		// ── LEFT: Depth knob with Shape dropdown pinned to bottom ─────────────
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Fill)
		.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
		[
			SNew(SVerticalBox)

			// Spacer pushes content to the bottom of the column
			+ SVerticalBox::Slot()
			.FillHeight(1.f)

			// Depth knob
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(SAudioFXLabeledKnob)
					.ParamName(LOCTEXT("DepthLbl", "Depth"))
					.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
						LOCTEXT("DepthTipLbl", "Modulation Depth"),
						0.f, 1.f, TEXT(""),
						LOCTEXT("DepthTip",
							"Ring modulation depth.\n"
							"AudioFactory Note: The wet output is scaled by WetLevel x Depth; "
							"at 0 the modulator has no effect on the wet signal.")))
					.Value(DepthAttr)
					.Style(DepthStyle)
					.AudioUnitsValueType(EAudioUnitsValueType::Linear)
					.ValueFormat(EAudioFXValueFormat::Percentage)
					.DefaultValue(0.5f).MinValue(0.f).MaxValue(1.f)
					.OnValueChanged_Lambda([this, WeakPreset](float NewNorm)
					{
						if (auto* EffectPreset = WeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							EffectPreset->Settings.Depth = NewNorm;
							EffectPreset->SetSettings(EffectPreset->Settings);
						}
					})
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.f, 8.f, 0.f, 0.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)
				[
					SNew(STextBlock)
						.Text(LOCTEXT("ShapeL", "Shape:"))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
						.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).MinDesiredWidth(90.f)
					[
						SNew(SAudioFXDropdown)
							.Options(ModTypeOpts)
							.AssetColor(AssetCol)
							.SelectedIndex((int32)Preset->Settings.ModulatorType)
							.OnSelectionChanged_Lambda([this, WeakPreset](int32 OptionIndex)
							{
								if (auto* EffectPreset = WeakPreset.Get())
								{
									MarkDirty.ExecuteIfBound();
									EffectPreset->Settings.ModulatorType = (ERingModulatorTypeSourceEffect)OptionIndex;
									EffectPreset->SetSettings(EffectPreset->Settings);
								}
							})
					]
				]
			]
		]

		// ── CENTER: Frequency hero knob ────────────────────────────
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
		[
			SNew(SAudioFXLabeledKnob)
				.ParamName(LOCTEXT("FreqLbl", "Frequency"))
				.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
					LOCTEXT("FreqTipLbl", "Modulation Frequency"),
					10.f, 5000.f, TEXT("Hz"),
					LOCTEXT("FreqTip",
						"Ring modulation frequency.\n"
						"AudioFactory Note: Low frequencies produce tremolo, "
						"higher values create metallic inharmonic tones.")))
				.Value(FreqAttr)
				.Style(FreqStyle)
				.AudioUnitsValueType(EAudioUnitsValueType::Frequency)
				.ValueFormat(EAudioFXValueFormat::Frequency)
				.DefaultValue(FAudioFXTheme::LogFreqToNorm(100.f, 10.f, 5000.f))
				.MinValue(10.f).MaxValue(5000.f)
				.OnValueChanged_Lambda([this, WeakPreset](float NewNorm)
				{
					if (auto* EffectPreset = WeakPreset.Get())
					{
						MarkDirty.ExecuteIfBound();
						EffectPreset->Settings.Frequency = FAudioFXTheme::NormToLogFreq(NewNorm, 10.f, 5000.f);
						EffectPreset->SetSettings(EffectPreset->Settings);
					}
				})
		]

		// ── RIGHT: Wet + Dry stacked ───────────────────────────────
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 4.f)
			[
				SNew(SAudioFXLabeledKnob)
					.ParamName(LOCTEXT("WetLbl", "Wet"))
					.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
						LOCTEXT("WetTipLbl", "Wet Level"), 0.f, 1.f, TEXT(""),
						LOCTEXT("WetTip", "Gain for the wet signal (with ring mod).")))
					.Value(WetAttr)
					.Style(WetStyle)
					.AudioUnitsValueType(EAudioUnitsValueType::Linear)
					.ValueFormat(EAudioFXValueFormat::Percentage)
					.DefaultValue(0.5f).MinValue(0.f).MaxValue(1.f)
					.OnValueChanged_Lambda([this, WeakPreset](float NewNorm)
					{
						if (auto* EffectPreset = WeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							EffectPreset->Settings.WetLevel = NewNorm;
							EffectPreset->SetSettings(EffectPreset->Settings);
						}
					})
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(SAudioFXLabeledKnob)
					.ParamName(LOCTEXT("DryLbl", "Dry"))
					.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
						LOCTEXT("DryTipLbl", "Dry Level"), 0.f, 1.f, TEXT(""),
						LOCTEXT("DryTip", "Gain for the dry signal (no ring mod).")))
					.Value(DryAttr)
					.Style(DryStyle)
					.AudioUnitsValueType(EAudioUnitsValueType::Linear)
					.ValueFormat(EAudioFXValueFormat::Percentage)
					.DefaultValue(1.f).MinValue(0.f).MaxValue(1.f)
					.OnValueChanged_Lambda([this, WeakPreset](float NewNorm)
					{
						if (auto* EffectPreset = WeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							EffectPreset->Settings.DryLevel = NewNorm;
							EffectPreset->SetSettings(EffectPreset->Settings);
						}
					})
			]
		]
	];
}

#undef LOCTEXT_NAMESPACE
