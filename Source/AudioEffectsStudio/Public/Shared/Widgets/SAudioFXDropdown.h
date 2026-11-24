// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Input/SMenuAnchor.h"

struct FSlateRoundedBoxBrush;

DECLARE_DELEGATE_OneParam(FOnAudioFXDropdownSelectionChanged, int32 /* NewIndex */);

/**
 * SAudioFXDropdown
 *
 * Fully themed dropdown built on SMenuAnchor + custom popup panel.
 * Gives complete control over popup styling (rounded corners, dark background,
 * hover highlights) consistent with the rest of the Audio Effects Studio UI.
 */
class AUDIOEFFECTSSTUDIO_API SAudioFXDropdown : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAudioFXDropdown)
		: _AssetColor(FLinearColor::White)
		, _SelectedIndex(0)
		, _MinWidth(0.f)
	{}
		SLATE_ARGUMENT(TArray<FText>, Options)
		SLATE_ARGUMENT(FLinearColor, AssetColor)
		SLATE_ATTRIBUTE(int32, SelectedIndex)
		SLATE_EVENT(FOnAudioFXDropdownSelectionChanged, OnSelectionChanged)
		/** Minimum width of the trigger button in logical pixels (0 = auto). Prevents
		 *  the surrounding layout from reshuffling when the selected text length changes. */
		SLATE_ARGUMENT(float, MinWidth)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetSelectedIndex(int32 NewIndex);
	int32 GetSelectedIndex() const;
	FText GetSelectedText() const;

private:
	/** Build and return the floating popup panel */
	TSharedRef<SWidget> BuildPopupContent();

	/** Toggle the popup open/closed */
	void TogglePopup();

	/** Select an item from the popup */
	void SelectOption(int32 Index);

	TArray<FText>                        Options;
	FLinearColor                         AssetColor = FLinearColor::White;
	TAttribute<int32>                    SelectedIndex;
	FOnAudioFXDropdownSelectionChanged   OnSelectionChanged;
	float                                MinWidth = 0.f;

	TSharedPtr<SMenuAnchor>              MenuAnchor;
	TSharedPtr<class SBorder>            TriggerBorder;
	TUniquePtr<FSlateRoundedBoxBrush>    BodyBrush;
	TUniquePtr<FSlateRoundedBoxBrush>    BodyHoverBrush;
	TUniquePtr<FSlateRoundedBoxBrush>    PopupBrush;

	/** Keeps per-row brushes alive for the duration of the popup */
	TArray<TSharedPtr<FSlateRoundedBoxBrush>> PopupRowBrushes;
};
