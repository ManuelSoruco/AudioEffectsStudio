// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "SourceEffects/AudioFXSimpleDelayEditor.h"
#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Widgets/SAudioFXDropdown.h"
#include "Shared/Widgets/SAudioFXButton.h"
#include "Shared/Visualizers/SAudioFXReverbDecayCurve.h"
#include "Shared/Widgets/SAudioFXLabeledKnob.h"
#include "SourceEffects/SourceEffectSimpleDelay.h"
#include "AudioWidgetsEnums.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AudioFXSimpleDelay"

TSharedRef<SWidget> FAudioFXSimpleDelayEditor::CreateEffectWidget(USoundEffectPreset* InPreset)
{
	USourceEffectSimpleDelayPreset* Preset = Cast<USourceEffectSimpleDelayPreset>(InPreset);
	if (!Preset) { UE_LOG(LogAudioEffectsStudio, Warning, TEXT("AudioEffectsStudio: Cast to preset type failed for '%s'. Check asset registration."), InPreset ? *InPreset->GetClass()->GetName() : TEXT("null")); return FAudioFXEffectEditor::CreateEffectWidget(InPreset); }
	WeakPreset = Preset;
	TSharedRef<SBox> Root = SNew(SBox);
	RootBox = Root;
	RebuildWidget();
	return Root;
}

void FAudioFXSimpleDelayEditor::RebuildWidget()
{
	if (!RootBox.IsValid()) return;
	USourceEffectSimpleDelayPreset* Preset = WeakPreset.Get();
	if (!Preset) return;

	HeroKnob.Reset(); DryKnob.Reset(); WetKnob.Reset(); FeedbackKnob.Reset();

	const FLinearColor AssetCol = GetAssetColor();
	TWeakObjectPtr<USourceEffectSimpleDelayPreset> LocalWeakPreset = WeakPreset;
	const bool bDistMode = Preset->Settings.bDelayBasedOnDistance;
	FSimpleDelegate MarkDirtyLocal = MarkDirty;

	// ── Knob styles: Feedback=Hero Blue, Delay=Green Primary, Wet/Dry=Orange ──
	const FAudioMaterialKnobStyle* DryStyle  = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(
		EAudioFXParamColor::Gain, AssetCol, EAudioFXKnobSize::Secondary));
	const FAudioMaterialKnobStyle* WetStyle  = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(
		EAudioFXParamColor::Gain, AssetCol, EAudioFXKnobSize::Secondary));
	const FAudioMaterialKnobStyle* FbStyle   = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(
		EAudioFXParamColor::Frequency, AssetCol, EAudioFXKnobSize::Hero));  // Feedback hero
	const FAudioMaterialKnobStyle* DelayStyle = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(
		EAudioFXParamColor::Mix, AssetCol, EAudioFXKnobSize::Primary));      // Delay Time = Green

	const FText HeroLabel = bDistMode ? LOCTEXT("SpeedLbl","Speed (m/s)") : LOCTEXT("DelayLbl","Delay (s)");
	const float HeroMin   = bDistMode ? 1.f  : 0.f;
	const float HeroMax   = bDistMode ? 1000.f : 2.f;
	const float HeroDef   = bDistMode ? FMath::GetMappedRangeValueClamped(FVector2D(1.f,1000.f),FVector2D(0.f,1.f),343.f)
	                                  : FMath::GetMappedRangeValueClamped(FVector2D(0.f,2.f),FVector2D(0.f,1.f),0.5f);

	// ── Hero knob (Delay s or Speed m/s) ─────────────────────────────
	const FText HeroTip = bDistMode
		? FAudioFXTheme::MakeKnobTooltip(
			LOCTEXT("SpeedTipLbl","Speed of Sound"), 1.f, 1000.f, TEXT("m/s"),
			LOCTEXT("SpeedTip",
				"Speed of sound in meters per second when using distance-based delay.\n"
				"AudioFactory Note: Default 343 m/s matches real air at ~20 °C. Lower values increase perceived distance."))
		: FAudioFXTheme::MakeKnobTooltip(
			LOCTEXT("DelayTipLbl","Delay Time"), 0.f, 2.f, TEXT("s"),
			LOCTEXT("DelayTip",
				"Delay amount in seconds."));

	TSharedRef<SAudioFXLabeledKnob> HeroKnobRef = SAssignNew(HeroKnob, SAudioFXLabeledKnob)
		.ParamName(HeroLabel).KnobToolTipText(HeroTip)
		.Value_Lambda([LocalWeakPreset, bDistMode, HeroMin, HeroMax]() -> float {
			if (USourceEffectSimpleDelayPreset* Preset = LocalWeakPreset.Get())
			{
				const float Real = bDistMode ? Preset->Settings.SpeedOfSound : Preset->Settings.DelayAmount;
				return FMath::GetMappedRangeValueClamped(FVector2D(HeroMin, HeroMax), FVector2D(0.f, 1.f), Real);
			}
			return 0.5f;
		})
		.Style(DelayStyle).AudioUnitsValueType(EAudioUnitsValueType::Linear)
		.ValueFormat(bDistMode ? EAudioFXValueFormat::Default : EAudioFXValueFormat::Time)
		.DefaultValue(HeroDef).MinValue(HeroMin).MaxValue(HeroMax)
		.OnValueChanged_Lambda([LocalWeakPreset, bDistMode, HeroMin, HeroMax, MarkDirtyLocal](float NewNorm) {
		if (USourceEffectSimpleDelayPreset* Preset = LocalWeakPreset.Get()) {
			MarkDirtyLocal.ExecuteIfBound();
				if (bDistMode) Preset->Settings.SpeedOfSound = FMath::GetMappedRangeValueClamped(FVector2D(0.f,1.f),FVector2D(HeroMin,HeroMax),NewNorm);
				else           Preset->Settings.DelayAmount  = FMath::GetMappedRangeValueClamped(FVector2D(0.f,1.f),FVector2D(HeroMin,HeroMax),NewNorm);
				Preset->SetSettings(Preset->Settings);
			}
		});

	// ── Dry knob ──────────────────────────────────────────────────────
	TSharedRef<SAudioFXLabeledKnob> DryKnobRef = SAssignNew(DryKnob, SAudioFXLabeledKnob)
		.ParamName(LOCTEXT("DryLbl","Dry"))
		.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
			LOCTEXT("DryTipLbl","Dry"), 0.f, 1.f, TEXT(""),
			LOCTEXT("DryTip","Gain stage on dry (non-delayed) signal.")))
		.Value_Lambda([LocalWeakPreset]() -> float { if (USourceEffectSimpleDelayPreset* Preset = LocalWeakPreset.Get()) { return Preset->Settings.DryAmount; } return 1.f; }).Style(DryStyle).AudioUnitsValueType(EAudioUnitsValueType::Volume)
		.DefaultValue(1.f).MinValue(0.f).MaxValue(1.f)
				.OnValueChanged_Lambda([this, LocalWeakPreset](float NewNorm){ if(USourceEffectSimpleDelayPreset* Preset = LocalWeakPreset.Get()){Preset->Modify();Preset->Settings.DryAmount=NewNorm;Preset->SetSettings(Preset->Settings);} });

	// ── Wet knob ──────────────────────────────────────────────────────
	TSharedRef<SAudioFXLabeledKnob> WetKnobRef = SAssignNew(WetKnob, SAudioFXLabeledKnob)
		.ParamName(LOCTEXT("WetLbl","Wet"))
		.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
			LOCTEXT("WetTipLbl","Wet"), 0.f, 1.f, TEXT(""),
			LOCTEXT("WetTip","Gain stage on wet (delayed) signal.")))
		.Value_Lambda([LocalWeakPreset]() -> float { if (USourceEffectSimpleDelayPreset* Preset = LocalWeakPreset.Get()) { return Preset->Settings.WetAmount; } return 0.5f; }).Style(WetStyle).AudioUnitsValueType(EAudioUnitsValueType::Volume)
		.DefaultValue(0.5f).MinValue(0.f).MaxValue(1.f)
				.OnValueChanged_Lambda([this, LocalWeakPreset](float NewNorm){ if(USourceEffectSimpleDelayPreset* Preset = LocalWeakPreset.Get()){Preset->Modify();Preset->Settings.WetAmount=NewNorm;Preset->SetSettings(Preset->Settings);} });

	// ── Feedback knob ─────────────────────────────────────────────────
	TSharedRef<SAudioFXLabeledKnob> FbKnobRef = SAssignNew(FeedbackKnob, SAudioFXLabeledKnob)
		.ParamName(LOCTEXT("FbLbl","Feedback"))
		.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
			LOCTEXT("FbTipLbl","Feedback"), 0.f, 1.f, TEXT(""),
			LOCTEXT("FbTip",
				"Amount to feed back into the delay line.\n"
				"AudioFactory Note: Values near 1.0 create runaway feedback; keep below 0.9 for safe use.")))
		.Value_Lambda([LocalWeakPreset]() -> float { if (USourceEffectSimpleDelayPreset* Preset = LocalWeakPreset.Get()) { return Preset->Settings.Feedback; } return 0.3f; }).Style(FbStyle).AudioUnitsValueType(EAudioUnitsValueType::Linear)
		.DefaultValue(0.3f).MinValue(0.f).MaxValue(1.f)
				.OnValueChanged_Lambda([this, LocalWeakPreset](float NewNorm){ if(USourceEffectSimpleDelayPreset* Preset = LocalWeakPreset.Get()){Preset->Modify();Preset->Settings.Feedback=NewNorm;Preset->SetSettings(Preset->Settings);} });

	// ── Build inner — Graph top, knobs middle, options bottom ────────────
	TSharedRef<SVerticalBox> Inner = SNew(SVerticalBox)

		// ── Delay echo visualization (graph, fill-width, gets extra 100px) ──
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 0.f, 0.f, 8.f)
		[
			SNew(SBox)
			.MinDesiredHeight(200.f)
			[
				SNew(SAudioFXReverbDecayCurve)
				.bDelayMode(true)
				.DecayTime_Lambda([LocalWeakPreset, bDistMode]()
				{
					if (const USourceEffectSimpleDelayPreset* Preset = LocalWeakPreset.Get())
					{
						return bDistMode ? 0.5f : Preset->Settings.DelayAmount;
					}
					return 0.5f;
				})
				.DelayFeedback_Lambda([LocalWeakPreset]()
				{
					if (const USourceEffectSimpleDelayPreset* Preset = LocalWeakPreset.Get())
					{
						return Preset->Settings.Feedback;
					}
					return 0.3f;
				})
				.AssetColor(AssetCol)
			]
		]

		// ── Knob row: [Delay] [Feedback HERO center] [Wet over Dry] ──────────────
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
		[
			SNew(SHorizontalBox)

			// Delay (Speed/Time) — left, Primary
			+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
			.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
			[ HeroKnobRef ]

			// Feedback — hero, center (larger fill to visually dominate)
			+ SHorizontalBox::Slot().FillWidth(1.5f).HAlign(HAlign_Center).VAlign(VAlign_Center)
			.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
			[ FbKnobRef ]

			// Wet (top) + Dry (bottom) stacked on the right
			+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
			.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 4.f)
				[ WetKnobRef ]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[ DryKnobRef ]
			]
		]

		// ── Options row (below knobs): [Distance ▼] [Dist. Override] ────
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(SHorizontalBox)

			// Mode dropdown
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(SAudioFXDropdown)
				.Options(TArray<FText>{ LOCTEXT("Dist","Distance"), LOCTEXT("Manual","Manual") })
				.AssetColor(AssetCol)
				.SelectedIndex(bDistMode ? 0 : 1)
				.OnSelectionChanged_Lambda([this, LocalWeakPreset](int32 OptionIndex){
					if(USourceEffectSimpleDelayPreset* Preset = LocalWeakPreset.Get()){
						MarkDirty.ExecuteIfBound();
						Preset->Settings.bDelayBasedOnDistance = (OptionIndex == 0);
						Preset->SetSettings(Preset->Settings);
						RebuildWidget();
					}
				})
			]

			// Conditional "Distance Override" toggle (only in Distance mode)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).Visibility(bDistMode ? EVisibility::Visible : EVisibility::Collapsed)
				[
					SNew(SAudioFXButton)
					.Options(TArray<FText>{ LOCTEXT("DistOvr","Dist. Override") })
					.AssetColor(AssetCol)
					.SelectedIndex(Preset->Settings.bUseDistanceOverride ? 0 : -1)
					.OnSelectionChanged_Lambda([this, LocalWeakPreset](int32 OptionIndex){
						if(USourceEffectSimpleDelayPreset* Preset = LocalWeakPreset.Get()){MarkDirty.ExecuteIfBound();Preset->Settings.bUseDistanceOverride=(OptionIndex==0);Preset->SetSettings(Preset->Settings);}
					})
				]
			]
		];

	RootBox->SetContent(
		SNew(SBox).Padding(FMargin(FAudioFXTheme::WindowPadding, FAudioFXTheme::SectionPadding))
		[ Inner ]
	);
}

#undef LOCTEXT_NAMESPACE
