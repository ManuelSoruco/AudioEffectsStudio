// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "AudioMaterialSlate/AudioMaterialSlateTypes.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Shared/Core/AudioFXThemeTypes.h"

/** Module-wide log category -- defined in AudioEffectsStudioModule.cpp */
AUDIOEFFECTSSTUDIO_API DECLARE_LOG_CATEGORY_EXTERN(LogAudioEffectsStudio, Log, All);

/**
 * Centralized theme for Audio Effects Studio.
 *
 * Provides the color palette, knob styles, brush factories, spacing constants,
 * value formatters, and utility helpers shared by all 27 effect editors.
 *
 * Three built-in theme palettes (Dark / Light / Unreal) are defined in
 * AudioFXTheme.cpp. Currently the Dark palette is always active; the others
 * are retained for future theme-switching support.
 *
 * All color getters route through GetPalette() and automatically reflect
 * the active theme.
 *
 * STATIC BRUSH NOTE: FSlateRoundedBoxBrush instances returned by Get*Brush()
 * are created once per process as static locals (lazy-initialized). Theme
 * changes take effect on the next editor window open -- brushes are not
 * hot-reloaded at runtime.
 */
class FAudioFXTheme
{
public:
	// ── Theme Palette Access ──────────────────────────────────────────────

	/**
	 * Returns the active color palette.
	 * Currently always returns the Dark palette. Light and Unreal palettes are
	 * defined in AudioFXTheme.cpp for future theme-switching support.
	 */
	static AUDIOEFFECTSSTUDIO_API const FAudioFXThemePalette& GetPalette();

	// ── Color Getters — Surface / Panel / Text ────────────────────────────
	//
	// IMPORTANT: FLinearColor values are in LINEAR space. To get an sRGB hex
	// value to display correctly, use FLinearColor(FColor(R, G, B)).
	// Putting sRGB float values directly into FLinearColor gives a washed-out look.

	// -- Window / Panel Colors ----------------------------------------
	static FLinearColor GetWindowBackground()  { return GetPalette().WindowBackground; }
	static FLinearColor GetPanelBackground()   { return GetPalette().PanelBackground; }
	static FLinearColor GetSectionBackground() { return GetPalette().SectionBackground; }
	static FLinearColor GetSurfaceLight()      { return GetPalette().SurfaceLight; }
	static FLinearColor GetSurfaceRaised()     { return GetPalette().SurfaceRaised; }
	static FLinearColor GetDividerColor()      { return GetPalette().DividerColor; }
	static FLinearColor GetHeaderTextColor()   { return GetPalette().HeaderTextColor; }
	static FLinearColor GetLabelTextColor()    { return GetPalette().LabelTextColor; }
	static FLinearColor GetValueTextColor()    { return GetPalette().ValueTextColor; }
	static FLinearColor GetTextDisabledColor() { return GetPalette().TextDisabledColor; }
	static FLinearColor GetTextAccentColor()   { return GetPalette().TextAccentColor; }
	static FLinearColor GetSurfaceMid()        { return GetPalette().SurfaceMid; }

	/** Keyboard focus ring color. */
	static FLinearColor GetFocusRingColor() { return GetPalette().FocusRingColor; }

	/** Darkens an accent color by 30% toward black for calmer effect category stripes. */
	static FLinearColor DarkenAccent(const FLinearColor& Color) { return FMath::Lerp(Color, FLinearColor::Black, 0.3f); }

	// -- Accent Colors ------------------------------------------------
	static FLinearColor GetAccentPrimary()   { return GetPalette().AccentPrimary; }
	static FLinearColor GetAccentSecondary() { return GetPalette().AccentSecondary; }
	static FLinearColor GetAccentWarn()      { return GetPalette().AccentWarn; }
	static FLinearColor GetAccentInfo()      { return GetPalette().AccentInfo; }

	// -- Effect Category Accent Colors (neon line at top of editor window) --
	static FLinearColor GetAccentDelay()      { return DarkenAccent(GetPalette().AccentDelayBase); }
	static FLinearColor GetAccentFilter()     { return DarkenAccent(GetPalette().AccentFilterBase); }
	static FLinearColor GetAccentDynamics()   { return DarkenAccent(GetPalette().AccentDynamicsBase); }
	static FLinearColor GetAccentModulation() { return DarkenAccent(GetPalette().AccentModulationBase); }
	static FLinearColor GetAccentDistortion() { return DarkenAccent(GetPalette().AccentDistortionBase); }
	static FLinearColor GetAccentReverb()     { return DarkenAccent(GetPalette().AccentReverbBase); }
	static FLinearColor GetAccentUtility()    { return DarkenAccent(GetPalette().AccentUtilityBase); }

	// -- Bypass -------------------------------------------------------
	static FLinearColor GetBypassActiveColor()   { return GetPalette().BypassActiveColor; }
	static FLinearColor GetBypassInactiveColor() { return GetPalette().BypassInactiveColor; }

	// ── Color Getters — Parameter Types, Knob Sizes, Visualization ──────────

	// -- Parameter Type Colors (full saturation) ----------------------
	/** Returns the arc-fill color for the given parameter type. */
	static FLinearColor GetParamColor(EAudioFXParamColor Type)
	{
		const auto& P = GetPalette();
		switch (Type)
		{
		case EAudioFXParamColor::Time:      return P.ParamColorTime;
		case EAudioFXParamColor::Gain:      return P.ParamColorGain;
		case EAudioFXParamColor::Mix:       return P.ParamColorMix;
		case EAudioFXParamColor::Frequency: return P.ParamColorFrequency;
		case EAudioFXParamColor::Resonance: return P.ParamColorResonance;
		case EAudioFXParamColor::Dynamics:  return P.ParamColorDynamics;
		default:                            return P.ParamColorDefault;
		}
	}

	// -- Knob Sizes (4-step FabFilter hierarchy) ----------------------
	/** Returns the pixel size (width = height) for the given knob size tier. */
	static float GetKnobSize(EAudioFXKnobSize Size)
	{
		switch (Size)
		{
		case EAudioFXKnobSize::Tertiary:  return 36.0f;
		case EAudioFXKnobSize::Secondary: return 48.0f;
		case EAudioFXKnobSize::Primary:   return 72.0f;
		case EAudioFXKnobSize::Hero:      return 120.0f;
		default:                          return 48.0f;
		}
	}

	/**
	 * Returns the global knob color saturation multiplier from UAudioEffectsStudioSettings.
	 * Implemented via a trampoline in AudioEffectsStudioSettings.cpp to avoid circular
	 * includes with UHT-generated code from AudioEffectsStudioSettings.h.
	 *
	 * 1.0 = identity, 0.0 = fully greyscale, 2.0 = vivid.
	 */
	static float GetKnobColorSaturation()
	{
		// Trampoline: body lives in AudioEffectsStudioSettings.cpp
		extern float AudioFXTheme_GetKnobColorSaturation_Impl();
		return AudioFXTheme_GetKnobColorSaturation_Impl();
	}

	// -- Visualization Colors -----------------------------------------
	/** Background for visualization areas (GR meter, frequency display, etc.) */
	static FLinearColor GetVisualizationBackground() { return GetPalette().VisualizationBackground; }
	/** Minor grid lines -- barely visible */
	static FLinearColor GetGridMinorColor()          { return GetPalette().GridMinorColor; }
	/** Major grid lines / 0 dB reference */
	static FLinearColor GetGridMajorColor()          { return GetPalette().GridMajorColor; }
	/**
	 * Primary visualization curve color -- elegant cool gray, not white.
	 * Inspired by FabFilter Pro-C 2 sum curve (#A8A8B2, ~67% brightness, slight blue tint).
	 * At 90% opacity this is clearly readable on #080808 without being harsh.
	 * NEVER use FLinearColor::White or bright (0.9x) for curves -- it is an eyesore.
	 */
	static FLinearColor GetVisualizationCurveColor() { return GetPalette().VisualizationCurveColor; }
	/** Soft glow pass for sum curves -- same hue, very low opacity. */
	static FLinearColor GetVisualizationGlowColor()
	{
		return GetPalette().VisualizationCurveColor.CopyWithNewOpacity(0.12f);
	}

	// ── Spacing & Corner Radius Constants ────────────────────────────────

	static constexpr float WindowPadding        = 12.0f;
	static constexpr float KnobHorizontalSpacing = 10.0f;
	static constexpr float SectionPadding       = 12.0f;
	static constexpr float SectionInnerPadding  = 8.0f;
	static constexpr float SectionGap           = 10.0f;
	static constexpr float ButtonGroupSpacing   = 2.0f;
	static constexpr float LabelToControlGap    = 8.0f;
	static constexpr float HeaderBarHeight      = 32.0f;
	static constexpr float PresetBarHeight      = 28.0f;

	static constexpr float PanelRadius   = 8.0f;
	static constexpr float SectionRadius = 6.0f;
	static constexpr float CardRadius    = 6.0f;
	static constexpr float ButtonRadius  = 4.0f;
	static constexpr float OutlineWidth  = 1.0f;

	// -- Arc Width Constants (stroke width per knob tier) -------------
	static constexpr float ArcWidthHero      = 4.0f;   ///< 120px Hero knobs
	static constexpr float ArcWidthPrimary   = 3.5f;   ///< 72px Primary knobs
	static constexpr float ArcWidthSecondary = 3.0f;   ///< 48px Secondary knobs
	static constexpr float ArcWidthTertiary  = 2.0f;   ///< 36px Tertiary knobs

	// ── Asset Dirty Marking ──────────────────────────────────────────────

	/**
	 * Call this BEFORE mutating any preset field from a knob/button/dropdown callback.
	 *
	 * Correct sequence:
	 * @code
	 *   FAudioFXTheme::MarkPresetDirty(P);   // snapshot pre-change state, mark package dirty
	 *   P->Settings.X = newValue;
	 *   P->SetSettings(P->Settings);
	 * @endcode
	 *
	 * Why a single function:
	 * - Modify() MUST come before mutation (undo system snapshots old state).
	 * - MarkPackageDirty() is what makes the asterisk appear in the Content Browser.
	 * - Without Modify() before mutation, Ctrl+Z does not work.
	 * - Centralising here means any future change (e.g. transaction labels) happens in one place.
	 *
	 * Accepts any UObject* (USoundEffectPreset*, USubmixEffectPreset*, etc.).
	 */
	static void MarkPresetDirty(UObject* Obj);

	// ── Widget Helpers — Labels, Dividers ────────────────────────────────

	/**
	 * Returns a dim 7pt label suitable for use below a knob group (e.g. "MODULATION", "MIX").
	 * Implemented in AudioFXTheme.cpp (requires STextBlock include).
	 */
	static AUDIOEFFECTSSTUDIO_API TSharedRef<SWidget> MakeGroupLabel(const FText& InText);

	/**
	 * Returns a 1px vertical divider for use between parameter groups in SHorizontalBox layouts.
	 * @param TopBottomPadding  Vertical padding applied to both ends of the divider line.
	 * Implemented in AudioFXTheme.cpp (requires SBox/SBorder includes).
	 */
	static AUDIOEFFECTSSTUDIO_API TSharedRef<SWidget> MakeVerticalDivider(float TopBottomPadding = 8.0f);

	// ── Rounded Box Brushes ───────────────────────────────────────────────
	// FSlateRoundedBoxBrush instances are static locals (lazy-initialized once per process).
	// Theme changes take effect on the next editor window open.

	/** Main window background -- large radius, no outline. */
	static const FSlateRoundedBoxBrush& GetPanelBrush()
	{
		static FSlateRoundedBoxBrush Brush(GetWindowBackground(), PanelRadius);
		return Brush;
	}

	/** Section box inset background -- theme-reactive, darker than window, subtle outline. */
	static const FSlateRoundedBoxBrush& GetSectionBrush()
	{
		static FSlateRoundedBoxBrush Brush(
			GetPalette().SectionFill, SectionRadius, GetPalette().SectionOutline, OutlineWidth);
		return Brush;
	}

	/** Card background (EQ bands, raised panels) -- theme-reactive. */
	static const FSlateRoundedBoxBrush& GetCardBrush()
	{
		static FSlateRoundedBoxBrush Brush(
			GetPalette().CardFill, CardRadius, GetPalette().CardOutline, OutlineWidth);
		return Brush;
	}

	/** Preset bar background -- rounded top corners only. Tinted by asset color at call site. */
	static const FSlateRoundedBoxBrush& GetPresetBarBrush()
	{
		static FSlateRoundedBoxBrush Brush(
			GetPanelBackground(),
			FVector4(4.0, 4.0, 0.0, 0.0),                  // Top-left, Top-right, Bottom-right, Bottom-left
			FLinearColor(FColor(0x24, 0x24, 0x24, 0x80)),   // Very subtle edge
			OutlineWidth);
		return Brush;
	}

	/** Button (inactive) -- pill-shaped, dim surface. */
	static const FSlateRoundedBoxBrush& GetButtonBrush()
	{
		static FSlateRoundedBoxBrush Brush(
			GetSurfaceLight(), ButtonRadius, GetDividerColor(), OutlineWidth);
		return Brush;
	}

	/** Button (active) -- pill-shaped, raised with lighter edge. Color tinting done at call site. */
	static const FSlateRoundedBoxBrush& GetButtonActiveBrush()
	{
		static FSlateRoundedBoxBrush Brush(
			GetSurfaceRaised(), ButtonRadius, FLinearColor(FColor(0x40, 0x40, 0x40)), OutlineWidth);
		return Brush;
	}

	/** Bypass button (inactive/processing) -- pill-shaped, neutral. */
	static const FSlateRoundedBoxBrush& GetBypassBrush()
	{
		static FSlateRoundedBoxBrush Brush(
			GetSurfaceLight(), ButtonRadius, GetDividerColor(), OutlineWidth);
		return Brush;
	}

	/** Bypass button (bypassed/active) -- theme-reactive red tint. */
	static const FSlateRoundedBoxBrush& GetBypassActiveBrush()
	{
		static FSlateRoundedBoxBrush Brush(
			GetPalette().BypassActiveFill, ButtonRadius, GetPalette().BypassActiveOutline, OutlineWidth);
		return Brush;
	}

	/** Inset panel for visualization areas -- theme-reactive darker with inner-shadow-like outline. */
	static const FSlateRoundedBoxBrush& GetInsetBrush()
	{
		static FSlateRoundedBoxBrush Brush(
			GetPalette().InsetFill, SectionRadius, GetPalette().InsetOutline, OutlineWidth);
		return Brush;
	}

	/** Action button (Add/+) -- neutral surface, muted outline. */
	static const FSlateRoundedBoxBrush& GetAddButtonBrush()
	{
		static FSlateRoundedBoxBrush Brush(
			GetSurfaceLight(), ButtonRadius, FLinearColor(FColor(0x33, 0x33, 0x33)), OutlineWidth);
		return Brush;
	}

	/** Action button (Add/+) hover -- slightly brighter, accent-tinted outline. */
	static const FSlateRoundedBoxBrush& GetAddButtonHoverBrush()
	{
		static FSlateRoundedBoxBrush Brush(
			FLinearColor(FColor(0x38, 0x38, 0x38)),
			ButtonRadius,
			GetAccentSecondary(),   // Teal outline on hover
			OutlineWidth);
		return Brush;
	}

	/** Remove/close button (x) -- subtle dark red tint. */
	static const FSlateRoundedBoxBrush& GetRemoveButtonBrush()
	{
		static FSlateRoundedBoxBrush Brush(
			FLinearColor(FColor(0x2E, 0x0D, 0x0D)),
			ButtonRadius,
			FLinearColor(FColor(0x4D, 0x14, 0x14)),
			OutlineWidth);
		return Brush;
	}

	/** Remove/close button hover -- brighter red. */
	static const FSlateRoundedBoxBrush& GetRemoveButtonHoverBrush()
	{
		static FSlateRoundedBoxBrush Brush(
			FLinearColor(FColor(0x59, 0x0F, 0x0F)),
			ButtonRadius,
			GetAccentWarn(),   // Bright red outline on hover
			OutlineWidth);
		return Brush;
	}

	/** Transparent brush -- used as "no background" placeholder. */
	static const FSlateRoundedBoxBrush& GetTransparentBrush()
	{
		static FSlateRoundedBoxBrush Brush(FLinearColor::Transparent, ButtonRadius);
		return Brush;
	}

	/** Card bottom shadow strip -- 1px dark line to simulate drop shadow below cards. */
	static const FSlateRoundedBoxBrush& GetCardShadowBrush()
	{
		static FSlateRoundedBoxBrush Brush(
			FLinearColor(0.0f, 0.0f, 0.0f, 0.5f),
			FVector4(0.0f, 0.0f, CardRadius, CardRadius));   // Rounded bottom corners only
		return Brush;
	}

	/** Inner top-shadow strip -- 1px dark line at top of content area for inset feel. */
	static const FSlateRoundedBoxBrush& GetInnerShadowBrush()
	{
		static FSlateRoundedBoxBrush Brush(FLinearColor(0.0f, 0.0f, 0.0f, 0.35f), 0.0f);
		return Brush;
	}

	/** Highlight line brush -- 1px top highlight for glass effect on preset bar. */
	static const FSlateRoundedBoxBrush& GetHighlightLineBrush()
	{
		static FSlateRoundedBoxBrush Brush(
			FLinearColor(1.0f, 1.0f, 1.0f, 0.06f),          // White at 6% -- subtle glass highlight
			FVector4(4.0, 4.0, 0.0, 0.0));                   // Rounded top corners to match preset bar
		return Brush;
	}

	// ── Knob & Button Style Factories ────────────────────────────────────

	/**
	 * Create a knob style tinted to the given parameter color,
	 * with optional influence from the effect's asset color.
	 *
	 * @param ParamType   The parameter type (determines arc fill color)
	 * @param AssetColor  The effect's GetPresetColor() -- subtly tints the knob body
	 * @param KnobSize    Desired knob size tier (default: Secondary = 48px)
	 */
	static FAudioMaterialKnobStyle MakeKnobStyle(
		EAudioFXParamColor ParamType,
		const FLinearColor& AssetColor,
		EAudioFXKnobSize KnobSize = EAudioFXKnobSize::Secondary)
	{
		FAudioMaterialKnobStyle Style = FAudioMaterialKnobStyle::GetDefault();

		// Phase 1: Resolve saturation-scaled parameter color.
		// Apply the global knob color saturation multiplier from settings.
		// GetKnobColorSaturation() is implemented via trampoline in AudioEffectsStudioSettings.cpp.
		//
		// Formula: boost BOTH saturation AND brightness in HSV space so knobs stand out
		// more without just shifting hue.
		//   SatMul = 1.0 -> identity (no change)
		//   SatMul = 2.0 -> fully vivid, ~40% brighter
		//   SatMul = 0.0 -> greyscale at original brightness
		const FLinearColor RawParamCol = GetParamColor(ParamType);
		const float SatMul = GetKnobColorSaturation();

		FLinearColor ParamCol;
		{
			FLinearColor HSV = RawParamCol.LinearRGBToHSV();
			HSV.G = FMath::Clamp(HSV.G * SatMul, 0.f, 1.f);
			// V: lerp toward 1.0 proportionally to the extra saturation added
			const float BrightnessBoost = FMath::Max(0.f, SatMul - 1.f) * 0.35f;
			HSV.B = FMath::Clamp(HSV.B + BrightnessBoost, 0.f, 1.f);
			ParamCol = HSV.HSVToLinearRGB();
			ParamCol.A = 1.f;
		}

		// Phase 2: Physical size for this tier.
		const float Size = GetKnobSize(KnobSize);

		// Phase 3: Knob body -- neutral dark base, subtly tinted by asset color (2%).
		const FLinearColor BaseDark = GetSurfaceLight();
		const FLinearColor KnobBody = FMath::Lerp(BaseDark, AssetColor, 0.02f);

		Style.SetKnobMainColor(KnobBody);
		Style.SetKnobAccentColor(FMath::Lerp(GetSurfaceRaised(), AssetColor, 0.02f));
		Style.SetKnobShadowColor(FLinearColor(FColor(0x05, 0x05, 0x05)));
		Style.SetSmoothBevelColor(FMath::Lerp(GetSurfaceRaised(), AssetColor, 0.03f));

		// Phase 4: Indicator dot and edge fill.
		Style.SetKnobIndicatorColor(ParamCol);
		Style.SetKnobEdgeFillColor(GetPanelBackground());

		// Phase 5: Arc bar track (unfilled background).
		Style.SetKnobBarColor(FLinearColor(FColor(0x16, 0x16, 0x16)));
		Style.SetKnobBarShadowColor(FLinearColor(FColor(0x08, 0x08, 0x08)));

		// Phase 6: Arc fill gradient -- desaturated -> full -> bright.
		// Min stop (tail of sweep): 50% saturation blend with mid-gray
		const FLinearColor DesatParamCol = FMath::Lerp(FLinearColor(FColor(0x4D, 0x4D, 0x4D)), ParamCol, 0.5f);

		// Max stop (tip of sweep): brightened +40% luminance + slight offset
		const FLinearColor BrightParamCol = FLinearColor(
			FMath::Min(ParamCol.R * 1.4f + 0.1f, 1.0f),
			FMath::Min(ParamCol.G * 1.4f + 0.1f, 1.0f),
			FMath::Min(ParamCol.B * 1.4f + 0.1f, 1.0f),
			1.0f);

		Style.SetKnobBarFillMinColor(DesatParamCol);
		Style.SetKnobFillMidColor(ParamCol);
		Style.SetKnobBarFillMaxColor(BrightParamCol);
		// Subtle warm tint from the effect's asset color (15% influence)
		Style.SetKnobBarFillTintColor(FMath::Lerp(FLinearColor::White, AssetColor, 0.15f));

		// Phase 7: Apply physical size.
		Style.DesiredSize = FVector2f(Size, Size);

		return Style;
	}

	/**
	 * Create a button style for toggle or enum-selection buttons.
	 *
	 * @param AssetColor  The effect's GetPresetColor() -- tints the button body
	 * @param bIsActive   Whether the button is currently active/selected
	 */
	static FAudioMaterialButtonStyle MakeButtonStyle(const FLinearColor& AssetColor, bool bIsActive = false)
	{
		FAudioMaterialButtonStyle Style = FAudioMaterialButtonStyle::GetDefault();

		const FLinearColor BaseDark = GetSurfaceLight();
		// Active: 45% blend for unmistakable active state; Inactive: 3% subtle tint
		Style.SetButtonMainColor(FMath::Lerp(BaseDark, AssetColor, bIsActive ? 0.45f : 0.03f));
		Style.SetButtonMainColorTint_1(FMath::Lerp(BaseDark, AssetColor, bIsActive ? 0.50f : 0.04f));
		Style.SetButtonMainColorTint_2(FMath::Lerp(BaseDark, AssetColor, bIsActive ? 0.40f : 0.02f));
		Style.SetButtonAccentColor(bIsActive ? AssetColor : GetTextDisabledColor());
		Style.SetButtonShadowColor(FLinearColor(FColor(0x08, 0x08, 0x0A)));
		// Inactive: no visible outline; Active: accent-colored outline
		Style.SetButtonUnpressedOutlineColor(bIsActive ? AssetColor : FLinearColor(FColor(0x1A, 0x1A, 0x1A)));
		Style.SetButtonPressedOutlineColor(bIsActive ? AssetColor : GetSurfaceRaised());

		Style.DesiredSize = FVector2f(60.0f, 24.0f);
		return Style;
	}

	// ── Tooltip, Frequency Mapping, Value Formatting ─────────────────────

	/**
	 * Build a standardized knob tooltip.
	 * The parameter name is displayed permanently above the knob via the label,
	 * so it is NOT repeated in the tooltip body.
	 *
	 * @param Min         Minimum real-unit value
	 * @param Max         Maximum real-unit value
	 * @param Units       Unit suffix string (e.g. "dB", "Hz", "ms")
	 * @param Description Human-readable description of what this parameter does
	 */
	static FText MakeKnobTooltip(float Min, float Max, const FString& Units, const FText& Description)
	{
		return Description;
	}

	/**
	 * Legacy overload -- kept for backward compatibility while old call sites migrate.
	 * Prefer the overload without ParamName.
	 */
	static FText MakeKnobTooltip(const FText& ParamName, float Min, float Max,
	                              const FString& Units, const FText& Description)
	{
		return FText::FromString(
			FString::Printf(TEXT("%s\n%s"), *ParamName.ToString(), *Description.ToString()));
	}

	// -- Logarithmic Frequency Mapping --------------------------------

	/**
	 * Convert a frequency in Hz to a normalised 0-1 value using logarithmic
	 * (perceptual) scaling. Used by EQ, Filter, Multiband, and Flexiverb editors.
	 *
	 * @param Hz     Frequency in Hertz
	 * @param MinHz  Minimum frequency of the range (default: 20 Hz)
	 * @param MaxHz  Maximum frequency of the range (default: 20000 Hz)
	 */
	static float LogFreqToNorm(float Hz, float MinHz = 20.f, float MaxHz = 20000.f)
	{
		const float Denom = FMath::Log2(MaxHz) - FMath::Log2(FMath::Max(MinHz, SMALL_NUMBER));
		if (FMath::IsNearlyZero(Denom)) { return 0.f; }
		return (FMath::Log2(FMath::Max(Hz, MinHz)) - FMath::Log2(FMath::Max(MinHz, SMALL_NUMBER))) / Denom;
	}

	/**
	 * Inverse of LogFreqToNorm -- normalised 0-1 back to Hz.
	 *
	 * @param N      Normalised value in [0, 1]
	 * @param MinHz  Minimum frequency of the range
	 * @param MaxHz  Maximum frequency of the range
	 */
	static float NormToLogFreq(float N, float MinHz = 20.f, float MaxHz = 20000.f)
	{
		const float LogMin = FMath::Log2(FMath::Max(MinHz, SMALL_NUMBER));
		const float LogMax = FMath::Log2(FMath::Max(MaxHz, SMALL_NUMBER));
		return FMath::Pow(2.f, LogMin + N * (LogMax - LogMin));
	}

	// -- Value Formatting --------------------------------------------

	/**
	 * Format a real-unit parameter value for display below a knob.
	 * Uses period decimals (industry standard), regardless of system locale.
	 *
	 * @param RealValue  The value in real units (not normalized 0-1)
	 * @param Format     The display format to use
	 * @param Suffix     Optional custom suffix (e.g. "bit") for EAudioFXValueFormat::Integer
	 */
	static FString FormatParameterValue(float RealValue, EAudioFXValueFormat Format, const FString& Suffix = TEXT(""))
	{
		switch (Format)
		{
		case EAudioFXValueFormat::Decibels:
			return FString::Printf(TEXT("%.1f dB"), RealValue);

		case EAudioFXValueFormat::Frequency:
			if (RealValue >= 1000.f)
				return FString::Printf(TEXT("%.2f kHz"), RealValue / 1000.f);
			return FString::Printf(TEXT("%.0f Hz"), RealValue);

		case EAudioFXValueFormat::Time:
			if (RealValue >= 1000.f)
				return FString::Printf(TEXT("%.2f s"), RealValue / 1000.f);
			return FString::Printf(TEXT("%.0f ms"), RealValue);

		case EAudioFXValueFormat::Ratio:
		{
			const int32 IntVal = FMath::RoundToInt32(RealValue);
			if (FMath::IsNearlyEqual(RealValue, (float)IntVal, 0.05f))
				return FString::Printf(TEXT("%d:1"), IntVal);
			return FString::Printf(TEXT("%.1f:1"), RealValue);
		}

		case EAudioFXValueFormat::Percentage:
			// RealValue is in [MinValue, MaxValue] -- for percentage knobs Min=0, Max=1.
			return FString::Printf(TEXT("%.0f%%"), RealValue * 100.f);

		case EAudioFXValueFormat::QFactor:
			return FString::Printf(TEXT("%.2f"), RealValue);

		case EAudioFXValueFormat::Integer:
			if (Suffix.IsEmpty())
				return FString::Printf(TEXT("%d"), FMath::RoundToInt32(RealValue));
			return FString::Printf(TEXT("%d %s"), FMath::RoundToInt32(RealValue), *Suffix);

		case EAudioFXValueFormat::BitDepth:
			return FString::Printf(TEXT("%d Bit"), FMath::RoundToInt32(RealValue));

		case EAudioFXValueFormat::Default:
		default:
			return FString::Printf(TEXT("%.2f"), RealValue);
		}
	}

	// ── Preset Name Cleanup ───────────────────────────────────────────────

	/**
	 * Clean up a UE asset name for display as a preset name.
	 * Strips common SourceEffect/SubmixEffect prefixes and trailing _N suffixes.
	 * Returns "Default" for auto-generated names that reduce to nothing.
	 *
	 * @param RawName  Raw asset name from UObject::GetName()
	 */
	static FString CleanPresetName(const FString& RawName)
	{
		FString Name = RawName;

		Name.RemoveFromStart(TEXT("SourceEffect"));
		Name.RemoveFromStart(TEXT("SubmixEffect"));

		// Strip trailing _N (auto-generated instance number) before stripping "Preset"
		// so "DynamicsProcessorPreset_0" -> "DynamicsProcessorPreset" -> "DynamicsProcessor"
		int32 UnderscoreIdx;
		if (Name.FindLastChar(TEXT('_'), UnderscoreIdx) && UnderscoreIdx > 0)
		{
			const FString TrailingSuffix = Name.Mid(UnderscoreIdx + 1);
			bool bAllDigits = !TrailingSuffix.IsEmpty();
			for (TCHAR Ch : TrailingSuffix)
			{
				if (!FChar::IsDigit(Ch)) { bAllDigits = false; break; }
			}
			if (bAllDigits) { Name = Name.Left(UnderscoreIdx); }
		}

		Name.RemoveFromEnd(TEXT("Preset"));
		Name = Name.TrimStartAndEnd();

		return Name.IsEmpty() ? TEXT("Default") : Name;
	}

	// ============================================================
	// Band / Tap Color Palette
	// ============================================================

	/** Number of distinct hues in the shared band/tap color palette. */
	static constexpr int32 NumBandPaletteColors = 8;

	/**
	 * Shared 8-color palette used by EQ, Multiband, and TapDelay card racks.
	 * Returns a pointer to the active theme's band palette array -- never allocates.
	 */
	static const FLinearColor* GetBandPalette() { return GetPalette().BandPalette; }

	// -- Divider Brush ------------------------------------------------
	/**
	 * Returns a shared 0-radius rounded box brush in the divider color.
	 * Replaces per-file static FSlateRoundedBoxBrush DivBrush(...) declarations.
	 */
	static const FSlateRoundedBoxBrush& GetDividerBrush()
	{
		static FSlateRoundedBoxBrush Brush(GetDividerColor(), 0.0f);
		return Brush;
	}
};
