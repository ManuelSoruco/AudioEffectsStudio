// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "Modules/ModuleManager.h"

class IAssetTypeActions;

/**
 * Primary module class for AudioEffectsStudio.
 *
 * On startup, displaces the engine's default IAssetTypeActions for all 27
 * SoundEffect preset classes and registers AudioFX-skinned replacements.
 * This ensures that double-clicking any preset opens the custom FAudioFXEffectEditor
 * window instead of the generic Details panel.
 *
 * On shutdown, unregisters AudioFX actions and restores the displaced engine actions.
 */
class FAudioEffectsStudioModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
#if WITH_EDITOR
	/** Custom AudioFX asset type actions we register for SoundEffect presets. */
	TArray<TSharedPtr<IAssetTypeActions>> RegisteredAssetTypeActions;

	/**
	 * Engine-default IAssetTypeActions that we displaced on startup.
	 * Restored in ShutdownModule so the engine returns to normal behavior when
	 * this plugin is unloaded.
	 */
	TArray<TSharedPtr<IAssetTypeActions>> DisplacedEngineAssetTypeActions;
#endif
};
