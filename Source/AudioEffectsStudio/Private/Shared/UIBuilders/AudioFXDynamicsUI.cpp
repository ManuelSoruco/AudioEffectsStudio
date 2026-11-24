// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/UIBuilders/AudioFXDynamicsUI.h"

#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Widgets/SAudioFXDropdown.h"
#include "Shared/Widgets/SAudioFXButton.h"
#include "Shared/Widgets/SAudioFXCollapsibleSection.h"
#include "Shared/Widgets/SAudioFXLabeledKnob.h"
#include "Shared/Visualizers/SAudioFXTransferCurve.h"

#include "AssetRegistry/AssetData.h"
#include "AudioWidgetsEnums.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "PropertyCustomizationHelpers.h"
#include "Sound/AudioBus.h"
#include "Sound/SoundSubmix.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AudioFXDynamicsUI"

// ═══════════════════════════════════════════════════════════════════════════
//  Local helpers
// ═══════════════════════════════════════════════════════════════════════════

static float MapToNorm(float Val, float Min, float Max)
{
	return FMath::GetMappedRangeValueClamped(
		FVector2D(Min, Max),
		FVector2D(0.f, 1.f),
		Val);
}

static float NormToVal(float NewNorm, float Min, float Max)
{
	return FMath::GetMappedRangeValueClamped(
		FVector2D(0.f, 1.f),
		FVector2D(Min, Max), NewNorm);
}

// ═══════════════════════════════════════════════════════════════════════════
//  Transfer Curve Panel (left side, 300x300)
// ═══════════════════════════════════════════════════════════════════════════

/** Build the 300×300 transfer-curve visualizer panel with IN/OUT axis labels. */
static TSharedRef<SWidget> BuildTransferCurvePanel(
	const FDynamicsUIParams& P,
	const FLinearColor& AssetCol)
{
	FDynGetTransferSettings GetSettings = P.GetTransferSettings;

	return SNew(SBox)
		.WidthOverride(300.f)
		.HeightOverride(300.f)
		[
			SNew(SOverlay)

			+ SOverlay::Slot()
			[
				SNew(SAudioFXTransferCurve)
				.AssetColor(AssetCol)
				.Settings_Lambda([GetSettings]() -> FSourceEffectDynamicsProcessorSettings
				{
					return GetSettings.Execute();
				})
			]

			+ SOverlay::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			.Padding(4.f, 3.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("OutLabel", "OUT"))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(FAudioFXTheme::GetTextDisabledColor())
			]

			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Bottom)
			.Padding(4.f, 3.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("InLabel", "IN"))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(FAudioFXTheme::GetTextDisabledColor())
			]
		];
}

// ═══════════════════════════════════════════════════════════════════════════
//  Type + Bypass row
// ═══════════════════════════════════════════════════════════════════════════

/** Build the Type dropdown row at the top of the right-hand panel. */
static TSharedRef<SWidget> BuildTypeRow(
	const FDynamicsUIParams& P,
	const FLinearColor& AssetCol,
	FSimpleDelegate MarkDirty,
	TSharedRef<int32> CurrentType)
{
	FDynSetInt SetType = P.SetType;

	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.f, 0.f, 6.f, 0.f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("TypeL", "Type"))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
			.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SAudioFXDropdown)
			.Options(P.TypeOptions)
			.AssetColor(AssetCol)
			.SelectedIndex(*CurrentType)
			.OnSelectionChanged_Lambda([MarkDirty, SetType, CurrentType](int32 OptionIndex)
			{
				MarkDirty.ExecuteIfBound();
				SetType.ExecuteIfBound(OptionIndex);
				*CurrentType = OptionIndex;
			})
		];
}

// ═══════════════════════════════════════════════════════════════════════════
//  Dynamics knob row: Threshold (Hero) | Ratio (conditional) | Knee (conditional)
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Build the primary Threshold (hero) | Ratio | Knee knob row.
 * Ratio and Knee knobs are visibility-toggled via ShouldShowRatio / ShouldShowKnee
 * based on the currently selected processor type.
 */
static TSharedRef<SWidget> BuildDynamicsRow(
	const FDynamicsUIParams& P,
	const FLinearColor& AssetCol,
	FAudioFXEffectEditor& Editor,
	FSimpleDelegate MarkDirty,
	TSharedRef<int32> CurrentType)
{
	// ── Styles ───────────────────────────────────────────────────
	const FAudioMaterialKnobStyle* ThreshStyle = Editor.StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Dynamics, AssetCol, EAudioFXKnobSize::Hero));
	const FAudioMaterialKnobStyle* RatioStyle = Editor.StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Dynamics, AssetCol, EAudioFXKnobSize::Primary));
	const FAudioMaterialKnobStyle* KneeStyle = Editor.StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Dynamics, AssetCol, EAudioFXKnobSize::Tertiary));

	// ── Capture delegates by value ──────────────────────────────
	FDynGetFloat GetThresh = P.GetThreshold;
	FDynSetFloat SetThresh = P.SetThreshold;
	FDynGetFloat GetRatio  = P.GetRatio;
	FDynSetFloat SetRatio  = P.SetRatio;
	FDynGetFloat GetKnee   = P.GetKnee;
	FDynSetFloat SetKnee   = P.SetKnee;

	const float ThMin = P.ThreshMin, ThMax = P.ThreshMax;
	const float RMin  = P.RatioMin,  RMax  = P.RatioMax;
	const float KMin  = P.KneeMin,   KMax  = P.KneeMax;

	TFunction<bool(int32)> ShowRatio = P.ShouldShowRatio;
	TFunction<bool(int32)> ShowKnee  = P.ShouldShowKnee;

	// ── Ratio widget ────────────────────────────────────────────
	TSharedPtr<SWidget> RatioWidget;
	SAssignNew(RatioWidget, SBox)
		.Visibility_Lambda([CurrentType, ShowRatio]() -> EVisibility
		{
			return ShowRatio(*CurrentType) ? EVisibility::Visible : EVisibility::Collapsed;
		})
		[
			SNew(SBox)
			.HAlign(HAlign_Center)
			.Padding(FAudioFXTheme::KnobHorizontalSpacing, 4.f)
			[
				SNew(SAudioFXLabeledKnob)
				.ParamName(LOCTEXT("RatioName", "Ratio"))
				.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
					LOCTEXT("RatioLbl", "Ratio"),
					RMin,
					RMax,
					TEXT(":1"),
					LOCTEXT("RatioTip", "The dynamics processor ratio used for compression/expansion.\nAudioFactory Note: Upward-compression gain is internally clamped at 36 dB.")))
				.Style(RatioStyle)
				.AudioUnitsValueType(EAudioUnitsValueType::Linear)
				.ValueFormat(EAudioFXValueFormat::Ratio)
				.MinValue(RMin)
				.MaxValue(RMax)
				.Value_Lambda([GetRatio, RMin, RMax]() -> float
				{
					return MapToNorm(GetRatio.Execute(), RMin, RMax);
				})
				.OnValueChanged_Lambda([MarkDirty, SetRatio, RMin, RMax](float NewNorm)
				{
					MarkDirty.ExecuteIfBound();
					SetRatio.ExecuteIfBound(NormToVal(NewNorm, RMin, RMax));
				})
			]
		];

	// ── Knee widget ─────────────────────────────────────────────
	TSharedPtr<SWidget> KneeWidget;
	SAssignNew(KneeWidget, SBox)
		.Visibility_Lambda([CurrentType, ShowKnee]() -> EVisibility
		{
			return ShowKnee(*CurrentType) ? EVisibility::Visible : EVisibility::Collapsed;
		})
		[
			SNew(SBox)
			.HAlign(HAlign_Center)
			.Padding(FAudioFXTheme::KnobHorizontalSpacing, 4.f)
			[
				SNew(SAudioFXLabeledKnob)
				.ParamName(LOCTEXT("KneeName", "Knee"))
				.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
					LOCTEXT("KneeLbl", "Knee"),
					KMin,
					KMax,
					TEXT("dB"),
					LOCTEXT("KneeTip", "Defines how hard or soft the gain reduction blends.\nAudioFactory Note: 0 dB = hard knee. Uses Lagrangian interpolation for the soft-knee curve.")))
				.Style(KneeStyle)
				.AudioUnitsValueType(EAudioUnitsValueType::Volume)
				.ValueFormat(EAudioFXValueFormat::Decibels)
				.MinValue(KMin)
				.MaxValue(KMax)
				.Value_Lambda([GetKnee, KMin, KMax]() -> float
				{
					return MapToNorm(GetKnee.Execute(), KMin, KMax);
				})
				.OnValueChanged_Lambda([MarkDirty, SetKnee, KMin, KMax](float NewNorm)
				{
					MarkDirty.ExecuteIfBound();
					SetKnee.ExecuteIfBound(NormToVal(NewNorm, KMin, KMax));
				})
			]
		];

	// ── Threshold + row ─────────────────────────────────────────
	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.FillWidth(0.85f)
		.Padding(0.f)
		.VAlign(VAlign_Center)
		[
			RatioWidget.ToSharedRef()
		]

		+ SHorizontalBox::Slot()
		.FillWidth(1.30f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
		[
			SNew(SAudioFXLabeledKnob)
			.ParamName(LOCTEXT("ThreshName", "Threshold"))
			.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
				LOCTEXT("ThreshLbl", "Threshold"),
				ThMin,
				ThMax,
				TEXT("dB"),
				LOCTEXT("ThreshTip", "The threshold at which to perform a dynamics processing operation.\nAudioFactory Note: At \u221260 dB with a high ratio the compressor acts as a noise gate.")))
			.Style(ThreshStyle)
			.AudioUnitsValueType(EAudioUnitsValueType::Volume)
			.ValueFormat(EAudioFXValueFormat::Decibels)
			.MinValue(ThMin)
			.MaxValue(ThMax)
			.Value_Lambda([GetThresh, ThMin, ThMax]() -> float
			{
				return MapToNorm(GetThresh.Execute(), ThMin, ThMax);
			})
			.OnValueChanged_Lambda([MarkDirty, SetThresh, ThMin, ThMax](float NewNorm)
			{
				MarkDirty.ExecuteIfBound();
				SetThresh.ExecuteIfBound(NormToVal(NewNorm, ThMin, ThMax));
			})
		]

		+ SHorizontalBox::Slot()
		.FillWidth(0.85f)
		.Padding(0.f)
		.VAlign(VAlign_Center)
		[
			KneeWidget.ToSharedRef()
		];
}

// ═══════════════════════════════════════════════════════════════════════════
//  Response & Gain section (collapsible)
//  Layout: Row 1 — [Attack | Release] + [Look Ahead] + [Input / Output gain]
//          Row 2 — Peak mode | Link mode | Analog toggle
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Build the collapsible RESPONSE & GAIN section.
 * Contains:  Row 1 — Attack | Release | LookAhead | InputGain / OutputGain
 *            Row 3 — Peak mode | Link mode | Analog toggle
 * Link mode renders as a 3-way dropdown (submix) or a single bool toggle (source)
 * depending on whether LinkOptions is populated.
 */
static TSharedRef<SWidget> BuildResponseSection(
	const FDynamicsUIParams& P,
	const FLinearColor& AssetCol,
	FAudioFXEffectEditor& Editor,
	FSimpleDelegate MarkDirty)
{
	// ── Styles ───────────────────────────────────────────────────
	const FAudioMaterialKnobStyle* AttackStyle    = Editor.StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Time, AssetCol, EAudioFXKnobSize::Primary));
	const FAudioMaterialKnobStyle* ReleaseStyle   = Editor.StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Time, AssetCol, EAudioFXKnobSize::Primary));
	const FAudioMaterialKnobStyle* LookAheadStyle = Editor.StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Time, AssetCol, EAudioFXKnobSize::Tertiary));
	const FAudioMaterialKnobStyle* InGainStyle    = Editor.StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain, AssetCol, EAudioFXKnobSize::Secondary));
	const FAudioMaterialKnobStyle* OutGainStyle   = Editor.StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain, AssetCol, EAudioFXKnobSize::Secondary));

	// ── Capture delegates ────────────────────────────────────────
	FDynGetFloat GetAttack    = P.GetAttack;
	FDynSetFloat SetAttack    = P.SetAttack;
	FDynGetFloat GetRelease   = P.GetRelease;
	FDynSetFloat SetRelease   = P.SetRelease;
	FDynGetFloat GetLookAhead = P.GetLookAhead;
	FDynSetFloat SetLookAhead = P.SetLookAhead;
	FDynGetFloat GetInGain    = P.GetInputGain;
	FDynSetFloat SetInGain    = P.SetInputGain;
	FDynGetFloat GetOutGain   = P.GetOutputGain;
	FDynSetFloat SetOutGain   = P.SetOutputGain;
	FDynGetInt   GetPeak      = P.GetPeakMode;
	FDynSetInt   SetPeak      = P.SetPeakMode;
	FDynGetBool  GetAnalog    = P.GetAnalog;
	FDynSetBool  SetAnalog    = P.SetAnalog;

	const float AtkMin = P.AttackMin, AtkMax = P.AttackMax;
	const float RelMin = P.ReleaseMin, RelMax = P.ReleaseMax;
	const float LaMin  = P.LookAheadMin, LaMax = P.LookAheadMax;
	const float IgMin  = P.InputGainMin, IgMax = P.InputGainMax;
	const float OgMin  = P.OutputGainMin, OgMax = P.OutputGainMax;

	// ── Link mode — either a 3-way strip or a single bool toggle ──
	const bool bUseLinkStrip = P.LinkOptions.Num() > 0;
	TArray<FText> LinkOpts   = P.LinkOptions;
	FDynGetInt  GetLink      = P.GetLinkMode;
	FDynSetInt  SetLink      = P.SetLinkMode;
	FDynGetBool GetStereoLnk = P.GetStereoLinked;
	FDynSetBool SetStereoLnk = P.SetStereoLinked;

	TArray<FText> PeakOpts = P.PeakOptions;

	const bool bExpandDefault = Editor.ShouldExpandAllSections();

	return SNew(SAudioFXCollapsibleSection)
		.SectionLabel(LOCTEXT("AdvLabel", "RESPONSE & GAIN"))
		.bDefaultExpanded(bExpandDefault)
		.OnToggled_Lambda([bExp = TSharedPtr<bool>(MakeShared<bool>(bExpandDefault))]() mutable
		{
			*bExp = !(*bExp);
			// No window resize — RESPONSE & GAIN collapses in place.
			// Only the SIDECHAIN section drives window resize.
		})
		[
			SNew(SVerticalBox)

			// ── Row 1: [Attack | Release] [Look Ahead (bottom)] [Input / Output] ─────
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.FillWidth(1.4f)
				.Padding(0.f)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
					[
						SNew(SAudioFXLabeledKnob)
						.ParamName(LOCTEXT("AttackName", "Attack"))
						.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
							LOCTEXT("AttackLbl", "Attack"),
							AtkMin,
							AtkMax,
							TEXT("ms"),
							LOCTEXT("AttackTip", "The period over which the compressor decreases gain to the level determined by the ratio.")))
						.Style(AttackStyle)
						.AudioUnitsValueType(EAudioUnitsValueType::Linear)
						.ValueFormat(EAudioFXValueFormat::Time)
						.MinValue(AtkMin)
						.MaxValue(AtkMax)
						.Value_Lambda([GetAttack, AtkMin, AtkMax]() -> float
						{
							return MapToNorm(GetAttack.Execute(), AtkMin, AtkMax);
						})
						.OnValueChanged_Lambda([MarkDirty, SetAttack, AtkMin, AtkMax](float NewNorm)
						{
							MarkDirty.ExecuteIfBound();
							SetAttack.ExecuteIfBound(NormToVal(NewNorm, AtkMin, AtkMax));
						})
					]

					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
					[
						SNew(SAudioFXLabeledKnob)
						.ParamName(LOCTEXT("ReleaseName", "Release"))
						.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
							LOCTEXT("ReleaseLbl", "Release"),
							RelMin,
							RelMax,
							TEXT("ms"),
							LOCTEXT("ReleaseTip", "The period over which the compressor increases gain to 0 dB once level has fallen below the threshold.")))
						.Style(ReleaseStyle)
						.AudioUnitsValueType(EAudioUnitsValueType::Linear)
						.ValueFormat(EAudioFXValueFormat::Time)
						.MinValue(RelMin)
						.MaxValue(RelMax)
						.Value_Lambda([GetRelease, RelMin, RelMax]() -> float
						{
							return MapToNorm(GetRelease.Execute(), RelMin, RelMax);
						})
						.OnValueChanged_Lambda([MarkDirty, SetRelease, RelMin, RelMax](float NewNorm)
						{
							MarkDirty.ExecuteIfBound();
							SetRelease.ExecuteIfBound(NormToVal(NewNorm, RelMin, RelMax));
						})
					]
				]

				// ── Row 2: Look Ahead (center-bottom) | Input / Output Gain (stacked right) ──
				+ SHorizontalBox::Slot()
				.FillWidth(0.8f)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Bottom)
				.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
				[
					SNew(SAudioFXLabeledKnob)
					.ParamName(LOCTEXT("LookAheadName", "Look Ahead"))
					.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
						LOCTEXT("LookAheadLbl", "Look Ahead"),
						LaMin,
						LaMax,
						TEXT("ms"),
						LOCTEXT("LookAheadTip", "The amount of time to look ahead of the current audio.\nAudioFactory Note: Higher values add latency but improve transient handling.")))
					.Style(LookAheadStyle)
					.AudioUnitsValueType(EAudioUnitsValueType::Linear)
					.ValueFormat(EAudioFXValueFormat::Time)
					.MinValue(LaMin)
					.MaxValue(LaMax)
					.Value_Lambda([GetLookAhead, LaMin, LaMax]() -> float
					{
						return MapToNorm(GetLookAhead.Execute(), LaMin, LaMax);
					})
					.OnValueChanged_Lambda([MarkDirty, SetLookAhead, LaMin, LaMax](float NewNorm)
					{
						MarkDirty.ExecuteIfBound();
						SetLookAhead.ExecuteIfBound(NormToVal(NewNorm, LaMin, LaMax));
					})
				]

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(0.f)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
					[
						SNew(SAudioFXLabeledKnob)
						.ParamName(LOCTEXT("InputGainName", "Input Gain"))
						.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
							LOCTEXT("InputGainLbl", "Input Gain"),
							IgMin,
							IgMax,
							TEXT("dB"),
							LOCTEXT("InputGainTip", "The input gain of the dynamics processor.")))
						.Style(InGainStyle)
						.AudioUnitsValueType(EAudioUnitsValueType::Volume)
						.ValueFormat(EAudioFXValueFormat::Decibels)
						.MinValue(IgMin)
						.MaxValue(IgMax)
						.Value_Lambda([GetInGain, IgMin, IgMax]() -> float
						{
							return MapToNorm(GetInGain.Execute(), IgMin, IgMax);
						})
						.OnValueChanged_Lambda([MarkDirty, SetInGain, IgMin, IgMax](float NewNorm)
						{
							MarkDirty.ExecuteIfBound();
							SetInGain.ExecuteIfBound(NormToVal(NewNorm, IgMin, IgMax));
						})
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(FAudioFXTheme::KnobHorizontalSpacing, 6.f, FAudioFXTheme::KnobHorizontalSpacing, 0.f)
					[
						SNew(SAudioFXLabeledKnob)
						.ParamName(LOCTEXT("OutputGainName", "Output Gain"))
						.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
							LOCTEXT("OutputGainLbl", "Output Gain"),
							OgMin,
							OgMax,
							TEXT("dB"),
							LOCTEXT("OutputGainTip", "The output gain of the dynamics processor.")))
						.Style(OutGainStyle)
						.AudioUnitsValueType(EAudioUnitsValueType::Volume)
						.ValueFormat(EAudioFXValueFormat::Decibels)
						.MinValue(OgMin)
						.MaxValue(OgMax)
						.Value_Lambda([GetOutGain, OgMin, OgMax]() -> float
						{
							return MapToNorm(GetOutGain.Execute(), OgMin, OgMax);
						})
						.OnValueChanged_Lambda([MarkDirty, SetOutGain, OgMin, OgMax](float NewNorm)
						{
							MarkDirty.ExecuteIfBound();
							SetOutGain.ExecuteIfBound(NormToVal(NewNorm, OgMin, OgMax));
						})
					]
				]
			]

			// ── Row 3: Peak | Link | Analog ─────────────────────
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 6.f, 0.f, 0.f)
			[
				SNew(SHorizontalBox)

				// Peak label + strip
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("PeakL", "Peak"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SAudioFXDropdown)
					.Options(PeakOpts)
					.AssetColor(AssetCol)
					.SelectedIndex(GetPeak.Execute())
					.OnSelectionChanged_Lambda([MarkDirty, SetPeak](int32 OptionIndex)
					{
						MarkDirty.ExecuteIfBound();
						SetPeak.ExecuteIfBound(OptionIndex);
					})
				]

				// Link — either multi-option strip or single bool toggle
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(bUseLinkStrip ? 8.f : 10.f, 0.f, 0.f, 0.f)
				.VAlign(VAlign_Center)
				[
					bUseLinkStrip
					? SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(0.f, 0.f, 4.f, 0.f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("LinkL", "Link"))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
							.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SAudioFXDropdown)
							.Options(LinkOpts)
							.AssetColor(AssetCol)
							.SelectedIndex(GetLink.Execute())
							.OnSelectionChanged_Lambda([MarkDirty, SetLink](int32 OptionIndex)
							{
								MarkDirty.ExecuteIfBound();
								SetLink.ExecuteIfBound(OptionIndex);
							})
						]
					: static_cast<TSharedRef<SWidget>>(
						SNew(SAudioFXButton)
						.Options(TArray<FText>{ LOCTEXT("StereoLink", "Stereo Link") })
						.AssetColor(AssetCol)
						.SelectedIndex(GetStereoLnk.Execute() ? 0 : -1)
						.OnSelectionChanged_Lambda([MarkDirty, SetStereoLnk](int32 OptionIndex)
						{
							MarkDirty.ExecuteIfBound();
							SetStereoLnk.ExecuteIfBound(OptionIndex == 0);
						})
					)
				]

				// Analog toggle
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(10.f, 0.f, 0.f, 0.f)
				[
					SNew(SAudioFXButton)
					.Options(TArray<FText>{ LOCTEXT("Analog", "Analog Mode") })
					.AssetColor(AssetCol)
					.SelectedIndex(GetAnalog.Execute() ? 0 : -1)
					.OnSelectionChanged_Lambda([MarkDirty, SetAnalog](int32 OptionIndex)
					{
						MarkDirty.ExecuteIfBound();
						SetAnalog.ExecuteIfBound(OptionIndex == 0);
					})
				]
			]
		];
}

// ═══════════════════════════════════════════════════════════════════════════
//  Sidechain section (submix only, collapsible)
// ═══════════════════════════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════════════════════════
//  Sidechain helpers — BuildShelfRow (used by BuildSidechainSection)
// ═══════════════════════════════════════════════════════════════════════════

/** Build a single shelf-filter row (On toggle + Cutoff knob + Gain knob). */
static TSharedRef<SWidget> BuildShelfRow(
	const FText& Label,
	const FDynShelfParams& Shelf,
	const FLinearColor& AssetCol,
	FAudioFXEffectEditor& Editor,
	FSimpleDelegate MarkDirty)
{
	const FAudioMaterialKnobStyle* CutStyle  = Editor.StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Frequency, AssetCol, EAudioFXKnobSize::Tertiary));
	const FAudioMaterialKnobStyle* GainStyle = Editor.StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain, AssetCol, EAudioFXKnobSize::Tertiary));

	FDynGetBool  GetEn   = Shelf.GetEnabled;
	FDynSetBool  SetEn   = Shelf.SetEnabled;
	FDynGetFloat GetCut  = Shelf.GetCutoff;
	FDynSetFloat SetCut  = Shelf.SetCutoff;
	FDynGetFloat GetGain = Shelf.GetGain;
	FDynSetFloat SetGain = Shelf.SetGain;

	const FText CutName  = FText::Format(LOCTEXT("ShelfCutFmt", "{0} Cutoff"), Label);
	const FText GainName = FText::Format(LOCTEXT("ShelfGainFmt", "{0} Gain"), Label);

	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.f, 0.f, 6.f, 0.f)
		[
			SNew(STextBlock)
			.Text(Label)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
			.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SAudioFXButton)
			.Options(TArray<FText>{ LOCTEXT("ShelfOn", "On") })
			.AssetColor(AssetCol)
			.SelectedIndex(GetEn.Execute() ? 0 : -1)
			.OnSelectionChanged_Lambda([MarkDirty, SetEn](int32 OptionIndex)
			{
				MarkDirty.ExecuteIfBound();
				SetEn.ExecuteIfBound(OptionIndex == 0);
			})
		]

		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
		[
			SNew(SAudioFXLabeledKnob)
			.ParamName(CutName)
			.Style(CutStyle)
			.AudioUnitsValueType(EAudioUnitsValueType::Frequency)
			.ValueFormat(EAudioFXValueFormat::Frequency)
			.MinValue(20.f)
			.MaxValue(20000.f)
			.Value_Lambda([GetCut]() -> float
			{
				return MapToNorm(GetCut.Execute(), 20.f, 20000.f);
			})
			.OnValueChanged_Lambda([MarkDirty, SetCut](float NewNorm)
			{
				MarkDirty.ExecuteIfBound();
				SetCut.ExecuteIfBound(NormToVal(NewNorm, 20.f, 20000.f));
			})
		]

		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
		[
			SNew(SAudioFXLabeledKnob)
			.ParamName(GainName)
			.Style(GainStyle)
			.AudioUnitsValueType(EAudioUnitsValueType::Volume)
			.ValueFormat(EAudioFXValueFormat::Decibels)
			.MinValue(-60.f)
			.MaxValue(6.f)
			.Value_Lambda([GetGain]() -> float
			{
				return MapToNorm(GetGain.Execute(), -60.f, 6.f);
			})
			.OnValueChanged_Lambda([MarkDirty, SetGain](float NewNorm)
			{
				MarkDirty.ExecuteIfBound();
				SetGain.ExecuteIfBound(NormToVal(NewNorm, -60.f, 6.f));
			})
		];
}

/**
 * Build the collapsible SIDECHAIN section (submix editors only).
 * Includes Key Source dropdown, Key Audition toggle, conditional AudioBus / Submix
 * asset pickers, Key Gain knob, and Highshelf / Lowshelf filter rows.
 */
static TSharedRef<SWidget> BuildSidechainSection(
	const FDynSidechainParams& SC,
	const FLinearColor& AssetCol,
	FAudioFXEffectEditor& Editor,
	FSimpleDelegate MarkDirty)
{
	const FAudioMaterialKnobStyle* KeyGainStyle = Editor.StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain, AssetCol, EAudioFXKnobSize::Tertiary));

	FDynGetInt   GetKeySrc   = SC.GetKeySource;
	FDynSetInt   SetKeySrc   = SC.SetKeySource;
	FDynGetBool  GetAudit    = SC.GetKeyAudition;
	FDynSetBool  SetAudit    = SC.SetKeyAudition;
	FDynGetFloat GetKeyGain  = SC.GetKeyGain;
	FDynSetFloat SetKeyGain  = SC.SetKeyGain;

	TSharedRef<int32> CurrentKeySource = MakeShared<int32>(GetKeySrc.Execute());

	auto GetBusPath    = SC.GetAudioBusPath;
	auto SetBus        = SC.SetAudioBus;
	auto GetSubmixPath = SC.GetSubmixPath;
	auto SetSubmix     = SC.SetSubmix;

	const bool bExpandDefault = Editor.ShouldExpandAllSections();

	return SNew(SAudioFXCollapsibleSection)
		.SectionLabel(LOCTEXT("SCLabel", "SIDECHAIN"))
		.bDefaultExpanded(bExpandDefault)
		.OnToggled_Lambda([&Editor, bExp = TSharedPtr<bool>(MakeShared<bool>(bExpandDefault))]() mutable
		{
			*bExp = !(*bExp);
			Editor.ResizeWindowByDelta((*bExp) ? 300.f : -300.f);
		})
		[
			SNew(SVerticalBox)

			// ── Key Source + Key Audition ────────────────────────
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("KSrcL", "Key Source"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SAudioFXDropdown)
					.Options(SC.KeySourceOptions)
					.AssetColor(AssetCol)
					.SelectedIndex(*CurrentKeySource)
					.OnSelectionChanged_Lambda([MarkDirty, SetKeySrc, CurrentKeySource](int32 OptionIndex)
					{
						MarkDirty.ExecuteIfBound();
						SetKeySrc.ExecuteIfBound(OptionIndex);
						*CurrentKeySource = OptionIndex;
					})
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(10.f, 0.f, 0.f, 0.f)
				[
					SNew(SAudioFXButton)
					.Options(TArray<FText>{ LOCTEXT("KAudit", "Key Audition") })
					.AssetColor(AssetCol)
					.SelectedIndex(GetAudit.Execute() ? 0 : -1)
					.OnSelectionChanged_Lambda([MarkDirty, SetAudit](int32 OptionIndex)
					{
						MarkDirty.ExecuteIfBound();
						SetAudit.ExecuteIfBound(OptionIndex == 0);
					})
				]
			]

			// ── External Audio Bus picker (visible when key == 1 / AudioBus) ──
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(SBox)
				.Visibility_Lambda([CurrentKeySource]() -> EVisibility
				{
					return (*CurrentKeySource == 1) ? EVisibility::Visible : EVisibility::Collapsed;
				})
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.f, 0.f, 6.f, 0.f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("KBusL", "Key Audio Bus"))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
						.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					[
						SNew(SObjectPropertyEntryBox)
						.AllowedClass(UAudioBus::StaticClass())
						.ObjectPath_Lambda([GetBusPath]() -> FString
						{
							return GetBusPath ? GetBusPath() : FString();
						})
						.OnObjectChanged_Lambda([MarkDirty, SetBus](const FAssetData& AssetData)
						{
							MarkDirty.ExecuteIfBound();
							if (SetBus) SetBus(AssetData);
						})
					]
				]
			]

			// ── External Submix picker (visible when key == 2 / Submix) ──
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(SBox)
				.Visibility_Lambda([CurrentKeySource]() -> EVisibility
				{
					return (*CurrentKeySource == 2) ? EVisibility::Visible : EVisibility::Collapsed;
				})
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.f, 0.f, 6.f, 0.f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("KSubmixL", "Key Submix"))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
						.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					[
						SNew(SObjectPropertyEntryBox)
						.AllowedClass(USoundSubmix::StaticClass())
						.ObjectPath_Lambda([GetSubmixPath]() -> FString
						{
							return GetSubmixPath ? GetSubmixPath() : FString();
						})
						.OnObjectChanged_Lambda([MarkDirty, SetSubmix](const FAssetData& AssetData)
						{
							MarkDirty.ExecuteIfBound();
							if (SetSubmix) SetSubmix(AssetData);
						})
					]
				]
			]

			// ── Key Gain knob ───────────────────────────────────
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
				[
					SNew(SAudioFXLabeledKnob)
					.ParamName(LOCTEXT("KeyGainName", "Key Gain"))
					.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
						LOCTEXT("KeyGainLbl", "Key Gain"),
						-60.f,
						30.f,
						TEXT("dB"),
						LOCTEXT("KeyGainTip", "Gain to apply to key signal if key source not set to default.")))
					.Style(KeyGainStyle)
					.AudioUnitsValueType(EAudioUnitsValueType::Volume)
					.ValueFormat(EAudioFXValueFormat::Decibels)
					.MinValue(-60.f)
					.MaxValue(30.f)
					.Value_Lambda([GetKeyGain]() -> float
					{
						return MapToNorm(GetKeyGain.Execute(), -60.f, 30.f);
					})
					.OnValueChanged_Lambda([MarkDirty, SetKeyGain](float NewNorm)
					{
						MarkDirty.ExecuteIfBound();
						SetKeyGain.ExecuteIfBound(NormToVal(NewNorm, -60.f, 30.f));
					})
				]
			]

			// ── Highshelf row ───────────────────────────────────
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 4.f)
			[
				BuildShelfRow(LOCTEXT("HSL", "Highshelf"), SC.Highshelf, AssetCol, Editor, MarkDirty)
			]

			// ── Lowshelf row ────────────────────────────────────
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				BuildShelfRow(LOCTEXT("LSL", "Lowshelf"), SC.Lowshelf, AssetCol, Editor, MarkDirty)
			]
		];
}

// ═══════════════════════════════════════════════════════════════════════════
//  Public: BuildWidget
// ═══════════════════════════════════════════════════════════════════════════

TSharedRef<SWidget> AudioFXDynamicsUI::BuildWidget(
	FAudioFXEffectEditor& Editor,
	const FDynamicsUIParams& Params,
	const FLinearColor& AssetCol)
{
	TWeakPtr<FAudioFXEffectEditor> WeakEditor = Editor.AsShared();
	FSimpleDelegate MarkDirty = FSimpleDelegate::CreateLambda([WeakEditor]{ if (auto Ed = WeakEditor.Pin()) { Ed->MarkPresetDirty(); } });
	TSharedRef<int32> CurrentType = MakeShared<int32>(Params.GetType.Execute());

	// ── Attack/Release knob styles for the sub-viz row ───────────
	const FAudioMaterialKnobStyle* AtkStyle = Editor.StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Time, AssetCol, EAudioFXKnobSize::Secondary));
	const FAudioMaterialKnobStyle* RelStyle = Editor.StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Mix, AssetCol, EAudioFXKnobSize::Secondary));

	FDynGetFloat GetAtk = Params.GetAttack; FDynSetFloat SetAtk = Params.SetAttack;
	FDynGetFloat GetRel = Params.GetRelease; FDynSetFloat SetRel = Params.SetRelease;
	const float AtkMin = Params.AttackMin, AtkMax = Params.AttackMax;
	const float RelMin = Params.ReleaseMin, RelMax = Params.ReleaseMax;

	return SNew(SBox)
		.Padding(FMargin(FAudioFXTheme::WindowPadding, FAudioFXTheme::SectionPadding))
		[
			SNew(SHorizontalBox)

			// ── LEFT: Transfer Curve + Attack/Release below ──────────
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Top)
			.Padding(0.f, 0.f, FAudioFXTheme::SectionGap, 0.f)
			[
				SNew(SVerticalBox)

				// Transfer curve
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					BuildTransferCurvePanel(Params, AssetCol)
				]

				// Attack | Release knobs (below viz)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 8.f, 0.f, 0.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
					.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
					[
						SNew(SAudioFXLabeledKnob)
						.ParamName(LOCTEXT("AtkSubLbl", "Attack"))
						.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(LOCTEXT("AtkSubTipLbl","Attack"), AtkMin, AtkMax, TEXT("ms"),
							LOCTEXT("AtkSubTip","The period over which the compressor decreases gain.")))
						.Style(AtkStyle)
						.AudioUnitsValueType(EAudioUnitsValueType::Linear)
						.ValueFormat(EAudioFXValueFormat::Time)
						.MinValue(AtkMin).MaxValue(AtkMax)
						.Value_Lambda([GetAtk, AtkMin, AtkMax]() -> float { return MapToNorm(GetAtk.Execute(), AtkMin, AtkMax); })
						.OnValueChanged_Lambda([MarkDirty, SetAtk, AtkMin, AtkMax](float NewNorm) { MarkDirty.ExecuteIfBound(); SetAtk.ExecuteIfBound(NormToVal(NewNorm, AtkMin, AtkMax)); })
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
					.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
					[
						SNew(SAudioFXLabeledKnob)
						.ParamName(LOCTEXT("RelSubLbl", "Release"))
						.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(LOCTEXT("RelSubTipLbl","Release"), RelMin, RelMax, TEXT("ms"),
							LOCTEXT("RelSubTip","The period over which the compressor increases gain after signal falls below threshold.")))
						.Style(RelStyle)
						.AudioUnitsValueType(EAudioUnitsValueType::Linear)
						.ValueFormat(EAudioFXValueFormat::Time)
						.MinValue(RelMin).MaxValue(RelMax)
						.Value_Lambda([GetRel, RelMin, RelMax]() -> float { return MapToNorm(GetRel.Execute(), RelMin, RelMax); })
						.OnValueChanged_Lambda([MarkDirty, SetRel, RelMin, RelMax](float NewNorm) { MarkDirty.ExecuteIfBound(); SetRel.ExecuteIfBound(NormToVal(NewNorm, RelMin, RelMax)); })
					]
				]
			]

			// ── Divider ─────────────────────────────────────────
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Fill)
			.Padding(0.f, 6.f)
			[
				SNew(SBox)
				.WidthOverride(1.f)
				[
					SNew(SBorder)
					.BorderImage(&FAudioFXTheme::GetDividerBrush())
				]
			]

			// ── RIGHT: Controls ─────────────────────────────────
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Top)
			.Padding(FAudioFXTheme::SectionGap, 0.f, 0.f, 0.f)
			[
				SNew(SVerticalBox)

				// Type + Bypass row
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					BuildTypeRow(Params, AssetCol, MarkDirty, CurrentType)
				]

				// Threshold | Ratio | Knee row
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					BuildDynamicsRow(Params, AssetCol, Editor, MarkDirty, CurrentType)
				]

				// RESPONSE & GAIN (collapsible)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					BuildResponseSection(Params, AssetCol, Editor, MarkDirty)
				]

				// SIDECHAIN (submix only, collapsible)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					Params.Sidechain.bEnabled
						? BuildSidechainSection(Params.Sidechain, AssetCol, Editor, MarkDirty)
						: SNullWidget::NullWidget
				]
			]
		];
}

#undef LOCTEXT_NAMESPACE
