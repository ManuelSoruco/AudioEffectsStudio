// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

/**
 * @file AudioFXPreviewEngine.cpp
 * @brief FAudioFXPreviewEngine -- preview playback state machine and metering backend.
 *
 * State flow:
 *   Idle -> Discovering  (Initialize() starts async asset-registry scan)
 *   Discovering -> Ready (pool discovery callback fires on game thread)
 *   Ready -> Playing     (Play() loads sound, installs GR swap + meter guards)
 *   Playing -> Ready     (Stop() or audio-finished callback tears down guards)
 *
 * Thread safety:
 *   State is an std::atomic<EPreviewState> -- readable from any thread.
 *   All other members are game-thread-only; PlayAsync() results are marshalled
 *   back to game thread via AsyncTask(ENamedThreads::GameThread, ...).
 */

#include "Shared/Core/AudioFXPreviewEngine.h"
#include "Shared/Core/AudioFXPoolDiscovery.h"
#include "Shared/Core/AudioFXGRSwapGuard.h"
#include "Shared/Core/AudioFXMeterGuard.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundSubmix.h"
#include "Components/AudioComponent.h"
#include "AudioDevice.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Async/Async.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Shared/Core/AudioFXTheme.h"  // LogAudioEffectsStudio

// v2: Unity-build log collision fix — replaced DEFINE_LOG_CATEGORY_STATIC with LogAudioEffectsStudio.

// ─────────────────────────────────────────────────────────────────────────────
//  Construction / Destruction
// ─────────────────────────────────────────────────────────────────────────────

FAudioFXPreviewEngine::FAudioFXPreviewEngine()
{
}

FAudioFXPreviewEngine::~FAudioFXPreviewEngine()
{
	Shutdown();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Initialize / Shutdown
// ─────────────────────────────────────────────────────────────────────────────

void FAudioFXPreviewEngine::Initialize(USoundEffectPreset* InPreset)
{
	Preset = InPreset;

	// Determine if this is a source effect (USoundEffectSourcePreset)
	// or a submix effect (USoundEffectSubmixPreset).
	bIsSourceEffect = Preset && Preset->IsA<USoundEffectSourcePreset>();

	if (!Preset)
	{
		State.store(EPreviewState::Idle, std::memory_order_relaxed);
		return;
	}

	if (bIsSourceEffect)
	{
		// Bind preset hot-reload detection (shared by both source and submix)
		FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(
			this, &FAudioFXPreviewEngine::OnPresetPropertyChanged);

		// Bind sound deletion detection
		FEditorDelegates::OnAssetsDeleted.AddRaw(
			this, &FAudioFXPreviewEngine::OnAssetsDeleted);

		DiscoverSourceEffectPool();
	}
	else
	{
		// Bind preset hot-reload detection
		FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(
			this, &FAudioFXPreviewEngine::OnPresetPropertyChanged);

		// Bind sound deletion detection
		FEditorDelegates::OnAssetsDeleted.AddRaw(
			this, &FAudioFXPreviewEngine::OnAssetsDeleted);

		DiscoverSubmixEffectPool();
	}
}

void FAudioFXPreviewEngine::Shutdown()
{
	// Cancel any in-flight async discovery
	if (DiscoveryCancelFlag.IsValid())
	{
		DiscoveryCancelFlag->AtomicSet(true);
		DiscoveryCancelFlag.Reset();
	}

	// Unbind all editor delegates
	FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);
	FEditorDelegates::OnAssetsDeleted.RemoveAll(this);

	// Stop playback cleanly (RAII guards restore chains on Reset)
	if (GetState() == EPreviewState::Playing)
	{
		if (GEditor)
		{
			if (UAudioComponent* Comp = GEditor->GetPreviewAudioComponent())
			{
				// Stop the component non-blocking FIRST (posts async command to audio thread).
				// This lets the audio device drain before the synchronous ResetPreviewAudioComponent
				// flush below, dramatically reducing the stall on window close.
				Comp->Stop();

				if (FinishedDelegateHandle.IsValid())
				{
					Comp->OnAudioFinishedNative.Remove(FinishedDelegateHandle);
					FinishedDelegateHandle.Reset();
				}
			}
			// Synchronous flush — audio thread must finish processing the Stop() command above.
			// Calling Stop() first makes this fence return much faster (idle thread = fast flush).
			GEditor->ResetPreviewAudioComponent();
		}
	}

	// Clear cached chain slot (bypass state lives permanently on the chain asset)
	ActiveSourceSlot  = INDEX_NONE;
	ActiveSourceChain = nullptr;

	// RAII: Meters must be destroyed before GRSwap (meter removal adjusts indices first)
	Meters.Reset();
	GRSwap.Reset();

	// Restore SoundSubmixObject if we had temporarily rerouted a sound wave.
	if (USoundWave* RoutedSound = SubmixRoutedSound.Get())
	{
		RoutedSound->SoundSubmixObject = OriginalSoundSubmix.Get();
	}
	SubmixRoutedSound.Reset();
	OriginalSoundSubmix.Reset();

	ActiveSubmix = nullptr;
	PreviewPool.Empty();
	TargetSubmixPaths.Empty();
	Preset = nullptr;
	State.store(EPreviewState::Idle, std::memory_order_relaxed);
}

//  Transport Controls

void FAudioFXPreviewEngine::Play()
{
	check(IsInGameThread());

	const EPreviewState CurrentState = GetState();
	if ((CurrentState != EPreviewState::Ready && CurrentState != EPreviewState::Playing)
		|| PreviewPool.Num() == 0)
	{
		return;
	}

	// If currently playing, stop cleanly first (unbinds delegates, resets guards)
	if (CurrentState == EPreviewState::Playing)
	{
		Stop();
	}

	// Resolve sound synchronously
	TSoftObjectPtr<USoundWave> SoftWave = PreviewPool.IsValidIndex(CurrentIndex)
		? PreviewPool[CurrentIndex]
		: TSoftObjectPtr<USoundWave>();

	USoundWave* Sound = SoftWave.IsNull() ? nullptr : SoftWave.LoadSynchronous();
	if (!Sound || !GEditor)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("Play: sound not found or GEditor unavailable (index %d)"), CurrentIndex);
		return;
	}

	UE_LOG(LogAudioEffectsStudio, Warning,
		TEXT("Play: === BEGIN === bIsSourceEffect=%d, Sound='%s', Preset='%s'"),
		bIsSourceEffect, *Sound->GetName(), Preset ? *Preset->GetName() : TEXT("null"));

	// ── Chain modification guards (only when meters are enabled) ─────
	if (bShowMeters)
	{
		// Construct RAII GR swap guard (no-op if preset is not a dynamics type)
		GRSwap = MakeUnique<FAudioFXGRSwapGuard>(
			Preset, Sound, TargetSubmixPaths, false, ActiveSubmix);
	}

	// For non-dynamics submix effects the GRSwap guard never sets ActiveSubmix.
	if (!bIsSourceEffect && ActiveSubmix.IsNull() && TargetSubmixPaths.Num() > 0)
	{
		ActiveSubmix = TSoftObjectPtr<USoundSubmix>(TargetSubmixPaths[0]);
	}

	// LoadSynchronous — .Get() returns null for an unloaded soft ptr.
	USoundSubmix* ActiveSubmixLoaded = ActiveSubmix.LoadSynchronous();

	UE_LOG(LogAudioEffectsStudio, Warning,
		TEXT("Play: ActiveSubmixLoaded=%p ('%s')"),
		ActiveSubmixLoaded,
		ActiveSubmixLoaded ? *ActiveSubmixLoaded->GetName() : TEXT("NULL"));

	// For submix effects: temporarily route this sound through the target submix.
	// We defer restoration to Stop() so the audio device captures the routing.
	SubmixRoutedSound   = nullptr;
	OriginalSoundSubmix = nullptr;
	if (!bIsSourceEffect && ActiveSubmixLoaded)
	{
		OriginalSoundSubmix      = Sound->SoundSubmixObject;
		SubmixRoutedSound        = Sound;
		Sound->SoundSubmixObject = ActiveSubmixLoaded;
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("Play: routed SoundSubmixObject -> '%s'"), *ActiveSubmixLoaded->GetName());
	}
	else if (!bIsSourceEffect)
	{
		UE_LOG(LogAudioEffectsStudio, Error,
			TEXT("Play: SUBMIX ROUTING SKIPPED — ActiveSubmixLoaded is NULL!"));
	}

	if (bShowMeters)
	{
		// Construct RAII meter guard (submix: SetSubmixEffectChainOverride; source: chain insert)
		Meters = MakeUnique<FAudioFXMeterGuard>(
			bIsSourceEffect, Preset, Sound, ActiveSubmixLoaded, GRSwap.Get());
	}

	// ── Discover chain slot for live bypass ──────────────────────────
	ActiveSourceSlot  = INDEX_NONE;
	ActiveSourceChain = nullptr;

	if (bIsSourceEffect)
	{
		if (USoundEffectSourcePresetChain* Chain = Sound->SourceEffectChain)
		{
			for (int32 i = 0; i < Chain->Chain.Num(); ++i)
			{
				if (Chain->Chain[i].Preset == Preset)
				{
					ActiveSourceSlot  = i;
					ActiveSourceChain = Chain;
					break;
				}
			}
		}
	}

	// -- THE ONLY CORRECT editor audio call --
	UAudioComponent* Comp = GEditor->PlayPreviewSound(Sound);

	UE_LOG(LogAudioEffectsStudio, Warning,
		TEXT("Play: PlayPreviewSound returned Comp=%p"), Comp);

	BindPlaybackDelegatesAndStart(Comp);

	State.store(EPreviewState::Playing, std::memory_order_relaxed);
	UE_LOG(LogAudioEffectsStudio, Log,
		TEXT("Play: started '%s' (index %d/%d, %s)"),
		*Sound->GetName(), CurrentIndex + 1, PreviewPool.Num(),
		bIsSourceEffect ? TEXT("source effect") : TEXT("submix effect"));
}

void FAudioFXPreviewEngine::Stop()
{
	check(IsInGameThread());

	UE_LOG(LogAudioEffectsStudio, Warning,
		TEXT("Stop: === BEGIN === State=%d, bIsSourceEffect=%d"),
		(int32)GetState(), bIsSourceEffect);

	if (GetState() != EPreviewState::Playing)
	{
		UE_LOG(LogAudioEffectsStudio, Warning, TEXT("Stop: not playing, early return"));
		return;
	}

	// Unbind finish delegate before stopping
	if (GEditor)
	{
		if (UAudioComponent* Comp = GEditor->GetPreviewAudioComponent())
		{
			if (FinishedDelegateHandle.IsValid())
			{
				Comp->OnAudioFinishedNative.Remove(FinishedDelegateHandle);
				FinishedDelegateHandle.Reset();
			}
		}
		// THE ONLY CORRECT stop call
		GEditor->ResetPreviewAudioComponent();
	}

	// Clear cached chain slot (bypass state lives permanently on the chain asset)
	ActiveSourceSlot  = INDEX_NONE;
	ActiveSourceChain = nullptr;

	// RAII restore: Meters first (removes injected entries), then GRSwap (restores original preset).
	// Order matters — meter indices are adjusted relative to GR slot index.
	Meters.Reset();
	GRSwap.Reset();

	// Restore the SoundSubmixObject on the sound wave we temporarily rerouted.
	// Deferred from Play() so the audio device has fully captured the routing.
	if (USoundWave* RoutedSound = SubmixRoutedSound.Get())
	{
		RoutedSound->SoundSubmixObject = OriginalSoundSubmix.Get();
	}
	SubmixRoutedSound.Reset();
	OriginalSoundSubmix.Reset();

	ActiveSubmix = nullptr;
	State.store(EPreviewState::Ready, std::memory_order_relaxed);
	UE_LOG(LogAudioEffectsStudio, Verbose, TEXT("Stop: playback stopped"));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Playback Delegate Binding
// ─────────────────────────────────────────────────────────────────────────────

void FAudioFXPreviewEngine::BindPlaybackDelegatesAndStart(UAudioComponent* Comp)
{
	if (!Comp)
	{
		UE_LOG(LogAudioEffectsStudio, Error, TEXT("BindPlaybackDelegatesAndStart: NULL component"));
		return;
	}

	// Bind finish event using AddLambda with a weak ptr — NEVER AddRaw (fires on audio
	// thread; engine may be destroyed by the time it fires).
	TWeakPtr<FAudioFXPreviewEngine> WeakSelf = AsShared();
	FinishedDelegateHandle = Comp->OnAudioFinishedNative.AddLambda(
		[WeakSelf](UAudioComponent* /*AudioComp*/)
		{
			if (TSharedPtr<FAudioFXPreviewEngine> Pinned = WeakSelf.Pin())
			{
				Pinned->OnAudioFinished(nullptr);
			}
		});
}

void FAudioFXPreviewEngine::Next()
{
	if (PreviewPool.Num() == 0)
	{
		return;
	}
	CurrentIndex = (CurrentIndex + 1) % PreviewPool.Num();
	if (GetState() == EPreviewState::Playing)
	{
		Stop();
		Play();
	}
}

void FAudioFXPreviewEngine::RandomNext()
{
	if (PreviewPool.Num() <= 1)
	{
		return;
	}
	int32 NewIndex = CurrentIndex;
	while (NewIndex == CurrentIndex)
	{
		NewIndex = FMath::RandRange(0, PreviewPool.Num() - 1);
	}
	CurrentIndex = NewIndex;
	if (GetState() == EPreviewState::Playing)
	{
		Stop();
		Play();
	}
}

void FAudioFXPreviewEngine::SetCurrentIndex(int32 NewIndex)
{
	if (PreviewPool.Num() == 0)
	{
		return;
	}
	CurrentIndex = FMath::Clamp(NewIndex, 0, PreviewPool.Num() - 1);
	if (GetState() == EPreviewState::Playing)
	{
		Stop();
		Play();
	}
}

TSoftObjectPtr<USoundWave> FAudioFXPreviewEngine::GetCurrentSound() const
{
	if (PreviewPool.IsValidIndex(CurrentIndex))
	{
		return PreviewPool[CurrentIndex];
	}
	return TSoftObjectPtr<USoundWave>();
}

void FAudioFXPreviewEngine::SetBypass(bool bBypass)
{
	check(IsInGameThread());
	if (!bIsSourceEffect || !Preset) { return; }

	// Find the chain and slot — use cached if playing, otherwise scan the pool
	USoundEffectSourcePresetChain* Chain = ActiveSourceChain.Get();
	int32 Slot = ActiveSourceSlot;

	if (!Chain || Slot == INDEX_NONE)
	{
		// Not playing or cache stale — scan the current pool sound for the chain
		if (PreviewPool.Num() > 0)
		{
			USoundWave* Sound = PreviewPool[FMath::Clamp(CurrentIndex, 0, PreviewPool.Num() - 1)].LoadSynchronous();
			if (Sound && Sound->SourceEffectChain)
			{
				Chain = Sound->SourceEffectChain;
				for (int32 i = 0; i < Chain->Chain.Num(); ++i)
				{
					if (Chain->Chain[i].Preset == Preset) { Slot = i; break; }
				}
			}
		}
	}

	if (!Chain || !Chain->Chain.IsValidIndex(Slot)) { return; }

	// Write bypass to the chain asset — this IS the source of truth
	Chain->Chain[Slot].bBypass = bBypass;
	Chain->MarkPackageDirty();

	// Push the updated chain to the live audio mixer (immediate effect if playing)
	if (GEngine)
	{
		if (FAudioDevice* AudioDevice = GEngine->GetMainAudioDevice().GetAudioDevice())
		{
			AudioDevice->UpdateSourceEffectChain(
				Chain->GetUniqueID(), Chain->Chain, Chain->bPlayEffectChainTails);
		}
	}
}

bool FAudioFXPreviewEngine::GetBypass() const
{
	if (!bIsSourceEffect || !Preset) { return false; }

	// Use cached chain/slot if available (playing)
	if (const USoundEffectSourcePresetChain* Chain = ActiveSourceChain.Get())
	{
		if (Chain->Chain.IsValidIndex(ActiveSourceSlot))
		{
			return Chain->Chain[ActiveSourceSlot].bBypass;
		}
	}

	// Not playing — scan the current pool sound for the chain
	if (PreviewPool.Num() > 0)
	{
		int32 Idx = FMath::Clamp(CurrentIndex, 0, PreviewPool.Num() - 1);
		USoundWave* Sound = PreviewPool[Idx].LoadSynchronous();
		if (Sound && Sound->SourceEffectChain)
		{
			const USoundEffectSourcePresetChain* Chain = Sound->SourceEffectChain;
			for (int32 i = 0; i < Chain->Chain.Num(); ++i)
			{
				if (Chain->Chain[i].Preset == Preset)
				{
					return Chain->Chain[i].bBypass;
				}
			}
		}
	}

	return false;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Playback Handlers
// ─────────────────────────────────────────────────────────────────────────────

void FAudioFXPreviewEngine::OnAudioFinished(UAudioComponent* /*AudioComponent*/)
{
	UE_LOG(LogAudioEffectsStudio, Warning,
		TEXT("OnAudioFinished: bLooping=%d, State=%d"),
		bLooping, (int32)GetState());
	if (bLooping)
	{
		TWeakPtr<FAudioFXPreviewEngine> WeakSelf = AsShared();
		Async(EAsyncExecution::TaskGraphMainThread, [WeakSelf]()
		{
			TSharedPtr<FAudioFXPreviewEngine> Pinned = WeakSelf.Pin();
			if (Pinned.IsValid() && Pinned->GetState() == EPreviewState::Playing)
			{
				// Stop() tears down guards and restores chains, then Play() rebuilds them.
				Pinned->Stop();
				Pinned->State.store(EPreviewState::Ready, std::memory_order_relaxed);
				Pinned->Play();
			}
		});
	}
	else
	{
		TWeakPtr<FAudioFXPreviewEngine> WeakSelf = AsShared();
		Async(EAsyncExecution::TaskGraphMainThread, [WeakSelf]()
		{
			TSharedPtr<FAudioFXPreviewEngine> Pinned = WeakSelf.Pin();
			if (Pinned.IsValid() && Pinned->GetState() == EPreviewState::Playing)
			{
				// MUST call Stop() to tear down RAII guards (meters, GR swap) and
				// restore SoundSubmixObject. Without this, submix chains stay modified
				// and corrupt the next Play() call — even from a different editor.
				Pinned->Stop();
			}
		});
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  Source Effect Pool Discovery
// ─────────────────────────────────────────────────────────────────────────────

void FAudioFXPreviewEngine::DiscoverSourceEffectPool()
{
	check(IsInGameThread());
	check(Preset);

	State.store(EPreviewState::Discovering, std::memory_order_relaxed);

	DiscoveryCancelFlag = MakeShared<FThreadSafeBool, ESPMode::ThreadSafe>(false);
	TSharedRef<FThreadSafeBool, ESPMode::ThreadSafe> CancelRef = DiscoveryCancelFlag.ToSharedRef();

	FSoftObjectPath PresetPath(Preset);
	TWeakPtr<FAudioFXPreviewEngine> WeakSelf = AsShared();

	// IAssetRegistry::GetAssets is NOT thread-safe — must run on the game thread.
	// We dispatch a deferred task on the main thread so the caller returns first,
	// keeping the non-blocking / non-stalling behavior while staying safe.
	Async(EAsyncExecution::TaskGraphMainThread,
		[WeakSelf, PresetPath, CancelRef]()
		{
			if (*CancelRef) { return; }
			TSharedPtr<FAudioFXPreviewEngine> Pinned = WeakSelf.Pin();
			if (!Pinned.IsValid()) { return; }

			TArray<TSoftObjectPtr<USoundWave>> Results =
				FAudioFXPoolDiscovery::DiscoverSourceEffectPool(PresetPath, CancelRef);

			if (!(*CancelRef))
			{
				Pinned->OnSourceDiscoveryComplete(MoveTemp(Results));
			}
		});
}


void FAudioFXPreviewEngine::OnSourceDiscoveryComplete(TArray<TSoftObjectPtr<USoundWave>>&& Results)
{
	check(IsInGameThread());

	PreviewPool = MoveTemp(Results);
	CurrentIndex = 0;

	const EPreviewState NewState = PreviewPool.Num() > 0
		? EPreviewState::Ready
		: EPreviewState::Idle;
	State.store(NewState, std::memory_order_relaxed);

	UE_LOG(LogAudioEffectsStudio, Log,
		TEXT("Pool discovery complete: %d sound(s) ready for preview"),
		PreviewPool.Num());

	OnPoolDiscoveryComplete.Broadcast();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Submix Effect Pool Discovery
// ─────────────────────────────────────────────────────────────────────────────

void FAudioFXPreviewEngine::DiscoverSubmixEffectPool()
{
	check(IsInGameThread());
	check(Preset);

	State.store(EPreviewState::Discovering, std::memory_order_relaxed);

	DiscoveryCancelFlag = MakeShared<FThreadSafeBool, ESPMode::ThreadSafe>(false);
	TSharedRef<FThreadSafeBool, ESPMode::ThreadSafe> CancelRef = DiscoveryCancelFlag.ToSharedRef();

	FSoftObjectPath PresetPath(Preset);
	TWeakPtr<FAudioFXPreviewEngine> WeakSelf = AsShared();

	// IAssetRegistry::GetAssets is NOT thread-safe — must run on the game thread.
	Async(EAsyncExecution::TaskGraphMainThread,
		[WeakSelf, PresetPath, CancelRef]()
		{
			if (*CancelRef) { return; }
			TSharedPtr<FAudioFXPreviewEngine> Pinned = WeakSelf.Pin();
			if (!Pinned.IsValid()) { return; }

			FAudioFXPoolDiscovery::FSubmixResult Result =
				FAudioFXPoolDiscovery::DiscoverSubmixEffectPool(PresetPath, CancelRef);

			if (!(*CancelRef))
			{
				Pinned->OnSubmixDiscoveryComplete(MoveTemp(Result));
			}
		});
}

void FAudioFXPreviewEngine::OnSubmixDiscoveryComplete(FAudioFXPoolDiscovery::FSubmixResult&& Result)
{
	check(IsInGameThread());

	PreviewPool = MoveTemp(Result.WavePool);
	TargetSubmixPaths = MoveTemp(Result.TargetSubmixPaths);
	CurrentIndex = 0;

	UE_LOG(LogAudioEffectsStudio, Warning,
		TEXT("OnSubmixDiscoveryComplete: %d sound(s), %d target submix(es)"),
		PreviewPool.Num(), TargetSubmixPaths.Num());

	for (int32 i = 0; i < TargetSubmixPaths.Num(); ++i)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("  TargetSubmixPaths[%d] = '%s'"), i, *TargetSubmixPaths[i].ToString());
	}
	for (int32 i = 0; i < FMath::Min(PreviewPool.Num(), 5); ++i)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("  PreviewPool[%d] = '%s'"), i, *PreviewPool[i].ToSoftObjectPath().ToString());
	}

	const EPreviewState NewState = PreviewPool.Num() > 0
		? EPreviewState::Ready
		: EPreviewState::Idle;
	State.store(NewState, std::memory_order_relaxed);

	UE_LOG(LogAudioEffectsStudio, Log,
		TEXT("Submix pool discovery complete: %d sound(s), %d target submix(es)"),
		PreviewPool.Num(), TargetSubmixPaths.Num());

	OnPoolDiscoveryComplete.Broadcast();
}


// ─────────────────────────────────────────────────────────────────────────────
//  Edge Case Handlers
// ─────────────────────────────────────────────────────────────────────────────

void FAudioFXPreviewEngine::OnPresetPropertyChanged(
	UObject* Object,
	FPropertyChangedEvent& /*PropertyChangedEvent*/)
{
	check(IsInGameThread());
	if (Object != Preset || !Preset) { return; }

	// Always sync the GR capture parameters — cheap, parameter-level change propagation.
	if (GRSwap) { GRSwap->SyncCaptureFromPreset(Preset); }

	// If audio is actively playing, skip the stop+rediscover entirely.
	// Parameter edits (knob moves) fire PostEditChange → OnObjectPropertyChanged → here,
	// but the preview pool (which sounds use this preset) hasn't changed — only a value did.
	// UE propagates parameter changes to the live audio instance automatically via SetSettings.
	if (GetState() == EPreviewState::Playing)
	{
		return;
	}

	UE_LOG(LogAudioEffectsStudio, Log,
		TEXT("OnPresetPropertyChanged: preset '%s' changed — re-discovering pool"),
		*Preset->GetName());

	if (DiscoveryCancelFlag.IsValid())
	{
		DiscoveryCancelFlag->AtomicSet(true);
		DiscoveryCancelFlag.Reset();
	}


	PreviewPool.Empty();
	TargetSubmixPaths.Empty();
	CurrentIndex = 0;

	if (bIsSourceEffect) { DiscoverSourceEffectPool(); }
	else                 { DiscoverSubmixEffectPool(); }
}

void FAudioFXPreviewEngine::OnAssetsDeleted(const TArray<UClass*>& /*DeletedAssetClasses*/)
{
	check(IsInGameThread());
	if (PreviewPool.Num() == 0) { return; }

	const int32 OldCount = PreviewPool.Num();

	IAssetRegistry* AR = IAssetRegistry::Get();
	if (AR)
	{
		PreviewPool.RemoveAll([&AR](const TSoftObjectPtr<USoundWave>& Entry) -> bool
		{
			if (Entry.IsNull()) return true;
			const FAssetData AssetData = AR->GetAssetByObjectPath(Entry.ToSoftObjectPath());
			return !AssetData.IsValid();
		});
	}

	const int32 Removed = OldCount - PreviewPool.Num();
	if (Removed > 0)
	{
		UE_LOG(LogAudioEffectsStudio, Log,
			TEXT("OnAssetsDeleted: removed %d deleted sound(s) from preview pool (%d remaining)"),
			Removed, PreviewPool.Num());

		CurrentIndex = FMath::Clamp(CurrentIndex, 0, FMath::Max(0, PreviewPool.Num() - 1));

		if (GetState() == EPreviewState::Playing) { Stop(); }

		const EPreviewState NewState = PreviewPool.Num() > 0
			? EPreviewState::Ready
			: EPreviewState::Idle;
		State.store(NewState, std::memory_order_relaxed);

		OnPoolDiscoveryComplete.Broadcast();
	}
}
