// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "Shared/Core/AudioFXEffectEditor.h"
#include "Shared/Core/AudioFXTheme.h"

/**
 * FAudioFXSubmixDynamicsEditor
 *
 * Dynamics Processor editor for USubmixEffectDynamicsProcessorPreset.
 * Delegates all UI construction to the shared AudioFXDynamicsUI::BuildWidget.
 *
 * Key differences from the source-effect variant:
 *   · Attack range: 1–300ms (source uses 1–200ms)
 *   · Output gain range: -60 to +30 dB
 *   · Sidechain section is enabled: includes key source, audio bus / submix pickers,
 *     key audition toggle, and sidechain high/lowshelf filters
 *   · Bypass lives in Settings.bBypass (hardware bypass, not UI-only)
 */
class AUDIOEFFECTSSTUDIO_API FAudioFXSubmixDynamicsEditor : public FAudioFXEffectEditor
{
public:
	virtual TSharedRef<SWidget> CreateEffectWidget(USoundEffectPreset* InPreset) override;
	virtual FVector2D           GetDefaultWindowSize() const override { return FVector2D(840, 560); }
	virtual FText               GetEditorTitle() const override { return FText::FromString(TEXT("Submix Dynamics")); }
	virtual bool                ShouldShowMeters() const override { return false; }
	virtual bool                ShouldShowGRMeter() const override { return false; }
};
