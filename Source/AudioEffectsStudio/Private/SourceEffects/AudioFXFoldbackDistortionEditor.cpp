// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

/**
 * FAudioFXFoldbackDistortionEditor
 *
 * Knob-based editor skin for USourceEffectFoldbackDistortionPreset.
 * Exposes InputGain, Threshold (hero), and OutputGain as three knobs.
 */

#include "SourceEffects/AudioFXFoldbackDistortionEditor.h"
#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Widgets/SAudioFXLabeledKnob.h"

#include "SourceEffects/SourceEffectFoldbackDistortion.h"
#include "AudioWidgetsEnums.h"
#include "Widgets/Layout/SBox.h"

#define LOCTEXT_NAMESPACE "AudioFXFoldbackDistortion"

TSharedRef<SWidget> FAudioFXFoldbackDistortionEditor::CreateEffectWidget(USoundEffectPreset* InPreset)
{
	USourceEffectFoldbackDistortionPreset* Preset = Cast<USourceEffectFoldbackDistortionPreset>(InPreset);
	if (!Preset)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("AudioEffectsStudio: Cast to preset type failed for '%s'. Check asset registration."),
			InPreset ? *InPreset->GetClass()->GetName() : TEXT("null"));
		return FAudioFXEffectEditor::CreateEffectWidget(InPreset);
	}

	const FLinearColor AssetCol = GetAssetColor();
	TWeakObjectPtr<USourceEffectFoldbackDistortionPreset> WeakPreset = Preset;

	// Visual Redesign: Threshold=Hero, InGain/OutGain=Secondary
	const FAudioMaterialKnobStyle* InputGainStyle  = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain,     AssetCol, EAudioFXKnobSize::Secondary));
	const FAudioMaterialKnobStyle* ThresholdStyle  = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Dynamics,  AssetCol, EAudioFXKnobSize::Hero));
	const FAudioMaterialKnobStyle* OutputGainStyle = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain,     AssetCol, EAudioFXKnobSize::Secondary));

	return SNew(SBox)
		.Padding(FAudioFXTheme::SectionPadding)
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
		[
			SNew(SAudioFXLabeledKnob)
				.ParamName(LOCTEXT("InGainLbl", "Input Gain"))
				.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
					LOCTEXT("InGainTipName", "Input Gain"),
					0.f, 20.f,
					TEXT("dB"),
					LOCTEXT("InGainTip",
						"The amount of gain to add to input to allow forcing the triggering of the threshold.\n"
						"AudioFactory Note: Drives the signal into the threshold. Higher values trigger more foldback cycles.")))
				.Value_Lambda([WeakPreset]() -> float
				{
					if (auto* Preset = WeakPreset.Get())
					{
						return FMath::GetMappedRangeValueClamped(FVector2D(0.f, 20.f), FVector2D(0.f, 1.f), Preset->Settings.InputGainDb);
					}
					return 0.f;
				})
				.Style(InputGainStyle)
				.AudioUnitsValueType(EAudioUnitsValueType::Volume)
				.ValueFormat(EAudioFXValueFormat::Decibels)
				.DefaultValue(FMath::GetMappedRangeValueClamped(FVector2D(0.f, 20.f), FVector2D(0.f, 1.f), 0.f))
				.MinValue(0.f)
				.MaxValue(20.f)
				.OnValueChanged_Lambda([this, WeakPreset](float NewNorm)
				{
					if (auto* Preset = WeakPreset.Get())
					{
						MarkDirty.ExecuteIfBound();
						Preset->Settings.InputGainDb = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 1.f), FVector2D(0.f, 20.f), NewNorm);
						Preset->SetSettings(Preset->Settings);
					}
				})
		]

		+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
		[
			SNew(SAudioFXLabeledKnob)
				.ParamName(LOCTEXT("ThreshLbl", "Threshold"))
				.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
					LOCTEXT("ThreshTipName", "Threshold"),
					-60.f, 0.f,
					TEXT("dB"),
					LOCTEXT("ThreshTip",
						"If the audio amplitude is higher than this, it will fold back.\n"
						"AudioFactory Note: Audio exceeding this level is reflected (folded) back. Lower thresholds produce more aggressive distortion.")))
				.Value_Lambda([WeakPreset]() -> float
				{
					if (auto* Preset = WeakPreset.Get())
					{
						return FMath::GetMappedRangeValueClamped(FVector2D(-60.f, 0.f), FVector2D(0.f, 1.f), Preset->Settings.ThresholdDb);
					}
					return 0.9f;
				})
				.Style(ThresholdStyle)
				.AudioUnitsValueType(EAudioUnitsValueType::Volume)
				.ValueFormat(EAudioFXValueFormat::Decibels)
				.DefaultValue(FMath::GetMappedRangeValueClamped(FVector2D(-60.f, 0.f), FVector2D(0.f, 1.f), -6.f))
				.MinValue(-60.f)
				.MaxValue(0.f)
				.OnValueChanged_Lambda([this, WeakPreset](float NewNorm)
				{
					if (auto* Preset = WeakPreset.Get())
					{
						MarkDirty.ExecuteIfBound();
						Preset->Settings.ThresholdDb = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 1.f), FVector2D(-60.f, 0.f), NewNorm);
						Preset->SetSettings(Preset->Settings);
					}
				})
		]

		+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
		[
			SNew(SAudioFXLabeledKnob)
				.ParamName(LOCTEXT("OutGainLbl", "Output Gain"))
				.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
					LOCTEXT("OutGainTipName", "Output Gain"),
					-60.f, 20.f,
					TEXT("dB"),
					LOCTEXT("OutGainTip", "The amount of gain to apply to the output.")))
				.Value_Lambda([WeakPreset]() -> float
				{
					if (auto* Preset = WeakPreset.Get())
					{
						return FMath::GetMappedRangeValueClamped(FVector2D(-60.f, 20.f), FVector2D(0.f, 1.f), Preset->Settings.OutputGainDb);
					}
					return 0.75f;
				})
				.Style(OutputGainStyle)
				.AudioUnitsValueType(EAudioUnitsValueType::Volume)
				.ValueFormat(EAudioFXValueFormat::Decibels)
				.DefaultValue(FMath::GetMappedRangeValueClamped(FVector2D(-60.f, 20.f), FVector2D(0.f, 1.f), 0.f))
				.MinValue(-60.f)
				.MaxValue(20.f)
				.OnValueChanged_Lambda([this, WeakPreset](float NewNorm)
				{
					if (auto* Preset = WeakPreset.Get())
					{
						MarkDirty.ExecuteIfBound();
						Preset->Settings.OutputGainDb = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 1.f), FVector2D(-60.f, 20.f), NewNorm);
						Preset->SetSettings(Preset->Settings);
					}
				})
		]
	];
}

#undef LOCTEXT_NAMESPACE
