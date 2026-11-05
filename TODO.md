# Spectacol TODO

What is missing or incomplete. Items that need ZxKit work first point to ZxKit's `TODO.md`.

Open GitHub issues reviewed on 2026-11-01: a curated backlog, grouped below and merged with existing work.
Work in the priority order below; audio continuity takes precedence over frame delivery.
Leave webcam/voice control, gesture controls, netplay, Google Play sharing, new platform ports and the broad metadata/container service out of scope.

## Audio

- [x] **GUI-independent audio delivery.** Engine samples enter a bounded queue directly; Qt's audio-thread
      callback consumes it without GUI event delivery, locks or allocation. Pause/mute generations discard stale
      samples with short fades. A live capture with repeated 500 ms GUI stalls and 100%/200%/50%/turbo playback
      has zero underruns/overflows and no silent 10 ms windows. Keep physical-device listening checks below.
- [x] AY quality: use ZxKit's band-limited synthesis and measured AY/YM volume curves, with an AY-3-8912 /
      YM2149 choice in Sound settings. Core regressions check alias rejection, envelope shapes and rewind;
      physical-device listening with real game music and effects remains below.
- [x] Speaker models: Sound settings select ZxKit's TV speaker, Beeper and Unfiltered responses, with persistent
      choices, safe fallback for older settings and smooth runtime transitions.
- [x] Audio output pacing: adaptive resampling corrects device-clock drift without changing emulation speed.
      Deterministic regressions cover 44.1/48/96 kHz, 50/100/200% speed and ten virtual minutes of clock mismatch.
      Packet timing keeps latency/phase continuous across speed changes; turbo estimates exclude paused time.
- [x] Sound at other speeds: resample fixed speeds and estimate the input rate during unbounded/manual/tape
      turbo, instead of muting. Pitch follows emulation speed; this is not pitch-preserving time stretching.
- [ ] Listening/latency checks on physical desktop and Android devices, including the actual games, Bluetooth
      outputs and device changes. Android x86_64/ARM64 packages compile with the new Qt 6.12 callback path.

## RZX compatibility and progress

- [x] RZX playback progress ([#47](https://github.com/bog-dan-ro/spectacol/issues/47)): desktop status and overlay
      show current/total frames and percentage, retaining completion/stop positions. Input desynchronization
      errors pause playback and surface through the normal error message without opening the debugger.
      ZxKit now plays all five recordings from [#48](https://github.com/bog-dan-ro/spectacol/issues/48) to completion.
      Recording uses ZxKit's fetch-counted API; pending saves remain retryable.
- [x] RZX recording/playback transport: start, pause, Stop/finalise at cursor, backward/forward seeking and
      seek-to-end. Seeking waits for a fresh gameplay input; resuming a recording discards its later attempt.
      Held controls, repeats, menu navigation and stick noise do not resume. F8/F9 and Media → Recording controls
      use the same core timeline; seeks run in small batches with the Engine paused.

## Media formats and disk auto-boot

- [x] PZX/CSW reading, TZX export, 128K SNA, Z80 and Next NEX writing. The export pages offer supported
      formats; SZX remains the snapshot default and NEX is labelled as a program export. Custom tapes default
      to TZX so no loading signals or control blocks are silently discarded.
- [x] Opening drive A with automatic loading enabled resets into TR-DOS or selects the +3 Loader through
      ZxKit's boot API. Inserting drive B does not reset the machine.
- [x] Tape browser: explicit “Reset and load from here” action (Y / L) for the selected block
      ([#76](https://github.com/bog-dan-ro/spectacol/issues/76)); ordinary selection still only seeks the deck.

## Controls, profiles and playback indicators

- [x] Named gamepad profiles with load/save, per-game selection and portable JSON import/export
      ([#116](https://github.com/bog-dan-ro/spectacol/issues/116), duplicate
      [#121](https://github.com/bog-dan-ro/spectacol/issues/121)). Per-game overrides preserve the default
      mapping; filename matching spans folders, quick snapshots and explicitly named snapshot exports.
      Profiles include directions, keys, button actions, joystick, dead zone and fast-forward mode.
      Snapshot joystick declarations retain priority. Import validation, collisions, persistence and actual
      keyboard/joystick routing have regressions; see `docs/controller-profiles.md`.
- [x] Independent opacity settings for the on-screen keyboard, joystick and action buttons
      ([#102](https://github.com/bog-dan-ro/spectacol/issues/102)); persist between sessions, without adding
      rendering layers or per-frame updates.
- [x] Hold or press-to-toggle fast-forward for touch/gamepad controls
      ([#110](https://github.com/bog-dan-ro/spectacol/issues/110)). Independent input ownership releases
      overrides on cancellation, disconnect, pause, mapping changes and backgrounding.
- [x] Pause and fast-forward indicators remain visible with physical controllers, independently of touch
      icon visibility ([#94](https://github.com/bog-dan-ro/spectacol/issues/94)). Desktop status and overlay
      chips follow the actual pause/manual/tape-turbo state; compiled UI checks cover both layouts.

## Files: trainers, ROMs and compact browsing

- [x] `.pok` trainers: automatically find a game’s adjacent `.pok`, then search the user-selected
      POKES folder by game name. Import explicitly, apply grouped/banked patches and requested values,
      and restore captured bytes on toggle-off
      ([#142](https://github.com/bog-dan-ro/spectacol/issues/142)).
- [x] Custom ROM selection per machine ([#152](https://github.com/bog-dan-ro/spectacol/issues/152)):
      machine-scoped overrides through ZxKit's ROM resolver, exact size checks, persistent private copies,
      and restoration of bundled/Next SD defaults at the next hard reset or machine switch.
      Snapshot saving already exists; its Android failures belong to the save-name and storage tasks.
- [x] A compact list/grid alternative to cover flow, with remembered folder/selection and local filename search
      ([#18](https://github.com/bog-dan-ro/spectacol/issues/18),
      [#32](https://github.com/bog-dan-ro/spectacol/issues/32)); usable with touch, keyboard and gamepad.

## Debugger

- [x] Memory view/editor with physical RAM/ROM banks and a masked, one-shot watchpoints UI.
- [x] Side-effect-free Next register latches and effective MMU read/write mappings.
- [x] D-pad/stick navigation, contextual gamepad shortcuts and Back support in the debugger and poke editor,
      with standard OS-keyboard text fields and physical writable-bank pokes beyond 64 KiB.
- [x] Load sjasmplus SLD v1 / symbol assignments, navigate labels and annotate disassembly with
      source file/line mappings for the currently mapped RAM page.
- [x] Step out and bank-aware run to cursor ([#27](https://github.com/bog-dan-ro/spectacol/issues/27)),
      with nested-call/interrupt handling in ZxKit and pause/continue controls.

See [Debugger controls and limits](docs/debugger.md) and [Files and symbols](docs/files-and-symbols.md).

## Performance and resource usage profiling

- [x] **Investigate the CPU/resource regression before further CI and UI polish.** User-reported CPU usage on
      this machine was about 0.2% just after the ZxKit port, before filtering; it is now about 0.6% without filters
      and 0.9–1.5% with some filters enabled. Reproduce these measurements with a consistent CPU percentage
      convention, optimized build, machine/media, speed and display workload; prioritize battery-powered devices.
- [x] Use `perf` for native C++ hotspots and the
      [Qt QML Profiler workflow](https://www.qt.io/blog/introducing-the-qml-profiler-skill-for-agentic-development)
      for QML logic, bindings/signals, rendering and memory traces. Compare the initial post-port/pre-filter
      version, the current unfiltered path and individual filters. Separate profiling overhead from production
      CPU measurements and distinguish emulation/audio, UI, frame copies/uploads and filter costs.
- [x] Record CPU time, memory/allocations and wakeups during gameplay, paused/menu states and backgrounding.
      Save reproducible traces and before/after results under the build directory. Fix measured bottlenecks
      without sacrificing audio continuity or changing the owner's screen/filter UI.

See [CPU and resource investigation](docs/profiling.md) for the historical pair,
htop conventions, idle/Dizzy results, all 36 presets, QML/native captures, fixes and
measurement limits. Generated evidence remains under the build directories.

## CI, tests, installation and remaining UI polish

- [ ] CI: build and run the native suite and compiled UI regressions; include Android packaging checks.
      Coordinate sanitizer coverage with ZxKit.
- [ ] Broader snapshot and boot coverage through the UI, alongside ZxKit's standalone per-machine tests.
- [ ] Classify the intermittent QML-cleanup SIGSEGV (`QQmlData::disconnectNotifiers`); its cause is
      unconfirmed. The older Qt Creator Profile build was `RelWithDebInfo` with `QT_ENABLE_QML_DEBUG=ON`.
      A historical offscreen RZX CLI failure also exists, but its original executable was overwritten.
      The owner cannot reproduce it in normal RelWithDebInfo use. Clean current RZX checks on 2026-11-04
      passed three times each with QML debugging disabled/enabled; a separate failure overlapped a build
      refresh and is confounded. Compare matching optimized binaries with debugging off/on and debugger
      attachment before blaming Qt or the application. Preserve matching binaries, flags and cores, and
      keep failed runs instead of hiding them with retries. Local evidence: `build/shutdown-review/` in ZxKit.


### UI review

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
  - [x] Compact Android page chrome: a 48 dp header (72 with Large text) with Back and an actions menu; retain
        48 dp touch targets. Hide keyboard shortcut footers on touch-only devices; keep connected-gamepad
        hints and OK/Back hints for non-touch Android devices. Narrow hint rows scroll rather than wrap.
        About/controller help scroll independently of the header, with compact phone tabs. Quick Open
        keeps view/search controls, moves file actions/device import to ⋮, and avoids reflecting its footer.
  - [ ] Options › Screen and the filters list keep the old look: they hold the screen filter UI (owner's work).
  - [ ] Still the old structure, only recoloured: poke finder, the
        gamepad calibration, the on-screen keyboard and joysticks.
  - [x] Controller mapping uses themed settings rows and a key/action dialog with D-pad navigation and focus
        restoration on close, including direction presets and custom keys.
  - [x] Tape creation/export and snapshot Save As use themed sheets with keyboard/gamepad actions and
        destination previews. Failed writes leave the editor open for retry.
  - [x] Online search keeps a gamepad-friendly grid of thumbnail cards. A/Enter opens a modal title/details/media
        dialog; Up/Down picks the media file and A plays it. B/Escape restores the same grid selection and scroll
        position. Full details and media are requested only after confirmation; X/Y reach search/source directly.
  - [x] One original SVG icon family using Theme's palette and rounded strokes, including the cover flow's
        folder/floppy placeholders, cassette states, touch controls and joystick directions. Controller help
        uses the same artwork with localizable QML labels and a spoken description. Game covers and launcher
        branding remain separate. See `spectacol/images/README.md` for the inventory and design rules.
  - [ ] Desktop menu shortcuts shown next to the items, and more shortcuts (reset, pause, screenshot).

### Android

Qt 6.12 is working with NDK r27c and JDK 21. The x86_64 and ARM64 APKs build; runtime checks use an
API 37 x86_64 emulator. Physical phone/TV validation remains open.

- [x] Port packaging to CMake / Qt 6: Qt 6 activity classes, SDK-generated Gradle files, fetched
      GamepadLegacy Java JAR, OpenSSL 3 and NDK C++23 support.
- [x] Keep app data in `AppDataLocation`; open/import and export files through the native document picker
      ([#154](https://github.com/bog-dan-ro/spectacol/issues/154), storage part of
      [#153](https://github.com/bog-dan-ro/spectacol/issues/153)). Local saves remain atomic;
      document-provider destinations use direct writes.
- [x] Open `content://` files from other apps on cold and warm starts; copy them into app storage and
      load through `QFile` / `MediaFile::fromBytes()`.
- [x] Drop obsolete storage permissions and `requestLegacyExternalStorage`; minimum API 28, current Qt
      target SDK, 16 KiB native library alignment. Build both x86_64 and ARM64 packages.
- [x] Detect touch hardware, show on-screen controls, balance pause/resume across application states and
      handle Android Back (dismiss input keyboard before closing the page).
- [x] Compact shared page/drawer chrome, with touch actions instead of keyboard-only footer hints.
      Hide Android system bars in fullscreen, remove the flat drawer's rounded-corner spacer, and align
      the landscape controls to the window's left edge independently of the ZX viewport; keep the
      Android ZX display aligned to the top right.
      Actions menus consume outside taps without closing the underlying page. The drawer focuses its
      list, handles D-pad/OK there and restores the newly opened page's focus.
      `tests/android_ui.py` covers landscape touch, Large text, native picker/ROM access and an explicit
      Android Menu/D-pad/OK/Back probe. The desktop file regression also drags a narrow hint row.
- [x] Default Android orientation to Any, preserving saved Landscape/Portrait choices. Apply the saved
      orientation after creating the Qt window so startup rotation also resizes its surface. On the
      Qt 6.12/API 37 phone emulator, `tests/android_ui.py --rotation-only` covers fresh defaults, both
      saved choices and live changes through Options › Screen; physical phone rotation remains to verify.
- [ ] Offer migration of old shared data when readable. The old directory is preserved and its location
      is remembered as `Android/legacyDataPath`; individual files can already be imported through the picker.
- [ ] Browse a granted games folder (`content://`) in the in-app cover flow, retaining its URI permission.
      The cover flow currently browses app storage; shared files use the native picker.
- [ ] Validate physical gamepads, on-screen controls, audio latency and Android TV remote navigation on devices.
- [ ] APK installation regression ([#156](https://github.com/bog-dan-ro/spectacol/issues/156)): emulator
      debug APK install/upgrade and signature/alignment checks pass. Verify release-signed APKs and upgrades
      with the actual publishing key on Android 13+ phones and TVs; diagnose the reported invalid package
      using that APK.
- [ ] Android TV remote navigation ([#124](https://github.com/bog-dan-ro/spectacol/issues/124)): verify Menu,
      Back, D-pad and OK/Select through the current UI, including toggles and file selection, without a mouse.

### Cleanup

- [x] Previously inactive settings now work: `joystickPrompt` controls the snapshot joystick prompt;
      `recreatedSpectrum` uses FUSE's Recreated ZX Spectrum keyboard decoding. `detectLoaders`,
      `loaderAcceleration` and `loadingSound` drive ZxKit's `TapeOptions`: auto play, acceleration
      (Turbo runs the emulation flat out while tape plays) and loading sound.
- [x] Remove the stale qmake / Qt 5 files the CMake build doesn't use: `spectacol/qml.qrc` and
      `spectacol/deployment.pri`; rewrite `BUILD.md` for CMake and Qt 6.
- [x] Signals QML listens to that nothing emitted (FUSE UI hooks): `hideDebugger` closes the debugger page when it
      deactivates, `configureJoystick` opens the joystick list after the file browser closes when a snapshot
      has no single supported joystick declaration and "Ask for the joystick" is enabled, `hideMenu` is gone, `uiIconUpdate` follows the media activity.
- [ ] Singletons (`ZxEmulator::instance()`, `ZxImage::instance()`, `NextSdCard::instance()`) and raw pointers.

## Completed features

- [x] Joysticks: the gamepad drives the joystick picked in the list (remembered): Cursor and Sinclair 1/2 press
      their keys, Kempston and Fuller attach their interfaces. Timex 1/2 feed the TC/TS2068 AY ports.
      Z80/SZX metadata selects a single supported type automatically and suppresses the chooser; absent,
      ambiguous or unsupported declarations use the prompt setting, including without a physical gamepad.
      SZX exports preserve every supported type; Z80 exports preserve the types its header can represent.
      Picking one no longer attaches Interface 1/2.
- [x] Disk UI for the +3 and TR-DOS drives: the Disks menu is generic (two drives on the +3, four on the Pentagon
      and the Scorpion; checked headless with `--ui-keys`).
- [x] Status indicators: the tape and disk icons follow ZxKit's media activity (the SD card lights the disk icon),
      and the tape browser follows the tape's current block.
- [x] CoBra 1 keys: on a physical keyboard Ctrl is CTRL, Esc is ESC, Scroll Lock is NO SCROLL, the keypad's Enter
      is LINE FEED, F1-F4 are F1-F4 (checked headless: `--ui-keys` gained `down:KEY` / `up:KEY`, CTRL+0 starts
      ROM CP/M from the boot manager; scripted key taps last 80 ms so the emulated keyboard scan sees them). The on-screen keyboard's Control stays Symbol Shift. Spectacol's keys
      moved to F5 on: F5 the menu (was F1), F6 the quick open (was F4); listed in the README.
  - [x] A CoBra on-screen keyboard: all eight sixth-column keys, independent CTRL/SYMBOL
        SHIFT, sticky CTRL for touch/gamepad chords, and release on hiding/canceling.
- [x] **Map directions to Spectrum keys**, for touch and physical gamepads: a QAOP+Space preset and custom keys
      ([#129](https://github.com/bog-dan-ro/spectacol/issues/129),
      [#131](https://github.com/bog-dan-ro/spectacol/issues/131),
      [#3](https://github.com/bog-dan-ro/spectacol/issues/3)). Options → (Virtual) Gamepad → Controller mapping
      chooses joystick, QAOP or custom Up/Down/Left/Right/Fire keys. Both sticks, the D-pad and touch have separate
      ownership and respect the dead zone. Neutral/disconnect, hidden/cancelled touch, menus/pause, reset and
      mapping changes release held input. Shared keys/modifiers/fire stay held until every owner releases them.
      Native controller tests and `tests/controller.py` verify state and actual emulated keyboard/Kempston ports.
- [x] Expose Select and stick-click buttons in the mapping UI; Start remains reserved for the menu
      ([#117](https://github.com/bog-dan-ro/spectacol/issues/117)).
- [x] Fix stuck keys on the touch keyboard with overlapping touches, cancellation and hiding the keyboard
      ([#100](https://github.com/bog-dan-ro/spectacol/issues/100)). Each finger/navigation press owns its original
      key and sticky chord modifiers; shared keys remain down until their last owner releases. Cancellation is
      local to its points; hiding, pause, deactivation and mode changes clear all holds. Native keyboard-state
      tests and `tests/keyboard.py` cover the compiled UI and real emulated keyboard ports.
- [x] **Portable save names** ([#153](https://github.com/bog-dan-ro/spectacol/issues/153),
      [#155](https://github.com/bog-dan-ro/spectacol/issues/155)): tape exports, quick snapshots and RZX files use
      `hh_mm_ss_zzz` timestamps and sanitized filename components. Shared `QSaveFile` writes check open, write and
      commit errors and preserve existing files on failure. Failed RZX saves keep their bytes in memory and offer
      "Retry recording save". Android document-provider export is covered below.
- [x] **Save BASIC to a tape** ([#155](https://github.com/bog-dan-ro/spectacol/issues/155)): Media → Tape → New blank
      tape keeps the program in RAM; ROM `SAVE` collects blocks, and Tape → Save exports `.tap` with a destination
      preview. The page explains the difference from RZX and stays open on failure. Tape metadata follows new SAVE
      blocks and clears on a new tape. `tests/saving.py` checks real-ROM SAVE, TAP checksums and exact program bytes
      after reload. TZX export remains dependent on ZxKit's writer.
- [x] **Snapshot Save As**: enable Media → Save snapshot as and wire its editor to the supported `.szx` writer,
      with portable names, destination preview and retry on failure. Quick Load compares file times across root
      Save As files and per-game folders, and never deletes an unreadable snapshot. The saving regression checks
      a BASIC program after a snapshot round trip; broader per-machine snapshot coverage remains in ZxKit's TODO.
- [x] **ZXInfo online search**: choose [zxinfo.dk](https://zxinfo.dk/) or Internet Archive, with paginated title
      search and browsing by initial. ZXInfo uses the public [v5 REST API](https://github.com/zxinfo-api/zxinfo-api-v5/blob/main/docs/swagger.yml)
      at `https://api.zxinfo.dk/v5/` (zero-based page offsets). Select a title for details, screenshots and supported
      media choices; download explicitly to Favourites for offline play. ZIP tapes auto-load using the unpacked
      media type. Requests cancel on source/query changes and closing; errors stay visible for retry, and failed
      downloads preserve existing files. `tests/test_online.cpp` checks both sources with an offline HTTP server;
      `tests/saving.py` also checks that a zipped BASIC tape loads through the ROM.
- [x] **SD card page** (Media → SD card, on the Next): the image (path, free space), its folders and files, copy a
      host file onto the card, make a folder, remove a file or an empty folder (the Next restarts after a change),
      pick another image or go back to the downloaded one.
  - [x] Re-download/update the distro from the page, preserving the previous default image as a backup.
        Failed/cancelled updates keep the current card; offline HTTP tests cover replacement and recovery.
  - [x] Add host files and select SD images through the in-app browser using a gamepad.
- [x] ESP/Pi UART modules: QJSEngine lives on the ZxKit Engine thread; direct byte hooks, host TCP/UDP,
      timers, bounded queues and a script watchdog. Local/HTTPS scripts can be selected and reloaded without
      rebuilding; see `UART.md`. Pi bridges to a host service; it does not emulate Linux or I2S.
- [x] Next extended keys on the physical/on-screen keyboards and separate Multiface/drive NMI actions.
- [x] Supply host local RTC time and persist its battery RAM.
- [x] **Border sizes configurable at runtime** ([#137](https://github.com/bog-dan-ro/spectacol/issues/137)).
      Options → Borders: independent top, left, bottom and right sizes, plus Wide, Full, None and Default presets.
      ZxKit supplies a full frame and its ULA paper rectangle; cropping lives entirely in Spectacol and updates
      immediately even while paused. Settings persist; aspect ratio, software filters and native PNG screenshots
      follow the crop. The existing Screen/filter page is unchanged. `tests/borders.py` checks the compiled UI.
- [x] Sticky touchscreen CS/SS ([#97](https://github.com/bog-dan-ro/spectacol/issues/97)): Main's
      `stickyCSSSButton` enables `OnScreen48Keyboard.stickyCSSS`; modifier taps latch until the next ordinary key
      is released (or the modifier is tapped again). Gamepad mode also enables sticky modifiers. Device validation
      belongs to the Android control review above; no new implementation task is needed for the original request.
- [x] Confirm machine selection ([#119](https://github.com/bog-dan-ro/spectacol/issues/119)):
      `MachineTypePage` moves `focusIndex` while navigating and only selects through `choose()` on confirmation/click.

## Deferred clones and extensions — lowest priority

Keep these at the very end. DivMMC/esxDOS on classic machines remains postponed.

- [ ] Microdrives and IDE images, as ZxKit gains them. For the microdrive part of
      [#152](https://github.com/bog-dan-ro/spectacol/issues/152): insert, eject, save and save as `.mdr`.
- [x] Timex 1/2: restored alongside None, Cursor, Kempston, Sinclair 1/2 and Fuller; real AY joystick
      input on TC2068/TS2068. Incompatible machine choices remain in the chooser with an explanation.
- [ ] ULAplus on classic machines: expose it only after ZxKit implements it; keep this behind the priorities above.
- [ ] Twister tape acceleration: follow up in Spectacol after ZxKit fixes its loader pattern; keep postponed.
- [ ] DivMMC/esxDOS on the other machines: **postponed**. It must remain an optional peripheral.
- [ ] Keep other unusual clones, peripherals and extensions after the main compatibility and usability work.

- [ ] Revisit the experimental FlatBuffers [zx_format](https://github.com/bog-dan-ro/zx_format/tree/master)
      container after the established formats and higher-priority compatibility work; no new dependency yet.
