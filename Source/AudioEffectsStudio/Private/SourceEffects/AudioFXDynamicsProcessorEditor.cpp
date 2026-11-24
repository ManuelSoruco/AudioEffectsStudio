// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "SourceEffects/AudioFXDynamicsProcessorEditor.h"

#include "Shared/Core/AudioFXTheme.h"
#include "Shared/UIBuilders/AudioFXDynamicsUI.h"
#include "Shared/Core/AudioFXPreviewEngine.h"
#include "SourceEffects/SourceEffectDynamicsProcessor.h"

#define LOCTEXT_NAMESPACE "AudioFXDynamicsProcessor"

static bool ShouldShowRatio(ESourceEffectDynamicsProcessorType Type)
{
	return Type != ESourceEffectDynamicsProcessorType::Limiter
		&& Type != ESourceEffectDynamicsProcessorType::Gate;
}

TSharedRef<SWidget> FAudioFXDynamicsProcessorEditor::CreateEffectWidget(USoundEffectPreset* InPreset)
{
	USourceEffectDynamicsProcessorPreset* TypedPreset = Cast<USourceEffectDynamicsProcessorPreset>(InPreset);
	if (!TypedPreset)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("AudioEffectsStudio: Cast to preset type failed for '%s'. Check asset registration."),
			InPreset ? *InPreset->GetClass()->GetName() : TEXT("null"));
		return FAudioFXEffectEditor::CreateEffectWidget(InPreset);
	}

	const FLinearColor AssetColor = GetAssetColor();
	TWeakObjectPtr<USourceEffectDynamicsProcessorPreset> WeakPreset(TypedPreset);

	TWeakPtr<FAudioFXPreviewEngine> WeakEngine = GetPreviewEngine();

	// Fill the shared params struct
	FDynamicsUIParams Params;

	// Transfer curve
	Params.GetTransferSettings.BindLambda([WeakPreset]() -> FSourceEffectDynamicsProcessorSettings
	{
		if (WeakPreset.IsValid())
		{
			return WeakPreset->Settings;
		}
		return FSourceEffectDynamicsProcessorSettings{};
	});

	// Type strip
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
	Params.SetType.BindLambda([WeakPreset](int32 Index)
	{
		if (WeakPreset.IsValid())
		{
			WeakPreset->Settings.DynamicsProcessorType = (ESourceEffectDynamicsProcessorType)Index;
			WeakPreset->SetSettings(WeakPreset->Settings);
		}
	});

	Params.ShouldShowRatio = [](int32 Type) { return ShouldShowRatio((ESourceEffectDynamicsProcessorType)Type); };
	Params.ShouldShowKnee  = [](int32 Type) { return ShouldShowRatio((ESourceEffectDynamicsProcessorType)Type); };

	// Bypass — reads/writes the chain slot bBypass via PreviewEngine (destructive, saved with chain)
	Params.bBypassInSettings = false;
	Params.GetBypass.BindLambda([WeakEngine]() -> bool
	{
		if (TSharedPtr<FAudioFXPreviewEngine> Engine = WeakEngine.Pin())
			return Engine->GetBypass();
		return false;
	});
	Params.SetBypass.BindLambda([WeakEngine](bool bEnabled)
	{
		if (TSharedPtr<FAudioFXPreviewEngine> Engine = WeakEngine.Pin())
		{
			Engine->SetBypass(bEnabled);
		}
	});

	// Threshold
	Params.GetThreshold.BindLambda([WeakPreset]() -> float
	{
		return WeakPreset.IsValid() ? WeakPreset->Settings.ThresholdDb : -30.f;
	});
	Params.SetThreshold.BindLambda([WeakPreset](float Value)
	{
		if (WeakPreset.IsValid())
		{
			WeakPreset->Settings.ThresholdDb = Value;
			WeakPreset->SetSettings(WeakPreset->Settings);
		}
	});

	// Ratio
	Params.GetRatio.BindLambda([WeakPreset]() -> float
	{
		return WeakPreset.IsValid() ? WeakPreset->Settings.Ratio : 4.f;
	});
	Params.SetRatio.BindLambda([WeakPreset](float Value)
	{
		if (WeakPreset.IsValid())
		{
			WeakPreset->Settings.Ratio = Value;
			WeakPreset->SetSettings(WeakPreset->Settings);
		}
	});

	// Knee
	Params.GetKnee.BindLambda([WeakPreset]() -> float
	{
		return WeakPreset.IsValid() ? WeakPreset->Settings.KneeBandwidthDb : 0.f;
	});
	Params.SetKnee.BindLambda([WeakPreset](float Value)
	{
		if (WeakPreset.IsValid())
		{
			WeakPreset->Settings.KneeBandwidthDb = Value;
			WeakPreset->SetSettings(WeakPreset->Settings);
		}
	});

	// Attack
	Params.GetAttack.BindLambda([WeakPreset]() -> float
	{
		return WeakPreset.IsValid() ? WeakPreset->Settings.AttackTimeMsec : 10.f;
	});
	Params.SetAttack.BindLambda([WeakPreset](float Value)
	{
		if (WeakPreset.IsValid())
		{
			WeakPreset->Settings.AttackTimeMsec = Value;
			WeakPreset->SetSettings(WeakPreset->Settings);
		}
	});

	// Release
	Params.GetRelease.BindLambda([WeakPreset]() -> float
	{
		return WeakPreset.IsValid() ? WeakPreset->Settings.ReleaseTimeMsec : 100.f;
	});
	Params.SetRelease.BindLambda([WeakPreset](float Value)
	{
		if (WeakPreset.IsValid())
		{
			WeakPreset->Settings.ReleaseTimeMsec = Value;
			WeakPreset->SetSettings(WeakPreset->Settings);
		}
	});

	// Look Ahead
	Params.GetLookAhead.BindLambda([WeakPreset]() -> float
	{
		return WeakPreset.IsValid() ? WeakPreset->Settings.LookAheadMsec : 0.f;
	});
	Params.SetLookAhead.BindLambda([WeakPreset](float Value)
	{
		if (WeakPreset.IsValid())
		{
			WeakPreset->Settings.LookAheadMsec = Value;
			WeakPreset->SetSettings(WeakPreset->Settings);
		}
	});

	// Input Gain
	Params.GetInputGain.BindLambda([WeakPreset]() -> float
	{
		return WeakPreset.IsValid() ? WeakPreset->Settings.InputGainDb : 0.f;
	});
	Params.SetInputGain.BindLambda([WeakPreset](float Value)
	{
		if (WeakPreset.IsValid())
		{
			WeakPreset->Settings.InputGainDb = Value;
			WeakPreset->SetSettings(WeakPreset->Settings);
		}
	});

	// Output Gain
	Params.GetOutputGain.BindLambda([WeakPreset]() -> float
	{
		return WeakPreset.IsValid() ? WeakPreset->Settings.OutputGainDb : 0.f;
	});
	Params.SetOutputGain.BindLambda([WeakPreset](float Value)
	{
		if (WeakPreset.IsValid())
		{
			WeakPreset->Settings.OutputGainDb = Value;
			WeakPreset->SetSettings(WeakPreset->Settings);
		}
	});

	// Peak mode
	Params.PeakOptions = { LOCTEXT("MS", "MS"), LOCTEXT("RMS", "RMS"), LOCTEXT("Peak", "Peak") };
	Params.GetPeakMode.BindLambda([WeakPreset]() -> int32
	{
		return WeakPreset.IsValid() ? (int32)WeakPreset->Settings.PeakMode : 0;
	});
	Params.SetPeakMode.BindLambda([WeakPreset](int32 Index)
	{
		if (WeakPreset.IsValid())
		{
			WeakPreset->Settings.PeakMode = (ESourceEffectDynamicsPeakMode)Index;
			WeakPreset->SetSettings(WeakPreset->Settings);
		}
	});

	// Stereo link - source uses single bool toggle (no multi-option strip)
	Params.GetStereoLinked.BindLambda([WeakPreset]() -> bool
	{
		return WeakPreset.IsValid() && WeakPreset->Settings.bStereoLinked;
	});
	Params.SetStereoLinked.BindLambda([WeakPreset](bool bEnabled)
	{
		if (WeakPreset.IsValid())
		{
			WeakPreset->Settings.bStereoLinked = (uint32)bEnabled;
			WeakPreset->SetSettings(WeakPreset->Settings);
		}
	});

	// Analog
	Params.GetAnalog.BindLambda([WeakPreset]() -> bool
	{
		return WeakPreset.IsValid() && WeakPreset->Settings.bAnalogMode;
	});
	Params.SetAnalog.BindLambda([WeakPreset](bool bEnabled)
	{
		if (WeakPreset.IsValid())
		{
			WeakPreset->Settings.bAnalogMode = (uint32)bEnabled;
			WeakPreset->SetSettings(WeakPreset->Settings);
		}
	});

	// No sidechain for source effect
	Params.Sidechain.bEnabled = false;
	// Source effect: Response & Gain section does NOT expand the window
	Params.bIsSourceEffect = true;

	// Override gain ranges — UE default is -60 to 30 dB for both
	Params.InputGainMin  = -12.f;
	Params.InputGainMax  = 20.f;
	Params.OutputGainMin = -60.f;
	Params.OutputGainMax = 20.f;

	return AudioFXDynamicsUI::BuildWidget(*this, Params, AssetColor);
}

#undef LOCTEXT_NAMESPACE
