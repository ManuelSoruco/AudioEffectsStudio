// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

/**
 * FAudioFXSubmixFilterEditor
 *
 * Knob-based editor for USubmixEffectFilterPreset.
 * Delegates UI construction to AudioFXFilterUI::BuildWidget with
 * submix-specific bindings for FilterType and FilterAlgorithm.
 */

#include "SubmixEffects/AudioFXSubmixFilterEditor.h"

#include "Shared/Core/AudioFXTheme.h"
#include "Shared/UIBuilders/AudioFXFilterUI.h"
#include "SubmixEffects/SubmixEffectFilter.h"

#define LOCTEXT_NAMESPACE "AudioFXSubmixFilter"

TSharedRef<SWidget> FAudioFXSubmixFilterEditor::CreateEffectWidget(USoundEffectPreset* InPreset)
{
	USubmixEffectFilterPreset* Preset = Cast<USubmixEffectFilterPreset>(InPreset);
	if (!Preset)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("AudioEffectsStudio: Cast to preset type failed for '%s'. Check asset registration."),
			InPreset ? *InPreset->GetClass()->GetName() : TEXT("null"));
		return FAudioFXEffectEditor::CreateEffectWidget(InPreset);
	}

	const FLinearColor AssetCol = GetAssetColor();
	TWeakObjectPtr<USubmixEffectFilterPreset> WeakPreset = Preset;

	FFilterUIParams Params;
	Params.FreqMin = 20.f;
	Params.FreqMax = 20000.f;

	// ── Filter curve type (drives the visualizer shape) ──────────────
	Params.GetCurveType.BindLambda([WeakPreset]() -> EAudioFXFilterCurveType
	{
		if (const USubmixEffectFilterPreset* P = WeakPreset.Get())
		{
			switch (P->Settings.FilterType)
			{
			case ESubmixFilterType::LowPass:  return EAudioFXFilterCurveType::LowPass;
			case ESubmixFilterType::HighPass:  return EAudioFXFilterCurveType::HighPass;
			case ESubmixFilterType::BandPass:  return EAudioFXFilterCurveType::BandPass;
			default:                           return EAudioFXFilterCurveType::Notch;
			}
		}
		return EAudioFXFilterCurveType::LowPass;
	});

	// ── Curve getter bindings (used by the visualizer) ────────────────
	Params.GetCutoffHz.BindLambda([WeakPreset]() -> float
	{
		return WeakPreset.IsValid()
			? FMath::Clamp(WeakPreset->Settings.FilterFrequency, 20.f, 20000.f)
			: 1000.f;
	});
	Params.GetQ.BindLambda([WeakPreset]() -> float
	{
		return WeakPreset.IsValid()
			? FMath::Clamp(WeakPreset->Settings.FilterQ, 0.5f, 10.f)
			: 0.7f;
	});

	// ── Circuit (Algorithm) dropdown ──────────────────────────────────
	Params.CircuitOptions = { LOCTEXT("1Pole", "1-Pole"), LOCTEXT("StateVar", "State Var"), LOCTEXT("Ladder", "Ladder") };
	Params.GetCircuit.BindLambda([WeakPreset]() -> int32
	{
		return WeakPreset.IsValid() ? (int32)WeakPreset->Settings.FilterAlgorithm : 0;
	});
	Params.SetCircuit.BindLambda([WeakPreset](int32 OptionIndex)
	{
		if (USubmixEffectFilterPreset* P = WeakPreset.Get())
		{
			P->Settings.FilterAlgorithm = (ESubmixFilterAlgorithm)OptionIndex;
			P->SetSettings(P->Settings);
		}
	});

	// ── Type (FilterType) dropdown ────────────────────────────────────
	Params.TypeOptions = { LOCTEXT("LP", "Low-Pass"), LOCTEXT("HP", "High-Pass"), LOCTEXT("BP", "Band-Pass"), LOCTEXT("BS", "Band-Stop") };
	Params.GetType.BindLambda([WeakPreset]() -> int32
	{
		return WeakPreset.IsValid() ? (int32)WeakPreset->Settings.FilterType : 0;
	});
	Params.SetType.BindLambda([WeakPreset](int32 OptionIndex)
	{
		if (USubmixEffectFilterPreset* P = WeakPreset.Get())
		{
			P->Settings.FilterType = (ESubmixFilterType)OptionIndex;
			P->SetSettings(P->Settings);
		}
	});

	// ── Cutoff knob ───────────────────────────────────────────────────
	Params.GetCutoff.BindLambda([WeakPreset]() -> float
	{
		return WeakPreset.IsValid()
			? FMath::Clamp(WeakPreset->Settings.FilterFrequency, 20.f, 20000.f)
			: 1000.f;
	});
	Params.SetCutoff.BindLambda([WeakPreset](float NewValue)
	{
		if (USubmixEffectFilterPreset* P = WeakPreset.Get())
		{
			P->Settings.FilterFrequency = NewValue;
			P->SetSettings(P->Settings);
		}
	});

	// ── Resonance (Q) knob ────────────────────────────────────────────
	Params.GetResonance.BindLambda([WeakPreset]() -> float
	{
		return WeakPreset.IsValid() ? WeakPreset->Settings.FilterQ : 0.7f;
	});
	Params.SetResonance.BindLambda([WeakPreset](float NewValue)
	{
		if (USubmixEffectFilterPreset* P = WeakPreset.Get())
		{
			P->Settings.FilterQ = NewValue;
			P->SetSettings(P->Settings);
		}
	});

	return AudioFXFilterUI::BuildWidget(*this, Params, AssetCol);
}

#undef LOCTEXT_NAMESPACE
