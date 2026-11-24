// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

/**
 * SAudioFXEnvelopeDisplay
 *
 * Real-time Attack/Release envelope shape drawn in OnPaint.
 * Shows a stylized AD (Attack-Decay) curve that reflects the current AttackTime
 * and ReleaseTime values — as those change with knobs, the curve redraws live.
 *
 * Layout:
 *   - Dark inset background
 *   - Subtle time grid lines (4 vertical segments)
 *   - 0-line at bottom (signal floor)
 *   - Envelope curve: attack rise → peak hold → release fall
 *     - Attack: exponential-style curve (concave up)
 *     - Release: exponential-style decay (concave down)
 *   - Glow pass beneath the curve
 *   - Attack / Release time labels at the inflection point
 *
 * All drawing is pure Slate — no materials, no textures.
 */
class AUDIOEFFECTSSTUDIO_API SAudioFXEnvelopeDisplay : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SAudioFXEnvelopeDisplay)
		: _AttackTime(10.f)
		, _ReleaseTime(200.f)
		, _AssetColor(FLinearColor(0.972f, 0.855f, 0.306f))
	{}
		/** Live attack time in ms — bind to preset */
		SLATE_ATTRIBUTE(float, AttackTime)

		/** Live release time in ms — bind to preset */
		SLATE_ATTRIBUTE(float, ReleaseTime)

		/** Effect asset color — SLATE_ARGUMENT last so its member is declared last */
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

	static float ClampAttackMs(float InAttackMs);
	static float ClampReleaseMs(float InReleaseMs);
	static float ComputeAttackFraction(float InAttackMs, float InReleaseMs);
	static float EvaluateEnvelopeNormalized(float NormalizedX, float InAttackMs, float InReleaseMs);

private:
	TAttribute<float> AttackTime;
	TAttribute<float> ReleaseTime;
	FLinearColor AssetColor = FLinearColor(0.972f, 0.855f, 0.306f);

	// Max time for display scaling
	static constexpr float MaxAttackMs  = 300.f;
	static constexpr float MaxReleaseMs = 5000.f;
	static constexpr int32 NumCurvePoints = 256;
	static constexpr float PreFrac = 0.08f;
	static constexpr float PeakFrac = 0.12f;
	static constexpr float TailFrac = 0.08f;
};
