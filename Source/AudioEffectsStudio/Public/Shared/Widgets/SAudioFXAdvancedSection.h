// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Images/SImage.h"

DECLARE_DELEGATE_OneParam(FOnExpandedChanged, bool /*bIsNowExpanded*/);

// Forward declare only — full include in .cpp to avoid circular dependency
class FAudioFXEffectEditor;

/**
 * SAudioFXAdvancedSection
 *
 * A collapsible "ADVANCED" panel for housing secondary/expert parameters
 * in AudioFX effect editors. Follows the FabFilter Pro-C "Expert" panel
 * design pattern: collapsed by default, toggled by clicking the header button.
 *
 * Centralized window-resize logic: pass OwnerEditor + HeightOffset and the
 * widget automatically resizes the parent window on expand/collapse.
 */
class AUDIOEFFECTSSTUDIO_API SAudioFXAdvancedSection : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAudioFXAdvancedSection)
		: _Label(NSLOCTEXT("AudioEffectsStudio", "AdvancedSectionLabel", "ADVANCED"))
		, _bStartExpanded(false)
		, _AssetColor(FLinearColor(1.f, 0.42f, 0.f, 1.f))
		, _HeightOffset(0.f)
	{}
		/** Header label text. Default: "ADVANCED" */
		SLATE_ARGUMENT(FText, Label)

		/** Whether to start expanded. Default: false (collapsed). */
		SLATE_ARGUMENT(bool, bStartExpanded)

		/** Effect's asset/preset color — used to tint the toggle button when expanded. */
		SLATE_ARGUMENT(FLinearColor, AssetColor)

		/**
		 * Weak pointer to the owning FAudioFXEffectEditor.
		 * When set together with HeightOffset, the widget automatically calls
		 * ResizeWindowByDelta() on expand/collapse — no OnExpandedChanged lambda needed.
		 */
		SLATE_ARGUMENT(TWeakPtr<FAudioFXEffectEditor>, OwnerEditor)

		/**
		 * Logical pixels to add to the window height when expanding (subtracted when collapsing).
		 * Only used when OwnerEditor is set.
		 */
		SLATE_ARGUMENT(float, HeightOffset)

		/** Fired when the section is expanded or collapsed (true = now expanded). */
		SLATE_EVENT(FOnExpandedChanged, OnExpandedChanged)

		/** Content to show/hide. */
		SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	/** Cached label for the header text */
	FText HeaderLabel;

	/** Effect's asset color for expanded-state tinting */
	FLinearColor CachedAssetColor = FLinearColor(1.f, 0.42f, 0.f, 1.f);

	/** Whether the content is currently expanded */
	bool bIsExpanded = false;

	/** Delegate fired when expanded state changes */
	FOnExpandedChanged OnExpandedChangedDelegate;

	/** Weak pointer to owner editor for auto window resize */
	TWeakPtr<FAudioFXEffectEditor> OwnerEditorWeak;

	/** Pixels to add/subtract from window height on expand/collapse */
	float HeightOffset = 0.f;

	/** Pointer to the content box so we can toggle its visibility */
	TSharedPtr<SWidget> ContentContainer;

	/** Arrow chevron image (replaces unicode glyph text block) */
	TSharedPtr<SImage> ArrowImage;

	/** Pointer to the header border for color switching */
	TSharedPtr<SBorder> HeaderBorder;

	/** Toggle expanded/collapsed state */
	FReply OnToggleClicked();

	/** Returns the current arrow brush (chevron down or right) */
	const FSlateBrush* GetArrowBrush() const;

	/** Returns the header text color based on expanded state */
	FSlateColor GetHeaderTextColor() const;

	/** Returns the header button background color */
	FSlateColor GetHeaderBackgroundColor() const;
};
