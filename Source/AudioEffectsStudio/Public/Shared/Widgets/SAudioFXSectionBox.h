// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Brushes/SlateRoundedBoxBrush.h"

/**
 * SAudioFXSectionBox
 *
 * A labeled section container rendered as a subtle panel card:
 * - #111115 background fill, #1A1A22 border (1px), 6px corner radius
 * - Group label at top-left (8pt, #707080)
 * - 12px inner padding
 */
class AUDIOEFFECTSSTUDIO_API SAudioFXSectionBox : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAudioFXSectionBox)
		: _SectionLabel(FText::GetEmpty())
		, _Visibility(EVisibility::Visible)
	{}
		/** Section header text (e.g., "Dynamics", "Response") */
		SLATE_ARGUMENT(FText, SectionLabel)

		/** Content slot */
		SLATE_DEFAULT_SLOT(FArguments, Content)

		/** Optional visibility binding */
		SLATE_ATTRIBUTE(EVisibility, Visibility)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	/** Card background brush — created per-instance (color depends on theme, not static) */
	TUniquePtr<FSlateRoundedBoxBrush> CardBrush;
};
