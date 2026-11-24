// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Shared/Core/AudioFXEffectEditor.h"
#include "Shared/Visualizers/SAudioFXFilterCurve.h"

/**
 * AudioFXFilterUI
 *
 * Shared UI builder for Filter editors (Source + Submix).
 * Both variants map their different enum types into lambdas.
 */

DECLARE_DELEGATE_RetVal(EAudioFXFilterCurveType, FFilterGetCurveType);
DECLARE_DELEGATE_RetVal(float, FFilterGetFloat);
DECLARE_DELEGATE_OneParam(FFilterSetFloat, float);
DECLARE_DELEGATE_RetVal(int32, FFilterGetInt);
DECLARE_DELEGATE_OneParam(FFilterSetInt, int32);

struct FFilterUIParams
{
	// ── Filter curve ─────────────────────────────────────────────
	FFilterGetCurveType  GetCurveType;
	FFilterGetFloat      GetCutoffHz;
	FFilterGetFloat      GetQ;

	// ── Circuit dropdown ─────────────────────────────────────────
	TArray<FText>  CircuitOptions;
	FFilterGetInt  GetCircuit;
	FFilterSetInt  SetCircuit;

	// ── Type dropdown ────────────────────────────────────────────
	TArray<FText>  TypeOptions;
	FFilterGetInt  GetType;
	FFilterSetInt  SetType;

	// ── Cutoff knob ──────────────────────────────────────────────
	FFilterGetFloat  GetCutoff;
	FFilterSetFloat  SetCutoff;
	float FreqMin = 20.f;
	float FreqMax = 20000.f;

	// ── Resonance knob ───────────────────────────────────────────
	FFilterGetFloat  GetResonance;
	FFilterSetFloat  SetResonance;
	float QMin = 0.5f;
	float QMax = 10.f;
};

// ── Builder ──────────────────────────────────────────────────────────────

namespace AudioFXFilterUI
{
	AUDIOEFFECTSSTUDIO_API TSharedRef<SWidget> BuildWidget(
		FAudioFXEffectEditor& Editor,
		const FFilterUIParams& Params,
		const FLinearColor& AssetCol);
}
