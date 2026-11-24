// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

/**
 * FAudioFXSubmixDelayEditor
 *
 * Knob-based editor for USubmixEffectDelayPreset.
 * Layout: Max Length (Primary) · Delay Length (Hero) · Interp Time (Primary)
 *
 * NOTE: Max Length sets the delay buffer at initialization; changing it at runtime
 *       resets the effect. Delay Length is always clamped to Max Length.
 */

#include "SubmixEffects/AudioFXSubmixDelayEditor.h"

#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Widgets/SAudioFXLabeledKnob.h"
#include "SubmixEffects/SubmixEffectDelay.h"

#include "AudioWidgetsEnums.h"
#include "Widgets/Layout/SBox.h"

#define LOCTEXT_NAMESPACE "AudioFXSubmixDelay"

TSharedRef<SWidget> FAudioFXSubmixDelayEditor::CreateEffectWidget(USoundEffectPreset* InPreset)
{
	USubmixEffectDelayPreset* Preset = Cast<USubmixEffectDelayPreset>(InPreset);
	if (!Preset)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("AudioEffectsStudio: Cast to preset type failed for '%s'. Check asset registration."),
			InPreset ? *InPreset->GetClass()->GetName() : TEXT("null"));
		return FAudioFXEffectEditor::CreateEffectWidget(InPreset);
	}

	const FLinearColor AssetCol = GetAssetColor();

	// ── Knob styles: Delay=Hero, MaxLen/Interp=Primary ───────────────
	const FAudioMaterialKnobStyle* MaxLenStyle = StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Time, AssetCol, EAudioFXKnobSize::Primary));
	const FAudioMaterialKnobStyle* InterpStyle = StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Time, AssetCol, EAudioFXKnobSize::Primary));
	const FAudioMaterialKnobStyle* DelayStyle = StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Time, AssetCol, EAudioFXKnobSize::Hero));

	TWeakObjectPtr<USubmixEffectDelayPreset> WeakPreset = Preset;

	TSharedRef<SWidget> Widget =
		SNew(SBox)
		.Padding(FAudioFXTheme::SectionPadding)
		[
			SNew(SHorizontalBox)

			// Max Length (left, smaller — sets buffer size at init)
			+ SHorizontalBox::Slot()
			.FillWidth(0.8f).HAlign(HAlign_Center).VAlign(VAlign_Center)
			.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.0f)
			[
				SNew(SAudioFXLabeledKnob)
				.ParamName(LOCTEXT("MaxLenLbl", "Max Length"))
				.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
					LOCTEXT("MaxLenTipName", "Max Delay Length"),
					10.f, 20000.f, TEXT("ms"),
					LOCTEXT("MaxLenTip",
						"Maximum possible length for a delay, in milliseconds.\n"
						"AudioFactory Note: Changing this at runtime will reset the effect. Sets the delay buffer size on initialization.")))
				.Value_Lambda([WeakPreset]()
				{
					if (const USubmixEffectDelayPreset* P = WeakPreset.Get())
						return (float)FMath::GetMappedRangeValueClamped(FVector2D(10.f, 20000.f), FVector2D(0.f, 1.f), P->Settings.MaximumDelayLength);
					return 0.5f;
				})
				.Style(MaxLenStyle)
				.AudioUnitsValueType(EAudioUnitsValueType::Linear)
				.ValueFormat(EAudioFXValueFormat::Time)
				.OnValueChanged_Lambda([this, WeakPreset](float NewNorm)
				{
					if (USubmixEffectDelayPreset* P = WeakPreset.Get())
					{
						MarkDirty.ExecuteIfBound();
						P->Settings.MaximumDelayLength = FMath::GetMappedRangeValueClamped(
							FVector2D(0.f, 1.f), FVector2D(10.f, 20000.f), NewNorm);
						P->Settings.DelayLength = FMath::Min(P->Settings.DelayLength, P->Settings.MaximumDelayLength);
						P->SetSettings(P->Settings);
					}
				})
			]

			// Delay Length (center, hero — main control)
			+ SHorizontalBox::Slot()
			.FillWidth(1.5f).HAlign(HAlign_Center).VAlign(VAlign_Center)
			.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.0f)
			[
				SNew(SAudioFXLabeledKnob)
				.ParamName(LOCTEXT("DelayLbl", "Delay"))
				.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
					LOCTEXT("DelayTipName", "Delay Length"),
					0.f, 20000.f, TEXT("ms"),
					LOCTEXT("DelayTip", "Number of milliseconds of delay. Caps at Max Delay Length at runtime.")))
				.Value_Lambda([WeakPreset]()
				{
					if (const USubmixEffectDelayPreset* P = WeakPreset.Get())
						return (float)FMath::GetMappedRangeValueClamped(FVector2D(0.f, 20000.f), FVector2D(0.f, 1.f), P->Settings.DelayLength);
					return 0.f;
				})
				.Style(DelayStyle)
				.AudioUnitsValueType(EAudioUnitsValueType::Linear)
				.ValueFormat(EAudioFXValueFormat::Time)
				.OnValueChanged_Lambda([this, WeakPreset](float NewNorm)
				{
					if (USubmixEffectDelayPreset* P = WeakPreset.Get())
					{
						MarkDirty.ExecuteIfBound();
						const float NewDelay = FMath::GetMappedRangeValueClamped(
							FVector2D(0.f, 1.f), FVector2D(0.f, 20000.f), NewNorm);
						P->Settings.DelayLength = FMath::Min(NewDelay, P->Settings.MaximumDelayLength);
						P->SetSettings(P->Settings);
					}
				})
			]

			// Interp Time (right, smaller)
			+ SHorizontalBox::Slot()
			.FillWidth(0.8f).HAlign(HAlign_Center).VAlign(VAlign_Center)
			.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.0f)
			[
				SNew(SAudioFXLabeledKnob)
				.ParamName(LOCTEXT("InterpLbl", "Interp Time"))
				.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
					LOCTEXT("InterpTipName", "Interpolation Time"),
					0.f, 20000.f, TEXT("ms"),
					LOCTEXT("InterpTip",
						"Number of milliseconds over which a tap will reach its set length and gain.\n"
						"AudioFactory Note: Smaller values are more responsive, while larger values will make pitching less dramatic.")))
				.Value_Lambda([WeakPreset]()
				{
					if (const USubmixEffectDelayPreset* P = WeakPreset.Get())
						return (float)FMath::GetMappedRangeValueClamped(FVector2D(0.f, 20000.f), FVector2D(0.f, 1.f), P->Settings.InterpolationTime);
					return 0.f;
				})
				.Style(InterpStyle)
				.AudioUnitsValueType(EAudioUnitsValueType::Linear)
				.ValueFormat(EAudioFXValueFormat::Time)
				.OnValueChanged_Lambda([this, WeakPreset](float NewNorm)
				{
					if (USubmixEffectDelayPreset* P = WeakPreset.Get())
					{
						MarkDirty.ExecuteIfBound();
						P->Settings.InterpolationTime = FMath::GetMappedRangeValueClamped(
							FVector2D(0.f, 1.f), FVector2D(0.f, 20000.f), NewNorm);
						P->SetSettings(P->Settings);
					}
				})
			]
		];

	return Widget;
}

#undef LOCTEXT_NAMESPACE
