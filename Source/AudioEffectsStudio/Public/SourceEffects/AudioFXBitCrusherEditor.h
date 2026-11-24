// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "Shared/Core/AudioFXEffectEditor.h"

/**
 * BitCrusher effect editor — 2 hero knobs: Sample Rate + Bit Depth.
 * Asset color: #C4B979 (golden tan).
 */
class AUDIOEFFECTSSTUDIO_API FAudioFXBitCrusherEditor : public FAudioFXEffectEditor
{
protected:
	virtual TSharedRef<SWidget> CreateEffectWidget(USoundEffectPreset* InPreset) override;
	virtual FVector2D GetDefaultWindowSize() const override { return FVector2D(460, 310); }
	virtual FText               GetEditorTitle() const override { return FText::FromString(TEXT("Bit Crusher")); }
};
