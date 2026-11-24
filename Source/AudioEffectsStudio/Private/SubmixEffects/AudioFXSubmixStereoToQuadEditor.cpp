// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "SubmixEffects/AudioFXSubmixStereoToQuadEditor.h"

#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Widgets/SAudioFXButton.h"
#include "Shared/Widgets/SAudioFXLabeledKnob.h"
#include "SubmixEffects/SubmixEffectStereoToQuad.h"

#include "AudioWidgetsEnums.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AudioFXSubmixStereoToQuad"

// ---------------------------------------------------------------------------

TSharedRef<SWidget> FAudioFXSubmixStereoToQuadEditor::CreateEffectWidget(USoundEffectPreset* InPreset)
{
	USubmixEffectStereoToQuadPreset* Preset = Cast<USubmixEffectStereoToQuadPreset>(InPreset);
	if (!Preset)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("AudioEffectsStudio: Cast to preset type failed for '%s'. Check asset registration."),
			InPreset ? *InPreset->GetClass()->GetName() : TEXT("null"));
		return FAudioFXEffectEditor::CreateEffectWidget(InPreset);
	}

	const FLinearColor AssetCol = GetAssetColor();
	TWeakObjectPtr<USubmixEffectStereoToQuadPreset> WeakPreset = Preset;

	const FAudioMaterialKnobStyle* GainStyle = StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain, AssetCol, EAudioFXKnobSize::Hero));

	return SNew(SBox)
		.Padding(FMargin(FAudioFXTheme::WindowPadding, FAudioFXTheme::SectionPadding))
		[
			SNew(SVerticalBox)

			// ── Rear Gain knob ───────────────────────────────────────
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f, FAudioFXTheme::KnobHorizontalSpacing, 6.f)
			[
				SNew(SAudioFXLabeledKnob)
				.ParamName(LOCTEXT("RearGainLbl", "Rear Gain"))
				.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
					LOCTEXT("RearGainTipName", "Rear Gain"),
					-24.f, 6.f, TEXT("dB"),
					LOCTEXT("RearGainTip",
						"The gain (in decibels) of the rear channels.\n"
						"AudioFactory Note: Adjusts the volume of audio routed to the rear surround speakers. "
						"Only effective with >2 output channels.")))
				.Value_Lambda([WeakPreset]() -> float
				{
					if (const auto* Preset = WeakPreset.Get())
					{
						return FMath::GetMappedRangeValueClamped(
							FVector2D(-24.f, 6.f), FVector2D(0.f, 1.f), Preset->Settings.RearChannelGain);
					}
					return 0.5f;
				})
				.Style(GainStyle)
				.AudioUnitsValueType(EAudioUnitsValueType::Volume)
				.ValueFormat(EAudioFXValueFormat::Decibels)
				.DefaultValue(FMath::GetMappedRangeValueClamped(FVector2D(-24.f, 6.f), FVector2D(0.f, 1.f), 0.f))
				.MinValue(-24.f)
				.MaxValue(6.f)
				.OnValueChanged_Lambda([this, WeakPreset](float NewNorm)
				{
					if (auto* Preset = WeakPreset.Get())
					{
						MarkDirty.ExecuteIfBound();
						Preset->Settings.RearChannelGain = FMath::GetMappedRangeValueClamped(
							FVector2D(0.f, 1.f), FVector2D(-24.f, 6.f), NewNorm);
						Preset->SetSettings(Preset->Settings);
					}
				})
			]

			// ── Flip Channels toggle ─────────────────────────────────
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Bottom)
			.Padding(0.f, 6.f, 0.f, 0.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SAudioFXButton)
					.Options(TArray<FText>{ LOCTEXT("Flip", "Flip Channels") })
					.AssetColor(AssetCol)
					.SelectedIndex(Preset->Settings.bFlipChannels ? 0 : -1)
					.OnSelectionChanged_Lambda([this, WeakPreset](int32 OptionIndex)
					{
						if (auto* Preset = WeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							Preset->Settings.bFlipChannels = (OptionIndex == 0);
							Preset->SetSettings(Preset->Settings);
						}
					})
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 4.f, 0.f, 0.f)
				[ FAudioFXTheme::MakeGroupLabel(LOCTEXT("FlipLbl", "ROUTING")) ]
			]
		];
}

#undef LOCTEXT_NAMESPACE
