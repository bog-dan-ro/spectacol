#!/usr/bin/env python3
"""Exercise profiles, opacity and playback shortcuts through the compiled application.

Copyright (c) 2015-2026, BogDan Vatra. GPL-3.0-or-later.
Uses authored media; controller tests inspect real Spectrum ports.
"""
import argparse
import configparser
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

from PIL import Image, ImageOps
from controller import make_diagnostic
from joysticks import read_snapshot, snapshot, chunk


TURBO = 0x1FFFFFF + 6
PAUSE = 0x1FFFFFF + 3


def main():
    repo = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=repo / 'build/bin/spectacol')
    parser.add_argument('--output', type=Path, default=repo / 'build/controls')
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='session-', dir=output) as temporary:
        session = Path(temporary)

        def run(name, keys, *, mode=0, toggle=False, switch=False, prompt=True, overlay=False, ui=False, assigned=True, declared=False, window="1280x800", large=False, failed_load=False, profile_name="Runner"):
            home = session / name
            config = home / 'config/Licentia'
            config.mkdir(parents=True)
            media = home / 'media'
            media.mkdir()
            source = media / 'Game One.sna'
            if declared:
                source = source.with_suffix('.szx')
                source.write_bytes(snapshot('szx', chunk(b'JOY\0', bytes([0, 0, 0, 0, 3, 8]))))
            else:
                make_diagnostic(source)
            shutil.copyfile(source, media / 'Game Two.sna')
            if failed_load:
                (media / 'Z Broken.szx').write_bytes(b'ZXST')
            mapping = dict(directionMode=mode, directionKeys=[87, 83, 65, 68, 32], joystick=2,
                           deadZone=0.2, fastForwardToggle=toggle, actions={'0': 0, '4': TURBO, '8': PAUSE})
            game_key = hashlib.sha256(b'game one').hexdigest()
            document = json.dumps(dict(version=1, profiles={profile_name: mapping}, games={game_key: profile_name} if assigned else {}), separators=(',', ':'))
            settings = config / 'Spectacol.conf'
            settings.write_text(
                f'[General]\nfirst_start=false\nautoSaveOnExit=false\njoystick=2\ndataPath={home / "data"}\n'
                f'[Peripherals]\njoystickPrompt={str(prompt).lower()}\n'
                '[Gamepad]\ndirectionMapping=0\nfastForwardToggle=false\n'
                f'[ControllerProfiles]\ndocument={json.dumps(document)}\n'
                f'[FileBrowser]\npath={media}\n[Ui]\nmode={1 if overlay else 2}\nlarge={str(large).lower()}\n')
            target = output / (name + '.szx')
            image = output / (name + '.png')
            seconds = max(3, 2 + 0.3*len(keys))
            command = [str(args.binary.resolve()), '--machine', '48k', '--quit-after', str(seconds),
                       '--window-size', window, '--ui-keys', ','.join(keys),
                       '--ui-screenshot', str(image), '--screenshot', str(target), str(source)]
            env = os.environ | {'QT_QPA_PLATFORM': 'offscreen', 'XDG_CONFIG_HOME': str(config.parent)}
            result = subprocess.run(command, env=env, capture_output=True, text=True, timeout=seconds+20)
            (output / (name + '.log')).write_text(result.stdout+result.stderr)
            assert result.returncode == 0, (name, result.stderr)
            assert not any(x in result.stderr for x in ('ReferenceError', 'TypeError', 'Binding loop', 'Cannot assign', 'is not a type')), (name, result.stderr)
            saved = configparser.ConfigParser()
            saved.read(settings)
            # Applying a per-game profile must not overwrite the global defaults.
            assert saved.getint('Gamepad', 'directionMapping') == 0, (name, settings.read_text())
            if assigned:
                assert not saved.getboolean('Gamepad', 'fastForwardToggle'), (name, settings.read_text())
            ram, joystick = read_snapshot(target)
            if declared:
                assert ram[0x5004] == 0x0C and joystick == 3, (name, ram[0x5004], joystick)
                assert saved.getint('General', 'joystick') == 2
            document_value = saved.get('ControllerProfiles', 'document')
            if document_value.startswith(chr(34)):
                document_value = json.loads(document_value)
            if document_value.startswith('@ByteArray('):
                document_value = document_value[len('@ByteArray('):-1]
            stored_document = json.loads(document_value)
            if assigned and not switch:
                assert hashlib.sha256(name.casefold().encode()).hexdigest() in stored_document['games'], (name, stored_document)
            if name == 'profile-ui-save':
                assert stored_document['games'][game_key] == 'NEW', stored_document
            if name in ('profile-applies', 'profile-restores-default', 'profile-failed-load'):
                expected = bytearray([0x1F]*8)
                for key in ([] if switch else [0x20, 0x51, 0x70]):  # QAOP: Q, O, Space
                    expected[key >> 4] &= ~(1 << (key & 15))
                assert ram[0x5000:0x5008] == expected, (name, ram[0x5000:0x5008].hex(), expected.hex())
                assert ram[0x5010] == (0x1A if switch else 0), (name, ram[0x5010])
            if ui:
                return settings.read_text(), saved
            picture = Image.open(image).convert('RGB')
            if overlay:
                picture = picture.crop((picture.width//2, 0, picture.width, 100))
            else:
                picture = picture.crop((0, picture.height-50, picture.width, picture.height))
            ocr = output / (name + '-ocr.png')
            picture = picture.convert('L')
            if not overlay:
                picture = picture.point(lambda value: 0 if value > 75 else 255)
            picture.resize((picture.width*3, picture.height*3)).save(ocr)
            text = subprocess.check_output(['tesseract', str(ocr), 'stdout', '--psm', '11' if overlay else '7'],
                                           text=True, stderr=subprocess.DEVNULL)
            (output / (name + '.txt')).write_text(text)
            return re.sub(r'[^a-z]', '', text.lower()), saved

        direction = ['pad:AxisLeftX:-1', 'pad:AxisLeftY:-1', 'pad:ButtonA:down']
        for name, switch in [('profile-applies', False), ('profile-restores-default', True)]:
            keys = (['F6', 'Right', 'Return', 'wait', 'wait'] if switch else []) + direction
            run(name, keys, mode=1, switch=switch, prompt=not switch)
            print(name + ': real keyboard/joystick routing and default settings verified', flush=True)

        run('profile-failed-load', ['F6', 'Right', 'Right', 'Return', 'wait', 'wait'] + direction,
            mode=1, failed_load=True)
        print('profile-failed-load: a failed load preserves the current game and mapping', flush=True)
        run('snapshot-joystick-priority', direction, declared=True)
        print('snapshot-joystick-priority: metadata wins without changing the default joystick', flush=True)

        cases = [
            ('hold-active', ['pad:ButtonL1:down'], False, False, True, False),
            ('hold-released', ['pad:ButtonL1:down', 'pad:ButtonL1:up'], False, False, False, False),
            ('toggle-active', ['pad:ButtonL1:down', 'pad:ButtonL1:up'], True, False, True, False),
            ('toggle-off', ['pad:ButtonL1:down', 'pad:ButtonL1:up']*2, True, False, False, False),
            ('touch-cancelled', ['touch:ButtonL1:down', 'touch:ButtonL1:cancel'], True, False, False, False),
            ('shared-hold', ['pad:ButtonL1:down', 'touch:ButtonL1:down', 'touch:ButtonL1:cancel'], False, False, True, False),
            ('pause-clears-turbo', ['pad:ButtonL1:down', 'pad:ButtonL1:up', 'pad:ButtonSelect:down', 'pad:ButtonSelect:up'], True, False, False, True),
            ('pause-resume', ['pad:ButtonSelect:down', 'pad:ButtonSelect:up']*2, False, False, False, False),
            ('overlay-turbo', ['pad:ButtonL1:down'], False, True, True, False),
            ('overlay-pause', ['pad:ButtonSelect:down', 'pad:ButtonSelect:up'], False, True, False, True),
        ]

        def shortcut(case):
            name, keys, toggle, overlay, turbo, paused = case
            text, _ = run(name, keys, toggle=toggle, overlay=overlay)
            assert ('fastforward' in text) == turbo, (name, text)
            assert ('paused' in text) == paused, (name, text)
            return name
        with ThreadPoolExecutor(max_workers=3) as pool:
            for name in pool.map(shortcut, cases):
                print(name + ': playback indicator and shortcut lifecycle match', flush=True)

        # Navigate the same overlay menu used by gamepads, including in desktop mode.
        gamepad_menu = ['F5'] + ['Down']*7 + ['Return'] + ['Down']*5 + ['Return']
        profiles = gamepad_menu + ['Down', 'Return', 'wait', 'wait']
        # Saved profile -> Load -> Save current mapping; enter a new named profile.
        settings, _ = run('profile-ui-save', profiles + ['Down', 'Down', 'Return', 'wait', 'Ctrl+A',
                                 'N', 'E', 'W', 'Return', 'wait', 'Down', 'Left'], ui=True)
        assert 'NEW' in settings, settings
        print('profile-ui-save: controller navigation creates a persistent named profile', flush=True)
        # First row chooses hold/toggle; the next rows adjust each opacity separately.
        controls = gamepad_menu + ['Down', 'Down', 'Return', 'wait', 'wait']
        _, saved = run('opacity-ui', controls + ['Right', 'Down', 'Left', 'Down', 'Left', 'Left', 'Down', 'Left'],
                       ui=True)
        assert saved.getfloat('Controls', 'keyboardOpacity') == 0.9
        assert saved.getfloat('Controls', 'joystickOpacity') == 0.8
        assert saved.getfloat('Controls', 'buttonsOpacity') == 0.9
        print('opacity-ui: independent opacity settings persist', flush=True)
        _, saved = run('global-mode-ui', controls + ['Right'], ui=True, assigned=False, prompt=False)
        assert saved.getboolean('Gamepad', 'fastForwardToggle')
        print('global-mode-ui: the default fast-forward mode persists', flush=True)
        run('profiles-narrow-ui', profiles, ui=True, window='320x640')
        run('controls-narrow-ui', controls, ui=True, window='320x640')
        run('profiles-large-ui', profiles, ui=True, large=True)
        run('profiles-long-name-ui', profiles, ui=True, window='320x640',
            profile_name='A long controller profile name with custom directions and buttons'[:64])
        print('New settings pages load at phone width, with long profile names and Large UI', flush=True)
    print('Controls and profiles compiled UI checks passed')


if __name__ == '__main__':
    main()
