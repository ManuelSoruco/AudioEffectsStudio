// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "Shared/Core/AudioFXEffectEditor.h"
#include "Shared/UIBuilders/AudioFXEQUI.h"

/**
 * Parametric EQ — horizontal rack: one band card per FSourceEffectEQBand.
 * Uses shared AudioFXEQUI builder.
 * Asset color: #359E99 (teal-green).
 */
class AUDIOEFFECTSSTUDIO_API FAudioFXEQEditor : public FAudioFXEffectEditor
{
protected:
	virtual TSharedRef<SWidget> CreateEffectWidget(USoundEffectPreset* InPreset) override;
	virtual FVector2D GetDefaultWindowSize() const override { return FVector2D(900, 700); }
	virtual FText               GetEditorTitle() const override { return FText::FromString(TEXT("Source EQ")); }
	virtual EVerticalAlignment GetContentVAlign() const override { return VAlign_Top; }

private:
	TSharedPtr<FEQUIState> EQState;
};
