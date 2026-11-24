// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Widgets/SAudioFXTransportBar.h"
#include "Shared/Core/AudioFXPreviewEngine.h"
#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Widgets/SAudioFXIconButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SComboBox.h"
#include "Sound/SoundWave.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "SAudioFXTransportBar"

void SAudioFXTransportBar::Construct(const FArguments& InArgs)
{
	Engine = InArgs._Engine;
	ComboItems.Empty();

	if (Engine.IsValid())
	{
		Engine->OnPoolDiscoveryComplete.AddSP(this, &SAudioFXTransportBar::RefreshSoundDropdown);
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(FAudioFXTheme::GetPanelBackground())
		.Padding(FMargin(8.0f, 4.0f))
		[
			SNew(SHorizontalBox)

			// ── Left spacer so content centres ──────────────────────────
			+ SHorizontalBox::Slot().FillWidth(1.0f)

			// ── Transport buttons ────────────────────────────────────────
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2.0f, 0.0f)
			[
				SNew(SAudioFXIconButton)
				.IconBrush(FAppStyle::GetBrush("Icons.Toolbar.Play"))
				.IconSize(FVector2D(12.0f, 12.0f))
				.ToolTipText(LOCTEXT("PlayTip", "Play preview sound"))
				.IsEnabled(this, &SAudioFXTransportBar::IsPlayEnabled)
				.OnClicked(this, &SAudioFXTransportBar::OnPlayClicked)
				.ActiveColor(this, &SAudioFXTransportBar::GetPlayActiveColor)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2.0f, 0.0f)
			[
				SNew(SAudioFXIconButton)
				.IconBrush(FAppStyle::GetBrush("Icons.Toolbar.Stop"))
				.IconSize(FVector2D(12.0f, 12.0f))
				.ToolTipText(LOCTEXT("StopTip", "Stop preview"))
				.IsEnabled(this, &SAudioFXTransportBar::IsStopEnabled)
				.OnClicked(this, &SAudioFXTransportBar::OnStopClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2.0f, 0.0f)
			[
				SNew(SAudioFXIconButton)
				.IconBrush(FAppStyle::GetBrush("Icons.ChevronRight"))
				.IconSize(FVector2D(12.0f, 12.0f))
				.ToolTipText(LOCTEXT("NextTip", "Next sound in pool"))
				.IsEnabled(this, &SAudioFXTransportBar::IsNavigationEnabled)
				.OnClicked(this, &SAudioFXTransportBar::OnNextClicked)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2.0f, 0.0f)
			[
				SNew(SAudioFXIconButton)
				.IconBrush(FAppStyle::GetBrush("Icons.Refresh"))
				.IconSize(FVector2D(12.0f, 12.0f))
				.ToolTipText(LOCTEXT("LoopTip", "Toggle loop mode"))
				.IsEnabled(this, &SAudioFXTransportBar::IsNavigationEnabled)
				.OnClicked(this, &SAudioFXTransportBar::OnLoopClicked)
				.ActiveColor(this, &SAudioFXTransportBar::GetLoopActiveColor)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.0f, 2.0f)
			[
				SNew(SBox).WidthOverride(1.0f).HeightOverride(18.0f)
				[
					SNew(SBorder).BorderBackgroundColor(FAudioFXTheme::GetDividerColor())
				]
			]
			// ── Sound dropdown — fixed width, centred in bar ─────────────
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(220.0f)
				[
					SAssignNew(SoundComboBox, SComboBox<TSharedPtr<FString>>)
					.OptionsSource(&ComboItems)
					.OnGenerateWidget(this, &SAudioFXTransportBar::GenerateComboItem)
					.OnSelectionChanged(this, &SAudioFXTransportBar::OnComboSelectionChanged)
					.IsEnabled(this, &SAudioFXTransportBar::IsNavigationEnabled)
					.ToolTipText(LOCTEXT("ComboTip", "Select preview sound. Only sounds using this effect can be previewed"))
					.ContentPadding(FMargin(4.0f, 2.0f))
					[
						SNew(STextBlock)
						.Text(this, &SAudioFXTransportBar::GetComboButtonText)
						.ColorAndOpacity(FAudioFXTheme::GetValueTextColor())
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
					]
				]
			]

			// ── Right spacer (mirrors left, keeps content centred) ───────
			+ SHorizontalBox::Slot().FillWidth(1.0f)
		]
	];
}

SAudioFXTransportBar::~SAudioFXTransportBar()
{
	if (Engine.IsValid())
	{
		Engine->OnPoolDiscoveryComplete.RemoveAll(this);
	}
}

void SAudioFXTransportBar::RefreshSoundDropdown()
{
	if (!Engine.IsValid() || !SoundComboBox.IsValid())
	{
		return;
	}

	// Rebuild display names from the engine pool
	ComboItems.Empty();
	for (const TSoftObjectPtr<USoundWave>& Wave : Engine->GetPreviewPool())
	{
		const FString DisplayName = Wave.IsNull()
			? TEXT("(unknown)")
			: FPaths::GetBaseFilename(Wave.GetAssetName());
		ComboItems.Add(MakeShared<FString>(DisplayName));
	}

	SoundComboBox->RefreshOptions();

	// Sync selection to engine's current index
	const int32 Idx = Engine->GetCurrentIndex();
	if (ComboItems.IsValidIndex(Idx))
	{
		SoundComboBox->SetSelectedItem(ComboItems[Idx]);
	}
}

TSharedRef<SWidget> SAudioFXTransportBar::GenerateComboItem(TSharedPtr<FString> Item) const
{
	const FString Name = Item.IsValid() ? *Item : TEXT("(unknown)");
	return SNew(STextBlock)
		.Text(FText::FromString(Name))
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
		.ColorAndOpacity(FAudioFXTheme::GetValueTextColor());
}

void SAudioFXTransportBar::OnComboSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	// Ignore programmatic changes (Direct = from RefreshSoundDropdown / Next / Random)
	if (!Engine.IsValid() || !NewSelection.IsValid() || SelectInfo == ESelectInfo::Direct)
	{
		return;
	}

	// Map display name back to pool index
	const int32 NewIndex = ComboItems.IndexOfByPredicate([&NewSelection](const TSharedPtr<FString>& Item)
	{
		return Item.IsValid() && *Item == *NewSelection;
	});

	if (NewIndex != INDEX_NONE)
	{
		Engine->SetCurrentIndex(NewIndex);
	}
}

FText SAudioFXTransportBar::GetComboButtonText() const
{
	if (!Engine.IsValid())
	{
		return FText::GetEmpty();
	}


	if (Engine->GetPreviewPool().Num() == 0)
	{
		return LOCTEXT("NoSoundsMessage", "Assign effect to sounds to preview");
	}

	const TSoftObjectPtr<USoundWave> Current = Engine->GetCurrentSound();
	const FString SoundName = Current.IsNull()
		? TEXT("(unknown)")
		: FPaths::GetBaseFilename(Current.GetAssetName());

	return FText::Format(
		LOCTEXT("SoundNameFormat", "{0}  ({1}/{2})"),
		FText::FromString(SoundName),
		FText::AsNumber(Engine->GetCurrentIndex() + 1),
		FText::AsNumber(Engine->GetPreviewPool().Num()));
}

FReply SAudioFXTransportBar::OnPlayClicked()
{
	if (Engine.IsValid()) Engine->Play();
	return FReply::Handled();
}

FReply SAudioFXTransportBar::OnStopClicked()
{
	if (Engine.IsValid()) Engine->Stop();
	return FReply::Handled();
}

FReply SAudioFXTransportBar::OnNextClicked()
{
	if (Engine.IsValid())
	{
		Engine->Next();
		const int32 Idx = Engine->GetCurrentIndex();
		if (SoundComboBox.IsValid() && ComboItems.IsValidIndex(Idx))
		{
			SoundComboBox->SetSelectedItem(ComboItems[Idx]);
		}
	}
	return FReply::Handled();
}

FReply SAudioFXTransportBar::OnLoopClicked()
{
	if (Engine.IsValid()) Engine->SetLooping(!Engine->IsLooping());
	return FReply::Handled();
}

bool SAudioFXTransportBar::IsTransportEnabled() const
{
	return Engine.IsValid() && Engine->GetPreviewPool().Num() > 0;
}

bool SAudioFXTransportBar::IsPlayEnabled() const
{
	if (!Engine.IsValid() || Engine->GetPreviewPool().Num() == 0) return false;
	const EPreviewState St = Engine->GetState();
	return St == EPreviewState::Ready;
}

bool SAudioFXTransportBar::IsStopEnabled() const
{
	return Engine.IsValid() && Engine->GetState() == EPreviewState::Playing;
}

bool SAudioFXTransportBar::IsNavigationEnabled() const
{
	return Engine.IsValid() && Engine->GetPreviewPool().Num() > 0;
}

FLinearColor SAudioFXTransportBar::GetPlayActiveColor() const
{
	// Glow teal when actively playing
	if (Engine.IsValid() && Engine->GetState() == EPreviewState::Playing)
	{
		return FAudioFXTheme::GetAccentSecondary(); // #00D4AA
	}
	return FLinearColor::Transparent;
}

FLinearColor SAudioFXTransportBar::GetLoopActiveColor() const
{
	// Glow orange when loop mode is on
	if (Engine.IsValid() && Engine->IsLooping())
	{
		return FAudioFXTheme::GetAccentPrimary(); // #FF6B00
	}
	return FLinearColor::Transparent;
}

#undef LOCTEXT_NAMESPACE
