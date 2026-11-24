// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"

/**
 * @file AudioFXThemeTypes.h
 * @brief Lightweight types used by FAudioFXTheme — enums, palette struct.
 *
 * Separated from AudioFXTheme.h so that code which only needs to reference
 * EAudioFXKnobSize, EAudioFXParamColor, EAudioFXValueFormat, or FAudioFXThemePalette
 * does not pull in the full FAudioFXTheme class (and its heavy includes).
 */

// ============================================================
// Enums
// ============================================================

/**
 * Parameter color types used by the knob arc fill.
 * Each semantic category maps to a distinct hue so parameters
 * are instantly recognizable across all 27 effect editors.
 */
enum class EAudioFXParamColor : uint8
{
	/** Attack, Release, Delay, PreDelay, DecayTime, LookAhead */
	Time,

	/** InputGain, OutputGain, Wet/Dry Volume, GainDb */
	Gain,

	/** WetLevel, DryLevel, Feedback, Spread, Depth, Mix, Amount */
	Mix,

	/** CutoffFrequency, Frequency, SampleRate */
	Frequency,

	/** FilterQ, Resonance, Bandwidth */
	Resonance,

	/** Threshold, Ratio, Knee */
	Dynamics,
};

/**
 * Knob size tiers — FabFilter-inspired 4-step hierarchy.
 *
 * Hero    (120px) — THE defining parameter. Only 1 per effect (rarely 2).
 * Primary  (72px) — Core parameters the user adjusts frequently. 2-4 per effect.
 * Secondary(48px) — Supporting parameters: Wet/Dry, Feedback, Q, etc.
 * Tertiary (36px) — Rarely-touched parameters: LookAhead, Knee, etc.
 *
 * Ratio:  Hero:Primary = 1.67,  Primary:Secondary = 1.50,  Secondary:Tertiary = 1.33
 *         Hero:Tertiary = 3.33 (matches FabFilter's hero:secondary ratio)
 */
enum class EAudioFXKnobSize : uint8
{
	/** 36x36 — rarely-touched/advanced params (LookAhead, Knee) */
	Tertiary,

	/** 48x48 — supporting parameters (Wet/Dry, Feedback, Q) */
	Secondary,

	/** 72x72 — core parameters adjusted frequently (Ratio, Attack, Release) */
	Primary,

	/** 120x120 — THE defining parameter of the effect (Threshold, Cutoff, Delay Time) */
	Hero,
};

/**
 * Extended value formatting types for the text displayed below a knob.
 * Supplements EAudioUnitsValueType with audio-specific formats.
 */
enum class EAudioFXValueFormat : uint8
{
	/** Use the EAudioUnitsValueType from the knob (legacy default) */
	Default,

	/** Display as dB: "-12.0 dB" */
	Decibels,

	/** Display as Hz/kHz: "440 Hz" or "1.20 kHz" */
	Frequency,

	/** Display as ms/s: "10 ms" or "1.50 s" */
	Time,

	/** Display as ratio: "4:1" */
	Ratio,

	/** Display as percentage: "50%" */
	Percentage,

	/** Display as Q with 2 decimals: "1.41" */
	QFactor,

	/** Display as integer with optional suffix: "16" */
	Integer,

	/** Display as bit depth: "8 Bit" */
	BitDepth,
};

// ============================================================
// Theme Palette Struct
// ============================================================

/**
 * All theme-variant colors collected in a single plain struct.
 * NOT a USTRUCT — no reflection needed. One static instance per palette preset.
 * Populated by FAudioFXTheme::GetPalette() and accessed via FAudioFXTheme color getters.
 */
struct FAudioFXThemePalette
{
	// -- Surface / Panel --------------------------------------------------
	FLinearColor WindowBackground;
	FLinearColor PanelBackground;
	FLinearColor SectionBackground;
	FLinearColor SurfaceLight;
	FLinearColor SurfaceRaised;
	FLinearColor SurfaceMid;
	FLinearColor DividerColor;

	// -- Text -------------------------------------------------------------
	FLinearColor HeaderTextColor;
	FLinearColor LabelTextColor;
	FLinearColor ValueTextColor;
	FLinearColor TextDisabledColor;
	FLinearColor TextAccentColor;
	FLinearColor FocusRingColor;

	// -- Accent -----------------------------------------------------------
	FLinearColor AccentPrimary;
	FLinearColor AccentSecondary;
	FLinearColor AccentWarn;
	FLinearColor AccentInfo;

	// -- Effect Category Accent Base Colors --------------------------------
	// DarkenAccent() is applied by the FAudioFXTheme getter methods.
	FLinearColor AccentDelayBase;
	FLinearColor AccentFilterBase;
	FLinearColor AccentDynamicsBase;
	FLinearColor AccentModulationBase;
	FLinearColor AccentDistortionBase;
	FLinearColor AccentReverbBase;
	FLinearColor AccentUtilityBase;

	// -- Bypass -----------------------------------------------------------
	FLinearColor BypassActiveColor;
	FLinearColor BypassInactiveColor;

	// -- Brush Surface Colors ---------------------------------------------
	// Used by FAudioFXTheme::Get*Brush() factory methods.
	FLinearColor SectionFill;          ///< SAudioFXSectionBox background
	FLinearColor SectionOutline;       ///< SAudioFXSectionBox border
	FLinearColor CardFill;             ///< Card (EQ band) background
	FLinearColor CardOutline;          ///< Card border
	FLinearColor InsetFill;            ///< Visualization inset background
	FLinearColor InsetOutline;         ///< Visualization inset border
	FLinearColor BypassActiveFill;     ///< Bypass button active background
	FLinearColor BypassActiveOutline;  ///< Bypass button active border

	// -- Visualization ----------------------------------------------------
	FLinearColor VisualizationBackground;
	FLinearColor GridMinorColor;
	FLinearColor GridMajorColor;
	FLinearColor VisualizationCurveColor;

	// -- Parameter Type Colors --------------------------------------------
	FLinearColor ParamColorTime;
	FLinearColor ParamColorGain;
	FLinearColor ParamColorMix;
	FLinearColor ParamColorFrequency;
	FLinearColor ParamColorResonance;
	FLinearColor ParamColorDynamics;
	FLinearColor ParamColorDefault;

	// -- Band Palette (8 hues for EQ / Multiband / TapDelay) -------------
	static constexpr int32 NumBandColors = 8;
	FLinearColor BandPalette[NumBandColors];
};
