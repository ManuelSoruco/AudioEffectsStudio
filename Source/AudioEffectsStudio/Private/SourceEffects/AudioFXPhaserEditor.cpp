// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

/**
 * FAudioFXPhaserEditor
 *
 * Knob-based editor skin for USourceEffectPhaserPreset.
 * Exposes Wet, Frequency, and Feedback as modulation knobs with
 * LFO shape and quadrature-phase toggles in a side panel.
 */

#include "SourceEffects/AudioFXPhaserEditor.h"
#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Widgets/SAudioFXDropdown.h"
#include "Shared/Widgets/SAudioFXButton.h"
#include "Shared/Widgets/SAudioFXLabeledKnob.h"

#include "SourceEffects/SourceEffectPhaser.h"
#include "AudioWidgetsEnums.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AudioFXPhaser"

TSharedRef<SWidget> FAudioFXPhaserEditor::CreateEffectWidget(USoundEffectPreset* InPreset)
{
	USourceEffectPhaserPreset* Preset = Cast<USourceEffectPhaserPreset>(InPreset);
	if (!Preset)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("AudioEffectsStudio: Cast to preset type failed for '%s'. Check asset registration."),
			InPreset ? *InPreset->GetClass()->GetName() : TEXT("null"));
		return FAudioFXEffectEditor::CreateEffectWidget(InPreset);
	}

	const FLinearColor AssetCol = GetAssetColor();
	TWeakObjectPtr<USourceEffectPhaserPreset> WeakPreset = Preset;

	// Visual Redesign: Freq=Hero, Wet=Primary, Feedback=Secondary
	const FAudioMaterialKnobStyle* WetStyle  = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain,      AssetCol, EAudioFXKnobSize::Primary));
	const FAudioMaterialKnobStyle* FreqStyle = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Frequency, AssetCol, EAudioFXKnobSize::Hero));
	const FAudioMaterialKnobStyle* FbStyle   = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Mix,       AssetCol, EAudioFXKnobSize::Secondary));

	TArray<FText> LFOOptions = { LOCTEXT("Sine", "Sine"), LOCTEXT("UpSaw", "Up Saw"), LOCTEXT("DownSaw", "Down Saw"),
	                             LOCTEXT("Square", "Square"), LOCTEXT("Tri", "Triangle"), LOCTEXT("Exp", "Exponential"), LOCTEXT("SH", "Sample & Hold") };

	return SNew(SBox)
		.Padding(FMargin(FAudioFXTheme::WindowPadding, FAudioFXTheme::SectionPadding))
	[
		SNew(SVerticalBox)

		// ── Knob row: Wet (Primary) | Frequency (Hero) | Feedback (Secondary) ──
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		.VAlign(VAlign_Top)
		.Padding(0.f, 4.f, 0.f, 0.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
			[
				SNew(SAudioFXLabeledKnob)
					.ParamName(LOCTEXT("WetLbl", "Wet"))
					.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
						LOCTEXT("WetTipLbl", "Wet Level"),
						0.f, 1.f, TEXT(""),
						LOCTEXT("WetTip",
							"The wet level of the phaser effect.\n"
							"AudioFactory Note: Internally mixed as Wet*processed + (1-Wet)*dry, "
							"so 0.5 gives equal blend.")))
					.Value_Lambda([WeakPreset]() -> float
					{
						if (auto* LambdaPreset = WeakPreset.Get())
						{
							return LambdaPreset->Settings.WetLevel;
						}
						return 0.5f;
					})
					.Style(WetStyle)
					.AudioUnitsValueType(EAudioUnitsValueType::Linear)
					.ValueFormat(EAudioFXValueFormat::Percentage)
					.DefaultValue(0.5f)
					.MinValue(0.f)
					.MaxValue(1.f)
					.OnValueChanged_Lambda([this, WeakPreset](float NewNorm)
					{
						if (auto* LambdaPreset = WeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							LambdaPreset->Settings.WetLevel = NewNorm;
							LambdaPreset->SetSettings(LambdaPreset->Settings);
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
					.ParamName(LOCTEXT("FreqLbl", "Frequency"))
					.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
						LOCTEXT("FreqTipLbl", "LFO Frequency"),
						0.f, 5.f, TEXT("Hz"),
						LOCTEXT("FreqTip",
							"The LFO frequency of the phaser effect.\n"
							"AudioFactory Note: Drives a chain of 6 all-pass filters whose cutoffs "
							"sweep across fixed frequency bands scaled to the sample rate.")))
					.Value_Lambda([WeakPreset]() -> float
					{
						if (auto* LambdaPreset = WeakPreset.Get())
						{
							return FMath::GetMappedRangeValueClamped(
								FVector2D(0.f, 5.f), FVector2D(0.f, 1.f),
								LambdaPreset->Settings.Frequency);
						}
						return 0.2f;
					})
					.Style(FreqStyle)
					.AudioUnitsValueType(EAudioUnitsValueType::Frequency)
					.ValueFormat(EAudioFXValueFormat::Frequency)
					.DefaultValue(FMath::GetMappedRangeValueClamped(
						FVector2D(0.f, 5.f), FVector2D(0.f, 1.f), 1.f))
					.MinValue(0.f)
					.MaxValue(5.f)
					.OnValueChanged_Lambda([this, WeakPreset](float NewNorm)
					{
						if (auto* LambdaPreset = WeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							LambdaPreset->Settings.Frequency = FMath::GetMappedRangeValueClamped(
								FVector2D(0.f, 1.f), FVector2D(0.f, 5.f), NewNorm);
							LambdaPreset->SetSettings(LambdaPreset->Settings);
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
					.ParamName(LOCTEXT("FbLbl", "Feedback"))
					.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
						LOCTEXT("FbTipLbl", "Feedback"),
						0.f, 1.f, TEXT(""),
						LOCTEXT("FbTip",
							"The feedback of the phaser effect.\n"
							"AudioFactory Note: Feedback is taken from the output of the final "
							"all-pass stage and mixed back into the input.")))
					.Value_Lambda([WeakPreset]() -> float
					{
						if (auto* LambdaPreset = WeakPreset.Get())
						{
							return LambdaPreset->Settings.Feedback;
						}
						return 0.5f;
					})
					.Style(FbStyle)
					.AudioUnitsValueType(EAudioUnitsValueType::Linear)
					.ValueFormat(EAudioFXValueFormat::Percentage)
					.DefaultValue(0.5f)
					.MinValue(0.f)
					.MaxValue(1.f)
					.OnValueChanged_Lambda([this, WeakPreset](float NewNorm)
					{
						if (auto* LambdaPreset = WeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							LambdaPreset->Settings.Feedback = NewNorm;
							LambdaPreset->SetSettings(LambdaPreset->Settings);
						}
					})
			]
		]

		// ── Controls row: Shape dropdown + Quadrature toggle ──
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(0.f, 32.f, 0.f, 0.f)  // extra top padding to push controls down
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(STextBlock)
					.Text(LOCTEXT("ShapeL", "Shape:"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 10.f, 0.f)
			[
				SNew(SAudioFXDropdown)
					.Options(LFOOptions)
					.AssetColor(AssetCol)
					.SelectedIndex((int32)Preset->Settings.LFOType)
					.OnSelectionChanged_Lambda([this, WeakPreset](int32 OptionIndex)
					{
						if (auto* LambdaPreset = WeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							LambdaPreset->Settings.LFOType = (EPhaserLFOType)OptionIndex;
							LambdaPreset->SetSettings(LambdaPreset->Settings);
						}
					})
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SAudioFXButton)
					.Options(TArray<FText>{LOCTEXT("Quad", "Quadrature Phase")})
					.AssetColor(AssetCol)
					.SelectedIndex(Preset->Settings.UseQuadraturePhase ? 0 : -1)
					.OnSelectionChanged_Lambda([this, WeakPreset](int32 OptionIndex)
					{
						if (auto* LambdaPreset = WeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							LambdaPreset->Settings.UseQuadraturePhase = (OptionIndex == 0);
							LambdaPreset->SetSettings(LambdaPreset->Settings);
						}
					})
			]
		]
	];
}

#undef LOCTEXT_NAMESPACE
