// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "Shared/Core/AudioFXEffectEditor.h"

/**
 * Ring Modulation — Modulator type strip + Freq (log), Depth, Dry, Wet knobs.
 * Asset color: #7A7DC3 (steel blue-violet).
 */
class AUDIOEFFECTSSTUDIO_API FAudioFXRingModulationEditor : public FAudioFXEffectEditor
{
protected:
	virtual TSharedRef<SWidget> CreateEffectWidget(USoundEffectPreset* InPreset) override;
	virtual FVector2D GetDefaultWindowSize() const override { return FVector2D(560, 310); }
	virtual FText               GetEditorTitle() const override { return FText::FromString(TEXT("Ring Modulator")); }
};
