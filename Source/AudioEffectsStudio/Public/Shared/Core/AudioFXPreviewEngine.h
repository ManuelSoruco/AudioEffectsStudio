// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundEffectPreset.h"
#include "Sound/SoundSubmix.h"
#include "Shared/Core/AudioFXPoolDiscovery.h"
#include "Shared/Core/AudioFXGRSwapGuard.h"
#include "Shared/Core/AudioFXMeterGuard.h"

class UAudioComponent;
class USoundWave;
class USoundSubmix;
class FAudioDevice;
class FAudioFXPreviewEngine;

/**
 * Preview playback state machine.
 */
enum class EPreviewState : uint8
{
	/** No preview engine active or pool empty */
	Idle,

	/** Async asset registry scan in progress */
	Discovering,

	/** Pool populated, ready to play */
	Ready,

	/** Actively playing a preview sound */
	Playing,
};

/**
 * FAudioFXPreviewEngine
 *
 * Backend for preview playback and metering in AudioFX effect editors.
 * Composed of three focused helpers:
 *   - FAudioFXPoolDiscovery  — async asset-registry scan (source/submix)
 *   - FAudioFXGRSwapGuard    — RAII GR-capture preset swap (dynamics effects)
 *   - FAudioFXMeterGuard     — RAII input/output meter injection
 *
 * Lifetime: owned by FAudioFXEffectEditor via TSharedPtr.
 * NOT a UObject — plain C++ with shared pointer semantics.
 */
class AUDIOEFFECTSSTUDIO_API FAudioFXPreviewEngine : public TSharedFromThis<FAudioFXPreviewEngine>
{
public:
	FAudioFXPreviewEngine();
	~FAudioFXPreviewEngine();

	/** Initialize with the preset being edited. Triggers async pool discovery. */
	void Initialize(USoundEffectPreset* InPreset);

	/** Shutdown and cleanup. Called from editor destructor. */
	void Shutdown();

	// ── Transport Controls ───────────────────────────────────────────

	/** Start playing the current sound from the pool. */
	void Play();

	/** Stop playback. */
	void Stop();

	/** Advance to the next sound in the pool. If playing, restarts. */
	void Next();

	/** Pick a random sound from the pool. If playing, restarts. */
	void RandomNext();

	/** Toggle loop mode. */
	void SetLooping(bool bLoop) { bLooping = bLoop; }

	/** Is loop mode on? */
	bool IsLooping() const { return bLooping; }

	// ── State & Data ─────────────────────────────────────────────────

	/** Current engine state. Thread-safe read. */
	EPreviewState GetState() const { return State.load(std::memory_order_relaxed); }

	/** Is the preset a source effect (vs submix effect)? */
	bool IsSourceEffect() const { return bIsSourceEffect; }

	/** Get the discovered preview pool (sorted by name). Call only from game thread. */
	const TArray<TSoftObjectPtr<USoundWave>>& GetPreviewPool() const { return PreviewPool; }

	/** Get the currently selected sound. */
	TSoftObjectPtr<USoundWave> GetCurrentSound() const;

	/** Get the current sound index. */
	int32 GetCurrentIndex() const { return CurrentIndex; }

	/**
	 * Set the current sound index directly (used by combo box selection).
	 * Clamped to valid range. If currently playing, restarts with new sound.
	 */
	void SetCurrentIndex(int32 NewIndex);

	// ── Metering ─────────────────────────────────────────────────────

	/** Peak input level (linear 0..1) — brighter bar. */
	float GetInputLevel()      const { return Meters ? Meters->GetInputLevel()      : 0.0f; }
	/** RMS input level (linear 0..1) — darker/background bar. */
	float GetInputRmsLevel()   const { return Meters ? Meters->GetInputRmsLevel()   : 0.0f; }

	/** Peak output levels — brighter bar. */
	float GetOutputLevelL()    const { return Meters ? Meters->GetOutputLevelL()    : 0.0f; }
	float GetOutputLevelR()    const { return Meters ? Meters->GetOutputLevelR()    : 0.0f; }
	/** RMS output levels — darker/background bar. */
	float GetOutputRmsLevelL() const { return Meters ? Meters->GetOutputRmsLevelL() : 0.0f; }
	float GetOutputRmsLevelR() const { return Meters ? Meters->GetOutputRmsLevelR() : 0.0f; }

	/** Output level mono average (linear 0..1). */
	float GetOutputLevel()  const { return (GetOutputLevelL() + GetOutputLevelR()) * 0.5f; }

	/**
	 * Exchange-based peak readout: atomically reads AND resets the peak to 0.
	 * Returns the highest peak since the previous call. Used by SAudioFXMeterStrip
	 * to avoid missing transients between Slate Tick frames.
	 */
	float ExchangeOutputPeakL() const { return Meters ? Meters->ExchangeOutputPeakL() : 0.0f; }
	float ExchangeOutputPeakR() const { return Meters ? Meters->ExchangeOutputPeakR() : 0.0f; }
	float ExchangeInputPeak()   const { return Meters ? Meters->ExchangeInputPeak()   : 0.0f; }

	/** Gain reduction level in dB from active GR capture preset (0.0 if inactive). */
	float GetGainReductionDb() const { return GRSwap ? GRSwap->GetGainReductionDb() : 0.0f; }

	/** True when a GR capture preset is active for the current preview playback. */
	bool HasGainReductionData() const { return GRSwap && GRSwap->HasGainReductionData(); }

	/**
	 * Toggle bypass on the source effect chain slot that holds our preset.
	 * Writes bBypass on the chain entry, marks the chain package dirty, and
	 * pushes the change to the live audio mixer via UpdateSourceEffectChain.
	 * This is DESTRUCTIVE — the chain asset is modified and must be saved.
	 * Source effects only. Submix bypass is handled per-editor via SetBypassState.
	 */
	void SetBypass(bool bBypass);

	/**
	 * Read bypass state from the source effect chain slot (single source of truth).
	 * Returns false for submix effects or if preset is not found in any chain.
	 */
	bool GetBypass() const;

	/** Enable/disable chain modifications (GR swap, meter injection) during preview. */
	void SetShowMeters(bool bInShowMeters) { bShowMeters = bInShowMeters; }

	/** Delegate broadcast on game thread when pool discovery completes. */
	DECLARE_MULTICAST_DELEGATE(FOnPoolDiscoveryComplete);
	FOnPoolDiscoveryComplete OnPoolDiscoveryComplete;

private:
	// ── Playback Handlers ────────────────────────────────────────────

	/** Called when the preview sound finishes playing. */
	void OnAudioFinished(UAudioComponent* AudioComponent);

	// ── Edge Case Handlers ───────────────────────────────────────────
	void OnPresetPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent);
	void OnAssetsDeleted(const TArray<UClass*>& DeletedAssetClasses);

	// ── Pool Discovery ───────────────────────────────────────────────
	void DiscoverSourceEffectPool();
	void DiscoverSubmixEffectPool();
	void OnSourceDiscoveryComplete(TArray<TSoftObjectPtr<USoundWave>>&& Results);
	void OnSubmixDiscoveryComplete(FAudioFXPoolDiscovery::FSubmixResult&& Result);

	// ── Playback Delegate Binding ────────────────────────────────────
	void BindPlaybackDelegatesAndStart(UAudioComponent* Comp);

	// ── Core Members ─────────────────────────────────────────────────

	USoundEffectPreset* Preset        = nullptr;
	bool                bIsSourceEffect = false;

	/** Current state — atomic so meter strip can read from any thread tick */
	// std::atomic: no UE CAS equivalent for this use (relaxed load from non-game threads)
	std::atomic<EPreviewState> State{ EPreviewState::Idle };

	bool bLooping = false;

	/** Handle for the OnAudioFinishedNative lambda */
	FDelegateHandle FinishedDelegateHandle;

	/** Discovered sounds that use this effect (game thread only) */
	TArray<TSoftObjectPtr<USoundWave>> PreviewPool;
	int32 CurrentIndex = 0;

	/** Direct submix paths from pool discovery (submix effects only) */
	TArray<FSoftObjectPath> TargetSubmixPaths;

	/** Active submix for the current playback session (submix effects only) */
	TSoftObjectPtr<USoundSubmix> ActiveSubmix;

	/** Deferred SoundSubmixObject restore — kept alive until Stop(). */
	TWeakObjectPtr<USoundWave>       SubmixRoutedSound;
	TWeakObjectPtr<USoundSubmixBase> OriginalSoundSubmix;

	// ── Live bypass tracking ─────────────────────────────────────────
	// Cached during Play() for fast SetBypass→UpdateSourceEffectChain push.
	// Also used by Get/SetBypass() to read/write the chain when not playing.
	int32 ActiveSourceSlot = INDEX_NONE;
	TWeakObjectPtr<USoundEffectSourcePresetChain> ActiveSourceChain;

	/** When false, Play() skips ALL chain modifications (GR swap, meter injection).
	 *  Audio plays through the user's stock presets untouched. Default false. */
	bool bShowMeters = false;

	/** Cancellation flag for the async discovery task */
	TSharedPtr<FThreadSafeBool, ESPMode::ThreadSafe> DiscoveryCancelFlag;

	// ── Playback-lifetime RAII Guards ────────────────────────────────
	// Constructed in Play(), destroyed (Reset()) in Stop().

	/** GR capture swap guard — null when not playing or effect is not dynamics. */
	TUniquePtr<FAudioFXGRSwapGuard> GRSwap;

	/** Meter injection guard — null when not playing. */
	TUniquePtr<FAudioFXMeterGuard> Meters;
};
