// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundEffectPreset.h"
#include "AudioMaterialSlate/AudioMaterialSlateTypes.h"
#include "AudioEffectsStudioSettings.h"

class SAudioFXEffectPanel;
class USoundEffectPreset;
class FAudioFXPreviewEngine;
class SAudioFXTransportBar;
class SWindow;

/**
 * FAudioFXEffectEditor
 *
 * Owns a single fixed-size, non-resizable, non-minimizable top-level SWindow
 * for one USoundEffectPreset. The window shows:
 *   - Preset bar (dropdown + save)
 *   - Transport bar (preview playback)
 *   - Effect-specific knob skin widget
 *   - Optional flanking meter strips
 *
 * Subclasses only need to override CreateEffectWidget() and
 * GetDefaultWindowSize().
 *
 * Lifetime: owned by a TSharedPtr held in FAudioFXAssetTypeActions_Preset's
 * OpenEditors map. Cleared when the SWindow reports OnWindowClosed.
 */
class AUDIOEFFECTSSTUDIO_API FAudioFXEffectEditor : public TSharedFromThis<FAudioFXEffectEditor>
{
public:
	// ── Construction / Destruction ────────────────────────────────────────
	FAudioFXEffectEditor();
	virtual ~FAudioFXEffectEditor();

	// Non-copyable (TArray<TUniquePtr<>> member)
	FAudioFXEffectEditor(const FAudioFXEffectEditor&) = delete;
	FAudioFXEffectEditor& operator=(const FAudioFXEffectEditor&) = delete;

	/** Open (or bring-to-front) the fixed-size SWindow for this preset.
	 * Safe to call multiple times — re-uses the existing window.
	 */
	void InitEffectEditor(USoundEffectPreset* InPreset);

	// ── Window Access ─────────────────────────────────────────────────────

	/** True if the window is currently open. */
	bool IsWindowOpen() const { return WindowPtr.IsValid(); }

	/** Returns the editor's top-level SWindow, or nullptr if not open. */
	TSharedPtr<SWindow> GetWindow() const { return WindowPtr.Pin(); }

	/** Bring the window to the front if open. No-op if already closed. */
	void BringToFront();

	/**
	 * Adjust the window height by DeltaY pixels (positive = grow, negative = shrink).
	 * Used by collapsible sections to contract/expand the window when toggled.
	 * The window is clamped to a minimum of GetDefaultWindowSize() height.
	 * No-op if the window is not open.
	 */
	void ResizeWindowByDelta(float DeltaY);

	/** Get the preset being edited. May return null if the asset was GC'd. */
	USoundEffectPreset* GetPreset() const;

	// ── Dirty Marking ─────────────────────────────────────────────────────

	/**
	 * Call this BEFORE mutating any preset field from a knob/button/dropdown callback.
	 *
	 * Correct sequence in any editor lambda:
	 *   MarkPresetDirty();             // snapshot pre-change state for undo + show asterisk
	 *   P->Settings.X = newValue;
	 *   P->SetSettings(P->Settings);
	 *
	 * Internals:
	 *  1. Calls UObject::Modify() on the preset — records pre-change snapshot for undo.
	 *  2. Calls UPackage::MarkPackageDirty() — shows the asterisk in Content Browser / title.
	 *
	 * No other code anywhere in this plugin should call Modify() or MarkPackageDirty() directly.
	 */
	void MarkPresetDirty();

	// ── Virtual Interface — Subclass Overrides ────────────────────────────

	virtual FLinearColor GetAssetColor() const;

	/**
	 * Descriptive display name shown in the window title bar, e.g. "Source Delay",
	 * "Submix Reverb", "EQ". Defaults to empty string (falls back to asset file name).
	 * Override in each editor subclass.
	 */
	virtual FText GetEditorTitle() const { return FText::GetEmpty(); }

	/**
	 * Exact pixel size of the plugin window — NOT user-resizable.
	 * Subclasses override to specify their ideal size.
	 * The final size is multiplied by UAudioEffectsStudioSettings::ScaleWindowSize().
	 */
	virtual FVector2D GetDefaultWindowSize() const { return FVector2D(640, 300); }

	/**
	 * Public accessor for off-screen screenshot capture.
	 * Sets up the preset, preview engine, and builds the FULL window content
	 * (background + preset bar + transport + effect widget + meters).
	 * Does NOT create an OS window — returns the widget tree for off-screen rendering.
	 */
	TSharedRef<SWidget> BuildFullWidgetForScreenshot(USoundEffectPreset* InPreset);

	/**
	 * Store a knob style so it outlives the CreateEffectWidget() call.
	 * Returns a stable pointer suitable for SLATE_STYLE_ARGUMENT.
	 */
	const FAudioMaterialKnobStyle* StoreKnobStyle(FAudioMaterialKnobStyle&& Style);

	/**
	 * Return true to force all SAudioFXCollapsibleSection instances to start expanded.
	 * Used by screenshot variant subclasses to capture the "all expanded" state.
	 * Default: false (sections start collapsed per design language).
	 */
	virtual bool ShouldExpandAllSections() const { return false; }

	/**
	 * Call this BEFORE InitEffectEditor() or BuildFullWidgetForScreenshot() to lock
	 * the DPI scale to exactly 1.0 for this editor instance.
	 *
	 * Once set it CANNOT be unset — capture mode is permanent for the lifetime of the editor.
	 * This is called automatically by MakeEffectCapture() and BuildFullWidgetForScreenshot().
	 * Normal interactive editors never call this, so they always use the user's scale setting.
	 *
	 * NON-VIRTUAL BY DESIGN: no subclass can accidentally override the capture scale.
	 */
	void SetCaptureMode()  { bCaptureMode = true; }

protected:
	// ── Protected State & Delegates ───────────────────────────────────────

	/**
	 * Ready-to-use dirty-marking delegate, initialized in InitEffectEditor().
	 *
	 * Capture [this] in any lambda inside CreateEffectWidget() or helper methods
	 * and call MarkDirty.ExecuteIfBound() BEFORE mutating preset settings:
	 *
	 *   .OnValueChanged_Lambda([this, WeakPreset](float N)
	 *   {
	 *       MarkDirty.ExecuteIfBound();
	 *       if (auto* P = WeakPreset.Get()) { P->Settings.X = N; P->SetSettings(P->Settings); }
	 *   })
	 *
	 * Helper methods in derived classes access it directly — no parameter passing needed.
	 */
	FSimpleDelegate MarkDirty;

	// ── Protected Virtual Overrides ───────────────────────────────────────

	/**
	 * Override in derived classes to build the effect-specific widget.
	 */
	virtual TSharedRef<SWidget> CreateEffectWidget(USoundEffectPreset* InPreset);

	/**
	 * Unified bypass support.
	 *
	 * Override GetBypassState() to return the current bypass value from your preset's Settings.
	 * Override SetBypassState() to write it back (call MarkDirty + SetSettings inside).
	 * Default implementations return false / do nothing → no bypass button shown.
	 *
	 * When GetBypassState is overridden, BuildWindowContent() automatically shows the
	 * bypass button in the preset bar. No per-editor bypass widget needed.
	 */
	virtual bool GetBypassState() const { return false; }
	virtual void SetBypassState(bool /*bBypassed*/) {}

	/**
	 * Return true to show the bypass button in the preset bar.
	 * Default: true when GetBypassState is overridden (i.e. when the preset supports bypass).
	 * Base class always returns false; derived classes that override GetBypassState should
	 * also override this to return true.
	 */
	virtual bool ShouldShowBypass() const { return true; }

	/**
	 * Return true to show flanking input/output meter strips.
	 * Default true — all editors show meters. Override to false to suppress.
	 */
	virtual bool ShouldShowMeters() const { return false; }

	/**
	 * Return true to show gain-reduction meter strips (for dynamics-family editors).
	 */
	virtual bool ShouldShowGRMeter() const { return false; }

	/**
	 * Vertical alignment of the effect content area within the window.
	 * Default: VAlign_Center — looks good for simple fixed-layout effects.
	 * Override to VAlign_Top for editors with dynamic content (EQ bands,
	 * Multiband bands, TapDelay taps) to prevent the content from jumping
	 * vertically as elements are added/removed.
	 */
	virtual EVerticalAlignment GetContentVAlign() const { return VAlign_Top; }

	/** Accessor for derived editors that need preview engine state in custom widgets. */
	TSharedPtr<FAudioFXPreviewEngine> GetPreviewEngine() const { return PreviewEngine; }

private:
	// ── Private Helpers ───────────────────────────────────────────────────

	/** Build the full window content (preset bar + transport + effect + meters) */
	TSharedRef<SWidget> BuildWindowContent();

	/** Opens the standard UE Details/Properties window for this preset */
	void OnShowProperties();

	/** Closes this window and opens the default UE asset editor */
	void OnOpenDefaultEditor();

	/** Called when the SWindow is closed — cleans up the engine */
	void OnWindowClosed(const TSharedRef<SWindow>& ClosedWindow);

	// ── Private Members ───────────────────────────────────────────────────

	/** The preset being edited. TWeakObjectPtr — safe against GC while window is open. */
	TWeakObjectPtr<USoundEffectPreset> EditedPreset;

	/** The owned top-level window */
	TWeakPtr<SWindow> WindowPtr;

	/** The properties sub-window — tracked to prevent duplicate spawns */
	TWeakPtr<SWindow> PropertiesWindowPtr;

	/** Preview playback engine */
	TSharedPtr<FAudioFXPreviewEngine> PreviewEngine;

	/** Transport bar widget */
	TSharedPtr<SAudioFXTransportBar> TransportBar;

	/** Stable storage for knob styles */
	TArray<TUniquePtr<FAudioMaterialKnobStyle>> StoredKnobStyles;

	/** Handle for the UIScaleFactor change delegate — unsubscribed in destructor */
	FDelegateHandle ScaleChangeDelegateHandle;

	/**
	 * When true, ALL DPI scaling is locked to exactly 1.0.
	 * Set via SetCaptureMode() — non-virtual, cannot be overridden.
	 * Normal interactive editors never set this.
	 */
	bool bCaptureMode = false;

	/** Called when the user changes UIScaleFactor in settings — resizes the open window live */
	void OnUIScaleChanged(float NewScale);
};
