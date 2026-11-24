<p align="center">
  <img src="https://audiofactory.studio/oss/audio-effects-studio/screenshots/Source_EQ__4_Bands.png" width="720" alt="Audio Effects Studio — Parametric EQ">
</p>

<h1 align="center">Audio Effects Studio — User Documentation</h1>

<p align="center">
  <strong>DAW-grade visual editors for every built-in Unreal Engine audio effect.</strong><br>
  <em>By <a href="https://audiofactory.studio">AudioFactory</a></em>
</p>

---

## What Is Audio Effects Studio?

Unreal Engine ships with a powerful suite of built-in audio effects — EQ, dynamics, reverb, delay, filtering, distortion, and more. By default, editing any of these means staring at a flat property grid full of raw numbers.

**Audio Effects Studio** replaces every one of those property grids with a purpose-built visual editor inspired by professional DAW plugins like FabFilter, Waves, and iZotope. Think knobs, curves, and meters — not text boxes.

You get a full VST-style panel for every built-in effect, without touching engine code or changing your project setup. Just install the plugin and double-click.

---

## Getting Started

### Installation

```
YourProject/
└── Plugins/
    └── AudioEffectsStudio/     ← Copy this folder here
```

1. Copy the `AudioEffectsStudio` folder (the root of this repository) into your project's `Plugins/` directory.
2. Open your project. Unreal asks to build the plugin from source the first time; accept.
3. Double-click any audio effect preset asset in the Content Browser.

No configuration required and no engine modifications. This repository ships source code only, so the plugin is compiled on your machine (Visual Studio or the platform toolchain for your engine version is needed).

### Requirements

| Requirement | Version |
|-------------|---------|
| Unreal Engine | **5.6** |
| C++ toolchain | The one Unreal requires for your platform |
| AudioWidgets plugin | Enabled by default in UE |
| Synthesis plugin | Enabled by default in UE |

---

## Opening an Effect Editor

### The Normal Way — Double-Click

Find any audio effect preset asset in the **Content Browser** — a Source Effect or a Submix Effect — and **double-click** it. Instead of the standard property grid, the Audio Effects Studio editor opens: a floating, resizable window with knobs, sliders, and real-time visualizations, just like opening a VST plugin in your DAW.

<p align="center">
  <img src="https://audiofactory.studio/oss/audio-effects-studio/screenshots/Submix_Reverb.png" width="480" alt="Submix Reverb Editor">
  <img src="https://audiofactory.studio/oss/audio-effects-studio/screenshots/Source_Filter.png" width="480" alt="Source Filter Editor">
</p>

### The Fallback — Ctrl + Double-Click

Hold **Ctrl** while double-clicking to skip the custom editor entirely and open the standard Unreal Engine property editor instead. This is useful when you need raw access to every engine property, or when you're debugging something and want the classic grid.

You can also switch to the standard editor any time from inside the Audio Effects Studio window by clicking the **Open in Default Editor** button in the top toolbar.

> **Tip:** The two editors share the same underlying asset data — any changes you make in either one are live and immediately reflected in the other.

---

## The Editor Window

Every effect editor follows the same general layout:

| Zone | What It Does |
|------|-------------|
| **Preset Bar** (top) | Asset name, Save button, Browse-to-Asset, and Open-in-Default-Editor |
| **Visualization** | Real-time graph specific to the effect type (EQ curve, filter response, reverb decay, etc.) |
| **Knob Section** | Primary parameters — frequency, gain, time, mix, ratio, and so on |
| **Collapsible Sections** | Advanced controls (sidechain, modulation, response curves) tucked away by default |

### Controls

- **Knobs** — Click and drag up/down to change the value. The current value is always displayed beneath the knob.
- **Ctrl + Click a knob** — Resets it to its default value instantly.
- **Dropdowns and buttons** — Used for filter type, processor type, and other discrete choices.

---

## Effect Reference

### Source Effects (applied per Sound)

| Effect | What It Does |
|--------|-------------|
| **Bit Crusher** | Sample rate and bit depth reduction for lo-fi and glitch textures |
| **Chorus** | Pitch-modulated doubling with configurable depth and spread |
| **Convolution Reverb** | Impulse-response reverb with IR file picker and wet/dry mix |
| **Dynamics Processor** | Full-featured compressor, limiter, expander, or gate with transfer curve display |
| **Envelope Follower** | Extracts the amplitude envelope of a signal for sidechain-style modulation |
| **EQ** | Parametric equalizer with up to 8 bands and a live frequency response curve |
| **Foldback Distortion** | Wavefold distortion for aggressive harmonic saturation |
| **Mid-Side Spreader** | Width control via M/S processing |
| **Motion Filter** | Two filter modules that can be chained in serial or run in parallel, with independent LFOs |
| **Panner** | Stereo panning with distance rolloff and angle control |
| **Phaser** | LFO-driven phase sweeping with feedback |
| **Ring Modulation** | Carrier-frequency amplitude modulation for metallic, inharmonic tones |
| **Simple Delay** | Single-tap delay with time, feedback, and wet/dry |
| **Source Filter** | Single LP/HP/BP/Notch filter with a live frequency response curve |
| **Stereo Delay** | Independent left/right delay times with optional ping-pong |
| **Wave Shaper** | Waveshaping with a choice of transfer curve algorithms |

### Submix Effects (applied to a Submix bus)

| Effect | What It Does |
|--------|-------------|
| **Convolution Reverb** | Bus-level impulse-response reverb |
| **Delay** | Simple single-tap bus delay |
| **Dynamics** | Bus compressor, limiter, expander, or gate |
| **EQ** | Parametric bus EQ with up to 8 bands |
| **Filter** | Bus LP/HP/BP/Notch filter |
| **Flexiverb** | Lightweight algorithmic reverb with decay and damping |
| **Multiband Compressor** | Per-band compression with up to 4 frequency bands |
| **Reverb** | Full algorithmic reverb with early reflections and late reverb controls |
| **Stereo Delay** | Bus-level stereo delay with ping-pong |
| **Stereo-to-Quad** | Upmix stereo to four-channel output |
| **Tap Delay** | Multi-tap delay with individual time and gain per tap |

---

## Visualization Graphs

Audio Effects Studio includes several real-time graphs drawn directly in the editor — no extra materials or render passes. Each graph updates as you move knobs.

### Parametric EQ Curve

Used in the **EQ editors** (Source and Submix). Displays the frequency response across the full audible range (20 Hz – 20 kHz) on a logarithmic frequency axis. Each band draws its own colored contribution curve at reduced opacity, while a white combined-sum curve shows the final overall response. A subtle grid marks key frequency octaves and a ±12 dB scale.

Add and remove bands with the **+** and **×** buttons in the band cards below the curve. The curve rebuilds in real time.

### Filter Frequency Response

<p align="center">
  <img src="https://audiofactory.studio/oss/audio-effects-studio/screenshots/Source_Filter.png" width="480" alt="Filter Response">
</p>

Used in the **Filter editors** (Source and Submix) and **Motion Filter**. Shows the magnitude response of the current filter type (Low-Pass, High-Pass, Band-Pass, or Notch) including the resonance peak at the cutoff frequency. As you move the Cutoff and Resonance knobs, the curve moves with them.

### Dynamics Transfer Curve

<p align="center">
  <img src="https://audiofactory.studio/oss/audio-effects-studio/screenshots/Source_Dynamics.png" width="480" alt="Dynamics Transfer Curve">
</p>

Used in the **Dynamics editors** (Source and Submix). Shows the classic input/output gain transfer graph: diagonal unity line, threshold position, and the knee and ratio applied above it. The shape changes automatically when you switch between Compressor, Limiter, Expander, and Gate modes.

### Reverb Decay Graph

<p align="center">
  <img src="https://audiofactory.studio/oss/audio-effects-studio/screenshots/Submix_Reverb.png" width="480" alt="Reverb Decay">
</p>

Used in the **Submix Reverb** editor. Displays a time-domain envelope of the reverb tail: a spike for early reflections, a smooth exponential decay for the late reverb body, and a secondary HF-decay overlay showing how high frequencies fade faster than low ones. Adjust Decay Time and Reflections knobs and watch the curve reshape.

### Multiband Crossover Visualization

<p align="center">
  <img src="https://audiofactory.studio/oss/audio-effects-studio/screenshots/Submix_Multiband__3_Bands.png" width="600" alt="Multiband Compressor">
</p>

Used in the **Multiband Compressor** editor. Draws the frequency spectrum divided into up to four colored bands, with filled areas, crossover overlap zones, and gain envelopes per band. The crossover points move as you adjust the frequency split controls.

### Tap Delay Graph

<p align="center">
  <img src="https://audiofactory.studio/oss/audio-effects-studio/screenshots/Submix_TapDelay__3_Taps.png" width="600" alt="Tap Delay Graph">
</p>

Used in the **Tap Delay** editor. Bar visualization showing each tap's delay time on the horizontal axis and its gain on the vertical axis. Bars are color-coded by tap index. Add taps with the **+ Tap** button; each new tap gets its own card for time and gain control below the graph.

### Convolution Reverb Impulse Response

Used in the **Convolution Reverb editors** (Source and Submix). Displays the loaded impulse response as a waveform-style preview, giving you a visual sense of the IR length and shape before auditioning.

---

## Themes

Three color themes are available from **Edit → Project Settings → Plugins → Audio Effects Studio**:

| Theme | Character |
|-------|-----------|
| **Dark** | Deep blacks with vivid accent colors — the default, designed for long editing sessions |
| **Light** | Clean light backgrounds for bright monitor environments |
| **Unreal** | Muted grays matching UE5's native Starship chrome — blends into the editor naturally |

Changes apply immediately without restarting the editor.

---

## Tips and Workflow Notes

- **The editor is non-destructive.** Every value you change is saved back to the underlying preset asset exactly as if you had typed it into the property grid. Undo/Redo works as normal.
- **Floating windows.** Each effect opens in its own floating panel. You can have multiple effects open side by side, just like plugin windows in a DAW.
- **Collapsible sections.** Many editors have advanced sections (Sidechain, Response, Modulation, Advanced) that are collapsed by default to keep the interface clean. Click the section header to expand them.
- **Band rack editors** (EQ, Multiband Compressor, Tap Delay) start empty and grow as you add bands or taps. A prominent **+** button is shown when no bands exist yet.
- **Ctrl + Click any knob** to snap it back to its factory default value.

---

## Frequently Asked Questions

**Does this change how effects sound?**
No. Audio Effects Studio only changes the *editor* for effect assets. The audio processing pipeline is unmodified.

**Does it work with custom or third-party effects?**
The plugin covers all 27 built-in UE audio effects. Third-party effects not registered with Audio Effects Studio will continue to open with the standard Unreal property editor.

**Can I still use the standard property editor?**
Yes — hold **Ctrl** while double-clicking any effect asset, or click **Open in Default Editor** inside the Audio Effects Studio window.

**Does this require a C++ build?**
Yes. This is a source-only release, so Unreal compiles the plugin the first time you open the project. The plugin is an editor module; it adds nothing to packaged games.

---

<p align="center">
  <strong>Built by <a href="https://audiofactory.studio">AudioFactory</a></strong><br>
  <em>R&D studio specializing in custom Unreal Engine audio tooling.</em><br>
  <br>
  <a href="https://audiofactory.studio">audiofactory.studio</a>
</p>
