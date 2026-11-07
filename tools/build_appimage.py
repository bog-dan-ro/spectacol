#!/usr/bin/env python3
# Copyright (c) 2015-2026, BogDan Vatra <bogdan@kde.org>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build a Linux AppImage using a Qt SDK and linuxdeploy's Qt plugin."""

import argparse
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import tempfile


def main():
    source_dir = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--qt-prefix", type=Path, required=True,
                        help="Qt 6.11+ desktop SDK directory (contains bin/qmake)")
    parser.add_argument("--linuxdeploy", default=os.environ.get("LINUXDEPLOY", "linuxdeploy"),
                        help="linuxdeploy executable; place its Qt plugin alongside it or in PATH")
    parser.add_argument("--build-dir", type=Path, default=source_dir / "build" / "appimage")
    parser.add_argument("--output-dir", type=Path, default=source_dir / "build" / "packages")
    parser.add_argument("--jobs", type=int, default=min(4, os.cpu_count() or 1))
    parser.add_argument("cmake_args", nargs=argparse.REMAINDER,
                        help="additional CMake arguments after --")
    args = parser.parse_args()

    if platform.system() != "Linux":
        parser.error("AppImages must be built on Linux")
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    qt_prefix = args.qt_prefix.resolve()
    qmake = qt_prefix / "bin" / "qmake"
    if not qmake.is_file():
        parser.error(f"Qt SDK has no {qmake}")
    linuxdeploy = shutil.which(args.linuxdeploy)
    if not linuxdeploy:
        parser.error(f"cannot find linuxdeploy: {args.linuxdeploy}")
    linuxdeploy = str(Path(linuxdeploy).resolve())
    build_dir = args.build_dir.resolve()
    output_dir = args.output_dir.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    cmake_args = args.cmake_args
    if cmake_args[:1] == ["--"]:
        cmake_args = cmake_args[1:]

    subprocess.run([
        str(qt_prefix / "bin" / "qt-cmake"), "-S", str(source_dir), "-B", str(build_dir),
        "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release", f"-DCMAKE_PREFIX_PATH={qt_prefix}",
        "-DCMAKE_INSTALL_PREFIX=/usr", "-DCMAKE_INSTALL_LIBDIR=lib",
        "-DSPECTACOL_BUILD_TESTS=OFF", *cmake_args,
    ], check=True)
    subprocess.run(["cmake", "--build", str(build_dir), "--parallel", str(args.jobs)], check=True)

    # A fresh staging directory prevents files from older packages leaking in.
    app_dir = Path(tempfile.mkdtemp(prefix="AppDir-", dir=build_dir))
    app_dir.chmod(0o755)
    env = os.environ.copy()
    env["DESTDIR"] = str(app_dir)
    subprocess.run(["cmake", "--install", str(build_dir), "--component", "Runtime"],
                   check=True, env=env)
    env.pop("DESTDIR")
    # appimagetool's metadata lookup still uses the older AppStream suffix.
    metainfo = app_dir / "usr/share/metainfo/io.github.bog_dan_ro.spectacol.metainfo.xml"
    metainfo.with_suffix("").with_suffix(".appdata.xml").symlink_to(metainfo.name)
    # linuxdeploy's Qt plugin deploys the Wayland platform library without its
    # dynamically loaded EGL, shell and decoration integrations. Stage these
    # first so linuxdeploy also resolves and bundles their shared dependencies.
    for plugin_type in ("wayland-graphics-integration-client", "wayland-shell-integration",
                        "wayland-decoration-client"):
        plugins = qt_prefix / "plugins" / plugin_type
        if plugins.is_dir():
            shutil.copytree(plugins, app_dir / "usr/plugins" / plugin_type, symlinks=True)
    env["QMAKE"] = str(qmake)
    env["QML_SOURCES_PATHS"] = str(source_dir / "spectacol")
    env["QML_MODULES_PATHS"] = str(build_dir / "qml")
    env["LD_LIBRARY_PATH"] = os.pathsep.join(filter(None, [
        str(build_dir / "lib"), str(qt_prefix / "lib"), env.get("LD_LIBRARY_PATH"),
    ]))
    env["EXTRA_QT_MODULES"] = "svg;waylandclient"
    platforms = qt_prefix / "plugins" / "platforms"
    env["EXTRA_PLATFORM_PLUGINS"] = ";".join(
        path.name for path in sorted(platforms.glob("libq*.so"))
        if path.name == "libqoffscreen.so" or path.name.startswith("libqwayland")
    )
    # Both deployment tools and the generated image can run without a FUSE mount.
    env.setdefault("APPIMAGE_EXTRACT_AND_RUN", "1")
    version = re.search(r"^CMAKE_PROJECT_VERSION:STATIC=(.+)$",
                        (build_dir / "CMakeCache.txt").read_text(), re.MULTILINE).group(1)
    env["VERSION"] = version
    env["OUTPUT"] = str(output_dir / f"Spectacol-{version}-{platform.machine()}.AppImage")
    subprocess.run([linuxdeploy, "--appdir", str(app_dir), "--plugin", "qt",
                    "--output", "appimage"], cwd=output_dir, env=env, check=True)
    print(f"AppImage: {env['OUTPUT']}")
    print(f"Staging directory: {app_dir}")


if __name__ == "__main__":
    main()
