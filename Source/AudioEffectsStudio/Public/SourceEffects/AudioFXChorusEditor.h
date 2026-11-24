// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "Shared/Core/AudioFXEffectEditor.h"

/**
 * Chorus — 6 knobs in a row: Depth, Frequency, Feedback, Wet, Dry, Spread.
 * Asset color: #665579 (dark purple).
 */
class AUDIOEFFECTSSTUDIO_API FAudioFXChorusEditor : public FAudioFXEffectEditor
{
protected:
	virtual TSharedRef<SWidget> CreateEffectWidget(USoundEffectPreset* InPreset) override;
	virtual FVector2D GetDefaultWindowSize() const override { return FVector2D(700, 360); }
	virtual FText               GetEditorTitle() const override { return FText::FromString(TEXT("Chorus")); }
};
