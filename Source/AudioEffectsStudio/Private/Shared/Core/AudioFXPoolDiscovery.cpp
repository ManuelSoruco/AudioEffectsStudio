// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

/**
 * @file AudioFXPoolDiscovery.cpp
 * @brief Async asset-registry queries to discover which sounds reference a
 *        given SoundEffect preset.
 *
 * Source-effect path:
 *   Preset -> SoundEffectSourcePresetChain -> SoundWave
 *   Uses AssetRegistry dependency links to walk the reference tree.
 *
 * Submix-effect path:
 *   Preset -> SoundSubmix (BFS over submix tree) -> SoundWave
 *   All SoundWaves that route through a submix in the tree are added to the pool.
 *
 * Results are returned via callback on the game thread. Discovery can be
 * cancelled via the shared FThreadSafeBool at any point.
 */

#include "Shared/Core/AudioFXPoolDiscovery.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetRegistry/ARFilter.h"
#include "Modules/ModuleManager.h"
#include "Shared/Core/AudioFXTheme.h" // LogAudioEffectsStudio

// ----------------------------------------------------------------------------
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace AudioFXPoolDiscoveryPrivate
{
	static IAssetRegistry& GetAR()
	{
		return FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	}

	static void SortPoolByName(TArray<TSoftObjectPtr<USoundWave>>& Pool)
	{
		Pool.Sort([](const TSoftObjectPtr<USoundWave>& A, const TSoftObjectPtr<USoundWave>& B)
		{
			return FPaths::GetBaseFilename(A.GetAssetName()).ToLower()
				 < FPaths::GetBaseFilename(B.GetAssetName()).ToLower();
		});
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  Source Effect Discovery
// ─────────────────────────────────────────────────────────────────────────────

TArray<TSoftObjectPtr<USoundWave>> FAudioFXPoolDiscovery::DiscoverSourceEffectPool(
	const FSoftObjectPath& InPresetPath,
	TSharedRef<FThreadSafeBool, ESPMode::ThreadSafe> bCancelled)
{
	TArray<TSoftObjectPtr<USoundWave>> OutPool;
	IAssetRegistry& AR = AudioFXPoolDiscoveryPrivate::GetAR();

	const FName PresetPackage = FName(*InPresetPath.GetLongPackageName());
	TArray<FName> ChainPackages;
	AR.GetReferencers(PresetPackage, ChainPackages);

	if (*bCancelled || ChainPackages.Num() == 0)
	{
		return OutPool;
	}

	UE_LOG(LogAudioEffectsStudio, Verbose,
		TEXT("DiscoverSourceEffectPool: preset '%s' referenced by %d package(s), filtering to chains"),
		*InPresetPath.GetAssetName(), ChainPackages.Num());

	FARFilter ChainFilter;
	ChainFilter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("SoundEffectSourcePresetChain")));
	ChainFilter.bRecursiveClasses = false;
	for (const FName& Pkg : ChainPackages)
	{
		ChainFilter.PackageNames.Add(Pkg);
	}

	TArray<FAssetData> MatchingChainAssets;
	AR.GetAssets(ChainFilter, MatchingChainAssets);

	if (*bCancelled || MatchingChainAssets.Num() == 0)
	{
		UE_LOG(LogAudioEffectsStudio, Log,
			TEXT("DiscoverSourceEffectPool: no USoundEffectSourcePresetChain assets found for this preset"));
		return OutPool;
	}

	UE_LOG(LogAudioEffectsStudio, Log,
		TEXT("DiscoverSourceEffectPool: %d chain(s) reference this preset"),
		MatchingChainAssets.Num());

	TSet<FName> SoundPackageCandidates;
	for (const FAssetData& ChainData : MatchingChainAssets)
	{
		if (*bCancelled) { return OutPool; }
		TArray<FName> SoundReferencers;
		AR.GetReferencers(ChainData.PackageName, SoundReferencers);
		for (const FName& Pkg : SoundReferencers)
		{
			SoundPackageCandidates.Add(Pkg);
		}
	}

	if (*bCancelled || SoundPackageCandidates.Num() == 0)
	{
		return OutPool;
	}

	FARFilter WaveFilter;
	WaveFilter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("SoundWave")));
	WaveFilter.bRecursiveClasses = true;
	for (const FName& Pkg : SoundPackageCandidates)
	{
		WaveFilter.PackageNames.Add(Pkg);
	}

	TArray<FAssetData> WaveAssets;
	AR.GetAssets(WaveFilter, WaveAssets);

	if (*bCancelled) { return OutPool; }

	UE_LOG(LogAudioEffectsStudio, Log,
		TEXT("DiscoverSourceEffectPool: found %d USoundWave asset(s) for preview pool"),
		WaveAssets.Num());

	for (const FAssetData& WaveData : WaveAssets)
	{
		OutPool.Add(TSoftObjectPtr<USoundWave>(WaveData.ToSoftObjectPath()));
	}

	AudioFXPoolDiscoveryPrivate::SortPoolByName(OutPool);
	return OutPool;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Submix Effect Discovery
// ─────────────────────────────────────────────────────────────────────────────

FAudioFXPoolDiscovery::FSubmixResult FAudioFXPoolDiscovery::DiscoverSubmixEffectPool(
	const FSoftObjectPath& InPresetPath,
	TSharedRef<FThreadSafeBool, ESPMode::ThreadSafe> bCancelled)
{
	FSubmixResult OutResult;
	IAssetRegistry& AR = AudioFXPoolDiscoveryPrivate::GetAR();

	const FName PresetPackage = FName(*InPresetPath.GetLongPackageName());

	TArray<FName> DirectReferencers;
	AR.GetReferencers(PresetPackage, DirectReferencers);

	if (*bCancelled || DirectReferencers.Num() == 0)
	{
		return OutResult;
	}

	UE_LOG(LogAudioEffectsStudio, Verbose,
		TEXT("DiscoverSubmixEffectPool: preset '%s' referenced by %d package(s), filtering to submixes"),
		*InPresetPath.GetAssetName(), DirectReferencers.Num());

	FARFilter SubmixFilter;
	SubmixFilter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("SoundSubmix")));
	SubmixFilter.bRecursiveClasses = false;
	for (const FName& Pkg : DirectReferencers)
	{
		SubmixFilter.PackageNames.Add(Pkg);
	}

	TArray<FAssetData> TargetSubmixAssets;
	AR.GetAssets(SubmixFilter, TargetSubmixAssets);

	if (*bCancelled || TargetSubmixAssets.Num() == 0)
	{
		UE_LOG(LogAudioEffectsStudio, Log,
			TEXT("DiscoverSubmixEffectPool: no USoundSubmix assets found for this preset"));
		return OutResult;
	}

	UE_LOG(LogAudioEffectsStudio, Log,
		TEXT("DiscoverSubmixEffectPool: %d target submix(es) use this preset"),
		TargetSubmixAssets.Num());

	for (const FAssetData& TargetData : TargetSubmixAssets)
	{
		OutResult.TargetSubmixPaths.Add(TargetData.ToSoftObjectPath());
	}

	// BFS to collect full submix tree (child submixes also route audio through parent effects)
	TSet<FName> AllSubmixPackages;
	TArray<FName> FrontierPackages;
	for (const FAssetData& TargetData : TargetSubmixAssets)
	{
		AllSubmixPackages.Add(TargetData.PackageName);
		FrontierPackages.Add(TargetData.PackageName);
	}

	while (FrontierPackages.Num() > 0 && !(*bCancelled))
	{
		TArray<FName> NextFrontier;
		FARFilter ChildFilter;
		ChildFilter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("SoundSubmix")));
		ChildFilter.bRecursiveClasses = false;
		for (const FName& ParentPkg : FrontierPackages)
		{
			TArray<FName> ChildReferencers;
			AR.GetReferencers(ParentPkg, ChildReferencers);
			for (const FName& ChildPkg : ChildReferencers)
			{
				if (!AllSubmixPackages.Contains(ChildPkg))
				{
					ChildFilter.PackageNames.Add(ChildPkg);
				}
			}
		}
		if (ChildFilter.PackageNames.Num() == 0) { break; }

		TArray<FAssetData> ChildAssets;
		AR.GetAssets(ChildFilter, ChildAssets);
		for (const FAssetData& ChildData : ChildAssets)
		{
			if (!AllSubmixPackages.Contains(ChildData.PackageName))
			{
				AllSubmixPackages.Add(ChildData.PackageName);
				NextFrontier.Add(ChildData.PackageName);
			}
		}
		FrontierPackages = MoveTemp(NextFrontier);
	}

	if (*bCancelled) { return OutResult; }

	UE_LOG(LogAudioEffectsStudio, Verbose,
		TEXT("DiscoverSubmixEffectPool: submix tree has %d node(s)"),
		AllSubmixPackages.Num());

	// Find USoundWave assets routed through the submix tree
	TSet<FName> WavePackageCandidates;
	for (const FName& SubmixPkg : AllSubmixPackages)
	{
		if (*bCancelled) { return OutResult; }
		TArray<FName> SoundReferencers;
		AR.GetReferencers(SubmixPkg, SoundReferencers);
		for (const FName& Pkg : SoundReferencers)
		{
			WavePackageCandidates.Add(Pkg);
		}
	}

	if (*bCancelled || WavePackageCandidates.Num() == 0)
	{
		return OutResult;
	}

	FARFilter WaveFilter;
	WaveFilter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("SoundWave")));
	WaveFilter.bRecursiveClasses = true;
	for (const FName& Pkg : WavePackageCandidates)
	{
		WaveFilter.PackageNames.Add(Pkg);
	}

	TArray<FAssetData> WaveAssets;
	AR.GetAssets(WaveFilter, WaveAssets);

	if (*bCancelled) { return OutResult; }

	UE_LOG(LogAudioEffectsStudio, Log,
		TEXT("DiscoverSubmixEffectPool: found %d USoundWave asset(s) for preview pool"),
		WaveAssets.Num());

	for (const FAssetData& WaveData : WaveAssets)
	{
		OutResult.WavePool.Add(TSoftObjectPtr<USoundWave>(WaveData.ToSoftObjectPath()));
	}

	AudioFXPoolDiscoveryPrivate::SortPoolByName(OutResult.WavePool);
	return OutResult;
}
