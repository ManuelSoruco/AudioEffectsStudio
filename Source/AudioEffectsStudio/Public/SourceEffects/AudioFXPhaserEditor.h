// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "Shared/Core/AudioFXEffectEditor.h"

/**
 * Phaser — WetLevel, Frequency, Feedback knobs + LFO type strip + Quadrature toggle.
 * Asset color: #8C0404 (deep red).
 */
class AUDIOEFFECTSSTUDIO_API FAudioFXPhaserEditor : public FAudioFXEffectEditor
{
protected:
	virtual TSharedRef<SWidget> CreateEffectWidget(USoundEffectPreset* InPreset) override;
	virtual FVector2D GetDefaultWindowSize() const override { return FVector2D(560, 340); }
	virtual FText               GetEditorTitle() const override { return FText::FromString(TEXT("Phaser")); }
};
