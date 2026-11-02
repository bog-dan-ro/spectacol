# Building Spectacol

Spectacol builds with CMake and Qt 6. It is developed on Linux; the desktop build should also work on the other
platforms Qt supports, but only Linux is tested.

## Requirements

- CMake 3.25 or later and Ninja (or another generator).
- A C++23 compiler: GCC 14 or later, or Clang 18 or later.
- Qt 6.11 or later with the Quick, Multimedia and ShaderTools modules (the Qt online installer has them).
- zlib.
- Network access when configuring the first time: CMake's `FetchContent` downloads
  - [qtgamepadlegacy](https://github.com/bog-dan-ro/qtgamepadlegacy) (branch `dev`), the gamepad support;
  - [ZxKit](https://github.com/bog-dan-ro/ZxKit), the emulation library, unless Spectacol lives in ZxKit's
    `tests/spectacol` (then the parent directory is used);
  - [SafeCpp](https://github.com/bog-dan-ro/safecpp), which ZxKit needs.

## Desktop

```sh
git clone https://github.com/bog-dan-ro/spectacol.git
cd spectacol
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$HOME/Qt/6.12.0/gcc_64
cmake --build build
./build/bin/spectacol
```

Qt Creator opens `CMakeLists.txt` directly: pick a Qt 6 kit and build.

The command line:

```sh
./build/bin/spectacol [--machine ID] [file]   # e.g. --machine 128k game.tzx
./build/bin/spectacol --list-machines
```

Headless runs (for checks and screenshots) use Qt's offscreen platform. Use throwaway settings and data
directories, so your own settings stay untouched:

```sh
XDG_CONFIG_HOME=/tmp/spx/config XDG_DATA_HOME=/tmp/spx/data QT_QPA_PLATFORM=offscreen \
    ./build/bin/spectacol --machine 48k --quit-after 10 --screenshot screen.png game.tzx
```

`--screenshot` saves the emulated screen, `--ui-screenshot ui.png` the whole window (`--window-size 1280x800`
sets its size). `--ui-keys F5,Down,Return` presses keys in the window (one every 300 ms from the first second on),
to reach a page before the screenshot; `click:X:Y` in the list clicks at a window position (the desktop menu bar). The first run with new settings shows the welcome page, which pauses the emulation. To skip it,
and to keep the data (snapshots, downloads) out of your own folder, start with this
`$XDG_CONFIG_HOME/Licentia/Spectacol.conf` (`[General]` holds the top level keys, `[%General]` the "General" group of
the settings page):

```ini
[General]
first_start=false
dataPath=/tmp/spx/data/Spectacol/
```

## Software screen filters

Under **Options → Borders**, choose the top, left, bottom and right margins independently. Values are standard
Spectrum pixels/lines (scaled for Timex hi-res and the Next). Changes are immediate, including while paused,
and persist between runs. **Wide** keeps the full side borders with zero top/bottom margins; **Full borders**,
**No borders** and **Default borders** are also available. Each machine's available border limits the crop.
Spectacol crops ZxKit's full native frame; cropping never resets the machine or alters its timing. Native PNG
screenshots use the selected crop, before filtering or window scaling. Check the compiled UI with:

```sh
python3 tests/borders.py
```

Choose a filter under **Options → Screen → Screen filter**. Normal is the default.
The selector includes ZxKit's HQ, xBRZ, Scale/AdvMAME, SaI/Eagle, TV, dot matrix,
bilinear, nearest, scanlines and RGB masks, plus chained presets such as
HQ4x + Scanlines and xBRZ4x + RGB mask. HQ4x + TV4x enlarges each dimension
16 times; the same-resolution scanline presets retain the scaler's dimensions.

The choice is restored on startup using the stable preset ID in
`Screen/softwareFilter`, independent of menu ordering. For example, an isolated
headless configuration can append:

```ini
[Screen]
softwareFilter=hq:4+scanlines:1
```

Filtering runs during rendering and updates immediately even while emulation
is paused. Native screen dimensions still determine aspect ratio, integer
scaling and mouse input. Normal bypasses the CPU filters. Unsupported texture
sizes or processing failures restore Normal and show the error.

Use `--ui-screenshot` to capture the displayed filter. Saved emulator screenshots,
screen files and snapshot previews retain native pixels. Large software filter
chains can reduce rendering performance; the emulator frame callback continues
to publish native frames. Spectacol links the separate `ZX::VideoFilters` target;
other ZxKit consumers can continue to link only `ZX::Kit`.

## Offline catalogue and input tests

The Qt Test suite exercises both Internet Archive and ZXInfo through a local HTTP server. It checks paging,
URL encoding, metadata and download parsing, cancellation, unavailable results, malformed replies, checksum
failures and atomic save/retry. It makes no requests to the public services.
The controller tests cover dead zones, both sticks, D-pad/touch overlap, shared fire/key ownership,
mapping changes, cancelled presses and disconnect cleanup. Keyboard tests load the same QML input state
used by the UI and cover overlapping fingers/navigation, sticky chords, cancellation and late releases.

```sh
cmake -S . -B build -DSPECTACOL_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

ZXInfo uses the public [v5 API](https://github.com/zxinfo-api/zxinfo-api-v5/blob/main/docs/swagger.yml) at
`https://api.zxinfo.dk/v5/`. Its `offset` is a zero-based page number. Catalogue file paths follow
[ZXInfo's own link mapping](https://github.com/thomasheckmann/zxinfo-vue/blob/master/src/components/ZXInfo/linksInfo.vue)
to the World of Spectrum Archive mirror or Spectrum Computing; screenshots use ZXInfo's media host.
No API key is required. Use the headless UI check above to review layout and the live-service integration.

The controller integration check runs a generated Z80 diagnostic that samples the real keyboard rows and
Kempston port into RAM, then checks the values from saved snapshots. It uses temporary settings and no hardware:

```sh
python3 tests/controller.py
```

Outputs go to `build/controller`; `--binary` and `--output` override the defaults. Scripted `--ui-keys` also accepts
`pad:AxisLeftX:-1`, `pad:ButtonA:down` / `up` and corresponding `touch:` events (`cancel` tests cancelled touches).
`pad:clear` releases all input;
`touch:clear` releases only the touch joystick.

The on-screen keyboard check sends window touch events and navigation presses through the compiled UI,
then checks the real keyboard rows with the same diagnostic:

```sh
python3 tests/keyboard.py
```

It covers overlapping and shared keys, sliding off a key, cancelled touches, hiding/pause and sticky modifier
chords. Outputs go to `build/keyboard`; `--binary` and `--output` override the defaults. Scripted `--ui-keys`
accepts `point:ID:down:X:Y`, `point:ID:move:X:Y`, `point:ID:up:X:Y` (window coordinates) and `point:cancel`.
Tests use temporary settings and require no touch hardware.

## Saving regression

The saving check uses the real BASIC ROM to load a generated program, saves it to a blank tape, exports TAP
through the compiled UI and compares the program bytes after reload. It also checks snapshot Save As, portable
quick-save names, failed-write recovery and retrying an RZX recording without recording it again:

```sh
python3 tests/saving.py
```

Logs, screenshots and round-trip media go to `build/saving`. The test uses temporary settings and data;
`--binary` and `--output` override the defaults. Android storage access is a separate postponed task.

## Android

The Android build has not been ported to CMake and Qt 6 yet (see the Android section of `TODO.md`). It will need
the Qt for Android kit, the Android SDK and NDK, and the OpenSSL libraries of the `android_openssl` submodule:

```sh
git submodule update --init --recursive
```

The CoBra keyboard regression drives the compiled on-screen keyboard, selects sticky
CTRL+0 and checks the ROM CP/M CPU state (no external disk required):

```sh
python3 tests/cobra_keyboard.py
```

Its screenshots, snapshot and log go to `build/cobra-keyboard`; it uses temporary
settings and app data. `--binary` and `--output` override the defaults.
