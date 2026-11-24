// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "SourceEffects/AudioFXEQEditor.h"

#include "Shared/UIBuilders/AudioFXEQUI.h"
#include "SourceEffects/SourceEffectEQ.h"

#define LOCTEXT_NAMESPACE "AudioFXEQ"

TSharedRef<SWidget> FAudioFXEQEditor::CreateEffectWidget(USoundEffectPreset* InPreset)
{
	USourceEffectEQPreset* Preset = Cast<USourceEffectEQPreset>(InPreset);
	if (!Preset)
		return FAudioFXEffectEditor::CreateEffectWidget(InPreset);

	const FLinearColor AssetCol = GetAssetColor();
	TWeakObjectPtr<USourceEffectEQPreset> WeakPreset = Preset;

	FEQUIParams Params;

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
		if (USourceEffectEQPreset* Preset = WeakPreset.Get(); Preset && Preset->Settings.EQBands.IsValidIndex(BandIndex))
		{
			Preset->Settings.EQBands[BandIndex].Frequency = NewValue;
			Preset->SetSettings(Preset->Settings);
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
		if (USourceEffectEQPreset* Preset = WeakPreset.Get(); Preset && Preset->Settings.EQBands.IsValidIndex(BandIndex))
		{
			Preset->Settings.EQBands[BandIndex].Bandwidth = NewValue;
			Preset->SetSettings(Preset->Settings);
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
		if (USourceEffectEQPreset* Preset = WeakPreset.Get(); Preset && Preset->Settings.EQBands.IsValidIndex(BandIndex))
		{
			Preset->Settings.EQBands[BandIndex].GainDb = NewValue;
			Preset->SetSettings(Preset->Settings);
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
		if (USourceEffectEQPreset* Preset = WeakPreset.Get(); Preset && Preset->Settings.EQBands.IsValidIndex(BandIndex))
		{
			Preset->Settings.EQBands[BandIndex].bEnabled = bOn;
			Preset->SetSettings(Preset->Settings);
		}
	});
	Params.AddBand.BindLambda([WeakPreset]()
	{
		if (USourceEffectEQPreset* Preset = WeakPreset.Get())
		{
			FSourceEffectEQBand NewBand{};
			NewBand.bEnabled = true;
			NewBand.Frequency = 1000.f;
			NewBand.Bandwidth = 1.f;
			NewBand.GainDb = 0.f;
			Preset->Settings.EQBands.Add(NewBand);
			Preset->SetSettings(Preset->Settings);
		}
	});
	Params.RemoveBand.BindLambda([WeakPreset](int32 BandIndex)
	{
		if (USourceEffectEQPreset* Preset = WeakPreset.Get(); Preset && Preset->Settings.EQBands.IsValidIndex(BandIndex))
		{
			Preset->Settings.EQBands.RemoveAt(BandIndex);
			Preset->SetSettings(Preset->Settings);
		}
	});
	Params.GetSourceSettings.BindLambda([WeakPreset]() -> FSourceEffectEQSettings
	{
		return WeakPreset.IsValid() ? WeakPreset->Settings : FSourceEffectEQSettings{};
	});

	TSharedRef<SWidget> Widget = SNullWidget::NullWidget;
	EQState = AudioFXEQUI::BuildWidget(*this, Params, AssetCol, Widget);
	return Widget;
}

#undef LOCTEXT_NAMESPACE
