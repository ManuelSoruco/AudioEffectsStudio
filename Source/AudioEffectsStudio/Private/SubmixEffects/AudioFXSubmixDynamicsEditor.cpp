// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

/**
 * FAudioFXSubmixDynamicsEditor
 *
 * Dynamics Processor editor for USubmixEffectDynamicsProcessorPreset.
 * Builds an FDynamicsUIParams struct and delegates all UI construction to
 * AudioFXDynamicsUI::BuildWidget (shared with the source-effect variant).
 *
 * Differences from the source version:
 *  - Submix type enum has no UpwardCompressor entry (5 options but mapped 0-4)
 *  - Attack range: 1–300ms (source uses a wider range)
 *  - Output gain range: -60 to +30 dB
 *  - Bypass lives in Settings.bBypass (real bypass toggle, not a UI-only flag)
 *  - Sidechain section is enabled with AudioBus and Submix pickers
 */

#include "SubmixEffects/AudioFXSubmixDynamicsEditor.h"

#include "Shared/UIBuilders/AudioFXDynamicsUI.h"
#include "Shared/Core/AudioFXTheme.h"
#include "SourceEffects/SourceEffectDynamicsProcessor.h"
#include "SubmixEffects/AudioMixerSubmixEffectDynamicsProcessor.h"

#include "AssetRegistry/AssetData.h"
#include "Sound/AudioBus.h"
#include "Sound/SoundSubmix.h"
#include "UObject/UnrealType.h"

#define LOCTEXT_NAMESPACE "AudioFXSubmixDynamics"

// ─────────────────────────────────────────────────────────────────────────────
//  Local helpers — enum conversions
// ─────────────────────────────────────────────────────────────────────────────

/** Convert the submix processor type to its source-effect equivalent for the transfer-curve widget. */
static ESourceEffectDynamicsProcessorType SD_ToSourceType(ESubmixEffectDynamicsProcessorType T)
{
	switch (T)
	{
	case ESubmixEffectDynamicsProcessorType::Compressor: return ESourceEffectDynamicsProcessorType::Compressor;
	case ESubmixEffectDynamicsProcessorType::Limiter:    return ESourceEffectDynamicsProcessorType::Limiter;
	case ESubmixEffectDynamicsProcessorType::Expander:   return ESourceEffectDynamicsProcessorType::Expander;
	case ESubmixEffectDynamicsProcessorType::Gate:       return ESourceEffectDynamicsProcessorType::Gate;
	default:                                             return ESourceEffectDynamicsProcessorType::Compressor;
	}
}

/** Returns true when the Ratio and Knee controls should be visible for this type. */
static bool SD_ShowRatio(ESubmixEffectDynamicsProcessorType T)
{
	return T != ESubmixEffectDynamicsProcessorType::Limiter
		&& T != ESubmixEffectDynamicsProcessorType::Gate;
}

// ─────────────────────────────────────────────────────────────────────────────
//  CreateEffectWidget — fills FDynamicsUIParams, then calls the shared builder
// ─────────────────────────────────────────────────────────────────────────────

TSharedRef<SWidget> FAudioFXSubmixDynamicsEditor::CreateEffectWidget(USoundEffectPreset* InPreset)
{
	USubmixEffectDynamicsProcessorPreset* Preset = Cast<USubmixEffectDynamicsProcessorPreset>(InPreset);
	if (!Preset)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("AudioEffectsStudio: Cast to preset type failed for '%s'. Check asset registration."),
			InPreset ? *InPreset->GetClass()->GetName() : TEXT("null"));
		return FAudioFXEffectEditor::CreateEffectWidget(InPreset);
	}

	const FLinearColor AssetCol = GetAssetColor();
	TWeakObjectPtr<USubmixEffectDynamicsProcessorPreset> WeakPreset = Preset;

	FDynamicsUIParams Params;

	// ── Transfer curve ────────────────────────────────────────────────
	// Converts submix settings to source-type proxy so the shared curve
	// widget can read DynamicsProcessorType, ThresholdDb, Ratio, Knee.
	Params.GetTransferSettings.BindLambda([WeakPreset]() -> FSourceEffectDynamicsProcessorSettings
	{
		FSourceEffectDynamicsProcessorSettings Out;
		if (USubmixEffectDynamicsProcessorPreset* P = WeakPreset.Get())
		{
			Out.DynamicsProcessorType = SD_ToSourceType(P->Settings.DynamicsProcessorType);
			Out.ThresholdDb           = P->Settings.ThresholdDb;
			Out.Ratio                 = P->Settings.Ratio;
			Out.KneeBandwidthDb       = P->Settings.KneeBandwidthDb;
		}
		return Out;
	});

	// ── Processor type strip ──────────────────────────────────────────
	// Submix has 4 types (no UpwardCompressor exposed in the engine UE5 submix effect).
	Params.TypeOptions = {
		LOCTEXT("Comp",   "Compressor"),
		LOCTEXT("Lim",    "Limiter"),
		LOCTEXT("Exp",    "Expander"),
		LOCTEXT("Gate",   "Gate"),
		LOCTEXT("UpComp", "Upward Compressor")
	};
	Params.GetType.BindLambda([WeakPreset]() -> int32
	{
		return WeakPreset.IsValid() ? (int32)WeakPreset->Settings.DynamicsProcessorType : 0;
	});
	Params.SetType.BindLambda([WeakPreset](int32 OptionIndex)
	{
		if (USubmixEffectDynamicsProcessorPreset* P = WeakPreset.Get())
		{
			P->Settings.DynamicsProcessorType = (ESubmixEffectDynamicsProcessorType)OptionIndex;
			P->SetSettings(P->Settings);
		}
	});

	Params.ShouldShowRatio = [](int32 T) { return SD_ShowRatio((ESubmixEffectDynamicsProcessorType)T); };
	Params.ShouldShowKnee  = [](int32 T) { return SD_ShowRatio((ESubmixEffectDynamicsProcessorType)T); };

	// ── Bypass (real settings toggle for submix, not UI-only) ─────────
	Params.bBypassInSettings = true;
	Params.GetBypass.BindLambda([WeakPreset]() -> bool
	{
		return WeakPreset.IsValid() && WeakPreset->Settings.bBypass;
	});
	Params.SetBypass.BindLambda([WeakPreset](bool bOn)
	{
		if (USubmixEffectDynamicsProcessorPreset* P = WeakPreset.Get())
		{
			P->Settings.bBypass = (uint8)bOn;
			P->SetSettings(P->Settings);
		}
	});

	// ── Core dynamics knobs ───────────────────────────────────────────
	Params.GetThreshold.BindLambda([WeakPreset]() -> float
	{
		return WeakPreset.IsValid() ? WeakPreset->Settings.ThresholdDb : -30.f;
	});
	Params.SetThreshold.BindLambda([WeakPreset](float NewValue)
	{
		if (USubmixEffectDynamicsProcessorPreset* P = WeakPreset.Get())
		{
			P->Settings.ThresholdDb = NewValue;
			P->SetSettings(P->Settings);
		}
	});

	Params.GetRatio.BindLambda([WeakPreset]() -> float
	{
		return WeakPreset.IsValid() ? WeakPreset->Settings.Ratio : 4.f;
	});
	Params.SetRatio.BindLambda([WeakPreset](float NewValue)
	{
		if (USubmixEffectDynamicsProcessorPreset* P = WeakPreset.Get())
		{
			P->Settings.Ratio = NewValue;
			P->SetSettings(P->Settings);
		}
	});

	Params.GetKnee.BindLambda([WeakPreset]() -> float
	{
		return WeakPreset.IsValid() ? WeakPreset->Settings.KneeBandwidthDb : 0.f;
	});
	Params.SetKnee.BindLambda([WeakPreset](float NewValue)
	{
		if (USubmixEffectDynamicsProcessorPreset* P = WeakPreset.Get())
		{
			P->Settings.KneeBandwidthDb = NewValue;
			P->SetSettings(P->Settings);
		}
	});

	// ── Timing knobs (Attack range 1–300ms for submix) ────────────────
	Params.AttackMin = 1.f;
	Params.AttackMax = 300.f;
	Params.GetAttack.BindLambda([WeakPreset]() -> float
	{
		return WeakPreset.IsValid() ? WeakPreset->Settings.AttackTimeMsec : 10.f;
	});
	Params.SetAttack.BindLambda([WeakPreset](float NewValue)
	{
		if (USubmixEffectDynamicsProcessorPreset* P = WeakPreset.Get())
		{
			P->Settings.AttackTimeMsec = NewValue;
			P->SetSettings(P->Settings);
		}
	});

	Params.GetRelease.BindLambda([WeakPreset]() -> float
	{
		return WeakPreset.IsValid() ? WeakPreset->Settings.ReleaseTimeMsec : 100.f;
	});
	Params.SetRelease.BindLambda([WeakPreset](float NewValue)
	{
		if (USubmixEffectDynamicsProcessorPreset* P = WeakPreset.Get())
		{
			P->Settings.ReleaseTimeMsec = NewValue;
			P->SetSettings(P->Settings);
		}
	});

	Params.GetLookAhead.BindLambda([WeakPreset]() -> float
	{
		return WeakPreset.IsValid() ? WeakPreset->Settings.LookAheadMsec : 0.f;
	});
	Params.SetLookAhead.BindLambda([WeakPreset](float NewValue)
	{
		if (USubmixEffectDynamicsProcessorPreset* P = WeakPreset.Get())
		{
			P->Settings.LookAheadMsec = NewValue;
			P->SetSettings(P->Settings);
		}
	});

	// ── Gain knobs (Output range -60 to +30 dB for submix) ────────────
	Params.GetInputGain.BindLambda([WeakPreset]() -> float
	{
		return WeakPreset.IsValid() ? WeakPreset->Settings.InputGainDb : 0.f;
	});
	Params.SetInputGain.BindLambda([WeakPreset](float NewValue)
	{
		if (USubmixEffectDynamicsProcessorPreset* P = WeakPreset.Get())
		{
			P->Settings.InputGainDb = NewValue;
			P->SetSettings(P->Settings);
		}
	});

	Params.OutputGainMin = -60.f;
	Params.OutputGainMax = 30.f;
	Params.GetOutputGain.BindLambda([WeakPreset]() -> float
	{
		return WeakPreset.IsValid() ? WeakPreset->Settings.OutputGainDb : 0.f;
	});
	Params.SetOutputGain.BindLambda([WeakPreset](float NewValue)
	{
		if (USubmixEffectDynamicsProcessorPreset* P = WeakPreset.Get())
		{
			P->Settings.OutputGainDb = NewValue;
			P->SetSettings(P->Settings);
		}
	});

	// ── Detection mode strips ─────────────────────────────────────────
	Params.PeakOptions = { LOCTEXT("MS", "MS"), LOCTEXT("RMS", "RMS"), LOCTEXT("Peak", "Peak") };
	Params.GetPeakMode.BindLambda([WeakPreset]() -> int32
	{
		return WeakPreset.IsValid() ? (int32)WeakPreset->Settings.PeakMode : 0;
	});
	Params.SetPeakMode.BindLambda([WeakPreset](int32 OptionIndex)
	{
		if (USubmixEffectDynamicsProcessorPreset* P = WeakPreset.Get())
		{
			P->Settings.PeakMode = (ESubmixEffectDynamicsPeakMode)OptionIndex;
			P->SetSettings(P->Settings);
		}
	});

	// Submix uses a 3-way channel link strip (Off / Avg / Peak)
	Params.LinkOptions = { LOCTEXT("LnkOff", "Off"), LOCTEXT("LnkAvg", "Avg"), LOCTEXT("LnkPk", "Peak") };
	Params.GetLinkMode.BindLambda([WeakPreset]() -> int32
	{
		return WeakPreset.IsValid() ? (int32)WeakPreset->Settings.LinkMode : 0;
	});
	Params.SetLinkMode.BindLambda([WeakPreset](int32 OptionIndex)
	{
		if (USubmixEffectDynamicsProcessorPreset* P = WeakPreset.Get())
		{
			P->Settings.LinkMode = (ESubmixEffectDynamicsChannelLinkMode)OptionIndex;
			P->SetSettings(P->Settings);
		}
	});

	Params.GetAnalog.BindLambda([WeakPreset]() -> bool
	{
		return WeakPreset.IsValid() && WeakPreset->Settings.bAnalogMode;
	});
	Params.SetAnalog.BindLambda([WeakPreset](bool bOn)
	{
		if (USubmixEffectDynamicsProcessorPreset* P = WeakPreset.Get())
		{
			P->Settings.bAnalogMode = (uint8)bOn;
			P->SetSettings(P->Settings);
		}
	});

	// ─────────────────────────────────────────────────────────────────
	//  Sidechain section (submix only — source effect has no sidechain)
	// ─────────────────────────────────────────────────────────────────
	Params.Sidechain.bEnabled = true;

	// Key source selector (Default / AudioBus / Submix)
	Params.Sidechain.KeySourceOptions = {
		LOCTEXT("KSrc0", "Default"),
		LOCTEXT("KSrc1", "AudioBus"),
		LOCTEXT("KSrc2", "Submix")
	};
	Params.Sidechain.GetKeySource.BindLambda([WeakPreset]() -> int32
	{
		return WeakPreset.IsValid() ? (int32)WeakPreset->Settings.KeySource : 0;
	});
	Params.Sidechain.SetKeySource.BindLambda([WeakPreset](int32 OptionIndex)
	{
		if (USubmixEffectDynamicsProcessorPreset* P = WeakPreset.Get())
		{
			P->Settings.KeySource = (ESubmixEffectDynamicsKeySource)OptionIndex;
			P->SetSettings(P->Settings);
		}
	});

	// Key audition toggle
	Params.Sidechain.GetKeyAudition.BindLambda([WeakPreset]() -> bool
	{
		return WeakPreset.IsValid() && WeakPreset->Settings.bKeyAudition;
	});
	Params.Sidechain.SetKeyAudition.BindLambda([WeakPreset](bool bOn)
	{
		if (USubmixEffectDynamicsProcessorPreset* P = WeakPreset.Get())
		{
			P->Settings.bKeyAudition = (uint8)bOn;
			P->SetSettings(P->Settings);
		}
	});

	// Audio Bus object picker
	Params.Sidechain.GetAudioBusPath = [WeakPreset]() -> FString
	{
		if (const USubmixEffectDynamicsProcessorPreset* P = WeakPreset.Get())
		{
			const UObject* Current = P->Settings.ExternalAudioBus;
			return Current ? Current->GetPathName() : FString();
		}
		return FString();
	};
	Params.Sidechain.SetAudioBus = [WeakPreset](const FAssetData& AssetData)
	{
		if (USubmixEffectDynamicsProcessorPreset* P = WeakPreset.Get())
		{
			if (FObjectProperty* ObjProp = FindFProperty<FObjectProperty>(
				FSubmixEffectDynamicsProcessorSettings::StaticStruct(),
				GET_MEMBER_NAME_CHECKED(FSubmixEffectDynamicsProcessorSettings, ExternalAudioBus)))
			{
				void* ValPtr = ObjProp->ContainerPtrToValuePtr<void>(&P->Settings);
				ObjProp->SetObjectPropertyValue(ValPtr, AssetData.GetAsset());
				P->SetSettings(P->Settings);
			}
		}
	};

	// Submix object picker
	Params.Sidechain.GetSubmixPath = [WeakPreset]() -> FString
	{
		if (const USubmixEffectDynamicsProcessorPreset* P = WeakPreset.Get())
		{
			const UObject* Current = P->Settings.ExternalSubmix;
			return Current ? Current->GetPathName() : FString();
		}
		return FString();
	};
	Params.Sidechain.SetSubmix = [WeakPreset](const FAssetData& AssetData)
	{
		if (USubmixEffectDynamicsProcessorPreset* P = WeakPreset.Get())
		{
			if (FObjectProperty* ObjProp = FindFProperty<FObjectProperty>(
				FSubmixEffectDynamicsProcessorSettings::StaticStruct(),
				GET_MEMBER_NAME_CHECKED(FSubmixEffectDynamicsProcessorSettings, ExternalSubmix)))
			{
				void* ValPtr = ObjProp->ContainerPtrToValuePtr<void>(&P->Settings);
				ObjProp->SetObjectPropertyValue(ValPtr, AssetData.GetAsset());
				P->SetSettings(P->Settings);
			}
		}
	};

	// Key Gain knob
	Params.Sidechain.GetKeyGain.BindLambda([WeakPreset]() -> float
	{
		return WeakPreset.IsValid() ? WeakPreset->Settings.KeyGainDb : 0.f;
	});
	Params.Sidechain.SetKeyGain.BindLambda([WeakPreset](float NewValue)
	{
		if (USubmixEffectDynamicsProcessorPreset* P = WeakPreset.Get())
		{
			P->Settings.KeyGainDb = NewValue;
			P->SetSettings(P->Settings);
		}
	});

	// Highshelf filter (sidechain EQ)
	Params.Sidechain.Highshelf.GetEnabled.BindLambda([WeakPreset]() -> bool
	{
		return WeakPreset.IsValid() && WeakPreset->Settings.KeyHighshelf.bEnabled;
	});
	Params.Sidechain.Highshelf.SetEnabled.BindLambda([WeakPreset](bool bOn)
	{
		if (USubmixEffectDynamicsProcessorPreset* P = WeakPreset.Get())
		{
			P->Settings.KeyHighshelf.bEnabled = bOn;
			P->SetSettings(P->Settings);
		}
	});
	Params.Sidechain.Highshelf.GetCutoff.BindLambda([WeakPreset]() -> float
	{
		return WeakPreset.IsValid() ? WeakPreset->Settings.KeyHighshelf.Cutoff : 1000.f;
	});
	Params.Sidechain.Highshelf.SetCutoff.BindLambda([WeakPreset](float NewValue)
	{
		if (USubmixEffectDynamicsProcessorPreset* P = WeakPreset.Get())
		{
			P->Settings.KeyHighshelf.Cutoff = NewValue;
			P->SetSettings(P->Settings);
		}
	});
	Params.Sidechain.Highshelf.GetGain.BindLambda([WeakPreset]() -> float
	{
		return WeakPreset.IsValid() ? WeakPreset->Settings.KeyHighshelf.GainDb : 0.f;
	});
	Params.Sidechain.Highshelf.SetGain.BindLambda([WeakPreset](float NewValue)
	{
		if (USubmixEffectDynamicsProcessorPreset* P = WeakPreset.Get())
		{
			P->Settings.KeyHighshelf.GainDb = NewValue;
			P->SetSettings(P->Settings);
		}
	});

	// Lowshelf filter (sidechain EQ)
	Params.Sidechain.Lowshelf.GetEnabled.BindLambda([WeakPreset]() -> bool
	{
		return WeakPreset.IsValid() && WeakPreset->Settings.KeyLowshelf.bEnabled;
	});
	Params.Sidechain.Lowshelf.SetEnabled.BindLambda([WeakPreset](bool bOn)
	{
		if (USubmixEffectDynamicsProcessorPreset* P = WeakPreset.Get())
		{
			P->Settings.KeyLowshelf.bEnabled = bOn;
			P->SetSettings(P->Settings);
		}
	});
	Params.Sidechain.Lowshelf.GetCutoff.BindLambda([WeakPreset]() -> float
	{
		return WeakPreset.IsValid() ? WeakPreset->Settings.KeyLowshelf.Cutoff : 1000.f;
	});
	Params.Sidechain.Lowshelf.SetCutoff.BindLambda([WeakPreset](float NewValue)
	{
		if (USubmixEffectDynamicsProcessorPreset* P = WeakPreset.Get())
		{
			P->Settings.KeyLowshelf.Cutoff = NewValue;
			P->SetSettings(P->Settings);
		}
	});
	Params.Sidechain.Lowshelf.GetGain.BindLambda([WeakPreset]() -> float
	{
		return WeakPreset.IsValid() ? WeakPreset->Settings.KeyLowshelf.GainDb : 0.f;
	});
	Params.Sidechain.Lowshelf.SetGain.BindLambda([WeakPreset](float NewValue)
	{
		if (USubmixEffectDynamicsProcessorPreset* P = WeakPreset.Get())
		{
			P->Settings.KeyLowshelf.GainDb = NewValue;
			P->SetSettings(P->Settings);
		}
	});

	return AudioFXDynamicsUI::BuildWidget(*this, Params, AssetCol);
}

#undef LOCTEXT_NAMESPACE
