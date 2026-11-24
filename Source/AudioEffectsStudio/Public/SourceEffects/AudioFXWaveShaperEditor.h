// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "Shared/Core/AudioFXEffectEditor.h"

/**
 * WaveShaper effect editor — 2 hero knobs: Amount + Output Gain.
 * Asset color: #DAF84E (bright lime).
 */
class AUDIOEFFECTSSTUDIO_API FAudioFXWaveShaperEditor : public FAudioFXEffectEditor
{
protected:
	virtual TSharedRef<SWidget> CreateEffectWidget(USoundEffectPreset* InPreset) override;
	virtual FVector2D GetDefaultWindowSize() const override { return FVector2D(640, 320); }
	virtual FText               GetEditorTitle() const override { return FText::FromString(TEXT("Wave Shaper")); }
};
