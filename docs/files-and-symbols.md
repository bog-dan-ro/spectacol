# Trainers, custom ROMs, local files and assembler symbols

## Trainers

Open a game, then **Machine → Poke memory**. Spectacol finds a `.pok` beside the
opened game, matching the complete basename without regard to case. If none is
present, it searches the selected **POKES folder** the same way (one folder, not a
recursive scan). Adjacent trainers take precedence. **Load .pok…** explicitly
imports another file; folder/file forms accept a typed path or a picker. Desktop pickers use Qt Quick
for controller navigation; Android retains its system document picker.
A trainer inside a ZIP is not automatically extracted as a sidecar.

Trainers start off. Select a row and press A/Enter or tap it to toggle the whole
group. A value of 256 in a POK asks for a byte using ordinary OS-keyboard fields.
All patches are validated before any write. CPU addresses resolve to the current
physical write mapping on first activation; explicit POK pages address 16 KiB RAM
pages. Turning a trainer off restores the bytes captured before activation, even
if the emulated program has since paged RAM. Overlapping active trainers are
rejected so one cannot undo another. Reloading a list requires disabling its active
entries first. Clearing a list keeps memory values, as before.

Custom pokes and imported trainers share the persistent page model: closing and
reopening the page keeps them. A new game, reset or machine switch clears this
session state. The POKES search folder persists across app restarts; trainer
activation does not. POK imports are limited to 1 MiB, and invalid banks/addresses/
values fail without changing the previous list.

## Custom ROMs

Open **Machine → Choose machine**, focus a machine, then **ROMs** (X/F2). Choose
an individual named ROM slot. Slot names and exact byte counts come from ZxKit's
machine implementations, including 8 KiB Timex expansion ROMs and the Next's
64/8/8/32 KiB slots. This interface accepts separate slot files, not concatenated
multi-slot images.

Valid files are copied into the app's private data directory. Preferences and
cached bytes are scoped to the machine ID, so 16K and 48K can use different
versions of `48.rom`. Removing the original source file does not lose an override.
**Restore default ROMs for this machine** leaves other machines' choices intact.
Changes apply on the next hard reset or machine switch; **Hard reset the running
machine** restarts the current game. A soft reset preserves ROM bytes.

On the Next, custom slots take precedence over `/machines/next` on its SD image;
restoring defaults uses that firmware again. The SD image/NextZXOS is still needed
for normal app boot. Saved overrides with missing or invalid private files fall
back to defaults and expose an error on this page. ROM bytes are preloaded outside
the running emulator lock; the resolver never reads host files or GUI settings.

## Local browsing

Quick Open and the tape/disk/host file pickers offer **Covers**, **List** and **Grid**.
F2/Start changes the view; F3/Y focuses the standard filename search field.
Search is local to the current directory and matches filenames without regard to
case. It filters the cached listing without reading media again on each keystroke.
Enter/Done returns focus to the files. The D-pad or left stick navigates files. A/Enter or a tap opens an entry; B/Back closes
the picker. Up and Backspace go to the parent folder; Home/L2 returns to Spectacol's
folder. X/Delete removes with confirmation; L1/C copies and R1/P pastes.

The view preference and each picker's folder survive restart. Selection is saved
by pathname per folder and media class, so sorting or adding files does not turn a
remembered index into a different file. Typing a search does not overwrite that saved
selection; explicitly opening a filtered entry does. List mode uses text rows; compact modes do not render the cover-flow
reflection. Folder restrictions also check path boundaries and resolved symlinks.

## Assembler labels and source mappings

In **Machine → Debugger**, choose **Labels / source mappings**, then **Import…**.
Supported formats are sjasmplus SLD v1 (`--sld=project.sld.txt`) and text symbol
assignments such as `MainLoop: EQU 0x8010`, `Loop = $8020` or `Start EQU 32768`.
Hex values may also use an `h` suffix. Arbitrary vendor `.map` layouts are not
supported; the file extension does not select or guarantee a format.

SLD imports preserve memory page IDs, device page size, module/main/local names
and instruction source file/line mappings. The documented eight fields, column
ranges, UTF-8 BOM and CRLF work; the old seven-field shorthand is accepted too.
Assembler EQU constants are excluded from SLD address labels. Imported source
paths are displayed as metadata; source files are not opened or executed.

Search the label list and activate one to jump to disassembly. The disassembly's
address field also accepts an exact label name or a hex address. Current labels
and source `file:line` annotations appear beneath instructions. A banked label can
only resolve when its source RAM page is currently mapped; Spectacol does not
silently page memory. Multiple active addresses with the same label are ambiguous.
SLD supports one memory model per import; 48K, 128K and 8 KiB Next paging are covered.

Imports replace the prior symbol set only on success and are limited to 16 MiB.
New media, reset or a machine switch clears symbols to prevent stale annotations.
Symbols are session metadata and are not saved into emulator snapshots.

## Verification

`test_files_features` exercises grouped/banked trainer writes and undo, malformed
input, sidecar/folder priority, ROM size checks and restart persistence, machine
scope, Next slots, actual SLD v1, label filtering, MMU matching and filename-based
selection persistence. It runs as part of Spectacol's full CTest suite.

Compiled UI regressions (authored media only):

```sh
python3 tests/files_features.py --binary build-production/bin/spectacol --output build/features/ui
python3 tests/poke_memory.py --binary build-production/bin/spectacol --output build/features/poke-memory
python3 tests/debugger_gamepad.py --binary build-production/bin/spectacol --output build/features/debugger-gamepad
```

The scripts preserve commands, logs, screenshots and diagnostic snapshots under
the output directory. Qt's offscreen backend checks software rendering and simulated
controller/touch/OS input-method events; physical Android file grants, virtual
keyboards and controllers still need device verification.
