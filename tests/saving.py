#!/usr/bin/env python3
"""Exercise BASIC tape export, snapshots and failed-write recovery in Spectacol.

Copyright (c) 2015-2026, BogDan Vatra. GPL-3.0-or-later.
"""
import argparse
import functools
import operator
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile
import threading
import zipfile


def basic_bytes(snapshot):
    raw = snapshot.read_bytes()
    assert len(raw) == 49179, snapshot

    def word(address):
        return struct.unpack_from("<H", raw, 27 + address - 0x4000)[0]

    start, end = word(0x5C53), word(0x5C4B)  # PROG and VARS, from the real BASIC ROM
    assert 0x4000 <= start <= end <= 0xFFFF, (start, end)
    return raw[27 + start - 0x4000:27 + end - 0x4000]


def tape_blocks(path):
    raw = path.read_bytes()
    blocks = []
    pos = 0
    while pos < len(raw):
        length = struct.unpack_from("<H", raw, pos)[0]
        pos += 2
        block = raw[pos:pos + length]
        assert len(block) == length and length >= 2
        assert functools.reduce(operator.xor, block) == 0, "Bad tape checksum"
        blocks.append(block)
        pos += length
    assert pos == len(raw)
    return blocks


def main():
    repo = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=repo / "build/bin/spectacol")
    parser.add_argument("--output", type=Path, default=repo / "build/saving")
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    media = ["click:140:24"]
    # Give each asynchronously loaded sheet time to finish opening before editing.
    export = media + ["Down", "Down", "Right", "Down", "Down", "Down", "Return", "wait", "wait"]
    snapshot_as = media + ["Down", "Return", "wait", "wait"]
    new_tape = media + ["Down", "Down", "Right", "Down", "Return", "wait", "wait", "Return", "wait", "wait"]
    start_record = media + ["Down", "Down", "Down", "Right", "Down", "Return"]
    stop_record = ["click:50:24", "Down", "Down", "Down", "Return"]
    with tempfile.TemporaryDirectory(prefix="session-", dir=output) as temporary:
        session = Path(temporary)
        config = session / "config/Licentia"
        config.mkdir(parents=True)
        data = session / "data"
        (config / "Spectacol.conf").write_text(
            "[General]\nfirst_start=false\nautoSaveOnExit=false\ndataPath=" + str(data) + "\n")
        env = os.environ | {"QT_QPA_PLATFORM": "offscreen",
                            "XDG_CONFIG_HOME": str(config.parent), "XDG_DATA_HOME": str(data)}

        def run(name, keys=(), source=None, snapshot=False, repair=None):
            seconds = max(3, 2 + len(keys) * 0.3)
            command = [str(args.binary.resolve()), "--machine", "48k", "--quit-after", str(seconds),
                       "--window-size", "1280x800", "--ui-screenshot", str(output / f"{name}-ui.png")]
            if keys:
                command += ["--ui-keys", ",".join(keys)]
            if snapshot:
                command += ["--screenshot", str(output / f"{name}.sna")]
            if source:
                command.append(str(source))
            stopped = threading.Event()
            repair_errors = []

            def repair_after_failure():
                try:
                    repair(output / f"{name}.log", stopped)
                except Exception as error:
                    repair_errors.append(str(error))

            worker = threading.Thread(target=repair_after_failure) if repair else None
            try:
                with (output / f"{name}.log").open("w") as log:
                    if worker:
                        worker.start()
                    subprocess.run(command, env=env, stdout=log, stderr=subprocess.STDOUT,
                                   check=True, timeout=seconds + 15)
            finally:
                stopped.set()
                if worker:
                    worker.join()
            assert not repair_errors, repair_errors
            log = (output / f"{name}.log").read_text()
            assert not any(error in log for error in ("ReferenceError", "TypeError", "Unknown key")), log

        # Load 10 PRINT 42 through the real ROM, then save from an initially
        # empty deck. The generated input tape is test data, not an external fixture.
        expected = bytes.fromhex("000a0a00f534320e00002a00000d")
        header = b"\x00" + b"SAVE TEST " + struct.pack("<HHH", len(expected), 0x8000, len(expected))

        def encoded_block(flag, payload):
            block = bytes([flag]) + payload
            block += bytes([functools.reduce(operator.xor, block)])
            return struct.pack("<H", len(block)) + block

        input_tape = output / "input.tap"
        input_tape.write_bytes(encoded_block(0, header) + encoded_block(0xFF, expected))
        run("basic", source=input_tape, snapshot=True)
        initial = output / "basic.sna"
        assert basic_bytes(initial) == expected

        # Online catalogues commonly provide zipped tapes. Classification must
        # use the unpacked media so that the same ROM auto-load runs for a ZIP.
        zipped = output / "input.tap.zip"
        with zipfile.ZipFile(zipped, "w", compression=zipfile.ZIP_DEFLATED) as archive:
            archive.write(input_tape, "input.tap")
        run("zip-tape", source=zipped, snapshot=True)
        assert basic_bytes(output / "zip-tape.sna") == expected

        # SAVE a title with filesystem punctuation; export uses a portable filename
        # while the tape's original Spectrum header retains its six-character title.
        save = ["S", "down:Control", "P", "up:Control", "B", ":", "A", "/", "D", "?",
                "down:Control", "P", "up:Control", "down:Return", "wait", "up:Return", "wait", "A", "wait", "wait"]
        run("tape-export", save + export + ["Return"], initial)
        tapes = list((data / "Saves").glob("*.tap"))
        assert len(tapes) == 1, tapes
        tape = tapes[0]
        assert re.fullmatch(r"b_a_d__\d{4}-\d{2}-\d{2}_\d{2}_\d{2}_\d{2}_\d{3}\.tap", tape.name), tape.name
        header, block = tape_blocks(tape)
        assert header[:2] == b"\x00\x00" and header[2:12].rstrip() == b"b:a/d?"
        assert struct.unpack_from("<H", header, 12)[0] == len(expected)
        assert block[0] == 0xFF and block[1:-1] == expected
        shutil.copyfile(tape, output / "roundtrip.tap")
        run("tape-reloaded", source=tape, snapshot=True)
        assert basic_bytes(output / "tape-reloaded.sna") == expected
        print("BASIC SAVE → portable TAP → reload: original program bytes match")

        # A new tape clears blocks while retaining the running BASIC program.
        run("empty-tape", new_tape + export + ["Return"], tape, snapshot=True)
        assert basic_bytes(output / "empty-tape.sna") == expected
        assert list((data / "Saves").glob("*.tap")) == tapes

        # Snapshot Save As sanitizes the filename and restores the actual program.
        run("snapshot-as", snapshot_as + ["Ctrl+A", "B", "A", "D", ":", "/", "?", "Return"], initial)
        saved = data / "Snapshots/BAD___.szx"
        assert saved.read_bytes().startswith(b"ZXST")
        shutil.copyfile(saved, output / "saved.szx")
        run("snapshot-reloaded", source=saved, snapshot=True)
        assert basic_bytes(output / "snapshot-reloaded.sna") == expected
        print("Snapshot Save As → SZX → reload: original program bytes match")

        # A failed destination must remain intact; the page stays open for retry.
        blocked = data / "Snapshots/Snapshot.szx"
        blocked.mkdir()
        (blocked / "keep").write_bytes(b"existing data")
        run("snapshot-retry", snapshot_as + ["Return", "wait", "Ctrl+A", "F", "I", "X", "Return"], initial)
        assert (blocked / "keep").read_bytes() == b"existing data"
        assert (data / "Snapshots/FIX.szx").read_bytes().startswith(b"ZXST")

        run("quick-save", ["click:50:24", "Down", "Return"], initial)
        quick = list((data / "Snapshots/basic").glob("*.szx"))
        assert len(quick) == 1 and ":" not in quick[0].name
        os.utime(quick[0], (1, 1))  # Root Save As file is now newer than the per-game file.
        run("quick-load-as", ["click:50:24", "Down", "Down", "Return", "wait", "wait",
                              "click:50:24", "Down", "Return"], initial)
        assert len(list((data / "Snapshots/FIX").glob("*.szx"))) == 1

        # Stop with an unavailable recording folder, repair it, then retry the
        # pending recording. No re-recording is started between the attempts.
        recordings = data / "Recordings"
        recordings.rmdir()
        recordings.write_bytes(b"blocking file")

        def repair_recordings(log, stopped):
            while not stopped.wait(0.02):
                if "Can't create folder" in log.read_text():
                    recordings.unlink()
                    recordings.mkdir()
                    return
            raise AssertionError("The recording write never failed")

        run("recording-retry", start_record + ["wait"] * 3 + stop_record + ["wait"] * 8 + stop_record,
            initial, repair=repair_recordings)
        files = list(recordings.glob("*.rzx"))
        assert len(files) == 1 and ":" not in files[0].name, files
        raw = files[0].read_bytes()
        assert raw.startswith(b"RZX!")
        pos, frames = 10, 0
        while pos < len(raw):
            kind, size = struct.unpack_from("<BI", raw, pos)
            assert size >= 5 and pos + size <= len(raw)
            if kind == 0x80:
                frames += struct.unpack_from("<I", raw, pos + 5)[0]
            pos += size
        assert pos == len(raw) and frames > 10, frames
        shutil.copyfile(files[0], output / "recovered.rzx")
        print("Failed snapshot save and RZX save: existing data preserved; retry succeeds")
    print("Saving integration checks passed")


if __name__ == "__main__":
    main()
