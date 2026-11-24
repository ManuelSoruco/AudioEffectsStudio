// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

/**
 * FAudioFXConvReverbSourceEditor
 *
 * Layout: IR waveform visualization (top), Wet/Dry knobs, impulse response asset picker,
 * and a collapsible ADVANCED section with HW Acceleration toggle and Block Size dropdown.
 */

#include "SourceEffects/AudioFXConvReverbSourceEditor.h"
#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Widgets/SAudioFXDropdown.h"
#include "Shared/Widgets/SAudioFXButton.h"
#include "Shared/Widgets/SAudioFXLabeledKnob.h"
#include "Shared/Widgets/SAudioFXCollapsibleSection.h"
#include "Shared/Visualizers/SAudioFXImpulseResponseCurve.h"

#include "SourceEffects/SourceEffectConvolutionReverb.h"
#include "PropertyCustomizationHelpers.h"
#include "AudioWidgetsEnums.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "AssetRegistry/AssetData.h"
#include "EffectConvolutionReverb.h"

#define LOCTEXT_NAMESPACE "AudioFXConvReverbSource"

bool FAudioFXConvReverbSourceEditor::GetBypassState() const
{
	if (const USourceEffectConvolutionReverbPreset* Preset = Cast<USourceEffectConvolutionReverbPreset>(GetPreset()))
	{
		return Preset->Settings.bBypass;
	}
	return false;
}

void FAudioFXConvReverbSourceEditor::SetBypassState(bool bBypassed)
{
	if (USourceEffectConvolutionReverbPreset* Preset = Cast<USourceEffectConvolutionReverbPreset>(GetPreset()))
	{
		Preset->Settings.bBypass = bBypassed;
		Preset->SetSettings(Preset->Settings);
	}
}

TSharedRef<SWidget> FAudioFXConvReverbSourceEditor::CreateEffectWidget(USoundEffectPreset* InPreset)
{
	USourceEffectConvolutionReverbPreset* Preset = Cast<USourceEffectConvolutionReverbPreset>(InPreset);
	if (!Preset)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("AudioEffectsStudio: Cast to preset type failed for '%s'. Check asset registration."),
			InPreset ? *InPreset->GetClass()->GetName() : TEXT("null"));
		return FAudioFXEffectEditor::CreateEffectWidget(InPreset);
	}

	const FLinearColor AssetCol = GetAssetColor();
	TWeakObjectPtr<USourceEffectConvolutionReverbPreset> WeakPreset = Preset;

	// Wet/Dry = Orange (I/O)
	const FAudioMaterialKnobStyle* WetStyle = StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain, AssetCol, EAudioFXKnobSize::Secondary));
	const FAudioMaterialKnobStyle* DryStyle = StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain, AssetCol, EAudioFXKnobSize::Secondary));

	UClass* ImpulseResponseClass = FindObject<UClass>(
		nullptr, TEXT("/Script/Synthesis.AudioImpulseResponse"));

	return SNew(SBox)
		.Padding(FMargin(FAudioFXTheme::WindowPadding, FAudioFXTheme::SectionPadding))
	[
		SNew(SVerticalBox)

		// ── IR waveform visualization (hero visual, top) ──────
		+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(0.f, 0.f, 0.f, 8.f)
		[
			SNew(SBox)
			.MinDesiredHeight(200.f)
			[
				SNew(SBorder)
				.BorderImage(&FAudioFXTheme::GetCardBrush())
				.Padding(FMargin(8.f, 8.f))
				[
					SNew(SAudioFXImpulseResponseCurve)
					.AssetColor(AssetCol)
					.ImpulseResponseAsset_Lambda([WeakPreset]() -> const UObject*
					{
						if (const auto* Preset = WeakPreset.Get())
						{
							return Preset->ImpulseResponse.Get();
						}
						return nullptr;
					})
				]
			]
		]

		// ── Knob row: Wet | Dry (spread apart) ───────────────
		+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.f, 0.f, 0.f, 8.f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(FAudioFXTheme::KnobHorizontalSpacing * 2.f, 0.f)
			[
				SNew(SAudioFXLabeledKnob)
				.ParamName(LOCTEXT("WetLbl", "Wet"))
				.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
					LOCTEXT("WetTipLbl", "Wet Volume"), -96.f, 0.f, TEXT("dB"),
					LOCTEXT("WetTip", "Controls how much of the wet signal is mixed into the output, in Decibels.")))
				.Value_Lambda([WeakPreset]() -> float
				{
					if (auto* Preset = WeakPreset.Get())
					{
						return FMath::GetMappedRangeValueClamped(
							FVector2D(-96.f, 0.f), FVector2D(0.f, 1.f),
							Preset->GetSettings().WetVolumeDb);
					}
					return 1.f;
				})
				.Style(WetStyle)
				.AudioUnitsValueType(EAudioUnitsValueType::Volume)
				.ValueFormat(EAudioFXValueFormat::Decibels)
				.MinValue(-96.f)
				.MaxValue(0.f)
				.OnValueChanged_Lambda([this, WeakPreset](float NewNorm)
				{
					if (auto* Preset = WeakPreset.Get())
					{
						MarkDirty.ExecuteIfBound();
						FSourceEffectConvolutionReverbSettings NS = Preset->GetSettings();
						NS.WetVolumeDb = FMath::GetMappedRangeValueClamped(
							FVector2D(0.f, 1.f), FVector2D(-96.f, 0.f), NewNorm);
						Preset->SetSettings(NS);
					}
				})
			]

			+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(FAudioFXTheme::KnobHorizontalSpacing * 2.f, 0.f)
			[
				SNew(SAudioFXLabeledKnob)
				.ParamName(LOCTEXT("DryLbl", "Dry"))
				.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
					LOCTEXT("DryTipLbl", "Dry Volume"), -96.f, 0.f, TEXT("dB"),
					LOCTEXT("DryTip", "Controls how much of the dry signal is mixed into the output, in Decibels.")))
				.Value_Lambda([WeakPreset]() -> float
				{
					if (auto* Preset = WeakPreset.Get())
					{
						return FMath::GetMappedRangeValueClamped(
							FVector2D(-96.f, 0.f), FVector2D(0.f, 1.f),
							Preset->GetSettings().DryVolumeDb);
					}
					return 1.f;
				})
				.Style(DryStyle)
				.AudioUnitsValueType(EAudioUnitsValueType::Volume)
				.ValueFormat(EAudioFXValueFormat::Decibels)
				.MinValue(-96.f)
				.MaxValue(0.f)
				.OnValueChanged_Lambda([this, WeakPreset](float NewNorm)
				{
					if (auto* Preset = WeakPreset.Get())
					{
						MarkDirty.ExecuteIfBound();
						FSourceEffectConvolutionReverbSettings NS = Preset->GetSettings();
						NS.DryVolumeDb = FMath::GetMappedRangeValueClamped(
							FVector2D(0.f, 1.f), FVector2D(-96.f, 0.f), NewNorm);
						Preset->SetSettings(NS);
					}
				})
			]
		]

		// ── Impulse Response asset picker (below knobs) ───────
		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 8.f)
		[
			SNew(SBorder)
			.BorderImage(&FAudioFXTheme::GetCardBrush())
			.Padding(FMargin(8.f, 6.f))
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.f, 0.f, 8.f, 0.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("IRAssetLbl", "Impulse Response"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
				]

				+ SHorizontalBox::Slot()
					.FillWidth(1.f)
				[
					SNew(SObjectPropertyEntryBox)
					.AllowedClass(ImpulseResponseClass ? ImpulseResponseClass : UObject::StaticClass())
					.ObjectPath_Lambda([WeakPreset]() -> FString
					{
						if (const auto* Preset = WeakPreset.Get())
						{
							const UObject* Current = Preset->ImpulseResponse.Get();
							return Current ? Current->GetPathName() : FString();
						}
						return FString();
					})
					.OnObjectChanged_Lambda([this, WeakPreset](const FAssetData& AssetData)
					{
						if (auto* Preset = WeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							Preset->SetImpulseResponse((UAudioImpulseResponse*)AssetData.GetAsset());
						}
					})
				]
			]
		]

		// ── Advanced section (collapsible) ────────────────────
		+ SVerticalBox::Slot()
			.AutoHeight()
		[
			SNew(SAudioFXCollapsibleSection)
			.SectionLabel(LOCTEXT("AdvancedSec", "ADVANCED"))
			.bDefaultExpanded(false)
			.OwnerEditor(SharedThis(this))
			.HeightOffset(60.f)
			[
				SNew(SBox)
				.Padding(FMargin(0.f, 12.f, 0.f, 0.f))  // breathing room above content
				[
					SNew(SHorizontalBox)

					// Push content to center
					+ SHorizontalBox::Slot().FillWidth(1.f)

					// Hardware Acceleration toggle
				+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.f, 0.f, 12.f, 0.f)
				[
					SNew(SAudioFXButton)
					.Options(TArray<FText>{ LOCTEXT("HWAccel", "HW Accel") })
					.AssetColor(AssetCol)
					.SelectedIndex(Preset->bEnableHardwareAcceleration ? 0 : -1)
					.OnSelectionChanged_Lambda([this, WeakPreset](int32 OptionIndex)
					{
						if (auto* Preset = WeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							Preset->bEnableHardwareAcceleration = (OptionIndex == 0);
						}
					})
				]

				// Block Size label
				+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("BlockSizeLbl", "Block Size:"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
				]

				// Block Size dropdown
				+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
				[
					SNew(SAudioFXDropdown)
					.Options(TArray<FText>{
						LOCTEXT("BS256", "256"),
						LOCTEXT("BS512", "512"),
						LOCTEXT("BS1024", "1024") })
					.AssetColor(AssetCol)
					.SelectedIndex((int32)Preset->BlockSize)
					.OnSelectionChanged_Lambda([this, WeakPreset](int32 OptionIndex)
					{
						if (auto* Preset = WeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							Preset->BlockSize = (ESubmixEffectConvolutionReverbBlockSize)OptionIndex;
						}
					})
				]

				// Push content to center (right spacer)
				+ SHorizontalBox::Slot().FillWidth(1.f)
				]
			]
		]
	];
}

#undef LOCTEXT_NAMESPACE
