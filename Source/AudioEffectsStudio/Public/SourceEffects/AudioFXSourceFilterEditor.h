// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "Shared/Core/AudioFXEffectEditor.h"

/** Source Filter — Circuit/Type strips + Cutoff (log hero) + Q knob. Modulation section collapsed. */
class AUDIOEFFECTSSTUDIO_API FAudioFXSourceFilterEditor : public FAudioFXEffectEditor
{
protected:
	virtual TSharedRef<SWidget> CreateEffectWidget(USoundEffectPreset* InPreset) override;
    virtual FVector2D GetDefaultWindowSize() const override { return FVector2D(600, 430); }
	virtual FText               GetEditorTitle() const override { return FText::FromString(TEXT("Source Filter")); }
};
