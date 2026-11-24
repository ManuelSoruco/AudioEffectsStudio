// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Shared/Core/AudioFXEffectEditor.h"
#include "SourceEffects/SourceEffectDynamicsProcessor.h"

/**
 * AudioFXDynamicsUI
 *
 * Shared UI builder for Dynamics Processor editors.
 * Both Source and Submix variants map their settings into
 * FDynamicsUIParams (lambdas for get/set) and call BuildWidget().
 *
 * Source-specific: cosmetic bypass, bStereoLinked (bool toggle), no sidechain.
 * Submix-specific: settings bypass, LinkMode (3-way), full sidechain section.
 */

// ─── Forward declarations ────────────────────────────────────────────────
class SAudioFXLabeledKnob;
struct FAudioMaterialKnobStyle;

// ─── Normalized parameter accessors ──────────────────────────────────────

/** Read a float from settings. */
DECLARE_DELEGATE_RetVal(float, FDynGetFloat);

/** Write a float to settings. */
DECLARE_DELEGATE_OneParam(FDynSetFloat, float);

/** Read an int from settings. */
DECLARE_DELEGATE_RetVal(int32, FDynGetInt);

/** Write an int to settings. */
DECLARE_DELEGATE_OneParam(FDynSetInt, int32);

/** Read a bool from settings. */
DECLARE_DELEGATE_RetVal(bool, FDynGetBool);

/** Write a bool to settings. */
DECLARE_DELEGATE_OneParam(FDynSetBool, bool);

/** Read the transfer curve settings for SAudioFXTransferCurve (uses Source type). */
DECLARE_DELEGATE_RetVal(FSourceEffectDynamicsProcessorSettings, FDynGetTransferSettings);

// ─── Sidechain shelf sub-params ──────────────────────────────────────────

struct FDynShelfParams
{
	FDynGetBool   GetEnabled;
	FDynSetBool   SetEnabled;
	FDynGetFloat  GetCutoff;
	FDynSetFloat  SetCutoff;
	FDynGetFloat  GetGain;
	FDynSetFloat  SetGain;
};

// ─── Sidechain section params (submix only) ──────────────────────────────

struct FDynSidechainParams
{
	/** Whether to build the sidechain section at all. */
	bool bEnabled = false;

	TArray<FText>  KeySourceOptions;
	FDynGetInt     GetKeySource;
	FDynSetInt     SetKeySource;

	FDynGetBool    GetKeyAudition;
	FDynSetBool    SetKeyAudition;

	/** External Audio Bus picker (visible when key source == AudioBus). */
	TFunction<FString()>                 GetAudioBusPath;
	TFunction<void(const FAssetData&)>   SetAudioBus;

	/** External Submix picker (visible when key source == Submix). */
	TFunction<FString()>                 GetSubmixPath;
	TFunction<void(const FAssetData&)>   SetSubmix;

	FDynGetFloat  GetKeyGain;
	FDynSetFloat  SetKeyGain;

	FDynShelfParams Highshelf;
	FDynShelfParams Lowshelf;
};

// ─── Main parameter block ────────────────────────────────────────────────

struct FDynamicsUIParams
{
	// ── Source vs Submix ─────────────────────────────────────────
	/** If true, Response & Gain section does NOT expand the window (Source editors are smaller). */
	bool bIsSourceEffect = false;

	// ── Transfer curve ───────────────────────────────────────────
	FDynGetTransferSettings GetTransferSettings;

	// ── Type strip ───────────────────────────────────────────────
	TArray<FText>  TypeOptions;
	FDynGetInt     GetType;
	FDynSetInt     SetType;

	/** Callback: returns true if Ratio knob should be visible for current type. */
	TFunction<bool(int32)> ShouldShowRatio;
	/** Callback: returns true if Knee knob should be visible for current type. */
	TFunction<bool(int32)> ShouldShowKnee;

	// ── Bypass ───────────────────────────────────────────────────
	/** If true, bypass is a real settings toggle (submix). If false, bypass is cosmetic/preview (source). */
	bool bBypassInSettings = false;
	FDynGetBool   GetBypass;
	FDynSetBool   SetBypass;

	// ── Primary dynamics knobs ───────────────────────────────────
	FDynGetFloat  GetThreshold;
	FDynSetFloat  SetThreshold;
	float ThreshMin = -60.f;
	float ThreshMax = 0.f;

	FDynGetFloat  GetRatio;
	FDynSetFloat  SetRatio;
	float RatioMin = 1.f;
	float RatioMax = 20.f;

	FDynGetFloat  GetKnee;
	FDynSetFloat  SetKnee;
	float KneeMin = 0.f;
	float KneeMax = 20.f;

	// ── Response knobs ───────────────────────────────────────────
	FDynGetFloat  GetAttack;
	FDynSetFloat  SetAttack;
	float AttackMin = 1.f;
	float AttackMax = 200.f;

	FDynGetFloat  GetRelease;
	FDynSetFloat  SetRelease;
	float ReleaseMin = 20.f;
	float ReleaseMax = 5000.f;

	FDynGetFloat  GetLookAhead;
	FDynSetFloat  SetLookAhead;
	float LookAheadMin = 0.f;
	float LookAheadMax = 50.f;

	// ── Gain knobs ───────────────────────────────────────────────
	FDynGetFloat  GetInputGain;
	FDynSetFloat  SetInputGain;
	float InputGainMin = -12.f;
	float InputGainMax = 20.f;

	FDynGetFloat  GetOutputGain;
	FDynSetFloat  SetOutputGain;
	float OutputGainMin = -12.f;
	float OutputGainMax = 20.f;

	// ── Peak mode strip ──────────────────────────────────────────
	TArray<FText>  PeakOptions;
	FDynGetInt     GetPeakMode;
	FDynSetInt     SetPeakMode;

	// ── Link mode ────────────────────────────────────────────────
	/** Source uses a single "Stereo Link" bool toggle.
	 *  Submix uses a 3-way Link strip (Off/Avg/Peak).
	 *  If LinkOptions is empty, we show a single bool toggle instead. */
	TArray<FText>  LinkOptions;
	FDynGetInt     GetLinkMode;
	FDynSetInt     SetLinkMode;

	/** Fallback: Source-style single bool toggle for stereo link. */
	FDynGetBool    GetStereoLinked;
	FDynSetBool    SetStereoLinked;

	// ── Analog toggle ────────────────────────────────────────────
	FDynGetBool    GetAnalog;
	FDynSetBool    SetAnalog;

	// ── Sidechain (submix only) ──────────────────────────────────
	FDynSidechainParams Sidechain;
};

// ─── Builder ─────────────────────────────────────────────────────────────

namespace AudioFXDynamicsUI
{
	/**
	 * Build the complete Dynamics Processor widget.
	 * @param Editor    The editor instance (for StoreKnobStyle, MakeMarkDirtyDelegate, etc.)
	 * @param Params    Normalized parameter block filled by the caller.
	 * @param AssetCol  Asset color for styling.
	 * @return          Complete Slate widget tree.
	 */
	AUDIOEFFECTSSTUDIO_API TSharedRef<SWidget> BuildWidget(
		FAudioFXEffectEditor& Editor,
		const FDynamicsUIParams& Params,
		const FLinearColor& AssetCol);
}
