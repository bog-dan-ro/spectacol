#!/usr/bin/env python3
"""Compiled UI checks for trainers, ROMs, compact browsing and assembler symbols.

Copyright (c) 2015-2026, BogDan Vatra. GPL-3.0-or-later.
Uses authored media, Pillow and tesseract. Evidence stays under the build directory.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import os
from pathlib import Path
from PIL import Image, ImageOps
import subprocess

from debugger import OPEN as DEBUGGER, pc, program
from debugger_gamepad import pad
from joysticks import read_snapshot

POKES = ['F5'] + ['Down'] * 6 + ['Return'] + ['Down'] * 5 + ['Return', 'wait', 'wait']
MACHINES = ['F5'] + ['Down'] * 6 + ['Return', 'Return', 'wait', 'F2', 'wait']
BROWSER = ['F5', 'Down', 'Down', 'Return', 'wait', 'wait']
SYMBOLS = DEBUGGER + [pad('X')] + [pad('Down')] * 6 + [pad('A'), 'wait']
ERRORS = ('ReferenceError', 'TypeError', 'Binding loop', 'Cannot assign', 'is not a type',
          'Unavailable UI item', 'Required property', 'No such method', 'Unable to assign',
          'Unknown navigation', 'Unknown controller event', 'Cannot load library')


def ocr(path):
    text = subprocess.check_output(['tesseract', str(path), 'stdout', '--psm', '11'],
                                   stderr=subprocess.DEVNULL, text=True).lower()
    image = Image.open(path).convert('RGB')
    mask = Image.new('L', image.size)
    mask.putdata([0 if r > 140 and g < 130 and b < 130 else 255 for r, g, b in image.get_flattened_data()])
    red = path.with_name(path.stem + '-errors.png')
    mask.save(red)
    if 'symbols-import-jump' in path.stem:
        zoom = path.with_name(path.stem + '-annotation.png')
        image.crop((0, 240, image.width, min(360, image.height))).resize((image.width * 3, 360)).save(zoom)
        text += subprocess.check_output(['tesseract', str(zoom), 'stdout', '--psm', '11'], stderr=subprocess.DEVNULL, text=True).lower()
    return text + subprocess.check_output(['tesseract', str(red), 'stdout', '--psm', '11'],
                                         stderr=subprocess.DEVNULL, text=True).lower()


def main():
    repo = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=repo / 'build/bin/spectacol')
    parser.add_argument('--output', type=Path, default=repo / 'build/files-features')
    parser.add_argument('--case', action='append')
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    binary = args.binary.resolve()

    def run(name, keys, *, size='1280x800', trainer=False, folder=False, extra='', source=True, home=None):
        home = home or output / name
        config = home / 'config/Licentia'
        config.mkdir(parents=True, exist_ok=True)
        data = home / 'data'
        data.mkdir(parents=True, exist_ok=True)
        conf = config / 'Spectacol.conf'
        if not conf.exists():
            conf.write_text(f'[General]\nfirst_start=false\nautoSaveOnExit=false\ndataPath={data}\n'
                            f'[Peripherals]\njoystickPrompt=false\n[Ui]\nmode=2\n{extra}')
        game = data / 'Feature Game.sna'
        program(game)
        another = data / 'Another Game.sna'
        program(another)
        memory = bytearray(another.read_bytes())
        memory[27 + 0x5001] = 0x99
        another.write_bytes(memory)
        if trainer:
            (data / 'Feature Game.POK').write_text('NInfinite lives\nZ 8 36864 77 0\nNChoose energy\nZ 8 36865 256 0\nY\n')
        if folder:
            pokes = home / 'POKES'
            pokes.mkdir(exist_ok=True)
            (pokes / 'feature game.pok').write_text('NFolder trainer\nZ 8 36864 88 0\nY\n')
            with conf.open('a') as file:
                file.write(f'[Trainers]\npokesFolder={pokes}\n')
        symbols = home / 'game.sld.txt'
        symbols.write_text('|SLD.data.version|1\n'
                           'game.asm|1||0|-1|-1|Z|pages.size:16384,pages.count:4,slots.count:4\n'
                           'game.asm|42||0|2|32784|T|\n'
                           'game.asm|42||0|2|32784|L|,MainLoop,,+used\n')
        custom = home / 'custom.rom'
        custom.write_bytes(bytes([0x76]) * 16384)
        short = home / 'short.rom'
        short.write_bytes(b'1234')
        bad_symbols = home / 'invalid.sym'
        bad_symbols.write_text('not a symbol file\n')
        variables = {'SYMBOLS': symbols, 'ROM': custom, 'SHORT': short, 'BADSYMBOLS': bad_symbols}
        commands = [key.format(**variables) for key in keys]
        seconds = 2 + .3 * len(commands)
        image = output / (name + '.png')
        snapshot = output / (name + '.szx')
        command = [str(binary), '--machine', '48k', '--quit-after', str(seconds), '--window-size', size,
                   '--ui-keys', ','.join(commands), '--ui-screenshot', str(image), '--screenshot', str(snapshot)]
        if source:
            command.append(str(game))
        (output / (name + '.command.txt')).write_text('\n'.join(command))
        result = subprocess.run(command, env=os.environ | {
            'QT_QPA_PLATFORM': 'offscreen', 'XDG_CONFIG_HOME': str(config.parent),
            'XDG_DATA_HOME': str(home / 'xdg-data'), 'XDG_CACHE_HOME': str(home / 'cache')},
            capture_output=True, text=True, timeout=seconds + 25)
        (output / (name + '.log')).write_text(result.stdout + result.stderr)
        assert result.returncode == 0, (name, result.returncode, result.stderr)
        assert not any(error in result.stderr for error in ERRORS), (name, result.stderr)
        return snapshot, ocr(image), conf, home

    def scenario(name):
        if name == 'trainer-toggle':
            snapshot, text, *_ = run(name, POKES + [pad('A')], trainer=True)
            assert read_snapshot(snapshot)[0][0x5000] == 77
            assert 'infinite lives' in text, text
        elif name == 'trainer-restore':
            snapshot, *_ = run(name, POKES + [pad('A'), pad('A')], trainer=True)
            assert read_snapshot(snapshot)[0][0x5000] == 0x43
        elif name == 'trainer-value':
            snapshot, *_ = run(name, POKES + [pad('Down'), pad('A'), 'input:123', 'Return'], trainer=True)
            assert read_snapshot(snapshot)[0][0x5001] == 123
        elif name == 'trainer-value-cancel':
            snapshot, text, *_ = run(name, POKES + [pad('Down'), pad('A'), 'input:123', pad('B')], trainer=True)
            assert read_snapshot(snapshot)[0][0x5001] == 0
            assert 'trainer values' not in text, text
        elif name == 'trainer-folder':
            snapshot, text, *_ = run(name, POKES + [pad('A')], folder=True)
            assert read_snapshot(snapshot)[0][0x5000] == 88
            assert 'folder trainer' in text, text
        elif name == 'rom-invalid-size':
            _, text, *_ = run(name, MACHINES + [pad('Down'), 'Return', 'input:{SHORT}', 'Return'])
            assert '16384' in text and '4' in text and 'choose custom rom' in text, text
        elif name == 'rom-persistence':
            first, _, _, home = run(name, MACHINES + [pad('Down'), 'Return', 'input:{ROM}', 'Return', 'click:romRestart', 'wait', 'wait'])
            assert pc(first) == 0, hex(pc(first))
            second, *_ = run(name + '-restart', [], source=False, home=home)
            assert pc(second) == 0, hex(pc(second))
            _, text, *_ = run(name + '-restore', MACHINES + ['click:romRestore'], home=home)
            assert 'custom.rom' not in text, text
        elif name == 'rom-picker-back':
            _, text, *_ = run(name, MACHINES + [pad('Down'), pad('A'), 'click:hostFileBrowse', 'wait', pad('B'), 'wait'])
            assert 'choose custom rom' in text, text
        elif name == 'folder-picker-back':
            _, text, *_ = run(name, POKES + ['click:pokesFolder', 'click:hostFolderBrowse', 'wait', pad('B'), 'wait'])
            assert 'choose pokes folder' in text, text
        elif name == 'rom-back':
            _, text, *_ = run(name, MACHINES + [pad('Down'), 'Return', pad('B'), pad('B')])
            assert 'choose a machine' in text, text
        elif name == 'browser-list-search':
            _, text, *_ = run(name, BROWSER + ['F2', 'F3', 'input:feature', 'Return'])
            assert 'feature game' in text and 'another game' not in text, text
        elif name == 'browser-grid-pad':
            _, text, *_ = run(name, BROWSER + [pad('Start'), pad('Start'), pad('Y'), 'input:another', 'Return'])
            assert 'another game' in text and 'feature game' not in text, text
        elif name == 'browser-open-pad':
            snapshot, *_ = run(name, BROWSER + [pad('Start'), pad('Y'), 'input:another', 'Return', pad('A')])
            assert read_snapshot(snapshot)[0][0x5001] == 0x99
        elif name == 'browser-stick-open':
            snapshot, *_ = run(name, BROWSER + [pad('Start'), 'End', 'nav:AxisLeftY:-1', 'nav:AxisLeftY:0', pad('A')])
            assert read_snapshot(snapshot)[0][0x5001] == 0x99
        elif name == 'rom-stick':
            snapshot, *_ = run(name, MACHINES + ['nav:AxisLeftY:1', 'nav:AxisLeftY:0', pad('A'), 'input:{ROM}', 'Return', 'click:romRestart', 'wait'])
            assert pc(snapshot) == 0, hex(pc(snapshot))
        elif name == 'machine-stick':
            _, text, *_ = run(name, MACHINES[:-2] + ['nav:AxisLeftX:1', 'nav:AxisLeftX:0', pad('X')])
            assert 'custom roms' in text and 'ntsc' in text, text
        elif name == 'browser-open-touch':
            snapshot, *_ = run(name, BROWSER + ['F2', 'click:filesSearch', 'input:another', 'Return', 'point:0:down:200:217', 'point:0:up:200:217'])
            assert read_snapshot(snapshot)[0][0x5001] == 0x99
        elif name == 'browser-persistence':
            _, _, conf, home = run(name, BROWSER + ['F2', 'Down', pad('B')])
            assert '[FileSelections]' in conf.read_text(), conf.read_text()
            _, text, *_ = run(name + '-restart', BROWSER, home=home)
            assert 'list' in text and 'feature game' in text, text
        elif name == 'symbols-import-jump':
            _, text, *_ = run(name, SYMBOLS + ['click:importSymbols', 'input:{SYMBOLS}', 'Return',
                                               pad('Y'), 'input:MainLoop', 'Return', pad('A')])
            assert 'mainloop' in text and 'game.asm:42' in text.replace(' ', ''), text
        elif name == 'symbols-invalid':
            _, text, *_ = run(name, SYMBOLS + ['click:importSymbols', 'input:{BADSYMBOLS}', 'Return'])
            assert 'invalid symbol record' in text, text
        elif name == 'symbols-back':
            _, text, *_ = run(name, SYMBOLS + ['click:importSymbols', pad('B')])
            assert 'import assembler' not in text and 'labels / source mappings' in text, text
        elif name == 'browser-phone':
            _, text, *_ = run(name, BROWSER + ['F2', 'F3', 'input:feature', 'Return'], size='480x640', extra='large=true\n')
            assert 'feature game' in text, text
        elif name == 'symbols-phone':
            _, text, *_ = run(name, SYMBOLS + ['click:importSymbols'], size='480x640')
            assert 'import assembler' in text, text
        else:
            raise AssertionError(name)
        print(name + ': passed', flush=True)

    cases = ['trainer-toggle', 'trainer-restore', 'trainer-value', 'trainer-value-cancel', 'trainer-folder',
             'rom-invalid-size', 'rom-persistence', 'rom-back', 'browser-list-search', 'browser-grid-pad',
             'browser-persistence', 'symbols-import-jump', 'symbols-invalid', 'symbols-back',
             'browser-phone', 'symbols-phone', 'browser-open-pad', 'browser-open-touch', 'browser-stick-open', 'rom-stick', 'machine-stick', 'rom-picker-back', 'folder-picker-back']
    def checked(name):
        try:
            scenario(name)
            return None
        except Exception as error:
            print(f"{name}: FAILED: {error}", flush=True)
            return (name, str(error))
    with ThreadPoolExecutor(max_workers=3) as executor:
        failures = [result for result in executor.map(checked, args.case or cases) if result]
    assert not failures, failures


if __name__ == '__main__':
    main()
