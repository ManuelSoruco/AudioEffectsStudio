// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "AssetTypeActions_Base.h"
#include "Sound/SoundEffectPreset.h"
#include "UObject/StrongObjectPtr.h"

class FAudioFXEffectEditor;

/**
 * Custom IAssetTypeActions for all 27 AudioFX SoundEffect preset classes.
 *
 * One instance is created per preset class and registered in
 * FAudioEffectsStudioModule::StartupModule(), displacing the engine's default
 * action for that class so that double-click opens FAudioFXEffectEditor instead
 * of the generic Unreal Details panel.
 *
 * Ctrl+double-click falls through to the base class, opening the standard panel.
 * Window lifecycle is tracked via the OpenEditors map so that double-clicking
 * an already-open preset brings the existing window to front rather than
 * spawning a duplicate.
 */
class AUDIOEFFECTSSTUDIO_API FAudioFXAssetTypeActions_Preset : public FAssetTypeActions_Base
{
public:
	explicit FAudioFXAssetTypeActions_Preset(USoundEffectPreset* InEffectPreset);

	virtual bool CanFilter() override;
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual const TArray<FText>& GetSubMenus() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> ToolkitHost) override;

private:
	/** The preset CDO this action instance was registered for. */
	TStrongObjectPtr<USoundEffectPreset> EffectPreset;

	/** Tracks open editor windows per preset to allow bring-to-front on re-open. */
	mutable TMap<TWeakObjectPtr<USoundEffectPreset>, TSharedPtr<FAudioFXEffectEditor>> OpenEditors;
};
