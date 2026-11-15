#!/usr/bin/env python3
"""Add Qt and the MinGW runtime to an installed Windows build of Spectacol.

windeployqt only runs on Windows, so this does its job for cross builds: it
copies Qt's plugins, the QML modules the UI imports and every DLL that these
and the application need, beside spectacol.exe.

Copyright (c) 2015-2026, BogDan Vatra. GPL-3.0-or-later.
"""
import argparse
import json
import re
import shutil
import subprocess
from pathlib import Path

# The plugins of the Qt modules Spectacol uses: Gui, Svg, Network and Multimedia
PLUGIN_TYPES = ("platforms", "imageformats", "iconengines", "tls", "networkinformation", "multimedia")
# Windows, and offscreen for headless checks
PLATFORM_PLUGINS = ("qwindows.dll", "qoffscreen.dll")
# Development files in Qt's QML directory
QML_DEVELOPMENT_FILES = shutil.ignore_patterns("*.qmltypes", "*.a", "*.prl", "*.debug", "*.pdb")
# DLLs that must come from Qt or MinGW, never from Windows
PACKAGED_DLL = re.compile(r"(qt6|lib)", re.IGNORECASE)


def run(command):
    return subprocess.run([str(part) for part in command], check=True, capture_output=True,
                          text=True).stdout


def deploy_plugins(app, qt):
    for plugin_type in PLUGIN_TYPES:
        for plugin in sorted((qt / "plugins" / plugin_type).glob("*.dll")):
            if plugin_type == "platforms" and plugin.name.lower() not in PLATFORM_PLUGINS:
                continue
            target = app / "plugins" / plugin_type / plugin.name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(plugin, target)


def deploy_qml(app, qt, scanner, qml_roots, import_paths):
    command = [scanner]
    for root in qml_roots:
        command += ["-rootPath", root]
    for path in [qt / "qml", *import_paths]:
        command += ["-importPath", path]
    qt_qml = (qt / "qml").resolve()
    modules = sorted({Path(entry["path"]).resolve() for entry in json.loads(run(command))
                      if entry.get("type") == "module" and "path" in entry})
    copied = []
    for module in modules:
        # CMake installs the application's own modules. Like windeployqt, copy
        # whole module trees: Qt Quick Controls loads its styles at run time.
        if not module.is_relative_to(qt_qml) or any(module.is_relative_to(c) for c in copied):
            continue
        shutil.copytree(module, app / "qml" / module.relative_to(qt_qml),
                        ignore=QML_DEVELOPMENT_FILES, dirs_exist_ok=True)
        copied.append(module)
    print("QML modules:", ", ".join(str(m.relative_to(qt_qml)) for m in copied))


def deploy_dlls(app, objdump, dll_paths):
    available = {}
    for directory in dll_paths:
        for dll in directory.glob("*.dll"):
            available.setdefault(dll.name.lower(), dll)
    present = {dll.name.lower() for dll in app.glob("*.dll")}
    pending = [path for path in app.rglob("*") if path.suffix.lower() in (".exe", ".dll")]
    system = set()
    while pending:
        binary = pending.pop()
        for name in re.findall(r"^\s*DLL Name: (\S+)", run([objdump, "-p", binary]), re.MULTILINE):
            if name.lower() in present:
                continue
            source = available.get(name.lower())
            if source is None:
                system.add(name)
                continue
            # Plugins find their dependencies in the application's directory
            target = app / source.name
            shutil.copy2(source, target)
            present.add(name.lower())
            pending.append(target)
    missing = sorted(name for name in system if PACKAGED_DLL.match(name))
    if missing:
        raise SystemExit("Not found: " + ", ".join(missing))
    print("Windows DLLs:", ", ".join(sorted(system, key=str.lower)))


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("app", type=Path, help="the installed application folder")
    parser.add_argument("--qt", type=Path, required=True, help="the prefix of Qt for Windows")
    parser.add_argument("--qmlimportscanner", type=Path, required=True,
                        help="qmlimportscanner of the build machine's Qt")
    parser.add_argument("--qml-root", type=Path, action="append", required=True,
                        help="a directory of the application's QML files")
    parser.add_argument("--import-path", type=Path, action="append", default=[],
                        help="another QML import directory, e.g. the build tree's")
    parser.add_argument("--dll-path", type=Path, action="append", default=[],
                        help="another directory of DLLs, e.g. the MinGW runtime's")
    parser.add_argument("--objdump", default="x86_64-w64-mingw32-objdump")
    parser.add_argument("--strip", help="strip the binaries with this tool")
    args = parser.parse_args()

    deploy_plugins(args.app, args.qt)
    deploy_qml(args.app, args.qt, args.qmlimportscanner, args.qml_root, args.import_path)
    deploy_dlls(args.app, args.objdump, [args.qt / "bin", *args.dll_path])
    if args.strip:
        binaries = [path for path in args.app.rglob("*") if path.suffix.lower() in (".exe", ".dll")]
        run([args.strip, "--strip-unneeded", *binaries])


if __name__ == "__main__":
    main()
