// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

struct FSlateRoundedBoxBrush;

DECLARE_DELEGATE_OneParam(FOnAudioFXButtonSelectionChanged, int32 /* NewIndex */);

/**
 * SAudioFXButton
 *
 * A styled rounded toggle button — the primary interactive button primitive
 * for the Audio Effects Studio UI.
 *
 * Single-option mode (most common):
 *   Pass one entry in Options. The button acts as a boolean toggle:
 *   clicking an active button turns it OFF (SelectedIndex → -1),
 *   clicking an inactive button turns it ON (SelectedIndex → 0).
 *   OnSelectionChanged fires with -1 (off) or 0 (on).
 *
 * Multi-option mode (radio group):
 *   Pass 2–4 entries in Options. Buttons are rendered side-by-side.
 *   Only one can be active at a time (radio behavior).
 *   Clicking the already-active button does nothing.
 */
class AUDIOEFFECTSSTUDIO_API SAudioFXButton : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAudioFXButton)
		: _AssetColor(FLinearColor::White)
		, _SelectedIndex(0)
	{}
		/** Labels for each button */
		SLATE_ARGUMENT(TArray<FText>, Options)

		/** The effect's asset color for active button tinting */
		SLATE_ARGUMENT(FLinearColor, AssetColor)

		/** Currently selected index (-1 = none active) */
		SLATE_ATTRIBUTE(int32, SelectedIndex)

		/** Fired when selection changes */
		SLATE_EVENT(FOnAudioFXButtonSelectionChanged, OnSelectionChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Set the selected index programmatically */
	void SetSelectedIndex(int32 NewIndex);

private:
	void RebuildButtons();
	FReply OnButtonClicked(int32 Index);

	TArray<FText> Options;
	FLinearColor AssetColor = FLinearColor::White;
	TAttribute<int32> SelectedIndex;
	FOnAudioFXButtonSelectionChanged OnSelectionChanged;

	TSharedPtr<SHorizontalBox> ButtonBox;

	/** Per-button rounded box brushes — rebuilt on each RebuildButtons() call. */
	TArray<TUniquePtr<FSlateRoundedBoxBrush>> ButtonBrushes;

	/** Per-button hover-state brushes. */
	TArray<TUniquePtr<FSlateRoundedBoxBrush>> HoverBrushes;
};
