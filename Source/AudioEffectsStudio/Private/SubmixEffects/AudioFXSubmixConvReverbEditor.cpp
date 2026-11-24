// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

/**
 * FAudioFXSubmixConvReverbEditor
 *
 * Thin editor shell for USubmixEffectConvolutionReverbPreset.
 * All UI construction is delegated to AudioFXConvReverbUI::BuildWidget.
 * This class only handles bypass state read/write and preset validation.
 */

#include "SubmixEffects/AudioFXSubmixConvReverbEditor.h"

#include "Shared/Core/AudioFXTheme.h"
#include "Shared/UIBuilders/AudioFXConvReverbUI.h"
#include "SubmixEffects/SubmixEffectConvolutionReverb.h"

#define LOCTEXT_NAMESPACE "AudioFXSubmixConvReverb"

// ── Bypass passthrough ───────────────────────────────────────────────────────

bool FAudioFXSubmixConvReverbEditor::GetBypassState() const
{
	if (const USubmixEffectConvolutionReverbPreset* Preset = Cast<USubmixEffectConvolutionReverbPreset>(GetPreset()))
	{
		return Preset->Settings.bBypass;
	}
	return false;
}

void FAudioFXSubmixConvReverbEditor::SetBypassState(bool bBypassed)
{
	if (USubmixEffectConvolutionReverbPreset* Preset = Cast<USubmixEffectConvolutionReverbPreset>(GetPreset()))
	{
		Preset->Settings.bBypass = bBypassed;
		Preset->SetSettings(Preset->Settings);
	}
}

// ── Widget construction ──────────────────────────────────────────────────────

TSharedRef<SWidget> FAudioFXSubmixConvReverbEditor::CreateEffectWidget(USoundEffectPreset* InPreset)
{
	USubmixEffectConvolutionReverbPreset* Preset = Cast<USubmixEffectConvolutionReverbPreset>(InPreset);
	if (!Preset)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("AudioEffectsStudio: Cast to preset type failed for '%s'. Check asset registration."),
			InPreset ? *InPreset->GetClass()->GetName() : TEXT("null"));
		return FAudioFXEffectEditor::CreateEffectWidget(InPreset);
	}

	return AudioFXConvReverbUI::BuildWidget(*this, Preset, GetAssetColor());
}

#undef LOCTEXT_NAMESPACE
