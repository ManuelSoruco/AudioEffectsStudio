// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Widgets/SAudioFXLabeledKnob.h"
#include "Shared/Core/AudioFXTheme.h"

#include "AudioMaterialSlate/SAudioMaterialKnob.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Misc/DefaultValueHelper.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Input/Events.h"
#include "Rendering/DrawElementTypes.h"

#define LOCTEXT_NAMESPACE "SAudioFXLabeledKnob"

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

void SAudioFXLabeledKnob::Construct(const FArguments& InArgs)
{
	// ── Step 1: bind arguments ──────────────────────────────────────────────
	ParamName          = InArgs._ParamName;
	ValueAttr          = InArgs._Value;
	Style              = InArgs._Style;
	UnitsType          = InArgs._AudioUnitsValueType;
	ValueFormat        = InArgs._ValueFormat;
	DefaultValue       = InArgs._DefaultValue;
	MinValue           = InArgs._MinValue;
	MaxValue           = InArgs._MaxValue;
	bShowParamName     = InArgs._ShowParamName;
	OnValueChangedDelegate = InArgs._OnValueChanged;
	OnMarkDirtyDelegate    = InArgs._OnMarkDirty;
	EditedObject           = InArgs._EditedObject;

	const FText KnobTooltip = InArgs._KnobToolTipText;

	// SAudioMaterialKnob::Construct calls CommitValue() which fires OnFloatValueChanged.
	// Guard against that initial fire so we never dirty the asset on open.
	bConstructing = true;
	TSharedRef<SAudioMaterialKnob> KnobWidget =
		SAssignNew(InnerKnob, SAudioMaterialKnob)
		.Value(ValueAttr)
		.AudioMaterialKnobStyle(Style)
		.OnFloatValueChanged_Lambda([this](float NewNorm)
		{
			// Ignore the initial CommitValue() fired during SAudioMaterialKnob::Construct.
			if (bConstructing) { return; }

			// Phase A: notify owner of value change.
			// The owner (typically a USoundEffectPreset editor lambda) stores the
			// new normalized value back onto the preset's property.
			OnValueChangedDelegate.ExecuteIfBound(NewNorm);
			// Track that a value change occurred during this drag gesture.
			// Dirty marking happens on mouse RELEASE (see Tick), not every tick.
			bValueChangedDuringDrag = true;

			// Phase B: live-update the edit box to reflect the dragged value.
			// We compute real value from NewNorm here because ValueAttr hasn't been
			// updated yet (the outer preset lambda updates it after this fires).
			if (!bEditMode && EditBox.IsValid())
			{
				float RealVal;
				if (FMath::IsNearlyEqual(MinValue, MaxValue))
				{
					RealVal = NewNorm;
				}
				else if (UnitsType == EAudioUnitsValueType::Frequency)
				{
					RealVal = FAudioFXTheme::NormToLogFreq(NewNorm, MinValue, MaxValue);
				}
				else
				{
					RealVal = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 1.f), FVector2D(MinValue, MaxValue), NewNorm);
				}

				// Phase C: format the real value and push it to the edit box
				FText DisplayText;
				if (ValueFormat != EAudioFXValueFormat::Default)
				{
					DisplayText = FText::FromString(FAudioFXTheme::FormatParameterValue(RealVal, ValueFormat));
				}
				else
				{
					// Format with unit suffix for display
					switch (UnitsType)
					{
					case EAudioUnitsValueType::Volume:
						DisplayText = FText::FromString(FString::Printf(TEXT("%.1f dB"), RealVal)); break;
					case EAudioUnitsValueType::Frequency:
						if (RealVal >= 1000.f)
							DisplayText = FText::FromString(FString::Printf(TEXT("%.2f kHz"), RealVal / 1000.f));
						else
							DisplayText = FText::FromString(FString::Printf(TEXT("%.0f Hz"), RealVal));
						break;
					default:
						DisplayText = FText::FromString(FString::Printf(TEXT("%.2f"), RealVal)); break;
					}
				}
			bSuppressEditModeFromTextChange = true;
				EditBox->SetText(DisplayText);
				bSuppressEditModeFromTextChange = false;
			}
		});
	bConstructing = false;

	// ── Step 3: always-visible editable text box ──────────────────────────
	// The value is always editable: click to type, Enter/Escape to commit/cancel.
	// Note: We do NOT bind .Text as a TAttribute lambda because SEditableTextBox
	// only reads its Text attribute once at construction / on explicit SetText() calls.
	// We keep the display up-to-date by calling SetText() from CommitEditedValue().
	TSharedRef<SEditableTextBox> ValueEditBox =
		SAssignNew(EditBox, SEditableTextBox)
		.Text(GetDisplayText())
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
		.ForegroundColor(FAudioFXTheme::GetValueTextColor())
		.BackgroundColor(FAudioFXTheme::GetSurfaceRaised())
		.Justification(ETextJustify::Center)
		.SelectAllTextWhenFocused(true)
		.RevertTextOnEscape(true)
		.OnTextChanged_Lambda([this](const FText&)
		{
			// Only enter edit mode if the text change came from user typing,
			// NOT from our own programmatic SetText() calls during knob drag.
			if (!bSuppressEditModeFromTextChange)
			{
				bEditMode = true;
			}
		})
		.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type CommitType)
		{
			CommitEditedValue(Text, CommitType);
		});

	// ── Step 4: knob container box ────────────────────────────────────────
	// ContentBox holds the bare knob.
	TSharedRef<SBox> KnobBox = SAssignNew(ContentBox, SBox)
		.HAlign(HAlign_Center)
		[
			KnobWidget
		];

	// ── Step 5: outer layout (optional name label + knob + value box) ──────
	if (bShowParamName && !ParamName.IsEmpty())
	{
		ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.f, 0.f, 0.f, 2.f)
			[
				SNew(STextBlock)
				.Text(ParamName)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
				.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
				.Justification(ETextJustify::Center)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				KnobBox
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.Padding(0.f, 2.f, 0.f, 0.f)
			[
				ValueEditBox
			]
		];
	}
	else
	{
		ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				KnobBox
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.Padding(0.f, 2.f, 0.f, 0.f)
			[
				ValueEditBox
			]
		];
	}

	// Apply tooltip if provided
	if (!KnobTooltip.IsEmpty())
	{
		SetToolTipText(KnobTooltip);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Mouse event overrides
// ─────────────────────────────────────────────────────────────────────────────

FReply SAudioFXLabeledKnob::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// Ctrl + Left click → reset to default
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
		&& MouseEvent.IsControlDown())
	{
		ResetToDefault();
		return FReply::Handled();
	}

	// Right click → context menu
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		ShowContextMenu(MyGeometry, MouseEvent);
		return FReply::Handled();
	}

	return SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
}

FReply SAudioFXLabeledKnob::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	// Left double-click → enter inline edit mode
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
		&& !InMouseEvent.IsControlDown())
	{
		EnterEditMode();
		return FReply::Handled();
	}

	return SCompoundWidget::OnMouseButtonDoubleClick(InMyGeometry, InMouseEvent);
}

// ─────────────────────────────────────────────────────────────────────────────
// Keyboard navigation
// ─────────────────────────────────────────────────────────────────────────────

FReply SAudioFXLabeledKnob::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	// Ignore key input while the inline edit box is active — it handles its own keys
	if (bEditMode) { return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent); }

	const FKey Key         = InKeyEvent.GetKey();
	const bool bShift      = InKeyEvent.IsShiftDown();
	const bool bCtrl       = InKeyEvent.IsControlDown();

	// ── Arrow keys: adjust value ──────────────────────────────────────
	if (Key == EKeys::Up || Key == EKeys::Right || Key == EKeys::Down || Key == EKeys::Left)
	{
		const bool bIncrease = (Key == EKeys::Up || Key == EKeys::Right);
		const float Step     = bShift ? KeyStepFine : KeyStepCoarse;
		const float Current  = ValueAttr.Get(DefaultValue);
		const float NewVal   = FMath::Clamp(Current + (bIncrease ? Step : -Step), 0.f, 1.f);
		OnValueChangedDelegate.ExecuteIfBound(NewVal);
		OnMarkDirtyDelegate.ExecuteIfBound();
		return FReply::Handled();
	}

	// ── Enter / Space: open inline edit ──────────────────────────────
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar)
	{
		EnterEditMode();
		return FReply::Handled();
	}

	// ── Backspace / Delete: reset to default ─────────────────────────
	if (Key == EKeys::BackSpace || Key == EKeys::Delete)
	{
		ResetToDefault();
		return FReply::Handled();
	}

	// ── Escape: relinquish focus ──────────────────────────────────────
	if (Key == EKeys::Escape)
	{
		if (HasAnyUserFocus())
		{
			FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::Cleared);
		}
		return FReply::Handled();
	}

	// ── Ctrl+Z / Ctrl+Y: undo / redo ─────────────────────────────────
	// The knob fires OnValueChanged → editor calls Preset->Modify() before writing,
	// which registers the change with UE's transaction system.
	// We forward to the standard UE command so the global undo stack handles it.
	if (bCtrl && (Key == EKeys::Z || Key == EKeys::Y))
	{
		// Route to Slate's global command processing (maps to GEditor undo/redo)
		return FReply::Unhandled();  // Let the parent window command bindings handle Ctrl+Z/Y
	}

	// ── Tab: forward to next focusable widget ─────────────────────────
	// SCompoundWidget's default Tab handling navigates focus correctly — just unhandle.
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

void SAudioFXLabeledKnob::OnFocusLost(const FFocusEvent& InFocusEvent)
{
	// If focus moved to something outside this widget (not to our own edit box),
	// exit edit mode without committing.
	if (bEditMode && InFocusEvent.GetCause() != EFocusCause::SetDirectly)
	{
		CommitEditedValue(FText::GetEmpty(), ETextCommit::OnCleared);  // revert (OnCleared = no write)
	}
	SCompoundWidget::OnFocusLost(InFocusEvent);
}

// ─────────────────────────────────────────────────────────────────────────────
// Focus ring visual
// ─────────────────────────────────────────────────────────────────────────────

int32 SAudioFXLabeledKnob::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	// Paint children first
	const int32 MaxLayer = SCompoundWidget::OnPaint(
		Args, AllottedGeometry, MyCullingRect,
		OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	// Draw a subtle rounded focus ring when this widget has keyboard focus
	if (HasAnyUserFocus() && !bEditMode)
	{
		// Keyboard focus ring: blue outline at 60% opacity
		static FSlateRoundedBoxBrush FocusRingBrush(
			FLinearColor(0.f, 0.f, 0.f, 0.f),           // transparent fill
			FAudioFXTheme::ButtonRadius,
			FAudioFXTheme::GetFocusRingColor(),          // #4488FF
			1.5f);

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			MaxLayer + 1,
			AllottedGeometry.ToPaintGeometry(),
			&FocusRingBrush,
			ESlateDrawEffect::None,
			FLinearColor(1.f, 1.f, 1.f, 0.6f));
	}

	return MaxLayer + 1;
}

// ─────────────────────────────────────────────────────────────────────────────
// Tick — detect drag end by monitoring inner knob's mouse capture state
// ─────────────────────────────────────────────────────────────────────────────

void SAudioFXLabeledKnob::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (InnerKnob.IsValid())
	{
		const bool bIsDragging = InnerKnob->HasMouseCapture();
		if (bWasDragging && !bIsDragging && bValueChangedDuringDrag)
		{
			// User just released the mouse after dragging the knob — mark dirty once
			// via the centralized delegate (routes through MarkPresetDirty).
			OnMarkDirtyDelegate.ExecuteIfBound();
			bValueChangedDuringDrag = false;
		}
		bWasDragging = bIsDragging;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Edit mode
// ─────────────────────────────────────────────────────────────────────────────

void SAudioFXLabeledKnob::EnterEditMode()
{
	if (bEditMode || !EditBox.IsValid()) { return; }

	// Pre-fill the edit box with the current formatted value (without unit suffix)
	const float CurrentReal = NormToReal(ValueAttr.Get(0.5f));
	EditBox->SetText(FText::FromString(FormatValue(CurrentReal)));
	bEditMode = true;

	// Give focus to the always-present edit box
	FSlateApplication::Get().SetKeyboardFocus(EditBox.ToSharedRef(), EFocusCause::SetDirectly);
}

void SAudioFXLabeledKnob::CommitEditedValue(const FText& InText, ETextCommit::Type CommitType)
{
	if (!bEditMode) { return; }
	bEditMode = false;

	if (CommitType != ETextCommit::OnCleared)  // OnCleared = Escape → revert, no write
	{
		// Parse the typed string as a float in real units
		float ParsedReal = NormToReal(ValueAttr.Get(0.5f));  // fallback: keep current
		float TryVal = 0.f;
		if (FDefaultValueHelper::ParseFloat(InText.ToString(), TryVal))
		{
			ParsedReal = FMath::Clamp(TryVal, MinValue, MaxValue);
		}
		const float NewNorm = RealToNorm(ParsedReal);
		OnValueChangedDelegate.ExecuteIfBound(NewNorm);

		// Route dirty marking through the centralized delegate.
		OnMarkDirtyDelegate.ExecuteIfBound();
	}

	// The EditBox is always visible — update its display text to the committed value
	if (EditBox.IsValid())
	{
		bSuppressEditModeFromTextChange = true;
		EditBox->SetText(GetDisplayText());
		bSuppressEditModeFromTextChange = false;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Context menu
// ─────────────────────────────────────────────────────────────────────────────

void SAudioFXLabeledKnob::ShowContextMenu(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FMenuBuilder MenuBuilder(true, nullptr);

	// Label showing current value
	const float CurrentReal = NormToReal(ValueAttr.Get(0.5f));
	const FString CurrentStr = FormatValue(CurrentReal);
	const FText CurrentValueText = FText::Format(
		LOCTEXT("CurrentValue", "Value: {0}"), FText::FromString(CurrentStr));

	MenuBuilder.BeginSection("AudioFXKnobActions", ParamName.IsEmpty()
		? LOCTEXT("KnobActions", "Knob")
		: ParamName);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Reset", "Reset to Default"),
		LOCTEXT("ResetTT", "Ctrl+Click to reset"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &SAudioFXLabeledKnob::ResetToDefault)));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("EditValue", "Enter Value…"),
		LOCTEXT("EditValueTT", "Double-click to type a value"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &SAudioFXLabeledKnob::EnterEditMode)));

	MenuBuilder.AddSeparator();

	MenuBuilder.AddMenuEntry(
		FText::Format(LOCTEXT("CopyFmt", "Copy  ({0})"), FText::FromString(CurrentStr)),
		LOCTEXT("CopyTT", "Copy the current value to clipboard"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &SAudioFXLabeledKnob::CopyValue)));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Paste", "Paste Value"),
		LOCTEXT("PasteTT", "Paste a value from clipboard"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &SAudioFXLabeledKnob::PasteValue)));

	MenuBuilder.EndSection();

	const FWidgetPath WidgetPath = FSlateApplication::Get().LocateWindowUnderMouse(
		MouseEvent.GetScreenSpacePosition(),
		FSlateApplication::Get().GetInteractiveTopLevelWindows());

	FSlateApplication::Get().PushMenu(
		AsShared(),
		WidgetPath,
		MenuBuilder.MakeWidget(),
		MouseEvent.GetScreenSpacePosition(),
		FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
}

void SAudioFXLabeledKnob::CopyValue() const
{
	const float RealVal = NormToReal(ValueAttr.Get(0.5f));
	FPlatformApplicationMisc::ClipboardCopy(*FormatValue(RealVal));
}

void SAudioFXLabeledKnob::PasteValue()
{
	FString ClipText;
	FPlatformApplicationMisc::ClipboardPaste(ClipText);

	float TryVal = 0.f;
	if (FDefaultValueHelper::ParseFloat(ClipText, TryVal))
	{
		const float Clamped = FMath::Clamp(TryVal, MinValue, MaxValue);
		OnValueChangedDelegate.ExecuteIfBound(RealToNorm(Clamped));
		OnMarkDirtyDelegate.ExecuteIfBound();
	}
}

void SAudioFXLabeledKnob::ResetToDefault()
{
	OnValueChangedDelegate.ExecuteIfBound(DefaultValue);
	OnMarkDirtyDelegate.ExecuteIfBound();
}

// ─────────────────────────────────────────────────────────────────────────────
// Value conversion helpers
// ─────────────────────────────────────────────────────────────────────────────

float SAudioFXLabeledKnob::NormToReal(float Norm) const
{
	// Frequency knobs use logarithmic mapping.
	// If min/max were left at their defaults (0,1) — which are not valid Hz values —
	// fall back to the standard audio range so the display doesn't show "0 Hz" / "1 Hz".
	if (UnitsType == EAudioUnitsValueType::Frequency)
	{
		const float FreqMin = (MinValue < 1.f) ? 20.f    : MinValue;
		const float FreqMax = (MaxValue < 1.f) ? 20000.f : MaxValue;
		return FAudioFXTheme::NormToLogFreq(Norm, FreqMin, FreqMax);
	}
	// If MinValue == MaxValue (not configured), return norm as-is
	if (FMath::IsNearlyEqual(MinValue, MaxValue)) { return Norm; }
	return FMath::GetMappedRangeValueClamped(FVector2D(0.f, 1.f), FVector2D(MinValue, MaxValue), Norm);
}

float SAudioFXLabeledKnob::RealToNorm(float Real) const
{
	if (UnitsType == EAudioUnitsValueType::Frequency)
	{
		const float FreqMin = (MinValue < 1.f) ? 20.f    : MinValue;
		const float FreqMax = (MaxValue < 1.f) ? 20000.f : MaxValue;
		return FAudioFXTheme::LogFreqToNorm(Real, FreqMin, FreqMax);
	}
	if (FMath::IsNearlyEqual(MinValue, MaxValue)) { return Real; }
	return FMath::GetMappedRangeValueClamped(FVector2D(MinValue, MaxValue), FVector2D(0.f, 1.f), Real);
}

FString SAudioFXLabeledKnob::FormatValue(float RealValue) const
{
	// Format based on units type for sensible copy/paste strings
	switch (UnitsType)
	{
	case EAudioUnitsValueType::Volume:
		return FString::Printf(TEXT("%.1f"), RealValue);  // dB
	case EAudioUnitsValueType::Frequency:
		if (RealValue >= 1000.f) { return FString::Printf(TEXT("%.2f"), RealValue / 1000.f); }  // kHz
		return FString::Printf(TEXT("%.0f"), RealValue);  // Hz
	case EAudioUnitsValueType::Linear:
		return FString::Printf(TEXT("%.2f"), RealValue);  // 2 decimal places
	default:
		return FString::Printf(TEXT("%.2f"), RealValue);  // 2 decimal places
	}
}

FText SAudioFXLabeledKnob::GetDisplayText() const
{
	const float RealVal = NormToReal(ValueAttr.Get(DefaultValue));

	// Use extended format when specified
	if (ValueFormat != EAudioFXValueFormat::Default)
	{
		return FText::FromString(FAudioFXTheme::FormatParameterValue(RealVal, ValueFormat));
	}

	// Legacy path: use EAudioUnitsValueType
	switch (UnitsType)
	{
	case EAudioUnitsValueType::Volume:
		return FText::FromString(FString::Printf(TEXT("%.1f dB"), RealVal));
	case EAudioUnitsValueType::Frequency:
		if (RealVal >= 1000.f)
		{
			return FText::FromString(FString::Printf(TEXT("%.2f kHz"), RealVal / 1000.f));
		}
		return FText::FromString(FString::Printf(TEXT("%.0f Hz"), RealVal));
	case EAudioUnitsValueType::Linear:
		return FText::FromString(FString::Printf(TEXT("%.2f"), RealVal));  // 2 decimal places
	default:
		return FText::FromString(FString::Printf(TEXT("%.2f"), RealVal));  // 2 decimal places
	}
}

#undef LOCTEXT_NAMESPACE
