#!/usr/bin/env python3
"""Check document intents and snapshot export on a disposable English Android emulator.

Copyright (c) 2015-2026, BogDan Vatra. GPL-3.0-or-later.
Requires an installed debug APK. Temporarily replaces and then restores settings;
force-stops Spectacol. Do not run against a device being used interactively.
"""
import argparse
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import tempfile
import time
from urllib.parse import quote
import uuid
import xml.etree.ElementTree as ET

from controller import make_diagnostic


def main():
    repo = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--adb", default=shutil.which("adb") or "adb")
    parser.add_argument("--serial", required=True)
    parser.add_argument("--output", type=Path, default=repo / "build/android")
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    package = "eu.licentia.games.spectacol"
    activity = package + "/org.qtproject.qt.android.bindings.QtActivity"
    token = "spectacol-check-" + uuid.uuid4().hex[:12]

    def adb(*words, check=True):
        return subprocess.run([args.adb, "-s", args.serial, *words], capture_output=True,
                              check=check, timeout=45)

    def shell(*words, check=True):
        return adb("shell", shlex.join(words), check=check)

    def private(*words, check=True):
        return shell("run-as", package, *words, check=check)

    base = private("pwd").stdout.decode().strip() + "/files"
    data = base + "/" + token
    settings = base + "/settings/Licentia/Spectacol.conf"
    shared = "/sdcard/Download/" + token + ".sna"
    document = "content://com.android.externalstorage.documents/document/" + quote(
        "primary:Download/" + token + ".sna", safe="")
    shell("am", "force-stop", package)
    saved_settings = private("cat", settings, check=False)

    def start(name, uri=None, timed=True):
        words = ["am", "start", "-S", "-W", "-n", activity]
        if uri:
            words += ["-a", "android.intent.action.VIEW", "--grant-read-uri-permission", "-d", uri]
        if timed:
            words += ["--es", "applicationArguments",
                      "--machine 48k --quit-after 10 --screenshot " + data + "/" + name + ".sna"]
        result = shell(*words)
        (output / (name + "-start.log")).write_bytes(result.stdout + result.stderr)

    def snapshot(name, expected):
        deadline = time.monotonic() + 30
        raw = b""
        while time.monotonic() < deadline:
            result = private("cat", data + "/" + name + ".sna", check=False)
            if result.returncode == 0 and len(result.stdout) == 49179:
                raw = result.stdout
                break
            time.sleep(.5)
        (output / (name + ".sna")).write_bytes(raw)
        assert len(raw) == 49179, (name, raw[:100])
        assert raw[27:27 + 6912] == expected[27:27 + 6912], name + ": screen differs"
        offset = 27 + 0x9000 - 0x4000
        assert raw[offset:offset + 8] == bytes([0x1F]) * 8, name + ": CPU did not resume"
        print(name + ": document loaded, screen bytes match", flush=True)

    def nodes():
        shell("uiautomator", "dump", "/sdcard/" + token + ".xml")
        raw = shell("cat", "/sdcard/" + token + ".xml").stdout
        (output / "last-ui.xml").write_bytes(raw)
        return list(ET.fromstring(raw).iter("node"))

    def tap(predicate):
        for _ in range(4):
            for node in nodes():
                if predicate(node):
                    x1, y1, x2, y2 = map(int, re.findall(r"\d+", node.get("bounds")))
                    shell("input", "tap", str((x1 + x2) // 2), str((y1 + y2) // 2))
                    time.sleep(.5)
                    return
            time.sleep(.5)
        raise AssertionError("UI target missing; see last-ui.xml")

    def key(code):
        shell("input", "keyevent", str(code))
        time.sleep(.5)

    def label(node):
        return node.get("text") or node.get("content-desc")

    staging = "/data/local/tmp/" + token + ".conf"
    exported = "/sdcard/Download/" + token + ".szx"
    try:
        with tempfile.TemporaryDirectory(prefix=token) as temporary:
            temp = Path(temporary)
            fixture = temp / "input.sna"
            make_diagnostic(fixture)
            expected = bytearray(fixture.read_bytes())
            # A visible deterministic screen distinguishes a loaded snapshot from ROM boot.
            expected[27:27 + 6144] = bytes([0xAA, 0x55]) * 3072
            expected[27 + 6144:27 + 6912] = bytes([0x47]) * 768
            fixture.write_bytes(expected)
            adb("push", str(fixture), shared)
            config = temp / "Spectacol.conf"
            config.write_text("[General]\nfirst_start=false\nautoSaveOnExit=false\ndataPath="
                              + data + "\n[Screen]\norientation=1\n")
            adb("push", str(config), staging)
            private("mkdir", "-p", str(Path(settings).parent), data)
            private("cp", staging, settings)
            start("cold", document)
            snapshot("cold", expected)
            start("warm")
            assert any(label(n) == "=" for n in nodes()), "Main window did not appear"
            key(3)  # Home: the intent must bring a backgrounded activity back to life.
            shell("am", "start", "-W", "-n", activity, "-a", "android.intent.action.VIEW",
                  "--grant-read-uri-permission", "-d", document)
            snapshot("warm", expected)
            imported = private("ls", data + "/Imported").stdout.decode().splitlines()
            assert len(imported) == 2 and len(set(imported)) == 2, imported
            for name in imported:
                assert private("cat", data + "/Imported/" + name).stdout == expected
            start("export", document, timed=False)
            assert any(label(n) == "=" for n in nodes()), "Main window did not appear"
            # The window can become accessible before its queued VIEW import.
            deadline = time.monotonic() + 10
            while len(private("ls", data + "/Imported").stdout.decode().splitlines()) < 3:
                assert time.monotonic() < deadline, "Export source did not load"
                time.sleep(.2)
            time.sleep(.5)
            key(135)  # F5: drawer, then Media / Save snapshot as
            for _ in range(4):
                key(20)
            key(66)
            key(66)
            time.sleep(1)
            assert any(label(n) == "Export…" for n in nodes()), "Save page did not open"
            key(4)  # Dismiss the input method; the save page must remain open.
            tap(lambda n: label(n) == "Export…")
            tap(lambda n: n.get("resource-id") == "android:id/title"
                and n.get("class") == "android.widget.EditText")
            shell("input", "keycombination", "113", "29")  # Ctrl+A
            shell("input", "text", token + ".szx")
            tap(lambda n: n.get("resource-id") == "android:id/button1")
            time.sleep(1)
            raw = shell("cat", exported).stdout
            assert raw.startswith(b"ZXST"), raw[:100]
            (output / "exported.szx").write_bytes(raw)
            uri = "content://com.android.externalstorage.documents/document/" + quote(
                "primary:Download/" + token + ".szx", safe="")
            start("export-reload", uri)
            snapshot("export-reload", expected)
            print("Native snapshot export and content-provider reload passed", flush=True)
    finally:
        (output / "logcat.txt").write_bytes(adb("logcat", "-d", "-t", "3000", check=False).stdout)
        (output / "last-screen.png").write_bytes(adb("exec-out", "screencap", "-p", check=False).stdout)
        shell("am", "force-stop", package, check=False)
        if saved_settings.returncode == 0:
            with tempfile.TemporaryDirectory() as temporary:
                backup = Path(temporary) / "settings.conf"
                backup.write_bytes(saved_settings.stdout)
                adb("push", str(backup), staging)
                private("cp", staging, settings)
        else:
            private("rm", "-f", settings, check=False)
        private("rm", "-rf", data, check=False)
        shell("rm", "-f", staging, shared, exported, "/sdcard/" + token + ".xml", check=False)


if __name__ == "__main__":
    main()
