// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/UIBuilders/AudioFXStereoDelayUI.h"
#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Widgets/SAudioFXDropdown.h"
#include "Shared/Widgets/SAudioFXButton.h"
#include "Shared/Widgets/SAudioFXLabeledKnob.h"
#include "Shared/Visualizers/SAudioFXReverbDecayCurve.h"
#include "SourceEffects/SourceEffectStereoDelay.h"
#include "SubmixEffects/SubmixEffectStereoDelay.h"
#include "AudioWidgetsEnums.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AudioFXStereoDelayUI"

namespace
{
// ============================================================
// BuildWidgetImpl — shared template for Source + Submix presets
// ============================================================

template<typename PresetType>
TSharedRef<SWidget> BuildWidgetImpl(
	FAudioFXEffectEditor& Editor,
	PresetType* Preset,
	const FLinearColor& AssetCol)
{
	if (!Preset) { return SNullWidget::NullWidget; }

	TWeakObjectPtr<PresetType> WeakPreset = Preset;
	TWeakPtr<FAudioFXEffectEditor> WeakEditor = Editor.AsShared();
	FSimpleDelegate MarkDirty = FSimpleDelegate::CreateLambda([WeakEditor]{ if (auto Ed = WeakEditor.Pin()) { Ed->MarkPresetDirty(); } });

	// Knob color roles: Time=Blue Hero, Feedback=Purple Primary, Ratio=Green Secondary, Wet/Dry=Orange
	const FAudioMaterialKnobStyle* TimeStyle     = Editor.StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Frequency, AssetCol, EAudioFXKnobSize::Hero));
	const FAudioMaterialKnobStyle* FbStyle       = Editor.StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Time,      AssetCol, EAudioFXKnobSize::Primary));
	const FAudioMaterialKnobStyle* RatioStyle    = Editor.StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Mix,       AssetCol, EAudioFXKnobSize::Secondary));
	const FAudioMaterialKnobStyle* WetStyle      = Editor.StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain,      AssetCol, EAudioFXKnobSize::Secondary));
	const FAudioMaterialKnobStyle* DryStyle      = Editor.StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain,      AssetCol, EAudioFXKnobSize::Secondary));
	const FAudioMaterialKnobStyle* FiltFreqStyle = Editor.StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Frequency, AssetCol, EAudioFXKnobSize::Secondary));
	const FAudioMaterialKnobStyle* FiltQStyle    = Editor.StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Resonance, AssetCol, EAudioFXKnobSize::Tertiary));

	// Logarithmic frequency helpers for the filter knob
	auto FiltFreqToNorm = [](float Hz)
	{
		return (FMath::Log2(FMath::Max(Hz, 20.f)) - FMath::Log2(20.f))
		     / (FMath::Log2(20000.f) - FMath::Log2(20.f));
	};
	auto NormToFiltFreq = [](float NewNorm)
	{
		return FMath::Pow(2.f, FMath::Log2(20.f) + NewNorm * (FMath::Log2(20000.f) - FMath::Log2(20.f)));
	};

	TArray<FText> ModeOpts     = { LOCTEXT("Normal","Normal"), LOCTEXT("Cross","Cross"), LOCTEXT("PingPong","PingPong") };
	TArray<FText> FiltTypeOpts = { LOCTEXT("LP","Low-Pass"), LOCTEXT("HP","High-Pass"), LOCTEXT("BP","Band-Pass"), LOCTEXT("Notch","Notch") };

	TSharedPtr<SAudioFXLabeledKnob> TimeKnob, FbKnob, RatioKnob, WetKnob, DryKnob, FiltFreqKnob, FiltQKnob;

	TSharedRef<SWidget> Widget =
		SNew(SBox)
		.Padding(FMargin(FAudioFXTheme::WindowPadding, FAudioFXTheme::SectionPadding))
		[
			SNew(SVerticalBox)

			// ── Graph visualization (TOP) ─────────────────────────────
			+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(SBox).MinDesiredHeight(200.f)
				[
					SNew(SAudioFXReverbDecayCurve)
					.bDelayMode(true)
					.DecayTime_Lambda([WeakPreset]()
					{
						if (const auto* DelayPreset = WeakPreset.Get()) { return DelayPreset->Settings.DelayTimeMsec / 1000.f; }
						return 0.5f;
					})
					.DelayFeedback_Lambda([WeakPreset]()
					{
						if (const auto* DelayPreset = WeakPreset.Get()) { return DelayPreset->Settings.Feedback; }
						return 0.5f;
					})
					.AssetColor(AssetCol)
				]
			]

			// ── Knob row: [Ratio] [Time HERO centered] [Feedback] [Wet+Dry] ─
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(SHorizontalBox)

				// Delay Ratio (left of hero)
				+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
				.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
				[
					SAssignNew(RatioKnob, SAudioFXLabeledKnob)
					.ParamName(LOCTEXT("RatioLbl", "Delay Ratio"))
					.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
						LOCTEXT("RatioTipName", "Delay Ratio"), -1.f, 1.f, TEXT(""),
						LOCTEXT("RatioTip",
							"Delay spread for left and right channels.\n"
							"AudioFactory Note: Offsets L/R delay times from each other.")))
					.Value_Lambda([WeakPreset]() -> float
					{
						if (const auto* DelayPreset = WeakPreset.Get())
							return FMath::GetMappedRangeValueClamped(FVector2D(-1.f, 1.f), FVector2D(0.f, 1.f), DelayPreset->Settings.DelayRatio);
						return 0.5f;
					})
					.Style(RatioStyle).AudioUnitsValueType(EAudioUnitsValueType::Linear)
					.DefaultValue(0.5f).MinValue(-1.f).MaxValue(1.f)
					.OnValueChanged_Lambda([WeakPreset, MarkDirty, Preset](float NewNorm)
					{
						if (auto* DelayPreset = WeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							DelayPreset->Settings.DelayRatio = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 1.f), FVector2D(-1.f, 1.f), NewNorm);
							Preset->SetSettings(Preset->Settings);
						}
					})
				]

				// Delay Time (Hero) — PERFECTLY CENTERED (equal FillWidth on both sides)
				+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
				.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
				[
					SAssignNew(TimeKnob, SAudioFXLabeledKnob)
					.ParamName(LOCTEXT("TimeLbl", "Delay Time"))
					.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
						LOCTEXT("TimeTipName", "Delay Time"), 0.f, 2000.f, TEXT("ms"),
						LOCTEXT("TimeTip",
							"The base amount of delay in the left and right channels.\n"
							"AudioFactory Note: In PingPong mode, the delay alternates between channels.")))
					.Value_Lambda([WeakPreset]() -> float
					{
						if (const auto* DelayPreset = WeakPreset.Get())
							return FMath::GetMappedRangeValueClamped(FVector2D(0.f, 2000.f), FVector2D(0.f, 1.f), DelayPreset->Settings.DelayTimeMsec);
						return 0.25f;
					})
					.Style(TimeStyle).AudioUnitsValueType(EAudioUnitsValueType::Linear)
					.ValueFormat(EAudioFXValueFormat::Time)
					.DefaultValue(FMath::GetMappedRangeValueClamped(FVector2D(0.f, 2000.f), FVector2D(0.f, 1.f), 500.f))
					.MinValue(0.f).MaxValue(2000.f)
					.OnValueChanged_Lambda([WeakPreset, MarkDirty, Preset](float NewNorm)
					{
						if (auto* DelayPreset = WeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							DelayPreset->Settings.DelayTimeMsec = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 1.f), FVector2D(0.f, 2000.f), NewNorm);
							Preset->SetSettings(Preset->Settings);
						}
					})
				]

				// Feedback (right of hero)
				+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
				.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
				[
					SAssignNew(FbKnob, SAudioFXLabeledKnob)
					.ParamName(LOCTEXT("FbLbl", "Feedback"))
					.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
						LOCTEXT("FbTipName", "Feedback"), 0.f, 1.f, TEXT(""),
						LOCTEXT("FbTip", "The amount of audio to feed back into the delay line once the delay has been tapped.")))
					.Value_Lambda([WeakPreset]() -> float
					{
						if (const auto* DelayPreset = WeakPreset.Get()) { return DelayPreset->Settings.Feedback; }
						return 0.5f;
					})
					.Style(FbStyle).AudioUnitsValueType(EAudioUnitsValueType::Linear)
					.DefaultValue(0.5f).MinValue(0.f).MaxValue(1.f)
					.OnValueChanged_Lambda([WeakPreset, MarkDirty, Preset](float NewNorm)
					{
						if (auto* DelayPreset = WeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							DelayPreset->Settings.Feedback = NewNorm;
							Preset->SetSettings(Preset->Settings);
						}
					})
				]

				// Dry + Wet stacked in one column
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 4.f)
					[
						SAssignNew(DryKnob, SAudioFXLabeledKnob)
						.ParamName(LOCTEXT("DryLbl", "Dry Level"))
						.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
							LOCTEXT("DryTipName", "Dry Level"), 0.f, 1.f, TEXT(""),
							LOCTEXT("DryTip", "The dry level of the effect.")))
						.Value_Lambda([WeakPreset]() -> float
						{
							if (const auto* DelayPreset = WeakPreset.Get()) { return DelayPreset->Settings.DryLevel; }
							return 1.f;
						})
						.Style(DryStyle).AudioUnitsValueType(EAudioUnitsValueType::Linear)
						.DefaultValue(1.f).MinValue(0.f).MaxValue(1.f)
						.OnValueChanged_Lambda([WeakPreset, MarkDirty, Preset](float NewNorm)
						{
							if (auto* DelayPreset = WeakPreset.Get())
							{
								MarkDirty.ExecuteIfBound();
								DelayPreset->Settings.DryLevel = NewNorm;
								Preset->SetSettings(Preset->Settings);
							}
						})
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SAssignNew(WetKnob, SAudioFXLabeledKnob)
						.ParamName(LOCTEXT("WetLbl", "Wet Level"))
						.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
							LOCTEXT("WetTipName", "Wet Level"), 0.f, 1.f, TEXT(""),
							LOCTEXT("WetTip", "The wet level of the delay effect.")))
						.Value_Lambda([WeakPreset]() -> float
						{
							if (const auto* DelayPreset = WeakPreset.Get()) { return DelayPreset->Settings.WetLevel; }
							return 0.5f;
						})
						.Style(WetStyle).AudioUnitsValueType(EAudioUnitsValueType::Linear)
						.DefaultValue(0.5f).MinValue(0.f).MaxValue(1.f)
						.OnValueChanged_Lambda([WeakPreset, MarkDirty, Preset](float NewNorm)
						{
							if (auto* DelayPreset = WeakPreset.Get())
							{
								MarkDirty.ExecuteIfBound();
								DelayPreset->Settings.WetLevel = NewNorm;
								Preset->SetSettings(Preset->Settings);
							}
						})
					]
				]
			]

			// ── Filter card: Filter toggle | Type | Freq | Q | Mode ──
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(&FAudioFXTheme::GetCardBrush())
				.Padding(FMargin(8.f, 6.f))
				.HAlign(HAlign_Center)
				[
				SNew(SHorizontalBox)

				// Filter on/off toggle
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 8.f, 0.f).VAlign(VAlign_Center)
				[
					SNew(SAudioFXButton)
					.Options(TArray<FText>{ LOCTEXT("FiltOn", "Filter") })
					.AssetColor(AssetCol)
					.SelectedIndex(Preset->Settings.bFilterEnabled ? 0 : -1)
					.OnSelectionChanged_Lambda([WeakPreset, MarkDirty, Preset](int32 OptionIndex)
					{
						if (auto* DelayPreset = WeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							DelayPreset->Settings.bFilterEnabled = (OptionIndex == 0);
							Preset->SetSettings(Preset->Settings);
						}
					})
				]

				// Filter type label + dropdown
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("FTypeL", "Type:"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 12.f, 0.f)
				[
					SNew(SAudioFXDropdown)
					.Options(FiltTypeOpts).AssetColor(AssetCol)
					.SelectedIndex((int32)Preset->Settings.FilterType)
					.OnSelectionChanged_Lambda([WeakPreset, MarkDirty, Preset](int32 OptionIndex)
					{
						if (auto* DelayPreset = WeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							DelayPreset->Settings.FilterType = (EStereoDelayFiltertype)OptionIndex;
							Preset->SetSettings(Preset->Settings);
						}
					})
				]

				// Filter frequency knob
				+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Center).VAlign(VAlign_Center)
				.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
				[
					SAssignNew(FiltFreqKnob, SAudioFXLabeledKnob)
					.ParamName(LOCTEXT("FiltFreqLbl", "Frequency"))
					.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
						LOCTEXT("FiltFreqTipName", "Frequency"), 20.f, 20000.f, TEXT("Hz"),
						LOCTEXT("FiltFreqTip", "Cutoff frequency of the feedback filter.")))
					.Value_Lambda([WeakPreset, FiltFreqToNorm]() -> float
					{
						if (const auto* DelayPreset = WeakPreset.Get()) { return FiltFreqToNorm(DelayPreset->Settings.FilterFrequency); }
						return FiltFreqToNorm(1000.f);
					})
					.Style(FiltFreqStyle).AudioUnitsValueType(EAudioUnitsValueType::Frequency)
					.ValueFormat(EAudioFXValueFormat::Frequency)
					.DefaultValue(FiltFreqToNorm(1000.f)).MinValue(20.f).MaxValue(20000.f)
					.OnValueChanged_Lambda([WeakPreset, MarkDirty, NormToFiltFreq, Preset](float NewNorm)
					{
						if (auto* DelayPreset = WeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							DelayPreset->Settings.FilterFrequency = NormToFiltFreq(NewNorm);
							Preset->SetSettings(Preset->Settings);
						}
					})
				]

				// Filter Q knob
				+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Center).VAlign(VAlign_Center)
				.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
				[
					SAssignNew(FiltQKnob, SAudioFXLabeledKnob)
					.ParamName(LOCTEXT("FiltQLbl", "Q"))
					.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
						LOCTEXT("FiltQTipName", "Q"), 0.5f, 10.f, TEXT(""),
						LOCTEXT("FiltQTip", "Resonance (Q) of the feedback filter.")))
					.Value_Lambda([WeakPreset]() -> float
					{
						if (const auto* DelayPreset = WeakPreset.Get())
							return FMath::GetMappedRangeValueClamped(FVector2D(0.5f, 10.f), FVector2D(0.f, 1.f), DelayPreset->Settings.FilterQ);
						return FMath::GetMappedRangeValueClamped(FVector2D(0.5f, 10.f), FVector2D(0.f, 1.f), 0.7f);
					})
					.Style(FiltQStyle).AudioUnitsValueType(EAudioUnitsValueType::Linear)
					.ValueFormat(EAudioFXValueFormat::QFactor)
					.DefaultValue(FMath::GetMappedRangeValueClamped(FVector2D(0.5f, 10.f), FVector2D(0.f, 1.f), 0.7f))
					.MinValue(0.5f).MaxValue(10.f)
					.OnValueChanged_Lambda([WeakPreset, MarkDirty, Preset](float NewNorm)
					{
						if (auto* DelayPreset = WeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							DelayPreset->Settings.FilterQ = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 1.f), FVector2D(0.5f, 10.f), NewNorm);
							Preset->SetSettings(Preset->Settings);
						}
					})
				]

				// Delay Mode dropdown — right of filter controls
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.f, 0.f, 0.f, 0.f)
				[
					SNew(SAudioFXDropdown)
					.Options(ModeOpts).AssetColor(AssetCol)
					.SelectedIndex((int32)Preset->Settings.DelayMode)
					.OnSelectionChanged_Lambda([WeakPreset, MarkDirty, Preset](int32 OptionIndex)
					{
						if (auto* DelayPreset = WeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							DelayPreset->Settings.DelayMode = (EStereoDelaySourceEffect)OptionIndex;
							Preset->SetSettings(Preset->Settings);
						}
					})
				]
				] // end SHorizontalBox
			] // end SBorder card
		];

	return Widget;
}
} // anonymous namespace

// ═══════════════════════════════════════════════════════════════════════════
//  Public overloads — both delegate to the template
// ═══════════════════════════════════════════════════════════════════════════

TSharedRef<SWidget> AudioFXStereoDelayUI::BuildWidget(
	FAudioFXEffectEditor& Editor,
	USubmixEffectStereoDelayPreset* Preset,
	const FLinearColor& AssetCol)
{
	return BuildWidgetImpl(Editor, Preset, AssetCol);
}

TSharedRef<SWidget> AudioFXStereoDelayUI::BuildWidget(
	FAudioFXEffectEditor& Editor,
	USourceEffectStereoDelayPreset* Preset,
	const FLinearColor& AssetCol)
{
	return BuildWidgetImpl(Editor, Preset, AssetCol);
}

#undef LOCTEXT_NAMESPACE
