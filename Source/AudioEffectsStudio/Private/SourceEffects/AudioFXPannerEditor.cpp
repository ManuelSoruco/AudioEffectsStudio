// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

/**
 * FAudioFXPannerEditor
 *
 * Knob-based editor skin for USourceEffectPannerPreset.
 * Vertical layout: PanDisplay on top, Pan (hero) + Spread knobs below.
 */

#include "SourceEffects/AudioFXPannerEditor.h"
#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Widgets/SAudioFXLabeledKnob.h"

#include "SourceEffects/SourceEffectPanner.h"
#include "Shared/Visualizers/SAudioFXPanDisplay.h"
#include "AudioWidgetsEnums.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AudioFXPanner"

TSharedRef<SWidget> FAudioFXPannerEditor::CreateEffectWidget(USoundEffectPreset* InPreset)
{
	USourceEffectPannerPreset* Preset = Cast<USourceEffectPannerPreset>(InPreset);
	if (!Preset)
	{
		UE_LOG(LogAudioEffectsStudio, Warning,
			TEXT("AudioEffectsStudio: Cast to preset type failed for '%s'. Check asset registration."),
			InPreset ? *InPreset->GetClass()->GetName() : TEXT("null"));
		return FAudioFXEffectEditor::CreateEffectWidget(InPreset);
	}

	const FLinearColor AssetCol = GetAssetColor();
	TWeakObjectPtr<USourceEffectPannerPreset> WeakPreset = Preset;

	// Pan = Hero Blue, Spread = Secondary Purple
	const FAudioMaterialKnobStyle* PanStyle = StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Frequency, AssetCol, EAudioFXKnobSize::Hero));
	const FAudioMaterialKnobStyle* SpreadStyle = StoreKnobStyle(
		FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Time, AssetCol, EAudioFXKnobSize::Secondary));
	return SNew(SBox)
		.Padding(FMargin(FAudioFXTheme::WindowPadding, FAudioFXTheme::SectionPadding))
	[
		SNew(SVerticalBox)

		// ── Pan Display (top, fill-width) ────────────────────────
		+ SVerticalBox::Slot()
			.FillHeight(1.f)
			.HAlign(HAlign_Fill)
			.Padding(0.f, 0.f, 0.f, FAudioFXTheme::SectionGap)
		[
			SNew(SBox)
				.MinDesiredHeight(100.f)
			[
				SNew(SOverlay)

				+ SOverlay::Slot()
				[
					SNew(SAudioFXPanDisplay)
					.AssetColor(AssetCol)
					.Pan_Lambda([WeakPreset]() -> float
					{
						if (auto* Preset = WeakPreset.Get()) { return Preset->Settings.Pan; }
						return 0.f;
					})
					.Spread_Lambda([WeakPreset]() -> float
					{
						if (auto* Preset = WeakPreset.Get()) { return Preset->Settings.Spread; }
						return 0.f;
					})
				]

				+ SOverlay::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					.Padding(4.f, 0.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("L", "L"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
					.ColorAndOpacity(FAudioFXTheme::GetTextDisabledColor())
				]

				+ SOverlay::Slot()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Center)
					.Padding(0.f, 0.f, 4.f, 0.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("R", "R"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
					.ColorAndOpacity(FAudioFXTheme::GetTextDisabledColor())
				]
			]
		]

		// ── Knob row: [Spread mirror spacer] [Pan hero center] [Spread right] ───
		+ SVerticalBox::Slot()
			.AutoHeight()
		[
			SNew(SHorizontalBox)

			// Left spacer — matches Spread slot width exactly to keep Pan centered
			+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
			[
				// Invisible spacer widget — same slot params as Spread
				SNew(SBox)
			]

			// Pan — hero, center
			+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
			[
				SNew(SAudioFXLabeledKnob)
				.ParamName(LOCTEXT("PanLbl", "Pan"))
				.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
					LOCTEXT("PanTipName", "Pan"),
					-1.f, 1.f, TEXT(""),
					LOCTEXT("PanTip",
						"The pan of the source. -1.0 means left, 0.0 means center, 1.0 means right.\n"
						"AudioFactory Note: Uses equal power (cosine) panning law for"
						" smooth level transitions across the stereo field.")))
				.Value_Lambda([WeakPreset]() -> float
				{
					if (auto* Preset = WeakPreset.Get())
					{
						return FMath::GetMappedRangeValueClamped(FVector2D(-1.f,1.f), FVector2D(0.f,1.f), Preset->Settings.Pan);
					}
					return 0.5f;
				})
				.Style(PanStyle)
				.AudioUnitsValueType(EAudioUnitsValueType::Linear)
				.ValueFormat(EAudioFXValueFormat::Default)
				.DefaultValue(0.5f)
				.MinValue(-1.f)
				.MaxValue(1.f)
				.OnValueChanged_Lambda([this, WeakPreset](float NewNorm)
				{
					if (auto* Preset = WeakPreset.Get())
					{
						MarkDirty.ExecuteIfBound();
						Preset->Settings.Pan = FMath::GetMappedRangeValueClamped(FVector2D(0.f,1.f), FVector2D(-1.f,1.f), NewNorm);
						Preset->SetSettings(Preset->Settings);
					}
				})
			]

			// Spread — secondary, right
			+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(FAudioFXTheme::KnobHorizontalSpacing, 0.f)
			[
				SNew(SAudioFXLabeledKnob)
				.ParamName(LOCTEXT("SpreadLbl", "Spread"))
				.KnobToolTipText(FAudioFXTheme::MakeKnobTooltip(
					LOCTEXT("SpreadTipName", "Spread"),
					-1.f, 1.f, TEXT(""),
					LOCTEXT("SpreadTip",
						"The spread of the source. 1.0 means full stereo, 0.0 means mono,"
						" -1.0 inverts channels.\n"
						"AudioFactory Note: At 0.0, left and right channels are summed to mono."
						" Negative values invert the stereo image.")))
				.Value_Lambda([WeakPreset]() -> float
				{
					if (auto* Preset = WeakPreset.Get())
					{
						return FMath::GetMappedRangeValueClamped(FVector2D(-1.f,1.f), FVector2D(0.f,1.f), Preset->Settings.Spread);
					}
					return 0.5f;
				})
				.Style(SpreadStyle)
				.AudioUnitsValueType(EAudioUnitsValueType::Linear)
				.ValueFormat(EAudioFXValueFormat::Default)
				.DefaultValue(1.0f)
				.MinValue(-1.f)
				.MaxValue(1.f)
				.OnValueChanged_Lambda([this, WeakPreset](float NewNorm)
				{
					if (auto* Preset = WeakPreset.Get())
					{
						MarkDirty.ExecuteIfBound();
						Preset->Settings.Spread = FMath::GetMappedRangeValueClamped(FVector2D(0.f,1.f), FVector2D(-1.f,1.f), NewNorm);
						Preset->SetSettings(Preset->Settings);
					}
				})
			]
		]
	];
}

#undef LOCTEXT_NAMESPACE
