// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "Shared/Core/AudioFXEffectEditor.h"

/**
 * Source Dynamics Processor — Type strip, grouped knob sections (General/Dynamics/Response),
 * Stereo Linked + Analog Mode toggles, Bypass.
 * Asset color: #DAC70B (gold-yellow).
 */
class AUDIOEFFECTSSTUDIO_API FAudioFXDynamicsProcessorEditor : public FAudioFXEffectEditor
{
protected:
	virtual TSharedRef<SWidget> CreateEffectWidget(USoundEffectPreset* InPreset) override;
	virtual FVector2D GetDefaultWindowSize() const override { return FVector2D(840, 540); }
	virtual FText               GetEditorTitle() const override { return FText::FromString(TEXT("Source Dynamics")); }
	virtual bool ShouldShowMeters() const override { return false; }
	virtual bool ShouldShowGRMeter() const override { return false; }
};
