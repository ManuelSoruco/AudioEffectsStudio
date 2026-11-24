// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "AudioMaterialSlate/AudioMaterialSlateTypes.h"
#include "AudioWidgetsEnums.h"
#include "Shared/Core/AudioFXTheme.h"

class SAudioMaterialKnob;
class SEditableTextBox;

/**
 * SAudioFXLabeledKnob
 *
 * Composite knob widget. Wraps the engine's bare SAudioMaterialKnob
 * (SLeafWidget — pure dial, zero internal labels) and owns all surrounding
 * layout itself.  This is the deep-root approach: by NOT using
 * SAudioMaterialLabeledKnob we avoid the engine-owned SAudioTextBox,
 * eliminating clipping hacks and height-ratio math.
 *
 * Features:
 *  - Parameter name label ABOVE the knob (dim, Text Secondary color)
 *  - Live formatted value label BELOW the knob
 *  - Ctrl+Click resets to the declared default value
 *  - Double-click opens an inline editable text box for precise entry
 *  - Right-click shows context menu: Reset / Copy / Paste
 *
 * Layout (top → bottom):
 *   ┌───────────────┐
 *   │  Threshold    │  ← ParamName label (dim)
 *   │   ╭────╮      │
 *   │   │    │      │  ← SAudioMaterialKnob (bare dial)
 *   │   ╰────╯      │
 *   │  -24.0 dB     │  ← STextBlock (formatted)
 *   └───────────────┘
 *
 * Drop-in replacement for SAudioMaterialLabeledKnob in all editors.
 */
class AUDIOEFFECTSSTUDIO_API SAudioFXLabeledKnob : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAudioFXLabeledKnob)
		: _ParamName()
		, _Value(0.5f)
		, _Style(nullptr)
		, _AudioUnitsValueType(EAudioUnitsValueType::Linear)
		, _DefaultValue(0.5f)
		, _MinValue(0.f)
		, _MaxValue(1.f)
		, _ShowParamName(true)
	{}
		/** Parameter name shown above the knob (e.g. "Threshold", "Attack") */
		SLATE_ARGUMENT(FText, ParamName)

		/** Normalized [0,1] value forwarded to SAudioMaterialLabeledKnob */
		SLATE_ATTRIBUTE(float, Value)

		/** Knob style pointer — must be stable (stored in FAudioFXEffectEditor) */
		SLATE_ARGUMENT(const FAudioMaterialKnobStyle*, Style)

		/** Controls value display formatting below the knob */
		SLATE_ARGUMENT(EAudioUnitsValueType, AudioUnitsValueType)

		/** Extended value format. Overrides AudioUnitsValueType when not Default. */
		SLATE_ARGUMENT(EAudioFXValueFormat, ValueFormat)

		/**
		 * The normalized [0,1] default value to restore on Ctrl+Click / "Reset".
		 * Set to the preset's default value at the call site.
		 */
		SLATE_ARGUMENT(float, DefaultValue)

		/**
		 * Raw min/max for the editable text box (in the real unit, not 0-1).
		 * Used to parse and clamp typed values.
		 * e.g. MinValue = -60.f, MaxValue = 0.f for threshold in dB.
		 */
		SLATE_ARGUMENT(float, MinValue)
		SLATE_ARGUMENT(float, MaxValue)

		/** Whether to render the param name label above the knob. Default true. */
		SLATE_ARGUMENT(bool, ShowParamName)

		/**
		 * Tooltip text for this knob — shown on hover.
		 * Use FAudioFXTheme::MakeKnobTooltip() for the standard format:
		 *   Range / Description
		 */
		SLATE_ARGUMENT(FText, KnobToolTipText)

		/**
		 * Called when the knob value changes (same signature as SAudioMaterialLabeledKnob).
		 * Receives the new normalized [0,1] value.
		 */
		SLATE_EVENT(FOnFloatValueChanged, OnValueChanged)

		/**
		 * Called whenever the knob value changes (drag or text commit).
		 * Bind this to mark the edited asset's package as dirty.
		 * This fires in ADDITION to OnValueChanged — it's the foolproof dirty hook.
		 */
		SLATE_EVENT(FSimpleDelegate, OnMarkDirty)

		/**
		 * The UObject being edited by this knob (typically a USoundEffectPreset).
		 * When set, the knob automatically calls Modify() + MarkPackageDirty()
		 * on mouse release after a drag, making dirty marking foolproof at the
		 * core level without each editor having to wire it up manually.
		 */
		SLATE_ARGUMENT(TWeakObjectPtr<UObject>, EditedObject)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// ── SWidget overrides ────────────────────────────────────────────

	// Mouse
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override;

	// Keyboard
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual void OnFocusLost(const FFocusEvent& InFocusEvent) override;

	// Focus ring
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	// Tick — used to detect drag end (inner knob loses mouse capture) for dirty marking
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	/** Switch the value display to an editable text box */
	void EnterEditMode();

	/** Commit the typed value and return to knob display */
	void CommitEditedValue(const FText& InText, ETextCommit::Type CommitType);

	/** Show the right-click context menu */
	void ShowContextMenu(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

	/** Copy current value (in real units) to clipboard */
	void CopyValue() const;

	/** Paste value from clipboard */
	void PasteValue();

	/** Reset to default value */
	void ResetToDefault();

public:
	// ── Value Conversion Helpers (public for testability) ────────────

	/** Convert normalized [0,1] → real unit value for display/copy */
	float NormToReal(float Norm) const;

	/** Convert real unit → normalized [0,1] for knob */
	float RealToNorm(float Real) const;

	/** Format the current real value as a string for copy/edit (no units suffix) */
	FString FormatValue(float RealValue) const;

	/** Format the current real value as display FText with unit suffix (e.g. "-24.0 dB") */
	FText GetDisplayText() const;

private:

	// ── State ────────────────────────────────────────────────────────
	FText                           ParamName;
	TAttribute<float>               ValueAttr;
	const FAudioMaterialKnobStyle*  Style              = nullptr;
	EAudioUnitsValueType            UnitsType          = EAudioUnitsValueType::Linear;
	EAudioFXValueFormat             ValueFormat        = EAudioFXValueFormat::Default;
	float                           DefaultValue       = 0.5f;
	float                           MinValue           = 0.f;
	float                           MaxValue           = 1.f;
	bool                            bShowParamName     = true;
	FOnFloatValueChanged            OnValueChangedDelegate;
	FSimpleDelegate                 OnMarkDirtyDelegate;

	/** The UObject being edited — for automatic dirty marking on drag end */
	TWeakObjectPtr<UObject>         EditedObject;

	/** Whether the value actually changed during the current drag gesture */
	bool                            bValueChangedDuringDrag = false;

	/** Whether we are currently in inline-edit mode */
	bool                            bEditMode          = false;

	/** Guard flag: when true, programmatic SetText() won't trigger bEditMode */
	bool                            bSuppressEditModeFromTextChange = false;

	/** Whether the inner knob had mouse capture last tick (for drag-end detection) */
	bool                            bWasDragging       = false;

	/**
	 * True during Construct() while SAudioMaterialKnob is being built.
	 * SAudioMaterialKnob::Construct calls CommitValue() which fires
	 * OnFloatValueChanged — we must ignore that initial fire so we don't
	 * dirty the asset on open.
	 */
	bool                            bConstructing      = false;

	/**
	 * Arrow key step sizes — normalized [0,1] units.
	 * Coarse: 1% of full range.  Fine (Shift held): 0.1% of full range.
	 */
	static constexpr float          KeyStepCoarse      = 0.01f;
	static constexpr float          KeyStepFine        = 0.001f;

	/** Weak ref to the inner knob for value queries */
	TSharedPtr<SAudioMaterialKnob>        InnerKnob;

	/** Always-visible editable text box below the knob */
	TSharedPtr<SEditableTextBox>    EditBox;

	/** The outer switchable slot — holds the knob */
	TSharedPtr<SBox>                ContentBox;
};
