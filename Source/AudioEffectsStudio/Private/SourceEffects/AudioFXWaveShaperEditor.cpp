// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

/**
 * AudioFXWaveShaperEditor.cpp
 *
 * Custom Slate editor widget for the WaveShaper source effect.
 */

#include "SourceEffects/AudioFXWaveShaperEditor.h"
#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Widgets/SAudioFXLabeledKnob.h"

#include "SourceEffects/SourceEffectWaveShaper.h"
#include "Shared/Visualizers/SAudioFXWaveShaperCurve.h"
#include "AudioWidgetsEnums.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AudioFXWaveShaper"

TSharedRef<SWidget> FAudioFXWaveShaperEditor::CreateEffectWidget(USoundEffectPreset* InPreset)
{
	USourceEffectWaveShaperPreset* Preset = Cast<USourceEffectWaveShaperPreset>(InPreset);
	if (!Preset)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("AudioEffectsStudio: Cast to preset type failed for '%s'. Check asset registration."),
			InPreset ? *InPreset->GetClass()->GetName() : TEXT("null"));
		return FAudioFXEffectEditor::CreateEffectWidget(InPreset);
	}

	const FLinearColor AssetCol = GetAssetColor();
	TWeakObjectPtr<USourceEffectWaveShaperPreset> WeakPreset = Preset;

	// Amount = Blue Hero, Output Gain = Orange I/O
	const FAudioMaterialKnobStyle* AmountStyle = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Frequency, AssetCol, EAudioFXKnobSize::Hero));
	const FAudioMaterialKnobStyle* GainStyle   = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain,      AssetCol, EAudioFXKnobSize::Primary));

	return SNew(SBox)
		.Padding(FMargin(FAudioFXTheme::WindowPadding, FAudioFXTheme::SectionPadding))
	[
		SNew(SHorizontalBox)

		// ── LEFT: WaveShaper transfer curve — perfect square ─────────
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.f, 0.f, FAudioFXTheme::WindowPadding, 0.f)
		[
			SNew(SBox)
			.WidthOverride(280.f)
			.HeightOverride(280.f)
			[
				SNew(SOverlay)

				+ SOverlay::Slot()
				[
					SNew(SAudioFXWaveShaperCurve)
						.AssetColor(AssetCol)
						.Amount_Lambda([WeakPreset]() -> float
						{
							if (auto* Preset = WeakPreset.Get()) { return Preset->Settings.Amount; }
							return 1.f;
						})
				]

				+ SOverlay::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Top)
					.Padding(4.f, 3.f)
				[
					SNew(STextBlock)
						.Text(LOCTEXT("OutLbl", "OUT"))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
						.ColorAndOpacity(FAudioFXTheme::GetTextDisabledColor())
				]

				+ SOverlay::Slot()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Bottom)
					.Padding(4.f, 3.f)
				[
					SNew(STextBlock)
						.Text(LOCTEXT("InLbl", "IN"))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
						.ColorAndOpacity(FAudioFXTheme::GetTextDisabledColor())
				]
			]
		]

		// ── RIGHT: Amount (Hero Blue) + Output Gain (Orange) side-by-side ──
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
			.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
			[
				SNew(SAudioFXLabeledKnob)
					.ParamName(LOCTEXT("AmtLbl", "Amount"))
					.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
						LOCTEXT("AmtTipName", "Amount"), 0.f, 500.f, TEXT(""),
						LOCTEXT("AmtTip",
							"The amount of wave shaping. 0.0 = no wave shaping.\n"
							"AudioFactory Note: Internally clamped to a minimum of 1.0. Uses ATan transfer curve.")))
					.Value_Lambda([WeakPreset]() -> float
					{
						if (auto* Preset = WeakPreset.Get())
						{
							return FMath::GetMappedRangeValueClamped(FVector2D(0.f, 500.f), FVector2D(0.f, 1.f), Preset->Settings.Amount);
						}
						return FMath::GetMappedRangeValueClamped(FVector2D(0.f, 500.f), FVector2D(0.f, 1.f), 1.f);
					})
					.Style(AmountStyle)
					.AudioUnitsValueType(EAudioUnitsValueType::Linear)
					.ValueFormat(EAudioFXValueFormat::Default)
					.DefaultValue(FMath::GetMappedRangeValueClamped(FVector2D(0.f, 500.f), FVector2D(0.f, 1.f), 1.f))
					.MinValue(0.f)
					.MaxValue(500.f)
					.OnValueChanged_Lambda([this, WeakPreset](float NewNorm)
					{
						if (auto* Preset = WeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							Preset->Settings.Amount = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 1.f), FVector2D(0.f, 500.f), NewNorm);
							Preset->SetSettings(Preset->Settings);
						}
					})
			]

			+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
			.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
			[
				SNew(SAudioFXLabeledKnob)
					.ParamName(LOCTEXT("GainLbl", "Output Gain"))
					.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
						LOCTEXT("GainTipName", "Output Gain"), -60.f, 20.f, TEXT("dB"),
						LOCTEXT("GainTip",
							"Sets the output gain of the waveshaper.\n"
							"AudioFactory Note: Applied after the shaping stage. Use to compensate for level changes.")))
					.Value_Lambda([WeakPreset]() -> float
					{
						if (auto* Preset = WeakPreset.Get())
						{
							return FMath::GetMappedRangeValueClamped(FVector2D(-60.f, 20.f), FVector2D(0.f, 1.f), Preset->Settings.OutputGainDb);
						}
						return FMath::GetMappedRangeValueClamped(FVector2D(-60.f, 20.f), FVector2D(0.f, 1.f), 0.f);
					})
					.Style(GainStyle)
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
		]
	];
}

#undef LOCTEXT_NAMESPACE
