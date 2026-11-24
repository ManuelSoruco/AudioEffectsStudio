// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "Shared/Core/AudioFXEffectEditor.h"
#include "SourceEffects/SourceEffectMotionFilter.h"

/**
 * Source Motion Filter editor.
 *
 * Layout (vertical, 3 sections):
 *   GLOBAL  : [Serial | Parallel] topology strip | Mix knob | Dry (dB) knob
 *             + modulation summary row (N mappings + Edit Mappings button)
 *   FILTER A: Circuit strip (3) | Type strip (4) | Cutoff (Hero) | Q (Standard)
 *   FILTER B: same as A
 *   MODULATION overlay: full mapping editor shown on top when Edit Mappings clicked
 *
 * Asset color: (0, 185, 211) — cyan
 */
class AUDIOEFFECTSSTUDIO_API FAudioFXMotionFilterEditor : public FAudioFXEffectEditor
{
protected:
	virtual TSharedRef<SWidget> CreateEffectWidget(USoundEffectPreset* InPreset) override;
    virtual FVector2D GetDefaultWindowSize() const override { return FVector2D(920, 520); }
	virtual FText               GetEditorTitle() const override { return FText::FromString(TEXT("Motion Filter")); }

private:
	/** Build the strip+knob block for one of the two filters. */
	TSharedRef<SWidget> BuildFilterSection(
		const FText& SectionLabel,
		const FSourceEffectIndividualFilterSettings& Settings,
		TFunction<void(ESourceEffectMotionFilterCircuit)> OnCircuit,
		TFunction<void(ESourceEffectMotionFilterType)>    OnType,
		TFunction<float()>                                GetCutoffNorm,
		TFunction<float()>                                GetQNorm,
		TFunction<void(float)>                            OnCutoff,
		TFunction<void(float)>                            OnQ,
		const FLinearColor& AssetCol,
		TWeakObjectPtr<UObject> InEditedObject = nullptr);
};
