// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "Shared/Core/AudioFXEffectEditor.h"

/** Source Convolution Reverb — Bypass toggle + Wet/Dry hero knobs + read-only normalization text. */
class AUDIOEFFECTSSTUDIO_API FAudioFXConvReverbSourceEditor : public FAudioFXEffectEditor
{
protected:
	virtual TSharedRef<SWidget> CreateEffectWidget(USoundEffectPreset* InPreset) override;
	virtual FVector2D GetDefaultWindowSize() const override { return FVector2D(640, 420); }
	virtual FText               GetEditorTitle() const override { return FText::FromString(TEXT("Source Conv. Reverb")); }
	virtual bool ShouldShowMeters() const override { return false; }
	virtual bool ShouldShowBypass() const override { return true; }
	virtual bool GetBypassState() const override;
	virtual void SetBypassState(bool bBypassed) override;
};
