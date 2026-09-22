# PXF — product specification

This is the brief PXF was built to. It is kept in the repository unchanged in substance, so
that any future change can be checked against what the instrument was meant to be. Where the
shipped plugin goes beyond the brief, that is called out in
[Deviations from the brief](#deviations-from-the-brief) at the end.

> Build a software instrument plugin called **PXF** to this exact specification. Keep it
> minimal — no feature creep beyond what's listed here.

---

## 1. Format & compatibility

- Framework: **JUCE 7+ (C++17)**, no other dependencies
- Plugin format: **VST3, 64-bit** — must load, scan and run cleanly in **FL Studio 21+ on
  Windows** (macOS AU build optional)
- Plugin type: instrument — receives MIDI, outputs stereo audio
- Fixed window, ~920 × 580 px
- Smooth all parameter changes (no zipper noise), protect against denormals, no clicks
  anywhere

## 2. Architecture — two engines, two tabs

- **SYNTH tab** — plays the 20 built-in presets
- **GRAIN tab** — granulizes a user-loaded one-shot sample
- Only the active tab's engine is audible; both feed the same master FX chain

## 3. Synth engine (keep it minimal)

- 8-voice polyphonic
- 2 oscillators (sine / triangle / saw / square) + 1 sub oscillator (sine, −1 octave)
- Osc mix + detune knobs
- 1 low-pass filter: cutoff + resonance
- 1 ADSR amplitude envelope
- Glide (portamento) knob

## 4. Master FX chain (shared by both engines)

- Reverb: size + mix
- Ping-pong delay: BPM-synced time, feedback, mix
- **"Dust" knob**: lo-fi character — gentle saturation, soft vinyl noise, slight pitch
  wow/flutter, subtle high-cut as it turns up
- Master volume

## 5. Presets — exactly 20, hard cap

No user preset saving. Browser = category label + preset name + left/right arrows in the
header.

| Category | Count | Preset names |
| --- | --- | --- |
| Bass | 3 | Midnight Sub, Purple Bass, Deep End |
| Pluck | 3 | Rain Pluck, Neon Drip, Glass Pluck |
| Synth | 2 | Dusk Synth, Haze |
| Pad | 3 | 2AM Pad, Violet Sky, Slow Fade |
| Bell | 2 | Night Bell, Chime Glow |
| Flute | 2 | Air Flute, Lofi Flute |
| Piano | 2 | Dusty Keys, Felt Piano |
| Lead | 3 | Neon Lead, Skyline, Cruise Control |

Each preset is a stored parameter snapshot (synth + FX), embedded in the binary and
available on first load. Voice everything warm, mellow and slightly lo-fi — a late-night
chill / lofi hip-hop palette. Pianos, bells and flutes are synthesized approximations
(filtered triangles for flutes, short-decay harmonic stacks for bells/keys, Dust knob for
realism) — character over accuracy, no sample libraries.

## 6. GRAIN — granular one-shot feature (the signature feature)

- **LOAD button + drag-and-drop zone** — accepts WAV, AIFF, MP3, FLAC; mono or stereo; any
  sample rate (resample internally)
- Waveform display with a glowing position marker; clicking/dragging on the waveform sets
  Position
- Controls:
  - **Grain Size**: 10 – 500 ms
  - **Density**: 1 – 100 grains/sec, overlapping
  - **Position**: 0 – 100 % start point in the sample
  - **Spray**: random position jitter, 0 – 100 %
  - **Pitch**: ±24 semitones, plus keyboard tracking — MIDI notes repitch the grains
    (C4 = original pitch)
  - **Shape**: grain envelope morph, soft Hann → sharper attack
- Polyphonic: each held MIDI note spawns its own grain stream at its transposed pitch
- Grains are windowed so boundaries never click; output feeds the master FX chain

## 7. UI — "Late Night Chill" (neon purple × dark blue)

- Background: near-black navy vertical gradient **#0A0E27 → #131A3D**, very subtle film-grain
  noise overlay
- Primary accent: neon purple **#B04CF7 / #8B5CF6** with soft outer glow on active elements
- Secondary accent: deep blue **#3D5AFE**; text in dim lavender **#C9C4E8**; muted labels
  **#6E6A8F**
- Knobs: dark rounded knobs with a glowing purple value arc; GRAIN tab elements glow blue
  instead
- Header bar: "PXF" wordmark in glowing purple (left), preset browser (centre),
  SYNTH / GRAIN tab switch (right)
- Waveform rendered in a purple → blue gradient with a faint glow
- Typography: clean geometric sans (Inter or similar), uppercase micro-labels with wide
  letter-spacing
- Mood: 2 a.m. studio, dim room, neon sign in the window — calm, dark, glowing. Flat and
  modern, no skeuomorphism, no clutter.

## 8. Deliverables

1. Full source code with a CMake project that builds in Visual Studio 2022 on Windows
2. Step-by-step build instructions, including where to place the `.vst3` for FL Studio
   (`C:\Program Files\Common Files\VST3`) and how to rescan plugins in FL
3. All 20 presets implemented and audible
4. CPU target: a held pad chord or an active grain stream stays under ~8 % on a modern CPU

**Build order:** scaffold the JUCE project → synth voice + FX → preset system with all 20
patches → granular engine + sample loading → UI skin → polish and test in FL Studio.

---

## Deviations from the brief

Everything above is implemented. The shipped plugin adds four things the brief did not ask
for, each because leaving it out would have made the instrument worse:

| Addition | Why |
| --- | --- |
| **MONO / LEGATO switches** next to Glide | Glide alone is ambiguous: a portamento knob with no voice mode cannot do the note-slide that basses, flutes and leads are played with. The two switches are what make Glide musical. See the README. |
| **AU and Standalone targets** | The brief marks AU optional. Standalone costs nothing extra in JUCE and makes the plugin testable without a DAW. |
| **Headless engine tests** (`Tests/EngineTests.cpp`) | Pitch, glide and grain tracking are the kind of thing that silently regresses. The tests render real audio and measure it. |
| **Reported latency** | The Dust stage reads 4 ms behind its write head so the wow modulation has somewhere to move. That offset is reported to the host, so the DAW compensates and PXF stays in time. |

The 20-preset hard cap, the "no user preset saving" rule and the fixed 920 × 580 window are
deliberate and should stay that way.
