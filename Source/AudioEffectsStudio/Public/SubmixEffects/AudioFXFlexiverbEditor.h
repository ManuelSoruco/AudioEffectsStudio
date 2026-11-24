// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "Shared/Core/AudioFXEffectEditor.h"
#include "Shared/Core/AudioFXTheme.h"

/**
 * FAudioFXFlexiverbEditor
 *
 * Algorithmic reverb editor for USubmixEffectFlexiverbPreset.
 * Layout (Template 2: graph on top, knobs below):
 *
 *   TOP  — SAudioFXReverbDecayCurve (fills height, min 200px)
 *   KNOB ROW — Pre-Delay (Primary) · Decay (Hero) · Damping (Secondary) · Complexity (Tertiary)
 *
 * Pre-delay and damping settings are mapped to the curve's reflections/HF-decay
 * parameters so the visualizer accurately represents the Flexiverb algorithm.
 */
class AUDIOEFFECTSSTUDIO_API FAudioFXFlexiverbEditor : public FAudioFXEffectEditor
{
protected:
	virtual TSharedRef<SWidget> CreateEffectWidget(USoundEffectPreset* InPreset) override;
	virtual FVector2D           GetDefaultWindowSize() const override { return FVector2D(780, 460); }
	virtual FText               GetEditorTitle() const override { return FText::FromString(TEXT("Flexiverb")); }
	virtual bool                ShouldShowMeters() const override { return false; }
};
