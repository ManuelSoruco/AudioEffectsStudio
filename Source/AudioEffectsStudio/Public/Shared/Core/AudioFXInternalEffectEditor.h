// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "Shared/Core/AudioFXEffectEditor.h"
#include "Shared/Core/AudioFXTheme.h"

/**
 * FAudioFXInternalEffectEditor
 *
 * Editor skin shown when users open a preset that is managed internally
 * by Audio Effects Studio (e.g. GainReductionCapture effects).
 *
 * Displays a styled informational window explaining that the effect is
 * not intended for direct use and is automatically managed by the plugin.
 */
class AUDIOEFFECTSSTUDIO_API FAudioFXInternalEffectEditor : public FAudioFXEffectEditor
{
protected:
	virtual TSharedRef<SWidget> CreateEffectWidget(USoundEffectPreset* InPreset) override;
	virtual FLinearColor        GetAssetColor() const override { return FAudioFXTheme::GetAccentWarn(); }
	virtual FVector2D           GetDefaultWindowSize() const override { return FVector2D(460, 260); }
};
