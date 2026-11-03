#!/usr/bin/env python3
"""Check controller routing against the emulated keyboard and Kempston ports.

Copyright (c) 2015-2026, BogDan Vatra. GPL-3.0-or-later.
"""
import argparse
import os
from pathlib import Path
import struct
import subprocess
import tempfile
from concurrent.futures import ThreadPoolExecutor


def make_diagnostic(source):
    # A tiny DI loop samples the machine's real I/O into RAM, rather than
    # relying on facade properties to establish that keys reach the computer.
    ram = bytearray(49152)
    program = bytearray(b"\xf3")
    for row in range(8):
        program += b"\x01" + struct.pack("<H", ((0xFF ^ (1 << row)) << 8) | 0xFE)
        program += b"\xed\x78\xe6\x1f\x32" + struct.pack("<H", 0x9000 + row)
    program += b"\xdb\x1f\xe6\x1f\x32\x10\x90\xc3\x01\x80"
    ram[0x4000:0x4000 + len(program)] = program
    ram[-4:-2] = struct.pack("<H", 0x8000)
    header = bytearray(27)
    struct.pack_into("<H", header, 23, 0xFFFC)
    header[25:27] = b"\x01\x07"
    source.write_bytes(header + ram)


def main():
    repo = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=repo / "build/bin/spectacol")
    parser.add_argument("--output", type=Path, default=repo / "build/controller")
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    source = output / "ports.sna"
    make_diagnostic(source)
    cases = [
        ("qaop-diagonal", 1, 2, "pad:AxisLeftX:-1,pad:AxisLeftY:-1,pad:ButtonA:down", [0x20, 0x51, 0x70], 0),
        ("two-sticks", 1, 2, "pad:AxisLeftX:-1,pad:AxisRightX:-1,pad:AxisLeftX:0", [0x51], 0),
        ("dpad-and-stick", 1, 2, "pad:ButtonUp:down,pad:AxisLeftY:-1,pad:AxisLeftY:0", [0x20], 0),
        ("shared-fire", 1, 2, "pad:ButtonA:down,pad:ButtonB:down,pad:ButtonA:up", [0x70], 0),
        ("keyboard-fire", 1, 2, "down:Space,pad:ButtonA:down,pad:ButtonA:up", [0x70], 0),
        ("custom-diagonal", 2, 2, "pad:AxisLeftX:-1,pad:AxisLeftY:-1,pad:ButtonA:down", [0x10, 0x21, 0x71], 0),
        ("custom-modifier", 2, 2, "down:Control,pad:ButtonA:down,pad:ButtonA:up", [0x71], 0),
        ("kempston", 0, 2, "pad:AxisLeftX:1,pad:AxisLeftY:1,pad:ButtonA:down", [], 0x15),
        ("cursor-key-owner", 0, 1, "down:5,pad:AxisLeftX:-1,pad:AxisLeftX:0", [0x34], 0),
        ("touch-and-stick", 1, 2, "pad:AxisLeftX:-1,touch:AxisLeftX:-1,touch:clear", [0x51], 0),
        ("touch-fire", 1, 2, "pad:ButtonA:down,touch:ButtonA:down,pad:ButtonA:up", [0x70], 0),
        ("touch-hide", 1, 2, "pad:ButtonA:down,touch:ButtonA:down,pad:ButtonA:up,touch:clear", [], 0),
        ("touch-cancel-shared-fire", 1, 2, "touch:ButtonA:down,touch:ButtonB:down,touch:ButtonA:cancel", [0x70], 0),
        ("touch-cancel-shortcut", 1, 2, "touch:ButtonX:down,touch:ButtonX:cancel,pad:AxisLeftX:-1", [0x51], 0),
        ("neutral", 1, 2, "pad:AxisLeftX:-1,pad:AxisLeftX:0", [], 0),
        ("menu-release", 1, 2, "pad:AxisLeftX:-1,F5,wait,F5", [], 0),
        ("menu-touch-release", 1, 2, "touch:AxisLeftX:-1,F5,touch:AxisLeftX:1,F5", [], 0),
    ]
    with tempfile.TemporaryDirectory(prefix="session-", dir=output) as temporary:
        session = Path(temporary)

        def run(case):
            name, mode, joystick, keys, pressed, expected_joystick = case
            config = session / name / "config/Licentia"
            config.mkdir(parents=True)
            (config / "Spectacol.conf").write_text(
                "[General]\nfirst_start=false\nautoSaveOnExit=false\njoystick=" + str(joystick)
                + "\ndataPath=" + str(session / name / "data") + "\n[Peripherals]\njoystickPrompt=false\n[Gamepad]\ndirectionMapping=" + str(mode)
                + "\ndirectionKeys=87, 83, 65, 68, 16777249\n")
            env = os.environ | {"QT_QPA_PLATFORM": "offscreen", "XDG_CONFIG_HOME": str(config.parent),
                                "XDG_DATA_HOME": str(session / name / "data")}
            target = output / (name + ".sna")
            seconds = 2 + len(keys.split(",")) * 0.3
            command = [str(args.binary.resolve()), "--machine", "48k", "--quit-after", str(seconds),
                       "--ui-keys", keys, "--screenshot", str(target), str(source)]
            with (output / (name + ".log")).open("w") as log:
                subprocess.run(command, env=env, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=20)
            log = (output / (name + ".log")).read_text()
            assert not any(x in log for x in ("ReferenceError", "TypeError", "Unknown controller", "Unknown key")), log
            raw = target.read_bytes()
            assert len(raw) == 49179, name
            actual = raw[27 + 0x5000:27 + 0x5008]
            expected = bytearray([0x1F] * 8)
            for key in pressed:
                expected[key >> 4] &= ~(1 << (key & 15))
            assert actual == expected, (name, actual.hex(), expected.hex())
            actual_joystick = raw[27 + 0x5010]
            assert actual_joystick == expected_joystick, (name, actual_joystick, expected_joystick)
            return name

        with ThreadPoolExecutor(max_workers=3) as pool:
            for name in pool.map(run, cases):
                print(name + ": keyboard and joystick ports match")
    print("Controller integration checks passed")


if __name__ == "__main__":
    main()
