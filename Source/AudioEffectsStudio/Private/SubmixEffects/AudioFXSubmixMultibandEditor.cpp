// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

/**
 * FAudioFXSubmixMultibandEditor — implementation
 *
 * Multiband Compressor editor for USubmixEffectMultibandCompressorPreset.
 * Layout (top-to-bottom):
 *   1. GLOBAL bar (2-column)
 *        LEFT:  Type / Peak / Link dropdowns stacked vertically
 *        RIGHT: LookAhead knob + Analog / 4-Pole toggles stacked vertically
 *        (Compact height — knob no longer inflates the entire bar)
 *   2. SAudioFXMultibandCurve — composite frequency-response + transfer curve (min 80px)
 *   3. Band rack (scrollable) — one card per band, crossover knob between cards, trailing "+" button
 *
 * Each band card contains:
 *   · Color stripe + label + hover-to-show Remove button
 *   · Threshold (Hero)
 *   · Ratio + Knee side-by-side
 *   · RESPONSE collapsible: Attack / Release / In Gain / Out Gain
 *   · SAudioFXGainReductionMeter inside the card above RESPONSE
 *
 * Minimum 2 bands enforced: opening a preset with fewer shows a blocking
 * dialog (EnforceMinimumBands) offering to create 2 default bands or open
 * the native asset editor instead.
 *
 * Key functions:
 *   EnforceMinimumBands()   — validates band count and shows a dialog if needed
 *   ClampCrossoverHz()      — monotonic ordering constraint for crossover knobs
 *   BuildBandCard()         — constructs one band card widget
 *   BuildCrossoverControl() — constructs the knob between two adjacent bands
 *   BuildAddBandButton()    — the trailing "+" button
 *   RebuildRack()           — rebuilds the band rack + crossover bar after add/remove
 */

// Batch F — ground-up multiband redesign.
// Layout: GlobalBar → Curve → CrossoverBar → BandRack
// Min 2 bands enforced. Crossovers decoupled from band cards.
#include "SubmixEffects/AudioFXSubmixMultibandEditor.h"

#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Widgets/SAudioFXDropdown.h"
#include "Shared/Widgets/SAudioFXButton.h"
#include "Shared/Widgets/SAudioFXCollapsibleSection.h"
#include "Shared/Visualizers/SAudioFXGainReductionMeter.h"
#include "Shared/Widgets/SAudioFXLabeledKnob.h"
#include "Shared/Visualizers/SAudioFXMultibandCurve.h"
#include "Shared/Widgets/SAudioFXSectionBox.h"

#include "AudioWidgetsEnums.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AudioFXSubmixMultiband"

static constexpr int32    GNumMBColors       = FAudioFXTheme::NumBandPaletteColors;
static constexpr float    GCardWidth         = 160.f;   // 20px narrower than before
static constexpr float    GCrossoverKnobW    = 50.f;    // tighter crossover gap

// ─────────────────────────────────────────────────────────────────────────────
//  Bypass passthrough — forwards to preset settings so the PresetBar button works
// ─────────────────────────────────────────────────────────────────────────────

bool FAudioFXSubmixMultibandEditor::GetBypassState() const
{
	if (const USubmixEffectMultibandCompressorPreset* Preset = WeakPreset.Get())
	{
		return Preset->Settings.bBypass;
	}
	return false;
}

void FAudioFXSubmixMultibandEditor::SetBypassState(bool bBypassed)
{
	if (USubmixEffectMultibandCompressorPreset* Preset = WeakPreset.Get())
	{
		Preset->Settings.bBypass = bBypassed;
		Preset->SetSettings(Preset->Settings);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  ClampCrossoverHz — enforces monotonic ordering between crossover frequencies
//
//  CrossIdx i maps to Bands[i].CrossoverTopFrequency.
//  Valid range: (Bands[i-1].CrossoverTopFrequency + Gap) .. (Bands[i+1].CrossoverTopFrequency - Gap)
//  Edge cases: lower bound defaults to 20 Hz + Gap, upper bound to 20 kHz - Gap.
// ─────────────────────────────────────────────────────────────────────────────
float FAudioFXSubmixMultibandEditor::ClampCrossoverHz(
	const TArray<FDynamicsBandSettings>& Bands, int32 CrossIdx, float DesiredHz)
{
	const float LowerBound = (CrossIdx == 0)
		? 20.f + MinCrossoverGapHz
		: Bands[CrossIdx - 1].CrossoverTopFrequency + MinCrossoverGapHz;

	// CrossIdx is always < NumBands-1 (last band's crossover is never shown).
	const bool bHasNextCrossover = (CrossIdx + 1) < (Bands.Num() - 1);
	const float UpperBound = bHasNextCrossover
		? Bands[CrossIdx + 1].CrossoverTopFrequency - MinCrossoverGapHz
		: 20000.f - MinCrossoverGapHz;

	return FMath::Clamp(DesiredHz, FMath::Max(LowerBound, 20.f), FMath::Min(UpperBound, 20000.f));
}

// ─────────────────────────────────────────────────────────────────────────────
//  EnforceMinimumBands
//
//  Called before CreateEffectWidget to guard against degenerate presets.
//  Shows a modal dialog explaining the 2-band minimum and offering two actions:
//    · "Create Default Bands" — inserts 2 default bands and saves
//    · "Open Asset Editor"    — opens the native preset details panel
//  Returns false if the preset is invalid or the user chose the native editor.
// ─────────────────────────────────────────────────────────────────────────────

bool FAudioFXSubmixMultibandEditor::EnforceMinimumBands(USubmixEffectMultibandCompressorPreset* Preset)
{
	if (!Preset || Preset->Settings.Bands.Num() >= 2)
	{
		return true;   // Enough bands — proceed normally
	}

	// ── Headless / capture mode: skip the dialog, auto-create bands ──
	if (bSuppressMinBandsDialog)
	{
		MarkDirty.ExecuteIfBound();
		while (Preset->Settings.Bands.Num() < 2)
		{
			Preset->Settings.Bands.AddDefaulted();
		}
		Preset->Settings.Bands[0].CrossoverTopFrequency = 1000.f;
		Preset->Settings.Bands[0].ThresholdDb           = -18.f;
		Preset->Settings.Bands[0].Ratio                 = 4.f;
		Preset->Settings.Bands[0].AttackTimeMsec        = 10.f;
		Preset->Settings.Bands[0].ReleaseTimeMsec       = 100.f;
		Preset->Settings.Bands[1].CrossoverTopFrequency = 20000.f;
		Preset->Settings.Bands[1].ThresholdDb           = -18.f;
		Preset->Settings.Bands[1].Ratio                 = 4.f;
		Preset->Settings.Bands[1].AttackTimeMsec        = 10.f;
		Preset->Settings.Bands[1].ReleaseTimeMsec       = 100.f;
		Preset->SetSettings(Preset->Settings);
		return true;
	}

	// ── Show a blocking dialog ───────────────────────────────────────
	bool bUserChoseCreate = false;

	TSharedRef<SWindow> Dialog = SNew(SWindow)
		.Title(LOCTEXT("MinBandsTitle", "Multiband Compressor — Minimum Bands Required"))
		.SizingRule(ESizingRule::Autosized)
		.SupportsMinimize(false)
		.SupportsMaximize(false)
		.IsTopmostWindow(true);

	TSharedRef<SVerticalBox> Content = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(24.f, 20.f, 24.f, 8.f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("MinBandsMsg",
				"The Audio Effects Studio multiband UI requires at least 2 bands.\n"
				"This preset currently has fewer than 2 bands.\n\n"
				"Create 2 default bands now?"))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
			.AutoWrapText(true)
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(24.f, 8.f, 24.f, 20.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("MinBandsYes", "Yes, create bands"))
				.OnClicked_Lambda([&bUserChoseCreate, &Dialog]() -> FReply
				{
					bUserChoseCreate = true;
					Dialog->RequestDestroyWindow();
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("MinBandsNo", "No, open default editor"))
				.OnClicked_Lambda([&Dialog]() -> FReply
				{
					Dialog->RequestDestroyWindow();
					return FReply::Handled();
				})
			]
		];

	Dialog->SetContent(Content);

	if (TSharedPtr<SWindow> ParentWindow = GetWindow())
	{
		FSlateApplication::Get().AddModalWindow(Dialog, ParentWindow);
	}
	else
	{
		FSlateApplication::Get().AddModalWindow(Dialog, FSlateApplication::Get().GetActiveTopLevelWindow());
	}

	if (!bUserChoseCreate)
	{
		// User said "No" — open the default asset editor
		if (GEditor)
		{
			GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Preset);
		}
		// Close our window
		if (TSharedPtr<SWindow> OurWindow = GetWindow())
		{
			OurWindow->RequestDestroyWindow();
		}
		return false;
	}

	// ── Create 2 default bands ───────────────────────────────────────
	MarkDirty.ExecuteIfBound();
	while (Preset->Settings.Bands.Num() < 2)
	{
		Preset->Settings.Bands.AddDefaulted();
	}
	// Band 0: up to 1kHz
	Preset->Settings.Bands[0].CrossoverTopFrequency = 1000.f;
	Preset->Settings.Bands[0].ThresholdDb           = -18.f;
	Preset->Settings.Bands[0].Ratio                 = 4.f;
	Preset->Settings.Bands[0].AttackTimeMsec        = 10.f;
	Preset->Settings.Bands[0].ReleaseTimeMsec        = 100.f;
	// Band 1: 1kHz to 20kHz (CrossoverTopFrequency ignored for last band)
	Preset->Settings.Bands[1].CrossoverTopFrequency = 20000.f;
	Preset->Settings.Bands[1].ThresholdDb           = -18.f;
	Preset->Settings.Bands[1].Ratio                 = 4.f;
	Preset->Settings.Bands[1].AttackTimeMsec        = 10.f;
	Preset->Settings.Bands[1].ReleaseTimeMsec        = 100.f;

	Preset->SetSettings(Preset->Settings);
	return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// ─────────────────────────────────────────────────────────────────────────────
//  CreateEffectWidget — top-level composition
//
//  Validates preset, caches state, then assembles:
//    1. GLOBAL bar: processor-type, peak-mode, link-mode, lookahead knob
//    2. SAudioFXMultibandCurve (frequency + transfer curve, min 80px)
//    3. Scrollable band rack (BuildBandCard + BuildCrossoverControl inline)
// ─────────────────────────────────────────────────────────────────────────────
// ═══════════════════════════════════════════════════════════════════════════

TSharedRef<SWidget> FAudioFXSubmixMultibandEditor::CreateEffectWidget(USoundEffectPreset* InPreset)
{
	USubmixEffectMultibandCompressorPreset* Preset = Cast<USubmixEffectMultibandCompressorPreset>(InPreset);
	if (!Preset)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("AudioEffectsStudio: Cast to preset type failed for '%s'. Check asset registration."),
			InPreset ? *InPreset->GetClass()->GetName() : TEXT("null"));
		return FAudioFXEffectEditor::CreateEffectWidget(InPreset);
	}

	WeakPreset     = Preset;
	CachedAssetCol = GetAssetColor();
	TWeakObjectPtr<USubmixEffectMultibandCompressorPreset> LocalWeakPreset = Preset;

	// ── Min-band enforcement (MULTIBAND-02) ─────────────────────────
	if (!EnforceMinimumBands(Preset))
	{
		// Returned a placeholder — the window will close itself
		return SNew(SBox);
	}

	// ── Global knob styles ───────────────────────────────────────────
	const FAudioMaterialKnobStyle* LookAheadStyle = StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Time, CachedAssetCol, EAudioFXKnobSize::Tertiary));

	// ── Enum options ─────────────────────────────────────────────────
	TArray<FText> TypeOpts = {
		LOCTEXT("Comp", "Compressor"), LOCTEXT("Lim", "Limiter"),
		LOCTEXT("Exp", "Expander"),    LOCTEXT("Gate", "Gate"),
		LOCTEXT("UpComp", "Upward Compressor")
	};
	TArray<FText> PeakOpts = {
		LOCTEXT("MS", "MS"), LOCTEXT("RMS", "RMS"), LOCTEXT("Peak", "Peak")
	};
	TArray<FText> LinkOpts = {
		LOCTEXT("Off", "Off"), LOCTEXT("Avg", "Avg"), LOCTEXT("PeakL", "Peak")
	};

	// ── Band rack ────────────────────────────────────────────────────
	TSharedRef<SScrollBox>     Scroll     = SNew(SScrollBox).Orientation(Orient_Horizontal);
	TSharedRef<SHorizontalBox> Rack       = SNew(SHorizontalBox);
	BandRack = Rack;

	const FLinearColor CapturedAssetColor = CachedAssetCol;

	{
		const FLinearColor* BandPalette = FAudioFXTheme::GetBandPalette();
		for (int32 BandIndex = 0; BandIndex < Preset->Settings.Bands.Num(); ++BandIndex)
		{
			const FLinearColor BandCol = BandPalette[BandIndex % GNumMBColors];
			Rack->AddSlot().AutoWidth().VAlign(VAlign_Top).Padding(4.f, 0.f)
			[ BuildBandCard(BandIndex, Preset, CachedAssetCol, BandCol, GetPreviewEngine()) ];
			// Add crossover knob after every band except the last
			if (BandIndex < Preset->Settings.Bands.Num() - 1)
			{
				Rack->AddSlot().AutoWidth().VAlign(VAlign_Top).Padding(0.f, 0.f)
				[ BuildCrossoverControl(BandIndex, Preset) ];
			}
		}
	}
	Rack->AddSlot().AutoWidth().VAlign(VAlign_Top).Padding(8.f, 0.f)
	[ BuildAddBandButton() ];
	Scroll->AddSlot().HAlign(HAlign_Center)[ Rack ];

	// ── Layout ───────────────────────────────────────────────────────
	return SNew(SVerticalBox)

		// ── GLOBAL BAR ───────────────────────────────────────────────
		// 3 equal columns: [Dropdowns] | [Analog + 4-Pole] | [LookAhead]
		// Each FillWidth(1) → each column = 1/3 of bar width, evenly spread.
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SAudioFXSectionBox)
			.SectionLabel(LOCTEXT("GlobalSec", "GLOBAL"))
			[
				SNew(SHorizontalBox)

				// ── COL 1: Type / Peak / Link dropdowns ──────────────────
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).HAlign(HAlign_Left).Padding(0.f, 0.f, 8.f, 0.f)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)
						[
							SNew(STextBlock).Text(LOCTEXT("TypeL", "Type:"))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
							.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(SAudioFXDropdown)
							.Options(TypeOpts).AssetColor(CachedAssetCol)
							.SelectedIndex((int32)Preset->Settings.DynamicsProcessorType)
							.OnSelectionChanged_Lambda([this, LocalWeakPreset](int32 OptionIndex)
							{
								if (USubmixEffectMultibandCompressorPreset* Preset = LocalWeakPreset.Get())
								{
									MarkDirty.ExecuteIfBound();
									Preset->Settings.DynamicsProcessorType = (ESubmixEffectDynamicsProcessorType)OptionIndex;
									Preset->SetSettings(Preset->Settings);
								}
							})
						]
					]

					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)
						[
							SNew(STextBlock).Text(LOCTEXT("PeakL", "Peak:"))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
							.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(SAudioFXDropdown)
							.Options(PeakOpts).AssetColor(CachedAssetCol)
							.SelectedIndex((int32)Preset->Settings.PeakMode)
							.OnSelectionChanged_Lambda([this, LocalWeakPreset](int32 OptionIndex)
							{
								if (USubmixEffectMultibandCompressorPreset* Preset = LocalWeakPreset.Get())
								{
									MarkDirty.ExecuteIfBound();
									Preset->Settings.PeakMode = (ESubmixEffectDynamicsPeakMode)OptionIndex;
									Preset->SetSettings(Preset->Settings);
								}
							})
						]
					]

					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)
						[
							SNew(STextBlock).Text(LOCTEXT("LinkL", "Link:"))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
							.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(SAudioFXDropdown)
							.Options(LinkOpts).AssetColor(CachedAssetCol)
							.SelectedIndex((int32)Preset->Settings.LinkMode)
							.OnSelectionChanged_Lambda([this, LocalWeakPreset](int32 OptionIndex)
							{
								if (USubmixEffectMultibandCompressorPreset* Preset = LocalWeakPreset.Get())
								{
									MarkDirty.ExecuteIfBound();
									Preset->Settings.LinkMode = (ESubmixEffectDynamicsChannelLinkMode)OptionIndex;
									Preset->SetSettings(Preset->Settings);
								}
							})
						]
					]
				]

				// ── COL 2: Analog + 4-Pole toggles centered ──────────────
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).HAlign(HAlign_Center)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 3.f)
					[
						SNew(SAudioFXButton)
						.Options(TArray<FText>{ LOCTEXT("Analog", "Analog") })
						.AssetColor(CachedAssetCol)
						.SelectedIndex(Preset->Settings.bAnalogMode ? 0 : -1)
						.OnSelectionChanged_Lambda([this, LocalWeakPreset](int32 OptionIndex)
						{
							if (USubmixEffectMultibandCompressorPreset* Preset = LocalWeakPreset.Get())
							{
								MarkDirty.ExecuteIfBound();
								Preset->Settings.bAnalogMode = (OptionIndex == 0);
								Preset->SetSettings(Preset->Settings);
							}
						})
					]

					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 3.f)
					[
						SNew(SAudioFXButton)
						.Options(TArray<FText>{ LOCTEXT("FourPole", "4-Pole") })
						.AssetColor(CachedAssetCol)
						.SelectedIndex(Preset->Settings.bFourPole ? 0 : -1)
						.OnSelectionChanged_Lambda([this, LocalWeakPreset](int32 OptionIndex)
						{
							if (USubmixEffectMultibandCompressorPreset* Preset = LocalWeakPreset.Get())
							{
								MarkDirty.ExecuteIfBound();
								Preset->Settings.bFourPole = (OptionIndex == 0);
								Preset->SetSettings(Preset->Settings);
							}
						})
					]
				]

				// ── COL 3: LookAhead knob ─────────────────────────────────
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).HAlign(HAlign_Right)
				[
					SNew(SAudioFXLabeledKnob)
					.ParamName(LOCTEXT("LookAheadName", "Look Ahead"))
					.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
						LOCTEXT("LookAheadLbl", "Look Ahead"), 0.f, 50.f, TEXT("ms"),
						LOCTEXT("LookAheadTip", "Amount of look-ahead time (adds latency).")))
					.Style(LookAheadStyle)
					.AudioUnitsValueType(EAudioUnitsValueType::Linear)
					.ValueFormat(EAudioFXValueFormat::Time)
					.MinValue(0.f)
					.MaxValue(50.f)
					.Value_Lambda([LocalWeakPreset]() -> float
					{
						if (const USubmixEffectMultibandCompressorPreset* Preset = LocalWeakPreset.Get())
						{
							return Preset->Settings.LookAheadMsec / 50.f;
						}
						return 0.f;
					})
					.OnValueChanged_Lambda([this, LocalWeakPreset](float NewNorm)
					{
						if (USubmixEffectMultibandCompressorPreset* Preset = LocalWeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							Preset->Settings.LookAheadMsec = NewNorm * 50.f;
							Preset->SetSettings(Preset->Settings);
						}
					})
				]
			]
		]

		// ── MULTIBAND CURVE (shorter) ────────────────────────────────────
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
		[
			SNew(SBox).MinDesiredHeight(80.f)
			[
				SNew(SAudioFXMultibandCurve)
				.AssetColor(CapturedAssetColor)
				.Settings_Lambda([LocalWeakPreset]() -> FSubmixEffectMultibandCompressorSettings
				{
					if (const auto* Preset = LocalWeakPreset.Get()) { return Preset->Settings; }
					return FSubmixEffectMultibandCompressorSettings();
				})
				.BandColors_Lambda([LocalWeakPreset]() -> TArray<FLinearColor>
				{
					TArray<FLinearColor> Out;
					if (const auto* Preset = LocalWeakPreset.Get())
					{
						const FLinearColor* Pal = FAudioFXTheme::GetBandPalette();
						for (int32 BandIndex = 0; BandIndex < Preset->Settings.Bands.Num(); ++BandIndex)
						{
							Out.Add(Pal[BandIndex % GNumMBColors]);
						}
					}
					return Out;
				})
			]
		]

		// ── BAND RACK ────────────────────────────────────────────────
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 4.f, 0.f, 0.f)
		[ Scroll ];
}

// ─────────────────────────────────────────────────────────────────────────────
//  BuildCrossoverControl — one frequency knob displayed between adjacent cards
//
//  CrossIdx is the band index whose CrossoverTopFrequency we expose.
//  Valid range: 0 .. NumBands-2  (last band's crossover is always hidden).
//  Frequency conversion is log-space (20Hz..20kHz) to keep musical proportions.
// ─────────────────────────────────────────────────────────────────────────────

TSharedRef<SWidget> FAudioFXSubmixMultibandEditor::BuildCrossoverControl(
	int32 CrossIdx, USubmixEffectMultibandCompressorPreset* Preset)
{
	// CrossIdx is the band index whose CrossoverTopFrequency we expose.
	// Valid range: 0 .. NumBands-2  (last band's crossover is always hidden).
	TWeakObjectPtr<USubmixEffectMultibandCompressorPreset> LocalWeakPreset = WeakPreset;
	const FLinearColor* BandPalette = FAudioFXTheme::GetBandPalette();
	const FLinearColor CrossColor = BandPalette[CrossIdx % GNumMBColors];

	const FAudioMaterialKnobStyle* XoverStyle = StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Frequency, CrossColor, EAudioFXKnobSize::Tertiary));

	// Log-space freq <-> normalized [0,1] conversions (20Hz..20kHz)
	static constexpr float SMinFreqHz = 20.f;
	static constexpr float SMaxFreqHz = 20000.f;
	auto FreqToNorm = [](float Hz) -> float
	{
		const float LogMin = FMath::Log2(SMinFreqHz);
		const float LogRange = FMath::Log2(SMaxFreqHz) - LogMin;
		return (FMath::Log2(FMath::Clamp(Hz, SMinFreqHz, SMaxFreqHz)) - LogMin) / LogRange;
	};
	auto NormToFreq = [](float NewNorm) -> float
	{
		const float LogMin = FMath::Log2(SMinFreqHz);
		const float LogRange = FMath::Log2(SMaxFreqHz) - LogMin;
		return FMath::Pow(2.f, LogMin + FMath::Clamp(NewNorm, 0.f, 1.f) * LogRange);
	};

	return SNew(SBox).WidthOverride(GCrossoverKnobW)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(SAudioFXLabeledKnob)
			.ParamName(FText::GetEmpty())      // no title — crossover position shown inline
			.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
				FText::Format(LOCTEXT("XoverTipLbl", "Crossover {0}"), FText::AsNumber(CrossIdx + 1)),
				20.f, 20000.f, TEXT("Hz"),
				LOCTEXT("XoverTipBody", "Frequency boundary between this band and the next.")))
			.Style(XoverStyle)
			.AudioUnitsValueType(EAudioUnitsValueType::Frequency)
			.ValueFormat(EAudioFXValueFormat::Frequency)
			.MinValue(20.f)
			.MaxValue(20000.f)
			.Value_Lambda([LocalWeakPreset, CrossIdx, FreqToNorm]() -> float
			{
				if (const USubmixEffectMultibandCompressorPreset* Preset = LocalWeakPreset.Get(); Preset && Preset->Settings.Bands.IsValidIndex(CrossIdx))
				{
					return FreqToNorm(Preset->Settings.Bands[CrossIdx].CrossoverTopFrequency);
				}
				return 0.5f;
			})
			.OnValueChanged_Lambda([this, LocalWeakPreset, CrossIdx, NormToFreq](float NewNorm)
			{
				if (USubmixEffectMultibandCompressorPreset* Preset = LocalWeakPreset.Get())
				{
					MarkDirty.ExecuteIfBound();
					if (Preset->Settings.Bands.IsValidIndex(CrossIdx))
					{
						const float Desired = NormToFreq(NewNorm);
						Preset->Settings.Bands[CrossIdx].CrossoverTopFrequency =
							ClampCrossoverHz(Preset->Settings.Bands, CrossIdx, Desired);
						Preset->SetSettings(Preset->Settings);
					}
				}
			})
		]
	];
}

// ─────────────────────────────────────────────────────────────────────────────
//  RebuildCrossoverBar — rebuilds the dedicated crossover label + knob bar
//  (called from CreateEffectWidget; the rack also has inline crossover controls)
// ─────────────────────────────────────────────────────────────────────────────

void FAudioFXSubmixMultibandEditor::RebuildCrossoverBar()
{
	if (!CrossoverBar.IsValid()) { return; }
	CrossoverBar->ClearChildren();

	USubmixEffectMultibandCompressorPreset* Preset = WeakPreset.Get();
	if (!Preset) { return; }

	CrossoverBar->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
	[
		SNew(STextBlock)
		.Text(LOCTEXT("XoverBarLabel", "Crossovers"))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
		.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
	];

	// N bands → N-1 crossovers  (index 0 .. NumBands-2)
	const int32 NumCrossovers = FMath::Max(0, Preset->Settings.Bands.Num() - 1);
	for (int32 CrossoverIndex = 0; CrossoverIndex < NumCrossovers; ++CrossoverIndex)
	{
		CrossoverBar->AddSlot().AutoWidth().Padding(2.f, 0.f)
		[ BuildCrossoverControl(CrossoverIndex, Preset) ];
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  BuildBandCard
//
//  Constructs one compressor band card for BandIndex.
//  Card structure (top-to-bottom inside SBorder):
//    · Color stripe (4px)
//    · Band N label + hover-to-show Remove button
//    · Threshold (Hero)
//    · Ratio + Knee (side-by-side, both Tertiary)
//    · RESPONSE collapsible: Attack/Release + In Gain/Out Gain (all Tertiary)
//  A SAudioFXGainReductionMeter is rendered inside the card, above RESPONSE.
//
//  Remove is disabled when NumBands <= 2 (minimum enforcement).
// ─────────────────────────────────────────────────────────────────────────────

TSharedRef<SWidget> FAudioFXSubmixMultibandEditor::BuildBandCard(
	int32 BandIndex,
	USubmixEffectMultibandCompressorPreset* Preset,
	const FLinearColor& AssetCol,
	const FLinearColor& BandCol,
	TSharedPtr<FAudioFXPreviewEngine> InEngine)
{
	TWeakObjectPtr<USubmixEffectMultibandCompressorPreset> LocalWeakPreset = WeakPreset;
	const int32 NumBands = Preset->Settings.Bands.Num();

	// ── Per-card knob styles (all Tertiary = compact, user requested much smaller) ─
	const FAudioMaterialKnobStyle* ThreshStyle  = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Dynamics,  BandCol, EAudioFXKnobSize::Tertiary));
	const FAudioMaterialKnobStyle* RatioStyle   = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Dynamics,  BandCol, EAudioFXKnobSize::Tertiary));
	const FAudioMaterialKnobStyle* KneeStyle    = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Dynamics,  BandCol, EAudioFXKnobSize::Tertiary));
	const FAudioMaterialKnobStyle* AttackStyle  = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Time,      BandCol, EAudioFXKnobSize::Tertiary));
	const FAudioMaterialKnobStyle* ReleaseStyle = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Time,      BandCol, EAudioFXKnobSize::Tertiary));
	const FAudioMaterialKnobStyle* InGainStyle  = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain,      BandCol, EAudioFXKnobSize::Tertiary));
	const FAudioMaterialKnobStyle* OutGainStyle = StoreKnobStyle(FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain,      BandCol, EAudioFXKnobSize::Tertiary));

	// ── Color stripe brush ───────────────────────────────────────────
	const FLinearColor StripeCol = FMath::Lerp(BandCol, FLinearColor(0.5f,0.5f,0.5f,1.f), 0.15f);
	TUniquePtr<FSlateRoundedBoxBrush> StripeBrush = MakeUnique<FSlateRoundedBoxBrush>(
		StripeCol, FVector4(FAudioFXTheme::CardRadius, FAudioFXTheme::CardRadius, 0.f, 0.f));
	const FSlateBrush* StripeBrushPtr = StripeBrush.Get();
	BandStripeBrushes.Add(MoveTemp(StripeBrush));

	// ── Remove button (disabled when only 2 bands) ───────────────────
	const bool bCanRemove = (NumBands > 2);
	TSharedPtr<SBorder> RemoveBorder;
	TSharedRef<SWidget> RemoveBtn = SAssignNew(RemoveBorder, SBorder)
		.BorderImage(&FAudioFXTheme::GetRemoveButtonBrush())
		.Padding(FMargin(0.f))
		.Visibility(EVisibility::Hidden)
		[
			SNew(SButton)
			.IsEnabled(bCanRemove)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.ContentPadding(FMargin(7.f, 3.f))
			.OnClicked_Lambda([this, LocalWeakPreset, BandIndex]() -> FReply
			{
				if (USubmixEffectMultibandCompressorPreset* Preset = LocalWeakPreset.Get())
				{
					if (Preset->Settings.Bands.Num() > 2 && Preset->Settings.Bands.IsValidIndex(BandIndex))
					{
						MarkDirty.ExecuteIfBound();
						Preset->Settings.Bands.RemoveAt(BandIndex);
						// Fix the last band's crossover to be 20kHz
						if (Preset->Settings.Bands.Num() > 0)
						{
							Preset->Settings.Bands.Last().CrossoverTopFrequency = 20000.f;
						}
						Preset->SetSettings(Preset->Settings);
						RebuildRack();
					}
				}
				return FReply::Handled();
			})
			.OnHovered_Lambda([RemoveBorder]()
			{
				if (RemoveBorder.IsValid()) { RemoveBorder->SetBorderImage(&FAudioFXTheme::GetRemoveButtonHoverBrush()); }
			})
			.OnUnhovered_Lambda([RemoveBorder]()
			{
				if (RemoveBorder.IsValid()) { RemoveBorder->SetBorderImage(&FAudioFXTheme::GetRemoveButtonBrush()); }
			})
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("\u2715")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(bCanRemove
					? FLinearColor(0.8f, 0.35f, 0.35f, 1.f)
					: FLinearColor(0.4f, 0.4f, 0.4f, 1.f))
			]
		];

	// ── Card body ────────────────────────────────────────────────────
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SBox).WidthOverride(GCardWidth)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.OnHovered_Lambda([RemoveBorder]()
				{
					if (RemoveBorder.IsValid()) { RemoveBorder->SetVisibility(EVisibility::Visible); }
				})
				.OnUnhovered_Lambda([RemoveBorder]()
				{
					if (RemoveBorder.IsValid()) { RemoveBorder->SetVisibility(EVisibility::Hidden); }
				})
				[
					SNew(SBorder)
					.BorderImage(&FAudioFXTheme::GetCardBrush())
					[
						SNew(SVerticalBox)

						// ── Color stripe ──────────────────────────────────
						+ SVerticalBox::Slot().AutoHeight()
						[ SNew(SBox).HeightOverride(4.f)[ SNew(SBorder).BorderImage(StripeBrushPtr) ] ]

						// ── Band label + remove ───────────────────────────
						+ SVerticalBox::Slot().AutoHeight().Padding(6.f, 4.f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.f)
							[
								SNew(STextBlock)
								.Text(FText::Format(LOCTEXT("BandNFmt", "Band {0}"), FText::AsNumber(BandIndex + 1)))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
								.ColorAndOpacity(BandCol)
							]
							+ SHorizontalBox::Slot().AutoWidth()
							[ RemoveBtn ]
						]

						// ── THRESHOLD — hero, centered ────────────────────
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 6.f, 0.f, 2.f)
						[
							SNew(SAudioFXLabeledKnob)
							.ParamName(LOCTEXT("ThreshName", "Threshold"))
							.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
								LOCTEXT("ThreshLbl", "Threshold"), -72.f, 0.f, TEXT("dB"),
								LOCTEXT("ThreshTip", "Dynamics threshold for this band.")))
							.Style(ThreshStyle)
							.AudioUnitsValueType(EAudioUnitsValueType::Volume)
							.ValueFormat(EAudioFXValueFormat::Decibels)
							.MinValue(-72.f)
							.MaxValue(0.f)
							.Value_Lambda([LocalWeakPreset, BandIndex]() -> float
							{
								if (const USubmixEffectMultibandCompressorPreset* Preset = LocalWeakPreset.Get(); Preset && Preset->Settings.Bands.IsValidIndex(BandIndex))
								{
									return FMath::GetMappedRangeValueClamped(
										FVector2D(-72.f,0.f), FVector2D(0.f,1.f),
										Preset->Settings.Bands[BandIndex].ThresholdDb);
								}
								return 0.f;
							})
							.OnValueChanged_Lambda([this, LocalWeakPreset, BandIndex](float NewNorm)
							{
								if (USubmixEffectMultibandCompressorPreset* Preset = LocalWeakPreset.Get())
								{
									MarkDirty.ExecuteIfBound();
									if (Preset->Settings.Bands.IsValidIndex(BandIndex))
									{
										Preset->Settings.Bands[BandIndex].ThresholdDb =
											FMath::GetMappedRangeValueClamped(FVector2D(0.f,1.f), FVector2D(-72.f,0.f), NewNorm);
										Preset->SetSettings(Preset->Settings);
									}
								}
							})
						]

						// ── Ratio + Knee side-by-side ─────────────────────
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SNew(SHorizontalBox)

							// Ratio
							+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Center).Padding(4.f, 2.f)
							[
								SNew(SAudioFXLabeledKnob)
								.ParamName(LOCTEXT("RatioName", "Ratio"))
								.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
									LOCTEXT("RatioLbl", "Ratio"), 1.f, 20.f, TEXT(":1"),
									LOCTEXT("RatioTip", "Compression ratio.")))
								.Style(RatioStyle)
								.AudioUnitsValueType(EAudioUnitsValueType::Linear)
								.ValueFormat(EAudioFXValueFormat::Ratio)
								.MinValue(1.f)
								.MaxValue(20.f)
								.Value_Lambda([LocalWeakPreset, BandIndex]() -> float
								{
									if (const USubmixEffectMultibandCompressorPreset* Preset = LocalWeakPreset.Get(); Preset && Preset->Settings.Bands.IsValidIndex(BandIndex))
									{
										return FMath::GetMappedRangeValueClamped(
											FVector2D(1.f,20.f), FVector2D(0.f,1.f),
											Preset->Settings.Bands[BandIndex].Ratio);
									}
									return 0.f;
								})
								.OnValueChanged_Lambda([this, LocalWeakPreset, BandIndex](float NewNorm)
								{
									if (USubmixEffectMultibandCompressorPreset* Preset = LocalWeakPreset.Get())
									{
										MarkDirty.ExecuteIfBound();
										if (Preset->Settings.Bands.IsValidIndex(BandIndex))
										{
											Preset->Settings.Bands[BandIndex].Ratio =
												FMath::GetMappedRangeValueClamped(FVector2D(0.f,1.f), FVector2D(1.f,20.f), NewNorm);
											Preset->SetSettings(Preset->Settings);
										}
									}
								})
							]

							// Knee
							+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Center).Padding(4.f, 2.f)
							[
								SNew(SAudioFXLabeledKnob)
								.ParamName(LOCTEXT("KneeName", "Knee"))
								.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
									LOCTEXT("KneeLbl", "Knee"), 0.f, 20.f, TEXT("dB"),
									LOCTEXT("KneeTip", "Knee bandwidth (0 = hard knee).")))
								.Style(KneeStyle)
								.AudioUnitsValueType(EAudioUnitsValueType::Volume)
								.ValueFormat(EAudioFXValueFormat::Decibels)
								.MinValue(0.f)
								.MaxValue(20.f)
								.Value_Lambda([LocalWeakPreset, BandIndex]() -> float
								{
									if (const USubmixEffectMultibandCompressorPreset* Preset = LocalWeakPreset.Get(); Preset && Preset->Settings.Bands.IsValidIndex(BandIndex))
									{
										return FMath::GetMappedRangeValueClamped(
											FVector2D(0.f,20.f), FVector2D(0.f,1.f),
											Preset->Settings.Bands[BandIndex].KneeBandwidthDb);
									}
									return 0.f;
								})
								.OnValueChanged_Lambda([this, LocalWeakPreset, BandIndex](float NewNorm)
								{
									if (USubmixEffectMultibandCompressorPreset* Preset = LocalWeakPreset.Get())
									{
										MarkDirty.ExecuteIfBound();
										if (Preset->Settings.Bands.IsValidIndex(BandIndex))
										{
											Preset->Settings.Bands[BandIndex].KneeBandwidthDb =
												FMath::GetMappedRangeValueClamped(FVector2D(0.f,1.f), FVector2D(0.f,20.f), NewNorm);
											Preset->SetSettings(Preset->Settings);
										}
									}
								})
							]
						]

						// ── GAIN REDUCTION meter (inside card, above RESPONSE) ───────────
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(6.f, 4.f, 6.f, 2.f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("GRLbl", "GR"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
								.ColorAndOpacity(FAudioFXTheme::GetTextDisabledColor())
							]
							+ SHorizontalBox::Slot().AutoWidth()
							[
								SNew(SAudioFXGainReductionMeter)
								.Engine(InEngine)
								.BandIndex(BandIndex)
							]
						]

						// ── RESPONSE (collapsible): Attack/Release + In/Out Gain ──
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SAudioFXCollapsibleSection)
							.SectionLabel(LOCTEXT("AdvLbl", "RESPONSE"))
							.bDefaultExpanded(bResponseSectionExpanded)
							.OnToggled_Lambda([this]()
							{
								bResponseSectionExpanded = !bResponseSectionExpanded;
								ResizeWindowByDelta(bResponseSectionExpanded ? 230.f : -230.f);
								RebuildRack();
							})
							[
								SNew(SVerticalBox)

								// Attack + Release
								+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Center).Padding(4.f, 2.f)
									[
										SNew(SAudioFXLabeledKnob)
										.ParamName(LOCTEXT("AttackName", "Attack"))
										.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
											LOCTEXT("AttackLbl","Attack"), 1.f, 300.f, TEXT("ms"),
											LOCTEXT("AttackTip","Attack time.")))
										.Style(AttackStyle)
										.AudioUnitsValueType(EAudioUnitsValueType::Linear)
										.ValueFormat(EAudioFXValueFormat::Time)
										.MinValue(1.f)
										.MaxValue(300.f)
										.Value_Lambda([LocalWeakPreset, BandIndex]() -> float
										{
											if (const USubmixEffectMultibandCompressorPreset* Preset = LocalWeakPreset.Get(); Preset && Preset->Settings.Bands.IsValidIndex(BandIndex))
											{
												return FMath::GetMappedRangeValueClamped(
													FVector2D(1.f,300.f), FVector2D(0.f,1.f),
													Preset->Settings.Bands[BandIndex].AttackTimeMsec);
											}
										 return 0.f;
										})
										.OnValueChanged_Lambda([this, LocalWeakPreset, BandIndex](float NewNorm)
										{
											if (USubmixEffectMultibandCompressorPreset* Preset = LocalWeakPreset.Get())
											{
												MarkDirty.ExecuteIfBound();
												if (Preset->Settings.Bands.IsValidIndex(BandIndex))
												{
													Preset->Settings.Bands[BandIndex].AttackTimeMsec =
														FMath::GetMappedRangeValueClamped(FVector2D(0.f,1.f), FVector2D(1.f,300.f), NewNorm);
													Preset->SetSettings(Preset->Settings);
												}
											}
										})
									]
									+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Center).Padding(4.f, 2.f)
									[
										SNew(SAudioFXLabeledKnob)
										.ParamName(LOCTEXT("ReleaseName", "Release"))
										.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
											LOCTEXT("ReleaseLbl","Release"), 20.f, 5000.f, TEXT("ms"),
											LOCTEXT("ReleaseTip","Release time.")))
										.Style(ReleaseStyle)
										.AudioUnitsValueType(EAudioUnitsValueType::Linear)
										.ValueFormat(EAudioFXValueFormat::Time)
										.MinValue(20.f)
										.MaxValue(5000.f)
										.Value_Lambda([LocalWeakPreset, BandIndex]() -> float
										{
											if (const USubmixEffectMultibandCompressorPreset* Preset = LocalWeakPreset.Get(); Preset && Preset->Settings.Bands.IsValidIndex(BandIndex))
											{
												return FMath::GetMappedRangeValueClamped(
													FVector2D(20.f,5000.f), FVector2D(0.f,1.f),
													Preset->Settings.Bands[BandIndex].ReleaseTimeMsec);
											}
										 return 0.f;
										})
										.OnValueChanged_Lambda([this, LocalWeakPreset, BandIndex](float NewNorm)
										{
											if (USubmixEffectMultibandCompressorPreset* Preset = LocalWeakPreset.Get())
											{
												MarkDirty.ExecuteIfBound();
												if (Preset->Settings.Bands.IsValidIndex(BandIndex))
												{
													Preset->Settings.Bands[BandIndex].ReleaseTimeMsec =
														FMath::GetMappedRangeValueClamped(FVector2D(0.f,1.f), FVector2D(20.f,5000.f), NewNorm);
													Preset->SetSettings(Preset->Settings);
												}
											}
										})
									]
								]

								// In Gain + Out Gain
								+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Center).Padding(4.f, 2.f)
									[
										SNew(SAudioFXLabeledKnob)
										.ParamName(LOCTEXT("InGainName", "In Gain"))
										.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
											LOCTEXT("InGainLbl","Input Gain"), -12.f, 20.f, TEXT("dB"),
											LOCTEXT("InGainTip","Input gain.")))
										.Style(InGainStyle)
										.AudioUnitsValueType(EAudioUnitsValueType::Volume)
										.ValueFormat(EAudioFXValueFormat::Decibels)
										.MinValue(-12.f)
										.MaxValue(20.f)
										.Value_Lambda([LocalWeakPreset, BandIndex]() -> float
										{
											if (const USubmixEffectMultibandCompressorPreset* Preset = LocalWeakPreset.Get(); Preset && Preset->Settings.Bands.IsValidIndex(BandIndex))
											{
												return FMath::GetMappedRangeValueClamped(
													FVector2D(-12.f,20.f), FVector2D(0.f,1.f),
													Preset->Settings.Bands[BandIndex].InputGainDb);
											}
										 return 0.f;
										})
										.OnValueChanged_Lambda([this, LocalWeakPreset, BandIndex](float NewNorm)
										{
											if (USubmixEffectMultibandCompressorPreset* Preset = LocalWeakPreset.Get())
											{
												MarkDirty.ExecuteIfBound();
												if (Preset->Settings.Bands.IsValidIndex(BandIndex))
												{
													Preset->Settings.Bands[BandIndex].InputGainDb =
														FMath::GetMappedRangeValueClamped(FVector2D(0.f,1.f), FVector2D(-12.f,20.f), NewNorm);
													Preset->SetSettings(Preset->Settings);
												}
											}
										})
									]
									+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Center).Padding(4.f, 2.f)
									[
										SNew(SAudioFXLabeledKnob)
										.ParamName(LOCTEXT("OutGainName", "Out Gain"))
										.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
											LOCTEXT("OutGainLbl","Output Gain"), -12.f, 20.f, TEXT("dB"),
											LOCTEXT("OutGainTip","Output gain.")))
										.Style(OutGainStyle)
										.AudioUnitsValueType(EAudioUnitsValueType::Volume)
										.ValueFormat(EAudioFXValueFormat::Decibels)
										.MinValue(-12.f)
										.MaxValue(20.f)
										.Value_Lambda([LocalWeakPreset, BandIndex]() -> float
										{
											if (const USubmixEffectMultibandCompressorPreset* Preset = LocalWeakPreset.Get(); Preset && Preset->Settings.Bands.IsValidIndex(BandIndex))
											{
												return FMath::GetMappedRangeValueClamped(
													FVector2D(-12.f,20.f), FVector2D(0.f,1.f),
													Preset->Settings.Bands[BandIndex].OutputGainDb);
											}
										 return 0.f;
										})
										.OnValueChanged_Lambda([this, LocalWeakPreset, BandIndex](float NewNorm)
										{
											if (USubmixEffectMultibandCompressorPreset* Preset = LocalWeakPreset.Get())
											{
												MarkDirty.ExecuteIfBound();
												if (Preset->Settings.Bands.IsValidIndex(BandIndex))
												{
													Preset->Settings.Bands[BandIndex].OutputGainDb =
														FMath::GetMappedRangeValueClamped(FVector2D(0.f,1.f), FVector2D(-12.f,20.f), NewNorm);
													Preset->SetSettings(Preset->Settings);
												}
											}
										})
									]
								]
							]
						]

					]
				]
			]
		];
}

// ─────────────────────────────────────────────────────────────────────────────
//  BuildAddBandButton — trailing "+" card at the end of the rack
// ─────────────────────────────────────────────────────────────────────────────

TSharedRef<SWidget> FAudioFXSubmixMultibandEditor::BuildAddBandButton()
{
	TWeakObjectPtr<USubmixEffectMultibandCompressorPreset> LocalWeakPreset = WeakPreset;

	return SNew(SBox).WidthOverride(GCardWidth)
	[
		SNew(SBorder).BorderImage(&FAudioFXTheme::GetCardBrush()).Padding(FMargin(0.f))
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.ContentPadding(FMargin(10.f, 6.f))
			.ToolTipText(LOCTEXT("AddBandTip", "Add a new dynamics band"))
			.OnClicked_Lambda([this, LocalWeakPreset]() -> FReply
			{
				if (USubmixEffectMultibandCompressorPreset* Preset = LocalWeakPreset.Get())
				{
					MarkDirty.ExecuteIfBound();
					const int32 OldNum = Preset->Settings.Bands.Num();

					// ── Compute new crossover in log-freq space ───────
					float NewCrossHz = 2000.f;
					if (OldNum >= 2)
					{
						// Midpoint (in log space) between the last two crossovers
						const float LastXover    = Preset->Settings.Bands[OldNum - 2].CrossoverTopFrequency;
						const float SecondToLast = (OldNum >= 3) ? Preset->Settings.Bands[OldNum - 3].CrossoverTopFrequency : 20.f;
						NewCrossHz = FMath::Sqrt(LastXover * SecondToLast);  // geometric mean
						NewCrossHz = FMath::Clamp(NewCrossHz, LastXover + MinCrossoverGapHz, 20000.f - MinCrossoverGapHz);
					}
					else if (OldNum == 1)
					{
						NewCrossHz = FMath::Max(Preset->Settings.Bands[0].CrossoverTopFrequency + MinCrossoverGapHz, 2000.f);
					}

					// The new band takes the "last" slot, so set its crossover to 20kHz.
					// The current last band gets the new crossover.
					Preset->Settings.Bands.Last().CrossoverTopFrequency = NewCrossHz;
					Preset->Settings.Bands.AddDefaulted();
					Preset->Settings.Bands.Last().CrossoverTopFrequency = 20000.f;
					Preset->Settings.Bands.Last().ThresholdDb           = -18.f;
					Preset->Settings.Bands.Last().Ratio                 = 4.f;
					Preset->Settings.Bands.Last().AttackTimeMsec        = 10.f;
					Preset->Settings.Bands.Last().ReleaseTimeMsec        = 100.f;

					Preset->SetSettings(Preset->Settings);
					RebuildRack();
				}
				return FReply::Handled();
			})
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(1.f).VAlign(VAlign_Center).HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("+")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
					.ColorAndOpacity(CachedAssetCol)
				]
			]
		]
	];
}

// ─────────────────────────────────────────────────────────────────────────────
//  RebuildRack — clears and re-populates the band rack + inline crossover knobs
//
//  Called after any band add or remove operation.
//  Inserts alternating: [BandCard][CrossoverControl][BandCard]...[AddButton]
//  The crossover between bands is embedded inline (not in the separate CrossoverBar).
// ─────────────────────────────────────────────────────────────────────────────

void FAudioFXSubmixMultibandEditor::RebuildRack()
{
	if (!BandRack.IsValid()) { return; }

	BandRack->ClearChildren();
	BandStripeBrushes.Empty();

	USubmixEffectMultibandCompressorPreset* Preset = WeakPreset.Get();
	if (!Preset) { return; }

	{
		const FLinearColor* BandPalette = FAudioFXTheme::GetBandPalette();
		for (int32 BandIndex = 0; BandIndex < Preset->Settings.Bands.Num(); ++BandIndex)
		{
			const FLinearColor BandCol = BandPalette[BandIndex % GNumMBColors];
			BandRack->AddSlot().AutoWidth().Padding(4.f, 0.f)
			[ BuildBandCard(BandIndex, Preset, CachedAssetCol, BandCol, GetPreviewEngine()) ];
			// Add crossover knob after every band except the last
			if (BandIndex < Preset->Settings.Bands.Num() - 1)
			{
				BandRack->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f)
				[ BuildCrossoverControl(BandIndex, Preset) ];
			}
		}
	}

	BandRack->AddSlot().AutoWidth().VAlign(VAlign_Fill).Padding(8.f, 0.f)
	[ BuildAddBandButton() ];

	// Crossovers now embedded in rack — no separate bar rebuild needed
}

#undef LOCTEXT_NAMESPACE
