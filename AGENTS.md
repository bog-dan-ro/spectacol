# Agent instructions

## Project

Spectacol is a ZX Spectrum emulator: the emulation core is [ZxKit](https://github.com/bog-dan-ro/ZxKit) (C++23), and the UI is Qt 6 / QML, designed for gamepad and touch use. It targets desktop (Linux) and Android. GPL-3.0.

## Build

The project uses CMake and needs Qt ≥ 6.12 (Quick, Multimedia, ShaderTools) and C++23. `BUILD.md` describes the build; the Android CMake build uses Qt’s Android toolchain, NDK r27c and JDK 21.

```sh
git submodule update --init --recursive   # android_openssl (Android only)
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64
cmake --build build
./build/bin/spectacol [--machine ID] [--list-machines] [file.tzx|.tap|.z80|.nex|...]
# headless check: QT_QPA_PLATFORM=offscreen ./build/bin/spectacol --machine next --quit-after 14 --screenshot next.png
# the whole UI: --ui-screenshot ui.png [--window-size 1280x800] (the offscreen platform renders with Qt Quick's
# software backend, ZxScreen has a path for it); --ui-keys F5,Down,Return presses keys to open a page first
# (click:X:Y clicks), e.g. the desktop menu bar
```

Configuring needs network access. The top-level `CMakeLists.txt` uses `FetchContent` for:
- `qtgamepadlegacy` (bog-dan-ro fork, branch `dev`), which provides `Qt::GamepadLegacy`.
- ZxKit: the parent directory when Spectacol lives in ZxKit's `tests/spectacol`, otherwise from GitHub. ZxKit itself fetches SafeCpp (`v1.0`).

The ROMs are Qt resources under `spectacol/roms/`, ZxKit gets them through the `romResolver` set in `ZxEmulator`. The ZX Spectrum Next boots from an SD card image that `NextSdCard` downloads (like jnext does) into the app data dir.

Spectacol's catalogue tests use a local HTTP server and its input tests use simulated controllers and touch: configure with
`-DSPECTACOL_BUILD_TESTS=ON`, build, then `ctest --test-dir build --output-on-failure`.
Check UI changes with the headless run above; `tests/*.py` contains compiled UI regressions.
ZxKit has its own complete unit suite, which must also pass.

## Architecture

### ZxKit
- `zxkit::Emulator` is the whole computer (machine, peripherals, media, debugger). It is shared with the emulation thread as `zxkit::Shared<Emulator>`: the only way in is `writeLock()` / `readLock()`.
- `zxkit::Engine` runs it in real time on its own thread, locking it one scanline at a time (a fair spin lock), so the GUI thread can lock it directly: it waits at most a scanline.
- Frames, audio, pauses (breakpoints) and errors come through `Weak` callbacks owned by `CallbackHandle`s in `ZxEmulator`, called on the Engine thread without any lock held.

### Threading model (important)
- GUI thread code talks to the emulator synchronously: `ZxEmulator::instance().emulator().writeLock()->...`, or `withEmulator(fn)` which also shows a ZxKit error to the user. Keep the locked parts short: read files before taking the lock (`loadMediaFile()`), write them after releasing it (`saveMediaFile()`). The lock is not recursive: don't call something that locks again while holding it.
- GUI notifications hop from Engine callbacks with `QMetaObject::invokeMethod(this, ..., Qt::QueuedConnection)`.
  Audio is an exception: `AudioOutput::write` feeds `AudioBuffer` directly on the Engine thread; Qt's audio-thread
  callback consumes it without allocation, locks or GUI event processing. Only device lifecycle belongs to the
  owner thread. Pause/resume and mute use shared atomic generation controls; never read GUI settings in the producer.
- Pausing nests: every `ZxEmulator::setPaused(true)` must be balanced by a `setPaused(false)`. The page `Loader` in `Main.qml` pauses while any page is open (a page can opt out with `readonly property bool pausesEmulation: false`, like the debugger, which pauses through `activateDebugger()` / `deactivateDebugger()`).
- Rendering: the video callback hands each frame to `ZxImage`, which copies the rows that changed into a mutex-guarded image and emits `needsUpdate`. `ZxScreen` (a plain `QQuickItem`, the QML element) draws it through a `QSGSimpleTextureNode` placed at the fill-mode rect. `ZxTexture` (a `QSGTexture`) uploads only the dirty rect in `commitTextureOperations()`.
- The Engine is stopped on `QCoreApplication::aboutToQuit` (`ZxEmulator::stopEngine()`).

### QML side
- `qt_add_qml_module(spectacol URI Spectacol)`. `main.cpp` calls `engine.loadFromModule("Spectacol", "Main")`. C++ types register through `QML_ELEMENT`/`QML_SINGLETON` macros.
- `ZxEmulator` is the central QML singleton. It exposes emulator state (paused, registers as hex strings, filters, joysticks) and owns the sub-objects/models: `ZxTape`, `ZxRecording`, `ZxSettings`, `Disks`, `BreakpointsModel`, `DisassambleModel`, `PokeFinderModel`, `ZXGamesModel` (online game search via `zxrequests`).
- `ZxSettings` persists the settings (`QSettings`) and applies them to the emulator at startup (`applyToEmulator()`), including the last machine picked in the UI.
- `SpectrumScreenImageProvider` (`image://spectrum/...`) renders `.scr` and snapshot screenshots for the file browser and cover flow.
- `z80assembler.cpp` plus `z80_opcodes.csv` implement the inline assembler that the debugger uses.

### Adding files
New C++/QML sources, shaders, images and ROMs must be listed explicitly in `spectacol/CMakeLists.txt` under `qt_add_qml_module`, `qt_add_shaders` or `qt_add_resources`. There is no globbing. Shaders are Vulkan-style GLSL, precompiled by `qt_add_shaders` for GLSL "100es,120,150,310es,450".

`spectacol/shaders/slang_parser.py` is a standalone helper for converting libretro slang shaders. It is not part of the build.

### UI
- `Theme` (QML singleton) holds the design tokens: colour roles, the type scale (`caption`, `body`, `title`,
  `pageTitle`), spacing (`sp(n)` = n × 4 px, scaled by "Large text"), motion (respects "Reduce motion"), and
  `desktopMode` (menu bar and docked pages, else the overlay for gamepad, touch and TV). No hard-coded colours or sizes.
- A page is a `Sheet` (crumb, title, `hints` for the `HintBar`, content); settings pages use `SettingsList` and
  `SettingRow`; pick-one lists use `OptionList`. `TextSizes` only maps the old names onto `Theme`.
- Leave the screen filter UI (`OptionsScreenPage.qml`, `FiltersPage.qml`, `filtersModel`, `selectedFilterIndex`) alone.

#### Input and navigation

- All UI pages, windows, dialogs and interactive controls must be navigable with a D-pad, gamepad or
  joystick, with visible focus and predictable movement between controls.
- Every page, window and dialog must support the gamepad Back action. It closes the current dialog/window
  or returns one level, restoring focus to the control that opened it. Keep Back/B mappings consistent.
- Add contextual gamepad shortcuts (A, B, X, Y, L1, R1, etc.) where useful, with clear on-screen hints.
- Devices may have no **physical** keyboard attached, but text and number entry can use the OS-provided
  virtual keyboard. Use standard Qt text fields and appropriate input-method hints for numeric/text input.
- Keep navigation straightforward. Do not replace ordinary text fields with custom D-pad digit pickers,
  controller keyboards or extra input wizards when the OS virtual keyboard covers the task.

## Conventions
- Each source file starts with the GPL-3.0 license header (`Copyright (c) 2015-2026, BogDan Vatra`).
- Uses `#pragma once` and `using namespace Qt::StringLiterals`: `u"..."_s` and `u'c'`, never `QLatin1String`, `QLatin1Char`, `QStringLiteral` or `_()` helpers.
- For standard strings, prefer `"..."s` (`std::string`) and `"..."sv` (`std::string_view`) with `using namespace std::literals;` in implementation files or function bodies alongside Qt's literals. Include `<string>` or `<string_view>` directly as needed; keep namespace-wide using directives out of public headers. Keep C string literals for APIs requiring null-terminated character pointers, including Qt translation and meta-object APIs.
- No raw owning pointers and no singletons in ZxKit; ZxKit lookups return `OptionalRef` (a reference that may be missing).
- The existing spelling `Disassamble` is used in class and file names. Keep it consistent.
