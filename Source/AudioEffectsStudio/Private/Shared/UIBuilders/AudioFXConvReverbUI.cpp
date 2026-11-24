// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/UIBuilders/AudioFXConvReverbUI.h"
#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Visualizers/SAudioFXImpulseResponseCurve.h"
#include "Shared/Widgets/SAudioFXButton.h"
#include "Shared/Widgets/SAudioFXDropdown.h"
#include "Shared/Widgets/SAudioFXCollapsibleSection.h"
#include "Shared/Widgets/SAudioFXLabeledKnob.h"
#include "PropertyCustomizationHelpers.h"
#include "AudioWidgetsEnums.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "AssetRegistry/AssetData.h"
#include "EffectConvolutionReverb.h"

#define LOCTEXT_NAMESPACE "AudioFXConvReverbUI"

// ============================================================
// Static helpers — called by BuildWidget below
// ============================================================

/** Impulse response waveform visualization card. */
static TSharedRef<SWidget> BuildIRVisualizationPanel(
	TWeakObjectPtr<USubmixEffectConvolutionReverbPreset> WeakPreset,
	const FLinearColor& AssetCol)
{
	return SNew(SBox)
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
						return Preset->ImpulseResponse.Get();
					return nullptr;
				})
			]
		];
}

/** Wet and Dry mix knobs displayed side by side. */
static TSharedRef<SWidget> BuildWetDryKnobRow(
	TWeakObjectPtr<USubmixEffectConvolutionReverbPreset> WeakPreset,
	const FAudioMaterialKnobStyle* WetStyle,
	const FAudioMaterialKnobStyle* DryStyle,
	FSimpleDelegate MarkDirty)
{
	return SNew(SHorizontalBox)

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
				if (const auto* Preset = WeakPreset.Get())
					return FMath::GetMappedRangeValueClamped(FVector2D(-96.f, 0.f), FVector2D(0.f, 1.f), Preset->Settings.WetVolumeDb);
				return 1.f;
			})
			.Style(WetStyle)
			.AudioUnitsValueType(EAudioUnitsValueType::Volume)
			.ValueFormat(EAudioFXValueFormat::Decibels)
			.MinValue(-96.f).MaxValue(0.f)
			.OnValueChanged_Lambda([WeakPreset, MarkDirty](float NewNorm)
			{
				if (auto* Preset = WeakPreset.Get())
				{
					MarkDirty.ExecuteIfBound();
					Preset->Settings.WetVolumeDb = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 1.f), FVector2D(-96.f, 0.f), NewNorm);
					Preset->SetSettings(Preset->Settings);
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
				if (const auto* Preset = WeakPreset.Get())
					return FMath::GetMappedRangeValueClamped(FVector2D(-96.f, 0.f), FVector2D(0.f, 1.f), Preset->Settings.DryVolumeDb);
				return 1.f;
			})
			.Style(DryStyle)
			.AudioUnitsValueType(EAudioUnitsValueType::Volume)
			.ValueFormat(EAudioFXValueFormat::Decibels)
			.MinValue(-96.f).MaxValue(0.f)
			.OnValueChanged_Lambda([WeakPreset, MarkDirty](float NewNorm)
			{
				if (auto* Preset = WeakPreset.Get())
				{
					MarkDirty.ExecuteIfBound();
					Preset->Settings.DryVolumeDb = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 1.f), FVector2D(-96.f, 0.f), NewNorm);
					Preset->SetSettings(Preset->Settings);
				}
			})
		];
}

/** Inline asset picker for the UAudioImpulseResponse reference. */
static TSharedRef<SWidget> BuildIRAssetPicker(
	TWeakObjectPtr<USubmixEffectConvolutionReverbPreset> WeakPreset,
	UClass* ImpulseResponseClass,
	FSimpleDelegate MarkDirty)
{
	return SNew(SBorder)
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
				.OnObjectChanged_Lambda([WeakPreset, MarkDirty](const FAssetData& AssetData)
				{
					if (auto* Preset = WeakPreset.Get())
					{
						MarkDirty.ExecuteIfBound();
						UAudioImpulseResponse* IR = Cast<UAudioImpulseResponse>(AssetData.GetAsset());
						Preset->SetImpulseResponse(IR);
					}
				})
			]
		];
}

/** Collapsible ADVANCED section: HW acceleration toggle + block size dropdown. */
static TSharedRef<SWidget> BuildConvReverbAdvancedSection(
	USubmixEffectConvolutionReverbPreset* Preset,
	TWeakObjectPtr<USubmixEffectConvolutionReverbPreset> WeakPreset,
	const FLinearColor& AssetCol,
	FSimpleDelegate MarkDirty,
	FAudioFXEffectEditor& Editor)
{
	return SNew(SAudioFXCollapsibleSection)
		.SectionLabel(LOCTEXT("AdvancedSec", "ADVANCED"))
		.bDefaultExpanded(false)
		.OwnerEditor(Editor.AsShared())
		.HeightOffset(60.f)
		[
			SNew(SBox)
			.Padding(FMargin(0.f, 12.f, 0.f, 0.f))
			[
				SNew(SHorizontalBox)

				// Left spacer
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
					.OnSelectionChanged_Lambda([WeakPreset, MarkDirty](int32 OptionIndex)
					{
						if (auto* ReverbPreset = WeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							ReverbPreset->bEnableHardwareAcceleration = (OptionIndex == 0);
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
						LOCTEXT("BS256",  "256"),
						LOCTEXT("BS512",  "512"),
						LOCTEXT("BS1024", "1024") })
					.AssetColor(AssetCol)
					.SelectedIndex((int32)Preset->BlockSize)
					.OnSelectionChanged_Lambda([WeakPreset, MarkDirty](int32 OptionIndex)
					{
						if (auto* ReverbPreset = WeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							ReverbPreset->BlockSize = (ESubmixEffectConvolutionReverbBlockSize)OptionIndex;
						}
					})
				]

				// Right spacer
				+ SHorizontalBox::Slot().FillWidth(1.f)
			]
		];
}

// ============================================================
// Public: BuildWidget
// ============================================================

TSharedRef<SWidget> AudioFXConvReverbUI::BuildWidget(
	FAudioFXEffectEditor& Editor,
	USubmixEffectConvolutionReverbPreset* Preset,
	const FLinearColor& AssetCol)
{
	if (!Preset) { return SNullWidget::NullWidget; }

	TWeakObjectPtr<USubmixEffectConvolutionReverbPreset> WeakPreset = Preset;
	TWeakPtr<FAudioFXEffectEditor> WeakEditor = Editor.AsShared();
	FSimpleDelegate MarkDirty = FSimpleDelegate::CreateLambda([WeakEditor]{ if (auto Ed = WeakEditor.Pin()) { Ed->MarkPresetDirty(); } });

	const FAudioMaterialKnobStyle* WetStyle = Editor.StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain, AssetCol, EAudioFXKnobSize::Secondary));
	const FAudioMaterialKnobStyle* DryStyle = Editor.StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain, AssetCol, EAudioFXKnobSize::Secondary));

	UClass* ImpulseResponseClass = FindObject<UClass>(nullptr, TEXT("/Script/Synthesis.AudioImpulseResponse"));

	return SNew(SBox)
		.Padding(FMargin(FAudioFXTheme::WindowPadding, FAudioFXTheme::SectionPadding))
		[
			SNew(SVerticalBox)

			// ── IR waveform visualization (hero visual, top) ──────
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				BuildIRVisualizationPanel(WeakPreset, AssetCol)
			]

			// ── Knob row: Wet | Dry ───────────────────────────────
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				BuildWetDryKnobRow(WeakPreset, WetStyle, DryStyle, MarkDirty)
			]

			// ── Impulse Response asset picker ─────────────────────
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				BuildIRAssetPicker(WeakPreset, ImpulseResponseClass, MarkDirty)
			]

			// ── Advanced section (collapsible) ────────────────────
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				BuildConvReverbAdvancedSection(Preset, WeakPreset, AssetCol, MarkDirty, Editor)
			]
		];
}

#undef LOCTEXT_NAMESPACE
