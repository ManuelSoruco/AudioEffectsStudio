// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Brushes/SlateRoundedBoxBrush.h"

// Forward-declare only — never include AudioFXEffectEditor.h in widget headers
class FAudioFXEffectEditor;

/**
 * SAudioFXCollapsibleSection
 *
 * A panel card whose content can be collapsed/expanded by clicking the header.
 * If OwnerEditor and HeightOffset are set, toggling automatically resizes the
 * parent window by ±HeightOffset logical pixels (no manual OnToggled callback needed).
 */
class AUDIOEFFECTSSTUDIO_API SAudioFXCollapsibleSection : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAudioFXCollapsibleSection)
		: _SectionLabel(FText::GetEmpty())
		, _bDefaultExpanded(false)
		, _HeightOffset(0.f)
	{}
		/** Section header label */
		SLATE_ARGUMENT(FText, SectionLabel)

		/** Whether the section starts expanded (default: false) */
		SLATE_ARGUMENT(bool, bDefaultExpanded)

		/** Content slot */
		SLATE_DEFAULT_SLOT(FArguments, Content)

		/**
		 * Called when the section is toggled. Passes the new expanded state.
		 * Legacy callback — prefer OwnerEditor + HeightOffset for new code.
		 */
		SLATE_EVENT(FSimpleDelegate, OnToggled)

		/**
		 * Optional: weak pointer to the owning FAudioFXEffectEditor.
		 * When set (with HeightOffset > 0), toggling calls ResizeWindowByDelta automatically.
		 */
		SLATE_ARGUMENT(TWeakPtr<FAudioFXEffectEditor>, OwnerEditor)

		/**
		 * Logical pixel delta to apply to the window height on expand (+) / collapse (-).
		 * Only used when OwnerEditor is set.
		 */
		SLATE_ARGUMENT(float, HeightOffset)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Returns true if the section is currently expanded */
	bool IsExpanded() const { return bExpanded; }

private:
	FReply OnHeaderClicked();

	bool bExpanded = false;
	FSimpleDelegate OnToggledDelegate;
	TWeakPtr<FAudioFXEffectEditor> OwnerEditorWeak;
	float HeightOffset = 0.f;

	TSharedPtr<SWidget> ContentWidget;
	TUniquePtr<FSlateRoundedBoxBrush> CardBrush;
};
