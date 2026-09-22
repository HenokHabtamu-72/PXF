# PXF

[![Build](https://github.com/HenokHabtamu-72/PXF/actions/workflows/build.yml/badge.svg)](https://github.com/HenokHabtamu-72/PXF/actions/workflows/build.yml)
[![License: AGPL v3](https://img.shields.io/badge/license-AGPL--3.0-8B5CF6.svg)](LICENSE)
[![JUCE 8](https://img.shields.io/badge/JUCE-8.0.4-3D5AFE.svg)](https://juce.com)
[![VST3 · AU · Standalone](https://img.shields.io/badge/VST3%20%C2%B7%20AU%20%C2%B7%20Standalone-B04CF7.svg)](docs/BUILD.md)

**A late-night software instrument.** Two engines share one master FX chain inside a
neon-purple, dark-navy interface:

- **SYNTH** — a warm 2-oscillator + sub subtractive synth playing 20 built-in presets,
  with full mono / legato note-sliding.
- **GRAIN** — a polyphonic granulizer: drop in any one-shot and play it as streams of
  overlapping grains, pitched from the keyboard.

VST3 instrument (stereo out, MIDI in), JUCE 8 / C++17, fixed 920 × 580 UI. Standalone and
macOS AU targets are included. Built to load, scan and run cleanly in **FL Studio 21+ on
Windows**.

---

## Quick start

You need **Visual Studio 2022** with the *Desktop development with C++* workload
(that ships the MSVC compiler and CMake — nothing else to install). From an
**x64 Native Tools Command Prompt for VS 2022**:

```bat
cmake --preset windows
cmake --build --preset windows
```

The first configure downloads JUCE 8.0.4 (~100 MB, once). When it finishes the plugin is at:

```
build\PXF_artefacts\Release\VST3\PXF.vst3
```

Copy that **whole `PXF.vst3` folder** into `C:\Program Files\Common Files\VST3\`, then in
FL Studio: **Options ▸ Manage plugins ▸ Find plugins** with *Verify plugins* ticked.

Full walk-through, plus macOS and Linux: **[docs/BUILD.md](docs/BUILD.md)**.

---

## SYNTH

8-voice polyphonic. Two oscillators (sine / triangle / saw / square) with an osc-mix and a
detune spread, a sine sub one octave down, one low-pass filter, and one ADSR. Oscillators
are band-limited with PolyBLEP/PolyBLAMP correction, so saws and squares stay clean all the
way up the keyboard.

### Mono & legato (note sliding)

The VOICE panel has a **Glide** knob and two switches:

| Control | What it does |
| --- | --- |
| **Glide** | How long a slide between notes takes (0 – 2000 ms). |
| **MONO** | One voice, last-note priority. Play a new note while another is held and the pitch *slides* to it over the Glide time. Release the top note and it slides back down to the note still held. |
| **LEGATO** | Shapes the envelope, not the pitch. On: only the first note of a phrase triggers the attack — overlapping notes slide smoothly with no re-attack. Off: every note re-attacks (from the current level, so it never clicks) while the pitch still slides. |

Notes played after a full release always start on pitch — you slide by *overlapping* notes,
the way a player expects.

With MONO off the synth is 8-voice polyphonic; Glide then acts as classic poly portamento
from the previously played note.

The **basses, flutes and leads** among the presets ship with MONO + LEGATO on and a musical
Glide time, so the slide is one arrow-click away.

---

## The 20 presets

Exactly twenty, compiled into the binary, available on first load. There is deliberately no
user preset saving. Browse with the ◀ ▶ arrows in the header, or click the preset name for
the full menu.

| Category | Presets |
| --- | --- |
| **BASS** | Midnight Sub · Purple Bass · Deep End |
| **PLUCK** | Rain Pluck · Neon Drip · Glass Pluck |
| **SYNTH** | Dusk Synth · Haze |
| **PAD** | 2AM Pad · Violet Sky · Slow Fade |
| **BELL** | Night Bell · Chime Glow |
| **FLUTE** | Air Flute · Lofi Flute |
| **PIANO** | Dusty Keys · Felt Piano |
| **LEAD** | Neon Lead · Skyline · Cruise Control |

Everything is voiced warm, mellow and slightly lo-fi — a late-night chill / lofi hip-hop
palette. Pianos, bells and flutes are synthesized approximations (filtered triangles for
flutes, short-decay harmonic stacks for bells and keys, the Dust knob for realism):
character over accuracy, no sample libraries. Per-preset notes are in
**[docs/PRESETS.md](docs/PRESETS.md)**.

---

## GRAIN in 20 seconds

Press **LOAD** (or drag a WAV / AIFF / FLAC / MP3 onto the waveform — mono or stereo, any
sample rate, resampled internally). Click or drag on the waveform to set the read
**Position**; the glowing dots are the live grain read-heads.

| Control | Range | What it does |
| --- | --- | --- |
| **Size** | 10 – 500 ms | Grain length. |
| **Density** | 1 – 100 /s | How many overlapping grains spawn per second. |
| **Position** | 0 – 100 % | Read point in the sample. |
| **Spray** | 0 – 100 % | Random jitter around that read point. |
| **Pitch** | ±24 st | Transposition on top of keyboard tracking. |
| **Shape** | 0 – 100 % | Grain window morph, smooth Hann → plucky attack. |

**C4 plays the sample at its original pitch.** Each held note runs its own grain stream, so
chords work. Every grain is windowed to zero at both ends, so grain boundaries never click.

---

## Master FX (shared by both engines)

Ping-pong **delay** synced to the host tempo (1/16 … 1/2) → **reverb** → **DUST** → volume.

**DUST** is the lo-fi character knob. One turn adds tape wow and flutter, gentle saturation,
vinyl hiss and sparse crackle, and eases the top end off. At zero it is transparent (the
4 ms wow read-offset is reported to the host as latency, so the DAW compensates).

---

## UI — "Late Night Chill"

Near-black navy vertical gradient `#0A0E27 → #131A3D` under a subtle film-grain overlay.
Neon purple `#B04CF7 / #8B5CF6` accents with soft outer glow; GRAIN elements glow deep blue
`#3D5AFE` instead. Dim lavender text `#C9C4E8`, muted labels `#6E6A8F`, uppercase
micro-labels with wide letter-spacing. Flat and modern — no skeuomorphism, no clutter.
2 a.m. studio, dim room, a neon sign in the window.

---

## Repository layout

```
CMakeLists.txt          the project (downloads JUCE automatically)
CMakePresets.json       ready-made configure/build/test presets
Source/
  DspUtils.h            band-limited oscillators, TPT filter, delay line
  Parameters.*          every automatable parameter
  SynthEngine.*         voices, the mono/legato note stack, glide
  GrainEngine.*         polyphonic granular engine + file loading
  FxChain.*             delay → reverb → Dust → volume
  Presets.*             the 20 patches
  PluginProcessor.*     MIDI routing, tabs, state, preset browser
  Theme.h               palette, fonts, glow helpers
  PxfLookAndFeel.*      the glowing knob and combo styling
  UIComponents.*        cards, knob cells, chips, waveform display
  PluginEditor.*        the 920 × 580 layout
Tests/
  EngineTests.cpp       headless audio tests
docs/
  BUILD.md              build & install, per platform
  SPEC.md               the original product specification
  ARCHITECTURE.md       signal flow, threading, design notes
  PRESETS.md            what each of the 20 patches is for
```

---

## Tests

A headless binary renders real audio through both engines and checks it, rather than just
checking that the code compiles:

```bash
cmake --preset linux-tests      # or windows-tests / macos-tests
cmake --build --preset linux-tests
ctest --preset linux
```

What it asserts: the mono glide measurably slides 130.8 → 261.6 Hz and lands on pitch;
releasing the top note falls back to the note still held; legato on/off changes the envelope
and not the pitch; grains track the keyboard (C4 ≈ 440 Hz and C5 ≈ 880 Hz for a 440 Hz
source); voices fall silent after release; and no output sample is ever NaN or Inf.

CI runs this on Windows, macOS and Linux for every push — see the badge at the top.

---

## Design goals

- No feature creep. Two engines, 20 presets, one FX chain, one screen.
- Smooth all parameter changes, protect against denormals, no clicks anywhere.
- A held pad chord or an active grain stream stays under ~8 % CPU on a modern machine.

The full original specification is preserved in **[docs/SPEC.md](docs/SPEC.md)**.

---

## Licence

Copyright © 2026 Henok Habtamu.

PXF is released under the **GNU Affero General Public License v3.0** — see [LICENSE](LICENSE).

PXF links [JUCE](https://juce.com), which is dual-licensed under AGPLv3 and a commercial
licence. AGPLv3 is the licence that keeps this build legal without a JUCE subscription, and
it is why the JUCE splash screen is left on in `CMakeLists.txt`. If you hold a paid JUCE
licence you may set `JUCE_DISPLAY_SPLASH_SCREEN=0` there and relicense your own fork
accordingly.
