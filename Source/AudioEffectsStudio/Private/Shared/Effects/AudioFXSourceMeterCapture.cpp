// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Effects/AudioFXSourceMeterCapture.h"
#include "Shared/Core/AudioFXTheme.h" // LogAudioEffectsStudio

namespace AudioFXSourceMeter
{
	constexpr float PeakAttackMs  =    1.0f;
	constexpr float PeakReleaseMs =  300.0f;
	constexpr float RmsAttackMs   =    5.0f;
	constexpr float RmsReleaseMs  = 1500.0f;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Preset boilerplate
// ─────────────────────────────────────────────────────────────────────────────

FText UAudioFXSourceMeterCapturePreset::GetAssetActionName() const
{
	return FText::FromString(TEXT("AudioFXSourceMeterCapture"));
}

UClass* UAudioFXSourceMeterCapturePreset::GetSupportedClass() const
{
	return UAudioFXSourceMeterCapturePreset::StaticClass();
}

FSoundEffectBase* UAudioFXSourceMeterCapturePreset::CreateNewEffect() const
{
	return new FAudioFXSourceMeterWrapper;
}

USoundEffectPreset* UAudioFXSourceMeterCapturePreset::CreateNewPreset(
	UObject* InParent, FName Name, EObjectFlags Flags) const
{
	USoundEffectPreset* P = NewObject<UAudioFXSourceMeterCapturePreset>(
		InParent, GetSupportedClass(), Name, Flags);
	P->Init();
	return P;
}

void UAudioFXSourceMeterCapturePreset::Init()
{
	FScopeLock Lock(&SettingsCritSect);
	SettingsCopy = Settings;
}

void UAudioFXSourceMeterCapturePreset::UpdateSettings(const FAudioFXSourceMeterCaptureSettings& InSettings)
{
	FScopeLock Lock(&SettingsCritSect);
	SettingsCopy = InSettings;
	Update();
}

void UAudioFXSourceMeterCapturePreset::SetSettings(const FAudioFXSourceMeterCaptureSettings& InSettings)
{
	UpdateSettings(InSettings);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Atomic helpers
// ─────────────────────────────────────────────────────────────────────────────

void UAudioFXSourceMeterCapturePreset::AtomicMax(std::atomic<float>& Target, float NewVal)
{
	float Old = Target.load(std::memory_order_relaxed);
	while (NewVal > Old && !Target.compare_exchange_weak(Old, NewVal, std::memory_order_relaxed)) {}
}

void UAudioFXSourceMeterCapturePreset::SetInputLevels(float PeakL, float PeakR, float RmsL, float RmsR)
{
	AtomicMax(InPeakL, PeakL);
	AtomicMax(InPeakR, PeakR);
	InRmsL.store(RmsL, std::memory_order_relaxed);
	InRmsR.store(RmsR, std::memory_order_relaxed);
}

void UAudioFXSourceMeterCapturePreset::SetLatestLevels(float PeakL, float PeakR, float RmsL, float RmsR)
{
	AtomicMax(OutPeakL, PeakL);
	AtomicMax(OutPeakR, PeakR);
	OutRmsL.store(RmsL, std::memory_order_relaxed);
	OutRmsR.store(RmsR, std::memory_order_relaxed);
}

// ─────────────────────────────────────────────────────────────────────────────
//  DSP Wrapper — Init
// ─────────────────────────────────────────────────────────────────────────────

void FAudioFXSourceMeterWrapper::Init(const FSoundEffectSourceInitData& D)
{
	NumChannels = FMath::Max(D.NumSourceChannels, 1);

	UE_LOG(LogAudioEffectsStudio, Warning,
		TEXT("SourceMeterWrapper::Init: NumCh=%d, SampleRate=%.0f, GetPreset()=%p, InnerEffect=%d"),
		NumChannels, D.SampleRate, GetPreset(), InnerEffect.IsValid() ? 1 : 0);

	auto MakePeak = [&]() {
		Audio::FEnvelopeFollowerInitParams P;
		P.SampleRate = D.SampleRate; P.NumChannels = 1;
		P.AttackTimeMsec = AudioFXSourceMeter::PeakAttackMs; P.ReleaseTimeMsec = AudioFXSourceMeter::PeakReleaseMs;
		P.Mode = Audio::EPeakMode::Peak; P.bIsAnalog = true;
		return P;
	};
	auto MakeRms = [&]() {
		Audio::FEnvelopeFollowerInitParams P;
		P.SampleRate = D.SampleRate; P.NumChannels = 1;
		P.AttackTimeMsec = AudioFXSourceMeter::RmsAttackMs; P.ReleaseTimeMsec = AudioFXSourceMeter::RmsReleaseMs;
		P.Mode = Audio::EPeakMode::RootMeanSquared; P.bIsAnalog = true;
		return P;
	};
	InPeakFollowerL.Init(MakePeak());  InPeakFollowerR.Init(MakePeak());
	InRmsL.Init(MakeRms());            InRmsR.Init(MakeRms());
	OutPeakFollowerL.Init(MakePeak()); OutPeakFollowerR.Init(MakePeak());
	OutRmsL.Init(MakeRms());           OutRmsR.Init(MakeRms());

	if (!InnerEffect.IsValid())
	{
		auto* WP = CastChecked<UAudioFXSourceMeterCapturePreset>(GetPreset(), ECastCheckedType::NullAllowed);
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("SourceMeterWrapper::Init: WP=%p, PendingInner=%p ('%s')"),
			WP,
			WP ? WP->GetPendingInnerPreset() : nullptr,
			(WP && WP->GetPendingInnerPreset()) ? *WP->GetPendingInnerPreset()->GetName() : TEXT("null"));

		if (WP)
		{
			if (USoundEffectSourcePreset* IP = WP->GetPendingInnerPreset())
			{
				FSoundEffectBase* Raw = IP->CreateNewEffect();
				UE_LOG(LogAudioEffectsStudio, Warning,
					TEXT("SourceMeterWrapper::Init: CreateNewEffect returned Raw=%p"), Raw);
				if (Raw)
				{
					// Mirror UE's USoundEffectPreset::CreateInstance() order:
					// 1. Set Preset + ParentPresetUniqueId (so Init/OnPresetChanged can read settings)
					// 2. Setup (calls Init)
					// 3. AddEffectInstance (registers for future OnPresetChanged notifications)
					Raw->GetPresetPtr() = IP;
					FSourceEffectPtr Ptr = MakeShareable(static_cast<FSoundEffectSource*>(Raw));
					Ptr->SetEnabled(true);
					Ptr->Setup(D);
					TSoundEffectPtr BP = Ptr;
					IP->AddEffectInstance(BP);
					InnerEffect = Ptr;
					UE_LOG(LogAudioEffectsStudio, Warning,
						TEXT("SourceMeterWrapper::Init: InnerEffect created and initialized OK"));
				}
			}
		}
	}
}

FAudioFXSourceMeterWrapper::FLevels FAudioFXSourceMeterWrapper::MeasureBuffer(
	const float* Buf, int32 NumFrames, int32 NC,
	Audio::FEnvelopeFollower& PeakFollowerL, Audio::FEnvelopeFollower& PeakFollowerR,
	Audio::FEnvelopeFollower& RmsL,          Audio::FEnvelopeFollower& RmsR)
{
	FLevels Out = { 0.f, 0.f, 0.f, 0.f };
	if (NumFrames <= 0 || NC <= 0 || !Buf) { return Out; }

	MonoScratch.SetNumUninitialized(NumFrames);
	float* Mono = MonoScratch.GetData();

	auto RunCh = [&](int32 Ch, Audio::FEnvelopeFollower& PeakFollower,
	                 Audio::FEnvelopeFollower& Rms, float& OutPeak)
	{
		for (int32 f = 0; f < NumFrames; ++f)
		{
			Mono[f] = FMath::Abs(Buf[f * NC + Ch]);
		}
		// Peak follower: fast attack catches transients, slow release smooths
		// buffer-to-buffer energy swings. Eliminates per-buffer aliasing.
		PeakFollower.ProcessAudio(Mono, NumFrames);
		Rms.ProcessAudio(Mono, NumFrames);
		OutPeak = FMath::Clamp(PeakFollower.GetEnvelopeValues()[0], 0.f, 1.f);
	};

	if (NC >= 2)
	{
		RunCh(0, PeakFollowerL, RmsL, Out.PeakL);
		RunCh(1, PeakFollowerR, RmsR, Out.PeakR);
	}
	else
	{
		RunCh(0, PeakFollowerL, RmsL, Out.PeakL);
		Out.PeakR = Out.PeakL;
		PeakFollowerR = PeakFollowerL;
		RmsR = RmsL;
	}

	Out.RmsL = FMath::Clamp(RmsL.GetEnvelopeValues()[0], 0.f, 1.f);
	Out.RmsR = FMath::Clamp(RmsR.GetEnvelopeValues()[0], 0.f, 1.f);
	return Out;
}

void FAudioFXSourceMeterWrapper::ProcessAudio(
	const FSoundEffectSourceInputData& InData, float* Out)
{
	static bool bLoggedOnce = false;
	if (!bLoggedOnce)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("SourceMeterWrapper::ProcessAudio: FIRST CALL — InnerEffect=%d, NumSamples=%d"),
			InnerEffect.IsValid() ? 1 : 0, InData.NumSamples);
		bLoggedOnce = true;
	}

	const int32 NC = FMath::Max(NumChannels, 1);
	const int32 NS = InData.NumSamples;
	const int32 NF = NS / NC;
	const float* In = InData.InputSourceEffectBufferPtr;
	if (NS <= 0 || !In || !Out) { return; }

	auto* P = CastChecked<UAudioFXSourceMeterCapturePreset>(GetPreset(), ECastCheckedType::NullAllowed);

	if (P)
	{
		FLevels L = MeasureBuffer(In, NF, NC, InPeakFollowerL, InPeakFollowerR, InRmsL, InRmsR);
		P->SetInputLevels(L.PeakL, L.PeakR, L.RmsL, L.RmsR);
	}

	if (InnerEffect.IsValid())
	{
		InnerEffect->ProcessAudio(InData, Out);
	}
	else
	{
		FMemory::Memcpy(Out, In, NS * sizeof(float));
	}

	if (P)
	{
		FLevels L = MeasureBuffer(Out, NF, NC, OutPeakFollowerL, OutPeakFollowerR, OutRmsL, OutRmsR);
		P->SetLatestLevels(L.PeakL, L.PeakR, L.RmsL, L.RmsR);
	}
}
