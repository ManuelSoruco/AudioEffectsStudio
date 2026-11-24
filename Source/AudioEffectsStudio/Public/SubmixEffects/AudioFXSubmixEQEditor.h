// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "Shared/Core/AudioFXEffectEditor.h"
#include "Shared/UIBuilders/AudioFXEQUI.h"
#include "Shared/Core/AudioFXTheme.h"

/**
 * FAudioFXSubmixEQEditor
 *
 * Parametric EQ editor for USubmixEffectSubmixEQPreset.
 * Converts FSubmixEffectEQBand ↔ FSourceEffectEQBand for the shared builder,
 * then delegates all UI to AudioFXEQUI::BuildWidget.
 *
 * VAlign_Top is set so the band rack grows downward when bands are added.
 */
class AUDIOEFFECTSSTUDIO_API FAudioFXSubmixEQEditor : public FAudioFXEffectEditor
{
public:
	virtual TSharedRef<SWidget>  CreateEffectWidget(USoundEffectPreset* InPreset) override;
	virtual FVector2D            GetDefaultWindowSize() const override { return FVector2D(900, 700); }
	virtual FText               GetEditorTitle() const override { return FText::FromString(TEXT("Submix EQ")); }
	virtual EVerticalAlignment   GetContentVAlign() const override { return VAlign_Top; }

private:
	TSharedPtr<FEQUIState> EQState;
};
