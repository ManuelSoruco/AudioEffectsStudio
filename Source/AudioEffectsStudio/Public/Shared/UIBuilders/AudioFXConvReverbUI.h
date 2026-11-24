// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Shared/Core/AudioFXEffectEditor.h"
#include "SubmixEffects/SubmixEffectConvolutionReverb.h"

/**
 * AudioFXConvReverbUI
 *
 * Shared UI builder for the Convolution Reverb skin — Submix variant.
 * The Source variant uses its own editor (FAudioFXConvReverbSourceEditor) which
 * handles FSourceEffectConvolutionReverbSettings (different struct with Normalization).
 * This builder handles FSubmixEffectConvolutionReverbSettings.
 */

// ============================================================
// Builder
// ============================================================

namespace AudioFXConvReverbUI
{
	/**
	 * Build the ConvReverb widget for the Submix preset.
	 * @param Editor    The FAudioFXEffectEditor this is building for (used for StoreKnobStyle).
	 * @param Preset    The USubmixEffectConvolutionReverbPreset being edited.
	 * @param AssetCol  The asset color from GetAssetColor().
	 * @return          The complete Slate widget tree.
	 */
	TSharedRef<SWidget> BuildWidget(
		FAudioFXEffectEditor& Editor,
		USubmixEffectConvolutionReverbPreset* Preset,
		const FLinearColor& AssetCol);
}
