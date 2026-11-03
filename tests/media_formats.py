#!/usr/bin/env python3
"""Check that UI export choices survive editing a filename without its suffix.

Copyright (c) 2015-2026, BogDan Vatra. GPL-3.0-or-later.
"""
import argparse
import os
from pathlib import Path
import struct
import subprocess
import tempfile


def main():
    repo = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=repo / "build/bin/spectacol")
    parser.add_argument("--output", type=Path, default=repo / "build/media-formats")
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="formats-", dir=output) as temporary:
        session = Path(temporary)
        config = session / "config/Licentia"
        config.mkdir(parents=True)
        data = session / "data"
        (config / "Spectacol.conf").write_text(
            f"[General]\nfirst_start=false\nautoSaveOnExit=false\nautoLoad=false\ndataPath={data}\n[Peripherals]\njoystickPrompt=false\n")
        env = os.environ | {"QT_QPA_PLATFORM": "offscreen", "XDG_CONFIG_HOME": str(config.parent),
                            "XDG_DATA_HOME": str(data)}
        # Authored 48K SNA: JR $ at 8000, PC on the SNA stack at FFFC.
        snapshot = bytearray(49179)
        snapshot[25] = 1
        snapshot[26] = 3
        struct.pack_into("<H", snapshot, 23, 0xFFFC)
        snapshot[27 + 0x4000:27 + 0x4002] = b"\x18\xFE"
        struct.pack_into("<H", snapshot, 27 + 0xBFFC, 0x8000)
        source = session / "Dizzy.5.sna"
        source.write_bytes(snapshot)

        def run(name, keys, media=source):
            seconds = 2 + len(keys) * 0.3
            command = [str(args.binary.resolve()), "--machine", "48k", "--quit-after", str(seconds),
                       "--window-size", "1280x800", "--ui-keys", ",".join(keys),
                       "--ui-screenshot", str(output / f"{name}.png")]
            if media:
                command.append(str(media))
            result = subprocess.run(command, env=env, capture_output=True, text=True, check=True, timeout=seconds + 15)
            log = result.stdout + result.stderr
            (output / f"{name}.log").write_text(log)
            assert not any(s in log for s in ("ReferenceError", "TypeError", "Unknown key")), log

        save_as = ["click:140:24", "Down", "Return", "wait", "wait"]
        run("suggested-snapshot-name", save_as + ["Return"])
        assert (data / "Snapshots/Dizzy.5.szx").read_bytes().startswith(b"ZXST"), \
            "Snapshot suggestion lost the loaded filename or its embedded dots"
        run("suggested-name-z80", save_as + ["click:820:448", "click:800:500", "click:820:225", "Return"])
        assert (data / "Snapshots/Dizzy.5.z80").exists(), "Changing format lost the suggested basename"
        run("fallback-snapshot-name", save_as + ["Return"], None)
        assert (data / "Snapshots/Snapshot.szx").read_bytes().startswith(b"ZXST"), \
            "A session with no loaded file needs a generic snapshot suggestion"
        # Material's popup places its selected row over the ComboBox: z80 is one row below it.
        run("z80-export", save_as + ["click:820:448", "click:800:500", "click:820:225", "Ctrl+A",
                                    "E", "X", "P", "O", "R", "T", "Return"])
        z80 = data / "Snapshots/EXPORT.z80"
        assert z80.exists(), "Editing the basename lost the selected Z80 export format"
        raw = z80.read_bytes()
        assert raw[6:8] == b"\0\0" and struct.unpack_from("<H", raw, 30)[0] == 55
        assert struct.unpack_from("<H", raw, 32)[0] == 0x8000
        assert raw[87:90] == b"\xFF\xFF\x08"  # uncompressed 4000 page

        # A custom signal cannot be saved as TAP; the page must choose TZX automatically.
        tape = session / "custom.tzx"
        original = b"ZXTape!\x1A\x01\x14" + b"\x12" + struct.pack("<HH", 2168, 10)
        tape.write_bytes(original)
        export_tape = ["click:140:24", "Down", "Down", "Right", "Down", "Down", "Down", "Return", "wait", "wait"]
        run("tzx-export", export_tape + ["Ctrl+A", "C", "U", "S", "T", "O", "M", "Return"], tape)
        saved = data / "Saves/CUSTOM.tzx"
        assert saved.exists(), "Custom tape export did not select TZX"
        assert saved.read_bytes() == original, "Custom tape signal changed during UI export"
        print("UI Z80 export preserves the chosen format after basename edits")
        print("UI snapshot suggestions use the loaded filename, preserve dots and format, and fall back for a fresh session")
        print("UI custom tape export selects TZX and preserves the signal")


if __name__ == "__main__":
    main()
