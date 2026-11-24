// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

/**
 * SAudioFXWaveShaperCurve
 * Draws y = tanh(x * k) transfer curve in real-time.
 * k is derived from Amount (0-500). Square widget, defaults 160x160.
 */
class AUDIOEFFECTSSTUDIO_API SAudioFXWaveShaperCurve : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SAudioFXWaveShaperCurve)
		: _Amount(1.0f)
		, _AssetColor(FLinearColor(0.855f, 0.969f, 0.306f))
	{}
		SLATE_ATTRIBUTE(float, Amount)
		SLATE_ARGUMENT(FLinearColor, AssetColor)
	SLATE_END_ARGS()

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

	/** Clamp and sanitize amount before mapping to drive. */
	static float ClampAmount(float InAmount);

	/** Map amount [0,500] to display drive k [0.01,20]. */
	static float AmountToDrive(float InAmount);

	/** Evaluate y=tanh(x*k) for normalized x in [-1,1]. */
	static float EvaluateTransfer(float NormalizedInput, float DriveK);

private:
	TAttribute<float>  Amount;
	FLinearColor       AssetColor = FLinearColor::White;
};
