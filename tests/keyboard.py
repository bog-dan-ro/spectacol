#!/usr/bin/env python3
"""Check overlapping on-screen keyboard input through actual window touch events.

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
        ("navigation-selects", "F7,down:Return,Right,up:Return,down:Return", [0x11]),
        ("navigation-moves", "F7,down:Return,Right,up:Return,point:1:down:70:560", [0x10]),
        ("cancel-keeps-navigation", "F7,down:Return,point:1:down:195:560,point:cancel", [0x10]),
        ("sticky-held-modifier", "F7,point:1:down:70:690,point:2:down:70:560,point:2:up:70:560", [0x00]),
        ("sticky-overlap", "F7,point:1:down:70:690,point:1:up:70:690,point:1:down:70:560,point:2:down:195:560,point:1:up:70:560", [0x00, 0x11]),
        ("sticky-release", "F7,point:1:down:70:690,point:1:up:70:690,point:1:down:70:560,point:2:down:195:560,point:1:up:70:560,point:2:up:195:560", []),
        ("cancel-modifier", "F7,point:1:down:1070:690,point:cancel", []),
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
    print("On-screen keyboard integration checks passed")


if __name__ == "__main__":
    main()
