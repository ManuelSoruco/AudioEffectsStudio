// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "Shared/Core/AudioFXEffectEditor.h"
#include "Shared/Core/AudioFXTheme.h"

/**
 * FAudioFXSubmixStereoDelayEditor
 *
 * Thin editor shell for USubmixEffectStereoDelayPreset.
 * Delegates all UI to AudioFXStereoDelayUI::BuildWidget (shared with source effect).
 *
 * UI includes: Mode strip (Normal / Cross / PingPong) + knob rack (delay times,
 * feedback, wetness) + collapsible filter section.
 */
class AUDIOEFFECTSSTUDIO_API FAudioFXSubmixStereoDelayEditor : public FAudioFXEffectEditor
{
protected:
	virtual TSharedRef<SWidget> CreateEffectWidget(USoundEffectPreset* InPreset) override;
	virtual FVector2D           GetDefaultWindowSize() const override { return FVector2D(720, 540); }
	virtual FText               GetEditorTitle() const override { return FText::FromString(TEXT("Submix Stereo Delay")); }
	virtual bool                ShouldShowMeters() const override { return false; }
};
