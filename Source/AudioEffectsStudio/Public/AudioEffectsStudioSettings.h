// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AudioEffectsStudioSettings.generated.h"

/**
 * UAudioEffectsStudioSettings
 *
 * Editor-only developer settings for the Audio Effects Studio plugin.
 * Open via the gear icon in any effect editor window.
 */
// NOTE (for maintainers): UDeveloperSettings is intentional here.
// AudioEffectsStudio is an EDITOR-ONLY plugin — it never runs in a packaged game.
// Config saved to EditorPerProjectUserSettings.ini.
UCLASS(Config=EditorPerProjectUserSettings, meta=(DisplayName="Audio Effects Studio"))
class AUDIOEFFECTSSTUDIO_API UAudioEffectsStudioSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UAudioEffectsStudioSettings();

	/** Singleton accessor. */
	static UAudioEffectsStudioSettings* Get();

	// ── UI Scaling ─────────────────────────────────────────────────────────

	/**
	 * Zoom multiplier applied to the initial size of every effect editor window.
	 * 1.0 = 100% (normal)   1.5 = 150% (recommended for 4K displays).
	 * Takes effect the next time an effect editor is opened.
	 */
	UPROPERTY(Config, EditAnywhere, Category="UI", meta=(
		DisplayName="Editor Window Scale",
		ClampMin=0.5, ClampMax=3.0,
		UIMin=0.5, UIMax=3.0,
		Delta=0.05))
	float UIScaleFactor = 1.0f;   // Set to 1.5 on first use if 4K display detected

	// ── Knob Appearance ────────────────────────────────────────────────────

	/**
	 * Saturation multiplier for all knob arc colors.
	 * 1.0 = default colors.  Increase to make knobs more vivid; decrease for a more muted look.
	 */
	UPROPERTY(Config, EditAnywhere, Category="UI", meta=(
		DisplayName="Knob Color Saturation",
		ClampMin=0.0, ClampMax=2.0,
		UIMin=0.0, UIMax=2.0,
		Delta=0.05))
	float KnobColorSaturation = 1.0f;

	/**
	 * Returns the effective scale factor.
	 * Auto-detects from display resolution and saves if not yet initialised.
	 */
	float GetEffectiveScale() const;


	/** Multiply a base window size by the effective scale. */
	FVector2D ScaleWindowSize(FVector2D BaseSize) const;

	/**
	 * Delegate fired when UIScaleFactor changes (via PostEditChangeProperty).
	 * Subscribe to this to resize open windows in real-time when the user changes
	 * the scale slider in settings. Passes the new scale value.
	 */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnUIScaleChanged, float /*NewScale*/);
	static FOnUIScaleChanged OnUIScaleChanged;

	// UDeveloperSettings interface
	virtual FName GetCategoryName() const override { return FName("Plugins"); }

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
