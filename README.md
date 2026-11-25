<p align="center">
  <img src="https://github.com/ManuelSoruco/AudioEffectsStudio/releases/download/v1.0.0/Source_EQ__4_Bands.png" width="720" alt="Audio Effects Studio, Parametric EQ">
</p>

<h1 align="center">Audio Effects Studio</h1>

<p align="center">
  <strong>DAW-grade visual editors for every built-in Unreal Engine audio effect.</strong><br>
  <em>By <a href="https://audiofactory.studio">AudioFactory</a>. Open source under the MIT License.</em>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Engine-Unreal%205.6-blue" alt="Unreal Engine 5.6">
  <img src="https://img.shields.io/badge/Effects-27%20Editors-green" alt="27 editors">
  <img src="https://img.shields.io/badge/License-MIT-yellow" alt="MIT">
  <img src="https://img.shields.io/badge/Type-Editor%20Plugin-purple" alt="Editor plugin">
</p>

---

This repository is version 1.0.0 of the plugin, released as source code. It targets **Unreal Engine 5.6**.

## The problem

Unreal Engine ships a powerful real-time audio engine with Source Effects and Submix Effects for EQ, dynamics, reverb, delay, filtering, distortion and more. The editing experience is a flat property grid: no knobs, no curves, no meters.

## The solution

Audio Effects Studio replaces the default editor for all 27 built-in audio effect presets with purpose-built visual editors:

- **Knob-driven controls** with logarithmic scaling, formatted value readouts and Ctrl+Click reset to default
- **Real-time graphs** such as EQ curves, filter responses, dynamics transfer curves and stereo meters
- **Colour-coded parameters** so frequency, gain, time, mix and dynamics each read at a glance
- **One consistent layout** across every effect

<p align="center">
  <img src="https://github.com/ManuelSoruco/AudioEffectsStudio/releases/download/v1.0.0/Source_Dynamics.png" width="420" alt="Dynamics Processor">
  <img src="https://github.com/ManuelSoruco/AudioEffectsStudio/releases/download/v1.0.0/Source_Filter.png" width="420" alt="Source Filter with frequency response">
</p>

## Effects covered

**16 Source Effects:** Bit Crusher, Chorus, Convolution Reverb, Dynamics Processor, Envelope Follower, EQ, Foldback Distortion, Mid-Side Spreader, Motion Filter, Panner, Phaser, Ring Modulation, Simple Delay, Source Filter, Stereo Delay, Wave Shaper

**11 Submix Effects:** Convolution Reverb, Delay, Dynamics, EQ, Filter, Flexiverb, Multiband Compressor, Reverb, Stereo Delay, Stereo-to-Quad, Tap Delay

## Visualisations

Graphs are drawn in plain Slate, with no materials or textures:

- Parametric EQ curve with per-band and combined response
- Filter response for low-pass, high-pass, band-pass and notch
- Dynamics transfer curve with threshold, ratio and knee
- Reverb decay graph (Submix Reverb)
- Multiband crossover view (Multiband Compressor)
- Tap delay graph (Tap Delay)
- Gain reduction metering and stereo level meters with peak hold

<p align="center">
  <img src="https://github.com/ManuelSoruco/AudioEffectsStudio/releases/download/v1.0.0/Submix_Multiband__3_Bands.png" width="420" alt="Multiband Compressor">
  <img src="https://github.com/ManuelSoruco/AudioEffectsStudio/releases/download/v1.0.0/Submix_Reverb.png" width="420" alt="Submix Reverb">
</p>

## Workflow

- **Drop-in replacement.** Double-click any audio effect asset in the Content Browser and its Audio Effects Studio editor opens.
- **Classic view fallback.** Hold **Ctrl** while double-clicking to open the standard property editor, or use **Open in Default Editor** in the toolbar.
- **Collapsible sections** keep advanced parameters out of the way.
- **Band racks.** EQ, Multiband Compressor and Tap Delay grow and shrink as you add or remove bands and taps.
- **Preset bar.** Every editor has a header bar with save, browse-to-asset and show-properties buttons.

### Themes

Three colour themes are available under Project Settings, Plugins, Audio Effects Studio: **Dark** (default), **Light** and **Unreal** (muted greys matching the editor).

## Installation

This is a source-only plugin. Unreal builds it on your machine.

1. Copy this repository into your project as `Plugins/AudioEffectsStudio/` (so that `AudioEffectsStudio.uplugin` sits directly inside that folder).
2. Open the project. When Unreal offers to build the plugin, accept.
3. Double-click any audio effect preset asset in the Content Browser.

```
YourProject/
`-- Plugins/
    `-- AudioEffectsStudio/
        |-- AudioEffectsStudio.uplugin
        `-- Source/
```

### Requirements

- Unreal Engine **5.6**
- The C++ toolchain Unreal requires for your platform
- The **AudioWidgets** and **Synthesis** plugins (enabled by default in the engine)

The plugin is an editor-only module. It adds nothing to packaged games.

## Gallery

<p align="center">
  <img src="https://github.com/ManuelSoruco/AudioEffectsStudio/releases/download/v1.0.0/Source_Chorus.png" width="280" alt="Chorus">
  <img src="https://github.com/ManuelSoruco/AudioEffectsStudio/releases/download/v1.0.0/Source_Phaser.png" width="280" alt="Phaser">
  <img src="https://github.com/ManuelSoruco/AudioEffectsStudio/releases/download/v1.0.0/Source_RingMod.png" width="280" alt="Ring Modulation">
</p>
<p align="center">
  <img src="https://github.com/ManuelSoruco/AudioEffectsStudio/releases/download/v1.0.0/Submix_StereoDelay.png" width="280" alt="Stereo Delay (Submix)">
  <img src="https://github.com/ManuelSoruco/AudioEffectsStudio/releases/download/v1.0.0/Submix_TapDelay__3_Taps.png" width="280" alt="Tap Delay">
  <img src="https://github.com/ManuelSoruco/AudioEffectsStudio/releases/download/v1.0.0/Source_FoldbackDist.png" width="280" alt="Foldback Distortion">
</p>
<p align="center">
  <img src="https://github.com/ManuelSoruco/AudioEffectsStudio/releases/download/v1.0.0/Source_MotionFilter.png" width="280" alt="Motion Filter">
  <img src="https://github.com/ManuelSoruco/AudioEffectsStudio/releases/download/v1.0.0/Submix_Flexiverb.png" width="280" alt="Flexiverb">
  <img src="https://github.com/ManuelSoruco/AudioEffectsStudio/releases/download/v1.0.0/Source_StereoDelay.png" width="280" alt="Stereo Delay (Source)">
</p>

The pictures are hosted outside this repository.

## Documentation

See [DOCUMENTATION.md](DOCUMENTATION.md) for the full user guide.

## Project structure

```
AudioEffectsStudio/
|-- AudioEffectsStudio.uplugin
|-- Config/
`-- Source/AudioEffectsStudio/
    |-- AudioEffectsStudio.Build.cs
    |-- Public/                     # Module, settings and shared widget headers
    `-- Private/
        |-- AudioEffectsStudioModule.cpp   # Registers the editors for all 27 effect types
        |-- Shared/                 # Theme, base editor, knobs, curves, meters
        |-- SourceEffects/          # 16 source effect editors
        `-- SubmixEffects/          # 11 submix effect editors
```

The UI theme and every widget live inside this plugin. There are no submodules and no other plugins to install.

## Tests

This release has no automated tests. Contributions that add them are welcome.

## Contributing

Issues and pull requests are welcome, for example support for third-party effect plugins, new visualisations or new themes.

## License

MIT License. See [LICENSE](LICENSE).

---

<p align="center">
  <strong>Built by <a href="https://audiofactory.studio">AudioFactory</a></strong><br>
  <em>R&amp;D studio specialising in custom Unreal Engine audio tooling.</em>
</p>
