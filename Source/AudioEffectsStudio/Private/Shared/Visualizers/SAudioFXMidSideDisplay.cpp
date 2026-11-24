// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Visualizers/SAudioFXMidSideDisplay.h"
#include "Shared/Core/AudioFXTheme.h"

#include "Rendering/DrawElementTypes.h"

// ============================================================
// Lifecycle
// ============================================================

void SAudioFXMidSideDisplay::Construct(const FArguments& InArgs)
{
	MidLevel = InArgs._MidLevel;
	SideLevel = InArgs._SideLevel;
	AssetColor = InArgs._AssetColor;
}

FVector2D SAudioFXMidSideDisplay::ComputeDesiredSize(float) const
{
	return FVector2D(220.f, 130.f);
}

// ═══════════════════════════════════════════════════════════════════════════
//  Static helpers
// ═══════════════════════════════════════════════════════════════════════════

float SAudioFXMidSideDisplay::ClampNormalizedLevel(float InLevel, float Fallback)
{
	const float SafeValue = FMath::IsFinite(InLevel) ? InLevel : Fallback;
	return FMath::Clamp(SafeValue, 0.0f, 1.0f);
}

float SAudioFXMidSideDisplay::ComputeApexNormalizedY(float InMidLevel)
{
	return 1.0f - ClampNormalizedLevel(InMidLevel, 0.0f);
}

float SAudioFXMidSideDisplay::ComputeHalfWidthNormalized(float InSideLevel)
{
	return ClampNormalizedLevel(InSideLevel, 0.0f) * 0.5f;
}

void SAudioFXMidSideDisplay::BuildTriangleNormalized(float InMidLevel, float InSideLevel, TArray<FVector2f>& OutPoints)
{
	const float ApexY     = ComputeApexNormalizedY(InMidLevel);   // [0=top, 1=bottom]
	const float HalfWidth = ComputeHalfWidthNormalized(InSideLevel); // fraction of full width

	// Closed polyline: bottom-left → apex → bottom-right → back to bottom-left
	OutPoints.Reset(4);
	OutPoints.Add(FVector2f(0.5f - HalfWidth, 1.0f));   // base-left corner
	OutPoints.Add(FVector2f(0.5f,             ApexY));   // apex
	OutPoints.Add(FVector2f(0.5f + HalfWidth, 1.0f));   // base-right corner
	OutPoints.Add(FVector2f(0.5f - HalfWidth, 1.0f));   // close the shape
}

// ═══════════════════════════════════════════════════════════════════════════
//  OnPaint
// ═══════════════════════════════════════════════════════════════════════════

int32 SAudioFXMidSideDisplay::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const FVector2f Size  = UE::Slate::CastToVector2f(AllottedGeometry.GetLocalSize());
	// Inset the drawable area by GraphInset on all sides
	const float Width  = FMath::Max(Size.X - GraphInset * 2.f, 1.f);
	const float Height = FMath::Max(Size.Y - GraphInset * 2.f, 1.f);

	const FPaintGeometry PlotGeo = AllottedGeometry.ToPaintGeometry(
		FVector2f(Width, Height),
		FSlateLayoutTransform(FVector2f(GraphInset, GraphInset)));

	// ── Layer 0: dark inset background ───────────────────────────────────
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		PlotGeo,
		&FAudioFXTheme::GetInsetBrush(),
		ESlateDrawEffect::None,
		FAudioFXTheme::GetVisualizationBackground());
	++LayerId;

	// ── Build normalized triangle and map to pixel coordinates ───────────
	TArray<FVector2f> NormalizedPoints;
	BuildTriangleNormalized(MidLevel.Get(0.5f), SideLevel.Get(0.5f), NormalizedPoints);

	TArray<FVector2f> PixelPoints;
	PixelPoints.Reserve(NormalizedPoints.Num());
	for (const FVector2f& Point : NormalizedPoints)
	{
		PixelPoints.Add(FVector2f(Point.X * Width, Point.Y * Height));
	}

	// ── Layer 1: wide glow pass (bloom effect behind the edges) ──────────
	FSlateDrawElement::MakeLines(
		OutDrawElements,
		LayerId,
		PlotGeo,
		PixelPoints,
		ESlateDrawEffect::None,
		AssetColor.CopyWithNewOpacity(0.18f),
		true,
		7.0f);
	++LayerId;

	// ── Layer 2: solid triangle outline ──────────────────────────────────
	FSlateDrawElement::MakeLines(
		OutDrawElements,
		LayerId,
		PlotGeo,
		PixelPoints,
		ESlateDrawEffect::None,
		AssetColor.CopyWithNewOpacity(0.9f),
		true,
		2.0f);
	++LayerId;

	// ── Layer 3: listener dot — 6px horizontal tick at the top-center ────
	// Represents the "listener" position as a fixed reference point.
	const TArray<FVector2f> ListenerPoint = {
		FVector2f(Width * 0.5f - 1.0f, 0.0f),
		FVector2f(Width * 0.5f + 1.0f, 0.0f)
	};
	FSlateDrawElement::MakeLines(
		OutDrawElements,
		LayerId,
		PlotGeo,
		ListenerPoint,
		ESlateDrawEffect::None,
		AssetColor,
		true,
		6.0f);

	return LayerId;
}
