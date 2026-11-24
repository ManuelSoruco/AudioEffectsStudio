// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

/**
 * FAudioFXSubmixReverbEditor
 *
 * Physical-model reverb editor for USubmixEffectReverbPreset.
 * Layout (horizontal split):
 *
 *   LEFT  — SAudioFXReverbDecayCurve (top, fills height)
 *           REVERB CORE section: Pre-Delay · Decay (hero) · Diffusion · HF Damping · Density
 *           ADVANCED collapsible: Late Delay · Decay HF · Refl. Gain · Late Gain · HF Gain
 *
 *   RIGHT — OUTPUT section: Gain · Wet · Dry (stacked vertically)
 *
 * Bypass state (bBypass) is forwarded through GetBypassState / SetBypassState
 * so the PresetBar's bypass button drives it.
 */

#include "SubmixEffects/AudioFXSubmixReverbEditor.h"

#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Widgets/SAudioFXCollapsibleSection.h"
#include "Shared/Widgets/SAudioFXLabeledKnob.h"
#include "Shared/Visualizers/SAudioFXReverbDecayCurve.h"
#include "Shared/Widgets/SAudioFXSectionBox.h"
#include "SubmixEffects/AudioMixerSubmixEffectReverb.h"

#include "AudioWidgetsEnums.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"

#define LOCTEXT_NAMESPACE "AudioFXSubmixReverb"

// ── Bypass passthrough ───────────────────────────────────────────────────────

bool FAudioFXSubmixReverbEditor::GetBypassState() const
{
	if (const USubmixEffectReverbPreset* Preset = Cast<USubmixEffectReverbPreset>(GetPreset()))
	{
		return Preset->Settings.bBypass;
	}
	return false;
}

void FAudioFXSubmixReverbEditor::SetBypassState(bool bBypassed)
{
	if (USubmixEffectReverbPreset* Preset = Cast<USubmixEffectReverbPreset>(GetPreset()))
	{
		Preset->Settings.bBypass = bBypassed;
		Preset->SetSettings(Preset->Settings);
	}
}

// ── Widget construction ──────────────────────────────────────────────────────

TSharedRef<SWidget> FAudioFXSubmixReverbEditor::CreateEffectWidget(USoundEffectPreset* InPreset)
{
	USubmixEffectReverbPreset* Preset = Cast<USubmixEffectReverbPreset>(InPreset);
	if (!Preset)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("AudioEffectsStudio: Cast to preset type failed for '%s'. Check asset registration."),
			InPreset ? *InPreset->GetClass()->GetName() : TEXT("null"));
		return FAudioFXEffectEditor::CreateEffectWidget(InPreset);
	}

	const FLinearColor AssetCol = GetAssetColor();
	TWeakObjectPtr<USubmixEffectReverbPreset> LocalWeakPreset = Preset;

	// ── Knob styles (grouped by visual role) ─────────────────────────
	// Core knobs
	const FAudioMaterialKnobStyle* ReflDelayStyle = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Mix,       AssetCol, EAudioFXKnobSize::Secondary));
	const FAudioMaterialKnobStyle* DensityStyle   = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Mix,       AssetCol, EAudioFXKnobSize::Secondary));
	const FAudioMaterialKnobStyle* DecayStyle     = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Frequency, AssetCol, EAudioFXKnobSize::Hero));
	const FAudioMaterialKnobStyle* DiffStyle      = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Time,      AssetCol, EAudioFXKnobSize::Secondary));
	const FAudioMaterialKnobStyle* AirAbsStyle    = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Time,      AssetCol, EAudioFXKnobSize::Secondary));
	// Advanced knobs
	const FAudioMaterialKnobStyle* LateDelayStyle = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Time,      AssetCol, EAudioFXKnobSize::Tertiary));
	const FAudioMaterialKnobStyle* DecayHFStyle   = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Frequency, AssetCol, EAudioFXKnobSize::Tertiary));
	const FAudioMaterialKnobStyle* ReflGainStyle  = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain,      AssetCol, EAudioFXKnobSize::Tertiary));
	const FAudioMaterialKnobStyle* LateGainStyle  = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain,      AssetCol, EAudioFXKnobSize::Tertiary));
	const FAudioMaterialKnobStyle* GainHFStyle    = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain,      AssetCol, EAudioFXKnobSize::Tertiary));
	// Output knobs
	const FAudioMaterialKnobStyle* GainStyle      = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain,      AssetCol, EAudioFXKnobSize::Secondary));
	const FAudioMaterialKnobStyle* WetStyle       = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain,      AssetCol, EAudioFXKnobSize::Secondary));
	const FAudioMaterialKnobStyle* DryStyle       = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain,      AssetCol, EAudioFXKnobSize::Secondary));

	TSharedRef<SWidget> Widget =
		SNew(SBox).Padding(FMargin(FAudioFXTheme::WindowPadding, FAudioFXTheme::SectionPadding))
		[
			SNew(SHorizontalBox)

			// ── LEFT COLUMN: graph + core controls + advanced ─────────────
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(SVerticalBox)

				// Reverb decay curve (fills remaining height, min 220px)
				+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 0.f, 0.f, 6.f)
				[
					SNew(SBox).MinDesiredHeight(220.f)
					[
						SNew(SAudioFXReverbDecayCurve)
						.DecayTime_Lambda([LocalWeakPreset]()
						{
							if (const USubmixEffectReverbPreset* P = LocalWeakPreset.Get()) { return P->Settings.DecayTime; }
							return 1.2f;
						})
						.DecayHFRatio_Lambda([LocalWeakPreset]()
						{
							if (const USubmixEffectReverbPreset* P = LocalWeakPreset.Get()) { return P->Settings.DecayHFRatio; }
							return 0.83f;
						})
						.ReflectionsDelay_Lambda([LocalWeakPreset]()
						{
							if (const USubmixEffectReverbPreset* P = LocalWeakPreset.Get()) { return P->Settings.ReflectionsDelay; }
							return 0.01f;
						})
						.LateDelay_Lambda([LocalWeakPreset]()
						{
							if (const USubmixEffectReverbPreset* P = LocalWeakPreset.Get()) { return P->Settings.LateDelay; }
							return 0.01f;
						})
						.Density_Lambda([LocalWeakPreset]()
						{
							if (const USubmixEffectReverbPreset* P = LocalWeakPreset.Get()) { return P->Settings.Density; }
							return 0.85f;
						})
						.AssetColor(AssetCol)
					]
				]

				// REVERB CORE section: three columns (Left / Decay / Right)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SAudioFXSectionBox).SectionLabel(LOCTEXT("CoreSec", "REVERB CORE"))
					[
						SNew(SHorizontalBox)

						// LEFT sub-column: Pre-Delay + Density
						+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center).Padding(2.f, 0.f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 4.f)
							[ FAudioFXTheme::MakeGroupLabel(LOCTEXT("LeftGroup", "LEFT")) ]

							+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
							[
								SNew(SAudioFXLabeledKnob)
								.ParamName(LOCTEXT("ReflDelayLbl", "Pre-Delay"))
								.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
									LOCTEXT("ReflDelayTipLbl", "Reflections Delay"), 0.f, 0.3f, TEXT("s"),
									LOCTEXT("ReflDelayTip", "The time between the direct path sound and the first reflection.")))
								.Value_Lambda([LocalWeakPreset]() -> float
								{
									if (const USubmixEffectReverbPreset* P = LocalWeakPreset.Get())
									{
										return FMath::GetMappedRangeValueClamped(FVector2D(0.f, 0.3f), FVector2D(0.f, 1.f), P->Settings.ReflectionsDelay);
									}
									return 0.f;
								})
								.Style(ReflDelayStyle)
								.AudioUnitsValueType(EAudioUnitsValueType::Linear)
								.ValueFormat(EAudioFXValueFormat::Time)
								.MinValue(0.f)
								.MaxValue(0.3f)
								.OnValueChanged_Lambda([this, LocalWeakPreset](float NewNorm)
								{
									if (USubmixEffectReverbPreset* P = LocalWeakPreset.Get())
									{
										MarkDirty.ExecuteIfBound();
										P->Settings.ReflectionsDelay = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 1.f), FVector2D(0.f, 0.3f), NewNorm);
										P->SetSettings(P->Settings);
									}
								})
							]

							+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 4.f, 0.f, 0.f)
							[
								SNew(SAudioFXLabeledKnob)
								.ParamName(LOCTEXT("DensityLbl", "Density"))
								.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
									LOCTEXT("DensityTipLbl", "Density"), 0.f, 1.f, TEXT(""),
									LOCTEXT("DensityTip", "Coloration of the late reverb. Lower values are more grainy.")))
								.Value_Lambda([LocalWeakPreset]()
								{
									if (const USubmixEffectReverbPreset* P = LocalWeakPreset.Get()) { return P->Settings.Density; }
									return 0.f;
								})
								.Style(DensityStyle)
								.AudioUnitsValueType(EAudioUnitsValueType::Linear)
								.ValueFormat(EAudioFXValueFormat::Percentage)
								.OnValueChanged_Lambda([this, LocalWeakPreset](float NewNorm)
								{
									if (USubmixEffectReverbPreset* P = LocalWeakPreset.Get())
									{
										MarkDirty.ExecuteIfBound();
										P->Settings.Density = NewNorm;
										P->SetSettings(P->Settings);
									}
								})
							]
						]

						// CENTER sub-column: Decay (hero)
						+ SHorizontalBox::Slot().FillWidth(1.2f).HAlign(HAlign_Center).Padding(2.f, 0.f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 4.f)
							[ FAudioFXTheme::MakeGroupLabel(LOCTEXT("CenterGroup", "DECAY")) ]

							+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
							[
								SNew(SAudioFXLabeledKnob)
								.ParamName(LOCTEXT("DecayLbl", "Decay"))
								.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
									LOCTEXT("DecayTipLbl", "Decay Time"), 0.1f, 20.f, TEXT("s"),
									LOCTEXT("DecayTip", "Reverb tail length. Larger values produce more reverb.")))
								.Value_Lambda([LocalWeakPreset]() -> float
								{
									if (const USubmixEffectReverbPreset* P = LocalWeakPreset.Get())
									{
										return FMath::GetMappedRangeValueClamped(FVector2D(0.1f, 20.f), FVector2D(0.f, 1.f), P->Settings.DecayTime);
									}
									return 0.f;
								})
								.Style(DecayStyle)
								.AudioUnitsValueType(EAudioUnitsValueType::Linear)
								.ValueFormat(EAudioFXValueFormat::Time)
								.MinValue(0.1f)
								.MaxValue(20.f)
								.OnValueChanged_Lambda([this, LocalWeakPreset](float NewNorm)
								{
									if (USubmixEffectReverbPreset* P = LocalWeakPreset.Get())
									{
										MarkDirty.ExecuteIfBound();
										P->Settings.DecayTime = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 1.f), FVector2D(0.1f, 20.f), NewNorm);
										P->SetSettings(P->Settings);
									}
								})
							]
						]

						// RIGHT sub-column: Diffusion + HF Damping
						+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center).Padding(2.f, 0.f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 4.f)
							[ FAudioFXTheme::MakeGroupLabel(LOCTEXT("RightGroup", "RIGHT")) ]

							+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
							[
								SNew(SAudioFXLabeledKnob)
								.ParamName(LOCTEXT("DiffLbl", "Diffusion"))
								.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
									LOCTEXT("DiffTipLbl", "Diffusion"), 0.f, 1.f, TEXT(""),
									LOCTEXT("DiffTip", "Echo density in the reverberation decay. Lower values are more grainy.")))
								.Value_Lambda([LocalWeakPreset]()
								{
									if (const USubmixEffectReverbPreset* P = LocalWeakPreset.Get()) { return P->Settings.Diffusion; }
									return 0.f;
								})
								.Style(DiffStyle)
								.AudioUnitsValueType(EAudioUnitsValueType::Linear)
								.ValueFormat(EAudioFXValueFormat::Percentage)
								.OnValueChanged_Lambda([this, LocalWeakPreset](float NewNorm)
								{
									if (USubmixEffectReverbPreset* P = LocalWeakPreset.Get())
									{
										MarkDirty.ExecuteIfBound();
										P->Settings.Diffusion = NewNorm;
										P->SetSettings(P->Settings);
									}
								})
							]

							+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 4.f, 0.f, 0.f)
							[
								SNew(SAudioFXLabeledKnob)
								.ParamName(LOCTEXT("AirAbsLbl", "HF Damping"))
								.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
									LOCTEXT("AirAbsTipLbl", "HF Damping"), 0.f, 1.f, TEXT(""),
									LOCTEXT("AirAbsTip", "High-frequency damping amount. Lower values increase damping (inverted control).")))
								.Value_Lambda([LocalWeakPreset]()
								{
									if (const USubmixEffectReverbPreset* P = LocalWeakPreset.Get()) { return P->Settings.AirAbsorptionGainHF; }
									return 0.f;
								})
								.Style(AirAbsStyle)
								.AudioUnitsValueType(EAudioUnitsValueType::Volume)
								.ValueFormat(EAudioFXValueFormat::Percentage)
								.OnValueChanged_Lambda([this, LocalWeakPreset](float NewNorm)
								{
									if (USubmixEffectReverbPreset* P = LocalWeakPreset.Get())
									{
										MarkDirty.ExecuteIfBound();
										P->Settings.AirAbsorptionGainHF = NewNorm;
										P->SetSettings(P->Settings);
									}
								})
							]
						]
					]
				]

				// ADVANCED collapsible section
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
				[
					SNew(SAudioFXCollapsibleSection)
					.SectionLabel(LOCTEXT("AdvancedSec", "ADVANCED"))
					.bDefaultExpanded(false)
					.OnToggled_Lambda([this]()
					{
						bAdvancedExpanded = !bAdvancedExpanded;
						ResizeWindowByDelta(bAdvancedExpanded ? 140.f : -140.f);
					})
					[
						SNew(SHorizontalBox)

						// Late Delay
						+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center)
						[
							SNew(SAudioFXLabeledKnob)
							.ParamName(LOCTEXT("LateDelayLbl", "Late Delay"))
							.Style(LateDelayStyle)
							.AudioUnitsValueType(EAudioUnitsValueType::Linear)
							.ValueFormat(EAudioFXValueFormat::Time)
							.MinValue(0.f)
							.MaxValue(0.1f)
							.Value_Lambda([LocalWeakPreset]() -> float
							{
								if (const USubmixEffectReverbPreset* P = LocalWeakPreset.Get())
								{
									return FMath::GetMappedRangeValueClamped(FVector2D(0.f, 0.1f), FVector2D(0.f, 1.f), P->Settings.LateDelay);
								}
								return 0.f;
							})
							.OnValueChanged_Lambda([this, LocalWeakPreset](float NewNorm)
							{
								if (USubmixEffectReverbPreset* P = LocalWeakPreset.Get())
								{
									MarkDirty.ExecuteIfBound();
									P->Settings.LateDelay = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 1.f), FVector2D(0.f, 0.1f), NewNorm);
									P->SetSettings(P->Settings);
								}
							})
						]

						// Decay HF Ratio
						+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center)
						[
							SNew(SAudioFXLabeledKnob)
							.ParamName(LOCTEXT("DecayHFLbl", "Decay HF"))
							.Style(DecayHFStyle)
							.AudioUnitsValueType(EAudioUnitsValueType::Linear)
							.ValueFormat(EAudioFXValueFormat::Ratio)
							.MinValue(0.1f)
							.MaxValue(2.f)
							.Value_Lambda([LocalWeakPreset]() -> float
							{
								if (const USubmixEffectReverbPreset* P = LocalWeakPreset.Get())
								{
									return FMath::GetMappedRangeValueClamped(FVector2D(0.1f, 2.f), FVector2D(0.f, 1.f), P->Settings.DecayHFRatio);
								}
								return 0.f;
							})
							.OnValueChanged_Lambda([this, LocalWeakPreset](float NewNorm)
							{
								if (USubmixEffectReverbPreset* P = LocalWeakPreset.Get())
								{
									MarkDirty.ExecuteIfBound();
									P->Settings.DecayHFRatio = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 1.f), FVector2D(0.1f, 2.f), NewNorm);
									P->SetSettings(P->Settings);
								}
							})
						]

						// Reflections Gain
						+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center)
						[
							SNew(SAudioFXLabeledKnob)
							.ParamName(LOCTEXT("ReflGainLbl", "Refl. Gain"))
							.Style(ReflGainStyle)
							.AudioUnitsValueType(EAudioUnitsValueType::Volume)
							.MinValue(0.f)
							.MaxValue(3.16f)
							.Value_Lambda([LocalWeakPreset]() -> float
							{
								if (const USubmixEffectReverbPreset* P = LocalWeakPreset.Get())
								{
									return FMath::GetMappedRangeValueClamped(FVector2D(0.f, 3.16f), FVector2D(0.f, 1.f), P->Settings.ReflectionsGain);
								}
								return 0.f;
							})
							.OnValueChanged_Lambda([this, LocalWeakPreset](float NewNorm)
							{
								if (USubmixEffectReverbPreset* P = LocalWeakPreset.Get())
								{
									MarkDirty.ExecuteIfBound();
									P->Settings.ReflectionsGain = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 1.f), FVector2D(0.f, 3.16f), NewNorm);
									P->SetSettings(P->Settings);
								}
							})
						]

						// Late Gain
						+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center)
						[
							SNew(SAudioFXLabeledKnob)
							.ParamName(LOCTEXT("LateGainLbl", "Late Gain"))
							.Style(LateGainStyle)
							.AudioUnitsValueType(EAudioUnitsValueType::Volume)
							.MinValue(0.f)
							.MaxValue(10.f)
							.Value_Lambda([LocalWeakPreset]() -> float
							{
								if (const USubmixEffectReverbPreset* P = LocalWeakPreset.Get())
								{
									return FMath::GetMappedRangeValueClamped(FVector2D(0.f, 10.f), FVector2D(0.f, 1.f), P->Settings.LateGain);
								}
								return 0.f;
							})
							.OnValueChanged_Lambda([this, LocalWeakPreset](float NewNorm)
							{
								if (USubmixEffectReverbPreset* P = LocalWeakPreset.Get())
								{
									MarkDirty.ExecuteIfBound();
									P->Settings.LateGain = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 1.f), FVector2D(0.f, 10.f), NewNorm);
									P->SetSettings(P->Settings);
								}
							})
						]

						// HF Gain
						+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center)
						[
							SNew(SAudioFXLabeledKnob)
							.ParamName(LOCTEXT("GainHFLbl", "HF Gain"))
							.Style(GainHFStyle)
							.AudioUnitsValueType(EAudioUnitsValueType::Volume)
							.ValueFormat(EAudioFXValueFormat::Percentage)
							.Value_Lambda([LocalWeakPreset]()
							{
								if (const USubmixEffectReverbPreset* P = LocalWeakPreset.Get()) { return P->Settings.GainHF; }
								return 0.f;
							})
							.OnValueChanged_Lambda([this, LocalWeakPreset](float NewNorm)
							{
								if (USubmixEffectReverbPreset* P = LocalWeakPreset.Get())
								{
									MarkDirty.ExecuteIfBound();
									P->Settings.GainHF = NewNorm;
									P->SetSettings(P->Settings);
								}
							})
						]
					]
				]
			]  // end LEFT column

			// ── RIGHT COLUMN: Output knobs stacked vertically ─────────────
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
			[
				SNew(SAudioFXSectionBox).SectionLabel(LOCTEXT("OutSec", "OUTPUT"))
				[
					SNew(SVerticalBox)

					// Master Gain
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 4.f)
					[
						SNew(SAudioFXLabeledKnob)
						.ParamName(LOCTEXT("GainLbl", "Gain"))
						.Style(GainStyle)
						.AudioUnitsValueType(EAudioUnitsValueType::Volume)
						.ValueFormat(EAudioFXValueFormat::Percentage)
						.Value_Lambda([LocalWeakPreset]()
						{
							if (const USubmixEffectReverbPreset* P = LocalWeakPreset.Get()) { return P->Settings.Gain; }
							return 0.f;
						})
						.OnValueChanged_Lambda([this, LocalWeakPreset](float NewNorm)
						{
							if (USubmixEffectReverbPreset* P = LocalWeakPreset.Get())
							{
								MarkDirty.ExecuteIfBound();
								P->Settings.Gain = NewNorm;
								P->SetSettings(P->Settings);
							}
						})
					]

					// Wet Level
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 4.f)
					[
						SNew(SAudioFXLabeledKnob)
						.ParamName(LOCTEXT("WetLbl", "Wet"))
						.Style(WetStyle)
						.AudioUnitsValueType(EAudioUnitsValueType::Volume)
						.MinValue(0.f)
						.MaxValue(10.f)
						.Value_Lambda([LocalWeakPreset]() -> float
						{
							if (const USubmixEffectReverbPreset* P = LocalWeakPreset.Get())
							{
								return FMath::GetMappedRangeValueClamped(FVector2D(0.f, 10.f), FVector2D(0.f, 1.f), P->Settings.WetLevel);
							}
							return 0.f;
						})
						.OnValueChanged_Lambda([this, LocalWeakPreset](float NewNorm)
						{
							if (USubmixEffectReverbPreset* P = LocalWeakPreset.Get())
							{
								MarkDirty.ExecuteIfBound();
								P->Settings.WetLevel = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 1.f), FVector2D(0.f, 10.f), NewNorm);
								P->SetSettings(P->Settings);
							}
						})
					]

					// Dry Level
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(SAudioFXLabeledKnob)
						.ParamName(LOCTEXT("DryLbl", "Dry"))
						.Style(DryStyle)
						.AudioUnitsValueType(EAudioUnitsValueType::Volume)
						.ValueFormat(EAudioFXValueFormat::Percentage)
						.Value_Lambda([LocalWeakPreset]()
						{
							if (const USubmixEffectReverbPreset* P = LocalWeakPreset.Get()) { return P->Settings.DryLevel; }
							return 0.f;
						})
						.OnValueChanged_Lambda([this, LocalWeakPreset](float NewNorm)
						{
							if (USubmixEffectReverbPreset* P = LocalWeakPreset.Get())
							{
								MarkDirty.ExecuteIfBound();
								P->Settings.DryLevel = NewNorm;
								P->SetSettings(P->Settings);
							}
						})
					]
				]
			]
		];  // end outer SHorizontalBox

	return Widget;
}

#undef LOCTEXT_NAMESPACE
