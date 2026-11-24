// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "AudioEffectsStudioSettings.h"
#include "GenericPlatform/GenericApplication.h"
#include "Framework/Application/SlateApplication.h"

// ── FAudioFXTheme::GetKnobColorSaturation() ─────────────────────────────────
// Implemented here (not in AudioFXTheme.h/.cpp) to avoid circular includes:
// AudioFXTheme.h pulls in AudioMaterialSlateTypes.h which conflicts with the
// UHT-generated code emitted by GENERATED_BODY() in AudioEffectsStudioSettings.h.
//
// We use a free-function trampoline visible to the linker. AudioFXTheme.h
// declares static float GetKnobColorSaturation(), but the linker links to
// this trampoline which is exported from this translation unit.
// The forward declaration of FAudioFXTheme here would cause ODR issues —
// instead we simply provide the function symbol directly via a non-member
// wrapper and make GetKnobColorSaturation() a non-inline static that is
// resolved at link time.
//
// LINKER NOTE: FAudioFXTheme is a non-UObject struct defined entirely in the
// header with inline statics. GetKnobColorSaturation() is the ONLY non-inline
// static — its symbol must be emitted exactly once.
AUDIOEFFECTSSTUDIO_API float AudioFXTheme_GetKnobColorSaturation_Impl()
{
	if (const UAudioEffectsStudioSettings* S = GetDefault<UAudioEffectsStudioSettings>())
	{
		return FMath::Clamp(S->KnobColorSaturation, 0.f, 4.f);
	}
	return 1.f;
}

// ─────────────────────────────────────────────────────────────────────────────

UAudioEffectsStudioSettings::UAudioEffectsStudioSettings()
{
	// Set a sensible default on first use so the settings UI shows 1.0 or 1.5
	// instead of the confusing "0.0". We check the CDO path (Config value not
	// yet written) and leave it alone if the user has already saved a value.
	if (UIScaleFactor <= 0.0f)
	{
		// Auto-detect: 4K primary display → 1.5, otherwise 1.0.
		float Detected = 1.0f;
		if (FSlateApplication::IsInitialized())
		{
			FDisplayMetrics Metrics;
			FDisplayMetrics::RebuildDisplayMetrics(Metrics);
			if (Metrics.PrimaryDisplayHeight >= 2160)
			{
				Detected = 1.5f;
			}
		}
		UIScaleFactor = Detected;
	}
}

UAudioEffectsStudioSettings* UAudioEffectsStudioSettings::Get()
{
	return GetMutableDefault<UAudioEffectsStudioSettings>();
}

float UAudioEffectsStudioSettings::GetEffectiveScale() const
{
	// UIScaleFactor is always > 0 after construction. Return it directly.
	return FMath::Max(UIScaleFactor, 0.5f);
}

FVector2D UAudioEffectsStudioSettings::ScaleWindowSize(FVector2D BaseSize) const
{
	const float ScaleFactor = GetEffectiveScale();
	return FVector2D(FMath::RoundToFloat(BaseSize.X * ScaleFactor),
	                 FMath::RoundToFloat(BaseSize.Y * ScaleFactor));
}

// Static delegate definition
UAudioEffectsStudioSettings::FOnUIScaleChanged UAudioEffectsStudioSettings::OnUIScaleChanged;

#if WITH_EDITOR
void UAudioEffectsStudioSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropName = PropertyChangedEvent.GetPropertyName();
	if (PropName == GET_MEMBER_NAME_CHECKED(UAudioEffectsStudioSettings, UIScaleFactor))
	{
		OnUIScaleChanged.Broadcast(GetEffectiveScale());
	}
}
#endif
