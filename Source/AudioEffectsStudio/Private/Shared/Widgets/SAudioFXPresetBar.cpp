// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#include "Shared/Widgets/SAudioFXPresetBar.h"
#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Widgets/SAudioFXBypassButton.h"
#include "Shared/Widgets/SAudioFXIconButton.h"

#include "Sound/SoundEffectPreset.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Rendering/DrawElementTypes.h"
#include "Styling/AppStyle.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "AudioEffectsStudioSettings.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Editor.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "SAudioFXPresetBar"

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

void SAudioFXPresetBar::Construct(const FArguments& InArgs)
{
	Preset = InArgs._Preset;    // TWeakObjectPtr — safe against GC
	AssetColor = InArgs._AssetColor;
	OnShowProperties = InArgs._OnShowPropertiesClicked;
	OnOpenDefaultEditor = InArgs._OnOpenDefaultEditorClicked;
	OnBypassChangedDelegate = InArgs._OnBypassChanged;
	const bool bShowBypass = InArgs._ShowBypass;

	// Brightened asset color for the effect name
	const FLinearColor GlowColor = FLinearColor(
		FMath::Min(AssetColor.R * 1.3f + 0.1f, 1.0f),
		FMath::Min(AssetColor.G * 1.3f + 0.1f, 1.0f),
		FMath::Min(AssetColor.B * 1.3f + 0.1f, 1.0f),
		1.0f);

	// Bar background brush — rounded top corners, asset-color tinted
	PresetBarBrush = MakeUnique<FSlateRoundedBoxBrush>(
		FMath::Lerp(FAudioFXTheme::GetPanelBackground(), AssetColor, 0.08f),
		FVector4(4.0, 4.0, 0.0, 0.0),
		FLinearColor(FColor(0x24, 0x24, 0x33, 0x80)),
		FAudioFXTheme::OutlineWidth);

	// Pill-shaped vertical divider brush
	static FSlateRoundedBoxBrush DividerBrush(FAudioFXTheme::GetDividerColor(), 2.0f);

	ChildSlot
	[
		SNew(SOverlay)

		// Layer 0: rounded box background
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(PresetBarBrush.Get())
			.Padding(FMargin(7.0f, 3.0f))   // was (10, 4) → reduced ~30%
			[
				SNew(SHorizontalBox)

				// ── Effect Type Name ────────────────
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 9.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(this, &SAudioFXPresetBar::GetEffectTypeName)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))   // was 12
					.ColorAndOpacity(GlowColor)
				]

				// ── Divider 1: between name and preset name ───
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					SNew(SBox)
					.WidthOverride(1.0f)
					.HeightOverride(10.0f)   // was 14
					[
						SNew(SBorder).BorderImage(&DividerBrush)
					]
				]

				// ── Preset Name (fills remaining width) ─────────────
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(this, &SAudioFXPresetBar::GetPresetName)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))  // was 9
					.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
					.ToolTipText(this, &SAudioFXPresetBar::GetPresetPackagePathTooltip)
				]

				// ── Divider 2: between preset name and action buttons ─
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(3.0f, 0.0f, 3.0f, 0.0f)
				[
					SNew(SBox)
					.WidthOverride(1.0f)
					.HeightOverride(10.0f)   // was 14
					[
						SNew(SBorder).BorderImage(&DividerBrush)
					]
				]

				// ── Open in default UE editor button (leftmost action) ──
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SAudioFXIconButton)
					.IconBrush(FAppStyle::GetBrush("Icons.Edit"))
					.ToolTipText(LOCTEXT("OpenDefaultEditorTip", "Close this window and open in the default Unreal asset editor"))
					.OnClicked(this, &SAudioFXPresetBar::OnOpenDefaultEditorClicked)
				]

				// ── Save button ───────────────────────────────────────
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2.0f, 0.0f)
				[
					SNew(SAudioFXIconButton)
					.IconBrush(FAppStyle::GetBrush("Icons.Save"))
					.ToolTipText(LOCTEXT("SavePresetTip", "Save this preset"))
					.IsDirty(this, &SAudioFXPresetBar::IsPresetDirty)
					.OnClicked(this, &SAudioFXPresetBar::OnSaveClicked)
				]

				// ── Browse to Asset button (search icon, rightmost before bypass) ─
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2.0f, 0.0f)
				[
					SNew(SAudioFXIconButton)
					.IconBrush(FAppStyle::GetBrush("Icons.Search"))
					.ToolTipText(LOCTEXT("BrowseToAssetTip", "Browse to this asset in the Content Browser"))
					.OnClicked(this, &SAudioFXPresetBar::OnBrowseToAssetClicked)
				]

				// ── Divider before bypass ────────────────────────────
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(5.0f, 0.0f, 5.0f, 0.0f)
				[
					bShowBypass
					? StaticCastSharedRef<SWidget>(
						SNew(SBox)
						.WidthOverride(1.0f)
						.HeightOverride(10.0f)   // was 14
						[
							SNew(SBorder).BorderImage(&DividerBrush)
						]
					)
					: StaticCastSharedRef<SWidget>(SNullWidget::NullWidget)
				]

				// ── Bypass button (rightmost) ────────────────────────
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					bShowBypass
					? StaticCastSharedRef<SWidget>(
						SNew(SAudioFXBypassButton)
						.IsBypassed(InArgs._IsBypassed)
						.OnBypassChanged_Lambda([this](bool bNewState)
						{
							OnBypassChangedDelegate.ExecuteIfBound(bNewState);
						})
					)
					: StaticCastSharedRef<SWidget>(SNullWidget::NullWidget)
				]
			]
		]

		// Layer 1: glass highlight line at top
		+ SOverlay::Slot()
		.VAlign(VAlign_Top)
		[
			SNew(SBox)
			.HeightOverride(1.0f)
			[
				SNew(SBorder)
				.BorderImage(&FAudioFXTheme::GetHighlightLineBrush())
			]
		]
	];
}

// ─────────────────────────────────────────────────────────────────────────────
// Display helpers
// ─────────────────────────────────────────────────────────────────────────────

FText SAudioFXPresetBar::GetEffectTypeName() const
{
	const USoundEffectPreset* PresetPtr = Preset.Get();
	if (PresetPtr)
	{
		// Step 1: start from the C++ class name
		// e.g. "USourceEffectBitCrusherPreset" → "SourceEffectBitCrusherPreset"
		FString ClassName = PresetPtr->GetClass()->GetName();

		// Step 2: detect and record the category prefix (SOURCE / SUBMIX)
		// then strip it along with the boilerplate suffix.
		FString CategoryPrefix;
		if (ClassName.RemoveFromStart(TEXT("SourceEffect")))
		{
			CategoryPrefix = TEXT("SOURCE ");
		}
		else if (ClassName.RemoveFromStart(TEXT("SubmixEffect")))
		{
			CategoryPrefix = TEXT("SUBMIX ");
		}
		ClassName.RemoveFromEnd(TEXT("Preset"));

		// Step 3: insert spaces before each new CamelCase word boundary
		// "BitCrusher" → "BIT CRUSHER", "DynamicsProcessor" → "DYNAMICS PROCESSOR"
		FString SemanticName;
		for (int32 Index = 0; Index < ClassName.Len(); ++Index)
		{
			const TCHAR Ch = ClassName[Index];
			if (Index > 0 && FChar::IsUpper(Ch) && !FChar::IsUpper(ClassName[Index - 1]))
			{
				SemanticName += TEXT(' ');
			}
			SemanticName += Ch;
		}

		// Step 4: combine as "SOURCE BIT CRUSHER" / "SUBMIX REVERB"
		// Both category and semantic parts are uppercased for consistent visual weight.
		const FString DisplayName = CategoryPrefix + SemanticName.ToUpper().TrimStartAndEnd();
		return FText::FromString(DisplayName);
	}
	return LOCTEXT("UnknownEffect", "Unknown Effect");
}

FText SAudioFXPresetBar::GetPresetName() const
{
	const USoundEffectPreset* P = Preset.Get();
	if (P)
	{
		// Clean up raw asset names for display
		return FText::FromString(FAudioFXTheme::CleanPresetName(P->GetName()));
	}
	return LOCTEXT("NoPreset", "\u2014");  // em dash — instead of raw "<No Preset>"
}

FText SAudioFXPresetBar::GetPresetPackagePathTooltip() const
{
	const USoundEffectPreset* P = Preset.Get();
	if (!P)
	{
		return FText::GetEmpty();
	}
	// Use the outermost package name — this gives the clean /Game/... path.
	// GetName() on the package returns e.g. "/Game/Sounds/MyPreset" without duplication.
	const UPackage* Pkg = P->GetOutermost();
	if (Pkg)
	{
		return FText::FromString(Pkg->GetName());
	}
	return FText::FromString(P->GetPathName());
}

// ─────────────────────────────────────────────────────────────────────────────
// Button action callbacks
// ─────────────────────────────────────────────────────────────────────────────

FReply SAudioFXPresetBar::OnSaveClicked()
{
	if (USoundEffectPreset* P = Preset.Get())
	{
		UPackage* Package = P->GetOutermost();
		if (Package && Package->IsDirty())
		{
			// Save silently without a modal dialog to avoid sending our window to background
			const FString PackageFilename = FPackageName::LongPackageNameToFilename(
				Package->GetName(), FPackageName::GetAssetPackageExtension());
			FSavePackageArgs SaveArgs;
			SaveArgs.TopLevelFlags = RF_Standalone;
			const FSavePackageResultStruct Result = UPackage::Save(Package, P, *PackageFilename, SaveArgs);
			if (Result.Result == ESavePackageResult::Success)
			{
				Package->SetDirtyFlag(false);
			}
		}
	}

	// Force our parent window to the front 1 frame after save, because the UE save
	// pipeline (asset registry notifications etc.) can steal focus.
	if (GEditor)
	{
		// Capture the parent SWindow weakly — find it by walking up the widget hierarchy.
		TSharedPtr<SWindow> ParentWin = FSlateApplication::Get().FindWidgetWindow(AsShared());
		if (ParentWin.IsValid())
		{
			TWeakPtr<SWindow> WeakWin = ParentWin;
			GEditor->GetTimerManager()->SetTimerForNextTick([WeakWin]()
			{
				if (TSharedPtr<SWindow> Win = WeakWin.Pin())
				{
					Win->BringToFront();
				}
			});
		}
	}

	return FReply::Handled();
}

FReply SAudioFXPresetBar::OnShowPropertiesClicked()
{
	OnShowProperties.ExecuteIfBound();
	return FReply::Handled();
}

FReply SAudioFXPresetBar::OnOpenPluginSettingsClicked()
{
	// Bring existing settings window to front if already open
	if (SettingsWindowPtr.IsValid())
	{
		if (TSharedPtr<SWindow> Win = SettingsWindowPtr.Pin())
		{
			Win->BringToFront();
			return FReply::Handled();
		}
	}

	FPropertyEditorModule& PropertyModule =
		FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.bShowPropertyMatrixButton = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

	TSharedRef<IDetailsView> DetailsView = PropertyModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetObject(UAudioEffectsStudioSettings::Get());

	TSharedRef<SWindow> SettingsWindow = SNew(SWindow)
		.Title(LOCTEXT("SettingsWindowTitle", "Audio Effects Studio — Settings"))
		.ClientSize(FVector2D(380.0f, 220.0f))
		.SizingRule(ESizingRule::FixedSize)
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.HasCloseButton(true)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(8.0f)
			[
				DetailsView
			]
		];

	SettingsWindowPtr = SettingsWindow;
	FSlateApplication::Get().AddWindow(SettingsWindow);
	return FReply::Handled();
}

FReply SAudioFXPresetBar::OnOpenDefaultEditorClicked()
{
	OnOpenDefaultEditor.ExecuteIfBound();
	return FReply::Handled();
}

FReply SAudioFXPresetBar::OnBrowseToAssetClicked()
{
	if (USoundEffectPreset* P = Preset.Get())
	{
		TArray<UObject*> Objects;
		Objects.Add(P);
		GEditor->SyncBrowserToObjects(Objects);
	}
	return FReply::Handled();
}

bool SAudioFXPresetBar::IsPresetDirty() const
{
	// TWeakObjectPtr: .IsValid() also catches GC'd objects (unlike raw ptr null check)
	const USoundEffectPreset* P = Preset.Get();
	return P && P->GetOutermost() && P->GetOutermost()->IsDirty();
}

int32 SAudioFXPresetBar::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	// Paint the child widget tree first (background border, text, buttons)
	const int32 MaxChildLayer = SCompoundWidget::OnPaint(
		Args, AllottedGeometry, MyCullingRect,
		OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	return MaxChildLayer;
}

#undef LOCTEXT_NAMESPACE
