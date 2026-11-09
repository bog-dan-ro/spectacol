# Building Spectacol

Spectacol builds with CMake and Qt 6. Linux and an Android emulator are tested; the desktop build
should also work on the other platforms Qt supports.

## Requirements

- CMake 3.25 or later and Ninja (or another generator).
- A C++23 compiler: GCC 14 or later, or Clang 18 or later.
- Qt 6.11 or later with the Quick, Multimedia and ShaderTools modules (the Qt online installer has them).
- zlib.
- Network access when configuring the first time: CMake's `FetchContent` downloads
  - [qtgamepadlegacy](https://github.com/bog-dan-ro/qtgamepadlegacy) (a pinned revision), the gamepad support;
  - [ZxKit](https://github.com/bog-dan-ro/ZxKit) (a pinned revision), the emulation library, unless Spectacol lives in ZxKit's
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

## Linux packages

Both package formats install the desktop launcher, the existing Spectacol icon,
AppStream metadata, ROM notices and third-party licenses. They include the
QtGamepadLegacy libraries, QML module and controller backends. Generated files
stay under `build/`.

### Flatpak

Install Flatpak 1.15.6 or later and `flatpak-builder` from your distribution, then add Flathub
and its KDE 6.11 SDK/runtime:

```sh
flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
flatpak install --user flathub org.kde.Sdk//6.11 org.kde.Platform//6.11
flatpak-builder --user --force-clean --jobs=4 \
    --state-dir=build/flatpak-state --repo=build/flatpak-repo \
    build/flatpak packaging/flatpak/eu.licentia.games.spectacol.yaml
mkdir -p build/packages
flatpak build-bundle --runtime-repo=https://dl.flathub.org/repo/flathub.flatpakrepo \
    build/flatpak-repo build/packages/Spectacol.flatpak eu.licentia.games.spectacol
flatpak install --user build/packages/Spectacol.flatpak
flatpak run eu.licentia.games.spectacol
```

The manifest builds the current Spectacol working tree. Dependencies are pinned
and downloaded by Flatpak Builder before the network-isolated build;
`FetchContent` uses those local sources. Update the dependency revisions in both
`CMakeLists.txt` and the manifest together. The pinned QtGamepadLegacy revision
supports Qt 6.11 and retains the Android build fixes.

The sandbox grants graphics, audio, network and controller access, including
read-only access to `/run/udev` for QtGamepadLegacy's controller discovery.
This lets it identify controllers such as Steam Input's virtual gamepad on Steam Deck.
Home and removable-media access support Spectacol's built-in file browser, writable disks
and SD images. Settings use Flatpak's private configuration directory; game data
defaults to `~/Spectacol` or the folder selected in the app. Host
serial ports for physical UART adapters need a separate user override; the
default package supports the JavaScript UART hooks and network adapters.
This is a local bundle recipe; publishing to Flathub also requires a release
source and Flathub submission.

### Desktop text input

When an editable field gains focus, desktop builds open Spectacol's ZX on-screen
keyboard if a gamepad is attached or no physical keyboard is attached. Connecting
a gamepad enables it even when a physical keyboard is present. The D-pad selects
a key, A types it, and B hides the keyboard before closing the page or dialog. A reopens
it while the field still has focus. Touch input works too. Caps Shift enters
uppercase letters; Caps Shift + 0 deletes, and Symbol Shift enters punctuation.
A copy of the edited field above the keys keeps text visible in dialogs.
Android and iOS continue using their system keyboards.

On Linux, keyboard detection reads `/sys/class/input`, including USB and Bluetooth
keyboards. Steam Input's virtual keyboard and the Steam Deck controller's keyboard
interface do not count as physical keyboards. Attachment and removal are checked
once per second, including inside Flatpak. Text is committed through Qt's input
method events to the original field, preserving its selection and validation.

### AppImage

Use a complete Qt 6.11+ desktop SDK with Quick, Multimedia, ShaderTools, SVG and
Wayland support, a C++23 compiler, Ninja, and
[linuxdeploy](https://github.com/linuxdeploy/linuxdeploy/releases) with its
[Qt plugin](https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases).
Download tools for the build machine's architecture, put both executables in the
same directory and make them executable. The build does not download them.

The `appImage` target packages a configured build directory, like `apk` on Android.
CMake looks for `linuxdeploy` (or `linuxdeploy-ARCH.AppImage`) in `PATH`, and for
`linuxdeploy-plugin-qt` (or `linuxdeploy-plugin-qt-ARCH.AppImage`) beside it or in `PATH`.
Otherwise pass `SPECTACOL_LINUXDEPLOY` and `SPECTACOL_LINUXDEPLOY_PLUGIN_QT`:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH=/path/to/Qt/6.11.1/gcc_64 \
    -DSPECTACOL_LINUXDEPLOY=/path/to/linuxdeploy-x86_64.AppImage \
    -DSPECTACOL_LINUXDEPLOY_PLUGIN_QT=/path/to/linuxdeploy-plugin-qt-x86_64.AppImage
cmake --build build --target appImage    # or: ninja -C build appImage
./build/packages/Spectacol-1.6.0.5-x86_64.AppImage
```

The target builds Spectacol and installs only the `Runtime` component into a fresh
`build/AppDir`. linuxdeploy bundles Qt, QML imports, SVG/Wayland support and
shared-library dependencies, then writes `build/packages/Spectacol-VERSION-ARCH.AppImage`.
The AppDir remains in the build directory for inspection.
Build release AppImages on the oldest distribution you support: the build host's
glibc and C++ runtime set the minimum host requirements. Build each architecture on a matching
machine/toolchain.

To run without FUSE, set `APPIMAGE_EXTRACT_AND_RUN=1`. A headless package check
can use isolated settings and data, as for the desktop build. Create
`build/package-check/config/Licentia/Spectacol.conf` first:

```ini
[General]
first_start=false
dataPath=/absolute/path/to/spectacol/build/package-check/data/
```

Then run:

```sh
APPIMAGE_EXTRACT_AND_RUN=1 QT_QPA_PLATFORM=offscreen \
    XDG_CONFIG_HOME="$PWD/build/package-check/config" \
    XDG_DATA_HOME="$PWD/build/package-check/data" \
    ./build/packages/Spectacol-1.6.0.5-x86_64.AppImage \
    --machine 48k --quit-after 3 --ui-screenshot "$PWD/build/package-check/ui.png"
```

Also launch the package with `QT_QPA_PLATFORM=wayland` in a Wayland session and
`QT_QPA_PLATFORM=xcb` with an X server. These exercise the real graphics paths;
the offscreen check does not load Wayland's EGL integration. The target stages
the Wayland graphics, shell and decoration plugins before dependency deployment.

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

## Audio continuity regression

The Engine feeds a bounded PCM queue directly. Qt 6.12's `QAudioSink` callback consumes it on the audio
thread; GUI event processing and frame presentation do not feed the device. The callback allocates no
memory and takes no locks. Device creation/destruction stays on the owning thread; a short producer-side
mutex only publishes/takes a queue reference and is never held during backend calls or sample processing.

Playback primes about 60 ms of audio, corrects device-clock drift through adaptive resampling, and fades
on deliberate pause/mute and resume. Fixed-speed playback changes pitch with speed. Unbounded turbo
estimates the incoming rate; it no longer mutes sound. ZxKit preserves fractional sample timing and writes
across video boundaries, avoiding gradual buffer starvation. The standalone Engine keeps wall-clock pacing.

The `audio_output` group in the full CTest suite needs no sound hardware. It checks rate conversion,
50/100/200% speeds, ten virtual minutes of device-clock mismatch, rate-boundary phase/latency,
turbo jitter and resume timing, stale audio after pause/mute, bounded
storage and actual producer/consumer threads. Root ZxKit tests also verify audio timing and callback order.

An optional live test uses an explicitly selected disposable device and repeatedly stalls its GUI/event
thread for 500 ms while its producer keeps feeding Qt's real audio backend. It cycles through 100%, 200%,
50% and measured turbo speed, then reports underruns, overflows and rendered frames:

```sh
./build/bin/test_audio_output --live-device "DEVICE NAME OR ID"
```

On Linux a private null sink can be created with `pactl load-module module-null-sink
sink_name=spectacol_audio_test sink_properties=device.description=SpectacolAudioTest`; pass
`SpectacolAudioTest` to the test. Record its monitor with `parec -d spectacol_audio_test.monitor` to check
for gaps, and unload only that module afterwards using the returned module ID. This leaves the system's
default device unchanged. Physical-device listening, Bluetooth/device switching and Android latency still
need checks; the test establishes output continuity under the exercised scheduling and speed changes.

## Saving regression

The saving check uses the real BASIC ROM to load a generated program, saves it to a blank tape, exports TAP
through the compiled UI and compares the program bytes after reload. It also checks snapshot Save As, portable
quick-save names, failed-write recovery and retrying an RZX recording without recording it again:

```sh
python3 tests/saving.py
```

Logs, screenshots and round-trip media go to `build/saving`. The test uses temporary settings and data;
`--binary` and `--output` override the defaults. Android document-provider checks are described below.

## Android

Use Qt 6.12's Android and desktop host kits, Android SDK/platform/build tools, NDK r27c
(`27.2.12479018`) and JDK 21. Both `android_x86_64` and `android_arm64_v8a` APKs build with
this toolchain. Qt supplies the Gradle wrapper and matching Android Gradle plugin; do not copy
the old Qt 5 Gradle files into the package directory. No Qt SDK modifications are required.

```sh
git submodule update --init --recursive
export ANDROID_SDK_ROOT="$HOME/android"
export ANDROID_NDK_ROOT="$ANDROID_SDK_ROOT/ndk/27.2.12479018"
qt_root="$HOME/Qt/6.12.0"
cmake -S . -B build-android-x86_64 -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$qt_root/android_x86_64/lib/cmake/Qt6/qt.toolchain.cmake" \
    -DQT_HOST_PATH="$qt_root/gcc_64" \
    -DANDROID_SDK_ROOT="$ANDROID_SDK_ROOT" -DANDROID_NDK_ROOT="$ANDROID_NDK_ROOT" \
    -DCMAKE_BUILD_TYPE=Release -DQT_ANDROID_DEPLOYMENT_TYPE=Debug
cmake --build build-android-x86_64 --target apk --parallel
```

For ARM64, use `android_arm64_v8a` and a separate build directory. The command builds optimized
native code in a debug-signed APK, suitable for installing and `adb run-as` testing. The APK is under
`spectacol/android-build/build/outputs/apk/debug/`. A distributable release needs your release signing
configuration and a monotonically increasing `-DSPECTACOL_ANDROID_VERSION_CODE=...`; the default
1641605 preserves the highest version code of the old per-ABI scheme. Debug signatures cannot upgrade
a release signed with another key.

The build fetches the Qt 6.12 GamepadLegacy fork, including its Java JAR, and packages the OpenSSL 3
libraries from `android_openssl`. ZxKit probes NDK support for `std::jthread` and propagates
`-fexperimental-library` only when needed. Native libraries use 16 KiB ELF alignment; check the packaged
APK with the SDK's `zipalign -c -P 16 4` and `apksigner verify` as well.

The Qt 6 manifest uses app storage and no obsolete shared-storage permissions. **Quick Open → ⋮ → Device
files…** imports a file through Android's picker; **Save snapshot → Export…** and **Export tape → Export
to device…** write to a selected document. Files opened from another app are copied into app storage
before loading, including when Spectacol is already running. App data stays private. Old shared data
is left in place; its saved location is retained as `Android/legacyDataPath` and files can be imported
through the picker. Automatic migration and browsing a granted folder in the cover flow remain TODOs.

Run device regressions on a disposable emulator with an English system UI:

```sh
python3 tests/android.py --adb "$ANDROID_SDK_ROOT/platform-tools/adb" --serial emulator-5554
```

The script requires an installed debug APK. It temporarily replaces the app's settings, restores them
on exit, and checks cold/warm document intents and native snapshot export. Do not interact with the
emulator while it runs. Physical gamepads, Android TV remotes and audio latency still need device checks.
On the tested API 37 x86_64 emulator, `-gpu host` renders correctly; SwiftShader's OpenGL driver showed
incomplete frames. Check that an AVD's system image is actually installed before diagnosing Qt.

Phone sheets and the drawer use a single 48 dp header (72 with Large text), with 48 dp minimum
navigation targets. Touch-only pages put secondary actions under **⋮**, rather than displaying desktop
shortcuts. Connected gamepads retain button hints; a TV without touch or a gamepad uses OK/Back hints.
Narrow hint rows scroll horizontally instead of taking multiple rows from the content. About and its
controller help scroll within the page. Quick Open keeps view/search controls visible, and omits the
cover reflection on mobile. Android starts fullscreen to hide system bars (swipe to reveal them);
its landscape control rail aligns to the window's left edge, independently of viewport and safe-area padding.
The emulated image stays at the top right on Android, using the selected scaling mode.

```sh
python3 tests/android_ui.py --adb "$ANDROID_SDK_ROOT/platform-tools/adb" --serial emulator-5554
```

This UI regression needs Pillow and tesseract. It uses an authored snapshot and a 360 × 800 dp
phone profile (800 × 360 in landscape). Each page starts in a fresh process; scripted navigation
opens it, then real Android touch tests the header, scrolling, actions, native picker and ROM settings.
`--case large-landscape` also checks Large text. `--native-input` adds Android Menu/D-pad/OK/Back;
`--native-only` runs just that input probe. `--display-only` checks right alignment in all three
scaling modes and taps the menu button at the window edge. Settings, display overrides and the emulator's
ignore-orientation policy are restored. Screenshots, accessibility trees and logcat go under
`build/android-ui`. This does not replace physical phone, controller or TV checks.

`--case portrait` and `--case large-portrait` also check that the Qt surface matches the native display.
Android defaults to Any (sensor orientation); existing Landscape/Portrait choices remain saved. The
saved choice is applied after creating the Qt window, allowing its surface to resize at startup.
`--rotation-only` checks a fresh default, both saved choices and live changes through Options › Screen:

```sh
python3 tests/android_ui.py --adb "$ANDROID_SDK_ROOT/platform-tools/adb" --serial emulator-5554 --rotation-only
```

These checks pass on the Qt 6.12/API 37 phone emulator; repeat sensor rotation on physical phones.

## CoBra keyboard regression

The CoBra keyboard regression drives the compiled on-screen keyboard, selects sticky
CTRL+0 and checks the ROM CP/M CPU state (no external disk required):

```sh
python3 tests/cobra_keyboard.py
```

Its screenshots, snapshot and log go to `build/cobra-keyboard`; it uses temporary
settings and app data. `--binary` and `--output` override the defaults.

RZX progress, completion, stop, desynchronization, overlay, seek/pause/resume and recording-branch checks use authored
recordings and the compiled application (requires Pillow and `tesseract` to read the rendered status):

```sh
python3 tests/rzx.py --binary build/bin/spectacol
```

During RZX recording or playback, F8 rewinds five seconds, F9 advances five seconds,
and Shift+F9 seeks to the end. Media → Recording → Playback and recording controls
also offers pause/resume and a seek-step choice. A seek stays paused until a fresh
gameplay input; resuming a recording replaces the future from that cursor. Stop
finalises at the cursor. Held controls, host modifiers and stick noise do not resume.
