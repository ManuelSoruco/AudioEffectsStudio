// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

/**
 * FAudioFXEnvelopeFollowerEditor — Slate skin for the Envelope Follower source effect.
 */

#include "SourceEffects/AudioFXEnvelopeFollowerEditor.h"
#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Widgets/SAudioFXDropdown.h"
#include "Shared/Widgets/SAudioFXButton.h"

#include "SourceEffects/SourceEffectEnvelopeFollower.h"
#include "Shared/Visualizers/SAudioFXEnvelopeDisplay.h"
#include "Shared/Widgets/SAudioFXLabeledKnob.h"
#include "AudioWidgetsEnums.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AudioFXEnvelopeFollower"

TSharedRef<SWidget> FAudioFXEnvelopeFollowerEditor::CreateEffectWidget(USoundEffectPreset* InPreset)
{
	USourceEffectEnvelopeFollowerPreset* Preset = Cast<USourceEffectEnvelopeFollowerPreset>(InPreset);
	if (!Preset)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("AudioEffectsStudio: Cast to preset type failed for '%s'. Check asset registration."),
			InPreset ? *InPreset->GetClass()->GetName() : TEXT("null"));
		return FAudioFXEffectEditor::CreateEffectWidget(InPreset);
	}

	const FLinearColor AssetCol = GetAssetColor();

	// Visual Redesign: Attack=Hero (Blue), Release=Hero (Green) — same size
	const FAudioMaterialKnobStyle* AttackStyle  = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Frequency, AssetCol, EAudioFXKnobSize::Hero));
	const FAudioMaterialKnobStyle* ReleaseStyle = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Mix,       AssetCol, EAudioFXKnobSize::Hero));

	TWeakObjectPtr<USourceEffectEnvelopeFollowerPreset> WeakPreset = Preset;

	TSharedPtr<SAudioFXLabeledKnob> AttackKnob, ReleaseKnob;

	TArray<FText> PeakModeOptions = { LOCTEXT("MS", "MS"), LOCTEXT("RMS", "RMS"), LOCTEXT("Peak", "Peak") };

	TSharedRef<SWidget> Widget =
		SNew(SBox)
		.Padding(FMargin(FAudioFXTheme::WindowPadding, FAudioFXTheme::SectionPadding))
		[
			SNew(SHorizontalBox)

			// ── LEFT: Envelope shape display (~55% width) ──────────────
			+ SHorizontalBox::Slot()
			.FillWidth(1.1f)
			.Padding(0.f, 0.f, FAudioFXTheme::WindowPadding, 0.f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SBox)
					.MinDesiredHeight(140.f)
					[
						SNew(SAudioFXEnvelopeDisplay)
						.AssetColor(AssetCol)
						.AttackTime_Lambda([WeakPreset]() -> float
						{
							if (auto* Preset = WeakPreset.Get())
							{
								return Preset->Settings.AttackTime;
							}
							return 10.f;
						})
						.ReleaseTime_Lambda([WeakPreset]() -> float
						{
							if (auto* Preset = WeakPreset.Get())
							{
								return Preset->Settings.ReleaseTime;
							}
							return 200.f;
						})
					]
				]
				+ SOverlay::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Bottom)
				.Padding(6.f, 0.f, 0.f, 4.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ALabel", "A"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
					.ColorAndOpacity(FAudioFXTheme::GetTextDisabledColor())
				]
				+ SOverlay::Slot()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Bottom)
				.Padding(0.f, 0.f, 6.f, 4.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("RLabel", "R"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
					.ColorAndOpacity(FAudioFXTheme::GetTextDisabledColor())
				]
			]

			// ── RIGHT: Attack + Release knobs + Mode controls ──────────
			+ SHorizontalBox::Slot()
			.FillWidth(0.9f)
			[
				SNew(SVerticalBox)

				// ── Knob row: Attack (Hero Blue) | Release (Hero Green) ──
				+ SVerticalBox::Slot()
				.FillHeight(1.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
					[
						SAssignNew(AttackKnob, SAudioFXLabeledKnob)
						.ParamName(LOCTEXT("AttackLbl", "Attack"))
					.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
						LOCTEXT("AttackTipLbl", "Attack Time"),
						0.f, 300.f, TEXT("ms"),
						LOCTEXT("AttackTip",
							"The attack time of the envelope follower in milliseconds.\n"
							"AudioFactory Note: Controls how fast the envelope responds to rising input; "
							"at 0 ms the response is instantaneous.")))
					.Value_Lambda([WeakPreset]() -> float
					{
						if (auto* Preset = WeakPreset.Get())
						{
							return FMath::GetMappedRangeValueClamped(FVector2D(0.f, 300.f), FVector2D(0.f, 1.f), Preset->Settings.AttackTime);
						}
						return 0.033f;
					})
					.Style(AttackStyle)
					.AudioUnitsValueType(EAudioUnitsValueType::Linear)
					.ValueFormat(EAudioFXValueFormat::Time)
					.MinValue(0.f)
					.MaxValue(300.f)
					.DefaultValue(FMath::GetMappedRangeValueClamped(FVector2D(0.f, 300.f), FVector2D(0.f, 1.f), 10.f))
					.OnValueChanged_Lambda([this, WeakPreset](float NewNorm)
					{
						if (auto* Preset = WeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							Preset->Settings.AttackTime = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 1.f), FVector2D(0.f, 300.f), NewNorm);
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
					SAssignNew(ReleaseKnob, SAudioFXLabeledKnob)
					.ParamName(LOCTEXT("ReleaseLbl", "Release"))
					.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
						LOCTEXT("ReleaseTipLbl", "Release Time"),
						0.f, 5000.f, TEXT("ms"),
						LOCTEXT("ReleaseTip",
							"The release time of the envelope follower in milliseconds.\n"
							"AudioFactory Note: Controls how slowly the envelope dampens after input drops; "
							"Analog mode uses an RC time constant (~1 ms), Digital mode uses ~4.6 ms.")))
					.Value_Lambda([WeakPreset]() -> float
					{
						if (auto* Preset = WeakPreset.Get())
						{
							return FMath::GetMappedRangeValueClamped(FVector2D(0.f, 5000.f), FVector2D(0.f, 1.f), Preset->Settings.ReleaseTime);
						}
						return 0.04f;
					})
					.Style(ReleaseStyle)
					.AudioUnitsValueType(EAudioUnitsValueType::Linear)
					.ValueFormat(EAudioFXValueFormat::Time)
					.MinValue(0.f)
					.MaxValue(5000.f)
					.DefaultValue(FMath::GetMappedRangeValueClamped(FVector2D(0.f, 5000.f), FVector2D(0.f, 1.f), 200.f))
					.OnValueChanged_Lambda([this, WeakPreset](float NewNorm)
					{
						if (auto* Preset = WeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							Preset->Settings.ReleaseTime = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 1.f), FVector2D(0.f, 5000.f), NewNorm);
							Preset->SetSettings(Preset->Settings);
						}
					})
				]
			]

				// ── Mode dropdown + Analog toggle ──
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.f, FAudioFXTheme::LabelToControlGap, 0.f, 0.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.f, 0.f, 6.f, 0.f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("PeakModeLabel", "Mode:"))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
						.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SAudioFXDropdown)
						.Options(PeakModeOptions)
						.AssetColor(AssetCol)
						.SelectedIndex((int32)Preset->Settings.PeakMode)
						.OnSelectionChanged_Lambda([this, WeakPreset](int32 Idx)
						{
							if (auto* Preset = WeakPreset.Get())
							{
								MarkDirty.ExecuteIfBound();
								Preset->Settings.PeakMode = (EEnvelopeFollowerPeakMode)Idx;
								Preset->SetSettings(Preset->Settings);
							}
						})
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(10.f, 0.f, 0.f, 0.f)
					[
						SNew(SAudioFXButton)
						.Options(TArray<FText>{ LOCTEXT("AnalogMode", "Analog") })
						.AssetColor(AssetCol)
						.SelectedIndex(Preset->Settings.bIsAnalogMode ? 0 : -1)
						.OnSelectionChanged_Lambda([this, WeakPreset](int32 Idx)
						{
							if (auto* Preset = WeakPreset.Get())
							{
								MarkDirty.ExecuteIfBound();
								Preset->Settings.bIsAnalogMode = (Idx == 0);
								Preset->SetSettings(Preset->Settings);
							}
						})
					]
				]
			]
		];

	return Widget;
}

#undef LOCTEXT_NAMESPACE
