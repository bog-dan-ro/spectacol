#!/usr/bin/env python3
"""Check the quit question with Android's Back, a D-pad/remote and touch on a disposable emulator.

Copyright (c) 2015-2026, BogDan Vatra. GPL-3.0-or-later.
Requires an installed debug APK. Temporarily replaces and then restores the app's settings and
force-stops Spectacol. Do not run it against a device being used interactively.
"""
import argparse
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import tempfile
import time
import uuid
import xml.etree.ElementTree as ET

PACKAGE = "eu.licentia.games.spectacol"
ACTIVITY = PACKAGE + "/org.qtproject.qt.android.bindings.QtActivity"
QUESTION = "Quit Spectacol?"
BACK, DPAD_LEFT, DPAD_CENTER, ENTER = 4, 21, 23, 66


def main():
    repo = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--adb", default=shutil.which("adb") or "adb")
    parser.add_argument("--serial", required=True)
    parser.add_argument("--output", type=Path, default=repo / "build/android-quit")
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    token = "spectacol-quit-" + uuid.uuid4().hex[:8]

    def adb(*words, check=True):
        return subprocess.run([args.adb, "-s", args.serial, *words], capture_output=True,
                              check=check, timeout=60)

    def shell(*words, check=True):
        return adb("shell", shlex.join(words), check=check)

    def private(*words, check=True):
        return shell("run-as", PACKAGE, *words, check=check)

    base = private("pwd").stdout.decode().strip() + "/files"
    settings = base + "/settings/Licentia/Spectacol.conf"
    staging = "/data/local/tmp/" + token + ".conf"
    data = base + "/" + token
    dump = "/sdcard/" + token + ".xml"

    def nodes():
        shell("uiautomator", "dump", dump)
        raw = shell("cat", dump).stdout
        (output / "last-ui.xml").write_bytes(raw)
        return list(ET.fromstring(raw).iter("node"))

    def label(node):
        return node.get("text") or node.get("content-desc") or ""

    def press(node):
        x1, y1, x2, y2 = map(int, re.findall(r"\d+", node.get("bounds")))
        shell("input", "tap", str((x1 + x2) // 2), str((y1 + y2) // 2))
        time.sleep(.7)

    def wait_for(text, timeout=20):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            found = nodes()
            cling = [n for n in found if n.get("resource-id") == "com.android.systemui:id/ok"]
            if cling:
                press(cling[0])
                continue
            matches = [n for n in found if label(n) == text]
            if matches:
                return matches[0]
            time.sleep(.5)
        raise AssertionError(repr(text) + " did not appear; see last-ui.xml")

    def gone(text, timeout=10):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if not any(label(n) == text for n in nodes()):
                return True
            time.sleep(.5)
        return False

    def key(code):
        shell("input", "keyevent", str(code))
        time.sleep(.7)

    def focused(text):
        return any(label(n) == text and n.get("focused") == "true" for n in nodes())

    def running():
        return bool(shell("pidof", PACKAGE, check=False).stdout.strip())

    def ask():
        key(BACK)
        wait_for(QUESTION)
        wait_for("Cancel")
        wait_for("Quit")

    saved_settings = private("cat", settings, check=False)
    shell("am", "force-stop", PACKAGE)
    try:
        with tempfile.TemporaryDirectory() as temporary:
            config = Path(temporary) / "Spectacol.conf"
            config.write_text("[General]\nfirst_start=false\nautoSaveOnExit=false\ndataPath=" + data + "\n")
            adb("push", str(config), staging)
        private("mkdir", "-p", str(Path(settings).parent), data)
        private("cp", staging, settings)
        shell("am", "start", "-S", "-W", "-n", ACTIVITY)
        wait_for("Menu")

        ask()
        key(BACK)  # Back answers no, it doesn't ask again
        assert gone(QUESTION), "Back did not answer the question"
        time.sleep(1)
        assert gone(QUESTION, 1) and running(), "Back asked again or quit"
        print("Back asks, Back answers no", flush=True)

        ask()
        assert focused("Quit"), "the focus does not start on Quit"
        key(DPAD_LEFT)  # from Quit to Cancel, as on a TV remote
        if not focused("Cancel"):
            key(DPAD_LEFT)  # after a touch Android takes the first D-pad press to leave touch mode
        assert focused("Cancel"), "D-pad Left did not reach Cancel"
        key(DPAD_CENTER)
        assert gone(QUESTION) and running(), "the remote's OK on Cancel"
        print("D-pad and OK choose Cancel", flush=True)

        ask()
        press(wait_for("Cancel"))
        assert gone(QUESTION) and running(), "tapping Cancel"
        print("Tapping Cancel keeps playing", flush=True)

        ask()
        assert focused("Quit"), "the focus does not start on Quit"
        key(ENTER)
        deadline = time.monotonic() + 15
        while running() and time.monotonic() < deadline:
            time.sleep(.5)
        assert not running(), "Quit did not quit"
        print("Enter on Quit quits", flush=True)
    finally:
        (output / "logcat.txt").write_bytes(adb("logcat", "-d", "-t", "3000", check=False).stdout)
        (output / "last-screen.png").write_bytes(adb("exec-out", "screencap", "-p", check=False).stdout)
        shell("am", "force-stop", PACKAGE, check=False)
        if saved_settings.returncode == 0:
            with tempfile.TemporaryDirectory() as temporary:
                backup = Path(temporary) / "settings.conf"
                backup.write_bytes(saved_settings.stdout)
                adb("push", str(backup), staging)
                private("cp", staging, settings)
        else:
            private("rm", "-f", settings, check=False)
        private("rm", "-rf", data, check=False)
        shell("rm", "-f", staging, dump, check=False)


if __name__ == "__main__":
    main()
