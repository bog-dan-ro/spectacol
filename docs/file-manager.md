# File manager

**Media → File manager** opens a twin-panel file manager in the spirit of Midnight Commander.
Each panel shows a host folder, or a folder inside a tape, disk, microdrive or NEX image. Enter
(A) on an image opens it like a folder; `..` leaves it. Both panels remember their folders.
Narrow windows show one panel at a time.

All tape and disk work goes through ZxKit's `MediaArchive`, the library behind `zxmedia`, so the
file manager supports what it supports: TAP, TZX, PZX and CSW tapes; DSK/EDSK, TRD, SCL, MGT,
OPD, D40/D80, P3D, TD0, CQM and raw IMG disks with +3DOS, CP/M, HC-2000, TR-DOS, +D/DISCiPLE,
Opus, MDOS and DEVIL filesystems; MDR cartridges and NEX programs. Images up to 128 MiB are
edited in memory.

FAT card images (the Next's SD card, `.img`/`.raw`) are edited in place, a block at a time,
whatever their size. Images inside images open the same way: Enter on `nextzxos/cpm-a.p3d`
inside the SD card opens the Next's CP/M drive A, and an HC-2000 CP/M disk in the other panel
can copy its programs onto it. A nested image is edited in memory and written back into the
image holding it at each change. When the running Next's own card changes, the Next restarts
as the file manager closes, because NextZXOS caches the card.

## Keys and buttons

| Action | Keyboard | Gamepad |
| --- | --- | --- |
| Open a folder or image, view a file | Enter | A |
| Back: close a dialog or the viewer, else the parent folder, else close | Esc, Backspace | B |
| Actions menu (every action below) | F2, F9 | X |
| Mark or unmark, then move down | Insert, Space | Y |
| Switch panel | Tab, ← → | D-pad ← → |
| Page up / down | PageUp, PageDown | L1, R1 |
| Move a tape block up / down | [ ] | L2, R2 |
| View | F3 | menu |
| Edit a BASIC program or text file | F4 | menu |
| New tape or disk image | Shift+F4 | menu |
| Copy / move to the other panel | F5 / F6 | menu |
| Rename | Shift+F6 | menu |
| New folder | F7 | menu |
| Delete (folders with everything in them) | F8, Delete | menu |
| Attributes | Alt+Enter | menu |
| Load the file or image in the emulator | Shift+Enter | menu |
| Mark or unmark all | * | menu |
| Same folder in the other panel | Alt+O | menu |
| Refresh | Ctrl+R | menu |
| Close | F10 | Select |
| Save in the editor | F2, Ctrl+S | X |

Operations act on the marked entries, or on the one under the cursor. Copies ask before
overwriting. In dialogs Return (A) confirms. Touch screens use the sheet's ⋮ menu; a long
press marks.

## Inside images

- **Tapes** show three views of the same blocks. `files/` holds the logical files (a header and
  its data), `basic/` the programs as editable BASIC text, and `blocks/` every raw block,
  headerless and turbo blocks included, listed by number and contents (`#0 Program: NAME`). A
  copied block is a one-block TAP file (`000000.tap`) from a TAP tape, else a one-block TZX file;
  either kind can be copied into any tape, though a TAP tape takes standard blocks only.
  - Copying a host file into a tape adds a CODE file (a 6912-byte `.scr` loads at 16384);
    copying a `.bas` text file compiles it into a program.
  - Copying blocks between tapes appends them; `[` `]` (L2/R2) move a block. Jump, call and
    select targets keep pointing at the same blocks.
  - Attributes: the type, both header parameters and the pause after the data.
- **Disks** list their files with the filesystem's own attributes: +3DOS and HC types and
  parameters, CP/M read-only/system/archive flags and user areas (`user01/` …), TR-DOS letters
  (`B`, `C`, `D`, `#`) and parameters, FAT and MDOS flags. Copies between tapes and TR-DOS
  translate the types to letters and back, and add the TR-DOS extension when a name has none.
  CP/M rounds files without a +3DOS header up to 128-byte records.
- Disk layouts are detected conservatively. When the usual detection fails, the file manager
  tries every filesystem profile: it opens the image when exactly one reads it, and otherwise
  asks, listing only the profiles that read it, with their file counts.

Changes are written back to the image at once, atomically. An image already inserted in the
emulator keeps the copy it loaded: reinsert it to use the changes.

## Viewer and editor

The viewer (F3) shows the views that fit the file; ← → switch them. Long lines wrap. The
pages come from ZxKit's `MediaView` API, which `zxmedia view` uses too, so the command line
shows a file the same way.

- ↑ ↓ move a line cursor. Space or Return (A) starts a selection that moving extends (Shift+↑ ↓
  and Shift+click too, Ctrl+A selects all); Ctrl+C (X) copies it, or the cursor's line.
- In Hex and Disassembly, G (Y) goes to an address: decimal (23803) or hex ($5CFB, 0x5CFB,
  5CFBh). An address inside an instruction restarts the disassembly there.
- A screen copies as an image (Ctrl+C, X), and Ctrl+S (Y) saves it as a PNG in the other
  panel's folder (or the folder holding its image).

- **Unhacked**: a BASIC listing like the ROM's LIST (`BasicCodec::listingLines`), without
  colour controls, with keywords, numbers, strings and REM comments highlighted. Where a line's
  digits are a decoy for the number the program stores (`CLEAR 99999` storing 25285), the decoy
  is struck through and the value used follows it in red;
  Lines show in any order (repeated line numbers, numbers above 9999), with embedded colour codes
  as `{INK 6}` and machine code in a REM as `{machine code: N bytes at ADDRESS}`; each USR into
  the program gets a **USR ADDRESS** disassembly (the program taken to start at 23755);
- **Unfiltered**: every byte of the program, as the editor has it: `\{KEYWORD}` tokens, hidden
  numbers and colour codes as `\xHH`, and the variables after `!raw`;
- **Screen**: 6912-byte screens (tape blocks too: flag, 6912 bytes, checksum) and snapshot
  previews, at whole-pixel zoom. Larger data has an **As screen** view of its first 6912 bytes;
- **Hex** and **Disassembly**, from the file's load address;
- **Array** for number and character arrays; **Block** and **Raw TZX** for tape blocks;
- **Text** for text files: by name, or any untyped file that reads as text (CP/M's end-of-file
  mark, CR, LF or CR LF lines; WordStar's bit-7 characters are shown as plain ASCII).

The editor (F4) edits tape and +3DOS-like BASIC programs, and text files on the host or in
images, keeping their line endings and encoding (WordStar's bit-7 formatting is dropped). It uses ZxKit's
lossless BASIC text: keywords may be typed plainly, and `\{KEYWORD}` and `\xHH` escapes keep a
program's exact bytes (hidden numbers, colour codes, variables). TR-DOS programs, which keep
their autostart line after the program, are view-only.
