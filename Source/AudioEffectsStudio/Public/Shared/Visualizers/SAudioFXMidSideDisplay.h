// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SLeafWidget.h"

/**
 * SAudioFXMidSideDisplay
 *
 * Triangle visualization of the Mid/Side stereo balance.
 * The triangle apex rises with MidLevel (mono content) and widens with
 * SideLevel (stereo width), giving an instant visual read of M/S energy.
 *
 * Layout:
 *   - Dark inset background
 *   - Wide glow pass (7px, 18% opacity) behind the triangle edges
 *   - Solid triangle outline (2px, 90% opacity, asset color)
 *   - 6px listener dot at the apex of the display (center-top)
 *
 * All values are normalized [0,1]. MidLevel=1 → apex at top;
 * SideLevel=1 → triangle spans full width.
 */
class AUDIOEFFECTSSTUDIO_API SAudioFXMidSideDisplay : public SLeafWidget
{
public:

	// ============================================================
	// Slate arguments
	// ============================================================

	SLATE_BEGIN_ARGS(SAudioFXMidSideDisplay)
		: _MidLevel(0.5f)
		, _SideLevel(0.5f)
		, _AssetColor(FLinearColor::White)
	{}
		/** Normalized mid (mono) energy [0,1] — controls apex Y position */
		SLATE_ATTRIBUTE(float, MidLevel)

		/** Normalized side (stereo) energy [0,1] — controls triangle base width */
		SLATE_ATTRIBUTE(float, SideLevel)

		/** Effect asset color — used for the triangle outline and listener dot */
		SLATE_ARGUMENT(FLinearColor, AssetColor)
	SLATE_END_ARGS()

	// ============================================================
	// Public interface
	// ============================================================

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;

	// ============================================================
	// Public statics (also used by tests)
	// ============================================================

	/** Clamp a normalized level to [0,1], substituting Fallback for non-finite values. */
	static float ClampNormalizedLevel(float InLevel, float Fallback = 0.0f);

	/** Map MidLevel to the apex normalized Y position (0=top, 1=bottom). */
	static float ComputeApexNormalizedY(float InMidLevel);

	/** Map SideLevel to the half-width as a fraction of the widget width. */
	static float ComputeHalfWidthNormalized(float InSideLevel);

	/**
	 * Build the normalized triangle outline as a closed polyline.
	 * Points are in [0,1]×[0,1] space; caller maps to pixel coords.
	 * The output array always contains 4 points (base-left → apex → base-right → base-left).
	 */
	static void BuildTriangleNormalized(float InMidLevel, float InSideLevel, TArray<FVector2f>& OutPoints);

private:

	// ============================================================
	// Members
	// ============================================================

	TAttribute<float> MidLevel;
	TAttribute<float> SideLevel;
	FLinearColor AssetColor = FLinearColor::White;

	static constexpr float GraphInset = 6.f;
};
