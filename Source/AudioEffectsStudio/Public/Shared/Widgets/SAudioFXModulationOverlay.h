// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "SourceEffects/SourceEffectMotionFilter.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "AudioMaterialSlate/AudioMaterialSlateTypes.h"

DECLARE_DELEGATE(FOnAudioFXModulationOverlayClose);

class SVerticalBox;

/**
 * SAudioFXModulationOverlay
 *
 * Full-screen modal overlay exposing the MotionFilter's modulation mapping table.
 * Renders as a semi-transparent dark backdrop with a centered, blurred floating panel
 * containing a scrollable list of source → destination → amount rows.
 *
 * Each row independently controls:
 *   - Modulation source      (e.g. Distance, Relative Speed, Emitter Speed)
 *   - Modulation destination (e.g. Filter A Cutoff, Filter Mix)
 *   - Modulation amount knob [-1..1], stored symmetrically across Min/Max range vectors
 *
 * All edits are transaction-aware: every mutation calls Preset->Modify() before writing,
 * then Preset->SetSettings() to propagate the change to the live audio graph immediately.
 *
 * Usage: embed this widget in an SOverlay above the effect editor to show/hide it.
 * Dismiss via the Close button, which fires OnCloseRequested.
 */
class AUDIOEFFECTSSTUDIO_API SAudioFXModulationOverlay : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAudioFXModulationOverlay)
		: _AssetColor(FLinearColor::White)
	{}
		/** The MotionFilter preset being edited */
		SLATE_ARGUMENT(TWeakObjectPtr<USourceEffectMotionFilterPreset>, Preset)

		/** Effect's asset color — used to tint the Amount knob and Add Mapping button */
		SLATE_ARGUMENT(FLinearColor, AssetColor)

		/** Fired when the Close button is clicked — caller should hide or destroy this overlay */
		SLATE_EVENT(FOnAudioFXModulationOverlayClose, OnCloseRequested)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// ── Public static utilities ──────────────────────────────────────────────

	/**
	 * Extract a scalar [-1..1] modulation amount from a settings struct.
	 * The amount is split symmetrically across Min/Max range vectors (X == Y convention).
	 * If both ranges are non-zero we conservatively fold to a mean to maintain a stable display value.
	 */
	static float AmountFromSettings(const FSourceEffectMotionFilterModulationSettings& Settings);

	/**
	 * Write a scalar [-1..1] amount back into a settings struct.
	 * Positive amounts go into ModulationOutputMaximumRange; negative into MinimumRange.
	 * Both X and Y components are set identically (mirrored vector convention).
	 */
	static void ApplyAmountToSettings(FSourceEffectMotionFilterModulationSettings& Settings, float Amount);

	/**
	 * Find the first destination slot not yet present in Mappings.
	 * Returns false when all known destinations are already mapped (table is full).
	 * OutDestination is only written when the return value is true.
	 */
	static bool TryGetNextDestination(
		const TMap<ESourceEffectMotionFilterModDestination, FSourceEffectMotionFilterModulationSettings>& Mappings,
		ESourceEffectMotionFilterModDestination& OutDestination);

private:
	// ── Layout helpers ───────────────────────────────────────────────────────

	/** Clear and repopulate RowsBox from the preset's current ModulationMappings map */
	void RebuildRows();

	/** Build the horizontal Source | Destination | Amount | Remove row widget for one mapping */
	TSharedRef<SWidget> BuildMappingRow(
		ESourceEffectMotionFilterModDestination Destination,
		const FSourceEffectMotionFilterModulationSettings& Settings);

	// ── Edit callbacks ───────────────────────────────────────────────────────

	/** Called when the user picks a new source from a row's Source dropdown */
	void OnSourceChanged(ESourceEffectMotionFilterModDestination Destination, int32 SourceIndex);

	/**
	 * Called when the user picks a new destination from a row's Destination dropdown.
	 * Performs a swap if the chosen destination already exists in another row.
	 */
	void OnDestinationChanged(ESourceEffectMotionFilterModDestination OldDestination, int32 DestinationIndex);

	/** Called when the Amount knob value changes (AmountNorm is normalized [0..1]) */
	void OnAmountChanged(ESourceEffectMotionFilterModDestination Destination, float AmountNorm);

	/** Remove the mapping for Destination and rebuild the row list */
	void RemoveMapping(ESourceEffectMotionFilterModDestination Destination);

	/** Add a new mapping at the first available destination slot (no-op when table is full) */
	void AddMapping();

	// ── State ────────────────────────────────────────────────────────────────

	TWeakObjectPtr<USourceEffectMotionFilterPreset> Preset;
	FLinearColor AssetColor = FLinearColor::White;
	FOnAudioFXModulationOverlayClose OnCloseRequested;

	/** Knob style for the Amount control — built at Construct time from the asset color */
	FAudioMaterialKnobStyle AmountKnobStyle;

	/** The VBox that holds one BuildMappingRow() widget per active mapping */
	TSharedPtr<SVerticalBox> RowsBox;
};
