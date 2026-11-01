# Spectacol

Spectacol (in Romanian it means **show**) is a [free](http://www.gnu.org/licenses/gpl-3.0.en.html) [ZX Spectrum](https://en.wikipedia.org/wiki/ZX_Spectrum) emulator built on [ZxKit](https://github.com/bog-dan-ro/ZxKit), a modern C++23 ZX Spectrum emulation library, with its own UI made in QML.

Why QML? Well, first and foremost because I love Qt and also because I needed a framework which allows me to quickly target [lots of platforms](https://doc.qt.io/qt-6/supported-platforms.html) painlessly.

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
single touch screen or gamepad. Keys are released when the keyboard closes or touch is canceled.

The original 64KB boot's SYS.COM disk loader is verified with diagnostic disks in both
FM and MFM formats. A complete historical 64KB CP/M system remains unverified because
no preserved SYS.COM system disk has been located.

Be aware that, Spectacol is in **BETA** stage, so, if you find bugs or you like to see new features, please add them to the [bugtracker](https://github.com/bog-dan-ro/spectacol/issues)

Download it from [google play](https://play.google.com/store/apps/details?id=eu.licentia.games.spectacol)

Or become a [beta tester](https://play.google.com/apps/testing/eu.licentia.games.spectacol)

# Build from source code
Check [build instruction](BUILD.md)
