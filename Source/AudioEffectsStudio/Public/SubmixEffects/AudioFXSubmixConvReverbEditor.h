// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "Shared/Core/AudioFXEffectEditor.h"
#include "Shared/Core/AudioFXTheme.h"

/**
 * FAudioFXSubmixConvReverbEditor
 *
 * Convolution Reverb editor for USubmixEffectConvolutionReverbPreset.
 * Delegates all UI construction to AudioFXConvReverbUI::BuildWidget,
 * which is shared with the source-effect convolution reverb editor.
 *
 * Only submix-specific responsibilities:
 *   · GetBypassState / SetBypassState forward to Settings.bBypass
 *   · GetDefaultWindowSize returns the submix variant dimensions
 */
class AUDIOEFFECTSSTUDIO_API FAudioFXSubmixConvReverbEditor : public FAudioFXEffectEditor
{
protected:
	virtual TSharedRef<SWidget> CreateEffectWidget(USoundEffectPreset* InPreset) override;
	virtual FVector2D           GetDefaultWindowSize() const override { return FVector2D(640, 420); }
	virtual FText               GetEditorTitle() const override { return FText::FromString(TEXT("Submix Conv. Reverb")); }
	virtual bool                ShouldShowMeters() const override { return false; }
	virtual bool                ShouldShowBypass() const override { return true; }
	virtual bool                GetBypassState() const override;
	virtual void                SetBypassState(bool bBypassed) override;
};
