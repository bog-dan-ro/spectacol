# Spectacol

Spectacol (in Romanian it means **show**) is a [free](http://www.gnu.org/licenses/gpl-3.0.en.html) [ZX Spectrum](https://en.wikipedia.org/wiki/ZX_Spectrum) emulator built on [ZxKit](https://github.com/bog-dan-ro/ZxKit), a modern C++23 ZX Spectrum emulation library, with its own UI made in QML.

Why QML? Well, first and foremost because I love Qt and also because I needed a framework which allows me to quickly target [lots of platforms](https://doc.qt.io/qt-6/supported-platforms.html) painlessly.

See [BUILD.md](BUILD.md) for desktop and Android builds, including
[Flatpak and AppImage packages](BUILD.md#linux-packages) for Linux.

**Spectacol**, currently, enables the following features:
 - Loads all ZX Spectrum files: tapes, snapshots, disks and RZX recordings
 - Accurate 16K, 48K, 128K, +2, +2A, +3, Pentagon, Scorpion and Timex emulation, the Romanian HC-2000 (with CP/M) and CoBra, and the **ZX Spectrum Next** (NextZXOS boots from its SD card image, which Spectacol downloads for you)
 - Quick load & save snapshots. It is quite useful when you play games
 - Easily select the Joystick type
 - Poke finder, disassambler & **inline assambler**. Allows the user to crack the games and gain infinite lives quickly.
 - Last but not least, it is very efficient: it needs very little CPU to emulate your games

On top of that, Spectacol adds:
 - The UI is design with gamepad controls in mind. The user will be able to play all the games that support Joysticks using the gamepad
 - **Online search** allows the user to access **thousands** of ZX Spectrum apps & games! The search results are downloaded locally and they can be used to start the games offline
 - On screen numeric cursor (available only when a gamepad is present), allows the user to quickly start & control the games
 - On Screen full/simplified 48K Keyboard needed when the numeric cursor is not enough
 - On screen virtual Joystick (available only when no gamepad is present), allows the user to play the games on touch screen

## Keyboard

F1 to F4 belong to the emulated machine (the CoBra's function keys); Spectacol's own keys come after them:

| Key | Action |
| --- | --- |
| F5 (or Menu) | open or close the menu, close a page |
| F6 | quick open a file |
| F7 | the on-screen keyboard |
| F10 | quit |
| F11 | full screen |

On a CoBra 1, Ctrl is CTRL, Esc is ESC, Scroll Lock is NO SCROLL and the keypad's Enter is LINE FEED.
Elsewhere Ctrl, Alt and Meta are Symbol Shift and Esc is BREAK.

## Controller mapping

Choose **Options → (Virtual) Gamepad → Controller mapping**. Directions can drive the selected joystick,
use **QAOP + Space**, or use five custom Spectrum keys. The mapping applies to both sticks, the D-pad and the
touch joystick. Buttons can use the mapped fire key, another key or an action such as pause or quick save.
Select and both stick-click buttons are available; Start keeps opening the menu. Settings are remembered.

Use Up/Down to move between settings and Left/Right to change the direction mode. A/Enter opens a key/action
picker; A confirms and B/Escape returns to the same row. Shared keys and fire buttons remain held until all
their input sources release them. Menus, pause, reset and mapping changes clear held input; disconnecting a
gamepad or hiding the touch joystick releases that input source.

## Online search

Choose **Spectacol → Search online**, then **Internet Archive** or **ZXInfo**. Enter a title, a single letter
to browse by initial, or leave the field empty to browse the catalogue. The source and search text are remembered.

Browse the grid with the D-pad or arrow keys. Press **A/Enter** (or click a card) to open its details dialog.
Choose a media file with Up/Down, then press **A/Enter** to download and play it. The file is kept under
`Downloaded/Online` and copied to Favourites for offline use. **B/Escape** closes the dialog and restores the
same card and scroll position in the grid.
Browsing results does not download game files. Unavailable titles keep a link to their source page; failed
searches and downloads display an error and can be retried. **X/F3** focuses the search field and **Y/F4** the
source selector. Inside the details dialog, **X/F3** opens the source page. ZIP tapes use the same auto-load
setting as loose tapes.

## Saving programs and snapshots

To save a BASIC program to a tape:

1. Choose **Media → Tape → New blank tape**, then **Create blank tape**. This replaces the inserted tape and
   keeps your program in memory. Export the old tape first if you want to keep its blocks.
2. In BASIC, enter `SAVE "name"`, press Enter, then press Enter again when the ROM asks you to start the tape.
3. Choose **Media → Tape → Save**, check the destination and export the `.tap` file. It includes the original
   Spectrum headers and data, ready to load in another emulator or transfer to a real Spectrum.

Input recording (`.rzx`) records a play session. Stop input recording/playback before using BASIC `SAVE`;
use tape export to share a program. Tape export currently supports `.tap`.

**Media → Save snapshot as** saves the current state as `.szx`, even if you started with a blank machine.
The file and folder can be edited. Generated tape, snapshot and RZX names use portable timestamps, and
filename punctuation such as colons and slashes is replaced with underscores.
Quick load finds the newest snapshot across Save As files and per-game quick-save folders.

Save failures keep the editor open and leave existing files intact. A failed RZX save keeps the recording in
memory; **Spectacol → Retry recording save** retries it after the destination is available. Retry before quitting.

## ZX Spectrum Next

**Media → SD card** browses the card, copies host files through the gamepad browser,
and selects another image. Updating the distro preserves the previous default
image as a backup; failed or cancelled updates keep the current image.

**Options → Peripherals → Next UART devices** configures the ESP AT module and
Pi TCP bridge. Choose local scripts or HTTPS URLs, edit and reload without
rebuilding. The JavaScript engines run on the emulation thread. See [UART.md](UART.md)
for the module API and supported commands.

The physical and on-screen keyboards provide the Next's extended keys. Arrows,
punctuation, Tab (Extend), Caps Lock/F2, Home (Graph), F1/Insert (Edit), F3/F4
(True/Inverse Video), Escape (Break) and Backspace/Delete reach NextREG B0/B1.
The menu provides separate Multiface and drive NMI buttons, subject to the
guest's device/button enables. The RTC follows host local time and retains its
battery RAM in settings. Optional DivMMC support on classic machines is postponed.

## CoBra

Pick CoBra 1 (64KB or 80KB) or CoBra 2 (TS) in the machine picker. CoBra 1 starts at its
boot manager: Ctrl+0 launches ROM CP/M; Enter selects the standard boot. For disk CP/M,
select the 80KB machine, insert a raw 720K `.img` system disk into drive A through Media →
Disks, then press Enter at the boot manager and D at the CoBra logo. Four drives are available. Teledisk `.td0` images (normal or advanced compression) can also be opened or inserted.
The 80-column CP/M display includes the extra cells in the left and right borders.

CoBra 2 uses the original Turbo Spectrum EEPROM: a key such as Enter leaves its logo and
starts Spectrum BASIC. The machines' official ROMs are bundled with their notices.

On CoBra 1, the on-screen keyboard adds F1–F4, ESC, LINE FEED, CTRL and NO SCROLL.
CTRL is separate from SYMBOL SHIFT; the sticky modifier option supports chords on a
single touch screen or gamepad. Each finger keeps its original key while sliding; overlapping keys and
sticky chords remain held until their last owner releases. Cancelled touches release their own holds;
closing the keyboard, pausing or leaving the app clears all keyboard holds.

The original 64KB boot's SYS.COM disk loader is verified with diagnostic disks in both
FM and MFM formats. A complete historical 64KB CP/M system remains unverified because
no preserved SYS.COM system disk has been located.

Be aware that, Spectacol is in **BETA** stage, so, if you find bugs or you like to see new features, please add them to the [bugtracker](https://github.com/bog-dan-ro/spectacol/issues)

Download it from [google play](https://play.google.com/store/apps/details?id=eu.licentia.games.spectacol)

Or become a [beta tester](https://play.google.com/apps/testing/eu.licentia.games.spectacol)

# Build from source code
Check [build instruction](BUILD.md)
