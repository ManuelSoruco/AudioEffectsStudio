// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Shared/Core/AudioFXEffectEditor.h"

class USubmixEffectStereoDelayPreset;
class USourceEffectStereoDelayPreset;

/**
 * AudioFXStereoDelayUI
 *
 * Shared UI builder for the Stereo Delay skin.
 * Used by both AudioFXStereoDelayEditor (Source) and AudioFXSubmixStereoDelayEditor (Submix).
 *
 * FSubmixEffectStereoDelaySettings and FSourceEffectStereoDelaySettings have identical fields
 * (DelayMode, DelayTimeMsec, Feedback, DelayRatio, WetLevel, DryLevel, bFilterEnabled,
 *  FilterType, FilterFrequency, FilterQ) so both overloads share the same implementation.
 */
namespace AudioFXStereoDelayUI
{
	/** Build from Submix preset. */
	TSharedRef<SWidget> BuildWidget(
		FAudioFXEffectEditor& Editor,
		USubmixEffectStereoDelayPreset* Preset,
		const FLinearColor& AssetCol);

	/** Build from Source preset. */
	TSharedRef<SWidget> BuildWidget(
		FAudioFXEffectEditor& Editor,
		USourceEffectStereoDelayPreset* Preset,
		const FLinearColor& AssetCol);
}
