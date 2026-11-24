// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

/**
 * FAudioFXSubmixEQEditor
 *
 * Parametric EQ editor for USubmixEffectSubmixEQPreset.
 * Converts between the submix band type (FSubmixEffectEQBand) and the
 * shared EQ UI builder's contract (FEQUIParams), then delegates rendering
 * to AudioFXEQUI::BuildWidget.
 *
 * Key responsibility: map FSubmixEffectEQBand ↔ FSourceEffectEQBand so the
 * shared SAudioFXEQCurve visualizer can read settings unchanged.
 */

#include "SubmixEffects/AudioFXSubmixEQEditor.h"

#include "Shared/Core/AudioFXTheme.h"
#include "Shared/UIBuilders/AudioFXEQUI.h"
#include "SourceEffects/SourceEffectEQ.h"
#include "SubmixEffects/AudioMixerSubmixEffectEQ.h"

#define LOCTEXT_NAMESPACE "AudioFXSubmixEQ"

TSharedRef<SWidget> FAudioFXSubmixEQEditor::CreateEffectWidget(USoundEffectPreset* InPreset)
{
	USubmixEffectSubmixEQPreset* Preset = Cast<USubmixEffectSubmixEQPreset>(InPreset);
	if (!Preset)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("AudioEffectsStudio: Cast to preset type failed for '%s'. Check asset registration."),
			InPreset ? *InPreset->GetClass()->GetName() : TEXT("null"));
		return FAudioFXEffectEditor::CreateEffectWidget(InPreset);
	}

	const FLinearColor AssetCol = GetAssetColor();
	TWeakObjectPtr<USubmixEffectSubmixEQPreset> WeakPreset = Preset;

	FEQUIParams Params;

	// ── Band count & per-band getters ─────────────────────────────────
	Params.GetBandCount.BindLambda([WeakPreset]() -> int32
	{
		return WeakPreset.IsValid() ? WeakPreset->Settings.EQBands.Num() : 0;
	});

	Params.GetFrequency.BindLambda([WeakPreset](int32 BandIndex) -> float
	{
		return (WeakPreset.IsValid() && WeakPreset->Settings.EQBands.IsValidIndex(BandIndex))
			? WeakPreset->Settings.EQBands[BandIndex].Frequency
			: 1000.f;
	});
	Params.SetFrequency.BindLambda([WeakPreset](int32 BandIndex, float NewValue)
	{
		if (USubmixEffectSubmixEQPreset* P = WeakPreset.Get(); P && P->Settings.EQBands.IsValidIndex(BandIndex))
		{
			P->Settings.EQBands[BandIndex].Frequency = NewValue;
			P->SetSettings(P->Settings);
		}
	});

	Params.GetBandwidth.BindLambda([WeakPreset](int32 BandIndex) -> float
	{
		return (WeakPreset.IsValid() && WeakPreset->Settings.EQBands.IsValidIndex(BandIndex))
			? WeakPreset->Settings.EQBands[BandIndex].Bandwidth
			: 1.f;
	});
	Params.SetBandwidth.BindLambda([WeakPreset](int32 BandIndex, float NewValue)
	{
		if (USubmixEffectSubmixEQPreset* P = WeakPreset.Get(); P && P->Settings.EQBands.IsValidIndex(BandIndex))
		{
			P->Settings.EQBands[BandIndex].Bandwidth = NewValue;
			P->SetSettings(P->Settings);
		}
	});

	Params.GetGainDb.BindLambda([WeakPreset](int32 BandIndex) -> float
	{
		return (WeakPreset.IsValid() && WeakPreset->Settings.EQBands.IsValidIndex(BandIndex))
			? WeakPreset->Settings.EQBands[BandIndex].GainDb
			: 0.f;
	});
	Params.SetGainDb.BindLambda([WeakPreset](int32 BandIndex, float NewValue)
	{
		if (USubmixEffectSubmixEQPreset* P = WeakPreset.Get(); P && P->Settings.EQBands.IsValidIndex(BandIndex))
		{
			P->Settings.EQBands[BandIndex].GainDb = NewValue;
			P->SetSettings(P->Settings);
		}
	});

	Params.GetEnabled.BindLambda([WeakPreset](int32 BandIndex) -> bool
	{
		return WeakPreset.IsValid()
			&& WeakPreset->Settings.EQBands.IsValidIndex(BandIndex)
			&& WeakPreset->Settings.EQBands[BandIndex].bEnabled;
	});
	Params.SetEnabled.BindLambda([WeakPreset](int32 BandIndex, bool bOn)
	{
		if (USubmixEffectSubmixEQPreset* P = WeakPreset.Get(); P && P->Settings.EQBands.IsValidIndex(BandIndex))
		{
			P->Settings.EQBands[BandIndex].bEnabled = bOn;
			P->SetSettings(P->Settings);
		}
	});

	// ── Add / Remove band ─────────────────────────────────────────────
	Params.AddBand.BindLambda([WeakPreset]()
	{
		if (USubmixEffectSubmixEQPreset* P = WeakPreset.Get())
		{
			FSubmixEffectEQBand Band{};
			Band.bEnabled  = true;
			Band.Frequency = 1000.f;
			Band.Bandwidth = 1.f;
			Band.GainDb    = 0.f;
			P->Settings.EQBands.Add(Band);
			P->SetSettings(P->Settings);
		}
	});
	Params.RemoveBand.BindLambda([WeakPreset](int32 BandIndex)
	{
		if (USubmixEffectSubmixEQPreset* P = WeakPreset.Get(); P && P->Settings.EQBands.IsValidIndex(BandIndex))
		{
			P->Settings.EQBands.RemoveAt(BandIndex);
			P->SetSettings(P->Settings);
		}
	});

	// ── Band type conversion for SAudioFXEQCurve ──────────────────────
	// The shared curve widget expects FSourceEffectEQSettings, so we convert
	// FSubmixEffectEQBand → FSourceEffectEQBand on every repaint.
	Params.GetSourceSettings.BindLambda([WeakPreset]() -> FSourceEffectEQSettings
	{
		FSourceEffectEQSettings Out;
		if (const USubmixEffectSubmixEQPreset* P = WeakPreset.Get())
		{
			for (const FSubmixEffectEQBand& SBand : P->Settings.EQBands)
			{
				FSourceEffectEQBand Band;
				Band.Frequency = SBand.Frequency;
				Band.Bandwidth = SBand.Bandwidth;
				Band.GainDb    = SBand.GainDb;
				Band.bEnabled  = SBand.bEnabled;
				Out.EQBands.Add(Band);
			}
		}
		return Out;
	});

	TSharedRef<SWidget> Widget = SNullWidget::NullWidget;
	EQState = AudioFXEQUI::BuildWidget(*this, Params, AssetCol, Widget);
	return Widget;
}

#undef LOCTEXT_NAMESPACE
