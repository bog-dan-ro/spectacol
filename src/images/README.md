# Spectacol UI icons

Original Spectacol artwork, Copyright (c) 2015-2026, BogDan Vatra.
GPL-3.0-or-later, like the application. No stock icon pack or traced artwork.

The family uses a 24 × 24 view box, two-unit rounded strokes, flat surfaces,
and transparent margins. The controller help diagram uses the same geometry
on a wider grid. It has QML text labels so they remain localizable and accessible.
SVGs have no fonts, embedded bitmaps, gradients, filters or external dependencies.
The 256 px intrinsic size is the default raster size. The browser requests its
display size at the screen's pixel density; the image provider rasterizes only
placeholders at that size and preserves Spectrum previews at native resolution.

Colours follow `qml/Theme.qml`: `text` (#eceef2), `raised` (#20242c),
`accent` (#00d7d7), `danger` (#ff6464), `warning` (#e6d64a), `success` (#48d672),
`textOnAccent` (#001a1b) for symbols on coloured badges and hint pills, `textMuted`
(#a4aab6) for diagram leaders, and the four `pad*` roles. Keep these
values in sync if Theme's palette changes. Shape conveys state as well as colour.

| Assets | Meaning and consumers |
| --- | --- |
| `folder.svg`, `floppy.svg` | Folders and disk media in Quick Open's covers/grid, through `SpectrumScreenImageProvider`; the grid also reads the folder SVG directly. |
| `binary.svg` | Data without a screen preview in the tape inspector. |
| `cassette-wait.svg` | Path whose preview is not yet available. |
| `cassette-broken.svg` | File cannot be read (cross badge). |
| `cassette-unknown.svg` | File has no recognised preview (question badge). |
| `cassette-tape.svg`, `cassette-fast.svg` | Cassette and fast tape variants, retained as members of the resource family. |
| `keyboard-icon.svg`, `controller-icon.svg` | On-screen keyboard and controller toggles. |
| `menu.svg` | The touch rail's menu button and the gamepad Menu/Start hint. |
| `back.svg`, `close.svg`, `more.svg`, `up.svg` | Page, menu and file browser header buttons (tinted through `icon.color`). |
| `chevron-right.svg`, `minus.svg`, `plus.svg` | Sub menus, settings actions and the settings rows' steppers (`back.svg` steps back). |
| `check.svg` | The chosen option and machine. |
| `backspace.svg` | The on-screen text keyboard's delete key. |
| `dpad-left-right.svg`, `dpad-up-down.svg` | D-pad hints for gamepads and TV remotes. |
| `status-info.svg`, `status-warning.svg`, `status-error.svg` | The message toast's level. |

The UI draws symbols with these icons, never with font characters (☰ ‹ › ⋮ × ✓ ▶ ⌫ ◀▶):
Android's fonts lack many of them. Buttons take `icon.source` (and `icon.color`); other
places use the `SvgIcon` element.
| `play.svg`, `pause.svg`, `fastforward.svg` | Playback and held fast-forward controls. |
| `arrow-*.svg`, `draw-fire.svg` | Eight joystick directions and fire. |
| `gamepad.svg` | Help diagram; `GamepadDiagram.qml` supplies button labels and a spoken description. |

Game covers, Spectrum screen previews and online thumbnails are content, not
icons. Android launcher icons and the TV banner are application branding.
They are outside this UI icon family.

Check the compiled application at normal and Large text sizes, including Quick
Open's covers/grid/list, Help's controller tabs, the touch control rail and all
joystick directions. Render the SVGs at 16, 24, 32 and 64 px, and check Android
at its native display density. Existing navigation and touch regressions are in
`tests/files_features.py`, `tests/controls.py`, `tests/joysticks.py` and
`tests/android_ui.py`. Keep screenshots and reports under a build directory.
