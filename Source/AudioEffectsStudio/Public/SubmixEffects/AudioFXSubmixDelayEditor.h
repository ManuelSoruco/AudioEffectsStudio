// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "Shared/Core/AudioFXEffectEditor.h"
#include "Shared/Core/AudioFXTheme.h"

/**
 * FAudioFXSubmixDelayEditor
 *
 * Knob-based editor for USubmixEffectDelayPreset (Template 2: graph on top).
 *
 * Layout:
 *   TOP      — SAudioFXReverbDecayCurve in delay mode (fills height, min 200px)
 *   KNOB ROW — Max Length (Primary) · Delay Length (Hero) · Interp Time (Primary)
 *
 * Max Length sets the delay buffer size at initialization; changing it at runtime
 * resets the effect. Delay Length is always kept ≤ Max Length.
 */
class AUDIOEFFECTSSTUDIO_API FAudioFXSubmixDelayEditor : public FAudioFXEffectEditor
{
protected:
	virtual TSharedRef<SWidget> CreateEffectWidget(USoundEffectPreset* InPreset) override;
	virtual FVector2D           GetDefaultWindowSize() const override { return FVector2D(520, 280); }
	virtual FText               GetEditorTitle() const override { return FText::FromString(TEXT("Submix Delay")); }
	virtual bool                ShouldShowMeters() const override { return false; }
};
