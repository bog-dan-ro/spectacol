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
sets its size). `--ui-keys F1,Down,Return` presses keys in the window (one every 300 ms from the first second on),
to reach a page before the screenshot; `click:X:Y` in the list clicks at a window position (the desktop menu bar). The first run with new settings shows the welcome page, which pauses the emulation. To skip it,
and to keep the data (snapshots, downloads) out of your own folder, start with this
`$XDG_CONFIG_HOME/Licentia/Spectacol.conf` (`[General]` holds the top level keys, `[%General]` the "General" group of
the settings page):

```ini
[General]
first_start=false
dataPath=/tmp/spx/data/Spectacol/
```

## Android

The Android build has not been ported to CMake and Qt 6 yet (see the Android section of `TODO.md`). It will need
the Qt for Android kit, the Android SDK and NDK, and the OpenSSL libraries of the `android_openssl` submodule:

```sh
git submodule update --init --recursive
```
