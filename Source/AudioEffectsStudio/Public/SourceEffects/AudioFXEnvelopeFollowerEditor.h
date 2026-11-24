// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "Shared/Core/AudioFXEffectEditor.h"

/**
 * Envelope Follower — Attack/Release knobs + PeakMode button strip + AnalogMode toggle.
 * Asset color: #F8DA4E (golden yellow).
 */
class AUDIOEFFECTSSTUDIO_API FAudioFXEnvelopeFollowerEditor : public FAudioFXEffectEditor
{
protected:
	virtual TSharedRef<SWidget> CreateEffectWidget(USoundEffectPreset* InPreset) override;
	virtual FVector2D GetDefaultWindowSize() const override { return FVector2D(700, 340); }
	virtual FText               GetEditorTitle() const override { return FText::FromString(TEXT("Envelope Follower")); }
};
