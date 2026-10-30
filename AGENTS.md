# Agent instructions

## Project

Spectacol is a ZX Spectrum emulator: the emulation core is [FUSE](http://fuse-emulator.sourceforge.net/) + libspectrum (C), and the UI is Qt 6 / QML, designed for gamepad and touch use. It targets desktop (Linux) and Android. GPL-3.0.

## Build

The project uses CMake and needs Qt ≥ 6.11 (Quick, Multimedia, ShaderTools) and C++23. `BUILD.md`, `spectacol/spectacol.pro`, `spectacol/qml.qrc` and `spectacol/deployment.pri` describe the old qmake/Qt 5 build and are stale. The CMake build does not use them.

```sh
git submodule update --init --recursive   # android_openssl (Android only)
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64
cmake --build build
./build/bin/spectacol [file.tzx|.tap|.z80|...]   # args are passed through to fuse_main
```

Configuring takes a while and needs network access. The top-level `CMakeLists.txt` uses `FetchContent` to pull three dependencies:
- `qtgamepadlegacy` (bog-dan-ro fork, branch `dev`), which provides `Qt::GamepadLegacy`.
- The author's forks of **libspectrum** and **fuse** from SourceForge (`master`). The `FetchAndBuildLibrary` macro builds both with autotools (`autogen.sh` → `configure` → `make` → `make install` into the build dir) via `execute_process` **at configure time**, then wraps them as imported static libraries. For Android it sets up the NDK clang toolchain env vars. Changes to fuse or libspectrum have to go into those forks (or into the copy under `build/_deps/`); they are not part of this repo.

The repo has no tests and no lint setup.

## Architecture

### FUSE ↔ Qt bridge
FUSE is a C program that expects a platform "UI" layer. Spectacol builds FUSE with `--with-no-ui` and implements that layer in C++ through `extern "C"` functions:
- `qmlui.cpp`: `ui_*` hooks (init, events, errors, queries, menus, debugger hooks).
- `zximage.cpp`: `uidisplay_*` hooks. FUSE draws pixels into `ZxImage`, a singleton. Only the FUSE thread touches its back buffer (no locking). At each frame end it copies the dirty bounding rect into a mutex-guarded front image that the renderer takes copy-on-write snapshots of (`takeFrame()`).
- `fuseemulator.cpp`: `sound_lowlevel_*`, which outputs audio through `QAudioSink`.
- `compat.cpp`: FUSE's `compat_*` file, dir, path and timer functions, implemented with Qt. These replace FUSE's own compat layer, apart from the few unix compat and timer sources compiled directly from `${fuse_import_SOURCE_DIR}` in `spectacol/CMakeLists.txt`.

### Threading model (important)
- `FuseThread` (in `fuseemulator.cpp`) runs `fuse_main()`. **All FUSE state must only be touched on this thread.**
- From the GUI thread, call `pokeEvent(std::function)` (`qmlui.h`). It queues a lambda that the FUSE thread runs during its UI event polling in `qmlui.cpp`.
- From the FUSE thread back to the GUI thread, use `FuseObject::callFunction` / `callFunctionDelayed` (`fuseobject.h`). It posts a custom `QEvent` that carries a lambda. All the Qt-side model classes derive from `FuseObject`.
- Rendering: `ZxImage` emits `needsUpdate`. `ZxScreen` (a plain `QQuickItem`, the QML element) fills its parent and draws the image through a `QSGSimpleTextureNode` placed at the fill-mode rect. `ZxTexture` (a `QSGTexture`) uploads only the dirty rect in `commitTextureOperations()`.
- The FUSE thread is stopped on `QCoreApplication::aboutToQuit` (`FuseEmulator::stopFuseThread()`), before the statics it uses are destroyed. Nothing on the FUSE thread may block waiting for the GUI thread.

### QML side
- `qt_add_qml_module(spectacol URI Spectacol)`. `main.cpp` calls `engine.loadFromModule("Spectacol", "Main")`. C++ types register through `QML_ELEMENT`/`QML_SINGLETON` macros.
- `FuseEmulator` is the central QML singleton. It exposes emulator state (paused, registers as hex strings, filters, joysticks) and owns the sub-objects/models: `FuseTape`, `FuseRecording`, `FuseSettings`, `BreakpointsModel`, `DisassambleModel`, `PokeFinderModel`, `ZXGamesModel` (online game search via `zxrequests`).
- `SpectrumScreenImageProvider` (`image://spectrum/...`) renders `.scr` and snapshot screenshots for the file browser and cover flow.
- `z80assembler.cpp` plus `z80_opcodes.csv` implement the inline assembler that the debugger uses.

### Adding files
New C++/QML sources, shaders, images and ROMs must be listed explicitly in `spectacol/CMakeLists.txt` under `qt_add_qml_module`, `qt_add_shaders` or `qt_add_resources`. There is no globbing. Shaders are Vulkan-style GLSL, precompiled by `qt_add_shaders` for GLSL "100es,120,150,310es,450".

`spectacol/shaders/slang_parser.py` is a standalone helper for converting libretro slang shaders. It is not part of the build.

## Conventions
- Each source file starts with the GPL-3.0 license header (`Copyright (c) 2015-2025, BogDan Vatra`).
- Uses `#pragma once`, `using namespace Qt::StringLiterals` (`u"..."_s`), and wraps FUSE headers in `extern "C" { }`.
- The existing spelling `Disassamble` is used in class and file names. Keep it consistent.
