// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

/**
 * @file AudioFXEffectEditor.cpp
 * @brief FAudioFXEffectEditor implementation -- window lifecycle, preset bar,
 *        transport bar, meter strips, and collapsible section wiring.
 *
 * Entry point: InitEffectEditor(USoundEffectPreset*) -- called by
 * FAudioFXAssetTypeActions_Preset::OpenAssetEditor() after the factory
 * resolves the concrete editor subclass via AudioFXEditorFactory.
 */

#include "Shared/Core/AudioFXEffectEditor.h"
#include "Shared/Core/AudioFXTheme.h"
#include "Shared/Widgets/SAudioFXPresetBar.h"
#include "Shared/Widgets/SAudioFXGradientPanel.h"
#include "Shared/Core/AudioFXPreviewEngine.h"
#include "Shared/Widgets/SAudioFXTransportBar.h"
#include "Shared/Widgets/SAudioFXMeterStrip.h"
#include "Shared/Visualizers/SAudioFXGainReductionMeter.h"
#include "AudioEffectsStudioSettings.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SDPIScaler.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "PropertyEditorModule.h"
#include "Modules/ModuleManager.h"
#include "Sound/SoundEffectPreset.h"
#include "Framework/Application/SlateApplication.h"
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "TimerManager.h"
#include "Toolkits/SimpleAssetEditor.h"
#include "UObject/Package.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "UObject/UObjectGlobals.h"

#define LOCTEXT_NAMESPACE "AudioFXEffectEditor"

// ============================================================
// Construction / Destruction
// ============================================================

FAudioFXEffectEditor::FAudioFXEffectEditor()
{
}

FAudioFXEffectEditor::~FAudioFXEffectEditor()
{
	// Unsubscribe from scale change notifications
	UAudioEffectsStudioSettings::OnUIScaleChanged.Remove(ScaleChangeDelegateHandle);

	// Close the window if it is still open (e.g. editor GC'd externally)
	if (TSharedPtr<SWindow> Win = WindowPtr.Pin())
	{
		Win->RequestDestroyWindow();
	}

	if (PreviewEngine.IsValid())
	{
		PreviewEngine->Shutdown();
		PreviewEngine.Reset();
	}
}

// ============================================================
// Editor Initialization
// ============================================================

void FAudioFXEffectEditor::InitEffectEditor(USoundEffectPreset* InPreset)
{
	// ── Bring-to-front if already open ───────────────────────────────
	if (TSharedPtr<SWindow> Existing = WindowPtr.Pin())
	{
		Existing->BringToFront();
		return;
	}

	EditedPreset = InPreset;   // TWeakObjectPtr — safe against GC

	// Initialize the shared dirty-marking delegate once.
	// All editor lambdas capture [this] and call MarkDirty.ExecuteIfBound().
	MarkDirty = FSimpleDelegate::CreateSP(this, &FAudioFXEffectEditor::MarkPresetDirty);

	// ── Preview engine ────────────────────────────────────────────────
	PreviewEngine = MakeShared<FAudioFXPreviewEngine>();
	PreviewEngine->SetShowMeters(ShouldShowMeters());
	PreviewEngine->Initialize(InPreset);

	// ── Window size: exact, scaled by settings ────────────────────────
	// Capture mode: always use the raw (unscaled) design size so the OS window
	// matches the SDPIScaler scale of 1.0. Without this, the window is created
	// at the user's DPI scale (e.g. 150%) but content renders at 1.0, causing
	// the content to spread across a larger-than-expected window.
	const FVector2D WindowSize = bCaptureMode
		? GetDefaultWindowSize()
		: UAudioEffectsStudioSettings::Get()->ScaleWindowSize(GetDefaultWindowSize());

	// ── Window title ──────────────────────────────────────────────────
	const FString PresetName = InPreset ? InPreset->GetName() : TEXT("AudioFX Effect");
	FText Title = FText::FromString(PresetName);

	// ── Create fixed-size, close-only SWindow ─────────────────────────
	// Window is scaled by ScaleWindowSize(), so content MUST also be
	// scaled by the same factor via SDPIScaler. Without this, the window grows
	// at 150% DPI but the widgets stay at 100%-pixel sizes → massive dead space.
	// The DPIScale attribute is a live lambda so real-time scale changes apply immediately.
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(Title)
		.ClientSize(WindowSize)
		.SizingRule(ESizingRule::FixedSize)
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.HasCloseButton(true)
		[
			SNew(SDPIScaler)
			.DPIScale_Lambda([this]()
			{
				// Capture mode: always 1.0 — no DPI scaling ever touches screenshot output.
				// Normal interactive mode: live-tracks the user's scale setting.
				return bCaptureMode ? 1.0f : UAudioEffectsStudioSettings::Get()->GetEffectiveScale();
			})
			[
				BuildWindowContent()
			]
		];

	Window->SetOnWindowClosed(FOnWindowClosed::CreateSP(
		this, &FAudioFXEffectEditor::OnWindowClosed));

	WindowPtr = Window;

	FSlateApplication::Get().AddWindow(Window);

	// Subscribe to real-time scale changes — when the user moves the scale slider in
	// settings, resize this window immediately. The SDPIScaler already uses a live
	// lambda so its content scale updates automatically; only the OS window size needs updating.
	ScaleChangeDelegateHandle = UAudioEffectsStudioSettings::OnUIScaleChanged.AddSP(
		this, &FAudioFXEffectEditor::OnUIScaleChanged);

	// Force to front 1 frame after AddWindow — the UE content browser / asset editor
	// subsystem sometimes steals focus right after a new window is created.
	if (GEditor)
	{
		TWeakPtr<SWindow> WeakWin = Window;
		GEditor->GetTimerManager()->SetTimerForNextTick([WeakWin]()
		{
			if (TSharedPtr<SWindow> Win = WeakWin.Pin())
			{
				Win->BringToFront();
			}
		});
	}
}

// ============================================================
// Window Content Building
// ============================================================

TSharedRef<SWidget> FAudioFXEffectEditor::BuildWindowContent()
{
	USoundEffectPreset* Preset = EditedPreset.Get();
	if (!Preset)
	{
		return SNew(STextBlock)
			.Text(LOCTEXT("NoPreset", "No preset"))
			.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor());
	}

	const bool bShowMeters  = ShouldShowMeters();
	const bool bShowGRMeter = ShouldShowGRMeter();
	const bool bShowBypass  = ShouldShowBypass();
	const FLinearColor AssetCol = GetAssetColor();

	// Transport divider is only visible when the pool has at least one sound.
	// The transport bar itself always renders (it handles disabled state internally).
	auto TransportDividerVisibility = [this]()
	{
		return (PreviewEngine.IsValid() && PreviewEngine->GetPreviewPool().Num() > 0)
			? EVisibility::Visible
			: EVisibility::Collapsed;
	};

	return SNew(SOverlay)

		// ── Layer 1: Primary content ─────────────────────────────────────────
		+ SOverlay::Slot()
		[
			SNew(SAudioFXGradientPanel)
			.CornerRadius(0.0f)
			[
				SNew(SVerticalBox)

				// ── Transport bar (play / stop / next / loop / pool selector) ──
				+ SVerticalBox::Slot().AutoHeight()
				[
					SAssignNew(TransportBar, SAudioFXTransportBar)
					.Engine(PreviewEngine)
				]

				// ── Asset color accent stripe (3 px, full width) ─────────────
				// P3: thin accent line using the effect's asset color at 80% opacity.
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBox).HeightOverride(3.0f)
					[
						SNew(SBorder)
						.BorderBackgroundColor(FLinearColor(AssetCol.R, AssetCol.G, AssetCol.B, 0.8f))
					]
				]

				// ── Preset bar (dropdown, save, bypass, properties) ───────────
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SAudioFXPresetBar)
					.Preset(Preset)
					.AssetColor(AssetCol)
					.ShowBypass(bShowBypass)
					.IsBypassed_Lambda([this]()
					{
						if (PreviewEngine.IsValid() && PreviewEngine->IsSourceEffect())
							return PreviewEngine->GetBypass();
						return GetBypassState();
					})
					.OnBypassChanged_Lambda([this](bool bNew)
					{
						if (PreviewEngine.IsValid() && PreviewEngine->IsSourceEffect())
						{
							PreviewEngine->SetBypass(bNew);
						}
						else
						{
							MarkDirty.ExecuteIfBound();
							SetBypassState(bNew);
						}
					})
					.OnShowPropertiesClicked(FSimpleDelegate::CreateSP(
						this, &FAudioFXEffectEditor::OnShowProperties))
					.OnOpenDefaultEditorClicked(FSimpleDelegate::CreateSP(
						this, &FAudioFXEffectEditor::OnOpenDefaultEditor))
				]

				// ── Transport divider (1 px, collapses when pool is empty) ───
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBox).HeightOverride(1.0f)
					.Visibility_Lambda(TransportDividerVisibility)
					[
						SNew(SBorder)
						.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.4f))
					]
				]

				// ── Effect content area: [InputMeter] [EffectWidget] [GRMeter] [OutputMeter] ──
				// VAlign is driven by GetContentVAlign() — VAlign_Center for fixed layouts,
				// VAlign_Top for editors with dynamic row counts (EQ, Multiband, TapDelay).
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(FMargin(
					FAudioFXTheme::WindowPadding,
					FAudioFXTheme::WindowPadding,
					FAudioFXTheme::WindowPadding,
					FAudioFXTheme::WindowPadding + 16.f))
				.VAlign(GetContentVAlign())
				[
					SNew(SHorizontalBox)

					// ── Input meter strip ─────────────────────────────────────
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox)
						.HeightOverride(200.0f)
						[
							SNew(SAudioFXMeterStrip)
							.Engine(PreviewEngine)
							.bIsInput(true)
							.Visibility(bShowMeters ? EVisibility::Visible : EVisibility::Collapsed)
						]
					]

				// ── Effect-specific skin widget (created by derived class) ─
				// When bypass is active, IsEnabled(false) greys out and locks all child knobs/controls.
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(SBox)
					.IsEnabled_Lambda([this]()
					{
						if (PreviewEngine.IsValid() && PreviewEngine->IsSourceEffect())
							return !PreviewEngine->GetBypass();
						return !GetBypassState();
					})
					[
						CreateEffectWidget(Preset)
					]
				]

					// ── Gain-reduction meter (dynamics editors only) ──────────
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox)
						.HeightOverride(200.0f)
						[
							SNew(SAudioFXGainReductionMeter)
							.Engine(PreviewEngine)
							.BandIndex(-1)
							.Visibility((bShowMeters && bShowGRMeter) ? EVisibility::Visible : EVisibility::Collapsed)
						]
					]

					// ── Output meter strip ────────────────────────────────────
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox)
						.HeightOverride(200.0f)
						[
							SNew(SAudioFXMeterStrip)
							.Engine(PreviewEngine)
							.bIsInput(false)
							.Visibility(bShowMeters ? EVisibility::Visible : EVisibility::Collapsed)
						]
					]
				]
			]
		]

		// ── Layer 2: Branding watermark (bottom-right, semi-transparent) ─────
		// Two-line watermark: "Audio Effects Studio" / "by AudioFactory".
		// Opacity raised to ~55-58% for legibility on all three themes.
		// Uses disabled text color so it adapts automatically.
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.0f, 0.0f, 8.0f, 4.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("BrandingWatermark", "Audio Effects Studio"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(TAttribute<FSlateColor>::CreateLambda([]()
				{
					return FSlateColor(FAudioFXTheme::GetTextDisabledColor().CopyWithNewOpacity(0.58f));
				}))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("BrandingSubtitle", "by AudioFactory"))
				.Font(FCoreStyle::GetDefaultFontStyle("Italic", 7))
				.ColorAndOpacity(TAttribute<FSlateColor>::CreateLambda([]()
				{
					return FSlateColor(FAudioFXTheme::GetTextDisabledColor().CopyWithNewOpacity(0.55f));
				}))
			]
		];
}

// ============================================================
// Window Lifecycle
// ============================================================

void FAudioFXEffectEditor::OnWindowClosed(const TSharedRef<SWindow>& /*ClosedWindow*/)
{
	// Stop preview playback immediately — before any Shutdown() logic.
	// UAudioComponent::Stop() is non-blocking (async command to audio thread).
	// Calling it here gives the audio thread time to drain BEFORE the synchronous
	// GEditor->ResetPreviewAudioComponent() fence inside Shutdown(), eliminating
	// the 1–3 second stall that was visible on first window close after playback.
	if (PreviewEngine.IsValid())
	{
		PreviewEngine->Stop();
	}

	// Fix B for UE auto-restore: notify the asset editor subsystem that this asset
	// is now closed so UE does not record it for RestorePreviouslyOpenAssets().
	// Fix A (bRestorePreviouslyOpenAssets=False in DefaultEditorPerProjectUserSettings.ini)
	// is the primary guard; this is a belt-and-suspenders secondary guard.
	if (GEditor)
	{
		if (UAssetEditorSubsystem* AES = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			if (USoundEffectPreset* Preset = EditedPreset.Get())
			{
				AES->NotifyAssetClosed(Preset, nullptr);
			}
		}
	}

	WindowPtr.Reset();
	PropertiesWindowPtr.Reset();

	if (PreviewEngine.IsValid())
	{
		PreviewEngine->Shutdown();
		PreviewEngine.Reset();
	}
}

FLinearColor FAudioFXEffectEditor::GetAssetColor() const
{
	if (const USoundEffectPreset* Preset = EditedPreset.Get())
	{
		return FLinearColor(Preset->GetPresetColor());
	}
	// Fallback: use FColor() conversion so sRGB values map to correct linear space.
	return FLinearColor(FColor(128, 77, 77));
}

// ============================================================
// Preset State
// ============================================================

USoundEffectPreset* FAudioFXEffectEditor::GetPreset() const
{
	return EditedPreset.Get();
}

void FAudioFXEffectEditor::BringToFront()
{
	if (TSharedPtr<SWindow> Win = WindowPtr.Pin())
	{
		Win->BringToFront();
	}
}

void FAudioFXEffectEditor::ResizeWindowByDelta(float DeltaY)
{
	TSharedPtr<SWindow> Win = WindowPtr.Pin();
	if (!Win) { return; }

	// DeltaY is in LOGICAL pixels (as measured by widget geometry inside SDPIScaler).
	// The SWindow operates in SCREEN pixels — multiply by the scale factor.
	const float Scale      = UAudioEffectsStudioSettings::Get()->GetEffectiveScale();
	const FVector2D Current = Win->GetClientSizeInScreen();
	const FVector2D Scaled  = UAudioEffectsStudioSettings::Get()->ScaleWindowSize(GetDefaultWindowSize());
	const float NewH = FMath::Max(Current.Y + DeltaY * Scale, Scaled.Y * 0.4f);
	Win->Resize(FVector2D(Current.X, NewH));
}

void FAudioFXEffectEditor::MarkPresetDirty()
{
	if (USoundEffectPreset* PresetPtr = EditedPreset.Get())
	{
		// 1. Modify() records pre-change state for undo (must come BEFORE mutation at call site)
		PresetPtr->Modify();

		// 2. MarkPackageDirty() flags the package as unsaved
		if (UPackage* Pkg = PresetPtr->GetOutermost())
		{
			Pkg->MarkPackageDirty();
		}

		// 3. PostEditChange() notifies the editor framework (Content Browser, Details panels,
		//    property change delegates) that the object was modified.
		PresetPtr->PostEditChange();

		// 4. Broadcast OnObjectPropertyChanged so the Content Browser tile repaints
		//    with the dirty asterisk immediately (same signal that the default UE property editor sends).
		{
			FPropertyChangedEvent DummyEvent(nullptr, EPropertyChangeType::Unspecified);
			FCoreUObjectDelegates::OnObjectPropertyChanged.Broadcast(PresetPtr, DummyEvent);
		}

		// 5. Notify the Asset Registry that this asset's tags changed.
		//    AssetTagsFinalized() causes the Content Browser to repaint the tile
		//    with the dirty asterisk immediately (without folder navigation).
		if (FModuleManager::Get().IsModuleLoaded("AssetRegistry"))
		{
			FAssetRegistryModule& ARModule =
				FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
			ARModule.Get().AssetTagsFinalized(*PresetPtr);
		}

		// 6. Force the Content Browser to refresh immediately so the asterisk (*) appears
		//    on the asset tile without requiring a manual folder navigation.
		if (FModuleManager::Get().IsModuleLoaded("ContentBrowser"))
		{
			FContentBrowserModule& CBModule =
				FModuleManager::GetModuleChecked<FContentBrowserModule>("ContentBrowser");
			TArray<UObject*> Objects = { PresetPtr };
			CBModule.Get().SyncBrowserToAssets(
				Objects,
				/*bAllowLockedBrowsers=*/false,
				/*bFocusContentBrowser=*/false);
		}
	}
}

// ============================================================
// Effect Widget / Screenshot
// ============================================================

TSharedRef<SWidget> FAudioFXEffectEditor::CreateEffectWidget(USoundEffectPreset* InPreset)
{
	StoredKnobStyles.Empty();

	return SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoSkinAvailable",
				"No AudioFX skin available for this effect type.\nShowing default editor."))
			.ColorAndOpacity(FAudioFXTheme::GetLabelTextColor())
		];
}

TSharedRef<SWidget> FAudioFXEffectEditor::BuildFullWidgetForScreenshot(USoundEffectPreset* InPreset)
{
	// Set up the editor state as if InitEffectEditor() was called,
	// but without creating an OS window.
	EditedPreset = InPreset;
	MarkDirty = FSimpleDelegate::CreateSP(this, &FAudioFXEffectEditor::MarkPresetDirty);

	// Create preview engine (needed by transport bar / meters)
	PreviewEngine = MakeShared<FAudioFXPreviewEngine>();
	PreviewEngine->SetShowMeters(ShouldShowMeters());
	PreviewEngine->Initialize(InPreset);

	// Build the full window content: gradient bg + preset bar + transport + effect + meters
	// Lock to capture mode first — guarantees DPI = 1.0, no exceptions.
	SetCaptureMode();
	return SNew(SDPIScaler)
		.DPIScale(1.0f)
		[
			BuildWindowContent()
		];
}

const FAudioMaterialKnobStyle* FAudioFXEffectEditor::StoreKnobStyle(FAudioMaterialKnobStyle&& Style)
{
	TUniquePtr<FAudioMaterialKnobStyle>& Stored =
		StoredKnobStyles.Emplace_GetRef(MakeUnique<FAudioMaterialKnobStyle>(MoveTemp(Style)));
	return Stored.Get();
}

// ============================================================
// Properties Window / Default Editor
// ============================================================

void FAudioFXEffectEditor::OnShowProperties()
{
	// Bring existing properties window to front instead of spawning a new one
	if (TSharedPtr<SWindow> ExistingProps = PropertiesWindowPtr.Pin())
	{
		ExistingProps->BringToFront();
		return;
	}

	USoundEffectPreset* Preset = EditedPreset.Get();
	if (!Preset)
	{
		return;
	}

	FPropertyEditorModule& PropertyModule =
		FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = true;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.bShowPropertyMatrixButton = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

	TSharedRef<IDetailsView> DetailsView = PropertyModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetObject(Preset);

	const FText WindowTitle = FText::Format(
		LOCTEXT("PropertiesWindowTitle", "{0} — Properties"),
		FText::FromString(Preset->GetName()));

	TSharedRef<SWindow> PropertiesWindow = SNew(SWindow)
		.Title(WindowTitle)
		.ClientSize(FVector2D(400.0f, 550.0f))
		.SizingRule(ESizingRule::UserSized)
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.HasCloseButton(true)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(4.0f)
			[
				DetailsView
			]
		];

	PropertiesWindowPtr = PropertiesWindow;
	FSlateApplication::Get().AddWindow(PropertiesWindow);
}

void FAudioFXEffectEditor::OnOpenDefaultEditor()
{
	USoundEffectPreset* Preset = EditedPreset.Get();
	if (!Preset)
	{
		return;
	}

	// Close our custom window first
	if (TSharedPtr<SWindow> Win = WindowPtr.Pin())
	{
		Win->RequestDestroyWindow();
	}

	// Open the SimpleAssetEditor 1 frame later (after window destruction completes)
	if (GEditor)
	{
		// Capture a weak ptr so it's safe even if GC runs during the deferred call
		TWeakObjectPtr<USoundEffectPreset> WeakPreset(Preset);
		GEditor->GetTimerManager()->SetTimerForNextTick([WeakPreset]()
		{
			if (USoundEffectPreset* P = WeakPreset.Get())
			{
				// Use FSimpleAssetEditor explicitly — this opens the raw property editor
				// for the preset, not our custom skin editor (which would re-open itself).
				TArray<UObject*> Objects;
				Objects.Add(P);
				FSimpleAssetEditor::CreateEditor(EToolkitMode::Standalone, TSharedPtr<IToolkitHost>(), Objects);
			}
		});
	}
}

// ============================================================
// Scale Change Handler
// ============================================================

void FAudioFXEffectEditor::OnUIScaleChanged(float /*NewScale*/)
{
	TSharedPtr<SWindow> Win = WindowPtr.Pin();
	if (!Win) { return; }

	// Recompute the window client size with the new scale factor.
	// SDPIScaler already uses a live lambda so content scale updates automatically.
	// We only need to resize the OS window.
	const FVector2D NewSize = UAudioEffectsStudioSettings::Get()->ScaleWindowSize(GetDefaultWindowSize());
	Win->Resize(NewSize);
}

#undef LOCTEXT_NAMESPACE
