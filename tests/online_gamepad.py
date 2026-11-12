#!/usr/bin/env python3
"""Check catalogue source selection and its virtual keyboard through real gamepad signals.

Copyright (c) 2015-2026, BogDan Vatra. GPL-3.0-or-later.
Requires tesseract; catalogue responses are not needed by these checks.
"""

import argparse
import configparser
from concurrent.futures import ThreadPoolExecutor
import os
from pathlib import Path
import re
import subprocess
import tempfile


def pad(button):
    return "nav:Button" + button


def main():
    repo = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=repo / "build/bin/spectacol")
    parser.add_argument("--output", type=Path, default=repo / "build/online-gamepad")
    parser.add_argument("--case", action="append")
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    opening = ["nav:connect:4242", "F5"] + [pad("Down")] * 2 + [pad("A"), "wait", "wait", pad("X")]
    cases = [
        ("source-dpad", [pad("Y"), pad("Down"), pad("A")], 1, "", False),
        ("source-left-stick", [pad("Y"), "nav:AxisLeftY:1", "nav:AxisLeftY:0", pad("A")], 1, "", False),
        ("source-right-stick", [pad("Y"), "nav:AxisRightY:1", "nav:AxisRightY:0", pad("A")], 1, "", False),
        ("source-cancel", [pad("Y"), pad("Down"), pad("B")], 0, "", False),
        ("source-open-with-a", [pad("Y"), pad("B"), pad("A"), pad("Down"), pad("A")], 1, "", False),
        ("keyboard-starts-on-g", [pad("A")], 0, "g", True),
        ("keyboard-caps", [pad("L1:1"), pad("A"), pad("L1:0")], 0, "G", True),
        ("keyboard-symbols", [pad("R1:1"), pad("A"), pad("R1:0")], 0, "}", True),
        ("keyboard-modifier-release", [pad("L1:1"), pad("L1:0"), pad("A")], 0, "g", True),
        ("keyboard-left-stick", ["nav:AxisLeftX:1", "nav:AxisLeftX:0", pad("A")], 0, "h", True),
        ("keyboard-right-stick", ["nav:AxisRightX:1", "nav:AxisRightX:0", pad("A")], 0, "h", True),
        ("keyboard-reopens-on-g", [pad("A"), pad("B"), pad("A"), pad("A")], 0, "gg", True),
        ("keyboard-left-enter", [pad("L2")], 0, "", False),
        ("keyboard-right-enter", [pad("R2")], 0, "", False),
        ("results-return-to-query", [pad("L2"), pad("Up"), pad("A")], 0, "g", True),
    ]
    with tempfile.TemporaryDirectory(prefix="session-", dir=output) as temporary:
        def run(case):
            name, keys, source_index, query, keyboard = case
            home = Path(temporary) / name
            config = home / "config/Licentia"
            config.mkdir(parents=True)
            settings = config / "Spectacol.conf"
            settings.write_text(
                f"[General]\nfirst_start=false\nautoSaveOnExit=false\ndataPath={home / 'data'}\n"
                "[Peripherals]\njoystickPrompt=false\n[Ui]\nmode=2\n"
                "[OnlineSearch]\nsourceIndex=0\nsearchString=\n"
            )
            commands = opening + keys
            seconds = 2 + 0.3 * len(commands)
            picture = output / (name + ".png")
            result = subprocess.run(
                [str(args.binary.resolve()), "--machine", "48k", "--quit-after", str(seconds),
                 "--window-size", "1280x800", "--ui-keys", ",".join(commands),
                 "--ui-screenshot", str(picture)],
                env=os.environ | {"QT_QPA_PLATFORM": "offscreen", "QT_SCALE_FACTOR": "1",
                                  "XDG_CONFIG_HOME": str(config.parent),
                                  "XDG_DATA_HOME": str(home / "data"),
                                  "http_proxy": "http://127.0.0.1:1",
                                  "https_proxy": "http://127.0.0.1:1"},
                capture_output=True, text=True, timeout=seconds + 20,
            )
            (output / (name + ".log")).write_text(result.stdout + result.stderr)
            assert result.returncode == 0, (name, result.stderr)
            assert not any(error in result.stderr for error in (
                "ReferenceError", "TypeError", "Binding loop", "Cannot assign", "is not a type",
                "Required property", "No such method", "Unable to assign", "Unknown navigation",
            )), (name, result.stderr)
            saved = configparser.ConfigParser(interpolation=None)
            saved.read(settings)
            assert saved.getint("OnlineSearch", "sourceIndex") == source_index, (name, settings.read_text())
            assert saved.get("OnlineSearch", "searchString") == query, (name, settings.read_text())
            text = subprocess.check_output(
                ["tesseract", str(picture), "stdout", "--psm", "11"],
                text=True, stderr=subprocess.DEVNULL,
            )
            (output / (name + ".txt")).write_text(text)
            words = re.sub(r"[^a-z0-9]", "", text.lower())
            assert "searchonline" in words, (name, text)
            assert ("closeb" in words) == keyboard, (name, text)
            print(name + ": passed", flush=True)

        with ThreadPoolExecutor(max_workers=3) as pool:
            list(pool.map(run, [case for case in cases if not args.case or case[0] in args.case]))


if __name__ == "__main__":
    main()
