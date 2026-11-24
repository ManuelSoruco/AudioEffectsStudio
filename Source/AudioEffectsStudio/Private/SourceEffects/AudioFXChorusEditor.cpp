// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

/**
 * FAudioFXChorusEditor
 *
 * Knob-based editor skin for USourceEffectChorusPreset.
 * Groups six parameters into MODULATION (Depth, Frequency, Feedback)
 * and MIX (Wet, Dry, Spread) section cards.
 */

#include "SourceEffects/AudioFXChorusEditor.h"
#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Widgets/SAudioFXSectionBox.h"

#include "SourceEffects/SourceEffectChorus.h"
#include "Shared/Widgets/SAudioFXLabeledKnob.h"
#include "AudioWidgetsEnums.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AudioFXChorus"

TSharedRef<SWidget> FAudioFXChorusEditor::CreateEffectWidget(USoundEffectPreset* InPreset)
{
	USourceEffectChorusPreset* Preset = Cast<USourceEffectChorusPreset>(InPreset);
	if (!Preset)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("AudioEffectsStudio: Cast to preset type failed for '%s'. Check asset registration."),
			InPreset ? *InPreset->GetClass()->GetName() : TEXT("null"));
		return FAudioFXEffectEditor::CreateEffectWidget(InPreset);
	}

	const FLinearColor AssetCol = GetAssetColor();
	TWeakObjectPtr<USourceEffectChorusPreset> WeakPreset = Preset;

	// Store all 6 knob styles — Visual Redesign: Depth=Hero, Frequency=Primary, rest=Secondary/Tertiary
	const FAudioMaterialKnobStyle* DepthStyle    = StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Mix, AssetCol, EAudioFXKnobSize::Hero));
	const FAudioMaterialKnobStyle* FreqStyle     = StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Frequency, AssetCol, EAudioFXKnobSize::Hero));
	const FAudioMaterialKnobStyle* FeedbackStyle = StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Time, AssetCol, EAudioFXKnobSize::Secondary));
	const FAudioMaterialKnobStyle* WetStyle      = StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain, AssetCol, EAudioFXKnobSize::Secondary));
	const FAudioMaterialKnobStyle* DryStyle      = StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain, AssetCol, EAudioFXKnobSize::Secondary));
	const FAudioMaterialKnobStyle* SpreadStyle   = StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Mix, AssetCol, EAudioFXKnobSize::Primary));

	// Shared lambda to apply all base settings (uses ChorusBaseSettings path)
	auto ApplySettings = [WeakPreset]()
	{
		if (auto* Preset = WeakPreset.Get())
		{
			FSourceEffectChorusBaseSettings Base;
			Base.Depth     = Preset->Settings.DepthModulation.Value;
			Base.Frequency = Preset->Settings.FrequencyModulation.Value;
			Base.Feedback  = Preset->Settings.FeedbackModulation.Value;
			Base.WetLevel  = Preset->Settings.WetModulation.Value;
			Base.DryLevel  = Preset->Settings.DryModulation.Value;
			Base.Spread    = Preset->Settings.SpreadModulation.Value;
			Preset->SetSettings(Base);
		}
	};

	TSharedPtr<SAudioFXLabeledKnob> DepthKnob, FreqKnob, FeedbackKnob, WetKnob, DryKnob, SpreadKnob;
	TSharedRef<SWidget> Widget =
		SNew(SBox)
		.Padding(FMargin(FAudioFXTheme::WindowPadding, FAudioFXTheme::SectionPadding * 0.5f))
		[
			SNew(SHorizontalBox)

			// ── MODULATION section (Depth hero center, Frequency left, Feedback right) ──
			+ SHorizontalBox::Slot()
			.FillWidth(7.0f)
			.Padding(0.f, 0.f, FAudioFXTheme::SectionGap * 0.5f, 0.f)
			[
				SNew(SAudioFXSectionBox)
				.SectionLabel(LOCTEXT("ModGrp","MODULATION"))
				[
					SNew(SHorizontalBox)
					// ── Frequency (hero, left) ─────────────────────────
					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Bottom)
					.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
					[
						SAssignNew(FreqKnob, SAudioFXLabeledKnob)
						.ParamName(LOCTEXT("FreqLbl","Frequency"))
						.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
							LOCTEXT("FreqTipLbl","Frequency"), 0.1f, 20.f, TEXT("Hz"),
							LOCTEXT("FreqTip",
								"The frequency of the chorus LFO.\n"
								"AudioFactory Note: Uses a triangle-wave LFO "
								"internally; rates above ~3 Hz produce a "
								"vibrato-like effect.")))
						.Value_Lambda([WeakPreset]() -> float
						{
							if (auto* Preset = WeakPreset.Get())
							{
								return FMath::GetMappedRangeValueClamped(
									FVector2D(0.1f, 20.f), FVector2D(0.f, 1.f),
									Preset->Settings.FrequencyModulation.Value);
							}
							return 0.1f;
						})
						.Style(FreqStyle)
						.AudioUnitsValueType(EAudioUnitsValueType::Linear)
						.ValueFormat(EAudioFXValueFormat::Frequency)
						.MinValue(0.1f)
						.MaxValue(20.f)
						.OnValueChanged_Lambda([this, WeakPreset, ApplySettings](float NewNorm)
						{
							if (auto* EffectPreset = WeakPreset.Get())
							{
								MarkDirty.ExecuteIfBound();
								EffectPreset->Settings.FrequencyModulation.Value =
									FMath::GetMappedRangeValueClamped(
										FVector2D(0.f, 1.f),
										FVector2D(0.1f, 20.f), NewNorm);
								ApplySettings();
							}
						})
					]
					// ── Feedback (secondary, center) ───────────────────
					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Bottom)
					.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
					[
						SAssignNew(FeedbackKnob, SAudioFXLabeledKnob)
						.ParamName(LOCTEXT("FbLbl","Feedback"))
						.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
							LOCTEXT("FbTipLbl","Feedback"), 0.f, 1.f, TEXT(""),
							LOCTEXT("FbTip",
								"The feedback of the chorus effect.\n"
								"AudioFactory Note: Feedback is fed back into "
								"each delay tap individually; high values add "
								"a flanging character.")))
						.Value_Lambda([WeakPreset]() -> float
						{
							if (auto* Preset = WeakPreset.Get())
							{
								return Preset->Settings.FeedbackModulation.Value;
							}
							return 0.5f;
						})
						.Style(FeedbackStyle)
						.AudioUnitsValueType(EAudioUnitsValueType::Linear)
						.OnValueChanged_Lambda([this, WeakPreset, ApplySettings](float NewNorm)
						{
							if (auto* EffectPreset = WeakPreset.Get())
							{
								MarkDirty.ExecuteIfBound();
								EffectPreset->Settings.FeedbackModulation.Value = NewNorm;
								ApplySettings();
							}
						})
					]
					// ── Depth (hero, right) ────────────────────────────
					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Bottom)
					.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
					[
						SAssignNew(DepthKnob, SAudioFXLabeledKnob)
						.ParamName(LOCTEXT("DepthLbl","Depth"))
						.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
							LOCTEXT("DepthTipLbl","Depth"), 0.f, 1.f, TEXT(""),
							LOCTEXT("DepthTip",
								"The depth of the chorus effect.\n"
								"AudioFactory Note: Depth modulates the delay "
								"time (5–50 ms range); higher values "
								"produce a wider pitch wobble.")))
						.Value_Lambda([WeakPreset]() -> float
						{
							if (auto* Preset = WeakPreset.Get())
							{
								return Preset->Settings.DepthModulation.Value;
							}
							return 0.5f;
						})
						.Style(DepthStyle)
						.AudioUnitsValueType(EAudioUnitsValueType::Linear)
						.OnValueChanged_Lambda([this, WeakPreset, ApplySettings](float NewNorm)
						{
							if (auto* EffectPreset = WeakPreset.Get())
							{
								MarkDirty.ExecuteIfBound();
								EffectPreset->Settings.DepthModulation.Value = NewNorm;
								ApplySettings();
							}
						})
					]
				]
			]

			// ── MIX section: Spread centered, Wet/Dry stacked right ──
			+ SHorizontalBox::Slot()
			.FillWidth(3.0f)
			.Padding(FAudioFXTheme::SectionGap * 0.5f, 0.f, 0.f, 0.f)
			[
				SNew(SAudioFXSectionBox)
				.SectionLabel(LOCTEXT("MixGrp","MIX"))
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
					[
						SAssignNew(SpreadKnob, SAudioFXLabeledKnob)
						.ParamName(LOCTEXT("SpreadLbl","Spread"))
						.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
							LOCTEXT("SpreadTipLbl","Spread"), 0.f, 1.f, TEXT(""),
							LOCTEXT("SpreadTip",
								"The spread of the effect (larger means greater "
								"difference between left and right delay "
								"lines).\n"
								"AudioFactory Note: Spread offsets the L/R LFO "
								"frequencies by up to ±10 Hz, widening "
								"the stereo image.")))
						.Value_Lambda([WeakPreset]() -> float
						{
							if (auto* Preset = WeakPreset.Get())
							{
								return Preset->Settings.SpreadModulation.Value;
							}
						 return 0.5f;
						})
						.Style(SpreadStyle)
						.AudioUnitsValueType(EAudioUnitsValueType::Linear)
						.OnValueChanged_Lambda([this, WeakPreset, ApplySettings](float NewNorm)
						{
							if (auto* EffectPreset = WeakPreset.Get())
							{
								MarkDirty.ExecuteIfBound();
								EffectPreset->Settings.SpreadModulation.Value = NewNorm;
								ApplySettings();
							}
						})
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.Padding(0.f, 0.f, 0.f, 4.f)
						[
							SAssignNew(WetKnob, SAudioFXLabeledKnob)
							.ParamName(LOCTEXT("WetLbl","Wet"))
							.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
								LOCTEXT("WetTipLbl","Wet Level"), 0.f, 1.f, TEXT(""),
								LOCTEXT("WetTip","The wet level of the chorus effect.")))
							.Value_Lambda([WeakPreset]() -> float
							{
								if (auto* Preset = WeakPreset.Get())
								{
									return Preset->Settings.WetModulation.Value;
								}
								return 0.5f;
							})
							.Style(WetStyle)
							.AudioUnitsValueType(EAudioUnitsValueType::Linear)
							.OnValueChanged_Lambda([this, WeakPreset, ApplySettings](float NewNorm)
							{
								if (auto* EffectPreset = WeakPreset.Get())
								{
									MarkDirty.ExecuteIfBound();
									EffectPreset->Settings.WetModulation.Value = NewNorm;
									ApplySettings();
								}
							})
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.Padding(0.f, 4.f, 0.f, 0.f)
						[
							SAssignNew(DryKnob, SAudioFXLabeledKnob)
							.ParamName(LOCTEXT("DryLbl","Dry"))
							.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
								LOCTEXT("DryTipLbl","Dry Level"), 0.f, 1.f, TEXT(""),
								LOCTEXT("DryTip","The dry level of the chorus effect.")))
							.Value_Lambda([WeakPreset]() -> float
							{
								if (auto* Preset = WeakPreset.Get())
								{
									return Preset->Settings.DryModulation.Value;
								}
								return 0.5f;
							})
							.Style(DryStyle)
							.AudioUnitsValueType(EAudioUnitsValueType::Linear)
							.OnValueChanged_Lambda([this, WeakPreset, ApplySettings](float NewNorm)
							{
								if (auto* EffectPreset = WeakPreset.Get())
								{
									MarkDirty.ExecuteIfBound();
									EffectPreset->Settings.DryModulation.Value = NewNorm;
									ApplySettings();
								}
							})
						]
					]
				]
			]
		];

	return Widget;
}

#undef LOCTEXT_NAMESPACE
