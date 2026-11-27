#!/usr/bin/env python3
"""Check physical and on-screen keyboard input through window events.

Copyright (c) 2015-2026, BogDan Vatra. GPL-3.0-or-later.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile
from concurrent.futures import ThreadPoolExecutor

from controller import make_diagnostic


def main():
    repo = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=repo / "build/bin/spectacol")
    parser.add_argument("--output", type=Path, default=repo / "build/keyboard")
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    source = output / "ports.sna"
    make_diagnostic(source)
    # Fixed 1280x800 window: A/S at (70/195,560), CS/SS at (70/1070,690).
    # Values encode row << 4 | bit in the real Spectrum keyboard matrix.
    cases = [
        ("overlap", "F7,point:1:down:70:560,point:2:down:195:560,point:1:up:70:560", [0x11]),
        ("same-key", "F7,point:1:down:70:560,point:2:down:80:570,point:1:up:70:560", [0x10]),
        ("slide-off", "F7,point:1:down:70:560,point:1:move:600:100", [0x10]),
        ("cancel", "F7,point:1:down:70:560,point:2:down:195:560,point:cancel", []),
        ("hide", "F7,point:1:down:70:560,point:2:down:195:560,Escape", []),
        ("pause", "F7,point:1:down:70:560,F5,point:1:up:70:560,F5", []),
        ("shared-navigation", "F7,down:Return,point:1:down:70:560,up:Return", [0x10]),
        ("navigation-starts-on-g", "F7,down:Return", [0x14]),
        ("navigation-selects", "F7,down:Return,Right,up:Return,down:Return", [0x64]),
        ("navigation-moves", "F7,down:Return,Right,up:Return,point:1:down:70:560", [0x10]),
        ("cancel-keeps-navigation", "F7,down:Return,point:1:down:195:560,point:cancel", [0x14]),
        ("sticky-held-modifier", "F7,point:1:down:70:690,point:2:down:70:560,point:2:up:70:560", [0x00]),
        ("sticky-overlap", "F7,point:1:down:70:690,point:1:up:70:690,point:1:down:70:560,point:2:down:195:560,point:1:up:70:560", [0x00, 0x11]),
        ("sticky-release", "F7,point:1:down:70:690,point:1:up:70:690,point:1:down:70:560,point:2:down:195:560,point:1:up:70:560,point:2:up:195:560", []),
        ("cancel-modifier", "F7,point:1:down:1070:690,point:cancel", []),
        ("gamepad-caps", "F7,nav:ButtonL1:1,down:Return", [0x00, 0x14]),
        ("gamepad-symbol", "F7,nav:ButtonR1:1,down:Return", [0x71, 0x14]),
        ("gamepad-modifiers-release", "F7,nav:ButtonL1:1,nav:ButtonR1:1,down:Return,up:Return,nav:ButtonL1:0,nav:ButtonR1:0", []),
        ("gamepad-left-enter", "F7,nav:ButtonA,nav:ButtonL2:1", [0x60]),
        ("gamepad-right-enter", "F7,nav:ButtonA,nav:ButtonR2:1", [0x60]),
        ("gamepad-shared-enter", "F7,nav:ButtonL2:1,nav:ButtonR2:1,nav:ButtonL2:0", [0x60]),
        ("gamepad-enter-release", "F7,nav:ButtonL2:1,nav:ButtonR2:1,nav:ButtonL2:0,nav:ButtonR2:0", []),
        ("gamepad-enter-deadzone", "F7,nav:ButtonL2:0.2,nav:ButtonR2:0.4", []),
        ("gamepad-hide", "F7,nav:ButtonL1:1,nav:ButtonL2:1,Escape", []),
        ("gamepad-disconnect", "nav:connect:4242,F7,nav:ButtonL1:1,nav:ButtonL2:1,nav:disconnect:4242", []),
        ("gamepad-pause", "F7,nav:ButtonR1:1,F5,F5", []),
        ("left-stick", "F7,nav:AxisLeftX:1,nav:AxisLeftX:0,down:Return", [0x64]),
        ("right-stick", "F7,nav:AxisRightX:1,nav:AxisRightX:0,down:Return", [0x64]),
        ("stick-deadzone", "F7,nav:AxisLeftX:0.3,nav:AxisRightY:-0.3,down:Return", [0x14]),
        ("quote-chord", "down:Shift:50,down:QuoteDbl:48", [0x50, 0x71]),
        ("quote-shift-released-first", "down:Shift:50,down:QuoteDbl:48,up:Shift:50,up:Apostrophe:48", []),
        ("colon-shift-released-first", "down:Shift:50,down:Colon:47,up:Shift:50,up:Semicolon:47", []),
        ("colon-shift-still-held", "down:Shift:50,down:Colon:47,up:Colon:47", [0x00]),
        ("arrows-left-down", "down:Left,down:Down", [0x00, 0x34, 0x44]),
        ("arrows-up-right", "down:Up,down:Right", [0x00, 0x43, 0x42]),
    ]
    with tempfile.TemporaryDirectory(prefix="session-", dir=output) as temporary:
        session = Path(temporary)

        def run(case):
            name, keys, pressed = case
            config = session / name / "config/Licentia"
            config.mkdir(parents=True)
            (config / "Spectacol.conf").write_text(
                "[General]\nfirst_start=false\nautoSaveOnExit=false\ndataPath="
                + str(session / name / "data") + "\n[Peripherals]\njoystickPrompt=false\n")
            env = os.environ | {"QT_QPA_PLATFORM": "offscreen", "XDG_CONFIG_HOME": str(config.parent),
                                "XDG_DATA_HOME": str(session / name / "data"), "QT_SCALE_FACTOR": "1"}
            target = output / (name + ".sna")
            command = [str(args.binary.resolve()), "--machine", "48k", "--window-size", "1280x800",
                       "--quit-after", str(2 + len(keys.split(",")) * 0.3), "--ui-keys", keys,
                       "--screenshot", str(target), "--ui-screenshot", str(output / (name + ".png")), str(source)]
            with (output / (name + ".log")).open("w") as log:
                subprocess.run(command, env=env, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=20)
            log = (output / (name + ".log")).read_text()
            assert not any(x in log for x in ("ReferenceError", "TypeError", "Unknown key", "Unknown touch event", "Cannot assign")), log
            raw = target.read_bytes()
            assert len(raw) == 49179, name
            actual = raw[27 + 0x5000:27 + 0x5008]
            expected = bytearray([0x1F] * 8)
            for key in pressed:
                expected[key >> 4] &= ~(1 << (key & 15))
            assert actual == expected, (name, actual.hex(), expected.hex())
            return name

        with ThreadPoolExecutor(max_workers=3) as pool:
            for name in pool.map(run, cases):
                print(name + ": keyboard ports match")

        config = session / "basic/config/Licentia"
        config.mkdir(parents=True)
        data = session / "basic/data"
        (config / "Spectacol.conf").write_text(
            "[General]\nfirst_start=false\nautoSaveOnExit=false\n"
            f"dataPath={data}\n[Peripherals]\njoystickPrompt=false\n")
        env = os.environ | {"QT_QPA_PLATFORM": "offscreen", "XDG_CONFIG_HOME": str(config.parent),
                            "XDG_DATA_HOME": str(data)}
        # The real ROM interprets the Extended Mode sequence and its shifted letter chords.
        punctuation = ["Minus", "Equal", "Plus", "BraceLeft", "BracketLeft", "BracketRight",
                       "BraceRight", "Semicolon", "Colon", "Apostrophe", "QuoteDbl", "Comma",
                       "Less", "Period", "Greater", "Slash", "Question", "BracketLeft", "BracketLeft"]
        keys = ["wait"] * 5  # Let the ROM finish its cold-start RAM check.
        for name in punctuation:
            keys.extend(("down:" + name, "up:" + name))
        target = output / "basic-punctuation.sna"
        command = [str(args.binary.resolve()), "--machine", "48k", "--ui-keys", ",".join(keys),
                   "--quit-after", str(2 + len(keys) * 0.3), "--screenshot", str(target)]
        with (output / "basic-punctuation.log").open("w") as log:
            subprocess.run(command, env=env, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=20)
        raw = target.read_bytes()
        edit_line = int.from_bytes(raw[27 + 23641 - 0x4000:27 + 23643 - 0x4000], "little")
        start = 27 + edit_line - 0x4000
        expected = b'-=+{[]};:\'",<.>/?[[\r'
        actual = raw[start:start + len(expected)]
        assert actual == expected, ("BASIC edit line", actual, expected)
    print("Keyboard integration checks passed")


if __name__ == "__main__":
    main()
