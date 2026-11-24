// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

/**
 * @file AudioEffectsStudioModule.cpp
 * @brief Module entry point for AudioEffectsStudio.
 *
 * On startup this module:
 *   1. Suppresses UE's RestorePreviouslyOpenAssets for SoundEffectPreset types,
 *      preventing ghost editor windows appearing on editor launch.
 *   2. Displaces the engine-default IAssetTypeActions for all 27 preset classes.
 *   3. Registers AudioFX-skinned IAssetTypeActions that route double-click to
 *      FAudioFXEffectEditor instead of the generic Details panel.
 *
 * On shutdown the displaced engine actions are restored and AudioFX actions removed.
 */

#include "AudioEffectsStudioModule.h"
#include "Modules/ModuleManager.h"
#include "Shared/Core/AudioFXTheme.h"  // DECLARE_LOG_CATEGORY_EXTERN(LogAudioEffectsStudio)

#if WITH_EDITOR
#include "AssetTypeActions_AudioFXPreset.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Sound/SoundEffectPreset.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Editor.h"

// -- Source effect preset types (needed for GetAllPresetClassesForActions) --
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

// -- Submix effect preset types (needed for GetAllPresetClassesForActions) --
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

#endif // WITH_EDITOR

#define LOCTEXT_NAMESPACE "AudioEffectsStudio"

DEFINE_LOG_CATEGORY(LogAudioEffectsStudio);

#if WITH_EDITOR

/**
 * Builds the complete list of all 27 SoundEffect preset classes that AudioFX
 * provides custom editor windows for. Used during module startup/shutdown to
 * displace and restore the engine's default IAssetTypeActions.
 */
static TArray<UClass*> GetAllPresetClassesForActions()
{
	return {
		USourceEffectBitCrusherPreset::StaticClass(),
		USourceEffectChorusPreset::StaticClass(),
		USourceEffectConvolutionReverbPreset::StaticClass(),
		USourceEffectDynamicsProcessorPreset::StaticClass(),
		USourceEffectEnvelopeFollowerPreset::StaticClass(),
		USourceEffectEQPreset::StaticClass(),
		USourceEffectFilterPreset::StaticClass(),
		USourceEffectFoldbackDistortionPreset::StaticClass(),
		USourceEffectMidSideSpreaderPreset::StaticClass(),
		USourceEffectMotionFilterPreset::StaticClass(),
		USourceEffectPannerPreset::StaticClass(),
		USourceEffectPhaserPreset::StaticClass(),
		USourceEffectRingModulationPreset::StaticClass(),
		USourceEffectSimpleDelayPreset::StaticClass(),
		USourceEffectStereoDelayPreset::StaticClass(),
		USourceEffectWaveShaperPreset::StaticClass(),
		USubmixEffectConvolutionReverbPreset::StaticClass(),
		USubmixEffectDelayPreset::StaticClass(),
		USubmixEffectDynamicsProcessorPreset::StaticClass(),
		USubmixEffectFilterPreset::StaticClass(),
		USubmixEffectFlexiverbPreset::StaticClass(),
		USubmixEffectMultibandCompressorPreset::StaticClass(),
		USubmixEffectReverbPreset::StaticClass(),
		USubmixEffectStereoDelayPreset::StaticClass(),
		USubmixEffectStereoToQuadPreset::StaticClass(),
		USubmixEffectSubmixEQPreset::StaticClass(),
		USubmixEffectTapDelayPreset::StaticClass(),
	};
}

#endif // WITH_EDITOR

// ----------------------------------------------------------------------------

void FAudioEffectsStudioModule::StartupModule()
{
	UE_LOG(LogAudioEffectsStudio, Log, TEXT("Audio Effects Studio: StartupModule -- registering custom IAssetTypeActions for 27 SoundEffect preset classes."));

#if WITH_EDITOR
	// Plugin-internal fix: prevent UE's RestorePreviouslyOpenAssets from
	// re-opening our preset editors on startup. This fires one frame after
	// PostEngineInit, which is BEFORE the asset restore tick. We close any
	// assets that snuck in via the restore system so they don't pop up.
	// This is entirely plugin-internal -- no project .ini modification required.
	FCoreDelegates::OnPostEngineInit.AddLambda([]()
	{
		if (GEditor)
		{
			// Close editors for all USoundEffectPreset subclasses opened by restore.
			// The restore fires in the same frame as PostEngineInit so we defer
			// one tick to catch it.
			GEditor->GetTimerManager()->SetTimerForNextTick([]()
			{
				if (!GEditor) return;
				UAssetEditorSubsystem* AES = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
				if (!AES) return;

				// Close any SoundEffectPreset editors that were silently auto-restored.
				// Our custom OpenAssetEditor re-opens them properly on double-click --
				// we only want to prevent the unsolicited auto-restore pop.
				TArray<UObject*> OpenAssets = AES->GetAllEditedAssets();
				for (UObject* Asset : OpenAssets)
				{
					if (Asset && Asset->IsA<USoundEffectPreset>())
					{
						AES->CloseAllEditorsForAsset(Asset);
						UE_LOG(LogAudioEffectsStudio, Log,
							TEXT("Audio Effects Studio: Suppressed auto-restore for '%s'"),
							*Asset->GetName());
					}
				}
			});
		}
	});

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	const TArray<UClass*> TargetClasses = GetAllPresetClassesForActions();

	// Build a fast lookup set for the classes we will displace.
	TSet<const UClass*> TargetClassSet;
	for (const UClass* C : TargetClasses)
	{
		TargetClassSet.Add(C);
	}

	// Displace existing engine actions for our target classes so AudioFX
	// actions exclusively own their OpenAssetEditor() call.
	TArray<TWeakPtr<IAssetTypeActions>> ExistingActions;
	AssetTools.GetAssetTypeActionsList(ExistingActions);
	for (const TWeakPtr<IAssetTypeActions>& WeakAction : ExistingActions)
	{
		TSharedPtr<IAssetTypeActions> Action = WeakAction.Pin();
		if (!Action.IsValid())
		{
			continue;
		}

		const UClass* SupportedClass = Action->GetSupportedClass();
		if (SupportedClass && TargetClassSet.Contains(SupportedClass))
		{
			AssetTools.UnregisterAssetTypeActions(Action.ToSharedRef());
			DisplacedEngineAssetTypeActions.Add(Action);
		}
	}

	// Register our custom actions (same classes, AudioFX-skinned OpenAssetEditor path).
	for (UClass* PresetClass : TargetClasses)
	{
		USoundEffectPreset* CDO = PresetClass ? Cast<USoundEffectPreset>(PresetClass->GetDefaultObject()) : nullptr;
		if (!CDO)
		{
			continue;
		}

		TSharedPtr<IAssetTypeActions> Action = MakeShared<FAudioFXAssetTypeActions_Preset>(CDO);
		AssetTools.RegisterAssetTypeActions(Action.ToSharedRef());
		RegisteredAssetTypeActions.Add(Action);
	}

	UE_LOG(LogAudioEffectsStudio, Log,
		TEXT("Audio Effects Studio: Displaced %d engine actions, registered %d custom actions."),
		DisplacedEngineAssetTypeActions.Num(),
		RegisteredAssetTypeActions.Num());

#endif // WITH_EDITOR
}

void FAudioEffectsStudioModule::ShutdownModule()
{
	UE_LOG(LogAudioEffectsStudio, Log, TEXT("Audio Effects Studio: ShutdownModule."));

#if WITH_EDITOR
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();

		// Unregister all AudioFX custom actions.
		for (const TSharedPtr<IAssetTypeActions>& Action : RegisteredAssetTypeActions)
		{
			if (Action.IsValid())
			{
				AssetTools.UnregisterAssetTypeActions(Action.ToSharedRef());
			}
		}
		RegisteredAssetTypeActions.Empty();

		// Restore the engine actions we displaced on startup.
		for (const TSharedPtr<IAssetTypeActions>& Action : DisplacedEngineAssetTypeActions)
		{
			if (Action.IsValid())
			{
				AssetTools.RegisterAssetTypeActions(Action.ToSharedRef());
			}
		}
		DisplacedEngineAssetTypeActions.Empty();
	}
#endif // WITH_EDITOR
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAudioEffectsStudioModule, AudioEffectsStudio)
