// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "Shared/Core/AudioFXEffectEditor.h"
#include "SourceEffects/SourceEffectSimpleDelay.h"

class SAudioFXLabeledKnob;

/**
 * Source Simple Delay editor.
 *
 * Layout (Template 2: Graph + Knobs):
 *   Graph (≥200px, fill-width) on top
 *   [Distance | Manual] mode strip + conditional [Dist. Override] toggle — centered options row
 *   Knob row: [Feedback Primary] [Speed/Delay Hero] [Wet/Dry Secondary stacked]
 *
 * Asset color: (100, 165, 85) — green
 */
class AUDIOEFFECTSSTUDIO_API FAudioFXSimpleDelayEditor : public FAudioFXEffectEditor
{
protected:
	virtual TSharedRef<SWidget> CreateEffectWidget(USoundEffectPreset* InPreset) override;
	virtual FVector2D GetDefaultWindowSize() const override { return FVector2D(640, 480); }
	virtual FText GetEditorTitle() const override { return FText::FromString(TEXT("Source Delay")); }

private:
	/** Rebuild the whole inner widget when mode changes. */
	void RebuildWidget();

	TWeakObjectPtr<USourceEffectSimpleDelayPreset> WeakPreset;
	TSharedPtr<SBox> RootBox;   // replaced on each rebuild

	// Stored knob ptr for SetOutputRange after construction
	TSharedPtr<SAudioFXLabeledKnob> HeroKnob;
	TSharedPtr<SAudioFXLabeledKnob> DryKnob;
	TSharedPtr<SAudioFXLabeledKnob> WetKnob;
	TSharedPtr<SAudioFXLabeledKnob> FeedbackKnob;
};
