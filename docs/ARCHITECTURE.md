# Architecture

How PXF is put together, and why. Read this before changing anything in `Source/`.

---

## Signal flow

```
            MIDI in
               │
        ┌──────┴──────┐          only the active tab's engine
        ▼             ▼          is fed notes and rendered
   SynthEngine    GrainEngine
   (8 voices)     (8 streams)
        └──────┬──────┘
               ▼
          ┌─────────┐
          │ FxChain │   ping-pong delay (BPM-synced)
          │         │        ▼
          │         │      reverb
          │         │        ▼
          │         │       DUST   wow/flutter · saturation · high-cut · vinyl noise
          │         │        ▼
          │         │   master volume → soft clip
          └────┬────┘
               ▼
          stereo out
```

`PxfAudioProcessor::processBlock` clears the buffer, pulls parameters once, splits the block
at every MIDI timestamp so note events are sample-accurate, renders the active engine into
each sub-range, then runs the whole block through `FxChain`.

Switching tabs releases every note on both engines, so a held chord never hangs on the
engine you just left.

---

## Modules

| File | Responsibility |
| --- | --- |
| `DspUtils.h` | The primitives: sine table, PolyBLEP/PolyBLAMP band-limiting, `renderWave`, soft clip, a Zavalishin TPT state-variable filter, one-pole low-pass, fractional-delay line. Header-only, depends on `juce_core` alone. |
| `Parameters.*` | Parameter IDs, ranges and the `AudioProcessorValueTreeState` layout. The single source of truth for anything automatable. |
| `Presets.*` | A `constexpr` array of 20 `PresetDef` structs. Pure data — no JUCE types, no allocation. |
| `SynthEngine.*` | `SynthVoice` (oscillators, filter, ADSR, glide) and `SynthEngine` (voice allocation, the mono note stack). |
| `GrainEngine.*` | Sample decoding and resampling, peak computation for the display, per-voice grain streams, the grain window table. |
| `FxChain.*` | Delay → reverb → Dust → volume, all parameter-smoothed. |
| `PluginProcessor.*` | Hosting: buses, MIDI routing, tab state, preset loading, session save/restore. |
| `Theme.h` | Palette, font selection, letter-spaced text drawing, the glow helpers. |
| `PxfLookAndFeel.*` | The glowing rotary and combo box. One instance per accent colour. |
| `UIComponents.*` | `CardPanel`, `KnobCell`, `ChipButton`, `TabChip`, `ArrowChip`, `WaveformDisplay`. |
| `PluginEditor.*` | The fixed 920 × 580 layout, the header, and the 30 Hz refresh timer. |

---

## Threading

Three threads touch the plugin, and the rules between them are the part most worth
preserving.

**Audio thread** — `processBlock` and everything below it. It never allocates, never locks
blockingly, and never touches the file system. `GrainEngine::render` takes a
`SpinLock::ScopedTryLockType` on the sample buffer: if the message thread happens to be
swapping a new sample in, the audio thread renders silence for that one block rather than
waiting.

**Message thread** — file loading and the editor. `GrainEngine::loadFile` decodes and
resamples into a scratch buffer, computes display peaks, and only then takes the spin lock
to swap the finished buffer in. Disk I/O therefore never happens under the lock.

**Host automation** — parameters are plain `std::atomic<float>*` reads pulled once per block
in `pullParameters()`. Tab index and preset index are `std::atomic<int>`.

Grain read-head positions for the waveform display are published through an array of
`std::atomic<float>` (`vizPos`) written by the audio thread and read by the editor's timer.
A torn read there is a dot one frame late — harmless by construction.

---

## Parameters and presets

Every automatable value lives in `createParameterLayout()`. Adding one means adding an ID in
`pid`, a range in the layout, a field in the relevant `*Params` struct, a line in
`pullParameters()`, and a knob in the editor — in that order.

A preset is a parameter snapshot, not a separate state: `loadPreset` writes each stored
value through `setValueNotifyingHost`, so the host sees the change, automation lanes stay
consistent, and undo works. That is also why there is no user preset saving to maintain —
the DAW already saves the whole parameter set with the session.

`getStateInformation` adds three things on top of the APVTS tree: the preset index, the
active tab, and the absolute path of the loaded grain sample, which is re-read on restore if
the file still exists.

---

## DSP notes worth knowing

**Oscillators** are band-limited with PolyBLEP (saw, square) and PolyBLAMP (triangle)
correction, so there is no aliasing scream at the top of the keyboard. Pitch, filter
coefficients and gains are recomputed once per 16-sample chunk — a ~3 kHz control rate,
which is smooth for glides and effectively free.

**Glide** is constant-*time*, not constant-rate: whatever the interval, the slide takes the
Glide knob's duration and lands exactly on the target note. `advanceGlide` snaps to the
target on the final chunk so there is no asymptotic drift.

**Voice stealing** never resets a voice's oscillator phases or filter state while it is
sounding. A stolen or legato-retriggered voice keeps its state and the envelope climbs from
the current level, which is why nothing clicks.

**Grains** are laid out so a whole grain fits inside the sample when the transposed span
allows it, and wrap only when it does not. Each grain is windowed to zero at both ends by a
512-point table, rebuilt only when the Shape knob actually moves. Output is scaled by
`1/sqrt(overlap)` so density changes do not change perceived loudness. There is a hard cap
of 220 simultaneous grains, which is what bounds worst-case CPU.

**Ping-pong delay**: input lands on the left line, the left tap feeds the right, the right
tap feeds back to the left. The feedback knob is square-rooted because the loop crosses
twice per cycle, so one full L→R→L cycle decays by exactly the knob amount. A `1e-20`
denormal guard is written into both lines.

**Dust** reads 4 ms behind its write head so the wow modulation has room to move in both
directions; that fixed offset is reported through `setLatencySamples`, so the host
compensates. The vinyl noise is gated by whether the engine is actually sounding, so an idle
instrument is silent.

---

## Performance

The budget in the brief is ~8 % of one modern core for a held pad chord or an active grain
stream. The things holding that line:

- control-rate updates every 16 samples rather than per sample
- the global grain cap (220) and per-voice cap (48)
- a table-driven sine and a table-driven grain window
- envelope tables rebuilt only on change, filter coefficients only when the cutoff moves
- the editor repaints at 30 Hz and only repaints the header rectangle when the preset
  changes

If you add anything to the per-sample inner loops, measure it.
