# Spectacol TODO

What is missing or incomplete. Items that need ZxKit work first point to ZxKit's `TODO.md`.

Open GitHub issues reviewed on 2026-11-01: a curated backlog, grouped below and merged with existing work.
Prioritize saving/loading, reliable controls and useful display options. Leave webcam/voice control, gesture
controls, netplay, Google Play sharing, new platform ports and the broad metadata/container service out of scope.

## UI review

- [ ] **The UI looks childish and dated.** The main culprits:
  - a monospace font for all the UI text (`FancyText`, menus, pages);
  - an orange outline glow on labels (`FancyText`: `style: Text.Outline`, `styleColor: "#66ff2200"`);
  - bouncy slide-in panes (`FancyPane`: `Easing.InOutBack`, amplitude 2);
  - stock Material Dark with a Teal accent, no design of its own;
  - inconsistent corner radii and icon styles (SVG and PNG icons mixed);
  - the message banner covers the top of the pages (the drive picker's "Insert disk into" title, for one) (fixed:
    a toast that stays clear of the page).

  Plan: a small design system first (a type scale with a proportional UI font, monospace only for code and
  registers; colour tokens; spacing; one icon set), calmer motion, a desktop layout (menu bar or side panel)
  besides the touch and gamepad one, then restyle page by page.
  - [x] Audit from headless screenshots (`--ui-screenshot`, `--ui-keys` reach any page) and a proposal: a `Theme`
        singleton (colour roles, a type scale with a TV size, spacing, motion) and two directions, a calm overlay
        with Spectrum accents or the 128K ROM menu look, plus an optional desktop mode.
  - [x] Restyle, direction A (calm overlay, Spectrum accents) with the platform font: `Theme` (colour roles, type
        scale, spacing, motion; "Large text" and "Reduce motion" settings), `Sheet` (every page's frame), `HintBar`
        (gamepad glyphs or keys, clickable), `SettingRow`/`SettingsList`, `OptionList`, the toast and the status chips.
        Restyled: the menu, General/Sound/Peripherals, joystick, machine (grouped) and drive pickers, About, the file
        browser's chrome, the tape browser, the SD card pages, the debugger's colours, the remaining pages' colours.
  - [x] Desktop layout (Options › General › Layout: Auto picks it without a gamepad or a touch screen): a menu bar
        built from the same menu, a status bar, pages docked beside the screen (the tape browser keeps the tape
        playing).
  - [ ] Options › Screen and the filters list keep the old look: they hold the screen filter UI (owner's work).
  - [ ] Still the old structure, only recoloured: poke finder and manager, online search, the save pages, the
        gamepad pages (customize, calibrate), the on-screen keyboard and joysticks.
  - [ ] One icon set (the cover flow's 3D folder and floppy art, SVG and PNG mixed).
  - [ ] Desktop menu shortcuts shown next to the items, and more shortcuts (reset, pause, screenshot).

## ZX Spectrum Next

- [x] **SD card page** (Media → SD card, on the Next): the image (path, free space), its folders and files, copy a
      host file onto the card, make a folder, remove a file or an empty folder (the Next restarts after a change),
      pick another image or go back to the downloaded one.
  - [ ] Re-download or update the distro (a newer NextZXOS) from the page.
  - [ ] The DivMMC on the other machines: ZxKit's `DivMmcPeripheral` (esxDOS) isn't offered in the peripherals
        page yet; it would use the same card (and this page).
  - [ ] Adding a file goes through the platform's file dialog: a gamepad can't drive it (the in-app cover flow
        browser could pick the file instead).

## Features and fixes

- [x] Joysticks: the gamepad drives the joystick picked in the list (remembered): Cursor and Sinclair 1/2 press
      their keys, Kempston and Fuller attach their interfaces. Picking one no longer attaches Interface 1/2.
  - [ ] "Timex 1/2" are gone from the list: ZxKit's TC2068/TS2068 have no AY, which reads their joysticks.
- [x] CoBra 1 keys: on a physical keyboard Ctrl is CTRL, Esc is ESC, Scroll Lock is NO SCROLL, the keypad's Enter
      is LINE FEED, F1-F4 are F1-F4 (checked headless: `--ui-keys` gained `down:KEY` / `up:KEY`, CTRL+0 starts
      ROM CP/M from the boot manager; scripted key taps last 80 ms so the emulated keyboard scan sees them). The on-screen keyboard's Control stays Symbol Shift. Spectacol's keys
      moved to F5 on: F5 the menu (was F1), F6 the quick open (was F4); listed in the README.
  - [x] A CoBra on-screen keyboard: all eight sixth-column keys, independent CTRL/SYMBOL
        SHIFT, sticky CTRL for touch/gamepad chords, and release on hiding/canceling.
- [ ] `.pok` trainers: ZxKit parses them, but the poke manager only has custom pokes. Load the `.pok` file that
      comes with a game; also search a user-selected POKES folder by game name
      ([#142](https://github.com/bog-dan-ro/spectacol/issues/142)).
- [x] Disk UI for the +3 and TR-DOS drives: the Disks menu is generic (two drives on the +3, four on the Pentagon
      and the Scorpion; checked headless with `--ui-keys`).
  - [ ] Opening a disk loads it into drive A: only: boot it as FUSE does (reset into TR-DOS, the +3 Loader), once
        ZxKit has a disk auto-load API.
  - [ ] Microdrives and IDE images, as ZxKit gains them. For the microdrive part of
        [#152](https://github.com/bog-dan-ro/spectacol/issues/152): insert, eject, save and save as `.mdr`.
- [x] Status indicators: the tape and disk icons follow ZxKit's media activity (the SD card lights the disk icon),
      and the tape browser follows the tape's current block.
  - [ ] Pause and fast-forward indicators visible when using a physical gamepad; keep the paused state discoverable
        ([#94](https://github.com/bog-dan-ro/spectacol/issues/94); `UiItemType::Paused` is never shown).
- [ ] Debugger: a memory view, a watchpoints UI, a Next registers and MMU view, loading SLD/symbol files, step out
      ([#27](https://github.com/bog-dan-ro/spectacol/issues/27); needs ZxKit's step-out support).
- [ ] Sound at speeds other than 100% (muted today, like FUSE).

## Display

- [x] **Border sizes configurable at runtime** ([#137](https://github.com/bog-dan-ro/spectacol/issues/137)).
      Options → Borders: independent top, left, bottom and right sizes, plus Wide, Full, None and Default presets.
      ZxKit supplies a full frame and its ULA paper rectangle; cropping lives entirely in Spectacol and updates
      immediately even while paused. Settings persist; aspect ratio, software filters and native PNG screenshots
      follow the crop. The existing Screen/filter page is unchanged. `tests/borders.py` checks the compiled UI.

## Controls

- [ ] **Map directions to Spectrum keys**, for touch and physical gamepads: a QAOP+fire preset and custom keys
      ([#129](https://github.com/bog-dan-ro/spectacol/issues/129),
      [#131](https://github.com/bog-dan-ro/spectacol/issues/131),
      [#3](https://github.com/bog-dan-ro/spectacol/issues/3)). Buttons already map to keys, but the sticks always
      drive the selected joystick. Map axis directions with a dead zone and release keys on neutral/disconnect.
- [ ] Save/load named gamepad mapping profiles, with per-game selection and import/export
      ([#116](https://github.com/bog-dan-ro/spectacol/issues/116), duplicate
      [#121](https://github.com/bog-dan-ro/spectacol/issues/121)). Keep profiles independent of a new game container.
- [ ] Expose Select and stick-click buttons in the mapping UI; decide how Start can be mapped while keeping a
      reliable way to open the menu ([#117](https://github.com/bog-dan-ro/spectacol/issues/117)).
- [ ] Fix stuck keys on the touch keyboard with overlapping touches, cancellation and hiding the keyboard
      ([#100](https://github.com/bog-dan-ro/spectacol/issues/100)); track each held key, including shared modifiers.
- [ ] Configurable opacity for the on-screen keyboard, joysticks and action buttons
      ([#102](https://github.com/bog-dan-ro/spectacol/issues/102)).
- [ ] Choose hold-to-fast-forward or press-to-toggle for touch/gamepad controls
      ([#110](https://github.com/bog-dan-ro/spectacol/issues/110)); release the speed override safely on disconnect.
- [ ] Android TV remote navigation ([#124](https://github.com/bog-dan-ro/spectacol/issues/124)): verify Menu,
      Back, D-pad and OK/Select through the current UI, including toggles and file selection, without a mouse.

## Files, saving and playback

- [ ] **Portable save names** ([#153](https://github.com/bog-dan-ro/spectacol/issues/153),
      [#155](https://github.com/bog-dan-ro/spectacol/issues/155)): replace `hh:mm:ss` in generated tape/RZX names
      with `hh_mm_ss`, sanitize the program-name component, and check/report write failures.
- [ ] **Save BASIC to a tape** ([#155](https://github.com/bog-dan-ro/spectacol/issues/155)): make a new blank tape,
      collect ROM `SAVE` blocks and export a reusable `.tap`; distinguish this from RZX input recording. The
      current SaveTape page asks for `.tzx`, but ZxKit only writes `.tap`: offer supported formats, then TZX when
      ZxKit gains its writer. Explain the workflow and verify save/reload on an empty tape.
- [ ] Tape browser: an explicit reset-and-auto-load-from-selected-block action
      ([#76](https://github.com/bog-dan-ro/spectacol/issues/76)); selecting a block currently only seeks the deck.
- [ ] Custom ROM selection per machine ([#152](https://github.com/bog-dan-ro/spectacol/issues/152)): use the
      existing ZxKit `romResolver`, validate the required ROM sizes, persist overrides and allow restoring bundled ROMs.
      Snapshot saving already exists; its Android failures belong to the save-name and storage tasks.
- [ ] A compact list/grid alternative to cover flow, with remembered folder/selection and local filename search
      ([#18](https://github.com/bog-dan-ro/spectacol/issues/18),
      [#32](https://github.com/bog-dan-ro/spectacol/issues/32)); usable with touch, keyboard and gamepad.
- [ ] **ZXInfo online search**: add [zxinfo.dk](https://zxinfo.dk/) alongside the existing Internet Archive search,
      using its public ZXDB REST API ([v5 OpenAPI specification](https://github.com/zxinfo-api/zxinfo-api-v5/blob/main/docs/swagger.yml),
      [Swagger UI](https://zxinfo-api.github.io/zxinfo-api-v5/)). Integrate paginated results, title details,
      screenshots and available media links with the current search/download flow; show the source and handle
      unavailable results or network errors. Confirm the production API base URL/version when implementing.
- [ ] Show RZX playback progress ([#47](https://github.com/bog-dan-ro/spectacol/issues/47)) once ZxKit exposes it.
      Recheck the compatibility reports in [#48](https://github.com/bog-dan-ro/spectacol/issues/48) after the core
      work in ZxKit's `TODO.md`; surface playback/desynchronization errors to the user.

## Issues already implemented (source review)

- [x] Sticky touchscreen CS/SS ([#97](https://github.com/bog-dan-ro/spectacol/issues/97)): Main's
      `stickyCSSSButton` enables `OnScreen48Keyboard.stickyCSSS`; modifier taps latch until the next ordinary key
      is released (or the modifier is tapped again). Gamepad mode also enables sticky modifiers. Device validation
      belongs to the Android control review below; no new implementation task is needed for the original request.
- [x] Confirm machine selection ([#119](https://github.com/bog-dan-ro/spectacol/issues/119)):
      `MachineTypePage` moves `focusIndex` while navigating and only selects through `choose()` on confirmation/click.

## Android (needs a review)

Postponed. To test: the SDK is in `~/android` (emulator images for APIs 29/31/35, an API 31 tablet AVD, NDKs up to
26.1); it needs Qt 6.12 for Android kits (x86_64 for the emulator, arm64 for a phone) and, first, the Android build
ported to CMake (`qt_android_*` properties, the manifest and `android_openssl`).

- [ ] **Storage** ([#154](https://github.com/bog-dan-ro/spectacol/issues/154), storage part of
      [#153](https://github.com/bog-dan-ro/spectacol/issues/153)). The data folder defaults to
      `/storage/emulated/0/Spectacol/` (`GenericDataLocation`), which
      apps can't use any more: `requestLegacyExternalStorage` is ignored from target SDK 30 and the
      `READ/WRITE_EXTERNAL_STORAGE` permissions no longer give access to shared storage. Keep Spectacol's own
      data (snapshots, recordings, saves, favourites, downloads, the Next SD card image) in app storage
      (`AppDataLocation`), and reach the user's files through Qt 6's shared storage support: the native
      `FileDialog` / `FolderDialog` (`QtQuick.Dialogs`, the Storage Access Framework on Android) to open and
      save files or to grant a games folder (persist the permission), and `QFile` / `QDir` on the `content://`
      URIs they return. Migrate the old `/sdcard/Spectacol` data while it is still readable.
- [ ] The in-app file browser (`FolderListModel`, the cover flow) walks file system paths: make it work on the
      granted folder (`content://`) and on app storage.
- [ ] Opening a file from another app: the intent filter takes `content://` URIs, but loading goes through
      ZxKit's `MediaFile::read()` (`std::ifstream`), which can't open them. Read them with `QFile` and use
      `MediaFile::fromBytes()`.
- [ ] Manifest: drop the obsolete storage permissions and `requestLegacyExternalStorage`; review the target and
      minimum SDK levels and the 16 KB page size requirement of current Android releases.
- [ ] Build and test with the new ZxKit (C++23 NDK toolchain, SafeCpp fetched by CMake), then gamepads, the
      on-screen controls, the back button and audio latency on a device.
- [ ] APK installation regression ([#156](https://github.com/bog-dan-ro/spectacol/issues/156)): verify signed
      release APKs install and upgrade on Android 13+ with the intended ABIs once the CMake port is ready; diagnose
      the reported invalid-package error with the actual APK rather than assuming a cause.

## Cleanup

- [x] Remove the stale qmake / Qt 5 files the CMake build doesn't use: `spectacol/qml.qrc` and
      `spectacol/deployment.pri`; rewrite `BUILD.md` for CMake and Qt 6.
- [x] Signals QML listens to that nothing emitted (FUSE UI hooks): `hideDebugger` closes the debugger page when it
      deactivates, `configureJoystick` opens the joystick list when a snapshot loads with a gamepad connected (the
      "Snapshot joystick prompt" setting), `hideMenu` is gone, `uiIconUpdate` follows the media activity.
- [ ] Settings that do nothing: `speakerType` (it needs ZxKit's speaker models). `joystickPrompt` (see above) and
      `recreatedSpectrum` (the Recreated ZX Spectrum keyboard's game mode, FUSE's decoding) work now. (`detectLoaders`, `loaderAcceleration` and
      `loadingSound` drive ZxKit's `TapeOptions` now: auto play, acceleration ("Turbo" also runs the emulation flat
      out while the tape plays), loading sound.)
- [ ] Singletons (`ZxEmulator::instance()`, `ZxImage::instance()`, `NextSdCard::instance()`) and raw pointers.
