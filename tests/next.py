#!/usr/bin/env python3
"""Exercise Next input, machine selection and SD copy through the compiled UI.

Copyright (c) 2015-2026, BogDan Vatra. GPL-3.0-or-later.
Uses a generated FAT image and diagnostic ROMs; no downloaded distro is required.
"""
import argparse
import os
from pathlib import Path
import struct
import subprocess
import tempfile


def image(path, cancel_matrix=False):
    raw = bytearray(2048 * 512)
    raw[:3] = b"\xeb\x3c\x90"
    struct.pack_into("<HBHBHHBHHH", raw, 11, 512, 1, 1, 1, 16, 2048, 0xF8, 7, 32, 64)
    raw[510:512] = b"\x55\xaa"
    fat, root, data = 512, 8 * 512, 9 * 512

    def chain(cluster, value):
        offset = fat + cluster * 3 // 2
        old = int.from_bytes(raw[offset:offset + 2], "little")
        value = ((old & 15) | (value << 4)) if cluster & 1 else ((old & 0xF000) | value)
        raw[offset:offset + 2] = value.to_bytes(2, "little")

    def entry(offset, name, cluster, size=0, directory=False):
        raw[offset:offset + 11] = name.encode("ascii")
        raw[offset + 11] = 0x10 if directory else 0x20
        struct.pack_into("<HI", raw, offset + 26, cluster, size)

    chain(0, 0xFF8)
    for cluster in (1, 2, 3):
        chain(cluster, 0xFFF)
    entry(root, "MACHINES   ", 2, directory=True)
    entry(data, "NEXT       ", 3, directory=True)
    # DI, read NextREG B0/B1 and the actual keyboard rows into RAM at 9000.
    program = bytearray(b"\xf3")
    if cancel_matrix:
        program += b"\x01\x3b\x24\x3e\x68\xed\x79\x04\x3e\x10\xed\x79"
    loop = len(program)
    for reg in (0xB0, 0xB1):
        program += b"\x01\x3b\x24\x3e" + bytes([reg]) + b"\xed\x79\x04\xed\x78\x32" + struct.pack("<H", 0x9000 + reg - 0xB0)
    for row in range(8):
        program += b"\x01" + struct.pack("<H", ((0xFF ^ (1 << row)) << 8) | 0xFE)
        program += b"\xed\x78\xe6\x1f\x32" + struct.pack("<H", 0x9002 + row)
    program += b"\xc3" + struct.pack("<H", loop)
    rom = bytearray(65536)
    for bank in range(4):
        rom[bank * 16384:bank * 16384 + len(program)] = program
    cluster = 4
    for slot, (name, content) in enumerate((("ENNEXTZXROM", rom), ("ENNXTMMCROM", rom[:8192]),
                                             ("ENALTZX ROM", bytes(32768)), ("ENNEXTMFROM", bytes(8192)))):
        count = (len(content) + 511) // 512
        entry(data + 512 + slot * 32, name, cluster, len(content))
        for number in range(count):
            chain(cluster + number, cluster + number + 1 if number + 1 < count else 0xFFF)
            start = data + (cluster + number - 2) * 512
            raw[start:start + 512] = content[number * 512:(number + 1) * 512]
        cluster += count
    path.write_bytes(raw)


def main():
    repo = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=repo / "build/bin/spectacol")
    parser.add_argument("--output", type=Path, default=repo / "build/next")
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    cases = [
        ("128k-without-divmmc", "128k", "", False, None),
        ("next-without-divmmc", "next", "", False, bytes([0, 0] + [31] * 8)),
        ("next-left", "next", "down:Left", False, bytes([2, 0, 30, 31, 31, 15, 31, 31, 31, 31])),
        ("next-cancel-matrix", "next", "down:Left", True, bytes([2, 0] + [31] * 8)),
        ("next-keyboard", "next", "F7,point:1:down:950:530", False, bytes([8, 0, 30, 31, 31, 31, 23, 31, 31, 31])),
        ("next-uart-page", "next", "click:320:24,Down,Down,Down,Down,Down,Return,wait,Down,Down,Down,Down,Return", False, None),
        ("sd-copy", "next", "click:140:24,Down,Down,Down,Return,wait,Insert,wait,Right,Right,Right,Return", False, None),
    ]
    with tempfile.TemporaryDirectory(prefix="session-", dir=output) as temporary:
        session = Path(temporary)
        for name, machine, keys, cancelled, expected in cases:
            folder = session / name
            config = folder / "config/Licentia"
            config.mkdir(parents=True)
            media = output / (name + ".img")
            image(media, cancelled)
            data = folder / "data"
            data.mkdir()
            (data / "payload.txt").write_text("Next SD copy\n")
            (config / "Spectacol.conf").write_text(
                "[General]\nfirst_start=false\nautoSaveOnExit=false\nrestrictToSpectacol=true\ndataPath=" + str(data)
                + "\n[Next]\nsdCardImage=" + str(media)
                + "\n[Sound]\nsoundEnabled=false\n[Peripherals]\ndivMmc=false\ninterface2=false\n")
            env = os.environ | {"QT_QPA_PLATFORM": "offscreen", "XDG_CONFIG_HOME": str(config.parent),
                                "XDG_DATA_HOME": str(folder / "xdg-data"), "QT_SCALE_FACTOR": "1"}
            target = output / (name + ".sna")
            command = [str(args.binary.resolve()), "--machine", machine, "--window-size", "1280x800",
                       "--quit-after", str(max(3, 2 + len(keys.split(",")) * 0.3)), "--screenshot", str(target),
                       "--ui-screenshot", str(output / (name + ".png"))]
            if keys:
                command += ["--ui-keys", keys]
            with (output / (name + ".log")).open("w") as log:
                subprocess.run(command, env=env, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=30)
            log = (output / (name + ".log")).read_text()
            assert not any(x in log for x in ("Can't select", "not attached", "ReferenceError", "TypeError", "is not a type", "Cannot assign")), (name, log)
            if expected is not None:
                raw = target.read_bytes()
                actual = raw[27 + 0x5000:27 + 0x500A]
                assert actual == expected, (name, actual.hex(), expected.hex())
            if name == "sd-copy":
                assert b"PAYLOAD TXT" in media.read_bytes()[8 * 512:9 * 512], "The gamepad picker did not copy the host file"
            print(name + ": passed")


if __name__ == "__main__":
    main()
