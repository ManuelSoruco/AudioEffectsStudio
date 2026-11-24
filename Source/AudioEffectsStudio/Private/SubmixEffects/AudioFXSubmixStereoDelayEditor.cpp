// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

/**
 * FAudioFXSubmixStereoDelayEditor
 *
 * Thin editor shell for USubmixEffectStereoDelayPreset.
 * All UI construction is delegated to AudioFXStereoDelayUI::BuildWidget,
 * which is shared with the source-effect stereo delay editor.
 */

#include "SubmixEffects/AudioFXSubmixStereoDelayEditor.h"

#include "Shared/Core/AudioFXTheme.h"
#include "Shared/UIBuilders/AudioFXStereoDelayUI.h"
#include "SubmixEffects/SubmixEffectStereoDelay.h"

#define LOCTEXT_NAMESPACE "AudioFXSubmixStereoDelay"

// ── Widget construction ──────────────────────────────────────────────────────

TSharedRef<SWidget> FAudioFXSubmixStereoDelayEditor::CreateEffectWidget(USoundEffectPreset* InPreset)
{
	USubmixEffectStereoDelayPreset* Preset = Cast<USubmixEffectStereoDelayPreset>(InPreset);
	if (!Preset)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("AudioEffectsStudio: Cast to preset type failed for '%s'. Check asset registration."),
			InPreset ? *InPreset->GetClass()->GetName() : TEXT("null"));
		return FAudioFXEffectEditor::CreateEffectWidget(InPreset);
	}

	return AudioFXStereoDelayUI::BuildWidget(*this, Preset, GetAssetColor());
}

#undef LOCTEXT_NAMESPACE
