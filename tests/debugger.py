#!/usr/bin/env python3
"""Exercise the compiled debugger with an authored program; no external media required.
Pillow and tesseract verify the rendered watchpoint reason.

Copyright (c) 2026, BogDan Vatra. GPL-3.0-or-later.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import os
import re
from pathlib import Path
import struct
import subprocess
import tempfile
from joysticks import read_snapshot
from PIL import Image

OPEN = ['F5'] + ['Down']*6 + ['Return'] + ['Down']*3 + ['Return', 'wait', 'wait']
TARGET = ['click:debuggerAddress', '8', '0', '1', '0', 'Return', 'F8']


def program(path):
    ram = bytearray(49152)
    ram[0x4000:0x4007] = bytes.fromhex('F3 CD1080 C30180')
    ram[0x4010:0x4019] = bytes.fromhex('3E42 CD2080 320090 C9')
    ram[0x4020:0x4022] = bytes.fromhex('3C C9')
    struct.pack_into('<H', ram, 0xBF00, 0x8000)
    header = bytearray(27)
    struct.pack_into('<H', header, 23, 0xFF00)
    header[25] = 1
    path.write_bytes(header + ram)


def pc(path):
    data = path.read_bytes()
    offset = 8
    while offset < len(data):
        tag, size = struct.unpack_from('<4sI', data, offset)
        if tag == b'Z80R':
            return struct.unpack_from('<H', data, offset+8+22)[0]
        offset += 8+size
    raise AssertionError('Snapshot has no registers')


def main():
    repo = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=repo/'build/bin/spectacol')
    parser.add_argument('--output', type=Path, default=repo/'build/debugger')
    parser.add_argument('--case', action='append', help='Run only this named scenario')
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='session-', dir=output) as temporary:
        def run(case):
            name, keys, expected, size = case
            home = Path(temporary)/name
            config = home/'config/Licentia'
            config.mkdir(parents=True)
            (config/'Spectacol.conf').write_text(
                f'[General]\nfirst_start=false\nautoSaveOnExit=false\ndataPath={home / "data"}\n'
                '[Peripherals]\njoystickPrompt=false\n[Ui]\nmode=2\n')
            source = home/'debug.sna'
            program(source)
            target = output/(name+'.szx')
            image = output/(name+'.png')
            commands = OPEN+keys
            seconds = 2+0.3*len(commands)
            result = subprocess.run(
                [str(args.binary.resolve()), '--machine', '48k', '--quit-after', str(seconds),
                 '--window-size', size, '--ui-keys', ','.join(commands), '--screenshot', str(target),
                 '--ui-screenshot', str(image), str(source)],
                env=os.environ | {'QT_QPA_PLATFORM': 'offscreen', 'XDG_CONFIG_HOME': str(config.parent)},
                capture_output=True, text=True, timeout=seconds+20)
            (output/(name+'.log')).write_text(result.stdout+result.stderr)
            assert result.returncode == 0, (name, result.stderr)
            assert not any(error in result.stderr for error in (
                'ReferenceError', 'TypeError', 'Binding loop', 'Cannot assign', 'is not a type',
                'Unavailable UI item', 'Required property', 'No such method', 'Unable to assign')), (name, result.stderr)
            if expected is not None:
                assert pc(target) == expected, (name, hex(pc(target)), hex(expected))
            if name.startswith('disassembly-address-'):
                first_rows = output/(name+'-address.png')
                Image.open(image).crop((0, 245, 1280, 650)).save(first_rows)
                text = subprocess.check_output(['tesseract', str(first_rows), 'stdout', '--psm', '11'], stderr=subprocess.DEVNULL, text=True).lower()
                address = int(name.rsplit('-', 1)[1], 16)
                addresses = re.findall(r'[0o]x([0-9a-f]{4})\b', text)
                assert addresses and int(addresses[0],16) == address, (name, text)
            if name.startswith('breakpoint-toggle-'):
                text = subprocess.check_output(['tesseract', str(image), 'stdout', '--psm', '11'], stderr=subprocess.DEVNULL, text=True).lower()
                count = len(re.findall(r'\bhits\b', text))
                expected_count = int(name.rsplit('-', 1)[1]) % 2
                assert count == expected_count, (name, count, expected_count, text)
            if name == 'memory-edit':
                ram, _ = read_snapshot(target)
                assert ram[0x5000] == 0xA5, (name,ram[0x5000])
            if name == 'memory-watch':
                status = output/(name+'-status.png')
                Image.open(image).crop((0,60,1280,130)).save(status)
                text = subprocess.check_output(['tesseract', str(status), 'stdout', '--psm', '6'], stderr=subprocess.DEVNULL, text=True).lower()
                assert 'watchpoint' in text and '9000' in text, (name,text)
            if name in ('step-out', 'memory-watch'):
                ram, _ = read_snapshot(target)
                assert ram[0x5000] == 0x43
            print(name+': passed', flush=True)

        def view(index):
            return ['F2']+['Down']*index+['Return']
        memory = view(2)+[ 'click:memoryAddress', 'click:memoryAddress', '9', '0', '0', '0', 'Return']
        cases = [
            *[('disassembly-address-'+format(address,'04x'), ['click:debuggerAddress', 'input:'+text, 'Return'], None, '1280x800')
              for address,text in ((0x0066,'0x0066'), (0x8002,'$8002'), (0xFFFF,'FFFF'), (0,'0000'))],
            ('run-to', TARGET, 0x8010, '1280x800'),
            ('step', TARGET+['F11'], 0x8012, '1280x800'),
            ('step-over', TARGET+['F11', 'F10'], 0x8015, '1280x800'),
            ('step-out', TARGET+['F11', 'F10', 'F12'], 0x8004, '1280x800'),
            ('pause-continue', TARGET+['F5', 'F5'], None, '1280x800'),
            ('memory-edit', memory+['click:memoryEdit', 'A', '5', 'Return'], None, '1280x800'),
            ('memory-watch', memory+['click:memoryWatchWrite', 'F5'], 0x8018, '1280x800'),
            ('memory', view(2), None, '1280x800'),
            ('breakpoints', view(3), None, '1280x800'),
            *[('breakpoint-toggle-'+str(presses), TARGET[:-1]+['X']*presses+view(3), None, '1280x800')
              for presses in (1, 2, 3, 10)],
            ('breakpoint-stop', TARGET[:-1]+['X', 'F5'], 0x8010, '1280x800'),
            ('next-registers', view(4), None, '1280x800'),
            ('mmu', view(5), None, '1280x800'),
            ('narrow-memory', view(2), None, '480x640'),
            ('narrow', [], None, '480x640'),
        ]
        with ThreadPoolExecutor(max_workers=3) as executor:
            list(executor.map(run, [case for case in cases if not args.case or case[0] in args.case]))


if __name__ == '__main__':
    main()
