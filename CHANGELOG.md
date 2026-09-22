# Changelog

All notable changes to PXF are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project uses
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] — 2026-09-22

First release. A VST3 / AU / Standalone instrument built on JUCE 8 and C++17.

### Added

- **SYNTH engine** — 8-voice polyphonic, two band-limited oscillators
  (sine / triangle / saw / square) plus a sine sub one octave down, osc mix and detune
  spread, a TPT state-variable low-pass with cutoff and resonance, and one ADSR.
- **Mono and legato voice modes** with a constant-time Glide (0 – 2000 ms): a held-note
  stack with last-note priority, pitch fall-back to the note still held on release, and a
  LEGATO switch that shapes the envelope rather than the pitch.
- **GRAIN engine** — a polyphonic granulizer. Loads WAV, AIFF, FLAC and MP3 one-shots in
  mono or stereo at any sample rate, resampling internally. Size, Density, Position, Spray,
  Pitch and Shape controls, with keyboard tracking where C4 plays the sample at its original
  pitch. Each held note runs its own grain stream.
- **Master FX chain** shared by both engines: a BPM-synced ping-pong delay (1/16 … 1/2),
  reverb, the DUST lo-fi character knob (tape wow and flutter, saturation, high-cut, vinyl
  hiss and crackle), and master volume.
- **20 built-in presets** across bass, pluck, synth, pad, bell, flute, piano and lead, as
  `constexpr` parameter snapshots compiled into the binary, with a header browser.
- **"Late Night Chill" UI** — fixed 920 × 580, neon purple on near-black navy with a
  film-grain overlay, glowing value arcs, and a purple-to-blue waveform display with live
  grain read-heads.
- **Session state** — all parameters, the active tab, the selected preset and the path of
  the loaded grain sample are saved and restored by the host.
- **Reported latency** so the host compensates for the 4 ms Dust read-offset.
- **Headless engine tests** (`Tests/EngineTests.cpp`) that render real audio and measure it:
  glide pitch tracking, legato envelope behaviour, grain keyboard tracking, silence after
  release, and finiteness of every output sample.
- **CMake presets** and a GitHub Actions workflow that builds and tests on Windows, macOS
  and Linux.

[1.0.0]: https://github.com/HenokHabtamu-72/PXF/releases/tag/v1.0.0
