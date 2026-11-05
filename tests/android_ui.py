#!/usr/bin/env python3
"""Android phone chrome checks using an installed debug APK, Pillow and tesseract.

Copyright (c) 2015-2026, BogDan Vatra. GPL-3.0-or-later.
Use a disposable English emulator. Restores settings and display overrides.
Each page starts in a fresh process; native rotation/Menu probes are explicit.
"""
import argparse
import csv
import configparser
import io
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import tempfile
import time
import uuid
import xml.etree.ElementTree as ET
from PIL import Image, ImageChops, ImageStat

from controller import make_diagnostic


def main():
    repo = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--adb', default=shutil.which('adb') or 'adb')
    parser.add_argument('--serial', required=True)
    parser.add_argument('--output', type=Path, default=repo / 'build/android-ui')
    parser.add_argument('--case', action='append', choices=['landscape', 'large-landscape', 'portrait', 'large-portrait'],
                        help='Default: landscape; portrait also probes native startup rotation')
    parser.add_argument('--native-input', action='store_true', help='Also probe Android Menu/D-pad/OK/Back')
    parser.add_argument('--native-only', action='store_true', help='Only the native Menu/D-pad/OK/Back probe')
    parser.add_argument('--rotation-only', action='store_true',
                        help='Only check default/saved orientation and live Options → Screen changes')
    parser.add_argument('--display-only', action='store_true',
                        help='Check fullscreen, right-aligned video in every scaling mode and the window-edge menu button')
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    package = 'eu.licentia.games.spectacol'
    token = 'ui-check-' + uuid.uuid4().hex[:12]

    def adb(*words, check=True):
        return subprocess.run([args.adb, '-s', args.serial, *words], capture_output=True,
                              check=check, timeout=45)

    def shell(*words, check=True):
        return adb('shell', shlex.join(words), check=check)

    def private(*words, check=True):
        return shell('run-as', package, *words, check=check)

    assert shell('getprop', 'ro.kernel.qemu').stdout.strip() == b'1', 'Use a disposable emulator'
    base = private('pwd').stdout.decode().strip() + '/files'
    settings = base + '/settings/Licentia/Spectacol.conf'
    data = base + '/' + token
    staging = '/data/local/tmp/' + token
    saved = private('cat', settings, check=False)
    display = {k: shell('wm', k).stdout.decode() for k in ('size', 'density')}
    rotation = shell('wm', 'get-ignore-orientation-request').stdout.decode()
    scenario = ''
    seq = 0

    def label(node):
        return node.get('text') or node.get('content-desc') or ''

    def bounds(node):
        return list(map(int, re.findall(r'\d+', node.get('bounds'))))

    def nodes():
        nonlocal seq
        shell('uiautomator', 'dump', '/sdcard/' + token + '.xml')
        raw = shell('cat', '/sdcard/' + token + '.xml').stdout
        seq += 1
        (output / f'{scenario}-{seq:03}.xml').write_bytes(raw)
        tree = list(ET.fromstring(raw).iter('node'))
        # Android shows its own one-time immersive-mode tutorial on a fresh AVD.
        if any(n.get('resource-id', '').endswith('/immersive_cling_title') for n in tree):
            confirmation = next(n for n in tree if n.get('resource-id') == 'com.android.systemui:id/ok')
            x1, y1, x2, y2 = bounds(confirmation)
            point((x1+x2)//2, (y1+y2)//2)
            return nodes()
        return tree

    def screen():
        return adb('exec-out', 'screencap', '-p').stdout

    def visual_target(text, raw):
        # Qt's accessibility cache may retain old delegate bounds after a flick.
        result = subprocess.run(['tesseract', 'stdin', 'stdout', '--psm', '11', 'tsv'],
                                input=raw, capture_output=True, check=True, timeout=20)
        lines = {}
        for word in csv.DictReader(io.StringIO(result.stdout.decode()), delimiter='\t'):
            if word['text'].strip() and float(word['conf']) >= 30:
                group = tuple(word[k] for k in ('page_num', 'block_num', 'par_num', 'line_num'))
                lines.setdefault(group, []).append(word)
        normalize = lambda value: re.sub(r'[^a-z0-9]', '', value.lower())
        for line in lines.values():
            if normalize(' '.join(w['text'] for w in line)) == normalize(text):
                x1 = min(int(w['left']) for w in line)
                y1 = min(int(w['top']) for w in line)
                x2 = max(int(w['left']) + int(w['width']) for w in line)
                y2 = max(int(w['top']) + int(w['height']) for w in line)
                return ((x1 + x2) // 2, (y1 + y2) // 2)
        return None

    def point(x, y):
        shell('input', 'tap', str(x), str(y))
        time.sleep(.4)

    def swipe(x=.7):
        width, height = Image.open(io.BytesIO(screen())).size
        shell('input', 'swipe', str(int(width*x)), str(int(height*.8)),
              str(int(width*x)), str(int(height*.35)), '350')
        time.sleep(.4)

    def tap(text, scroll=False):
        for _ in range(5 if scroll else 2):
            raw = screen()
            width, height = Image.open(io.BytesIO(raw)).size
            for node in nodes():
                if label(node) == text:
                    x1, y1, x2, y2 = bounds(node)
                    if 0 <= x1 < x2 <= width and 0 <= y1 < y2 <= height:
                        point((x1+x2)//2, (y1+y2)//2)
                        return
            target = visual_target(text, raw)
            if target:
                point(*target)
                return
            if scroll:
                swipe(.85)
        raise AssertionError((scenario, 'missing touch target', text))

    def key(code):
        shell('input', 'keyevent', str(code))
        time.sleep(.5)

    def capture(name, expected=()):
        tree = nodes()
        raw = screen()
        (output / f'{scenario}-{name}.png').write_bytes(raw)
        labels = [label(n) for n in tree]
        for text in expected:
            assert text in labels or visual_target(text, raw), (scenario, name, text, labels)
        assert not any(re.fullmatch(r'(F\d+|Esc|Enter|Del|Home|Tab)', text) for text in labels), labels
        print(f'{scenario}: {name}', flush=True)
        return tree

    def start(name, keys):
        shell('am', 'force-stop', package)
        words = ['--machine', '48k', '--quit-after', '90']
        if keys:
            words += ['--ui-keys', ','.join(keys)]
        words += [data + '/AndroidUI.sna']
        # NEW_TASK | CLEAR_TASK also removes a document picker left by a failed run.
        result = shell('am', 'start', '-W', '-f', '0x10008000', '-n', package + '/org.qtproject.qt.android.bindings.QtActivity',
                       '--es', 'applicationArguments', shlex.join(words))
        (output / f'{scenario}-{name}-start.log').write_bytes(result.stdout + result.stderr)
        time.sleep(2 + .3*len(keys))

    def outside_menu():
        width, _ = Image.open(io.BytesIO(screen())).size
        # Large text can make the menu fill the body; the header stays outside it.
        header = bounds(next(n for n in nodes() if label(n) == 'Page actions'))
        point(int(width*.4), (header[1]+header[3])//2)

    def native_check():
        start('native-input', [])
        key(82)
        capture('native-menu', ['Spectacol', 'Quick Open'])
        # Android may consume the first D-pad event when leaving touch mode.
        # Follow the focused visible item rather than assuming a fixed key count.
        for _ in range(4):
            tree = nodes()
            if any(label(n) == 'Quick Open' and n.get('focused') == 'true' for n in tree):
                break
            key(20)
        else:
            raise AssertionError('D-pad did not reach Quick Open')
        capture('native-selection')
        key(23)
        capture('native-open-files', ['Covers', 'Page actions'])
        key(4)
        capture('native-back', ['='])

    def configure(temp, large=False, orientation=None, fill_mode=0):
        shell('am', 'force-stop', package)
        config = temp/'Spectacol.conf'
        config.write_text(f'[General]\nfirst_start=false\nautoSaveOnExit=false\ndataPath={data}\n'
                          f'[Peripherals]\njoystickPrompt=false\n[Ui]\nmode=0\nlarge={str(large).lower()}\n'
                          + ('' if orientation is None else f'[Screen]\nleftMargin=true\norientation={orientation}\nfillMode={fill_mode}\n'))
        adb('push', str(config), staging)
        private('cp', staging, settings)

    def check_orientation(name, value, portrait=None):
        tree = capture(name)
        width, height = Image.open(io.BytesIO(screen())).size
        if portrait is not None:
            assert (height > width) == portrait, 'Native display orientation differs from requested profile'
        surfaces = [bounds(n) for n in tree if n.get('class') == 'android.view.SurfaceView']
        assert any(abs((b[2]-b[0])-width) < 4 and abs((b[3]-b[1])-height) < 4 for b in surfaces), \
            'Qt surface differs from native display after rotation (see screenshots/XML)'
        config = configparser.ConfigParser()
        config.read_string(private('cat', settings).stdout.decode())
        assert config.getint('Screen', 'orientation') == value, (name, dict(config['Screen']))

    def rotation_check(temp):
        nonlocal scenario
        scenario = 'rotation'
        options = ['F5']+['Down']*7+['Return', 'Down', 'Return']
        for name, value in [('default', None), ('landscape', 1), ('portrait', 2)]:
            configure(temp, orientation=value)
            start(name, options if value is not None else [])
            check_orientation('cold-'+name, value or 0, None if value is None else value == 2)
        # Use the owner's existing selector, including reopening it after each resize.
        for previous, selection, value in [('Portrait', 'Landscape', 1),
                                            ('Landscape', 'Portrait', 2),
                                            ('Portrait', 'Any', 0)]:
            tap(previous, scroll=True)
            tap(selection)
            time.sleep(1)
            check_orientation('live-'+selection.lower(), value, None if value == 0 else value == 2)

    try:
        with tempfile.TemporaryDirectory(prefix=token) as temporary:
            temp = Path(temporary)
            fixture = temp/'AndroidUI.sna'
            make_diagnostic(fixture)
            shell('am', 'force-stop', package)
            private('mkdir', '-p', data, str(Path(settings).parent))
            adb('push', str(fixture), staging)
            private('cp', staging, data+'/AndroidUI.sna')
            shell('wm', 'size', '1080x2400')
            shell('wm', 'density', '480')
            shell('wm', 'set-ignore-orientation-request', 'false')
            if args.rotation_only:
                rotation_check(temp)
            for scenario in ([] if args.rotation_only else args.case or ['landscape']):
                large = scenario.startswith('large-')
                portrait = scenario.endswith('portrait')
                configure(temp, large, 2 if portrait else 1)
                if args.display_only:
                    for fill_mode in range(3):
                        configure(temp, large, 2 if portrait else 1, fill_mode)
                        start(f'display-{fill_mode}', [])
                        tree = capture(f'display-{fill_mode}', ['='])
                        image = Image.open(io.BytesIO(screen())).convert('RGB')
                        # The authored snapshot has a white border. It must reach the
                        # usable window's right edge for fit, integer and stretch modes.
                        pixel = image.getpixel((image.width-2, image.height//4))
                        assert min(pixel) >= 160 and max(pixel)-min(pixel) <= 2, \
                            ('ZX image is inset from the right edge', fill_mode, pixel)
                        if not portrait:
                            control = bounds(next(n for n in tree if label(n) == '='))
                            assert control[0] == 0, control
                        if fill_mode == 0:
                            tap('=')
                            capture('edge-menu-opened', ['Spectacol', 'Quick Open'])
                            tap('Close menu')
                            capture('edge-menu-closed', ['='])
                    continue
                if args.native_only:
                    native_check()
                    continue
                start('drawer', ['F5'])
                tree = capture('drawer', ['Spectacol', 'Quick Open'])
                width, height = Image.open(io.BytesIO(screen())).size
                assert (height > width) == portrait, 'Native display orientation differs from requested profile'
                surfaces = [bounds(n) for n in tree if n.get('class') == 'android.view.SurfaceView']
                assert any(abs((b[2]-b[0])-width) < 4 and abs((b[3]-b[1])-height) < 4 for b in surfaces), \
                    'Qt surface differs from native display after rotation (see screenshots/XML)'
                tap('Close menu')
                tree = capture('drawer-closed', ['='])
                if not portrait:
                    for text in ['=', 'Sticky CS/SS']:
                        control = bounds(next(n for n in tree if label(n) == text))
                        assert control[0] == 0, ('Control rail is inset from window edge', text, control)
                windows = shell('dumpsys', 'window', 'windows').stdout.decode()
                (output/f'{scenario}-windows.txt').write_text(windows)
                status = re.search(r'Window #[^\n]* u0 StatusBar\}:.*?(?=\n  Window #|\Z)', windows, re.S)
                assert status and 'isVisible=false' in status.group(), 'Android status bar is visible'

                start('about', ['F5']+['Down']*8+['Return', 'Return'])
                tree = capture('about', ['Spectacol', 'Gamepad in games'])
                back = bounds(next(n for n in tree if label(n) == 'Back'))
                assert abs((back[3]-back[1]) - (216 if large else 144)) <= 2, back
                before = Image.open(io.BytesIO(screen())).convert('RGB')
                swipe()
                capture('about-scrolled')
                after = Image.open(io.BytesIO(screen())).convert('RGB')
                box = (0, min(450, height//2), width, height-100)
                assert max(ImageStat.Stat(ImageChops.difference(before.crop(box), after.crop(box))).mean) > 2, 'Help did not scroll'
                tap('Gamepad in games')
                swipe()
                capture('gamepad-help')
                tap('Back')
                capture('about-closed', ['='])

                start('files', ['F6'])
                capture('files', ['Page actions', 'Covers'])
                tap('Page actions')
                capture('file-actions', ['Copy', 'Remove'])
                outside_menu()
                capture('file-actions-closed', ['Page actions', 'Covers'])
                tap('Page actions')
                tap('Device files…', scroll=True)
                capture('native-picker')
                key(4)
                capture('picker-back', ['Page actions', 'Covers'])
                tap('Close files')
                capture('files-closed', ['='])

                start('machines', ['F5']+['Down']*6+['Return', 'Return'])
                capture('machines', ['Choose a machine', 'Page actions'])
                tap('Page actions')
                tap('ROMs')
                capture('roms', ['Custom ROMs'])
                tap('Back')
                capture('roms-back', ['Choose a machine'])
                tap('Back')
                capture('machines-closed', ['='])

                start('online', ['F5']+['Down']*3+['Return'])
                capture('online', ['Search online', 'Search'])
                if b'mInputShown=true' in shell('dumpsys', 'input_method').stdout:
                    key(4)
                    capture('online-keyboard-back', ['Search online'])
                tap('Page actions')
                outside_menu()
                capture('online-actions-closed', ['Search online'])
                tap('Back')
                capture('online-closed', ['='])
                if args.native_input:
                    native_check()
        print('Android chrome checks passed', flush=True)
    finally:
        (output/'logcat.txt').write_bytes(adb('logcat', '-d', '-t', '6000', check=False).stdout)
        (output/'last-screen.png').write_bytes(adb('exec-out', 'screencap', '-p', check=False).stdout)
        shell('am', 'force-stop', package, check=False)
        if saved.returncode == 0:
            with tempfile.TemporaryDirectory() as temporary:
                backup = Path(temporary)/'settings.conf'; backup.write_bytes(saved.stdout)
                adb('push', str(backup), staging); private('cp', staging, settings)
        else:
            private('rm', '-f', settings, check=False)
        private('rm', '-rf', data, check=False)
        shell('rm', '-f', staging, '/sdcard/'+token+'.xml', check=False)
        shell('wm', 'set-ignore-orientation-request', 'true' if 'true' in rotation.lower() else 'false', check=False)
        for kind, original in display.items():
            override = re.search(r'Override (?:size|density): (\S+)', original)
            shell('wm', kind, override.group(1) if override else 'reset', check=False)


if __name__ == '__main__':
    main()
