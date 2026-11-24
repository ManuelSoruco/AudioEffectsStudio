// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "Shared/Core/AudioFXEffectEditor.h"

/** Stereo Delay — Mode strip + 6 knobs + collapsible Filter section (EditCondition = bFilterEnabled). */
class AUDIOEFFECTSSTUDIO_API FAudioFXStereoDelayEditor : public FAudioFXEffectEditor
{
protected:
	virtual TSharedRef<SWidget> CreateEffectWidget(USoundEffectPreset* InPreset) override;
	virtual FVector2D GetDefaultWindowSize() const override { return FVector2D(780, 560); }
	virtual FText               GetEditorTitle() const override { return FText::FromString(TEXT("Source Stereo Delay")); }
	virtual bool ShouldShowMeters() const override { return false; }
};
