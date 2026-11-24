// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Visualizers/SAudioFXGainReductionMeter.h"
#include "Shared/Core/AudioFXPreviewEngine.h"
#include "Shared/Core/AudioFXTheme.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SAudioFXGainReductionMeter"

// ============================================================
// Lifecycle
// ============================================================

void SAudioFXGainReductionMeter::Construct(const FArguments& InArgs)
{
	Engine = InArgs._Engine;
	BandIndex = InArgs._BandIndex;

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(32.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.0f, 4.0f, 0.0f, 2.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("GRLabel", "GR"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FAudioFXTheme::GetHeaderTextColor())
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.HAlign(HAlign_Center)
			.Padding(0.0f, 2.0f, 0.0f, 2.0f)
			[
				SNew(SBox)
				.WidthOverride(12.0f)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.0f, 2.0f, 0.0f, 2.0f)
			[
				SNew(STextBlock)
				.Text_Lambda([this]()
				{
					return FText::FromString(FString::Printf(TEXT("-%0.1f dB"), DisplayReductionDb));
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity_Lambda([this]()
				{
					return bHasActiveData ? FAudioFXTheme::GetValueTextColor() : FAudioFXTheme::GetTextDisabledColor();
				})
			]
		]
	];
}

// ═══════════════════════════════════════════════════════════════════════════
//  Tick — poll the preview engine and update meter state
// ═══════════════════════════════════════════════════════════════════════════

void SAudioFXGainReductionMeter::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	float RawReductionDb = 0.0f;
	bHasActiveData = false;

	// ── Sample live gain reduction from the preview engine ────────────────
	if (Engine.IsValid() && Engine->HasGainReductionData())
	{
		// Current capture API is single-value. BandIndex is reserved for future per-band support.
		(void)BandIndex;
		RawReductionDb = FMath::Clamp(-Engine->GetGainReductionDb(), 0.0f, MaxDisplayDb);
		bHasActiveData = true;
	}

	// ── Instant-fill: attack is immediate, decay is slew-rate limited ─────
	if (RawReductionDb > DisplayReductionDb)
	{
		DisplayReductionDb = RawReductionDb;
	}
	else
	{
		DisplayReductionDb = FMath::Max(0.0f, DisplayReductionDb - DecayRateDbPerSec * InDeltaTime);
	}

	// ── Peak hold: reset timer on new peak, decay after PeakHoldTime ──────
	if (RawReductionDb >= PeakReductionDb)
	{
		PeakReductionDb = RawReductionDb;
		PeakHoldTimer   = 0.0f;
	}
	else
	{
		PeakHoldTimer += InDeltaTime;
		if (PeakHoldTimer > PeakHoldTime)
		{
			PeakReductionDb = FMath::Max(0.0f, PeakReductionDb - DecayRateDbPerSec * InDeltaTime);
		}
	}

	Invalidate(EInvalidateWidgetReason::Paint);
}

// ═══════════════════════════════════════════════════════════════════════════
//  OnPaint — draw the track, fill bar, and peak indicator
// ═══════════════════════════════════════════════════════════════════════════

int32 SAudioFXGainReductionMeter::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	LayerId = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	const FVector2D LocalSize  = AllottedGeometry.GetLocalSize();
	// Reserve space at top (GR label) and bottom (numeric readout) for the text slots
	const float TrackTop    = 20.0f;
	const float TrackBottom = LocalSize.Y - 18.0f;
	const float TrackHeight = FMath::Max(TrackBottom - TrackTop, 1.0f);
	const float BarWidth    = 12.0f;
	const float BarX        = (LocalSize.X - BarWidth) * 0.5f;  // centered horizontally

	const FSlateBrush* WhiteBox = FCoreStyle::Get().GetBrush("GenericWhiteBox");

	// ── Track background (full height, window background color) ──────────
	FSlateDrawElement::MakeBox(
		OutDrawElements, LayerId + 1,
		AllottedGeometry.ToPaintGeometry(
			FVector2f(BarWidth, TrackHeight),
			FSlateLayoutTransform(FVector2f(BarX, TrackTop))),
		WhiteBox, ESlateDrawEffect::None, FAudioFXTheme::GetWindowBackground());

	// ── Fill bar — height proportional to current reduction, color-coded ─
	if (DisplayReductionDb > KINDA_SMALL_NUMBER)
	{
		const float FillRatio  = FMath::Clamp(DisplayReductionDb / MaxDisplayDb, 0.0f, 1.0f);
		const float FillHeight = TrackHeight * FillRatio;
		// Switch to warn color when reduction exceeds 20 dB
		const FLinearColor MeterColor = DisplayReductionDb > 20.0f
			? FAudioFXTheme::GetAccentWarn()
			: FAudioFXTheme::GetAccentPrimary();

		FSlateDrawElement::MakeBox(
			OutDrawElements, LayerId + 2,
			AllottedGeometry.ToPaintGeometry(
				FVector2f(BarWidth, FillHeight),
				FSlateLayoutTransform(FVector2f(BarX, TrackTop))),
			WhiteBox, ESlateDrawEffect::None, bHasActiveData ? MeterColor : FAudioFXTheme::GetTextDisabledColor());
	}

	// ── Peak hold indicator — 2px horizontal tick at peak position ────────
	if (PeakReductionDb > KINDA_SMALL_NUMBER)
	{
		const float PeakY = TrackTop + TrackHeight * FMath::Clamp(PeakReductionDb / MaxDisplayDb, 0.0f, 1.0f);
		FSlateDrawElement::MakeBox(
			OutDrawElements, LayerId + 3,
			AllottedGeometry.ToPaintGeometry(
				FVector2f(BarWidth + 2.0f, 2.0f),
				FSlateLayoutTransform(FVector2f(BarX - 1.0f, PeakY))),
			WhiteBox, ESlateDrawEffect::None,
			bHasActiveData ? FAudioFXTheme::GetTextAccentColor() : FAudioFXTheme::GetTextDisabledColor());
	}

	return LayerId + 3;
}

#undef LOCTEXT_NAMESPACE
