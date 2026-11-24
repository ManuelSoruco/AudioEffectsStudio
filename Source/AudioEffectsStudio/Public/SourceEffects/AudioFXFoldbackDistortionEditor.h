// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "Shared/Core/AudioFXEffectEditor.h"

/**
 * Foldback Distortion editor — InputGain, Threshold (hero), OutputGain.
 * Asset color: #38E19C (mint green).
 */
class AUDIOEFFECTSSTUDIO_API FAudioFXFoldbackDistortionEditor : public FAudioFXEffectEditor
{
protected:
	virtual TSharedRef<SWidget> CreateEffectWidget(USoundEffectPreset* InPreset) override;
	virtual FVector2D GetDefaultWindowSize() const override { return FVector2D(560, 300); }
	virtual FText               GetEditorTitle() const override { return FText::FromString(TEXT("Foldback Distortion")); }
};
