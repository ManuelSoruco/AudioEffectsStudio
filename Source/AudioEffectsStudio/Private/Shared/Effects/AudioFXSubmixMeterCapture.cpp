// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Effects/AudioFXSubmixMeterCapture.h"
#include "Sound/SoundEffectPreset.h"

namespace AudioFXSubmixMeter
{
	constexpr float PeakAttackMs  =    1.0f;
	constexpr float PeakReleaseMs =  300.0f;
	constexpr float RmsAttackMs   =    5.0f;
	constexpr float RmsReleaseMs  = 1500.0f;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Preset boilerplate
// ─────────────────────────────────────────────────────────────────────────────

FText UAudioFXSubmixMeterCapturePreset::GetAssetActionName() const
{
	return FText::FromString(TEXT("AudioFXSubmixMeterCapture"));
}

UClass* UAudioFXSubmixMeterCapturePreset::GetSupportedClass() const
{
	return UAudioFXSubmixMeterCapturePreset::StaticClass();
}

FSoundEffectBase* UAudioFXSubmixMeterCapturePreset::CreateNewEffect() const
{
	return new FAudioFXSubmixMeterWrapper;
}

USoundEffectPreset* UAudioFXSubmixMeterCapturePreset::CreateNewPreset(
	UObject* InParent, FName Name, EObjectFlags Flags) const
{
	USoundEffectPreset* P = NewObject<UAudioFXSubmixMeterCapturePreset>(
		InParent, GetSupportedClass(), Name, Flags);
	P->Init();
	return P;
}

void UAudioFXSubmixMeterCapturePreset::Init()
{
	FScopeLock Lock(&SettingsCritSect);
	SettingsCopy = Settings;
}

void UAudioFXSubmixMeterCapturePreset::UpdateSettings(const FAudioFXSubmixMeterCaptureSettings& InSettings)
{
	FScopeLock Lock(&SettingsCritSect);
	SettingsCopy = InSettings;
	Update();
}

void UAudioFXSubmixMeterCapturePreset::SetSettings(const FAudioFXSubmixMeterCaptureSettings& InSettings)
{
	UpdateSettings(InSettings);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Atomic helpers
// ─────────────────────────────────────────────────────────────────────────────

void UAudioFXSubmixMeterCapturePreset::AtomicMax(std::atomic<float>& Target, float NewVal)
{
	float Old = Target.load(std::memory_order_relaxed);
	while (NewVal > Old && !Target.compare_exchange_weak(Old, NewVal, std::memory_order_relaxed)) {}
}

void UAudioFXSubmixMeterCapturePreset::SetInputLevels(float PeakL, float PeakR, float RmsL, float RmsR)
{
	AtomicMax(InPeakL, PeakL);
	AtomicMax(InPeakR, PeakR);
	InRmsL.store(RmsL, std::memory_order_relaxed);
	InRmsR.store(RmsR, std::memory_order_relaxed);
}

void UAudioFXSubmixMeterCapturePreset::SetLatestLevels(float PeakL, float PeakR, float RmsL, float RmsR)
{
	AtomicMax(OutPeakL, PeakL);
	AtomicMax(OutPeakR, PeakR);
	OutRmsL.store(RmsL, std::memory_order_relaxed);
	OutRmsR.store(RmsR, std::memory_order_relaxed);
}

// ─────────────────────────────────────────────────────────────────────────────
//  DSP Wrapper — Init
// ─────────────────────────────────────────────────────────────────────────────

void FAudioFXSubmixMeterWrapper::Init(const FSoundEffectSubmixInitData& D)
{
	auto MakePeak = [&]()
	{
		Audio::FEnvelopeFollowerInitParams P;
		P.SampleRate      = D.SampleRate;
		P.NumChannels     = 1;
		P.AttackTimeMsec  = AudioFXSubmixMeter::PeakAttackMs;
		P.ReleaseTimeMsec = AudioFXSubmixMeter::PeakReleaseMs;
		P.Mode            = Audio::EPeakMode::Peak;
		P.bIsAnalog       = true;
		return P;
	};

	auto MakeRms = [&]()
	{
		Audio::FEnvelopeFollowerInitParams P;
		P.SampleRate      = D.SampleRate;
		P.NumChannels     = 1;
		P.AttackTimeMsec  = AudioFXSubmixMeter::RmsAttackMs;
		P.ReleaseTimeMsec = AudioFXSubmixMeter::RmsReleaseMs;
		P.Mode            = Audio::EPeakMode::RootMeanSquared;
		P.bIsAnalog       = true;
		return P;
	};

	InPeakFollowerL.Init(MakePeak());  InPeakFollowerR.Init(MakePeak());
	InRmsL.Init(MakeRms());            InRmsR.Init(MakeRms());
	OutPeakFollowerL.Init(MakePeak()); OutPeakFollowerR.Init(MakePeak());
	OutRmsL.Init(MakeRms());           OutRmsR.Init(MakeRms());

	if (InnerEffect.IsValid())
	{
		InnerEffect->Setup(D);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  DSP Wrapper — MeasureBuffer
// ─────────────────────────────────────────────────────────────────────────────

FAudioFXSubmixMeterWrapper::FLevels FAudioFXSubmixMeterWrapper::MeasureBuffer(
	const float* Buffer, int32 NumFrames, int32 NumChannels,
	Audio::FEnvelopeFollower& PeakFollowerL, Audio::FEnvelopeFollower& PeakFollowerR,
	Audio::FEnvelopeFollower& RmsL,          Audio::FEnvelopeFollower& RmsR)
{
	FLevels Out = { 0.f, 0.f, 0.f, 0.f };
	if (NumFrames <= 0 || NumChannels <= 0 || !Buffer) { return Out; }

	MonoScratch.SetNumUninitialized(NumFrames);
	float* Mono = MonoScratch.GetData();

	auto RunCh = [&](int32 Ch, Audio::FEnvelopeFollower& PeakFollower,
	                 Audio::FEnvelopeFollower& Rms, float& OutPeak)
	{
		for (int32 f = 0; f < NumFrames; ++f)
		{
			Mono[f] = FMath::Abs(Buffer[f * NumChannels + Ch]);
		}
		PeakFollower.ProcessAudio(Mono, NumFrames);
		Rms.ProcessAudio(Mono, NumFrames);
		OutPeak = FMath::Clamp(PeakFollower.GetEnvelopeValues()[0], 0.f, 1.f);
	};

	if (NumChannels >= 2)
	{
		RunCh(0, PeakFollowerL, RmsL, Out.PeakL);
		RunCh(1, PeakFollowerR, RmsR, Out.PeakR);
	}
	else
	{
		RunCh(0, PeakFollowerL, RmsL, Out.PeakL);
		Out.PeakR     = Out.PeakL;
		PeakFollowerR = PeakFollowerL;
		RmsR          = RmsL;
	}

	Out.RmsL = FMath::Clamp(RmsL.GetEnvelopeValues()[0], 0.f, 1.f);
	Out.RmsR = FMath::Clamp(RmsR.GetEnvelopeValues()[0], 0.f, 1.f);
	return Out;
}

// ─────────────────────────────────────────────────────────────────────────────
//  DSP Wrapper — OnProcessAudio
// ─────────────────────────────────────────────────────────────────────────────

void FAudioFXSubmixMeterWrapper::OnProcessAudio(
	const FSoundEffectSubmixInputData& InData,
	FSoundEffectSubmixOutputData& OutData)
{
	const int32 NumFrames   = InData.NumFrames;
	const int32 NumChannels = FMath::Max(InData.NumChannels, 1);
	const int32 Total       = NumFrames * NumChannels;

	if (Total <= 0 || !InData.AudioBuffer || !OutData.AudioBuffer) { return; }

	const float* InBuf = InData.AudioBuffer->GetData();
	float*       OutBuf = OutData.AudioBuffer->GetData();

	UAudioFXSubmixMeterCapturePreset* P =
		CastChecked<UAudioFXSubmixMeterCapturePreset>(Preset.Get(), ECastCheckedType::NullAllowed);

	if (P)
	{
		FLevels L = MeasureBuffer(InBuf, NumFrames, NumChannels,
		                          InPeakFollowerL, InPeakFollowerR, InRmsL, InRmsR);
		P->SetInputLevels(L.PeakL, L.PeakR, L.RmsL, L.RmsR);
	}

	if (InnerEffect.IsValid())
	{
		FSoundEffectSubmixInputData  InCopy  = InData;
		FSoundEffectSubmixOutputData OutCopy = OutData;
		InnerEffect->ProcessAudio(InCopy, OutCopy);
	}
	else
	{
		FMemory::Memcpy(OutBuf, InBuf, Total * sizeof(float));
	}

	if (P)
	{
		FLevels L = MeasureBuffer(OutBuf, NumFrames, NumChannels,
		                          OutPeakFollowerL, OutPeakFollowerR, OutRmsL, OutRmsR);
		P->SetLatestLevels(L.PeakL, L.PeakR, L.RmsL, L.RmsR);
	}
}
