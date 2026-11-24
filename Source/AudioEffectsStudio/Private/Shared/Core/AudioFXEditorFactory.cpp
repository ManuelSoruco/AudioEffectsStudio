// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

/**
 * @file AudioFXEditorFactory.cpp
 * @brief Dispatch table mapping each USoundEffectPreset concrete class to its
 *        FAudioFXEffectEditor subclass.
 *
 * This is the single source of truth for which editor opens for which preset.
 * Adding support for a new preset type requires:
 *   1. Adding its editor header include (Source or Submix Effects section below).
 *   2. Adding its preset type header include.
 *   3. Adding a single `if (C == UXxxPreset::StaticClass())` dispatch line.
 *   4. Adding the class to GetAllPresetClassesForActions() in AudioEffectsStudioModule.cpp.
 */

#include "Shared/Core/AudioFXEditorFactory.h"

// ============================================================
// Source Effect Editors (16)
// ============================================================
#include "SourceEffects/AudioFXBitCrusherEditor.h"
#include "SourceEffects/AudioFXChorusEditor.h"
#include "SourceEffects/AudioFXConvReverbSourceEditor.h"
#include "SourceEffects/AudioFXDynamicsProcessorEditor.h"
#include "SourceEffects/AudioFXEnvelopeFollowerEditor.h"
#include "SourceEffects/AudioFXEQEditor.h"
#include "SourceEffects/AudioFXSourceFilterEditor.h"
#include "SourceEffects/AudioFXFoldbackDistortionEditor.h"
#include "SourceEffects/AudioFXMidSideSpreaderEditor.h"
#include "SourceEffects/AudioFXMotionFilterEditor.h"
#include "SourceEffects/AudioFXPannerEditor.h"
#include "SourceEffects/AudioFXPhaserEditor.h"
#include "SourceEffects/AudioFXRingModulationEditor.h"
#include "SourceEffects/AudioFXSimpleDelayEditor.h"
#include "SourceEffects/AudioFXStereoDelayEditor.h"
#include "SourceEffects/AudioFXWaveShaperEditor.h"

// ============================================================
// Submix Effect Editors (11)
// ============================================================
#include "SubmixEffects/AudioFXFlexiverbEditor.h"
#include "SubmixEffects/AudioFXSubmixConvReverbEditor.h"
#include "SubmixEffects/AudioFXSubmixDelayEditor.h"
#include "SubmixEffects/AudioFXSubmixDynamicsEditor.h"
#include "SubmixEffects/AudioFXSubmixEQEditor.h"
#include "SubmixEffects/AudioFXSubmixFilterEditor.h"
#include "SubmixEffects/AudioFXSubmixMultibandEditor.h"
#include "SubmixEffects/AudioFXSubmixReverbEditor.h"
#include "SubmixEffects/AudioFXSubmixStereoDelayEditor.h"
#include "SubmixEffects/AudioFXSubmixStereoToQuadEditor.h"
#include "SubmixEffects/AudioFXTapDelayEditor.h"

// ============================================================
// Internal Effect Editor
// ============================================================
// Internal effect editor (for plugin-managed presets)
#include "Shared/Core/AudioFXInternalEffectEditor.h"

// ============================================================
// Source Effect Preset Types (16)
// ============================================================
#include "SourceEffects/SourceEffectBitCrusher.h"
#include "SourceEffects/SourceEffectChorus.h"
#include "SourceEffects/SourceEffectConvolutionReverb.h"
#include "SourceEffects/SourceEffectDynamicsProcessor.h"
#include "SourceEffects/SourceEffectEnvelopeFollower.h"
#include "SourceEffects/SourceEffectEQ.h"
#include "SourceEffects/SourceEffectFilter.h"
#include "SourceEffects/SourceEffectFoldbackDistortion.h"
#include "SourceEffects/SourceEffectMidSideSpreader.h"
#include "SourceEffects/SourceEffectMotionFilter.h"
#include "SourceEffects/SourceEffectPanner.h"
#include "SourceEffects/SourceEffectPhaser.h"
#include "SourceEffects/SourceEffectRingModulation.h"
#include "SourceEffects/SourceEffectSimpleDelay.h"
#include "SourceEffects/SourceEffectStereoDelay.h"
#include "SourceEffects/SourceEffectWaveShaper.h"

// ============================================================
// Submix Effect Preset Types (11)
// ============================================================
#include "SubmixEffects/SubmixEffectConvolutionReverb.h"
#include "SubmixEffects/SubmixEffectDelay.h"
#include "SubmixEffects/SubmixEffectFilter.h"
#include "SubmixEffects/SubmixEffectFlexiverb.h"
#include "SubmixEffects/SubmixEffectMultiBandCompressor.h"
#include "SubmixEffects/SubmixEffectStereoDelay.h"
#include "SubmixEffects/SubmixEffectStereoToQuad.h"
#include "SubmixEffects/SubmixEffectTapDelay.h"
#include "SubmixEffects/AudioMixerSubmixEffectDynamicsProcessor.h"
#include "SubmixEffects/AudioMixerSubmixEffectEQ.h"
#include "SubmixEffects/AudioMixerSubmixEffectReverb.h"

// ============================================================
// Internal Plugin Preset Types
// ============================================================
// Internal plugin preset types
#include "Shared/Effects/AudioFXSubmixGainReductionCapture.h"
#include "Shared/Effects/AudioFXSourceGainReductionCapture.h"

#include "Sound/SoundEffectPreset.h"

// ============================================================
// Factory Implementation
// ============================================================

TSharedPtr<FAudioFXEffectEditor> AudioFXEditorFactory::CreateEditorForPreset(USoundEffectPreset* Preset)
{
	if (!Preset)
	{
		return nullptr;
	}

	const UClass* C = Preset->GetClass();

	// Source Effects (16)
	if (C == USourceEffectBitCrusherPreset::StaticClass())         { return MakeShared<FAudioFXBitCrusherEditor>(); }
	if (C == USourceEffectChorusPreset::StaticClass())             { return MakeShared<FAudioFXChorusEditor>(); }
	if (C == USourceEffectConvolutionReverbPreset::StaticClass())  { return MakeShared<FAudioFXConvReverbSourceEditor>(); }
	if (C == USourceEffectDynamicsProcessorPreset::StaticClass())  { return MakeShared<FAudioFXDynamicsProcessorEditor>(); }
	if (C == USourceEffectEnvelopeFollowerPreset::StaticClass())   { return MakeShared<FAudioFXEnvelopeFollowerEditor>(); }
	if (C == USourceEffectEQPreset::StaticClass())                 { return MakeShared<FAudioFXEQEditor>(); }
	if (C == USourceEffectFilterPreset::StaticClass())             { return MakeShared<FAudioFXSourceFilterEditor>(); }
	if (C == USourceEffectFoldbackDistortionPreset::StaticClass()) { return MakeShared<FAudioFXFoldbackDistortionEditor>(); }
	if (C == USourceEffectMidSideSpreaderPreset::StaticClass())    { return MakeShared<FAudioFXMidSideSpreaderEditor>(); }
	if (C == USourceEffectMotionFilterPreset::StaticClass())       { return MakeShared<FAudioFXMotionFilterEditor>(); }
	if (C == USourceEffectPannerPreset::StaticClass())             { return MakeShared<FAudioFXPannerEditor>(); }
	if (C == USourceEffectPhaserPreset::StaticClass())             { return MakeShared<FAudioFXPhaserEditor>(); }
	if (C == USourceEffectRingModulationPreset::StaticClass())     { return MakeShared<FAudioFXRingModulationEditor>(); }
	if (C == USourceEffectSimpleDelayPreset::StaticClass())        { return MakeShared<FAudioFXSimpleDelayEditor>(); }
	if (C == USourceEffectStereoDelayPreset::StaticClass())        { return MakeShared<FAudioFXStereoDelayEditor>(); }
	if (C == USourceEffectWaveShaperPreset::StaticClass())         { return MakeShared<FAudioFXWaveShaperEditor>(); }

	// Submix Effects (11)
	if (C == USubmixEffectConvolutionReverbPreset::StaticClass())      { return MakeShared<FAudioFXSubmixConvReverbEditor>(); }
	if (C == USubmixEffectDelayPreset::StaticClass())                  { return MakeShared<FAudioFXSubmixDelayEditor>(); }
	if (C == USubmixEffectDynamicsProcessorPreset::StaticClass())      { return MakeShared<FAudioFXSubmixDynamicsEditor>(); }
	if (C == USubmixEffectFilterPreset::StaticClass())                 { return MakeShared<FAudioFXSubmixFilterEditor>(); }
	if (C == USubmixEffectFlexiverbPreset::StaticClass())              { return MakeShared<FAudioFXFlexiverbEditor>(); }
	if (C == USubmixEffectMultibandCompressorPreset::StaticClass())    { return MakeShared<FAudioFXSubmixMultibandEditor>(); }
	if (C == USubmixEffectReverbPreset::StaticClass())                 { return MakeShared<FAudioFXSubmixReverbEditor>(); }
	if (C == USubmixEffectStereoDelayPreset::StaticClass())            { return MakeShared<FAudioFXSubmixStereoDelayEditor>(); }
	if (C == USubmixEffectStereoToQuadPreset::StaticClass())           { return MakeShared<FAudioFXSubmixStereoToQuadEditor>(); }
	if (C == USubmixEffectSubmixEQPreset::StaticClass())               { return MakeShared<FAudioFXSubmixEQEditor>(); }
	if (C == USubmixEffectTapDelayPreset::StaticClass())               { return MakeShared<FAudioFXTapDelayEditor>(); }

	// Internal (plugin-managed) presets
	if (C == UAudioFXSubmixGainReductionCapturePreset::StaticClass())  { return MakeShared<FAudioFXInternalEffectEditor>(); }
	if (C == UAudioFXSourceGainReductionCapturePreset::StaticClass())  { return MakeShared<FAudioFXInternalEffectEditor>(); }

	// Unknown/future type — return null so caller falls back to default editor
	return nullptr;
}
