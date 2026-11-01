#!/usr/bin/env python3
"""Check border cropping and persistence in the compiled Spectacol UI.

Copyright (c) 2015-2026, BogDan Vatra. GPL-3.0-or-later.
"""
import argparse
import os
from pathlib import Path
import struct
import subprocess
import tempfile


def png_size(path):
    header = path.read_bytes()[:24]
    if header[:8] != b"\x89PNG\r\n\x1a\n":
        raise AssertionError(f"Not a PNG: {path}")
    return struct.unpack(">II", header[16:24])


def main():
    repo = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=repo / "build/bin/spectacol")
    parser.add_argument("--output", type=Path, default=repo / "build/borders")
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    cases = {"default": ((32, 32, 32, 32), (320, 256)),
             "wide": ((48, 0, 48, 0), (352, 192)),
             "none": ((0, 0, 0, 0), (256, 192)),
             "full": ((48, 63, 48, 56), (352, 311)),
             "asymmetric": ((5, 7, 11, 13), (272, 212))}
    with tempfile.TemporaryDirectory(prefix="session-", dir=output) as temporary:
        session = Path(temporary)
        config = session / "config/Licentia"
        config.mkdir(parents=True)
        ini = config / "Spectacol.conf"
        env = os.environ | {"QT_QPA_PLATFORM": "offscreen",
                            "XDG_CONFIG_HOME": str(config.parent),
                            "XDG_DATA_HOME": str(session / "data")}

        def run(name, keys="", seconds=2):
            command = [str(args.binary.resolve()), "--machine", "48k", "--quit-after", str(seconds),
                       "--window-size", "1280x800", "--screenshot", str(output / f"{name}.png"),
                       "--ui-screenshot", str(output / f"{name}-ui.png")]
            if keys:
                command += ["--ui-keys", keys]
            with (output / f"{name}.log").open("w") as log:
                subprocess.run(command, env=env, stdout=log, stderr=subprocess.STDOUT,
                               check=True, timeout=30)
            log = (output / f"{name}.log").read_text()
            assert "ReferenceError" not in log and "TypeError" not in log, log

        for name, (margins, expected) in cases.items():
            ini.write_text("[General]\nfirst_start=false\nautoSaveOnExit=false\ndataPath="
                           + str(session / "data") + "/\n[Screen]\n"
                           + "".join(f"border{side}={value}\n" for side, value
                                     in zip(("Left", "Top", "Right", "Bottom"), margins)))
            run(name)
            assert png_size(output / f"{name}.png") == expected, name
            print(f"{name}: {expected[0]}x{expected[1]}")

        # The desktop Options menu opens the Borders sheet. While it pauses the
        # emulator, change all four margins using the compiled QML steppers.
        run("runtime", "click:320:24,Down,Down,Down,Return,Left,Down,Right,Down,Left,Down,Right", 6)
        assert png_size(output / "runtime.png") == (274, 210)
        settings = ini.read_text()
        for side, value in (("Top", 6), ("Left", 6), ("Bottom", 12), ("Right", 12)):
            assert f"border{side}={value}" in settings, settings
        run("restored")
        assert png_size(output / "restored.png") == (274, 210)
    print("Border presets, all four live controls, paused crop and persistence passed")


if __name__ == "__main__":
    main()
