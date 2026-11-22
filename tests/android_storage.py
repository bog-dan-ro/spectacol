#!/usr/bin/env python3
"""Check device folders and the Spectacol 4 file import on a disposable English Android emulator.

Copyright (c) 2015-2026, BogDan Vatra. GPL-3.0-or-later.
Requires an installed debug APK. Temporarily replaces and then restores the app's settings, adds
folders to shared storage and removes them afterwards, and force-stops Spectacol. It drives
Android's folder picker: do not run it against a device being used interactively.
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

from controller import make_diagnostic

PACKAGE = "eu.licentia.games.spectacol"
ACTIVITY = PACKAGE + "/org.qtproject.qt.android.bindings.QtActivity"
PICKER = "com.google.android.documentsui:id/"


def main():
    repo = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--adb", default=shutil.which("adb") or "adb")
    parser.add_argument("--serial", required=True)
    parser.add_argument("--output", type=Path, default=repo / "build/android-storage")
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    token = "spectacol-storage-" + uuid.uuid4().hex[:8]

    def adb(*words, check=True):
        return subprocess.run([args.adb, "-s", args.serial, *words], capture_output=True,
                              check=check, timeout=60)

    def shell(*words, check=True):
        return adb("shell", shlex.join(words), check=check)

    def private(*words, check=True):
        return shell("run-as", PACKAGE, *words, check=check)

    assert shell("getprop", "ro.kernel.qemu").stdout.strip() == b"1", "Use a disposable emulator"
    base = private("pwd").stdout.decode().strip() + "/files"
    settings = base + "/settings/Licentia/Spectacol.conf"
    staging = "/data/local/tmp/" + token + ".conf"
    data = base + "/" + token
    old_name = token + "-old"
    games_name = token + "-games"
    old = "/sdcard/" + old_name
    games = "/sdcard/" + games_name
    dump = "/sdcard/" + token + ".xml"

    def nodes():
        shell("uiautomator", "dump", dump)
        raw = shell("cat", dump).stdout
        (output / "last-ui.xml").write_bytes(raw)
        return list(ET.fromstring(raw).iter("node"))

    def label(node):
        return node.get("text") or node.get("content-desc") or ""

    def wait_for(predicate, what, timeout=20):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            found = nodes()
            # Android explains immersive mode the first time
            cling = [n for n in found if n.get("resource-id") == "com.android.systemui:id/ok"]
            if cling:
                press(cling[0])
                continue
            matches = [n for n in found if predicate(n)]
            if matches:
                return matches
            time.sleep(.5)
        raise AssertionError(what + " did not appear; see last-ui.xml")

    def press(node):
        x1, y1, x2, y2 = map(int, re.findall(r"\d+", node.get("bounds")))
        shell("input", "tap", str((x1 + x2) // 2), str((y1 + y2) // 2))
        time.sleep(.7)

    def long_press(text):
        node = wait_for(lambda n: label(n) == text, repr(text))[0]
        x1, y1, x2, y2 = map(int, re.findall(r"\d+", node.get("bounds")))
        x, y = str((x1 + x2) // 2), str((y1 + y2) // 2)
        shell("input", "swipe", x, y, x, y, "1400")

    def tap(text, what=None):
        press(wait_for(lambda n: label(n) == text, what or repr(text))[0])

    def gone(text, timeout=10):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if not any(label(n) == text for n in nodes()):
                return True
            time.sleep(.5)
        return False

    def picker_folder(name, from_root):
        if from_root:
            crumbs = wait_for(lambda n: n.get("resource-id") == PICKER + "breadcrumb_text",
                              "The folder picker")
            press(min(crumbs, key=lambda n: int(re.findall(r"\d+", n.get("bounds"))[0])))
            press(wait_for(lambda n: label(n) == name and n.get("resource-id") == "android:id/title",
                           "Folder " + name)[0])
        wait_for(lambda n: n.get("resource-id") == PICKER + "header_title"
                 and label(n) == "Files in " + name, "The picker in " + name)
        press(wait_for(lambda n: n.get("resource-id") == "android:id/button1", "Use this folder")[0])
        allow = wait_for(lambda n: n.get("resource-id") == "android:id/button1"
                         and label(n).lower() == "allow", "Allow")[0]
        opened = time.monotonic()
        press(allow)
        return opened

    def write_settings(text):
        with tempfile.TemporaryDirectory() as temporary:
            config = Path(temporary) / "Spectacol.conf"
            config.write_text(text)
            adb("push", str(config), staging)
        private("mkdir", "-p", str(Path(settings).parent))
        private("cp", staging, settings)

    def start(arguments=None):
        words = ["am", "start", "-S", "-W", "-n", ACTIVITY]
        if arguments:
            words += ["--es", "applicationArguments", shlex.join(arguments)]
        shell(*words)

    def persisted():
        """The trees Android keeps for Spectacol across restarts."""
        text = shell("dumpsys", "activity", "permissions").stdout.decode(errors="replace")
        trees = set()
        for block in re.findall(r"UriPermission\{\S+ (\S+)[^}]*\}\s*\n([^\n]*)\n([^\n]*)", text):
            uri, owner, modes = block
            if "targetPkg=" + PACKAGE in owner and "persisted=0x0" not in modes:
                trees.add(uri)
        return trees

    def read_private(path):
        return private("cat", path, check=False).stdout

    saved_settings = private("cat", settings, check=False)
    shell("am", "force-stop", PACKAGE)
    try:
        with tempfile.TemporaryDirectory(prefix=token) as temporary:
            temp = Path(temporary)
            fixture = temp / "game.sna"
            make_diagnostic(fixture)
            expected = bytearray(fixture.read_bytes())
            # A visible screen tells a loaded snapshot from the ROM's
            expected[27:27 + 6144] = bytes([0xAA, 0x55]) * 3072
            expected[27 + 6144:27 + 6912] = bytes([0x47]) * 768
            fixture.write_bytes(expected)
            old_snapshot = b"ZXST" + token.encode()
            (temp / "old.szx").write_bytes(old_snapshot)

            # A reinstall has neither first_start nor an old dataPath setting. Welcome closes
            # first; the import offer must follow it and remain available in General options.
            write_settings("[General]\nautoSaveOnExit=false\ndataPath=" + data + "\n")
            start()
            wait_for(lambda n: n.get("resource-id", "").endswith(".aboutTabs"), "The first-start welcome")
            shell("input", "keyevent", "4")
            tap("Choose the old folder", "The import offer after the first-start welcome")
            # No settings survived the reinstall: the picker uses the default Spectacol folder.
            wait_for(lambda n: n.get("resource-id") == "android:id/button1", "The folder picker")
            shell("input", "keyevent", "4")
            tap("Don't ask again")
            start()
            wait_for(lambda n: label(n) == "Menu", "The main window")
            assert gone("Choose the old folder", 2), "Don't ask again was ignored"
            print("Fresh-install import offered after welcome; declining is kept", flush=True)

            # Spectacol 4 kept its files in shared storage: the app offers to copy them once.
            shell("mkdir", "-p", old + "/Favourites", old + "/Snapshots/" + token)
            adb("push", str(fixture), old + "/Favourites/" + token + ".sna")
            adb("push", str(temp / "old.szx"), old + "/Snapshots/" + token + "/old.szx")
            shell("touch", "-d", "2019-05-04T13:14:15", old + "/Snapshots/" + token + "/old.szx")
            old_time = shell("stat", "-c", "%Y", old + "/Snapshots/" + token + "/old.szx")
            write_settings("[General]\nfirst_start=false\nautoSaveOnExit=false\ndataPath=/storage/emulated/0/"
                           + old_name + "/\n")
            start()
            tap("Choose the old folder", "The offer to copy Spectacol 4's files")
            # The picker starts in the old folder: the user only confirms it
            picker_folder(old_name, from_root=False)
            wait_for(lambda n: label(n) == "Done", "The copy's summary")
            assert read_private(base + "/Favourites/" + token + ".sna") == expected
            assert read_private(base + "/Snapshots/" + token + "/old.szx") == old_snapshot
            copied_time = private("stat", "-c", "%Y", base + "/Snapshots/" + token + "/old.szx")
            assert copied_time.stdout.strip() == old_time.stdout.strip(), "modification time"
            assert b"legacyImport=done" in read_private(settings)
            assert not any(old_name in uri for uri in persisted()), "the old folder's grant was kept"
            print("Spectacol 4 files copied, times kept, grant released", flush=True)
            tap("Done")
            start()
            wait_for(lambda n: label(n) == "Menu", "The main window")
            time.sleep(2)
            assert gone("Choose the old folder", 2), "the offer came back"
            print("No offer after the copy", flush=True)

            # Device folders: granted once, listed in the Spectacol folder, browsed in place.
            shell("am", "force-stop", PACKAGE)
            shell("mkdir", "-p", games + "/Sub")
            adb("push", str(fixture), games + "/Sub/" + token + ".sna")
            adb("push", str(temp / "old.szx"), games + "/notes.txt")
            bulk = temp / "bulk"
            bulk.mkdir()
            for index in range(100):
                (bulk / f"game-{index:03}.sna").write_bytes(expected)
            (bulk / "game-000.POK").write_text("NDevice trainer\nZ 8 36864 77 0\nY\n")
            adb("push", str(bulk) + "/.", games + "/")
            private("mkdir", "-p", data)
            write_settings("[General]\nfirst_start=false\nautoSaveOnExit=false\ndataPath=" + data
                           + "\n[Android]\nlegacyImport=never\n[Peripherals]\njoystickPrompt=false"
                           + "\n[LocalFilesView]\nmode=1\n")
            start(["--machine", "48k", "--quit-after", "90", "--screenshot", data + "/device.sna",
                   "--ui-keys", "page:FileBrowserPage.qml"])
            tap("Page actions", "Quick Open")
            tap("Add device folder…")
            opened = picker_folder(games_name, from_root=True)
            # Quick Open goes into the new folder
            wait_for(lambda n: label(n) == "game-000.sna", "Games in the 100-file device folder", timeout=8)
            elapsed = time.monotonic() - opened
            assert elapsed < 8, f"Opening the device folder took {elapsed:.1f}s"
            print(f"100-file device folder shown in {elapsed:.1f}s including UI automation", flush=True)
            wait_for(lambda n: label(n) == games_name and "Label" in (n.get("resource-id") or ""),
                     "The device folder's title")
            wait_for(lambda n: label(n) == "Sub", "The device folder's sub folder")
            assert gone("notes.txt", 1), "a text file was listed"
            tap("Sub")
            wait_for(lambda n: label(n) == token + ".sna", "The game in the subfolder")
            tap("Up a folder")
            game = wait_for(lambda n: label(n) == "game-000.sna", "The game in the large folder")[0]
            opened = time.monotonic()
            press(game)
            wait_for(lambda n: label(n) == "Menu", "Returning to the game", timeout=8)
            elapsed = time.monotonic() - opened
            assert elapsed < 8, f"Opening a game in the large device folder took {elapsed:.1f}s"
            print(f"Game opened in {elapsed:.1f}s including UI automation", flush=True)
            shell("input", "keyevent", "135")  # F5 opens the main menu.
            tap("Machine")
            tap("Poke memory")
            wait_for(lambda n: label(n) == "Device trainer", "The matching device-folder trainer")
            wait_for(lambda n: label(n).startswith("content://")
                     and label(n).endswith("game-000.POK"), "The matching trainer's document URI")
            shell("input", "keyevent", "4")
            deadline = time.monotonic() + 100
            raw = b""
            while time.monotonic() < deadline:
                raw = read_private(data + "/device.sna")
                if len(raw) == 49179:
                    break
                time.sleep(1)
            (output / "device.sna").write_bytes(raw)
            assert len(raw) == 49179, raw[:100]
            assert raw[27:27 + 6912] == expected[27:27 + 6912], "the device folder's game did not load"
            assert any(games_name in uri for uri in persisted()), "the device folder's grant was not kept"
            print("Device folder granted, browsed and its game loaded", flush=True)

            # After a restart Quick Open reopens the device folder with the saved grant.
            start(["--ui-keys", "page:FileBrowserPage.qml"])
            wait_for(lambda n: label(n) == "game-000.sna", "The game after a restart")
            adb("push", str(fixture), games + "/Sub/rename-me.sna")
            tap("Sub")
            wait_for(lambda n: label(n) == token + ".sna", "The game in the subfolder after a restart")
            long_press("rename-me.sna")
            tap("Rename")
            wait_for(lambda n: n.get("resource-id", "").endswith(".browserRenameName"),
                     "The rename field")
            shell("input", "text", "renamed.sna")
            press(wait_for(lambda n: n.get("resource-id", "").endswith(".formSave"),
                           "Confirm rename")[0])
            wait_for(lambda n: label(n) == "renamed.sna", "The renamed file")
            assert shell("cat", games + "/Sub/renamed.sna").stdout == expected
            long_press("renamed.sna")
            tap("Delete")
            wait_for(lambda n: label(n) == 'Delete "renamed.sna"?', "Confirm device deletion")
            tap("Delete")
            assert gone("renamed.sna"), "the deleted file is still listed"
            assert shell("test", "!", "-e", games + "/Sub/renamed.sna").returncode == 0
            print("File Browser renamed and deleted a file in a granted device folder", flush=True)
            tap("Up a folder")
            wait_for(lambda n: label(n) == "Sub", "The device folder after a restart")
            tap("Up a folder")
            wait_for(lambda n: label(n) == "Recordings", "The Spectacol folder")
            wait_for(lambda n: label(n) == games_name, "The device folder in the Spectacol folder")
            tap("Page actions")
            tap("Stop showing")
            # Spectacol's own dialog: it names the folder, and forgetting it is not deleting it
            message = wait_for(lambda n: label(n).startswith("Stop showing \""),
                               "Stop showing the device folder?")
            assert games_name in label(message[0]), label(message[0])
            assert "Its files stay on the device" in label(message[0]), label(message[0])
            tap("Stop showing")
            assert gone(games_name), "the forgotten folder is still listed"
            assert not any(games_name in uri for uri in persisted()), "the forgotten folder's grant"
            assert shell("ls", games + "/Sub/" + token + ".sna", check=False).returncode == 0, \
                "forgetting a device folder touched its files"
            print("Device folder kept across restarts and forgotten", flush=True)
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
        private("rm", "-rf", data, base + "/Favourites/" + token + ".sna",
                base + "/Snapshots/" + token, check=False)
        shell("rm", "-rf", old, games, staging, dump, check=False)


if __name__ == "__main__":
    main()
