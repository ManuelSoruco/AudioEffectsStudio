// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "Shared/Core/AudioFXEffectEditor.h"

/**
 * Mid/Side Spreader — Spread knob (hero) + InputMode & OutputMode button strips + EqualPower toggle.
 * Asset color: #7EB4FF (sky blue).
 */
class AUDIOEFFECTSSTUDIO_API FAudioFXMidSideSpreaderEditor : public FAudioFXEffectEditor
{
protected:
	virtual TSharedRef<SWidget> CreateEffectWidget(USoundEffectPreset* InPreset) override;
    virtual FVector2D GetDefaultWindowSize() const override { return FVector2D(480, 350); }
	virtual FText               GetEditorTitle() const override { return FText::FromString(TEXT("Mid/Side Spreader")); }
};
