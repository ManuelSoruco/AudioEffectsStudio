// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

/**
 * FAudioFXTapDelayEditor — implementation
 *
 * CreateEffectWidget builds:
 *   1. SAudioFXTapDelayGraph  (top, fills height)
 *   2. GLOBAL section:
 *       - Max Delay + Interp knobs (Secondary size)
 *       - START TAPPING / STOP transport (calls HandleTapRecordToggle)
 *       - TAP button (calls RegisterTapTempo during active session)
 *   3. Horizontal scrollable tap rack (BuildTapCard per tap + BuildAddTapButton)
 *
 * Key functions:
 *   BuildTapCard()             — one card per FTapDelayInfo entry
 *   HandleTapRecordToggle()    — START/STOP logic extracted from the button lambda
 *   BuildTapIntervalsMs()      — converts raw timestamps to intervals
 *   BuildTapGainEnvelopeDb()   — descending dB envelope over N taps
 *   RegisterTapTempo()         — appends one timestamp during active recording
 *   RebuildRack()              — clears and rebuilds the tap rack after add/remove
 */

#include "SubmixEffects/AudioFXTapDelayEditor.h"

#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Widgets/SAudioFXDropdown.h"
#include "Shared/Visualizers/SAudioFXTapDelayGraph.h"
#include "Shared/Widgets/SAudioFXLabeledKnob.h"
#include "Shared/Widgets/SAudioFXSectionBox.h"

#include "AudioWidgetsEnums.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AudioFXTapDelay"

// NOTE: Do NOT initialize GTapColors as a static here — GetBandPalette() calls
// GetDefault<UAudioEffectsStudioSettings>() which requires the UObject system to be ready.
// A static initializer runs at DLL-load time, before UObject init, causing a crash.
// Call FAudioFXTheme::GetBandPalette() lazily at actual usage sites instead.
static constexpr int32 GNumTapColors = FAudioFXTheme::NumBandPaletteColors;
static constexpr float GTapCardMinWidth = 126.f;  // matches GBandCardWidth in AudioFXEQUI.cpp
static constexpr float GTapTempoMinIntervalMs = 50.f;
static constexpr float GTapTempoMaxIntervalMs = 5000.f;
static constexpr int32 GTapTempoHistorySize = 8;

// ═══════════════════════════════════════════════════════════════════════════
// CreateEffectWidget
TSharedRef<SWidget> FAudioFXTapDelayEditor::CreateEffectWidget(USoundEffectPreset* InPreset)
{
	USubmixEffectTapDelayPreset* Preset = Cast<USubmixEffectTapDelayPreset>(InPreset);
	if (!Preset)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("AudioEffectsStudio: Cast to preset type failed for '%s'. Check asset registration."),
			InPreset ? *InPreset->GetClass()->GetName() : TEXT("null"));
		return FAudioFXEffectEditor::CreateEffectWidget(InPreset);
	}

	WeakPreset     = Preset;
	CachedAssetCol = GetAssetColor();

	TWeakObjectPtr<USubmixEffectTapDelayPreset> LocalWeakPreset = Preset;

	// ── Header knob styles — Visual Redesign: MaxLen=Secondary, Interp=Tertiary ──
	const FAudioMaterialKnobStyle* MaxLenStyle = StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Time, CachedAssetCol, EAudioFXKnobSize::Secondary));
	const FAudioMaterialKnobStyle* InterpStyle = StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Time, CachedAssetCol, EAudioFXKnobSize::Tertiary));

	// ── Tap rack ─────────────────────────────────────────────────
	TSharedRef<SScrollBox> Scroll = SNew(SScrollBox).Orientation(Orient_Horizontal);
	TSharedRef<SHorizontalBox> Rack = SNew(SHorizontalBox);
	TapRack = Rack;

	for (int32 Index = 0; Index < Preset->Settings.Taps.Num(); ++Index)
	{
		Rack->AddSlot().AutoWidth().Padding(4.f, 0.f)
		[ BuildTapCard(Index, Preset, CachedAssetCol) ];
	}
	Rack->AddSlot().AutoWidth().VAlign(VAlign_Fill).Padding(8.f, 0.f)
	[ BuildAddTapButton() ];

	Scroll->AddSlot()[ Rack ];

	return SNew(SVerticalBox)

		// ── TAP GRAPH (TOP) ───────────────────────────────────────
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 0.f, 0.f, 4.f)
		[
			SNew(SBox)
			.MinDesiredHeight(200.f)
			[
				SNew(SAudioFXTapDelayGraph)
				.MaxDelayLength_Lambda([LocalWeakPreset]()
				{
					if (const USubmixEffectTapDelayPreset* Preset = LocalWeakPreset.Get())
					{
						return Preset->Settings.MaximumDelayLength;
					}
					return SAudioFXTapDelayGraph::DefaultMaxDelayLengthMs;
				})
				.Taps_Lambda([LocalWeakPreset]()
				{
					if (const USubmixEffectTapDelayPreset* Preset = LocalWeakPreset.Get())
					{
						return Preset->Settings.Taps;
					}
					return TArray<FTapDelayInfo>();
				})
				.AssetColor(CachedAssetCol)
			]
		]

		// ── HEADER: grouped global controls + tap recording transport ──────────
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SAudioFXSectionBox).SectionLabel(LOCTEXT("HeaderSec", "GLOBAL"))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Center).VAlign(VAlign_Center)
				.Padding(FAudioFXTheme::KnobHorizontalSpacing, 4.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(SAudioFXLabeledKnob)
						.ParamName(LOCTEXT("MaxLenLbl", "Max Delay"))
						.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(LOCTEXT("MaxDelayTipLbl", "Maximum Delay Length"), 10.f, 20000.f, TEXT("ms"), LOCTEXT("MaxDelayTip", "Maximum possible length for a delay, in milliseconds.\nAudioFactory Note: Changing this at runtime will reset the effect.")))
						.Value_Lambda([LocalWeakPreset]()
						{
							if (const USubmixEffectTapDelayPreset* Preset = LocalWeakPreset.Get())
							{
								return (float)FMath::GetMappedRangeValueClamped(FVector2D(10.f, 20000.f), FVector2D(0.f, 1.f), Preset->Settings.MaximumDelayLength);
							}
							return 0.5f;
						})
						.Style(MaxLenStyle)
						.AudioUnitsValueType(EAudioUnitsValueType::Linear)
						.ValueFormat(EAudioFXValueFormat::Time)
						.DefaultValue(FMath::GetMappedRangeValueClamped(FVector2D(10.f, 20000.f), FVector2D(0.f, 1.f), 4000.f))
						.MinValue(10.f)
						.MaxValue(20000.f)
						.OnValueChanged_Lambda([this, LocalWeakPreset](float NewNorm)
						{
							if (USubmixEffectTapDelayPreset* Preset = LocalWeakPreset.Get())
							{
								MarkDirty.ExecuteIfBound();
								Preset->Settings.MaximumDelayLength = FMath::GetMappedRangeValueClamped(
									FVector2D(0.f, 1.f), FVector2D(10.f, 20000.f), NewNorm);
								Preset->Settings.MaximumDelayLength = FMath::Max(Preset->Settings.MaximumDelayLength, 1.f);
								Preset->SetSettings(Preset->Settings);
							}
						})
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 2.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("GlobalsLabel", "GLOBAL TIME"))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Center).VAlign(VAlign_Center)
				.Padding(FAudioFXTheme::KnobHorizontalSpacing, 4.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(SAudioFXLabeledKnob)
						.ParamName(LOCTEXT("InterpLbl", "Interp"))
						.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(LOCTEXT("InterpTipLbl", "Interpolation Time"), 0.f, 20000.f, TEXT("ms"), LOCTEXT("InterpTip", "Number of milliseconds over which a tap will reach its set length and gain. Smaller values are more responsive, larger values make pitching less dramatic.")))
						.Value_Lambda([LocalWeakPreset]()
						{
							if (const USubmixEffectTapDelayPreset* Preset = LocalWeakPreset.Get())
							{
								return (float)FMath::GetMappedRangeValueClamped(FVector2D(0.f, 20000.f), FVector2D(0.f, 1.f), Preset->Settings.InterpolationTime);
							}
							return 0.0f;
						})
						.Style(InterpStyle)
						.AudioUnitsValueType(EAudioUnitsValueType::Linear)
						.ValueFormat(EAudioFXValueFormat::Time)
						.DefaultValue(FMath::GetMappedRangeValueClamped(FVector2D(0.f, 20000.f), FVector2D(0.f, 1.f), 100.f))
						.MinValue(0.f)
						.MaxValue(20000.f)
						.OnValueChanged_Lambda([this, LocalWeakPreset](float NewNorm)
						{
							if (USubmixEffectTapDelayPreset* Preset = LocalWeakPreset.Get())
							{
								MarkDirty.ExecuteIfBound();
								Preset->Settings.InterpolationTime = FMath::GetMappedRangeValueClamped(
									FVector2D(0.f, 1.f), FVector2D(0.f, 20000.f), NewNorm);
								Preset->SetSettings(Preset->Settings);
							}
						})
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 2.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Text_Lambda([LocalWeakPreset]()
						{
							if (const USubmixEffectTapDelayPreset* Preset = LocalWeakPreset.Get())
							{
								return FText::Format(LOCTEXT("TapCountFmt", "{0} taps"), FText::AsNumber(Preset->Settings.Taps.Num()));
							}
							return LOCTEXT("TapCountFallback", "0 taps");
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Fill).Padding(12.f, 0.f, 0.f, 0.f)
				[
					FAudioFXTheme::MakeVerticalDivider()
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.f, 0.f, 0.f, 0.f)
				[
					SNew(SVerticalBox)

					// START TAPPING / STOP button — calls HandleTapRecordToggle()
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(SButton)
						.ButtonStyle(FAppStyle::Get(), "NoBorder")
						.ToolTipText(LOCTEXT("TapRecordStartStopTip", "Start/stop tap recording session"))
						.OnClicked_Lambda([this]() -> FReply
						{
							HandleTapRecordToggle();
							return FReply::Handled();
						})
						[
							SNew(SBorder)
							.BorderImage_Lambda([this]() -> const FSlateBrush*
							{
								return bTapRecordingActive
									? &FAudioFXTheme::GetRemoveButtonBrush()
									: &FAudioFXTheme::GetAddButtonBrush();
							})
							.Padding(FMargin(16.f, 6.f))
							[
								SNew(STextBlock)
								.Text_Lambda([this]()
								{
									return bTapRecordingActive
										? LOCTEXT("TapRecordStop",  "STOP")
										: LOCTEXT("TapRecordStart", "START TAPPING");
								})
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
								.ColorAndOpacity(FAudioFXTheme::GetValueTextColor())
							]
						]
					]

					// TAP button — records a single timestamp during an active session
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 6.f, 0.f, 0.f)
					[
						SNew(SBox).MinDesiredWidth(80.f).HeightOverride(34.f)
						[
							SNew(SButton)
							.ButtonStyle(FAppStyle::Get(), "NoBorder")
							.IsEnabled_Lambda([this]() { return bTapRecordingActive; })
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.ToolTipText(LOCTEXT("TapTempoTapTip", "Record one tap timestamp"))
							.OnClicked_Lambda([this]() -> FReply
							{
								RegisterTapTempo();
								return FReply::Handled();
							})
							[
								SNew(SBorder)
								.BorderImage_Lambda([this]() -> const FSlateBrush*
								{
									return bTapRecordingActive
										? &FAudioFXTheme::GetAddButtonBrush()
										: &FAudioFXTheme::GetCardBrush();
								})
								.Padding(FMargin(16.f, 4.f))
								[
									SNew(STextBlock)
									.Text(LOCTEXT("TapTempoTapButton", "TAP"))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
									.ColorAndOpacity(FAudioFXTheme::GetValueTextColor())
								]
							]
						]
					]
				]
			]
		]

		// ── TAP RACK (horizontal scroll) ─────────────────────────
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
		[ Scroll ];
}

// ═══════════════════════════════════════════════════════════════════════════
// BuildTapCard
TSharedRef<SWidget> FAudioFXTapDelayEditor::BuildTapCard(
	int32 TapIndex, USubmixEffectTapDelayPreset* Preset, const FLinearColor& AssetCol)
{
	// Each tap gets a hue from the cycling palette, blended slightly with the asset color
	const FLinearColor TapCol = FMath::Lerp(
		FAudioFXTheme::GetBandPalette()[TapIndex % GNumTapColors], AssetCol, 0.15f);

	TWeakObjectPtr<USubmixEffectTapDelayPreset> LocalWeakPreset = WeakPreset;

	// ── Knob styles — all Tertiary (smaller) per user request ──
	const FAudioMaterialKnobStyle* DelayStyle   = StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Time,    TapCol, EAudioFXKnobSize::Tertiary));
	const FAudioMaterialKnobStyle* GainStyle    = StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Gain,    TapCol, EAudioFXKnobSize::Tertiary));
	const FAudioMaterialKnobStyle* PanStyle     = StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Mix,     TapCol, EAudioFXKnobSize::Tertiary));
	const FAudioMaterialKnobStyle* ChannelStyle = StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Mix,     TapCol, EAudioFXKnobSize::Tertiary));

	const FTapDelayInfo& Tap = Preset->Settings.Taps[TapIndex];

	// TapLineMode options: SendToChannel=0, Panning=1, Disabled=2
	const int32 ModeIndex = (int32)Tap.TapLineMode;

	// Track live mode for visibility lambdas
	TSharedRef<ETapLineMode> LiveMode = MakeShared<ETapLineMode>(Tap.TapLineMode);

	// ── Visibility lambdas ───────────────────────────────────────
	// Pan knob visible when Mode == Panning
	// Channel knob visible when Mode == SendToChannel
	TSharedPtr<SWidget> PanWidget;
	TSharedPtr<SWidget> ChannelWidget;

	SAssignNew(PanWidget, SBox)
		.Visibility_Lambda([LiveMode]() -> EVisibility
		{
			return (*LiveMode == ETapLineMode::Panning)
				? EVisibility::Visible : EVisibility::Collapsed;
		})
		[
			SNew(SBox).HAlign(HAlign_Center).Padding(6.f, 2.f)
			[
				SNew(SAudioFXLabeledKnob)
				.ParamName(LOCTEXT("PanLbl", "Pan"))
				.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(LOCTEXT("TapPanTipLbl", "Pan"), -180.f, 180.f, TEXT("deg"), LOCTEXT("TapPanTip", "Angle at which the echo is panned. 0 is center, -90 is left, 90 is right, 180 is behind.")))
				.Value_Lambda([LocalWeakPreset, TapIndex]()
				{
					if (const USubmixEffectTapDelayPreset* Preset = LocalWeakPreset.Get(); Preset && Preset->Settings.Taps.IsValidIndex(TapIndex))
					{
						return (float)FMath::GetMappedRangeValueClamped(FVector2D(-180.f, 180.f), FVector2D(0.f, 1.f), Preset->Settings.Taps[TapIndex].PanInDegrees);
					}
					return 0.5f;
				})
				.Style(PanStyle)
				.AudioUnitsValueType(EAudioUnitsValueType::Linear)
				.DefaultValue(0.5f)
				.MinValue(-180.f)
				.MaxValue(180.f)
				.OnValueChanged_Lambda([this, LocalWeakPreset, TapIndex](float NewNorm)
				{
					if (USubmixEffectTapDelayPreset* Preset = LocalWeakPreset.Get())
					{
						MarkDirty.ExecuteIfBound();
						if (Preset->Settings.Taps.IsValidIndex(TapIndex))
						{
							Preset->Settings.Taps[TapIndex].PanInDegrees =
								FMath::GetMappedRangeValueClamped(
									FVector2D(0.f, 1.f), FVector2D(-180.f, 180.f), NewNorm);
							Preset->SetSettings(Preset->Settings);
						}
					}
				})
			]
		];

	SAssignNew(ChannelWidget, SBox)
		.Visibility_Lambda([LiveMode]() -> EVisibility
		{
			return (*LiveMode == ETapLineMode::SendToChannel)
				? EVisibility::Visible : EVisibility::Collapsed;
		})
		[
			SNew(SBox).HAlign(HAlign_Center).Padding(6.f, 2.f)
			[
				SNew(SAudioFXLabeledKnob)
				.ParamName(LOCTEXT("ChannelLbl", "Channel"))
				.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(LOCTEXT("TapChanTipLbl", "Output Channel"), 0.f, 7.f, TEXT(""), LOCTEXT("TapChanTip", "Index of the output channel to which this tap sends audio.")))
				.Value_Lambda([LocalWeakPreset, TapIndex]()
				{
					if (const USubmixEffectTapDelayPreset* Preset = LocalWeakPreset.Get(); Preset && Preset->Settings.Taps.IsValidIndex(TapIndex))
					{
						return (float)FMath::GetMappedRangeValueClamped(FVector2D(0.f, 7.f), FVector2D(0.f, 1.f), (float)Preset->Settings.Taps[TapIndex].OutputChannel);
					}
					return 0.f;
				})
				.Style(ChannelStyle)
				.AudioUnitsValueType(EAudioUnitsValueType::Linear)
				.ValueFormat(EAudioFXValueFormat::Integer)
				.DefaultValue(0.f)
				.MinValue(0.f)
				.MaxValue(7.f)
				.OnValueChanged_Lambda([this, LocalWeakPreset, TapIndex](float NewNorm)
				{
					if (USubmixEffectTapDelayPreset* Preset = LocalWeakPreset.Get())
					{
						MarkDirty.ExecuteIfBound();
						if (Preset->Settings.Taps.IsValidIndex(TapIndex))
						{
							Preset->Settings.Taps[TapIndex].OutputChannel =
								FMath::RoundToInt(FMath::GetMappedRangeValueClamped(
									FVector2D(0.f, 1.f), FVector2D(0.f, 7.f), NewNorm));
							Preset->SetSettings(Preset->Settings);
						}
					}
				})
			]
		];

	// ── Color stripe brush ────────────────────────────────────────
	TUniquePtr<FSlateRoundedBoxBrush> StripeBrush = MakeUnique<FSlateRoundedBoxBrush>(
		TapCol,
		FVector4(FAudioFXTheme::CardRadius, FAudioFXTheme::CardRadius, 0.0, 0.0));
	const FSlateBrush* StripeBrushPtr = StripeBrush.Get();
	TapStripeBrushes.Add(MoveTemp(StripeBrush));

	// ── Pre-build remove button (hidden, shown on card hover) ─────
	TSharedPtr<SBorder> RemoveBorder;
	TSharedRef<SWidget> RemoveBtn = SAssignNew(RemoveBorder, SBorder)
		.BorderImage(&FAudioFXTheme::GetRemoveButtonBrush())
		.Padding(FMargin(0.f))
		.Visibility(EVisibility::Hidden)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.ContentPadding(FMargin(7.f, 3.f))
			.OnClicked_Lambda([this, LocalWeakPreset, TapIndex]() -> FReply
			{
				if (USubmixEffectTapDelayPreset* Preset = LocalWeakPreset.Get())
				{
					MarkDirty.ExecuteIfBound();
					if (Preset->Settings.Taps.IsValidIndex(TapIndex))
					{
						Preset->Settings.Taps.RemoveAt(TapIndex);
						Preset->SetSettings(Preset->Settings);
						RebuildRack();
					}
				}
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
				.Text(FText::FromString(TEXT("✕")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FLinearColor(0.8f, 0.35f, 0.35f, 1.f))
			]
		];

	// ── Mode strip options ────────────────────────────────────────
	TArray<FText> ModeOpts = {
		LOCTEXT("ModeChannel", "Ch"),
		LOCTEXT("ModePan",     "Pan"),
		LOCTEXT("ModeOff",     "Off")
	};

	// ── Card ─────────────────────────────────────────────────────
	return SNew(SBox).MinDesiredWidth(GTapCardMinWidth)
	[
		SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "NoBorder")
		.ContentPadding(FMargin(0.f))
		.OnClicked_Lambda([]() -> FReply { return FReply::Unhandled(); })
		.OnHovered_Lambda([RemoveBorder]()
		{
			if (RemoveBorder.IsValid())
			{
				RemoveBorder->SetVisibility(EVisibility::Visible);
			}
		})
		.OnUnhovered_Lambda([RemoveBorder]()
		{
			if (RemoveBorder.IsValid())
			{
				RemoveBorder->SetVisibility(EVisibility::Hidden);
			}
		})
		[
			SNew(SBorder)
			.BorderImage(&FAudioFXTheme::GetCardBrush())
			.Padding(0.f)
			[
				SNew(SVerticalBox)

				// Color stripe (top)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBox).HeightOverride(4.f)
					[ SNew(SBorder).BorderImage(StripeBrushPtr) ]
				]

				// Inner top shadow
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBox).HeightOverride(1.f)
					[ SNew(SBorder).BorderImage(&FAudioFXTheme::GetInnerShadowBrush()) ]
				]

				// Tap label
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(4.f, 4.f, 4.f, 2.f)
				[
					SNew(STextBlock)
					.Text(FText::Format(LOCTEXT("TapFmt", "Tap {0}"), FText::AsNumber(TapIndex + 1)))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(FSlateColor(TapCol))
				]

				// Mode strip (Ch / Pan / Off)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(4.f, 2.f)
				[
					SNew(SAudioFXDropdown)
					.Options(ModeOpts)
					.AssetColor(TapCol)
					.SelectedIndex(ModeIndex)
					.OnSelectionChanged_Lambda([this, LocalWeakPreset, TapIndex, LiveMode](int32 OptionIndex)
					{
						if (USubmixEffectTapDelayPreset* Preset = LocalWeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							if (Preset->Settings.Taps.IsValidIndex(TapIndex))
							{
								const ETapLineMode NewMode = (ETapLineMode)FMath::Clamp(OptionIndex, 0, 2);
								Preset->Settings.Taps[TapIndex].TapLineMode = NewMode;
								*LiveMode = NewMode;
								Preset->SetSettings(Preset->Settings);
							}
						}
					})
				]

				// Delay knob (Hero)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(6.f, 4.f)
				[
					SNew(SAudioFXLabeledKnob)
					.ParamName(LOCTEXT("DelayLbl", "Delay"))
					.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(LOCTEXT("TapDelayTipLbl", "Tap Delay"), 0.f, 20000.f, TEXT("ms"), LOCTEXT("TapDelayTip", "Amount of time before this echo is heard in milliseconds.")))
					.Value_Lambda([LocalWeakPreset, TapIndex]()
					{
						if (const USubmixEffectTapDelayPreset* Preset = LocalWeakPreset.Get(); Preset && Preset->Settings.Taps.IsValidIndex(TapIndex))
						{
							return (float)FMath::GetMappedRangeValueClamped(FVector2D(0.f, 20000.f), FVector2D(0.f, 1.f), Preset->Settings.Taps[TapIndex].DelayLength);
						}
						return 0.f;
					})
					.Style(DelayStyle)
					.AudioUnitsValueType(EAudioUnitsValueType::Linear)
					.ValueFormat(EAudioFXValueFormat::Time)
					.DefaultValue(FMath::GetMappedRangeValueClamped(FVector2D(0.f, 20000.f), FVector2D(0.f, 1.f), 500.f))
					.MinValue(0.f)
					.MaxValue(20000.f)
					.OnValueChanged_Lambda([this, LocalWeakPreset, TapIndex](float NewNorm)
					{
						if (USubmixEffectTapDelayPreset* Preset = LocalWeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							if (Preset->Settings.Taps.IsValidIndex(TapIndex))
							{
								Preset->Settings.Taps[TapIndex].DelayLength =
									FMath::GetMappedRangeValueClamped(
										FVector2D(0.f, 1.f), FVector2D(0.f, 20000.f), NewNorm);
								Preset->SetSettings(Preset->Settings);
							}
						}
					})
				]

				// Gain knob (Standard)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(6.f, 2.f)
				[
					SNew(SAudioFXLabeledKnob)
					.ParamName(LOCTEXT("GainLbl", "Gain"))
					.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(LOCTEXT("TapGainTipLbl", "Tap Gain"), -60.f, 6.f, TEXT("dB"), LOCTEXT("TapGainTip", "How loud this echo should be, in decibels.")))
					.Value_Lambda([LocalWeakPreset, TapIndex]()
					{
						if (const USubmixEffectTapDelayPreset* Preset = LocalWeakPreset.Get(); Preset && Preset->Settings.Taps.IsValidIndex(TapIndex))
						{
							return (float)FMath::GetMappedRangeValueClamped(FVector2D(-60.f, 6.f), FVector2D(0.f, 1.f), Preset->Settings.Taps[TapIndex].Gain);
						}
						return 0.5f;
					})
					.Style(GainStyle)
					.AudioUnitsValueType(EAudioUnitsValueType::Volume)
					.ValueFormat(EAudioFXValueFormat::Decibels)
					.DefaultValue(FMath::GetMappedRangeValueClamped(FVector2D(-60.f, 6.f), FVector2D(0.f, 1.f), 0.f))
					.MinValue(-60.f)
					.MaxValue(6.f)
					.OnValueChanged_Lambda([this, LocalWeakPreset, TapIndex](float NewNorm)
					{
						if (USubmixEffectTapDelayPreset* Preset = LocalWeakPreset.Get())
						{
							MarkDirty.ExecuteIfBound();
							if (Preset->Settings.Taps.IsValidIndex(TapIndex))
							{
								Preset->Settings.Taps[TapIndex].Gain =
									FMath::GetMappedRangeValueClamped(
										FVector2D(0.f, 1.f), FVector2D(-60.f, 6.f), NewNorm);  // dB
								Preset->SetSettings(Preset->Settings);
							}
						}
					})
				]

				// Conditional: Pan knob (visible when Mode == Panning)
				+ SVerticalBox::Slot().AutoHeight()
				[ PanWidget.ToSharedRef() ]

				// Conditional: Channel knob (visible when Mode == SendToChannel)
				+ SVerticalBox::Slot().AutoHeight()
				[ ChannelWidget.ToSharedRef() ]

				// Remove button (hidden by default, shown on hover)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(4.f, 4.f, 4.f, 4.f)
				[ RemoveBtn ]

				// Bottom shadow strip
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBox).HeightOverride(2.f)
					[ SNew(SBorder).BorderImage(&FAudioFXTheme::GetCardShadowBrush()) ]
				]
			]
		]
	];
}

// ═══════════════════════════════════════════════════════════════════════════
// RebuildRack
void FAudioFXTapDelayEditor::RebuildRack()
{
	USubmixEffectTapDelayPreset* Preset = WeakPreset.Get();
	if (!Preset || !TapRack.IsValid())
	{
		return;
	}

	TapRack->ClearChildren();
	TapStripeBrushes.Empty();

	for (int32 Index = 0; Index < Preset->Settings.Taps.Num(); ++Index)
	{
		TapRack->AddSlot().AutoWidth().Padding(4.f, 0.f)
		[ BuildTapCard(Index, Preset, CachedAssetCol) ];
	}

	TapRack->AddSlot().AutoWidth().VAlign(VAlign_Fill).Padding(8.f, 0.f)
	[ BuildAddTapButton() ];
}

// ═══════════════════════════════════════════════════════════════════════════
// BuildAddTapButton
TSharedRef<SWidget> FAudioFXTapDelayEditor::BuildAddTapButton()
{
	return SNew(SBox).WidthOverride(GTapCardMinWidth)
	[
		SNew(SBorder)
		.BorderImage(&FAudioFXTheme::GetCardBrush())
		.Padding(FMargin(0.f))
		[
			SNew(SButton)
			.HAlign(HAlign_Center).VAlign(VAlign_Center)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.ContentPadding(FMargin(0.f))
			.ToolTipText(LOCTEXT("AddTapTip", "Add a new delay tap"))
			.OnClicked_Lambda([this]() -> FReply
			{
				if (USubmixEffectTapDelayPreset* Preset = WeakPreset.Get())
				{
					MarkDirty.ExecuteIfBound();
					FTapDelayInfo NewTap;
					NewTap.DelayLength = 250.f;    // Default 250ms
					NewTap.Gain        = -3.f;     // -3 dB
					NewTap.TapLineMode = ETapLineMode::Panning;
					Preset->Settings.Taps.Add(NewTap);
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
					.ColorAndOpacity(FAudioFXTheme::GetAccentSecondary())
				]
			]
		]
	];
}

// ═══════════════════════════════════════════════════════════════════════════
// BuildTapIntervalsMs
TArray<float> FAudioFXTapDelayEditor::BuildTapIntervalsMs(const TArray<double>& InTapTimestampsSec)
{
	TArray<float> IntervalsMs;
	if (InTapTimestampsSec.Num() < 2)
	{
		return IntervalsMs;
	}

	IntervalsMs.Reserve(InTapTimestampsSec.Num() - 1);
	for (int32 Index = 1; Index < InTapTimestampsSec.Num(); ++Index)
	{
		// Measure each tap's delay from the FIRST tap (index 0), not from the previous tap.
		// This gives absolute tap positions: tap 1 = distance from start, tap 2 = distance from start, etc.
		const double DeltaSeconds = InTapTimestampsSec[Index] - InTapTimestampsSec[0];
		const float IntervalMs = static_cast<float>(FMath::Max(DeltaSeconds, 0.0) * 1000.0);
		IntervalsMs.Add(FMath::Clamp(IntervalMs, GTapTempoMinIntervalMs, GTapTempoMaxIntervalMs));
	}

	return IntervalsMs;
}

// ═══════════════════════════════════════════════════════════════════════════
// BuildTapGainEnvelopeDb
TArray<float> FAudioFXTapDelayEditor::BuildTapGainEnvelopeDb(int32 NumIntervals)
{
	TArray<float> GainsDb;
	if (NumIntervals <= 0)
	{
		return GainsDb;
	}

	GainsDb.Reserve(NumIntervals);
	const float MaxGainDb = 0.f;
	const float MinGainDb = -24.f;
	if (NumIntervals == 1)
	{
		GainsDb.Add(MaxGainDb);
		return GainsDb;
	}
	for (int32 TapIdx = 0; TapIdx < NumIntervals; ++TapIdx)
	{
		const float Alpha = static_cast<float>(TapIdx) / static_cast<float>(NumIntervals - 1);
		GainsDb.Add(FMath::Lerp(MaxGainDb, MinGainDb, Alpha));
	}

	return GainsDb;
}

// ═══════════════════════════════════════════════════════════════════════════
// HandleTapRecordToggle
//
// Called by the START TAPPING / STOP button in the GLOBAL transport strip.
//
// START path: marks the session active, resets the timestamp list, and
//             records t=0 as the implicit first beat.
//
// STOP path:  derives a delay interval per tap from the recorded timestamps,
//             builds a descending gain envelope, applies both to the preset,
//             and rebuilds the tap rack UI.
// ═══════════════════════════════════════════════════════════════════════════
void FAudioFXTapDelayEditor::HandleTapRecordToggle()
{
	USubmixEffectTapDelayPreset* Preset = WeakPreset.Get();
	if (!Preset)
	{
		return;
	}

	// ── START ────────────────────────────────────────────────────────
	if (!bTapRecordingActive)
	{
		bTapRecordingActive   = true;
		TapRecordStartSeconds = FPlatformTime::Seconds();
		TapTimestamps.Reset();
		TapTimestamps.Add(0.0);   // first beat is at t=0
		return;
	}

	// ── STOP — derive intervals and apply to preset ───────────────────
	bTapRecordingActive = false;

	const TArray<float> IntervalsMs = BuildTapIntervalsMs(TapTimestamps);
	if (IntervalsMs.Num() <= 0)
	{
		// No usable taps recorded — clean up silently
		TapTimestamps.Reset();
		TapRecordStartSeconds = 0.0;
		return;
	}

	const TArray<float> GainEnvelopeDb = BuildTapGainEnvelopeDb(IntervalsMs.Num());
	if (GainEnvelopeDb.Num() != IntervalsMs.Num())
	{
		// Envelope size mismatch — unexpected, skip to avoid partial writes
		TapTimestamps.Reset();
		TapRecordStartSeconds = 0.0;
		return;
	}

	// Apply derived intervals + gain envelope
	MarkDirty.ExecuteIfBound();
	Preset->Settings.Taps.SetNum(IntervalsMs.Num());
	for (int32 TapIndex = 0; TapIndex < IntervalsMs.Num(); ++TapIndex)
	{
		FTapDelayInfo& Tap = Preset->Settings.Taps[TapIndex];
		Tap.DelayLength = FMath::Clamp(IntervalsMs[TapIndex], 0.f, Preset->Settings.MaximumDelayLength);
		Tap.Gain        = GainEnvelopeDb[TapIndex];
	}
	Preset->SetSettings(Preset->Settings);
	RebuildRack();

	// Reset recording state
	TapTimestamps.Reset();
	TapRecordStartSeconds = 0.0;
}

// ═══════════════════════════════════════════════════════════════════════════
// RegisterTapTempo
void FAudioFXTapDelayEditor::RegisterTapTempo()
{
	if (!bTapRecordingActive)
	{
		return;
	}

	const double Now = FPlatformTime::Seconds();
	if (TapRecordStartSeconds <= 0.0)
	{
		TapRecordStartSeconds = Now;
	}
	TapTimestamps.Add(Now - TapRecordStartSeconds);
	if (TapTimestamps.Num() > (GTapTempoHistorySize + 1))
	{
		TapTimestamps.RemoveAt(1, TapTimestamps.Num() - (GTapTempoHistorySize + 1));
	}

	// ── Live preview: update the graph on every tap ───────────────────
	// Compute provisional intervals and push to preset (no dirty marking —
	// the final write with MarkDirty happens on STOP).
	USubmixEffectTapDelayPreset* Preset = WeakPreset.Get();
	if (Preset && TapTimestamps.Num() >= 2)
	{
		const TArray<float> ProvisionalIntervalsMs = BuildTapIntervalsMs(TapTimestamps);
		const TArray<float> ProvisionalGainDb       = BuildTapGainEnvelopeDb(ProvisionalIntervalsMs.Num());
		if (ProvisionalIntervalsMs.Num() == ProvisionalGainDb.Num())
		{
			Preset->Settings.Taps.SetNum(ProvisionalIntervalsMs.Num());
			for (int32 TapIndex = 0; TapIndex < ProvisionalIntervalsMs.Num(); ++TapIndex)
			{
				FTapDelayInfo& Tap = Preset->Settings.Taps[TapIndex];
				Tap.DelayLength = FMath::Clamp(ProvisionalIntervalsMs[TapIndex], 0.f, Preset->Settings.MaximumDelayLength);
				Tap.Gain        = ProvisionalGainDb[TapIndex];
			}
			// SetSettings without Modify/MarkDirty — this is preview-only.
			Preset->SetSettings(Preset->Settings);
		}
	}
}

// ═══════════════════════════════════════════════════════════════════════════
// ApplyTapTempoToAll
void FAudioFXTapDelayEditor::ApplyTapTempoToAll(float DelayMs)
{
	if (USubmixEffectTapDelayPreset* Preset = WeakPreset.Get())
	{
		MarkDirty.ExecuteIfBound();
		const float ClampedDelay = FMath::Clamp(DelayMs, 0.f, Preset->Settings.MaximumDelayLength);
		if (!FMath::IsNearlyEqual(ClampedDelay, DelayMs))
		{
			UE_LOG(LogAudio, Verbose, TEXT("AudioEffectsStudio: Tap tempo %.2fms clamped to %.2fms (MaxDelayLength=%.2fms)."), DelayMs, ClampedDelay, Preset->Settings.MaximumDelayLength);
		}
		for (FTapDelayInfo& Tap : Preset->Settings.Taps)
		{
			Tap.DelayLength = ClampedDelay;
		}
		Preset->SetSettings(Preset->Settings);
	}
}

#undef LOCTEXT_NAMESPACE
