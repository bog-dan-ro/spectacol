# Debugger

Open **Machine → Debugger**. The page pauses emulation and blocks gameplay input.
**Continue** runs with the page open; **Pause** stops it again. Views remain frozen
and edits are disabled while running. **Close** resumes gameplay. Inspectors are
created on first use and refreshed on a stop or an explicit refresh, without polling.

- **Step (F11 / gamepad L1):** execute one instruction.
- **Next (F10 / R2):** step over CALL/RST and repeated block instructions.
- **Out (F12 or Shift+F11 / L2):** execute through the current routine's return,
  following nested calls and interrupts. It stops in the caller after the return.
- **Run to cursor (F8 or Ctrl+F10):** stop before the selected disassembly row,
  in its current physical memory bank. Other breakpoints can stop it earlier.
- **Continue / Pause (F5 / R1):** resume execution or cancel a pending run command.

In disassembly, use arrows to select an instruction, **X** to toggle an execution
breakpoint (also by double-click or long-press), or **Enter** to assemble
an instruction in place. The address field
accepts hexadecimal addresses (plain, `$` or `0x` prefixes) and starts decoding
exactly at the requested byte. CPU registers have their own view.

The memory view shows a 256-byte window in CPU address space or any physical
RAM/ROM bank. Arrows move by byte/row, Page Up/Down by 256 bytes; Enter opens the
byte editor. Addresses and bytes are hexadecimal, optionally prefixed with `$` or
`0x`. ROM writes require the editor's explicit **Allow editing ROM** checkbox.
Watch read/write adds a breakpoint for the selected logical address or physical
bank byte; physical watches follow paging and aliases.

The breakpoint list supports execute, memory read/write, port read/write, Next
register write, raster line and interrupt events, with masks, enable/disable,
one-shot lifetime and hit counts. Addresses/masks are hexadecimal; raster lines
are decimal; interrupt event **0** is accepted IRQ and **1** is accepted NMI.
Watchpoints stop after the operation's instruction; raster traps stop before the
line's instructions. Next register writes from ports, Z80N opcodes and Copper use
the same watch path. Step completion and watchpoint reasons appear above the view.

Next registers show raw register latches, without invoking I/O read handlers. MMU
shows each effective 1 KiB mapping, including separate read/write backing pages,
ROM/overlay mappings and contention. Memory inspection and editing also bypass
emulated I/O and watchpoints. These operations are debugger interventions and do
not become recorded RZX input. Stop an active RZX session before single-stepping;
normal run-to/step-out use the engine's regular execution path.

Step-out follows CALL/RST/RET, conditional returns and RETN/RETI, including nested
interrupt handlers and ignored DD/FD prefixes. Manually constructed call frames,
stack pivots and self-modified instruction fetches can defeat this unwinder; use
an execution breakpoint or Pause when needed. Symbol/SLD import remains deferred.

## Controllers and virtual keyboards

The debugger accepts both the D-pad and left stick, including repeat while held.
**A** activates the focused control or opens the OS keyboard for an editable field.
**B** and the gamepad **Back** button close the current dialog or page, restoring
focus to its opener. **X** opens the view dropdown; **Y** focuses the address field.
L1 steps, R1 continues/pauses, L2 steps out and R2 steps over. Stick clicks page
backward/forward. Input and repeat stop when the page closes, the controller
changes or the application leaves the foreground.

Use ordinary text fields and the OS virtual keyboard for addresses, bytes and
breakpoint settings. Byte edits and breakpoint creation each use one form with
Save and Cancel. D-pad arrows navigate between controls; within an open dropdown
or memory/instruction list they navigate its entries. Left/right in text fields
move the cursor. Cancelling an edit never writes memory.

## Poke memory

**Machine → Poke memory** uses a single **Add poke** form (Y): choose CPU address
space or a writable physical bank, then enter a decimal address/offset and a
value from 0 to 255. Numeric input hints request the OS numeric keyboard. The bank
list shows actual names and sizes, excluding read-only ROM banks. Physical offsets
cover the entire bank, including offsets above 64 KiB; CPU addresses write only
through writable mappings. A toggles the selected poke and restores the original
byte when turning it off; X forgets the list while retaining current memory values.
B/Back cancels the form before closing the page.

Compiled controller checks: `tests/debugger_gamepad.py` and `tests/poke_memory.py`
exercise Qt Gamepad button/axis signals and inspect exported memory and rendered
status. They use authored media, Pillow and tesseract; reports default to `build/`.
