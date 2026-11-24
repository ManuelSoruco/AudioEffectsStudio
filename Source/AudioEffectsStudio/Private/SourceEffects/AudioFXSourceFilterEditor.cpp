// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "SourceEffects/AudioFXSourceFilterEditor.h"
#include "Shared/Core/AudioFXTheme.h"
#include "Shared/UIBuilders/AudioFXFilterUI.h"
#include "SourceEffects/SourceEffectFilter.h"

#define LOCTEXT_NAMESPACE "AudioFXSourceFilter"

TSharedRef<SWidget> FAudioFXSourceFilterEditor::CreateEffectWidget(USoundEffectPreset* InPreset)
{
	USourceEffectFilterPreset* Preset = Cast<USourceEffectFilterPreset>(InPreset);
	if (!Preset)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("AudioEffectsStudio: Cast to preset type failed for '%s'. Check asset registration."),
			InPreset ? *InPreset->GetClass()->GetName() : TEXT("null"));
		return FAudioFXEffectEditor::CreateEffectWidget(InPreset);
	}

	const FLinearColor AssetCol = GetAssetColor();
	TWeakObjectPtr<USourceEffectFilterPreset> WeakPreset = Preset;

	FFilterUIParams Params;
	Params.FreqMin = 20.f;
	Params.FreqMax = 12000.f;

	// Filter curve
	Params.GetCurveType.BindLambda([WeakPreset]() -> EAudioFXFilterCurveType
	{
		if (const USourceEffectFilterPreset* Preset = WeakPreset.Get())
		{
			switch (Preset->Settings.FilterType)
			{
			case ESourceEffectFilterType::LowPass:  return EAudioFXFilterCurveType::LowPass;
			case ESourceEffectFilterType::HighPass:  return EAudioFXFilterCurveType::HighPass;
			case ESourceEffectFilterType::BandPass:  return EAudioFXFilterCurveType::BandPass;
			default:                                 return EAudioFXFilterCurveType::Notch;
			}
		}
		return EAudioFXFilterCurveType::LowPass;
	});
	Params.GetCutoffHz.BindLambda([WeakPreset]() -> float
	{
		return WeakPreset.IsValid() ? FMath::Clamp(WeakPreset->Settings.CutoffFrequency, 20.f, 12000.f) : 1000.f;
	});
	Params.GetQ.BindLambda([WeakPreset]() -> float
	{
		return WeakPreset.IsValid() ? FMath::Clamp(WeakPreset->Settings.FilterQ, 0.5f, 10.f) : 0.7f;
	});

	// Circuit dropdown
	Params.CircuitOptions = { LOCTEXT("1Pole", "1-Pole"), LOCTEXT("StateVar", "State Var"), LOCTEXT("Ladder", "Ladder") };
	Params.GetCircuit.BindLambda([WeakPreset]() -> int32 { return WeakPreset.IsValid() ? (int32)WeakPreset->Settings.FilterCircuit : 0; });
	Params.SetCircuit.BindLambda([WeakPreset](int32 OptionIndex)
	{
		if (USourceEffectFilterPreset* Preset = WeakPreset.Get()) { Preset->Settings.FilterCircuit = (ESourceEffectFilterCircuit)OptionIndex; Preset->SetSettings(Preset->Settings); }
	});

	// Type dropdown
	Params.TypeOptions = { LOCTEXT("LP", "Low-Pass"), LOCTEXT("HP", "High-Pass"), LOCTEXT("BP", "Band-Pass"), LOCTEXT("BS", "Band-Stop") };
	Params.GetType.BindLambda([WeakPreset]() -> int32 { return WeakPreset.IsValid() ? (int32)WeakPreset->Settings.FilterType : 0; });
	Params.SetType.BindLambda([WeakPreset](int32 OptionIndex)
	{
		if (USourceEffectFilterPreset* Preset = WeakPreset.Get()) { Preset->Settings.FilterType = (ESourceEffectFilterType)OptionIndex; Preset->SetSettings(Preset->Settings); }
	});

	// Cutoff knob
	Params.GetCutoff.BindLambda([WeakPreset]() -> float
	{
		return WeakPreset.IsValid() ? FMath::Clamp(WeakPreset->Settings.CutoffFrequency, 20.f, 12000.f) : 1000.f;
	});
	Params.SetCutoff.BindLambda([WeakPreset](float NewValue)
	{
		if (USourceEffectFilterPreset* Preset = WeakPreset.Get()) { Preset->Settings.CutoffFrequency = NewValue; Preset->SetSettings(Preset->Settings); }
	});

	// Resonance knob
	Params.GetResonance.BindLambda([WeakPreset]() -> float { return WeakPreset.IsValid() ? WeakPreset->Settings.FilterQ : 0.7f; });
	Params.SetResonance.BindLambda([WeakPreset](float NewValue)
	{
		if (USourceEffectFilterPreset* Preset = WeakPreset.Get()) { Preset->Settings.FilterQ = NewValue; Preset->SetSettings(Preset->Settings); }
	});

	return AudioFXFilterUI::BuildWidget(*this, Params, AssetCol);
}

#undef LOCTEXT_NAMESPACE
