// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

/**
 * @file AssetTypeActions_AudioFXPreset.cpp
 * @brief IAssetTypeActions implementation for AudioFX preset assets.
 *
 * Each instance of FAudioFXAssetTypeActions_Preset handles one concrete
 * USoundEffectPreset subclass. The module registers 27 instances (16 source +
 * 11 submix) after displacing the engine defaults for those classes.
 */

#include "AssetTypeActions_AudioFXPreset.h"
#include "Shared/Core/AudioFXEffectEditor.h"
#include "Shared/Core/AudioFXEditorFactory.h"
#include "Framework/Application/SlateApplication.h"
#include "Sound/SoundEffectPreset.h"

namespace AudioFXAssetTypeActions
{
	const TArray<FText> SubMenus
	{
		NSLOCTEXT("AssetTypeActions", "AssetEffectSubMenu", "Effects")
	};
}

FAudioFXAssetTypeActions_Preset::FAudioFXAssetTypeActions_Preset(USoundEffectPreset* InEffectPreset)
	: EffectPreset(InEffectPreset)
{
}

bool FAudioFXAssetTypeActions_Preset::CanFilter()
{
	return EffectPreset.IsValid() ? EffectPreset->CanFilter() : true;
}

FText FAudioFXAssetTypeActions_Preset::GetName() const
{
	if (EffectPreset.IsValid())
	{
		const FText Name = EffectPreset->GetAssetActionName();
		if (!Name.IsEmpty())
		{
			return Name;
		}
	}
	return NSLOCTEXT("AudioFX", "EffectPresetFallbackName", "Effect Preset");
}

FColor FAudioFXAssetTypeActions_Preset::GetTypeColor() const
{
	return EffectPreset.IsValid() ? EffectPreset->GetPresetColor() : FColor(200, 100, 100);
}

const TArray<FText>& FAudioFXAssetTypeActions_Preset::GetSubMenus() const
{
	return AudioFXAssetTypeActions::SubMenus;
}

UClass* FAudioFXAssetTypeActions_Preset::GetSupportedClass() const
{
	if (EffectPreset.IsValid())
	{
		if (UClass* Supported = EffectPreset->GetSupportedClass())
		{
			return Supported;
		}
		return EffectPreset->GetClass();
	}
	return USoundEffectPreset::StaticClass();
}

uint32 FAudioFXAssetTypeActions_Preset::GetCategories()
{
	return EAssetTypeCategories::Sounds;
}

void FAudioFXAssetTypeActions_Preset::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> ToolkitHost)
{
	// Ctrl+double-click: fall through to the generic Details panel.
	if (FSlateApplication::IsInitialized() && FSlateApplication::Get().GetModifierKeys().IsControlDown())
	{
		FAssetTypeActions_Base::OpenAssetEditor(InObjects, ToolkitHost);
		return;
	}

	// Purge stale entries (closed or GC'd windows) before searching for existing ones.
	for (auto It = OpenEditors.CreateIterator(); It; ++It)
	{
		if (!It->Key.IsValid() || !It->Value.IsValid() || !It->Value->IsWindowOpen())
		{
			It.RemoveCurrent();
		}
	}

	TArray<UObject*> FallbackObjects;
	for (UObject* Obj : InObjects)
	{
		USoundEffectPreset* Preset = Cast<USoundEffectPreset>(Obj);
		if (!Preset)
		{
			FallbackObjects.Add(Obj);
			continue;
		}

		// Bring existing editor to front if already open.
		if (TSharedPtr<FAudioFXEffectEditor>* Existing = OpenEditors.Find(Preset))
		{
			if (Existing->IsValid() && (*Existing)->IsWindowOpen())
			{
				(*Existing)->BringToFront();
				continue;
			}
		}

		// Create a new AudioFX editor window for this preset.
		TSharedPtr<FAudioFXEffectEditor> Editor = AudioFXEditorFactory::CreateEditorForPreset(Preset);
		if (!Editor.IsValid())
		{
			// Unknown preset type: fall back to the generic Details panel.
			FallbackObjects.Add(Obj);
			continue;
		}

		OpenEditors.Add(Preset, Editor);
		Editor->InitEffectEditor(Preset);
	}

	if (!FallbackObjects.IsEmpty())
	{
		FAssetTypeActions_Base::OpenAssetEditor(FallbackObjects, ToolkitHost);
	}
}
