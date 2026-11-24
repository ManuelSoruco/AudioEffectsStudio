// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.
// SAudioFXMeterStrip — triple-value level meter (RMS / Realtime Peak / Historic Peak).

#include "Shared/Widgets/SAudioFXMeterStrip.h"
#include "Shared/Core/AudioFXPreviewEngine.h"
#include "Shared/Core/AudioFXTheme.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"

#define LOCTEXT_NAMESPACE "SAudioFXMeterStrip"

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────

float SAudioFXMeterStrip::LinearToDb(float Linear)
{
	if (Linear <= 0.000001f) return -80.0f;
	return FMath::Clamp(20.0f * FMath::LogX(10.0f, Linear), -80.0f, 0.0f);
}

FString SAudioFXMeterStrip::DbToString(float Db)
{
	if (Db <= -80.0f) return TEXT("\u2212\u221E dB"); // −∞ dB
	return FString::Printf(TEXT("%.1f dB"), Db);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Construct
// ─────────────────────────────────────────────────────────────────────────────

void SAudioFXMeterStrip::Construct(const FArguments& InArgs)
{
	Engine    = InArgs._Engine;
	bIsInput  = InArgs._bIsInput;
	bIsStereo = InArgs._bIsStereo;

	const FText Label = bIsInput
		? LOCTEXT("InputLabel",  "IN")
		: LOCTEXT("OutputLabel", "OUT");

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(bIsStereo ? 34.0f : 20.0f)
		[
			SNew(SVerticalBox)

			// ── IN/OUT label ──────────────────────────────────────────────
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.0f, 4.0f, 0.0f, 2.0f)
			[
				SNew(STextBlock)
				.Text(Label)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FAudioFXTheme::GetHeaderTextColor())
			]

			// ── Bar area (fills remaining height minus readout) ───────────
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.HAlign(HAlign_Center)
			.Padding(0.0f, 2.0f, 0.0f, 0.0f)
			[
				SNew(SBox)
				.WidthOverride(bIsStereo ? 18.0f : 8.0f)
			]

			// ── Numeric readout placeholder (custom-painted below) ────────
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			[
				SNew(SBox)
				.HeightOverride(ReadoutHeight)
			]
		]
	];
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tick
// ─────────────────────────────────────────────────────────────────────────────

void SAudioFXMeterStrip::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (!Engine.IsValid())
	{
		DisplayLevel = DisplayLevelR = RmsLevel = RmsLevelR = 0.0f;
		PeakLevel = PeakLevelR = 0.0f;
		PeakHoldTimer = PeakHoldTimerR = 0.0f;
		// Historic peak intentionally preserved on engine disconnect
		return;
	}

	// ── Peak — Exchange (max-since-last-Tick, resets to 0) ───────────────
	float RawL, RawR;
	if (bIsInput) { RawL = RawR = Engine->ExchangeInputPeak(); }
	else          { RawL = Engine->ExchangeOutputPeakL(); RawR = Engine->ExchangeOutputPeakR(); }

	// ── RMS — plain load (DSP follower smooths it already) ───────────────
	float RmsL, RmsR;
	if (bIsInput) { RmsL = RmsR = Engine->GetInputRmsLevel(); }
	else          { RmsL = Engine->GetOutputRmsLevelL(); RmsR = Engine->GetOutputRmsLevelR(); }

	if (!bIsStereo) { RawR = RawL; RmsR = RmsL; }

	// ── Ballistics — instant attack, PPM decay ────────────────────────────
	auto ApplyBallistics = [&](float Raw, float& Display, float& Peak, float& HoldTimer)
	{
		if (Raw > Display) { Display = Raw; }
		else               { Display = FMath::Max(Display - DecayRate * InDeltaTime, 0.0f); }

		if (Raw >= Peak) { Peak = Raw; HoldTimer = 0.0f; }
		else
		{
			HoldTimer += InDeltaTime;
			if (HoldTimer > PeakHoldTime)
				Peak = FMath::Max(Peak - PeakDecayRate * InDeltaTime, 0.0f);
		}
	};

	ApplyBallistics(RawL, DisplayLevel,  PeakLevel,  PeakHoldTimer);
	ApplyBallistics(RawR, DisplayLevelR, PeakLevelR, PeakHoldTimerR);

	// ── RMS store ─────────────────────────────────────────────────────────
	RmsLevel  = RmsL;
	RmsLevelR = RmsR;

	// ── Historic peak — latch-highest, never decays (click to reset) ──────
	const float CurPeak = bIsStereo ? FMath::Max(RawL, RawR) : RawL;
	if (CurPeak > HistoricPeak)  HistoricPeak  = CurPeak;
	if (RawL    > HistoricPeak)  HistoricPeak  = RawL;
	if (RawR    > HistoricPeakR) HistoricPeakR = RawR;

	Invalidate(EInvalidateWidgetReason::Paint);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Click-to-reset
// ─────────────────────────────────────────────────────────────────────────────

FReply SAudioFXMeterStrip::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		HistoricPeak  = 0.0f;
		HistoricPeakR = 0.0f;
		Invalidate(EInvalidateWidgetReason::Paint);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

// ─────────────────────────────────────────────────────────────────────────────
//  OnPaint
// ─────────────────────────────────────────────────────────────────────────────

int32 SAudioFXMeterStrip::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	LayerId = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const FSlateBrush* WhiteBox = FCoreStyle::Get().GetBrush("GenericWhiteBox");

	// ── Track geometry ────────────────────────────────────────────────────
	const float TrackTop    = 20.0f;
	const float TrackBottom = LocalSize.Y - ReadoutHeight - 2.0f;
	const float TrackHeight = FMath::Max(TrackBottom - TrackTop, 1.0f);

	// ── Bar geometry ──────────────────────────────────────────────────────
	const float BarW   = 8.0f;
	const float TotalW = bIsStereo ? (BarW * 2.0f + 2.0f) : BarW;
	const float StartX = (LocalSize.X - TotalW) * 0.5f;

	// ── Per-channel draw ──────────────────────────────────────────────────
	auto DrawChannel = [&](float Display, float Rms, float Peak, float BarX)
	{
		// Background
		FSlateDrawElement::MakeBox(
			OutDrawElements, LayerId + 1,
			AllottedGeometry.ToPaintGeometry(FVector2f(BarW, TrackHeight), FSlateLayoutTransform(FVector2f(BarX, TrackTop))),
			WhiteBox, ESlateDrawEffect::None, FAudioFXTheme::GetWindowBackground());

		// RMS fill (dim)
		if (Rms > 0.001f)
		{
			const float H = TrackHeight * FMath::Clamp(Rms, 0.0f, 1.0f);
			const float Y = TrackTop + TrackHeight - H;
			FLinearColor C = (Rms < 0.6f) ? FLinearColor(0.18f,0.36f,0.18f,1.f)
			               : (Rms < 0.9f) ? FLinearColor(0.46f,0.26f,0.11f,1.f)
			               :                FLinearColor(0.5f, 0.10f,0.13f,1.f);
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 2,
				AllottedGeometry.ToPaintGeometry(FVector2f(BarW, H), FSlateLayoutTransform(FVector2f(BarX, Y))),
				WhiteBox, ESlateDrawEffect::None, C);
		}

		// Realtime peak fill (bright)
		if (Display > 0.001f)
		{
			const float H = TrackHeight * FMath::Clamp(Display, 0.0f, 1.0f);
			const float Y = TrackTop + TrackHeight - H;
			FLinearColor C = (Display < 0.6f) ? FLinearColor(0.361f,0.722f,0.361f,1.f)
			               : (Display < 0.9f) ? FLinearColor(0.910f,0.525f,0.227f,1.f)
			               :                    FLinearColor(1.0f,  0.2f,  0.267f,1.f);
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 3,
				AllottedGeometry.ToPaintGeometry(FVector2f(BarW, H), FSlateLayoutTransform(FVector2f(BarX, Y))),
				WhiteBox, ESlateDrawEffect::None, C);
		}

		// Peak hold tick (white 2px)
		if (Peak > 0.001f)
		{
			const float Y = TrackTop + TrackHeight * (1.0f - FMath::Clamp(Peak, 0.0f, 1.0f));
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 4,
				AllottedGeometry.ToPaintGeometry(FVector2f(BarW + 2.0f, 2.0f), FSlateLayoutTransform(FVector2f(BarX - 1.0f, Y))),
				WhiteBox, ESlateDrawEffect::None, FLinearColor(1.f,1.f,1.f,0.85f));
		}
	};

	if (bIsStereo)
	{
		DrawChannel(DisplayLevel,  RmsLevel,  PeakLevel,  StartX);
		DrawChannel(DisplayLevelR, RmsLevelR, PeakLevelR, StartX + BarW + 2.0f);
	}
	else
	{
		DrawChannel(DisplayLevel, RmsLevel, PeakLevel, StartX);
	}

	// ── Numeric readout — historic peak only (click anywhere to reset) ───────
	const float ReadoutY = TrackBottom + 4.0f;

	const float HistDb = LinearToDb(bIsStereo ? (HistoricPeak + HistoricPeakR) * 0.5f : HistoricPeak);

	// Right-align inside the bar column so the decimal stays fixed.
	// We paint into a box that spans from StartX to StartX+TotalW+2px margin,
	// right-justified by shifting the origin so the text ends at the right edge.
	const FSlateFontInfo SmFont   = FCoreStyle::GetDefaultFontStyle("Regular", 7);
	const FString        HistText = DbToString(HistDb);

	// Measure text width to compute right-aligned X offset inside the bar column
	const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FVector2D TextSize = FontMeasure->Measure(HistText, SmFont);

	// Align right edge of text to (StartX + TotalW - 2px)
	const float RightEdge = StartX + TotalW - 2.0f;
	const float TextX     = FMath::Max(RightEdge - TextSize.X, 0.0f);

	FSlateDrawElement::MakeText(
		OutDrawElements, LayerId + 5,
		AllottedGeometry.ToPaintGeometry(
			FVector2f(TextSize.X, ReadoutHeight),
			FSlateLayoutTransform(FVector2f(TextX, ReadoutY))),
		HistText,
		SmFont,
		ESlateDrawEffect::None,
		FLinearColor::White);

	// Dim separator line between bar and readout
	FSlateDrawElement::MakeBox(
		OutDrawElements, LayerId + 5,
		AllottedGeometry.ToPaintGeometry(
			FVector2f(LocalSize.X, 1.0f),
			FSlateLayoutTransform(FVector2f(0.0f, TrackBottom + 2.0f))),
		WhiteBox, ESlateDrawEffect::None,
		FLinearColor(1.f,1.f,1.f,0.12f));

	return LayerId + 5;
}

#undef LOCTEXT_NAMESPACE
