# Contributing to PXF

Bug reports, fixes and sound-design tweaks are all welcome. PXF is deliberately small, so
the most useful contributions are usually the ones that make what is already here better
rather than the ones that add something new.

## Before you open a pull request

1. **Read [docs/SPEC.md](docs/SPEC.md).** PXF is built to a brief. The 20-preset hard cap,
   the absence of user preset saving and the fixed 920 × 580 window are design decisions, not
   omissions.
2. **Read [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)** if you are touching `Source/`,
   especially the threading section. The audio thread never allocates, never blocks and never
   touches the file system.
3. **Build with the tests on and make sure they pass:**

   ```bash
   cmake --preset linux-tests      # windows-tests / macos-tests
   cmake --build --preset linux-tests
   ctest --preset linux
   ```

   CI runs the same thing on all three platforms for every pull request.

## House style

- C++17, JUCE conventions, four-space indent, no tabs.
- Allman braces, a space before the argument list (`foo (x)`) — match the surrounding code.
- `namespace pxf` for everything; `pxf::theme` for palette and drawing helpers.
- Comments explain *why*, not *what*. The existing files are the reference.
- Keep the build warning-free. `juce::juce_recommended_warning_flags` is on for a reason.

## Changing DSP

Anything in a per-sample inner loop is a performance decision. If you add work there, say in
the pull request what it costs. The budget is ~8 % of one modern core for a held pad chord
or an active grain stream.

If you change gain staging anywhere in `FxChain` or `SynthVoice::render`, re-check every one
of the 20 presets — their volume values are tuned against the current chain.

## Adding a parameter

In order: an ID in `pid` (`Parameters.h`), a range in `createParameterLayout()`, a field in
the relevant `*Params` struct, a line in `PxfAudioProcessor::pullParameters()`, and a knob in
`PluginEditor`. Missing the `pullParameters()` step is the classic way to add a control that
does nothing.

## Reporting a bug

Include your OS, DAW and version, how you installed the plugin, and what you expected versus
what happened. For audio problems, the sample rate and buffer size matter — please include
them.
