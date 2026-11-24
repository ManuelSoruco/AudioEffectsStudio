// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/UIBuilders/AudioFXEQUI.h"

#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Widgets/SAudioFXButton.h"
#include "Shared/Visualizers/SAudioFXEQCurve.h"
#include "Shared/Widgets/SAudioFXLabeledKnob.h"

#include "AudioMaterialSlate/AudioMaterialSlateTypes.h"
#include "AudioWidgetsEnums.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AudioFXEQUI"

static constexpr int32 GNumBandColors = FAudioFXTheme::NumBandPaletteColors;
static constexpr float GBandCardWidth = 126.f;
static constexpr int32 GEmptyBandPlusFontSize = 36;

// ═══════════════════════════════════════════════════════════════════════════
//  FEQUIState — BuildBandCard
//  UI Helpers
// ═══════════════════════════════════════════════════════════════════════════

TSharedRef<SWidget> FEQUIState::BuildBandCard(int32 BandIndex)
{
	if (BandIndex >= Params.GetBandCount.Execute())
		return SNullWidget::NullWidget;

	const FLinearColor BandCol = BandPalette[BandIndex % GNumBandColors];
	const FLinearColor StripeCol = FMath::Lerp(BandCol, FLinearColor(0.5f, 0.5f, 0.5f, 1.f), 0.15f);

	const FAudioMaterialKnobStyle* FreqStyle = Editor->StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Frequency, BandCol, EAudioFXKnobSize::Secondary));
	const FAudioMaterialKnobStyle* BWStyle   = Editor->StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Resonance, BandCol, EAudioFXKnobSize::Tertiary));
	const FAudioMaterialKnobStyle* GainStyle = Editor->StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain, BandCol, EAudioFXKnobSize::Secondary));

	// Capture delegates + MarkDirty by value
	FEQGetBandFloat  GetFrequency = Params.GetFrequency;
	FEQSetBandFloat  SetFrequency = Params.SetFrequency;
	FEQGetBandFloat  GetBandwidth   = Params.GetBandwidth;
	FEQSetBandFloat  SetBandwidth   = Params.SetBandwidth;
	FEQGetBandFloat  GetGain = Params.GetGainDb;
	FEQSetBandFloat  SetGain = Params.SetGainDb;
	FEQGetBandBool   GetEnabled   = Params.GetEnabled;
	FEQSetBandBool   SetEnabled   = Params.SetEnabled;
	FEQRemoveBand    RemoveBand  = Params.RemoveBand;
	FSimpleDelegate  MarkDirtyDelegate      = MarkDirty;
	TWeakPtr<FEQUIState> WeakSelf = AsShared();

	// RemoveBand button (hidden by default)
	TSharedPtr<SBorder> RemoveBorder;
	TSharedRef<SWidget> RemoveBtn = SAssignNew(RemoveBorder, SBorder)
		.BorderImage(&FAudioFXTheme::GetRemoveButtonBrush())
		.Padding(FMargin(0.f))
		.Visibility(EVisibility::Hidden)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.ContentPadding(FMargin(7.f, 3.f))
			.OnClicked_Lambda([MarkDirtyDelegate, RemoveBand, BandIndex, WeakSelf]() -> FReply
			{
				MarkDirtyDelegate.ExecuteIfBound();
				RemoveBand.ExecuteIfBound(BandIndex);
				if (TSharedPtr<FEQUIState> Self = WeakSelf.Pin())
					Self->RebuildRack();
				return FReply::Handled();
			})
			.OnHovered_Lambda([RemoveBorder]()
			{
				if (RemoveBorder.IsValid())
					RemoveBorder->SetBorderImage(&FAudioFXTheme::GetRemoveButtonHoverBrush());
			})
			.OnUnhovered_Lambda([RemoveBorder]()
			{
				if (RemoveBorder.IsValid())
					RemoveBorder->SetBorderImage(&FAudioFXTheme::GetRemoveButtonBrush());
			})
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("\u2715")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FLinearColor(0.8f, 0.35f, 0.35f, 1.f))
			]
		];

	// Color stripe brush
	TSharedPtr<FSlateRoundedBoxBrush> StripeBrush = MakeShared<FSlateRoundedBoxBrush>(
		StripeCol, FVector4(FAudioFXTheme::CardRadius, FAudioFXTheme::CardRadius, 0.f, 0.f));
	const FSlateBrush* StripeBrushPtr = StripeBrush.Get();
	BandStripeBrushes.Add(StripeBrush);

	return SNew(SBox)
		.WidthOverride(GBandCardWidth)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.ContentPadding(FMargin(0.f))
			.OnClicked_Lambda([]() -> FReply { return FReply::Unhandled(); })
			.OnHovered_Lambda([RemoveBorder]()
			{
				if (RemoveBorder.IsValid())
					RemoveBorder->SetVisibility(EVisibility::Visible);
			})
			.OnUnhovered_Lambda([RemoveBorder]()
			{
				if (RemoveBorder.IsValid())
					RemoveBorder->SetVisibility(EVisibility::Hidden);
			})
			[
				SNew(SBorder)
				.BorderImage(&FAudioFXTheme::GetCardBrush())
				.Padding(0.f)
				[
					SNew(SVerticalBox)

					// Color stripe
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SBox)
						.HeightOverride(4.f)
						[
							SNew(SBorder)
							.BorderImage(StripeBrushPtr)
						]
					]

					// Inner shadow
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SBox)
						.HeightOverride(1.f)
						[
							SNew(SBorder)
							.BorderImage(&FAudioFXTheme::GetInnerShadowBrush())
						]
					]

					// Label + bypass
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Fill)
					.Padding(6, 4, 6, 2)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.f)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::Format(LOCTEXT("BandFmt", "Band {0}"), FText::AsNumber(BandIndex + 1)))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
							.ColorAndOpacity(FSlateColor(BandCol))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SAudioFXButton)
							.Options(TArray<FText>{ LOCTEXT("Byp", "Bypass") })
							.AssetColor(BandCol)
							.SelectedIndex(GetEnabled.Execute(BandIndex) ? -1 : 0)
							.OnSelectionChanged_Lambda([MarkDirtyDelegate, SetEnabled, BandIndex](int32 OptionIndex)
							{
								MarkDirtyDelegate.ExecuteIfBound();
								SetEnabled.ExecuteIfBound(BandIndex, OptionIndex != 0);
							})
						]
					]

					// Freq knob
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.Padding(6, 0)
					[
						SNew(SAudioFXLabeledKnob)
						.ParamName(LOCTEXT("FreqName", "Freq"))
						.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
							LOCTEXT("FreqLbl", "Frequency"),
							20.f,
							15000.f,
							TEXT("Hz"),
							LOCTEXT("FreqTip", "The center frequency of the EQ band.\nAudioFactory Note: Uses logarithmic mapping across the audible range.")))
						.Style(FreqStyle)
						.AudioUnitsValueType(EAudioUnitsValueType::Frequency)
						.ValueFormat(EAudioFXValueFormat::Frequency)
						.MinValue(20.f)
						.MaxValue(15000.f)
						.Value_Lambda([GetFrequency, BandIndex]() -> float
						{
							return FAudioFXTheme::LogFreqToNorm(
								FMath::Clamp(GetFrequency.Execute(BandIndex), 20.f, 15000.f),
								20.f, 15000.f);
						})
						.OnValueChanged_Lambda([MarkDirtyDelegate, SetFrequency, BandIndex](float NewNorm)
						{
							MarkDirtyDelegate.ExecuteIfBound();
							SetFrequency.ExecuteIfBound(BandIndex, FAudioFXTheme::NormToLogFreq(NewNorm, 20.f, 15000.f));
						})
					]

					// BW knob
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.Padding(6, 4)
					[
						SNew(SAudioFXLabeledKnob)
						.ParamName(LOCTEXT("BWName", "BW"))
						.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
							LOCTEXT("BWLbl", "Bandwidth"),
							0.1f,
							2.f,
							TEXT("oct"),
							LOCTEXT("BWTip", "The bandwidth (in octaves) of the EQ band.\nAudioFactory Note: Can be set alternatively via Q/resonance in the DSP layer.")))
						.Style(BWStyle)
						.AudioUnitsValueType(EAudioUnitsValueType::Linear)
						.ValueFormat(EAudioFXValueFormat::QFactor)
						.MinValue(0.1f)
						.MaxValue(2.f)
						.Value_Lambda([GetBandwidth, BandIndex]() -> float
						{
							return FMath::GetMappedRangeValueClamped(
								FVector2D(0.1f, 2.f),
								FVector2D(0.f, 1.f),
								GetBandwidth.Execute(BandIndex));
						})
						.OnValueChanged_Lambda([MarkDirtyDelegate, SetBandwidth, BandIndex](float NewNorm)
						{
							MarkDirtyDelegate.ExecuteIfBound();
							SetBandwidth.ExecuteIfBound(BandIndex, FMath::GetMappedRangeValueClamped(
								FVector2D(0.f, 1.f),
								FVector2D(0.1f, 2.f), NewNorm));
						})
					]

					// Gain knob
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.Padding(6, 4)
					[
						SNew(SAudioFXLabeledKnob)
						.ParamName(LOCTEXT("GainName", "Gain"))
						.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
							LOCTEXT("GainLbl", "Gain"),
							-90.f,
							20.f,
							TEXT("dB"),
							LOCTEXT("GainTip", "The gain in decibels to apply to the EQ band.")))
						.Style(GainStyle)
						.AudioUnitsValueType(EAudioUnitsValueType::Volume)
						.ValueFormat(EAudioFXValueFormat::Decibels)
						.MinValue(-90.f)
						.MaxValue(20.f)
						.Value_Lambda([GetGain, BandIndex]() -> float
						{
							return FMath::GetMappedRangeValueClamped(
								FVector2D(-90.f, 20.f),
								FVector2D(0.f, 1.f),
								GetGain.Execute(BandIndex));
						})
						.OnValueChanged_Lambda([MarkDirtyDelegate, SetGain, BandIndex](float NewNorm)
						{
							MarkDirtyDelegate.ExecuteIfBound();
							SetGain.ExecuteIfBound(BandIndex, FMath::GetMappedRangeValueClamped(
								FVector2D(0.f, 1.f),
								FVector2D(-90.f, 20.f), NewNorm));
						})
					]

					// RemoveBand button row
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Right)
					.Padding(6, 4, 6, 6)
					[
						RemoveBtn
					]

					// Bottom shadow
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SBox)
						.HeightOverride(2.f)
						[
							SNew(SBorder)
							.BorderImage(&FAudioFXTheme::GetCardShadowBrush())
						]
					]
				]
			]
		];
}

// ═══════════════════════════════════════════════════════════════════════════
//  FEQUIState — BuildAddBandButton
// ═══════════════════════════════════════════════════════════════════════════

TSharedRef<SWidget> FEQUIState::BuildAddBandButton()
{
	FEQAddBand Add = Params.AddBand;
	FSimpleDelegate MarkDirtyDelegate = MarkDirty;
	TWeakPtr<FEQUIState> WeakSelf = AsShared();

	TSharedPtr<SBorder> AddBorder;
	return SNew(SBox)
		.WidthOverride(GBandCardWidth)
		.VAlign(VAlign_Fill)
		[
			SAssignNew(AddBorder, SBorder)
			.BorderImage(&FAudioFXTheme::GetCardBrush())
			.Padding(FMargin(0.f))
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.ContentPadding(FMargin(0.f))
				.ToolTipText(LOCTEXT("AddBandTip", "Add a new EQ band"))
				.OnClicked_Lambda([MarkDirtyDelegate, Add, WeakSelf]() -> FReply
				{
					MarkDirtyDelegate.ExecuteIfBound();
					Add.ExecuteIfBound();
					if (TSharedPtr<FEQUIState> Self = WeakSelf.Pin())
						Self->RebuildRack();
					return FReply::Handled();
				})
				.OnHovered_Lambda([AddBorder]()
				{
					if (AddBorder.IsValid())
						AddBorder->SetBorderImage(&FAudioFXTheme::GetAddButtonBrush());
				})
				.OnUnhovered_Lambda([AddBorder]()
				{
					if (AddBorder.IsValid())
						AddBorder->SetBorderImage(&FAudioFXTheme::GetCardBrush());
				})
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("+")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", GEmptyBandPlusFontSize))
					.ColorAndOpacity(FAudioFXTheme::GetAccentSecondary())
				]
			]
		];
}

// ═══════════════════════════════════════════════════════════════════════════
//  FEQUIState — BuildEmptyState
//  Full-width centered prompt shown when no bands exist.
// ═══════════════════════════════════════════════════════════════════════════

TSharedRef<SWidget> FEQUIState::BuildEmptyState()
{
	FEQAddBand Add = Params.AddBand;
	FSimpleDelegate MarkDirtyDelegate = MarkDirty;
	TWeakPtr<FEQUIState> WeakSelf = AsShared();

	TSharedPtr<SBorder> AddBorder;
	return SAssignNew(AddBorder, SBorder)
		.BorderImage(&FAudioFXTheme::GetCardBrush())
		.Padding(FMargin(40.f, 20.f))
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.ContentPadding(FMargin(0.f))
			.ToolTipText(LOCTEXT("AddFirstBandTip", "Add your first EQ band"))
			.OnClicked_Lambda([MarkDirtyDelegate, Add, WeakSelf]() -> FReply
			{
				MarkDirtyDelegate.ExecuteIfBound();
				Add.ExecuteIfBound();
				if (TSharedPtr<FEQUIState> Self = WeakSelf.Pin())
					Self->RebuildRack();
				return FReply::Handled();
			})
			.OnHovered_Lambda([AddBorder]()
			{
				if (AddBorder.IsValid())
					AddBorder->SetBorderImage(&FAudioFXTheme::GetAddButtonBrush());
			})
			.OnUnhovered_Lambda([AddBorder]()
			{
				if (AddBorder.IsValid())
					AddBorder->SetBorderImage(&FAudioFXTheme::GetCardBrush());
			})
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.f, 0.f, 0.f, 4.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("+")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", GEmptyBandPlusFontSize))
					.ColorAndOpacity(FAudioFXTheme::GetAccentSecondary())
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("AddFirstBandLabel", "Add your first EQ band"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
					.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
				]
			]
		];
}

// ═══════════════════════════════════════════════════════════════════════════
//  FEQUIState — RebuildRack
// ═══════════════════════════════════════════════════════════════════════════

void FEQUIState::RebuildRack()
{
	if (!BandRack.IsValid())
		return;

	BandRack->ClearChildren();
	BandStripeBrushes.Empty();

	const int32 Count = Params.GetBandCount.Execute();

	if (Count == 0)
	{
		// Empty state: full-width centered add button with prompt
		BandRack->AddSlot()
			.FillWidth(1.f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				BuildEmptyState()
			];
		return;
	}

	for (int32 BandIndex = 0; BandIndex < Count; ++BandIndex)
	{
		BandRack->AddSlot()
			.AutoWidth()
			.Padding(4.f, 0.f)
			[
				BuildBandCard(BandIndex)
			];
	}

	BandRack->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Fill)
		.Padding(8.f, 0.f)
		[
			BuildAddBandButton()
		];
}

// ═══════════════════════════════════════════════════════════════════════════
//  Public: BuildWidget  (Construct)
// ═══════════════════════════════════════════════════════════════════════════

TSharedRef<FEQUIState> AudioFXEQUI::BuildWidget(
	FAudioFXEffectEditor& Editor,
	const FEQUIParams& Params,
	const FLinearColor& AssetCol,
	TSharedRef<SWidget>& OutWidget)
{
	TSharedRef<FEQUIState> State = MakeShared<FEQUIState>();
	State->Editor   = &Editor;
	State->Params   = Params;
	State->AssetCol = AssetCol;
	TWeakPtr<FAudioFXEffectEditor> WeakEQEditor = Editor.AsShared();
	State->MarkDirty = FSimpleDelegate::CreateLambda([WeakEQEditor]{ if (auto Ed = WeakEQEditor.Pin()) { Ed->MarkPresetDirty(); } });

	// Build band palette
	const FLinearColor* EQBandColors = FAudioFXTheme::GetBandPalette();
	State->BandPalette.SetNum(GNumBandColors);
	for (int32 PaletteIndex = 0; PaletteIndex < GNumBandColors; ++PaletteIndex)
		State->BandPalette[PaletteIndex] = FMath::Lerp(EQBandColors[PaletteIndex], AssetCol, 0.15f);

	// Build rack
	TSharedRef<SHorizontalBox> Rack = SNew(SHorizontalBox);
	State->BandRack = Rack;

	// Use RebuildRack for initial population (includes empty state logic)
	State->RebuildRack();

	TSharedRef<SScrollBox> Scroll = SNew(SScrollBox).Orientation(Orient_Horizontal);
	Scroll->AddSlot() [ Rack ];

	// Capture for curve
	FEQGetSourceSettings GetSettings = Params.GetSourceSettings;
	const TArray<FLinearColor> PaletteCopy = State->BandPalette;

	OutWidget = SNew(SBox)
		.Padding(FMargin(FAudioFXTheme::WindowPadding, FAudioFXTheme::SectionPadding))
		[
			SNew(SVerticalBox)

			// ── Frequency Response Curve ─────────────────────────
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				.MinDesiredHeight(220.f)
				[
					SNew(SOverlay)

					+ SOverlay::Slot()
					[
						SNew(SAudioFXEQCurve)
						.AssetColor(AssetCol)
						.Settings_Lambda([GetSettings]() -> FSourceEffectEQSettings
						{
							return GetSettings.Execute();
						})
						.BandColors_Lambda([PaletteCopy]() -> TArray<FLinearColor>
						{
							return PaletteCopy;
						})
					]

					+ SOverlay::Slot()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Bottom)
					.Padding(6.f, 4.f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("HzLabel", "Hz"))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						.ColorAndOpacity(FAudioFXTheme::GetTextDisabledColor())
					]

					+ SOverlay::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Top)
					.Padding(6.f, 4.f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("DbLabel", "dB"))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						.ColorAndOpacity(FAudioFXTheme::GetTextDisabledColor())
					]
				]
			]

			// ── Band Rack ────────────────────────────────────────
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			.Padding(0.f, 4.f, 0.f, 0.f)
			[
				Scroll
			]
		];

	return State;
}

#undef LOCTEXT_NAMESPACE
