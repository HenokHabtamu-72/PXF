# The 20 presets

Exactly twenty, hard cap, compiled into the binary as `constexpr` parameter snapshots in
[`Source/Presets.cpp`](../Source/Presets.cpp). There is no user preset saving: a preset is
just a set of parameter values, and the DAW already saves those with the session.

Browse with the ◀ ▶ arrows in the header, or click the preset name for the grouped menu.

Column key — **Osc**: the two oscillator waves. **Cut**: filter cutoff in Hz. **ADSR**:
attack / decay / sustain / release, seconds except sustain. **Voice**: `mono+leg` means the
preset ships with MONO and LEGATO on, so overlapping notes slide. **Dust**: how much lo-fi
character is dialled in by default.

---

## BASS

Sub-heavy and mono, so basslines slide between notes the way they are played.

| Preset | Osc | Sub | Cut | ADSR | Glide | Voice | Dust | Character |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| **Midnight Sub** | sine + triangle | 0.85 | 320 | .006 / .40 / .60 / .30 | 45 ms | mono+leg | 0.28 | Round, almost pure sub. The default patch on a fresh instance. |
| **Purple Bass** | saw + square | 0.60 | 520 | .005 / .50 / .50 / .28 | 30 ms | mono+leg | 0.38 | Reedier, with bite from the square and a faster slide. |
| **Deep End** | sine + sine | 1.00 | 240 | .012 / .80 / .78 / .45 | 70 ms | mono+leg | 0.22 | Full sub, slow slide, long sustain. Sits under everything. |

## PLUCK

Short decay, no sustain, plenty of delay and reverb to leave a trail.

| Preset | Osc | Cut | ADSR | Voice | Dust | Character |
| --- | --- | --- | --- | --- | --- | --- |
| **Rain Pluck** | triangle + saw | 1 700 | .002 / .28 / 0 / .25 | poly | 0.35 | Soft and wet — the default late-night pluck. |
| **Neon Drip** | square + triangle | 2 400 | .002 / .22 / .05 / .30 | poly | 0.30 | Brighter and tighter, on a 1/16 ping-pong. |
| **Glass Pluck** | sine + triangle | 3 800 | .001 / .35 / 0 / .40 | poly | 0.25 | Clean and glassy, the least processed of the three. |

## SYNTH

The general-purpose middle ground: detuned saws with room to sit in a mix.

| Preset | Osc | Detune | Cut | ADSR | Voice | Dust | Character |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **Dusk Synth** | saw + saw | 14 ct | 1 200 | .05 / .60 / .55 / .50 | poly | 0.40 | Warm detuned workhorse. Chords and single lines both work. |
| **Haze** | triangle + saw | 18 ct | 900 | .25 / 1.0 / .60 / .80 | poly | 0.50 | Softer attack, darker filter, heavier Dust. Half-way to a pad. |

## PAD

Long attacks, long releases, big reverb. Play three notes and leave them alone.

| Preset | Osc | Detune | Cut | ADSR | Reverb | Dust | Character |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **2AM Pad** | saw + triangle | 18 ct | 1 000 | .90 / 1.5 / .70 / 1.6 | 0.80 / 0.45 | 0.40 | The signature patch. Slow swell, wide detune. |
| **Violet Sky** | saw + saw | 24 ct | 1 400 | 1.4 / 2.0 / .80 / 2.2 | 0.85 / 0.50 | 0.35 | Brighter and wider, on a half-note delay. |
| **Slow Fade** | triangle + sine | 10 ct | 700 | 2.0 / 2.5 / .65 / 3.0 | 0.90 / 0.55 | 0.45 | Darkest and slowest. Nearly a drone. |

## BELL

No sustain and a long decay — a short harmonic stack, not a sampled bell.

| Preset | Osc | Detune | Cut | ADSR | Dust | Character |
| --- | --- | --- | --- | --- | --- | --- |
| **Night Bell** | sine + sine | 22 ct | 5 000 | .001 / 1.2 / 0 / 1.4 | 0.30 | Clear strike, long ring. |
| **Chime Glow** | sine + triangle | 16 ct | 6 500 | .002 / 1.6 / 0 / 1.8 | 0.25 | Brighter and longer, on a 1/16 delay for sparkle. |

## FLUTE

Filtered triangles, mono and legato, so phrases slide like a wind instrument.

| Preset | Osc | Cut | ADSR | Glide | Voice | Dust | Character |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **Air Flute** | triangle + sine | 2 200 | .12 / .30 / .85 / .30 | 35 ms | mono+leg | 0.30 | Breathy and soft-edged. |
| **Lofi Flute** | triangle + triangle | 1 700 | .09 / .25 / .80 / .35 | 50 ms | mono+leg | 0.60 | Darker with the Dust knob well up — tape flute. |

## PIANO

Fast attack, long decay, very low sustain. Character over accuracy.

| Preset | Osc | Cut | ADSR | Dust | Character |
| --- | --- | --- | --- | --- | --- |
| **Dusty Keys** | triangle + saw | 2 600 | .002 / .90 / .18 / .35 | 0.55 | Bright attack, dusty tail. The lofi-hop keys sound. |
| **Felt Piano** | sine + triangle | 1 900 | .004 / 1.1 / .12 / .45 | 0.45 | Softer and rounder, as if the hammers were felted. |

## LEAD

Mono and legato with longer glide times, for single lines that slide between notes.

| Preset | Osc | Cut | ADSR | Glide | Voice | Dust | Character |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **Neon Lead** | saw + square | 2 100 | .010 / .30 / .75 / .30 | 65 ms | mono+leg | 0.30 | Cutting but warm, on a dotted-eighth delay. |
| **Skyline** | saw + saw | 2 600 | .020 / .40 / .80 / .50 | 55 ms | mono+leg | 0.35 | Wider and brighter, quarter-note delay. |
| **Cruise Control** | square + triangle | 1 800 | .030 / .50 / .70 / .60 | 120 ms | mono+leg | 0.40 | Long lazy glide — the slide is the point. |

---

## Editing a preset

Change the row in `Source/Presets.cpp` and rebuild. The comment above the table lists the
column order and the wave and delay-division encodings:

```
waves:  0 sine   1 triangle   2 saw   3 square
delay:  0 = 1/16   1 = 1/8T   2 = 1/8   3 = 1/8D   4 = 1/4   5 = 1/4D   6 = 1/2
```

Keep the count at twenty. `kNumPresets` and the browser arithmetic both assume it, and the
cap is a deliberate design constraint rather than an implementation limit.
