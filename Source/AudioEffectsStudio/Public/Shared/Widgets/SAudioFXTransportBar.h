// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Input/SComboBox.h"
#include "Sound/SoundWave.h"

class FAudioFXPreviewEngine;

/**
 * SAudioFXTransportBar
 *
 * Horizontal transport bar displayed between the preset bar and the effect widget.
 * Contains: Play | Stop | Next | Loop | [Sound Dropdown]
 *
 * All buttons are disabled when the preview pool is empty.
 * Themed to match the VST-style dark background.
 *
 * The sound dropdown is a SComboBox<TSharedPtr<FString>> populated from the engine's
 * preview pool display names. A parallel ComboSoftPtrs array maps indices to the actual
 * soft pointers for playback selection.
 */
class AUDIOEFFECTSSTUDIO_API SAudioFXTransportBar : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAudioFXTransportBar)
	{}
		/** The preview engine that drives this transport */
		SLATE_ARGUMENT(TSharedPtr<FAudioFXPreviewEngine>, Engine)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SAudioFXTransportBar();

	/** Rebuild the dropdown items list and force a combo refresh. Called after pool discovery completes. */
	void RefreshSoundDropdown();

private:
	// Button handlers
	FReply OnPlayClicked();
	FReply OnStopClicked();
	FReply OnNextClicked();
	FReply OnLoopClicked();

	// Enabled-state helpers
	bool IsTransportEnabled() const;
	bool IsPlayEnabled() const;
	bool IsStopEnabled() const;
	bool IsNavigationEnabled() const;

	// Active color helpers (for button glow)
	FLinearColor GetPlayActiveColor() const;
	FLinearColor GetLoopActiveColor() const;

	// Combo box helpers
	TSharedRef<SWidget> GenerateComboItem(TSharedPtr<FString> Item) const;
	void OnComboSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	FText GetComboButtonText() const;

	// Members
	TSharedPtr<FAudioFXPreviewEngine> Engine;

	/** Display names for the combo box items (TSharedPtr<FString> satisfies TIsValidListItem) */
	TArray<TSharedPtr<FString>> ComboItems;

	/** The combo box widget */
	TSharedPtr<SComboBox<TSharedPtr<FString>>> SoundComboBox;
};
