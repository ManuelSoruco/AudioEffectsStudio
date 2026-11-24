// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

/**
 * FAudioFXFlexiverbEditor
 *
 * Algorithmic reverb editor for USubmixEffectFlexiverbPreset.
 * Layout (Template 2: graph on top, knobs below):
 *
 *   TOP  — SAudioFXReverbDecayCurve (fills height, min 200px)
 *          Curve parameters are derived from the Flexiverb-specific settings:
 *            · PreDelay  → ReflectionsDelay (converted ms→s)
 *            · RoomDampening → DecayHFRatio (log-mapped 60..12kHz → 0.3..1.2)
 *            · Complexity → Density (mapped 2..6 → 0.4..1.0)
 *
 *   KNOB ROW — Pre-Delay (Primary) · Decay (Hero) · Damping (Secondary) · Complexity (Tertiary)
 */

#include "SubmixEffects/AudioFXFlexiverbEditor.h"

#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Widgets/SAudioFXLabeledKnob.h"
#include "Shared/Visualizers/SAudioFXReverbDecayCurve.h"
#include "SubmixEffects/SubmixEffectFlexiverb.h"

#include "AudioWidgetsEnums.h"
#include "Widgets/Layout/SBox.h"

#define LOCTEXT_NAMESPACE "AudioFXFlexiverb"

TSharedRef<SWidget> FAudioFXFlexiverbEditor::CreateEffectWidget(USoundEffectPreset* InPreset)
{
	USubmixEffectFlexiverbPreset* Preset = Cast<USubmixEffectFlexiverbPreset>(InPreset);
	if (!Preset)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("AudioEffectsStudio: Cast to preset type failed for '%s'. Check asset registration."),
			InPreset ? *InPreset->GetClass()->GetName() : TEXT("null"));
		return FAudioFXEffectEditor::CreateEffectWidget(InPreset);
	}

	const FLinearColor AssetCol = GetAssetColor();

	// ── Knob styles ───────────────────────────────────────────────────
	// Decay = blue Hero (dominant control), PreDelay = green Primary,
	// Damping = purple Secondary, Complexity = Tertiary
	const FAudioMaterialKnobStyle* PreDelayStyle  = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Mix,       AssetCol, EAudioFXKnobSize::Primary));
	const FAudioMaterialKnobStyle* DecayStyle     = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Frequency, AssetCol, EAudioFXKnobSize::Hero));
	const FAudioMaterialKnobStyle* DampingStyle   = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Time,      AssetCol, EAudioFXKnobSize::Secondary));
	const FAudioMaterialKnobStyle* ComplexityStyle = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Resonance, AssetCol, EAudioFXKnobSize::Tertiary));

	TWeakObjectPtr<USubmixEffectFlexiverbPreset> WeakPreset = Preset;

	TSharedRef<SWidget> Widget = SNew(SBox)
		.Padding(FAudioFXTheme::SectionPadding)
		[
			SNew(SVerticalBox)

			// ── Reverb decay curve (graph on top, fills height) ───────────
			+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(SBox).MinDesiredHeight(200.f)
				[
					SNew(SAudioFXReverbDecayCurve)
					.DecayTime_Lambda([WeakPreset]()
					{
						if (const USubmixEffectFlexiverbPreset* P = WeakPreset.Get()) { return P->Settings.DecayTime; }
						return 1.2f;
					})
					.ReflectionsDelay_Lambda([WeakPreset]()
					{
						// PreDelay is in ms; the curve expects seconds
						if (const USubmixEffectFlexiverbPreset* P = WeakPreset.Get()) { return P->Settings.PreDelay / 1000.f; }
						return 0.01f;
					})
					.DecayHFRatio_Lambda([WeakPreset]()
					{
						// Map damping frequency to HF decay ratio: lower freq = faster HF decay
						if (const USubmixEffectFlexiverbPreset* P = WeakPreset.Get())
						{
							return static_cast<float>(FMath::GetMappedRangeValueClamped(
								FVector2D(60.f, 12000.f), FVector2D(0.3f, 1.2f), P->Settings.RoomDampening));
						}
						return 0.83f;
					})
					.Density_Lambda([WeakPreset]()
					{
						// Map Complexity (2..6) to visual density (0.4..1.0)
						if (const USubmixEffectFlexiverbPreset* P = WeakPreset.Get())
						{
							return static_cast<float>(FMath::GetMappedRangeValueClamped(
								FVector2D(2.f, 6.f), FVector2D(0.4f, 1.0f), static_cast<float>(P->Settings.Complexity)));
						}
						return 0.7f;
					})
					.AssetColor(AssetCol)
				]
			]

			// ── Knob row: [PreDelay + Damping stacked] | [Decay HERO centered] | [Complexity] ─
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)

				// LEFT column: Pre-Delay (top) + Damping (bottom)
				+ SHorizontalBox::Slot()
				.FillWidth(1.f).VAlign(VAlign_Center)
				.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 4.f)
					[
						SNew(SAudioFXLabeledKnob)
						.ParamName(LOCTEXT("PreDelayLbl", "Pre-Delay"))
						.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
							LOCTEXT("PreDelayTipLbl", "Pre-Delay"), 0.01f, 30.f, TEXT("ms"),
							LOCTEXT("PreDelayTip", "Amount of delay to the first echo in milliseconds.")))
						.Value_Lambda([WeakPreset]()
						{
							if (const USubmixEffectFlexiverbPreset* P = WeakPreset.Get())
								return (float)FMath::GetMappedRangeValueClamped(FVector2D(0.01f, 30.f), FVector2D(0.f, 1.f), P->Settings.PreDelay);
							return 0.f;
						})
						.Style(PreDelayStyle)
						.AudioUnitsValueType(EAudioUnitsValueType::Linear)
						.ValueFormat(EAudioFXValueFormat::Time)
						.MinValue(0.01f)
						.MaxValue(30.f)
						.OnValueChanged_Lambda([this, WeakPreset](float NewNorm)
						{
							if (USubmixEffectFlexiverbPreset* P = WeakPreset.Get())
							{
								MarkDirty.ExecuteIfBound();
								P->Settings.PreDelay = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 1.f), FVector2D(0.01f, 30.f), NewNorm);
								P->SetSettings(P->Settings);
							}
						})
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(SAudioFXLabeledKnob)
						.ParamName(LOCTEXT("DampLbl", "Damping"))
						.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
							LOCTEXT("DampTipLbl", "Room Dampening"), 60.f, 12000.f, TEXT("Hz"),
							LOCTEXT("DampTip", "Frequency at which the room dampens high frequencies.")))
						.Value_Lambda([WeakPreset]()
						{
							if (const USubmixEffectFlexiverbPreset* P = WeakPreset.Get())
								return FAudioFXTheme::LogFreqToNorm(P->Settings.RoomDampening, 60.f, 12000.f);
							return 0.5f;
						})
						.Style(DampingStyle)
						.AudioUnitsValueType(EAudioUnitsValueType::Frequency)
						.ValueFormat(EAudioFXValueFormat::Frequency)
						.MinValue(60.f)
						.MaxValue(12000.f)
						.OnValueChanged_Lambda([this, WeakPreset](float NewNorm)
						{
							if (USubmixEffectFlexiverbPreset* P = WeakPreset.Get())
							{
								MarkDirty.ExecuteIfBound();
								P->Settings.RoomDampening = FAudioFXTheme::NormToLogFreq(NewNorm, 60.f, 12000.f);
								P->SetSettings(P->Settings);
							}
						})
					]
				]

				// CENTER: Decay Time (Hero) — perfectly centered
				+ SHorizontalBox::Slot()
				.FillWidth(1.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
				.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.0f)
				[
					SNew(SAudioFXLabeledKnob)
					.ParamName(LOCTEXT("DecayLbl", "Decay"))
					.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
						LOCTEXT("DecayTipLbl", "Decay Time"), 0.4f, 5.f, TEXT("s"),
						LOCTEXT("DecayTip", "Time in seconds for the impulse response to decay to -60 dB.")))
					.Value_Lambda([WeakPreset]()
					{
						if (const USubmixEffectFlexiverbPreset* P = WeakPreset.Get())
						{
							return (float)FMath::GetMappedRangeValueClamped(
								FVector2D(0.4f, 5.f), FVector2D(0.f, 1.f),
								FMath::Clamp(P->Settings.DecayTime, 0.4f, 5.f));
						}
						return 0.f;
					})
					.Style(DecayStyle)
					.AudioUnitsValueType(EAudioUnitsValueType::Linear)
					.ValueFormat(EAudioFXValueFormat::Time)
					.MinValue(0.4f)
					.MaxValue(5.f)
					.OnValueChanged_Lambda([this, WeakPreset](float NewNorm)
					{
						if (USubmixEffectFlexiverbPreset* P = WeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							P->Settings.DecayTime = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 1.f), FVector2D(0.4f, 5.0f), NewNorm);
							P->SetSettings(P->Settings);
						}
					})
				]

				// RIGHT column: Complexity
				+ SHorizontalBox::Slot()
				.FillWidth(1.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
				.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.0f)
				[
					SNew(SAudioFXLabeledKnob)
					.ParamName(LOCTEXT("ComplexLbl", "Complexity"))
					.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
						LOCTEXT("ComplexTipLbl", "Complexity"), 2.f, 6.f, TEXT(""),
						LOCTEXT("ComplexTip", "Number of allpass stages in the reverb network. Higher values are denser but cost more CPU.")))
					.Value_Lambda([WeakPreset]()
					{
						if (const USubmixEffectFlexiverbPreset* P = WeakPreset.Get())
							return (float)FMath::GetMappedRangeValueClamped(FVector2D(2.f, 6.f), FVector2D(0.f, 1.f), (float)P->Settings.Complexity);
						return 0.f;
					})
					.Style(ComplexityStyle)
					.AudioUnitsValueType(EAudioUnitsValueType::Linear)
					.ValueFormat(EAudioFXValueFormat::Integer)
					.MinValue(2.f)
					.MaxValue(6.f)
					.OnValueChanged_Lambda([this, WeakPreset](float NewNorm)
					{
						if (USubmixEffectFlexiverbPreset* P = WeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							P->Settings.Complexity = FMath::RoundToInt(
								FMath::GetMappedRangeValueClamped(FVector2D(0.f, 1.f), FVector2D(2.f, 6.f), NewNorm));
							P->SetSettings(P->Settings);
						}
					})
				]
			]  // end knob row SHorizontalBox
		];

	return Widget;
}

#undef LOCTEXT_NAMESPACE
