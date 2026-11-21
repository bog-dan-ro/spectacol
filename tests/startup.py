#!/usr/bin/env python3
"""Check the startup machine and deferred legacy import in the compiled application.

Copyright (c) 2015-2026, BogDan Vatra. GPL-3.0-or-later.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import os
from pathlib import Path
import subprocess
import tempfile


def main():
    repo = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=repo / 'build/bin/spectacol')
    parser.add_argument('--output', type=Path, default=repo / 'build/startup')
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    cases = [
        ('fresh', '', [], 2, False),
        ('saved-48k', '[%General]\nmachine=48k\n', [], 1, False),
        ('saved-48k-ntsc', '[%General]\nmachine=48k_ntsc\n', [], 1, False),
        ('saved-128k', '[%General]\nmachine=128k\n', [], 2, False),
        ('override-48k', '[%General]\nmachine=128k\n', ['--machine', '48k'], 1, False),
        ('invalid-saved', '[%General]\nmachine=missing\n', [], 2, False),
        ('import-after-welcome', '', [], 2, True),
        ('import-without-welcome', 'first_start=false\n', [], 2, True),
    ]
    with tempfile.TemporaryDirectory(prefix='session-', dir=output) as temporary:
        session = Path(temporary)

        def run(case):
            name, settings, options, machine, importing = case
            home = session / name
            config = home / 'config/Licentia'
            config.mkdir(parents=True)
            settings = f'[General]\nautoSaveOnExit=false\ndataPath={home / "data"}\n' + settings
            if importing:
                settings += f'[Android]\nlegacyDataPath={home / "old"}\n'
            (config / 'Spectacol.conf').write_text(settings)
            snapshot = output / (name + '.szx')
            screenshot = output / (name + '.png')
            command = [str(args.binary.resolve()), '--quit-after', '3', '--window-size', '1280x800',
                       '--screenshot', str(snapshot), '--ui-screenshot', str(screenshot), *options]
            if name == 'import-after-welcome':
                command += ['--ui-keys', 'Escape']
            env = os.environ | {'QT_QPA_PLATFORM': 'offscreen', 'XDG_CONFIG_HOME': str(config.parent),
                                'XDG_DATA_HOME': str(home / 'share'), 'SPECTACOL_TOUCHSCREEN': '0'}
            result = subprocess.run(command, env=env, capture_output=True, text=True, timeout=25)
            (output / (name + '.log')).write_text(result.stdout + result.stderr)
            assert result.returncode == 0, (name, result.stderr)
            assert not any(error in result.stderr for error in ('ReferenceError', 'TypeError', 'Binding loop'))
            data = snapshot.read_bytes()
            assert data[:4] == b'ZXST' and data[6] == machine, (name, data[:8])
            if name == 'saved-48k-ntsc':
                assert 'machine=48k\n' in (config / 'Spectacol.conf').read_text()
            text = subprocess.check_output(['tesseract', str(screenshot), 'stdout'], stderr=subprocess.DEVNULL,
                                           text=True, timeout=15).lower()
            if importing:
                assert 'choose the old folder' in text, (name, text)
            elif name == 'fresh':
                assert 'gamepad in menus' in text, (name, text)
            return name

        with ThreadPoolExecutor(max_workers=3) as pool:
            for name in pool.map(run, cases):
                print(name + ': passed', flush=True)


if __name__ == '__main__':
    main()
