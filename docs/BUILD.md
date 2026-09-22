# Building PXF

PXF is a JUCE 8 / C++17 project driven by CMake. There are no dependencies to install by
hand — the build downloads JUCE for you on the first configure.

Two ways to drive it: the **presets** in `CMakePresets.json` (shortest), or plain CMake
commands (works with any generator you like). Both are shown throughout.

---

## Windows (Visual Studio 2022) → FL Studio

### 1. Install the tools (one time)

1. **Visual Studio 2022** — Community edition is fine:
   <https://visualstudio.microsoft.com/>
   In the installer, tick the **"Desktop development with C++"** workload. That includes the
   MSVC compiler and CMake; nothing else is needed.
2. An internet connection for the first configure. It fetches JUCE 8.0.4 — about 100 MB,
   once.

> Visual Studio **Code** is not enough on its own: it is an editor, not a compiler. You can
> absolutely edit the project in VS Code, but the MSVC toolchain from Visual Studio 2022 (or
> the standalone *Build Tools for Visual Studio*) has to be installed for the build to run.

### 2. Build

Open **"x64 Native Tools Command Prompt for VS 2022"** from the Start menu, `cd` into the
folder containing `CMakeLists.txt`, then:

```bat
cmake --preset windows
cmake --build --preset windows
```

Or the equivalent long form:

```bat
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target PXF_VST3
```

The first command takes a few minutes while it downloads JUCE. The second compiles the
plugin. When it finishes, the plugin is at:

```
build\PXF_artefacts\Release\VST3\PXF.vst3
```

> **Already have a JUCE checkout?** Skip the download:
> `cmake -B build -G "Visual Studio 17 2022" -A x64 -DPXF_JUCE_PATH=C:\path\to\JUCE`

> **Prefer clicking?** After the configure step you can open `build\PXF.sln` in Visual
> Studio, set the configuration to **Release**, and build the **PXF_VST3** project.

> **Want the standalone app** to try it without a DAW? Build the `PXF_Standalone` target;
> it lands in `build\PXF_artefacts\Release\Standalone\PXF.exe`.

### 3. Install for FL Studio

Copy the **whole `PXF.vst3` folder** (it is a folder, not a single file) to the standard
VST3 location:

```
C:\Program Files\Common Files\VST3\
```

Windows will ask for administrator permission — accept it.

### 4. Make FL Studio find it (FL Studio 21+)

1. Open FL Studio.
2. **Options ▸ Manage plugins**.
3. Press **"Find plugins"** (the round scan button, top left). Make sure **Verify plugins**
   is ticked so instruments get sorted correctly.
4. When the scan finishes, **PXF** appears in the plugin list — filter:
   *Installed ▸ Generators ▸ VST3*.
5. Add it to a channel: **+ button in the Channel Rack ▸ More plugins ▸ PXF**, or drag it in
   from the Plugin database.

Play some notes — you should hear the **Midnight Sub** preset. Use the ◀ ▶ arrows in the
plugin header (or click the preset name) to browse all 20 sounds, and the **SYNTH / GRAIN**
switch on the right to move between the two engines.

---

## macOS

```bash
cmake --preset macos
cmake --build --preset macos
```

Or:

```bash
cmake -B build -G Xcode
cmake --build build --config Release --target PXF_VST3   # and/or PXF_AU
```

The build produces a universal binary (x86_64 + arm64) by default.

- **VST3** — copy `build/PXF_artefacts/Release/VST3/PXF.vst3` to
  `~/Library/Audio/Plug-Ins/VST3/`
- **AU** — copy `build/PXF_artefacts/Release/AU/PXF.component` to
  `~/Library/Audio/Plug-Ins/Components/`

Logic Pro rescans AUs on launch; run `killall -9 AudioComponentRegistrar` first if it does
not pick the plugin up.

---

## Linux

```bash
sudo apt install build-essential cmake libasound2-dev libx11-dev libxext-dev \
     libxrandr-dev libxcursor-dev libxinerama-dev libxcomposite-dev \
     libfreetype6-dev libfontconfig1-dev libgl1-mesa-dev

cmake --preset linux
cmake --build --preset linux
```

Copy `build/PXF_artefacts/Release/VST3/PXF.vst3` to `~/.vst3/`.

---

## The engine self-tests

A small headless binary renders real audio through both engines and checks it — including
that the mono/legato glide actually slides and lands on pitch. To build and run it:

```bash
cmake --preset linux-tests          # windows-tests / macos-tests on those platforms
cmake --build --preset linux-tests
ctest --preset linux
```

Or without presets:

```bash
cmake -B build -DPXF_BUILD_TESTS=ON
cmake --build build --target pxf_tests --config Release
./build/pxf_tests_artefacts/Release/pxf_tests
```

On Windows the binary is at `build\pxf_tests_artefacts\Release\pxf_tests.exe`. It prints one
line per check and exits non-zero if anything fails.

---

## Build options

| Option | Default | What it does |
| --- | --- | --- |
| `PXF_FETCH_JUCE` | `ON` | Download JUCE during configure. |
| `PXF_JUCE_PATH` | *(empty)* | Use an existing JUCE checkout instead of downloading. Takes priority over `PXF_FETCH_JUCE`. |
| `PXF_JUCE_TAG` | `8.0.4` | Which JUCE tag to fetch. |
| `PXF_BUILD_TESTS` | `OFF` | Also build the `pxf_tests` console target and register it with CTest. |

---

## Troubleshooting

**FL Studio doesn't list PXF.**
Confirm the *folder* `PXF.vst3` — not just a file from inside it — sits directly in
`C:\Program Files\Common Files\VST3`, then rescan with **Verify plugins** ticked. If it
still does not appear, check that you built the **x64** configuration; FL Studio 21 is
64-bit only.

**CMake can't find a generator.**
You are in a normal terminal. Use the *x64 Native Tools Command Prompt for VS 2022*, which
puts the compiler and CMake on `PATH`.

**`cmake` is not recognised.**
Either Visual Studio's C++ workload is not installed, or you are not in the Native Tools
prompt. Installing VS Code alone does not provide a compiler.

**The first configure is slow, or fails on the network.**
It is downloading JUCE. If your network blocks GitHub, download JUCE 8 manually and pass
`-DPXF_JUCE_PATH=<path>`.

**The build worked but the DAW crashes on scan.**
Delete `build/` and reconfigure from scratch. A half-finished JUCE fetch can leave the
generated VST3 manifest stale.

**Licensing note.**
JUCE is dual-licensed (AGPLv3 / commercial). This build shows the JUCE splash screen by
default. Only set `JUCE_DISPLAY_SPLASH_SCREEN=0` in `CMakeLists.txt` if your JUCE licence
permits it.
