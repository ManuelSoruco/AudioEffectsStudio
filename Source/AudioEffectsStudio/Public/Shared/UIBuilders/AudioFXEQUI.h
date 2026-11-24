// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Shared/Core/AudioFXEffectEditor.h"
#include "SourceEffects/SourceEffectEQ.h"

/**
 * AudioFXEQUI
 *
 * Shared UI builder for parametric EQ editors (Source + Submix).
 * Uses a stateful FEQUIState that the editor keeps alive to support
 * dynamic band add/remove via RebuildRack().
 */

struct FSlateRoundedBoxBrush;

// ─── Band-level delegates ────────────────────────────────────────────────

/** Get the number of bands. */
DECLARE_DELEGATE_RetVal(int32, FEQGetBandCount);

/** Get a band field by index. */
DECLARE_DELEGATE_RetVal_OneParam(float, FEQGetBandFloat, int32);
DECLARE_DELEGATE_RetVal_OneParam(bool, FEQGetBandBool, int32);

/** Set a band field by index. */
DECLARE_DELEGATE_TwoParams(FEQSetBandFloat, int32, float);
DECLARE_DELEGATE_TwoParams(FEQSetBandBool, int32, bool);

/** Add a default band. */
DECLARE_DELEGATE(FEQAddBand);

/** Remove band at index. */
DECLARE_DELEGATE_OneParam(FEQRemoveBand, int32);

/** Apply all pending changes (calls SetSettings). */
DECLARE_DELEGATE(FEQApplySettings);

/** Get the full settings for the EQ curve widget (returns Source-type). */
DECLARE_DELEGATE_RetVal(FSourceEffectEQSettings, FEQGetSourceSettings);

// ─── Params struct ───────────────────────────────────────────────────────

struct FEQUIParams
{
	FEQGetBandCount    GetBandCount;
	FEQGetBandFloat    GetFrequency;
	FEQSetBandFloat    SetFrequency;
	FEQGetBandFloat    GetBandwidth;
	FEQSetBandFloat    SetBandwidth;
	FEQGetBandFloat    GetGainDb;
	FEQSetBandFloat    SetGainDb;
	FEQGetBandBool     GetEnabled;
	FEQSetBandBool     SetEnabled;
	FEQAddBand         AddBand;
	FEQRemoveBand      RemoveBand;
	FEQApplySettings   ApplySettings;
	FEQGetSourceSettings GetSourceSettings;
};

// ─── Stateful helper ─────────────────────────────────────────────────────

struct AUDIOEFFECTSSTUDIO_API FEQUIState : public TSharedFromThis<FEQUIState>
{
	/** Rebuild the band rack (call after add/remove). */
	void RebuildRack();

	// Internal — set by BuildWidget
	FAudioFXEffectEditor* Editor = nullptr;
	FEQUIParams Params;
	FLinearColor AssetCol = FLinearColor::White;
	TArray<FLinearColor> BandPalette;
	TSharedPtr<SHorizontalBox> BandRack;
	TArray<TSharedPtr<FSlateRoundedBoxBrush>> BandStripeBrushes;
	FSimpleDelegate MarkDirty;

	TSharedRef<SWidget> BuildBandCard(int32 BandIndex);
	TSharedRef<SWidget> BuildAddBandButton();
	/** Build the centered empty-state widget shown when no bands exist. */
	TSharedRef<SWidget> BuildEmptyState();
};

// ─── Builder ─────────────────────────────────────────────────────────────

namespace AudioFXEQUI
{
	/**
	 * Build the complete EQ widget. Returns the state object that the caller must keep alive.
	 * @param Editor     The editor instance.
	 * @param Params     Band access delegates.
	 * @param AssetCol   Asset color.
	 * @param OutWidget  Receives the built widget.
	 * @return           State object — store as member to keep alive.
	 */
	AUDIOEFFECTSSTUDIO_API TSharedRef<FEQUIState> BuildWidget(
		FAudioFXEffectEditor& Editor,
		const FEQUIParams& Params,
		const FLinearColor& AssetCol,
		TSharedRef<SWidget>& OutWidget);
}
