// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Widgets/SAudioFXModulationOverlay.h"

#include "AudioWidgetsEnums.h"
#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Widgets/SAudioFXDropdown.h"
#include "Shared/Widgets/SAudioFXButton.h"
#include "Shared/Widgets/SAudioFXLabeledKnob.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AudioFXModulationOverlay"

namespace AudioFXModulationOverlay
{
	// ── Defaults ──────────────────────────────────────────────────────────────
	static constexpr float DefaultMappingAmount = 0.5f;   // Normalized 0.5 → real 0.0 (center)

	// ── Ordered destination list ───────────────────────────────────────────────
	// This ordering defines the sort key used by RebuildRows and controls which slot
	// TryGetNextDestination picks first for newly added mappings.
	static const TArray<ESourceEffectMotionFilterModDestination> Destinations =
	{
		ESourceEffectMotionFilterModDestination::FilterACutoffFrequency,
		ESourceEffectMotionFilterModDestination::FilterAResonance,
		ESourceEffectMotionFilterModDestination::FilterAOutputVolumeDB,
		ESourceEffectMotionFilterModDestination::FilterBCutoffFrequency,
		ESourceEffectMotionFilterModDestination::FilterBResonance,
		ESourceEffectMotionFilterModDestination::FilterBOutputVolumeDB,
		ESourceEffectMotionFilterModDestination::FilterMix
	};

	static const TArray<FText> DestinationLabels =
	{
		LOCTEXT("DestFAFreq", "Filter A Cutoff"),
		LOCTEXT("DestFARes", "Filter A Resonance"),
		LOCTEXT("DestFAVol", "Filter A Output"),
		LOCTEXT("DestFBFreq", "Filter B Cutoff"),
		LOCTEXT("DestFBRes", "Filter B Resonance"),
		LOCTEXT("DestFBVol", "Filter B Output"),
		LOCTEXT("DestMix", "Filter Mix")
	};

	// ── Ordered source list ────────────────────────────────────────────────────
	static const TArray<ESourceEffectMotionFilterModSource> Sources =
	{
		ESourceEffectMotionFilterModSource::DistanceFromListener,
		ESourceEffectMotionFilterModSource::SpeedRelativeToListener,
		ESourceEffectMotionFilterModSource::SpeedOfSourceEmitter,
		ESourceEffectMotionFilterModSource::SpeedOfListener,
		ESourceEffectMotionFilterModSource::SpeedOfAngleDelta
	};

	static const TArray<FText> SourceLabels =
	{
		LOCTEXT("SrcDistance", "Distance"),
		LOCTEXT("SrcRelSpeed", "Relative Speed"),
		LOCTEXT("SrcEmitterSpeed", "Emitter Speed"),
		LOCTEXT("SrcListenerSpeed", "Listener Speed"),
		LOCTEXT("SrcAngleDelta", "Angle Delta")
	};

	static int32 FindDestinationIndex(const ESourceEffectMotionFilterModDestination Destination)
	{
		return Destinations.IndexOfByKey(Destination);
	}

	static int32 FindSourceIndex(const ESourceEffectMotionFilterModSource Source)
	{
		return Sources.IndexOfByKey(Source);
	}
}

void SAudioFXModulationOverlay::Construct(const FArguments& InArgs)
{
	// ── Step 1: bind arguments ─────────────────────────────────────────────
	Preset = InArgs._Preset;
	AssetColor = InArgs._AssetColor;
	OnCloseRequested = InArgs._OnCloseRequested;
	AmountKnobStyle = FAudioFXTheme::MakeKnobStyle(EAudioFXParamColor::Mix, AssetColor, EAudioFXKnobSize::Secondary);

	ChildSlot
	[
		SNew(SOverlay)

		// ── Layer 0: semi-transparent dark full-screen backdrop ────────────
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.55f))
		]

		// ── Layer 1: centered blurred floating panel ───────────────────────
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(760.f)
			.HeightOverride(520.f)
			[
				SNew(SBackgroundBlur)
				.BlurStrength(6.f)
				[
					SNew(SBorder)
					.BorderImage(&FAudioFXTheme::GetCardBrush())
					.Padding(FMargin(10.f))
					[
						SNew(SVerticalBox)

						// ── Title row: "MODULATION MAPPINGS" label + Close button ──
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("OverlayTitle", "MODULATION MAPPINGS"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
								.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
							]
							+ SHorizontalBox::Slot().AutoWidth()
							[
							SNew(SAudioFXButton)
							.Options(TArray<FText>{ LOCTEXT("CloseOverlay", "Close") })
							.AssetColor(FAudioFXTheme::GetAccentWarn())
							.SelectedIndex(-1)
							.OnSelectionChanged_Lambda([this](int32)
							{
								OnCloseRequested.ExecuteIfBound();
							})
							]
						]
						// ── Scrollable mapping rows ──────────────────────────────
						+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 8.f, 0.f, 8.f)
						[
							SNew(SScrollBox)
							+ SScrollBox::Slot()
							[
								SAssignNew(RowsBox, SVerticalBox)
							]
						]
						// ── "Add Mapping" button at the bottom ──────────────────
						+ SVerticalBox::Slot().AutoHeight()
						[
						SNew(SAudioFXButton)
						.Options(TArray<FText>{ LOCTEXT("AddMapping", "Add Mapping") })
						.AssetColor(AssetColor)
						.SelectedIndex(-1)
						.OnSelectionChanged_Lambda([this](int32)
						{
							AddMapping();
						})
						]
					]
				]
			]
		]
	];

	RebuildRows();
}

float SAudioFXModulationOverlay::AmountFromSettings(const FSourceEffectMotionFilterModulationSettings& Settings)
{
	// Convention in this overlay: vectors are mirrored (X == Y). If external code writes
	// asymmetric values, we conservatively fold X/Y to a mean scalar so the overlay can
	// still display/edit a stable amount without introducing additional state.
	const float PositiveAmount = 0.5f * (Settings.ModulationOutputMaximumRange.X + Settings.ModulationOutputMaximumRange.Y);
	const float NegativeAmount = 0.5f * (Settings.ModulationOutputMinimumRange.X + Settings.ModulationOutputMinimumRange.Y);
	// Amount storage is split across positive and negative ranges; only one side is
	// expected to be non-zero at a time, but we handle both defensively.
	if (!FMath::IsNearlyZero(PositiveAmount))
	{
		return FMath::Clamp(PositiveAmount, -1.f, 1.f);
	}

	return FMath::Clamp(NegativeAmount, -1.f, 1.f);
}

void SAudioFXModulationOverlay::ApplyAmountToSettings(FSourceEffectMotionFilterModulationSettings& Settings, const float Amount)
{
	const float ClampedAmount = FMath::Clamp(Amount, -1.f, 1.f);
	const float MinAmount = FMath::Min(0.f, ClampedAmount);
	const float MaxAmount = FMath::Max(0.f, ClampedAmount);

	Settings.ModulationOutputMinimumRange = FVector2D(MinAmount, MinAmount);
	Settings.ModulationOutputMaximumRange = FVector2D(MaxAmount, MaxAmount);
}

bool SAudioFXModulationOverlay::TryGetNextDestination(
	const TMap<ESourceEffectMotionFilterModDestination, FSourceEffectMotionFilterModulationSettings>& Mappings,
	ESourceEffectMotionFilterModDestination& OutDestination)
{
	for (const ESourceEffectMotionFilterModDestination Destination : AudioFXModulationOverlay::Destinations)
	{
		if (!Mappings.Contains(Destination))
		{
			OutDestination = Destination;
			return true;
		}
	}

	return false;
}

void SAudioFXModulationOverlay::RebuildRows()
{
	if (!RowsBox.IsValid())
	{
		return;
	}

	RowsBox->ClearChildren();

	USourceEffectMotionFilterPreset* PresetPtr = Preset.Get();
	if (!PresetPtr)
	{
		RowsBox->AddSlot().AutoHeight().Padding(4.f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("InvalidPreset", "Invalid preset"))
			.ColorAndOpacity(FAudioFXTheme::GetTextDisabledColor())
		];
		return;
	}

	TArray<ESourceEffectMotionFilterModDestination> Destinations;
	PresetPtr->Settings.ModulationMappings.GetKeys(Destinations);
	Destinations.Sort([](const ESourceEffectMotionFilterModDestination A, const ESourceEffectMotionFilterModDestination B)
	{
		return static_cast<int32>(A) < static_cast<int32>(B);
	});

	if (Destinations.IsEmpty())
	{
		RowsBox->AddSlot().AutoHeight().Padding(4.f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoMappings", "No mappings configured. Click Add Mapping to create one."))
			.ColorAndOpacity(FAudioFXTheme::GetTextDisabledColor())
		];
		return;
	}

	for (const ESourceEffectMotionFilterModDestination Destination : Destinations)
	{
		if (const FSourceEffectMotionFilterModulationSettings* Settings = PresetPtr->Settings.ModulationMappings.Find(Destination))
		{
			RowsBox->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				BuildMappingRow(Destination, *Settings)
			];
		}
	}
}

TSharedRef<SWidget> SAudioFXModulationOverlay::BuildMappingRow(
	const ESourceEffectMotionFilterModDestination Destination,
	const FSourceEffectMotionFilterModulationSettings& Settings)
{
	const int32 DestinationIndex = FMath::Max(0, AudioFXModulationOverlay::FindDestinationIndex(Destination));
	const int32 SourceIndex = FMath::Max(0, AudioFXModulationOverlay::FindSourceIndex(Settings.ModulationSource));
	const float AmountNorm = FMath::GetMappedRangeValueClamped(
		FVector2D(-1.f, 1.f),
		FVector2D(0.f, 1.f),
		AmountFromSettings(Settings));

	return SNew(SBorder)
		.BorderImage(&FAudioFXTheme::GetInsetBrush())
		.Padding(FMargin(6.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.44f).VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 2.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SourceLabel", "Source"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
					.ColorAndOpacity(FAudioFXTheme::GetTextDisabledColor())
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SAudioFXDropdown)
					.Options(AudioFXModulationOverlay::SourceLabels)
					.AssetColor(AssetColor)
					.SelectedIndex(SourceIndex)
					.OnSelectionChanged_Lambda([this, Destination](int32 NewSourceIndex)
					{
						OnSourceChanged(Destination, NewSourceIndex);
					})
				]
			]
			+ SHorizontalBox::Slot().FillWidth(0.44f).VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 2.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("DestinationLabel", "Destination"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
					.ColorAndOpacity(FAudioFXTheme::GetTextDisabledColor())
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SAudioFXDropdown)
					.Options(AudioFXModulationOverlay::DestinationLabels)
					.AssetColor(AssetColor)
					.SelectedIndex(DestinationIndex)
					.OnSelectionChanged_Lambda([this, Destination](int32 NewDestinationIndex)
					{
						OnDestinationChanged(Destination, NewDestinationIndex);
					})
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(SAudioFXLabeledKnob)
				.ParamName(LOCTEXT("AmountLabel", "Amount"))
				.Style(&AmountKnobStyle)
				.AudioUnitsValueType(EAudioUnitsValueType::Linear)
				// Value is normalized [0..1] for Slate input, while Min/Max are the displayed
				// raw units that the labeled knob maps to for formatting/editing.
				.Value(AmountNorm)
				.DefaultValue(AudioFXModulationOverlay::DefaultMappingAmount)
				.MinValue(-1.f)
				.MaxValue(1.f)
				.OnValueChanged_Lambda([this, Destination](float NewAmountNorm)
				{
					OnAmountChanged(Destination, NewAmountNorm);
				})
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SAudioFXButton)
				.Options(TArray<FText>{ LOCTEXT("RemoveMapping", "Remove") })
				.AssetColor(FAudioFXTheme::GetAccentWarn())
				.SelectedIndex(-1)
				.OnSelectionChanged_Lambda([this, Destination](int32)
				{
					RemoveMapping(Destination);
				})
			]
		];
}

void SAudioFXModulationOverlay::OnSourceChanged(const ESourceEffectMotionFilterModDestination Destination, const int32 SourceIndex)
{
	USourceEffectMotionFilterPreset* PresetPtr = Preset.Get();
	if (!PresetPtr || !AudioFXModulationOverlay::Sources.IsValidIndex(SourceIndex))
	{
		return;
	}

	if (FSourceEffectMotionFilterModulationSettings* Settings = PresetPtr->Settings.ModulationMappings.Find(Destination))
	{
		PresetPtr->Modify();
		Settings->ModulationSource = AudioFXModulationOverlay::Sources[SourceIndex];
		PresetPtr->SetSettings(PresetPtr->Settings);
	}
}

void SAudioFXModulationOverlay::OnDestinationChanged(const ESourceEffectMotionFilterModDestination OldDestination, const int32 DestinationIndex)
{
	USourceEffectMotionFilterPreset* PresetPtr = Preset.Get();
	if (!PresetPtr || !AudioFXModulationOverlay::Destinations.IsValidIndex(DestinationIndex))
	{
		return;
	}

	const ESourceEffectMotionFilterModDestination NewDestination = AudioFXModulationOverlay::Destinations[DestinationIndex];
	if (NewDestination == OldDestination)
	{
		return;
	}

	if (const FSourceEffectMotionFilterModulationSettings* ExistingSettings = PresetPtr->Settings.ModulationMappings.Find(OldDestination))
	{
		PresetPtr->Modify();

		// Cache the row being moved before any map mutations
		const FSourceEffectMotionFilterModulationSettings CachedSettings = *ExistingSettings;

		// Check whether the new destination slot is already occupied
		const FSourceEffectMotionFilterModulationSettings* ExistingAtNewDestination = PresetPtr->Settings.ModulationMappings.Find(NewDestination);
		const FSourceEffectMotionFilterModulationSettings SwappedSettings = ExistingAtNewDestination
			? *ExistingAtNewDestination
			: FSourceEffectMotionFilterModulationSettings();

		// Remove both slots before re-inserting to avoid key collisions
		PresetPtr->Settings.ModulationMappings.Remove(OldDestination);
		PresetPtr->Settings.ModulationMappings.Remove(NewDestination);
		PresetPtr->Settings.ModulationMappings.Add(NewDestination, CachedSettings);
		if (ExistingAtNewDestination)
		{
			// Preserve the displaced mapping by swapping it into the row's previous destination.
			PresetPtr->Settings.ModulationMappings.Add(OldDestination, SwappedSettings);
		}
		PresetPtr->SetSettings(PresetPtr->Settings);
		RebuildRows();
	}
}

void SAudioFXModulationOverlay::OnAmountChanged(const ESourceEffectMotionFilterModDestination Destination, const float AmountNorm)
{
	USourceEffectMotionFilterPreset* PresetPtr = Preset.Get();
	if (!PresetPtr)
	{
		return;
	}

	if (FSourceEffectMotionFilterModulationSettings* Settings = PresetPtr->Settings.ModulationMappings.Find(Destination))
	{
		PresetPtr->Modify();
		const float Amount = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 1.f), FVector2D(-1.f, 1.f), AmountNorm);
		ApplyAmountToSettings(*Settings, Amount);
		PresetPtr->SetSettings(PresetPtr->Settings);
	}
}

void SAudioFXModulationOverlay::RemoveMapping(const ESourceEffectMotionFilterModDestination Destination)
{
	USourceEffectMotionFilterPreset* PresetPtr = Preset.Get();
	if (!PresetPtr)
	{
		return;
	}

	PresetPtr->Modify();
	PresetPtr->Settings.ModulationMappings.Remove(Destination);
	PresetPtr->SetSettings(PresetPtr->Settings);
	RebuildRows();
}

void SAudioFXModulationOverlay::AddMapping()
{
	USourceEffectMotionFilterPreset* PresetPtr = Preset.Get();
	if (!PresetPtr)
	{
		return;
	}

	ESourceEffectMotionFilterModDestination NewDestination;
	if (!TryGetNextDestination(PresetPtr->Settings.ModulationMappings, NewDestination))
	{
		return;
	}

	FSourceEffectMotionFilterModulationSettings NewSettings;
	NewSettings.ModulationSource = ESourceEffectMotionFilterModSource::DistanceFromListener;
	NewSettings.ModulationInputRange = FVector2D(0.f, 1.f);
	ApplyAmountToSettings(NewSettings, AudioFXModulationOverlay::DefaultMappingAmount);

	PresetPtr->Modify();
	PresetPtr->Settings.ModulationMappings.Add(NewDestination, NewSettings);
	PresetPtr->SetSettings(PresetPtr->Settings);
	RebuildRows();
}

#undef LOCTEXT_NAMESPACE
