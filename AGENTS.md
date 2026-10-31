# Agent instructions

## Project

Spectacol is a ZX Spectrum emulator: the emulation core is [ZxKit](https://github.com/bog-dan-ro/ZxKit) (C++23), and the UI is Qt 6 / QML, designed for gamepad and touch use. It targets desktop (Linux) and Android. GPL-3.0.

## Build

The project uses CMake and needs Qt ≥ 6.11 (Quick, Multimedia, ShaderTools) and C++23. `BUILD.md`, `spectacol/qml.qrc` and `spectacol/deployment.pri` describe the old qmake/Qt 5 build and are stale. The CMake build does not use them.

```sh
git submodule update --init --recursive   # android_openssl (Android only)
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64
cmake --build build
./build/bin/spectacol [--machine ID] [--list-machines] [file.tzx|.tap|.z80|.nex|...]
# headless check: QT_QPA_PLATFORM=offscreen ./build/bin/spectacol --machine next --quit-after 14 --screenshot next.png
```

Configuring needs network access. The top-level `CMakeLists.txt` uses `FetchContent` for:
- `qtgamepadlegacy` (bog-dan-ro fork, branch `dev`), which provides `Qt::GamepadLegacy`.
- ZxKit: the parent directory when Spectacol lives in ZxKit's `tests/spectacol`, otherwise from GitHub. ZxKit itself fetches SafeCpp (`v1.0`).

The ROMs are Qt resources under `spectacol/roms/`, ZxKit gets them through the `romResolver` set in `ZxEmulator`. The ZX Spectrum Next boots from an SD card image that `NextSdCard` downloads (like jnext does) into the app data dir.

Spectacol has no tests of its own (ZxKit has them); check changes with the headless run above.

## Architecture

### ZxKit
- `zxkit::Emulator` is the whole computer (machine, peripherals, media, debugger). It is shared with the emulation thread as `zxkit::Shared<Emulator>`: the only way in is `writeLock()` / `readLock()`.
- `zxkit::Engine` runs it in real time on its own thread, locking it one scanline at a time (a fair spin lock), so the GUI thread can lock it directly: it waits at most a scanline.
- Frames, audio, pauses (breakpoints) and errors come through `Weak` callbacks owned by `CallbackHandle`s in `ZxEmulator`, called on the Engine thread without any lock held.

### Threading model (important)
- GUI thread code talks to the emulator synchronously: `ZxEmulator::instance().emulator().writeLock()->...`, or `withEmulator(fn)` which also shows a ZxKit error to the user. Keep the locked parts short: read files before taking the lock (`loadMediaFile()`), write them after releasing it (`saveMediaFile()`). The lock is not recursive: don't call something that locks again while holding it.
- The Engine callbacks hop to the GUI thread with `QMetaObject::invokeMethod(this, ..., Qt::QueuedConnection)`.
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

## Conventions
- Each source file starts with the GPL-3.0 license header (`Copyright (c) 2015-2026, BogDan Vatra`).
- Uses `#pragma once` and `using namespace Qt::StringLiterals`: `u"..."_s` and `u'c'`, never `QLatin1String`, `QLatin1Char`, `QStringLiteral` or `_()` helpers.
- No raw owning pointers and no singletons in ZxKit; ZxKit lookups return `OptionalRef` (a reference that may be missing).
- The existing spelling `Disassamble` is used in class and file names. Keep it consistent.
