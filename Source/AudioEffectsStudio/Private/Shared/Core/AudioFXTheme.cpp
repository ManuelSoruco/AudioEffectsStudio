// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

/**
 * @file AudioFXTheme.cpp
 * @brief Non-inline implementations for FAudioFXTheme.
 *
 * Contains:
 *  - GetPalette()      -- Three complete palette definitions (Dark / Light / Unreal).
 *                         Currently Dark is always returned; the others are retained
 *                         for future theme-switching support.
 *  - MakeGroupLabel()  -- Requires STextBlock (not exposed in AudioFXTheme.h).
 *  - MakeVerticalDivider() -- Requires SBox / SBorder (not exposed in AudioFXTheme.h).
 *
 * MarkPresetDirty() and GetKnobColorSaturation() are implemented via trampolines
 * in AudioEffectsStudioSettings.cpp to avoid circular includes with UHT-generated code.
 */

#include "Shared/Core/AudioFXTheme.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"

// ============================================================
// Palette Definitions
// ============================================================

const FAudioFXThemePalette& FAudioFXTheme::GetPalette()
{
	// ---- Dark (default -- values match the original hardcoded colors) ----------
	static const FAudioFXThemePalette DarkPalette = {
		// Surface / Panel
		FLinearColor(FColor(0x0A, 0x0A, 0x0A)),  // WindowBackground   #0A0A0A
		FLinearColor(FColor(0x11, 0x11, 0x11)),  // PanelBackground    #111111
		FLinearColor(FColor(0x11, 0x11, 0x11)),  // SectionBackground  #111111
		FLinearColor(FColor(0x19, 0x19, 0x19)),  // SurfaceLight       #191919
		FLinearColor(FColor(0x22, 0x22, 0x22)),  // SurfaceRaised      #222222
		FLinearColor(FColor(0x19, 0x19, 0x19)),  // SurfaceMid         #191919
		FLinearColor(FColor(0x2A, 0x2A, 0x2A)),  // DividerColor       #2A2A2A
		// Text
		FLinearColor(FColor(0xB0, 0xB0, 0xB0)),  // HeaderTextColor    #B0B0B0
		FLinearColor(FColor(0xB0, 0xB0, 0xB0)),  // LabelTextColor     #B0B0B0
		FLinearColor(FColor(0xC8, 0xC8, 0xC8)),  // ValueTextColor     #C8C8C8
		FLinearColor(FColor(0x55, 0x55, 0x55)),  // TextDisabledColor  #555555
		FLinearColor(FColor(0xDD, 0xDD, 0xDD)),  // TextAccentColor    #DDDDDD
		FLinearColor(FColor(0x44, 0x88, 0xFF)),  // FocusRingColor     #4488FF
		// Accent
		FLinearColor(FColor(0xFF, 0x6B, 0x00)),  // AccentPrimary      #FF6B00
		FLinearColor(FColor(0x00, 0xD4, 0xAA)),  // AccentSecondary    #00D4AA
		FLinearColor(FColor(0xFF, 0x33, 0x44)),  // AccentWarn         #FF3344
		FLinearColor(FColor(0x44, 0x88, 0xFF)),  // AccentInfo         #4488FF
		// Effect Category Accent Base
		FLinearColor(FColor(0x2D, 0x9C, 0xDB)),  // AccentDelayBase       #2D9CDB
		FLinearColor(FColor(0xA8, 0x55, 0xF7)),  // AccentFilterBase      #A855F7
		FLinearColor(FColor(0xFF, 0x4D, 0x6A)),  // AccentDynamicsBase    #FF4D6A
		FLinearColor(FColor(0x22, 0xD3, 0xEE)),  // AccentModulationBase  #22D3EE
		FLinearColor(FColor(0xF5, 0x9E, 0x0B)),  // AccentDistortionBase  #F59E0B
		FLinearColor(FColor(0x34, 0xD3, 0x99)),  // AccentReverbBase      #34D399
		FLinearColor(FColor(0x94, 0xA3, 0xB8)),  // AccentUtilityBase     #94A3B8
		// Bypass
		FLinearColor(FColor(0xB3, 0x26, 0x26)),  // BypassActiveColor
		FLinearColor(FColor(0x66, 0x66, 0x66)),  // BypassInactiveColor
		// Brush Surface Colors
		FLinearColor(FColor(0x07, 0x07, 0x07)),  // SectionFill       -- deep inset
		FLinearColor(FColor(0x1A, 0x1A, 0x1A)),  // SectionOutline    -- subtle edge
		FLinearColor(FColor(0x16, 0x16, 0x16)),  // CardFill          -- recessed card
		FLinearColor(FColor(0x1F, 0x1F, 0x1F)),  // CardOutline       -- card edge
		FLinearColor(FColor(0x06, 0x06, 0x06)),  // InsetFill         -- very dark recessed
		FLinearColor(FColor(0x0D, 0x0D, 0x0D)),  // InsetOutline      -- dark edge
		FLinearColor(FColor(0x2E, 0x0A, 0x0A)),  // BypassActiveFill  -- dark red
		FLinearColor(FColor(0x66, 0x14, 0x14)),  // BypassActiveOutline -- red edge
		// Visualization
		FLinearColor(FColor(0x08, 0x08, 0x08)),  // VisualizationBackground
		FLinearColor(FColor(0x1A, 0x1A, 0x1A)),  // GridMinorColor
		FLinearColor(FColor(0x22, 0x22, 0x22)),  // GridMajorColor
		FLinearColor(FColor(0xA8, 0xA8, 0xB2)),  // VisualizationCurveColor
		// Parameter Type Colors
		FLinearColor(FColor(0x9B, 0x59, 0xB6)),  // ParamColorTime       -- violet/purple
		FLinearColor(FColor(0xE8, 0xA8, 0x38)),  // ParamColorGain       -- warm amber
		FLinearColor(FColor(0x44, 0xBB, 0x66)),  // ParamColorMix        -- green
		FLinearColor(FColor(0x29, 0x79, 0xFF)),  // ParamColorFrequency  -- royal blue
		FLinearColor(FColor(0x00, 0xCC, 0xDD)),  // ParamColorResonance  -- cyan
		FLinearColor(FColor(0xDD, 0x44, 0x44)),  // ParamColorDynamics   -- red
		FLinearColor(FColor(0xB3, 0xB3, 0xB3)),  // ParamColorDefault    -- neutral gray
		// Band Palette (8 hues)
		{
			FLinearColor(FColor(0x33, 0x85, 0xF5)),  // Blue
			FLinearColor(FColor(0x1A, 0xD9, 0xD9)),  // Cyan
			FLinearColor(FColor(0x33, 0xC7, 0x59)),  // Green
			FLinearColor(FColor(0xF2, 0xCC, 0x1A)),  // Yellow
			FLinearColor(FColor(0xFF, 0x6B, 0x00)),  // Orange
			FLinearColor(FColor(0xE6, 0x33, 0x4D)),  // Red
			FLinearColor(FColor(0xCC, 0x33, 0xCC)),  // Violet
			FLinearColor(FColor(0x80, 0xE6, 0x33)),  // Lime
		}
	};

	// ---- Light -- bright backgrounds, dark text --------------------------------
	// Retained for future theme-switching support. Currently unused.
	static const FAudioFXThemePalette LightPalette = {
		// Surface / Panel
		FLinearColor(FColor(0xF0, 0xF0, 0xF0)),  // WindowBackground
		FLinearColor(FColor(0xFA, 0xFA, 0xFA)),  // PanelBackground
		FLinearColor(FColor(0xF5, 0xF5, 0xF5)),  // SectionBackground
		FLinearColor(FColor(0xE8, 0xE8, 0xE8)),  // SurfaceLight
		FLinearColor(FColor(0xDD, 0xDD, 0xDD)),  // SurfaceRaised
		FLinearColor(FColor(0xEA, 0xEA, 0xEA)),  // SurfaceMid
		FLinearColor(FColor(0xD0, 0xD0, 0xD0)),  // DividerColor
		// Text
		FLinearColor(FColor(0x55, 0x55, 0x55)),  // HeaderTextColor
		FLinearColor(FColor(0x55, 0x55, 0x55)),  // LabelTextColor
		FLinearColor(FColor(0x22, 0x22, 0x22)),  // ValueTextColor
		FLinearColor(FColor(0x99, 0x99, 0x99)),  // TextDisabledColor
		FLinearColor(FColor(0x11, 0x11, 0x11)),  // TextAccentColor
		FLinearColor(FColor(0x22, 0x66, 0xDD)),  // FocusRingColor (darkened for contrast on light)
		// Accent
		FLinearColor(FColor(0xE0, 0x5A, 0x00)),  // AccentPrimary (darkened orange)
		FLinearColor(FColor(0x00, 0xAA, 0x88)),  // AccentSecondary (darkened teal)
		FLinearColor(FColor(0xDD, 0x22, 0x33)),  // AccentWarn
		FLinearColor(FColor(0x22, 0x66, 0xDD)),  // AccentInfo
		// Effect Category Accent Base (slightly darkened for light bg contrast)
		FLinearColor(FColor(0x1E, 0x80, 0xBD)),  // AccentDelayBase
		FLinearColor(FColor(0x8A, 0x40, 0xD9)),  // AccentFilterBase
		FLinearColor(FColor(0xDD, 0x3A, 0x55)),  // AccentDynamicsBase
		FLinearColor(FColor(0x18, 0xAA, 0xC8)),  // AccentModulationBase
		FLinearColor(FColor(0xD4, 0x84, 0x06)),  // AccentDistortionBase
		FLinearColor(FColor(0x26, 0xAA, 0x78)),  // AccentReverbBase
		FLinearColor(FColor(0x70, 0x80, 0x90)),  // AccentUtilityBase
		// Bypass
		FLinearColor(FColor(0xCC, 0x33, 0x33)),  // BypassActiveColor
		FLinearColor(FColor(0xAA, 0xAA, 0xAA)),  // BypassInactiveColor
		// Brush Surface Colors
		FLinearColor(FColor(0xE0, 0xE0, 0xE0)),  // SectionFill
		FLinearColor(FColor(0xC8, 0xC8, 0xC8)),  // SectionOutline
		FLinearColor(FColor(0xE8, 0xE8, 0xE8)),  // CardFill
		FLinearColor(FColor(0xCC, 0xCC, 0xCC)),  // CardOutline
		FLinearColor(FColor(0xF2, 0xF2, 0xF2)),  // InsetFill
		FLinearColor(FColor(0xDD, 0xDD, 0xDD)),  // InsetOutline
		FLinearColor(FColor(0xFF, 0xCC, 0xCC)),  // BypassActiveFill  -- light red
		FLinearColor(FColor(0xCC, 0x44, 0x44)),  // BypassActiveOutline -- red border
		// Visualization
		FLinearColor(FColor(0xF8, 0xF8, 0xF8)),  // VisualizationBackground
		FLinearColor(FColor(0xD0, 0xD0, 0xD0)),  // GridMinorColor
		FLinearColor(FColor(0xBB, 0xBB, 0xBB)),  // GridMajorColor
		FLinearColor(FColor(0x44, 0x44, 0x50)),  // VisualizationCurveColor (dark on light bg)
		// Parameter Type Colors (15-20% more saturated for light bg)
		FLinearColor(FColor(0x7D, 0x48, 0x98)),  // ParamColorTime
		FLinearColor(FColor(0xCC, 0x8E, 0x20)),  // ParamColorGain
		FLinearColor(FColor(0x2E, 0xA0, 0x50)),  // ParamColorMix
		FLinearColor(FColor(0x1F, 0x5F, 0xCC)),  // ParamColorFrequency
		FLinearColor(FColor(0x00, 0xA8, 0xBB)),  // ParamColorResonance
		FLinearColor(FColor(0xBB, 0x22, 0x22)),  // ParamColorDynamics
		FLinearColor(FColor(0x77, 0x77, 0x77)),  // ParamColorDefault
		// Band Palette (same hues, more saturated)
		{
			FLinearColor(FColor(0x1E, 0x6A, 0xE0)),  // Blue
			FLinearColor(FColor(0x0E, 0xB8, 0xB8)),  // Cyan
			FLinearColor(FColor(0x22, 0xA8, 0x44)),  // Green
			FLinearColor(FColor(0xD4, 0xAA, 0x0E)),  // Yellow
			FLinearColor(FColor(0xE0, 0x5A, 0x00)),  // Orange
			FLinearColor(FColor(0xCC, 0x22, 0x3A)),  // Red
			FLinearColor(FColor(0xAA, 0x22, 0xAA)),  // Violet
			FLinearColor(FColor(0x66, 0xCC, 0x22)),  // Lime
		}
	};

	// ---- Unreal (Professional) -- medium-gray, muted, enterprise ---------------
	// Retained for future theme-switching support. Currently unused.
	static const FAudioFXThemePalette UnrealPalette = {
		// Surface / Panel
		FLinearColor(FColor(0x30, 0x30, 0x30)),  // WindowBackground
		FLinearColor(FColor(0x38, 0x38, 0x38)),  // PanelBackground
		FLinearColor(FColor(0x35, 0x35, 0x35)),  // SectionBackground
		FLinearColor(FColor(0x40, 0x40, 0x40)),  // SurfaceLight
		FLinearColor(FColor(0x48, 0x48, 0x48)),  // SurfaceRaised
		FLinearColor(FColor(0x3C, 0x3C, 0x3C)),  // SurfaceMid
		FLinearColor(FColor(0x50, 0x50, 0x50)),  // DividerColor
		// Text
		FLinearColor(FColor(0x99, 0x99, 0x99)),  // HeaderTextColor
		FLinearColor(FColor(0x99, 0x99, 0x99)),  // LabelTextColor
		FLinearColor(FColor(0xCC, 0xCC, 0xCC)),  // ValueTextColor
		FLinearColor(FColor(0x66, 0x66, 0x66)),  // TextDisabledColor
		FLinearColor(FColor(0xDD, 0xDD, 0xDD)),  // TextAccentColor
		FLinearColor(FColor(0x55, 0x88, 0xCC)),  // FocusRingColor (muted blue)
		// Accent (desaturated ~40% toward gray)
		FLinearColor(FColor(0xBB, 0x7A, 0x44)),  // AccentPrimary (muted orange)
		FLinearColor(FColor(0x44, 0xA0, 0x88)),  // AccentSecondary (muted teal)
		FLinearColor(FColor(0xBB, 0x55, 0x60)),  // AccentWarn
		FLinearColor(FColor(0x55, 0x88, 0xCC)),  // AccentInfo
		// Effect Category Accent Base (desaturated ~40%)
		FLinearColor(FColor(0x4E, 0x88, 0xAA)),  // AccentDelayBase
		FLinearColor(FColor(0x88, 0x66, 0xBB)),  // AccentFilterBase
		FLinearColor(FColor(0xBB, 0x66, 0x77)),  // AccentDynamicsBase
		FLinearColor(FColor(0x55, 0xA0, 0xAA)),  // AccentModulationBase
		FLinearColor(FColor(0xBB, 0x8E, 0x44)),  // AccentDistortionBase
		FLinearColor(FColor(0x55, 0xA0, 0x77)),  // AccentReverbBase
		FLinearColor(FColor(0x80, 0x88, 0x90)),  // AccentUtilityBase
		// Bypass
		FLinearColor(FColor(0x99, 0x44, 0x44)),  // BypassActiveColor (muted red)
		FLinearColor(FColor(0x77, 0x77, 0x77)),  // BypassInactiveColor
		// Brush Surface Colors
		FLinearColor(FColor(0x28, 0x28, 0x28)),  // SectionFill
		FLinearColor(FColor(0x44, 0x44, 0x44)),  // SectionOutline
		FLinearColor(FColor(0x2C, 0x2C, 0x2C)),  // CardFill
		FLinearColor(FColor(0x3A, 0x3A, 0x3A)),  // CardOutline
		FLinearColor(FColor(0x22, 0x22, 0x22)),  // InsetFill
		FLinearColor(FColor(0x2E, 0x2E, 0x2E)),  // InsetOutline
		FLinearColor(FColor(0x60, 0x30, 0x30)),  // BypassActiveFill  -- muted dark red
		FLinearColor(FColor(0x88, 0x44, 0x44)),  // BypassActiveOutline -- muted red border
		// Visualization
		FLinearColor(FColor(0x28, 0x28, 0x28)),  // VisualizationBackground
		FLinearColor(FColor(0x48, 0x48, 0x48)),  // GridMinorColor
		FLinearColor(FColor(0x55, 0x55, 0x55)),  // GridMajorColor
		FLinearColor(FColor(0x99, 0x99, 0xA0)),  // VisualizationCurveColor (muted cool gray)
		// Parameter Type Colors (saturation x 0.6 -- muted pastels)
		FLinearColor(FColor(0x88, 0x66, 0xAA)),  // ParamColorTime       -- muted purple
		FLinearColor(FColor(0xBB, 0x99, 0x5E)),  // ParamColorGain
		FLinearColor(FColor(0x5E, 0x99, 0x70)),  // ParamColorMix
		FLinearColor(FColor(0x5E, 0x88, 0xCC)),  // ParamColorFrequency  -- muted royal blue
		FLinearColor(FColor(0x44, 0xAA, 0xB0)),  // ParamColorResonance
		FLinearColor(FColor(0xAA, 0x55, 0x55)),  // ParamColorDynamics
		FLinearColor(FColor(0x90, 0x90, 0x90)),  // ParamColorDefault
		// Band Palette (muted pastels -- same hue angles, saturation x 0.6)
		{
			FLinearColor(FColor(0x5E, 0x88, 0xBB)),  // Blue
			FLinearColor(FColor(0x44, 0xA0, 0xA0)),  // Cyan
			FLinearColor(FColor(0x55, 0xA0, 0x66)),  // Green
			FLinearColor(FColor(0xBB, 0xAA, 0x55)),  // Yellow
			FLinearColor(FColor(0xBB, 0x7A, 0x44)),  // Orange
			FLinearColor(FColor(0xAA, 0x55, 0x60)),  // Red
			FLinearColor(FColor(0x99, 0x55, 0x99)),  // Violet
			FLinearColor(FColor(0x77, 0xAA, 0x55)),  // Lime
		}
	};

	// Theme selection: Dark is always active.
	// LightPalette and UnrealPalette are retained for future theme-switching.
	// Suppress unused-variable warnings:
	(void)LightPalette;
	(void)UnrealPalette;
	return DarkPalette;
}

// ============================================================
// Widget Helpers
// ============================================================

TSharedRef<SWidget> FAudioFXTheme::MakeGroupLabel(const FText& InText)
{
	return SNew(STextBlock)
		.Text(InText)
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
		.ColorAndOpacity(GetTextDisabledColor())
		.Justification(ETextJustify::Center);
}

TSharedRef<SWidget> FAudioFXTheme::MakeVerticalDivider(float TopBottomPadding)
{
	// Use GetDividerBrush() so the color is palette-routed, not a stale static capture.
	return SNew(SBox)
		.WidthOverride(1.0f)
		.Padding(FMargin(0.0f, TopBottomPadding))
		[
			SNew(SBorder)
			.BorderImage(&GetDividerBrush())
		];
}
