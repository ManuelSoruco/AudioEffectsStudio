// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundWave.h"

/**
 * FAudioFXPoolDiscovery
 *
 * Stateless helper that discovers which sounds use a given effect preset.
 * Runs the asset-registry queries on the game thread (AR is not thread-safe)
 * but dispatches via Async() so the caller returns immediately.
 *
 * Source effects:  Preset -> SoundEffectSourcePresetChain -> SoundWave
 * Submix effects:  Preset -> SoundSubmix (BFS tree) -> SoundWave
 */
struct AUDIOEFFECTSSTUDIO_API FAudioFXPoolDiscovery
{
	/** Result of a submix discovery scan. */
	struct FSubmixResult
	{
		TArray<TSoftObjectPtr<USoundWave>> WavePool;
		/** Soft paths to the submixes that directly reference this preset. */
		TArray<FSoftObjectPath> TargetSubmixPaths;
	};

	/**
	 * Discover all SoundWaves that reference a source-effect preset
	 * (via SoundEffectSourcePresetChain).
	 * Cancellable via the shared bool.
	 */
	static TArray<TSoftObjectPtr<USoundWave>> DiscoverSourceEffectPool(
		const FSoftObjectPath& InPresetPath,
		TSharedRef<FThreadSafeBool, ESPMode::ThreadSafe> bCancelled);

	/**
	 * Discover all SoundWaves routed through submixes that use
	 * a submix-effect preset, including child submixes (BFS).
	 * Cancellable via the shared bool.
	 */
	static FSubmixResult DiscoverSubmixEffectPool(
		const FSoftObjectPath& InPresetPath,
		TSharedRef<FThreadSafeBool, ESPMode::ThreadSafe> bCancelled);
};
