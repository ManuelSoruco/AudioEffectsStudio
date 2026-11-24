// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "SourceEffects/AudioFXMotionFilterEditor.h"
#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Widgets/SAudioFXDropdown.h"
#include "Shared/Widgets/SAudioFXButton.h"
#include "Shared/Widgets/SAudioFXModulationOverlay.h"
#include "Shared/Widgets/SAudioFXSectionBox.h"
#include "SourceEffects/SourceEffectMotionFilter.h"
#include "Shared/Widgets/SAudioFXLabeledKnob.h"
#include "AudioWidgetsEnums.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AudioFXMotionFilter"

TSharedRef<SWidget> FAudioFXMotionFilterEditor::CreateEffectWidget(USoundEffectPreset* InPreset)
{
	USourceEffectMotionFilterPreset* Preset = Cast<USourceEffectMotionFilterPreset>(InPreset);
	if (!Preset) { UE_LOG(LogAudioEffectsStudio, Warning, TEXT("AudioEffectsStudio: Cast to preset type failed for '%s'. Check asset registration."), InPreset ? *InPreset->GetClass()->GetName() : TEXT("null")); return FAudioFXEffectEditor::CreateEffectWidget(InPreset); }

	const FLinearColor AssetCol = GetAssetColor();
	TWeakObjectPtr<USourceEffectMotionFilterPreset> WeakPreset = Preset;

	// ── Global knob styles — Mix AB = Hero (Blue), Dry = Secondary (Orange) ─
	const FAudioMaterialKnobStyle* MixStyle = StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Frequency, AssetCol, EAudioFXKnobSize::Hero));
	const FAudioMaterialKnobStyle* DryStyle = StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain, AssetCol, EAudioFXKnobSize::Secondary));

	TSharedPtr<SAudioFXLabeledKnob> MixKnob, DryKnob;
	const TSharedRef<bool> OverlayVisibleState = MakeShared<bool>(false);

	// ── Filter A & B sections ─────────────────────────────────────────
	auto MakeFilterSection = [&](const FText& Label, bool bIsA) -> TSharedRef<SWidget>
	{
		return BuildFilterSection(
			Label,
			bIsA ? Preset->Settings.FilterASettings : Preset->Settings.FilterBSettings,
			/* OnCircuit */ [WeakPreset, bIsA](ESourceEffectMotionFilterCircuit FilterCircuit){
				if(USourceEffectMotionFilterPreset* Preset = WeakPreset.Get()){Preset->Modify();
					(bIsA ? Preset->Settings.FilterASettings : Preset->Settings.FilterBSettings).FilterCircuit = FilterCircuit;
					Preset->SetSettings(Preset->Settings);}
			},
			/* OnType */    [WeakPreset, bIsA](ESourceEffectMotionFilterType FilterType){
				if(USourceEffectMotionFilterPreset* Preset = WeakPreset.Get()){Preset->Modify();
					(bIsA ? Preset->Settings.FilterASettings : Preset->Settings.FilterBSettings).FilterType = FilterType;
					Preset->SetSettings(Preset->Settings);}
			},
			/* GetCutoffNorm */ [WeakPreset, bIsA]() -> float {
				if (USourceEffectMotionFilterPreset* Preset = WeakPreset.Get())
				{
					const float Cutoff = (bIsA ? Preset->Settings.FilterASettings : Preset->Settings.FilterBSettings).CutoffFrequency;
					return FAudioFXTheme::LogFreqToNorm(Cutoff, 20.f, 12000.f);
				}
				return FAudioFXTheme::LogFreqToNorm(1000.f, 20.f, 12000.f);
			},
			/* GetQNorm */ [WeakPreset, bIsA]() -> float {
				if (USourceEffectMotionFilterPreset* Preset = WeakPreset.Get())
				{
					const float Q = (bIsA ? Preset->Settings.FilterASettings : Preset->Settings.FilterBSettings).FilterQ;
					return FMath::GetMappedRangeValueClamped(FVector2D(0.5f,10.f), FVector2D(0.f,1.f), Q);
				}
				return FMath::GetMappedRangeValueClamped(FVector2D(0.5f,10.f), FVector2D(0.f,1.f), 0.7f);
			},
			/* OnCutoff */  [WeakPreset, bIsA](float NewNorm){
				if(USourceEffectMotionFilterPreset* Preset = WeakPreset.Get()){Preset->Modify();
					(bIsA ? Preset->Settings.FilterASettings : Preset->Settings.FilterBSettings).CutoffFrequency = FAudioFXTheme::NormToLogFreq(NewNorm, 20.f, 12000.f);
					Preset->SetSettings(Preset->Settings);}
			},
			/* OnQ */       [WeakPreset, bIsA](float NewNorm){
				if(USourceEffectMotionFilterPreset* Preset = WeakPreset.Get()){Preset->Modify();
					(bIsA ? Preset->Settings.FilterASettings : Preset->Settings.FilterBSettings).FilterQ =
						FMath::GetMappedRangeValueClamped(FVector2D(0.f,1.f), FVector2D(0.5f,10.f), NewNorm);
					Preset->SetSettings(Preset->Settings);}
			},
			AssetCol,
			WeakPreset
		);
	};

	TSharedRef<SWidget> Widget =
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBox).Padding(FMargin(FAudioFXTheme::WindowPadding, FAudioFXTheme::SectionPadding))
			[
				SNew(SVerticalBox)

			// ── GLOBAL ──────────────────────────────────────────────
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
			[
				SNew(SAudioFXSectionBox).SectionLabel(LOCTEXT("GlobalSec","GLOBAL"))
				[
					SNew(SVerticalBox)

					// ── Row 1: Topology | Mix A/B | Dry ──────────────
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)

					// ── LEFT: Topology + Mod mappings stacked ────────────
					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					.Padding(4.f, 0.f)
					[
						SNew(SVerticalBox)

						// Topology row
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f,0.f,6.f,0.f)
							[ SNew(STextBlock).Text(LOCTEXT("TopoL","Topology:"))
							  .Font(FCoreStyle::GetDefaultFontStyle("Regular",9))
							  .ColorAndOpacity(FAudioFXTheme::GetLabelTextColor()) ]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								SNew(SAudioFXDropdown)
								.Options(TArray<FText>{ LOCTEXT("Serial","Serial"), LOCTEXT("Parallel","Parallel") })
								.AssetColor(AssetCol)
								.SelectedIndex((int32)Preset->Settings.MotionFilterTopology)
								.OnSelectionChanged_Lambda([this, WeakPreset](int32 OptionIndex){
									if(USourceEffectMotionFilterPreset* Preset = WeakPreset.Get()){Preset->Modify();
										Preset->Settings.MotionFilterTopology=(ESourceEffectMotionFilterTopology)OptionIndex;
										Preset->SetSettings(Preset->Settings);}
								})
							]
						]

						// Modulation count text
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
						[
							SNew(STextBlock)
							.Text_Lambda([WeakPreset]()
							{
								if (const USourceEffectMotionFilterPreset* P = WeakPreset.Get())
									return FText::Format(LOCTEXT("ModSummary", "{0} modulation mapping(s)"), FText::AsNumber(P->Settings.ModulationMappings.Num()));
								return LOCTEXT("ModInvalid", "Invalid preset");
							})
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
							.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
						]

						// Edit Mappings button — never stays highlighted
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left)
						[
							SNew(SAudioFXButton)
							.Options(TArray<FText>{ LOCTEXT("EditMappings", "Edit Mappings") })
							.AssetColor(AssetCol)
							.SelectedIndex(TAttribute<int32>::CreateLambda([]() { return -1; }))
							.OnSelectionChanged_Lambda([OverlayVisibleState](int32)
							{
								*OverlayVisibleState = true;
							})
						]
					]

					// ── CENTER: Mix A/B hero knob ─────────────────────
					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SAssignNew(MixKnob, SAudioFXLabeledKnob)
						.ParamName(LOCTEXT("MixLbl","Mix A/B"))
						.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
							LOCTEXT("MixTipLbl", "Filter Mix"),
							-1.f, 1.f, TEXT(""),
							LOCTEXT("MixTip",
								"Controls the amount of each filter in the signal where -1.0 outputs only Filter A, "
								"0.0 is an equal balance, and 1.0 outputs only Filter B.\n"
								"AudioFactory Note: The blend uses a sine/cosine equal-power crossfade; "
								"behavior depends on the Topology (Serial vs Parallel).")))
						.Value_Lambda([WeakPreset]() -> float { if (USourceEffectMotionFilterPreset* Preset = WeakPreset.Get()) { return FMath::GetMappedRangeValueClamped(FVector2D(-1.f,1.f), FVector2D(0.f,1.f), Preset->Settings.MotionFilterMix); } return 0.5f; })
						.Style(MixStyle)
						.AudioUnitsValueType(EAudioUnitsValueType::Linear)
						.DefaultValue(0.5f).MinValue(-1.f).MaxValue(1.f)
						.OnValueChanged_Lambda([this, WeakPreset](float NewNorm){
							if(USourceEffectMotionFilterPreset* Preset = WeakPreset.Get()){Preset->Modify();
								Preset->Settings.MotionFilterMix=FMath::GetMappedRangeValueClamped(FVector2D(0.f,1.f),FVector2D(-1.f,1.f),NewNorm);
								Preset->SetSettings(Preset->Settings);}
						})
					]

					// ── RIGHT: Dry knob ───────────────────────────────
					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
					[
						SAssignNew(DryKnob, SAudioFXLabeledKnob)
						.ParamName(LOCTEXT("DryLbl","Dry"))
						.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
							LOCTEXT("DryTipLbl", "Dry Volume"),
							-96.f, 10.f, TEXT("dB"),
							LOCTEXT("DryTip",
								"Dry volume pass-through in dB.\n"
								"AudioFactory Note: The linear gain is clamped to prevent "
								"excessive output levels.")))
						.Value_Lambda([WeakPreset]() -> float { if (USourceEffectMotionFilterPreset* Preset = WeakPreset.Get()) { return FMath::GetMappedRangeValueClamped(FVector2D(-96.f,10.f), FVector2D(0.f,1.f), Preset->Settings.DryVolumeDb); } return FMath::GetMappedRangeValueClamped(FVector2D(-96.f,10.f), FVector2D(0.f,1.f), 0.f); })
						.Style(DryStyle)
						.AudioUnitsValueType(EAudioUnitsValueType::Volume)
						.ValueFormat(EAudioFXValueFormat::Decibels)
						.DefaultValue(FMath::GetMappedRangeValueClamped(FVector2D(-96.f,10.f),FVector2D(0.f,1.f),0.f))
						.MinValue(-96.f).MaxValue(10.f)
				.OnValueChanged_Lambda([this, WeakPreset](float NewNorm){
							if(USourceEffectMotionFilterPreset* Preset = WeakPreset.Get()){Preset->Modify();
								Preset->Settings.DryVolumeDb=FMath::GetMappedRangeValueClamped(FVector2D(0.f,1.f),FVector2D(-96.f,10.f),NewNorm);
								Preset->SetSettings(Preset->Settings);}
						})
					]
				]  // end Row 1 SHorizontalBox

				]  // end SVerticalBox inside GLOBAL section
			]

		// ── FILTER A + B side by side ────────────────────────────────
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 2.f, 0.f)
				[ MakeFilterSection(LOCTEXT("FilterASec","FILTER A"), true) ]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f, 0.f, 0.f, 0.f)
				[ MakeFilterSection(LOCTEXT("FilterBSec","FILTER B"), false) ]
			]
		]
	]
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.Visibility_Lambda([OverlayVisibleState]()
			{
				return *OverlayVisibleState ? EVisibility::Visible : EVisibility::Collapsed;
			})
			.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
			[
				SNew(SAudioFXModulationOverlay)
				.Preset(WeakPreset)
				.AssetColor(AssetCol)
				.OnCloseRequested_Lambda([OverlayVisibleState]()
				{
					*OverlayVisibleState = false;
				})
			]
		];


	return Widget;
}

TSharedRef<SWidget> FAudioFXMotionFilterEditor::BuildFilterSection(
	const FText& SectionLabel,
	const FSourceEffectIndividualFilterSettings& Settings,
	TFunction<void(ESourceEffectMotionFilterCircuit)> OnCircuit,
	TFunction<void(ESourceEffectMotionFilterType)>    OnType,
	TFunction<float()>                                GetCutoffNorm,
	TFunction<float()>                                GetQNorm,
	TFunction<void(float)>                            OnCutoff,
	TFunction<void(float)>                            OnQ,
	const FLinearColor& AssetCol,
	TWeakObjectPtr<UObject> InEditedObject)
{
	// Visual Redesign: Cutoff=Primary, Q=Secondary (compact for side-by-side layout)
	const FAudioMaterialKnobStyle* FreqStyle = StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Frequency, AssetCol, EAudioFXKnobSize::Primary));
	const FAudioMaterialKnobStyle* QStyle = StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Resonance, AssetCol, EAudioFXKnobSize::Secondary));

	TArray<FText> CircuitOpts = { LOCTEXT("1Pole","1-Pole"), LOCTEXT("StateVar","SV"), LOCTEXT("Ladder","Ladder") };
	TArray<FText> TypeOpts    = { LOCTEXT("LP","Low-Pass"), LOCTEXT("HP","High-Pass"), LOCTEXT("BP","Band-Pass"), LOCTEXT("BS","Band-Stop") };

	TSharedPtr<SAudioFXLabeledKnob> FreqKnob, QKnob;

	TSharedRef<SWidget> Section =
		SNew(SAudioFXSectionBox).SectionLabel(SectionLabel)
		[
			SNew(SHorizontalBox)

			// Circuit + Type strips (vertical stack)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			.Padding(0.f, 0.f, FAudioFXTheme::SectionGap, 0.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f,0.f,0.f,6.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f,0.f,8.f,0.f)
					[
						SNew(SBox).MinDesiredWidth(52.f)
						[
							SNew(STextBlock).Text(LOCTEXT("CircuitL","Circuit:"))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular",9))
							.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
						]
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SAudioFXDropdown).Options(CircuitOpts).AssetColor(AssetCol)
						.SelectedIndex((int32)Settings.FilterCircuit)
						.OnSelectionChanged_Lambda([OnCircuit](int32 OptionIndex){
							OnCircuit((ESourceEffectMotionFilterCircuit)OptionIndex);
						})
					]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f,0.f,8.f,0.f)
					[
						SNew(SBox).MinDesiredWidth(52.f)
						[
							SNew(STextBlock).Text(LOCTEXT("TypeL","Type:"))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular",9))
							.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
						]
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SAudioFXDropdown).Options(TypeOpts).AssetColor(AssetCol)
						.SelectedIndex((int32)Settings.FilterType)
						.OnSelectionChanged_Lambda([OnType](int32 OptionIndex){
							OnType((ESourceEffectMotionFilterType)OptionIndex);
						})
					]
				]
			]

			// Vertical divider
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Fill).Padding(FAudioFXTheme::SectionGap, 4.f)
			[ FAudioFXTheme::MakeVerticalDivider() ]

			// Cutoff (Hero) + Q (Standard) knobs — FillWidth distributes evenly
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
				.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
				[
					SAssignNew(FreqKnob, SAudioFXLabeledKnob)
					.ParamName(LOCTEXT("CutoffLbl","Cutoff"))
					.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
						LOCTEXT("CutoffTipLbl", "Cutoff Frequency"),
						20.f, 12000.f, TEXT("Hz"),
						LOCTEXT("CutoffTip",
							"The filter cutoff frequency.\n"
							"AudioFactory Note: Can be further modulated by the motion source mappings.")))
					.Value_Lambda([GetCutoffNorm]() -> float { return GetCutoffNorm ? GetCutoffNorm() : FAudioFXTheme::LogFreqToNorm(1000.f, 20.f, 12000.f); }).Style(FreqStyle)
					.AudioUnitsValueType(EAudioUnitsValueType::Frequency)
					.ValueFormat(EAudioFXValueFormat::Frequency)
					.DefaultValue(FAudioFXTheme::LogFreqToNorm(1000.f, 20.f, 12000.f)).MinValue(20.f).MaxValue(12000.f)
					.EditedObject(InEditedObject)
				.OnValueChanged_Lambda([OnCutoff](float NewNorm){ OnCutoff(NewNorm); })
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
				.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
				[
					SAssignNew(QKnob, SAudioFXLabeledKnob)
					.ParamName(LOCTEXT("QLbl","Resonance"))
					.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
						LOCTEXT("ResTipLbl", "Resonance (Q)"),
						0.5f, 10.f, TEXT(""),
						LOCTEXT("ResTip",
							"The filter resonance.\n"
							"AudioFactory Note: Resonance is clamped to 0.5–10.0 and "
							"can be modulated by motion source mappings.")))
					.Value_Lambda([GetQNorm]() -> float { return GetQNorm ? GetQNorm() : FMath::GetMappedRangeValueClamped(FVector2D(0.5f,10.f), FVector2D(0.f,1.f), 0.7f); }).Style(QStyle)
					.AudioUnitsValueType(EAudioUnitsValueType::Linear)
					.ValueFormat(EAudioFXValueFormat::QFactor)
					.DefaultValue(FMath::GetMappedRangeValueClamped(FVector2D(0.5f,10.f),FVector2D(0.f,1.f),0.7f))
					.MinValue(0.5f).MaxValue(10.f)
					.EditedObject(InEditedObject)
				.OnValueChanged_Lambda([OnQ](float NewNorm){ OnQ(NewNorm); })
				]
			]
		];


	return Section;
}


#undef LOCTEXT_NAMESPACE
