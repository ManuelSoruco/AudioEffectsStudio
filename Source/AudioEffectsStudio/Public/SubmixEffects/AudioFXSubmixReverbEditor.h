// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "Shared/Core/AudioFXEffectEditor.h"
#include "Shared/Core/AudioFXTheme.h"

/**
 * FAudioFXSubmixReverbEditor
 *
 * Physical-model reverb editor for USubmixEffectReverbPreset.
 *
 * Layout (horizontal split):
 *   LEFT  — SAudioFXReverbDecayCurve (top, fills height)
 *           REVERB CORE: Pre-Delay · Decay (hero) · Diffusion · HF Damping · Density
 *           ADVANCED (collapsible): Late Delay · Decay HF · Refl. Gain · Late Gain · HF Gain
 *   RIGHT — OUTPUT: Gain · Wet · Dry (stacked vertically)
 *
 * Bypass is forwarded through GetBypassState/SetBypassState → Settings.bBypass.
 */
class AUDIOEFFECTSSTUDIO_API FAudioFXSubmixReverbEditor : public FAudioFXEffectEditor
{
public:
	virtual TSharedRef<SWidget> CreateEffectWidget(USoundEffectPreset* InPreset) override;
    virtual FVector2D           GetDefaultWindowSize() const override { return FVector2D(840, 580); }
	virtual FText               GetEditorTitle() const override { return FText::FromString(TEXT("Submix Reverb")); }
	virtual bool                ShouldShowMeters() const override { return false; }
	virtual bool                ShouldShowBypass() const override { return true; }
	virtual bool                GetBypassState() const override;
	virtual void                SetBypassState(bool bBypassed) override;

private:
	bool bAdvancedExpanded = false;
};
