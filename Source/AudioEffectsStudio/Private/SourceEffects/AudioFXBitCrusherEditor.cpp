// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

/**
 * FAudioFXBitCrusherEditor
 *
 * Knob-based editor skin for USourceEffectBitCrusherPreset.
 * Exposes Sample Rate and Bit Depth as dual hero-sized knobs.
 */

#include "SourceEffects/AudioFXBitCrusherEditor.h"
#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Widgets/SAudioFXLabeledKnob.h"

#include "SourceEffects/SourceEffectBitCrusher.h"
#include "AudioWidgetsEnums.h"
#include "Widgets/Layout/SBox.h"

#define LOCTEXT_NAMESPACE "AudioFXBitCrusher"

TSharedRef<SWidget> FAudioFXBitCrusherEditor::CreateEffectWidget(USoundEffectPreset* InPreset)
{
	USourceEffectBitCrusherPreset* Preset = Cast<USourceEffectBitCrusherPreset>(InPreset);
	if (!Preset)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("AudioEffectsStudio: Cast to preset type failed for '%s'. Check asset registration."),
			InPreset ? *InPreset->GetClass()->GetName() : TEXT("null"));
		return FAudioFXEffectEditor::CreateEffectWidget(InPreset);
	}

	const FLinearColor AssetCol = GetAssetColor();
	TWeakObjectPtr<USourceEffectBitCrusherPreset> WeakPreset = Preset;

	// ── Knob Styles ──
	const FAudioMaterialKnobStyle* SampleRateStyle = StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Mix, AssetCol, EAudioFXKnobSize::Hero));
	const FAudioMaterialKnobStyle* BitDepthStyle = StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Frequency, AssetCol, EAudioFXKnobSize::Hero));

	// ── Parameter Ranges ──
	constexpr float SampleRateMin = 500.f, SampleRateMax = 16000.f, SampleRateDefault = 8000.f;
	constexpr float BitDepthMin = 1.f,   BitDepthMax = 16.f,    BitDepthDefault = 8.f;

	// ── Layout ──
	return SNew(SBox).Padding(FAudioFXTheme::SectionPadding)
	[
		SNew(SHorizontalBox)

		// ── Bit Depth (hero, left) ──────────────────────────────
		+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
		[
			SNew(SAudioFXLabeledKnob)
			.ParamName(LOCTEXT("BdLabel", "Bit Depth"))
			.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
				LOCTEXT("BdTipName", "Bit Depth"),
				BitDepthMin, BitDepthMax,
				TEXT("bits"),
				LOCTEXT("BdTip",
					"The reduced bit depth to use for the audio stream.\n"
					"AudioFactory Note: Quantization steps = 2^BitDepth. Below 4 bits, audio becomes heavily distorted.")))
			.Value_Lambda([WeakPreset]() -> float
			{
				if (auto* Preset = WeakPreset.Get())
				{
					return FMath::GetMappedRangeValueClamped(
						FVector2D(1.f, 16.f), FVector2D(0.f, 1.f),
						Preset->Settings.BitModulation.Value);
				}
				return FMath::GetMappedRangeValueClamped(
					FVector2D(1.f, 16.f), FVector2D(0.f, 1.f), 8.f);
			})
			.Style(BitDepthStyle)
			.AudioUnitsValueType(EAudioUnitsValueType::Linear)
			.ValueFormat(EAudioFXValueFormat::BitDepth)
			.DefaultValue(FMath::GetMappedRangeValueClamped(
				FVector2D(BitDepthMin, BitDepthMax), FVector2D(0.f, 1.f), BitDepthDefault))
			.MinValue(BitDepthMin)
		.MaxValue(BitDepthMax)
		.OnValueChanged_Lambda([this, WeakPreset](float NewNorm)
		{
			if (auto* Preset = WeakPreset.Get())
			{
				const float Bits = FMath::GetMappedRangeValueClamped(
					FVector2D(0.f, 1.f), FVector2D(BitDepthMin, BitDepthMax), NewNorm);
				MarkDirty.ExecuteIfBound();
				Preset->Settings.BitModulation.Value = Bits;
				FSourceEffectBitCrusherBaseSettings Base;
				Base.SampleRate = Preset->Settings.SampleRateModulation.Value;
				Base.BitDepth   = Preset->Settings.BitModulation.Value;
				Preset->SetSettings(Base);
			}
		})
		]

		// ── Sample Rate (hero, right) ───────────────────────────
		+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
		[
			SNew(SAudioFXLabeledKnob)
			.ParamName(LOCTEXT("SrLabel", "Sample Rate"))
			.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
				LOCTEXT("SrTipName", "Sample Rate"),
				SampleRateMin, SampleRateMax,
				TEXT("Hz"),
				LOCTEXT("SrTip",
					"The reduced frequency to use for the audio stream.\n"
					"AudioFactory Note: Uses sample-and-hold internally. Values below ~4000 Hz produce obvious aliasing artifacts.")))
			.Value_Lambda([WeakPreset]() -> float
			{
				if (auto* Preset = WeakPreset.Get())
				{
					return FMath::GetMappedRangeValueClamped(
						FVector2D(500.f, 16000.f), FVector2D(0.f, 1.f),
						Preset->Settings.SampleRateModulation.Value);
				}
				return FMath::GetMappedRangeValueClamped(
					FVector2D(500.f, 16000.f), FVector2D(0.f, 1.f), 8000.f);
			})
			.Style(SampleRateStyle)
			.AudioUnitsValueType(EAudioUnitsValueType::Frequency)
			.ValueFormat(EAudioFXValueFormat::Frequency)
			.DefaultValue(FMath::GetMappedRangeValueClamped(
				FVector2D(SampleRateMin, SampleRateMax), FVector2D(0.f, 1.f), SampleRateDefault))
		.MinValue(SampleRateMin)
		.MaxValue(SampleRateMax)
		.OnValueChanged_Lambda([this, WeakPreset](float NewNorm)
		{
			if (auto* Preset = WeakPreset.Get())
			{
				const float Hz = FMath::GetMappedRangeValueClamped(
					FVector2D(0.f, 1.f), FVector2D(SampleRateMin, SampleRateMax), NewNorm);
				MarkDirty.ExecuteIfBound();
				Preset->Settings.SampleRateModulation.Value = Hz;
				FSourceEffectBitCrusherBaseSettings Base;
				Base.SampleRate = Preset->Settings.SampleRateModulation.Value;
				Base.BitDepth   = Preset->Settings.BitModulation.Value;
				Preset->SetSettings(Base);
			}
		})
		]
	];
}

#undef LOCTEXT_NAMESPACE
