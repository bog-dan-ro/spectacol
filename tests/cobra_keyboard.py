#!/usr/bin/env python3
"""Check the compiled CoBra on-screen keyboard through real window key events.

Copyright (c) 2015-2026, BogDan Vatra. GPL-3.0-or-later.
The ROM CP/M reaches its disk prompt/error without needing an external disk.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile


def main():
    repo = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=repo / "build/bin/spectacol")
    parser.add_argument("--output", type=Path, default=repo / "build/cobra-keyboard")
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    snapshot = output / "rom-cpm.sna"
    with tempfile.TemporaryDirectory(prefix="session-", dir=output) as temporary:
        session = Path(temporary)
        config = session / "config/Licentia"
        config.mkdir(parents=True)
        (config / "Spectacol.conf").write_text(
            "[General]\nfirst_start=false\nautoSaveOnExit=false\ndataPath="
            + str(session / "data") + "/\n")
        env = os.environ | {"QT_QPA_PLATFORM": "offscreen",
                            "XDG_CONFIG_HOME": str(config.parent),
                            "XDG_DATA_HOME": str(session / "data")}
        # A -> CTRL, select sticky CTRL, then move to 0 and hold it for several scans.
        keys = (["F7", "Down"] + ["Right"] * 10 + ["Return"] + ["Up"] * 3
                + ["Left", "down:Return", "wait", "wait", "wait", "up:Return",
                   "wait", "wait", "Escape"])
        with (output / "spectacol.log").open("w") as log:
            subprocess.run([str(args.binary.resolve()), "--machine", "cobra1_80k",
                            "--quit-after", "13", "--ui-keys", ",".join(keys),
                            "--window-size", "1000x700", "--screenshot", str(snapshot),
                            "--ui-screenshot", str(output / "keyboard-result.png")],
                           env=env, stdout=log, stderr=subprocess.STDOUT,
                           check=True, timeout=30)
    state = snapshot.read_bytes()
    # The 48K SNA header stores I at 0 and IM at 25. The boot manager is IM 0;
    # CoBra ROM CP/M installs its CTC vector table in page FD and enters IM 2.
    if len(state) < 27 or state[0] != 0xFD or state[25] != 2:
        raise SystemExit("On-screen CTRL+0 did not enter ROM CP/M; inspect " + str(output))
    print("CoBra on-screen sticky CTRL+0: ROM CP/M reached (I=FD, IM=2)")


if __name__ == "__main__":
    main()
