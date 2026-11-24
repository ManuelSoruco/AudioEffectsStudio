// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "Shared/Core/AudioFXEffectEditor.h"
#include "Shared/Core/AudioFXTheme.h"

/**
 * FAudioFXSubmixStereoToQuadEditor
 *
 * Editor for USubmixEffectStereoToQuadPreset.
 * Provides: Flip Channels toggle + Rear Channel Gain knob.
 */
class AUDIOEFFECTSSTUDIO_API FAudioFXSubmixStereoToQuadEditor : public FAudioFXEffectEditor
{
protected:
	virtual TSharedRef<SWidget> CreateEffectWidget(USoundEffectPreset* InPreset) override;
	virtual FVector2D           GetDefaultWindowSize() const override { return FVector2D(300, 350); }
	virtual FText               GetEditorTitle() const override { return FText::FromString(TEXT("Stereo to Quad")); }
};
