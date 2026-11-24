// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "Shared/Core/AudioFXEffectEditor.h"
#include "Shared/Core/AudioFXTheme.h"

/**
 * FAudioFXSubmixFilterEditor
 *
 * Filter editor for USubmixEffectFilterPreset.
 * Fills FFilterUIParams and delegates UI to AudioFXFilterUI::BuildWidget.
 *
 * Controls:
 *   · Algorithm (Circuit) strip — 1-Pole / State Var / Ladder
 *   · Type strip               — Low-Pass / High-Pass / Band-Pass / Band-Stop
 *   · SAudioFXFilterCurve visualizer (driven by GetCurveType / GetCutoffHz / GetQ)
 *   · Cutoff (hero, log-space) + Resonance knobs
 */
class AUDIOEFFECTSSTUDIO_API FAudioFXSubmixFilterEditor : public FAudioFXEffectEditor
{
protected:
	virtual TSharedRef<SWidget> CreateEffectWidget(USoundEffectPreset* InPreset) override;
    virtual FVector2D           GetDefaultWindowSize() const override { return FVector2D(560, 430); }
	virtual FText               GetEditorTitle() const override { return FText::FromString(TEXT("Submix Filter")); }
};
