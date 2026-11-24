// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "SourceEffects/SourceEffectDynamicsProcessor.h"

/**
 * SAudioFXTransferCurve
 *
 * Real-time dynamics transfer curve display — drawn entirely in OnPaint.
 * Shows the input/output dB mapping for the current compressor/limiter/expander/gate settings.
 *
 * Layout (square, typically 200x200):
 *   - Dark inset background (GetInsetBrush)
 *   - Subtle dB grid lines (every 12dB)
 *   - 1:1 reference diagonal (dashed, 15% opacity)
 *   - Threshold crossover line (vertical dashed, asset color at 30%)
 *   - Knee region highlight (subtle fill around threshold)
 *   - Transfer curve (solid, 2px, asset color, antialiased)
 *   - dB axis labels (small text, TextDisabled color)
 *
 * All drawing is pure Slate — no materials, no textures.
 */
class AUDIOEFFECTSSTUDIO_API SAudioFXTransferCurve : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SAudioFXTransferCurve)
		: _AssetColor(FLinearColor(0.855f, 0.780f, 0.043f))  // Gold, matching dynamics default
	{}
		/** Live settings read every frame — bind to the preset's Settings struct */
		SLATE_ATTRIBUTE(FSourceEffectDynamicsProcessorSettings, Settings)

		/** Effect asset color — used for the curve and threshold marker */
		SLATE_ARGUMENT(FLinearColor, AssetColor)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// ============================================================
	// SLeafWidget interface
	// ============================================================

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

	/**
	 * Compute output dB from input dB using the current settings.
	 * Applies input gain, threshold, ratio, knee, and upward compression if relevant.
	 * Public: pure math function, also used by automated tests.
	 */
	static float ComputeOutputDb(float InputDb,
		const FSourceEffectDynamicsProcessorSettings& S);

private:

	// ============================================================
	// Members
	// ============================================================

	TAttribute<FSourceEffectDynamicsProcessorSettings> Settings;
	FLinearColor AssetColor = FLinearColor(0.855f, 0.780f, 0.043f);

	// dB range displayed on both axes
	static constexpr float DisplayDbMin = -60.f;
	static constexpr float DisplayDbMax =  12.f;
	static constexpr float DisplayDbRange = DisplayDbMax - DisplayDbMin;  // 72 dB

	// ============================================================
	// Private paint helpers (called by OnPaint)
	// ============================================================

	/** Draw the dark inset background box. */
	int32 PaintTransferBackground(
		const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements,
		int32 LayerId) const;

	/** Draw the dB grid lines (every 12 dB, both axes). */
	int32 PaintTransferGrid(
		const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, float Width, float Height) const;

	/** Draw the dashed 1:1 diagonal reference line. */
	int32 PaintTransferDiagonalRef(
		const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, float Width, float Height) const;

	/** Draw the soft knee region highlight box. */
	int32 PaintTransferKnee(
		const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, float Width, float Height,
		const FSourceEffectDynamicsProcessorSettings& DynSettings) const;

	/** Draw the dashed threshold crossover marker. */
	int32 PaintTransferThreshold(
		const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, float Width, float Height,
		const FSourceEffectDynamicsProcessorSettings& DynSettings) const;

	/** Draw the wide, low-opacity glow pass behind the transfer curve. */
	int32 PaintTransferGlow(
		const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, float Width, float Height,
		const FSourceEffectDynamicsProcessorSettings& DynSettings) const;

	/** Draw the solid 2px transfer curve. */
	int32 PaintTransferCurve(
		const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, float Width, float Height,
		const FSourceEffectDynamicsProcessorSettings& DynSettings) const;
};
