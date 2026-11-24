// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/UIBuilders/AudioFXFilterUI.h"

#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Widgets/SAudioFXDropdown.h"
#include "Shared/Visualizers/SAudioFXFilterCurve.h"
#include "Shared/Widgets/SAudioFXLabeledKnob.h"

#include "AudioWidgetsEnums.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AudioFXFilterUI"

// ============================================================
// Static helpers — called by BuildWidget below
// ============================================================

/** Filter frequency response visualization. */
static TSharedRef<SWidget> BuildFilterCurvePanel(
	FFilterGetCurveType GetCurveType,
	FFilterGetFloat GetCutoffHz,
	FFilterGetFloat GetQVal)
{
	return SNew(SBox)
		.HeightOverride(150.f)
		[
			SNew(SAudioFXFilterCurve)
			.FilterType_Lambda([GetCurveType]()
			{
				return GetCurveType.Execute();
			})
			.CutoffHz_Lambda([GetCutoffHz]() -> float
			{
				return GetCutoffHz.Execute();
			})
			.Q_Lambda([GetQVal]() -> float
			{
				return GetQVal.Execute();
			})
			.CurveColor(FAudioFXTheme::GetParamColor(EAudioFXParamColor::Frequency))
		];
}

/** Circuit and Type dropdown selectors stacked vertically. */
static TSharedRef<SWidget> BuildFilterDropdownsPanel(
	const FFilterUIParams& Params,
	const FLinearColor& AssetCol,
	FFilterGetInt GetCircuit,
	FFilterSetInt SetCircuit,
	FFilterGetInt GetType,
	FFilterSetInt SetType,
	FSimpleDelegate MarkDirty)
{
	return SNew(SVerticalBox)

		// Circuit
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 0.f, 0.f, 6.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("CircuitL", "Circuit:"))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SAudioFXDropdown)
				.Options(Params.CircuitOptions)
				.AssetColor(AssetCol)
				.SelectedIndex(GetCircuit.Execute())
				.OnSelectionChanged_Lambda([MarkDirty, SetCircuit](int32 OptionIndex)
				{
					MarkDirty.ExecuteIfBound();
					SetCircuit.ExecuteIfBound(OptionIndex);
				})
			]
		]

		// Type
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("TypeL", "Type:"))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SAudioFXDropdown)
				.Options(Params.TypeOptions)
				.AssetColor(AssetCol)
				.SelectedIndex(GetType.Execute())
				.OnSelectionChanged_Lambda([MarkDirty, SetType](int32 OptionIndex)
				{
					MarkDirty.ExecuteIfBound();
					SetType.ExecuteIfBound(OptionIndex);
				})
			]
		];
}

// ============================================================
// Public: BuildWidget
// ============================================================

TSharedRef<SWidget> AudioFXFilterUI::BuildWidget(
	FAudioFXEffectEditor& Editor,
	const FFilterUIParams& Params,
	const FLinearColor& AssetCol)
{
	TWeakPtr<FAudioFXEffectEditor> WeakEditor = Editor.AsShared();
	FSimpleDelegate MarkDirty = FSimpleDelegate::CreateLambda([WeakEditor]{ if (auto Ed = WeakEditor.Pin()) { Ed->MarkPresetDirty(); } });

	const FAudioMaterialKnobStyle* FreqStyle = Editor.StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Frequency, AssetCol, EAudioFXKnobSize::Hero));
	const FAudioMaterialKnobStyle* QStyle = Editor.StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Resonance, AssetCol, EAudioFXKnobSize::Primary));

	// Capture delegates by value for lambdas
	FFilterGetCurveType GetCurveType = Params.GetCurveType;
	FFilterGetFloat     GetCutoffHz  = Params.GetCutoffHz;
	FFilterGetFloat     GetQVal      = Params.GetQ;
	FFilterGetFloat     GetCutoff    = Params.GetCutoff;
	FFilterSetFloat     SetCutoff    = Params.SetCutoff;
	FFilterGetFloat     GetRes       = Params.GetResonance;
	FFilterSetFloat     SetRes       = Params.SetResonance;
	FFilterGetInt       GetCircuit   = Params.GetCircuit;
	FFilterSetInt       SetCircuit   = Params.SetCircuit;
	FFilterGetInt       GetType      = Params.GetType;
	FFilterSetInt       SetType      = Params.SetType;

	const float FreqMin = Params.FreqMin;
	const float FreqMax = Params.FreqMax;
	const float QMin    = Params.QMin;
	const float QMax    = Params.QMax;

	return SNew(SBox)
		.Padding(FMargin(FAudioFXTheme::WindowPadding, FAudioFXTheme::SectionPadding))
		[
			SNew(SVerticalBox)

			// ── Filter Curve ─────────────────────────────────────
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				BuildFilterCurvePanel(GetCurveType, GetCutoffHz, GetQVal)
			]

			// ── Controls row: 3 equal columns so Cutoff is always window-center ──
			// LEFT col (FillWidth 1): dropdowns, left-aligned within the column.
			// CENTER col (FillWidth 1): Cutoff hero knob, centered.
			// RIGHT col (FillWidth 1): Resonance knob, centered.
			// Equal FillWidth = each column is 1/3 of the available width →
			// the center column's midpoint is exactly the window midpoint.
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SNew(SHorizontalBox)

				// LEFT: Circuit + Type dropdowns, left-aligned
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(0.f, 0.f, FAudioFXTheme::SectionGap, 0.f)
				[
					BuildFilterDropdownsPanel(Params, AssetCol, GetCircuit, SetCircuit, GetType, SetType, MarkDirty)
				]

				// CENTER: Cutoff hero knob — true window center
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SAudioFXLabeledKnob)
					.ParamName(LOCTEXT("CutoffLbl", "Cutoff"))
					.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
						LOCTEXT("CutoffTipLbl", "Cutoff Frequency"),
						FreqMin, FreqMax, TEXT("Hz"),
						LOCTEXT("CutoffTip",
							"The filter cutoff frequency.\n"
							"AudioFactory Note: The 1-Pole circuit rolls off gently at 6 dB/oct; "
							"State Variable and Ladder provide steeper slopes with resonance.")))
					.Style(FreqStyle)
					.AudioUnitsValueType(EAudioUnitsValueType::Frequency)
					.ValueFormat(EAudioFXValueFormat::Frequency)
					.MinValue(FreqMin)
					.MaxValue(FreqMax)
					.Value_Lambda([GetCutoff, FreqMin, FreqMax]() -> float
					{
						return FAudioFXTheme::LogFreqToNorm(GetCutoff.Execute(), FreqMin, FreqMax);
					})
					.OnValueChanged_Lambda([MarkDirty, SetCutoff, FreqMin, FreqMax](float NewNorm)
					{
						MarkDirty.ExecuteIfBound();
						SetCutoff.ExecuteIfBound(FAudioFXTheme::NormToLogFreq(NewNorm, FreqMin, FreqMax));
					})
				]

				// RIGHT: Resonance knob, centered in its column
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(FAudioFXTheme::SectionGap, 0.f, 0.f, 0.f)
				[
					SNew(SAudioFXLabeledKnob)
					.ParamName(LOCTEXT("QLbl", "Resonance"))
					.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
						LOCTEXT("ResTipLbl", "Resonance (Q)"),
						QMin, QMax, TEXT(""),
						LOCTEXT("ResTip",
							"The filter resonance.\n"
							"AudioFactory Note: High Q values near the cutoff can cause a sharp peak; "
							"the Ladder circuit self-oscillates at extreme settings.")))
					.Style(QStyle)
					.AudioUnitsValueType(EAudioUnitsValueType::Linear)
					.ValueFormat(EAudioFXValueFormat::QFactor)
					.MinValue(QMin)
					.MaxValue(QMax)
					.Value_Lambda([GetRes, QMin, QMax]() -> float
					{
						return FMath::GetMappedRangeValueClamped(
							FVector2D(QMin, QMax), FVector2D(0.f, 1.f),
							GetRes.Execute());
					})
					.OnValueChanged_Lambda([MarkDirty, SetRes, QMin, QMax](float NewNorm)
					{
						MarkDirty.ExecuteIfBound();
						SetRes.ExecuteIfBound(FMath::GetMappedRangeValueClamped(
							FVector2D(0.f, 1.f), FVector2D(QMin, QMax), NewNorm));
					})
				]
			]
		];
}

#undef LOCTEXT_NAMESPACE
