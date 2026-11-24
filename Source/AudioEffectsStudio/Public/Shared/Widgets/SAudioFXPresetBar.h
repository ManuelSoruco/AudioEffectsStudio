// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Brushes/SlateRoundedBoxBrush.h"

class USoundEffectPreset;

DECLARE_DELEGATE_OneParam(FOnAudioFXPresetBypassChanged, bool /* bIsBypassed */);

/**
 * SAudioFXPresetBar
 *
 * VST-style header bar displayed at the top of every effect editor.
 * Contains (per plan §1.3):
 *   - Effect type name (left-aligned, in the asset color)
 *   - Bypass toggle button
 *   - Asset name / preset selector (center)
 *   - Browse / Save / Properties buttons
 *
 * Visually resembles the preset bar in FabFilter / Arturia plugins:
 * a thin, dark horizontal strip with the effect name glowing in
 * the asset's own color.
 */
class AUDIOEFFECTSSTUDIO_API SAudioFXPresetBar : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAudioFXPresetBar)
		: _AssetColor(FLinearColor(0.5f, 0.3f, 0.3f))
		, _ShowBypass(false)
		, _IsBypassed(false)
	{}
		/** The preset being edited */
		SLATE_ARGUMENT(USoundEffectPreset*, Preset)

		/** The effect's asset color for tinting */
		SLATE_ARGUMENT(FLinearColor, AssetColor)

		/** Whether to show the bypass button (not all effects have bypass) */
		SLATE_ARGUMENT(bool, ShowBypass)

		/** Current bypass state */
		SLATE_ATTRIBUTE(bool, IsBypassed)

		/** Fired when user clicks the "Show Properties" button */
		SLATE_EVENT(FSimpleDelegate, OnShowPropertiesClicked)

		/** Fired when user clicks "Open Default Editor" button */
		SLATE_EVENT(FSimpleDelegate, OnOpenDefaultEditorClicked)

		/** Fired when bypass state changes */
		SLATE_EVENT(FOnAudioFXPresetBypassChanged, OnBypassChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	/** Get the display name for the effect type (e.g. "Bit Crusher", "Dynamics Processor") */
	FText GetEffectTypeName() const;

	/** Get the asset name for the dropdown display */
	FText GetPresetName() const;

	/** Get the package path tooltip (e.g. /Game/Sounds/MyPreset) */
	FText GetPresetPackagePathTooltip() const;

	/** Save the current preset */
	FReply OnSaveClicked();

	/** Open standard properties window */
	FReply OnShowPropertiesClicked();

	/** Open plugin settings window */
	FReply OnOpenPluginSettingsClicked();

	/** Open the default UE asset editor */
	FReply OnOpenDefaultEditorClicked();

	/** Navigate to asset in Content Browser */
	FReply OnBrowseToAssetClicked();

	/** True when the preset's package has unsaved changes */
	bool IsPresetDirty() const;

	TWeakObjectPtr<USoundEffectPreset> Preset;
	FLinearColor AssetColor = FLinearColor(0.5f, 0.3f, 0.3f, 1.0f);
	FSimpleDelegate OnShowProperties;
	FSimpleDelegate OnOpenDefaultEditor;
	FOnAudioFXPresetBypassChanged OnBypassChangedDelegate;

	/** Per-instance rounded box brush tinted by asset color. */
	TUniquePtr<FSlateRoundedBoxBrush> PresetBarBrush;

	/** Weak ref to the settings window so we can bring it to front instead of spawning duplicates. */
	TWeakPtr<SWindow> SettingsWindowPtr;
};
