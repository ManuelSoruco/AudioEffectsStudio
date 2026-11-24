// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "SourceEffects/AudioFXStereoDelayEditor.h"
#include "Shared/Core/AudioFXTheme.h"
#include "Shared/UIBuilders/AudioFXStereoDelayUI.h"
#include "SourceEffects/SourceEffectStereoDelay.h"

TSharedRef<SWidget> FAudioFXStereoDelayEditor::CreateEffectWidget(USoundEffectPreset* InPreset)
{
	USourceEffectStereoDelayPreset* Preset = Cast<USourceEffectStereoDelayPreset>(InPreset);
	if (!Preset)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("AudioEffectsStudio: Cast to preset type failed for '%s'. Check asset registration."),
			InPreset ? *InPreset->GetClass()->GetName() : TEXT("null"));
		return FAudioFXEffectEditor::CreateEffectWidget(InPreset);
	}

	return AudioFXStereoDelayUI::BuildWidget(*this, Preset, GetAssetColor());
}
