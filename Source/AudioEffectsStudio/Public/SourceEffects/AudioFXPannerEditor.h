// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "Shared/Core/AudioFXEffectEditor.h"

/**
 * Panner effect editor — Spread (standard) + Pan (hero) knobs.
 * Asset color: #7F9B65 (muted green).
 */
class AUDIOEFFECTSSTUDIO_API FAudioFXPannerEditor : public FAudioFXEffectEditor
{
protected:
	virtual TSharedRef<SWidget> CreateEffectWidget(USoundEffectPreset* InPreset) override;
	virtual FVector2D GetDefaultWindowSize() const override { return FVector2D(480, 410); }
	virtual FText               GetEditorTitle() const override { return FText::FromString(TEXT("Panner")); }
};
