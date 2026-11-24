// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"

class FAudioFXEffectEditor;
class USoundEffectPreset;

/**
 * AudioFXEditorFactory
 *
 * Single source of truth for mapping a USoundEffectPreset concrete class
 * to its FAudioFXEffectEditor subclass. Used by FAudioFXAssetTypeActions_Preset
 * to resolve which editor window to open on double-click.
 */
namespace AudioFXEditorFactory
{
	/**
	 * Create the appropriate editor for the given preset.
	 * @param Preset  The preset to open an editor for. Must not be null.
	 * @return The new editor, or an invalid pointer if the preset type has no
	 *         custom editor (caller should fall back to the default Details panel).
	 *
	 * Covers all 16 source effects, 11 submix effects, and internal effects.
	 */
	TSharedPtr<FAudioFXEffectEditor> CreateEditorForPreset(USoundEffectPreset* Preset);
}
