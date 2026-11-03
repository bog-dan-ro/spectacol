#!/usr/bin/env python3
"""Check snapshot joystick preferences and the compiled chooser against real machine I/O.

Copyright (c) 2015-2026, BogDan Vatra. GPL-3.0-or-later.
All media are authored here; no controller or downloaded snapshots are needed.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import configparser
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import zipfile
import zlib


def chunk(tag, body):
    return tag + struct.pack('<I', len(body)) + body


def diagnostic_ram():
    ram = bytearray(49152)
    program = bytearray(b'\xf3')
    for row in range(8):
        program += b'\x01' + struct.pack('<H', ((0xFF ^ (1 << row)) << 8) | 0xFE)
        program += b'\xed\x78\xe6\x1f\x32' + struct.pack('<H', 0x9000 + row)
    for port, target in ((0x001F, 0x9010), (0x007F, 0x9011)):
        program += b'\x01' + struct.pack('<H', port) + b'\xed\x78\x32' + struct.pack('<H', target)
    program += b'\x3e\x0e\xd3\xf5'  # AY register 14, joystick inputs
    for port, target in ((0x01F6, 0x9012), (0x02F6, 0x9013)):
        program += b'\x01' + struct.pack('<H', port) + b'\xed\x78\x32' + struct.pack('<H', target)
    program += b'\xc3\x01\x80'
    ram[0x4000:0x4000 + len(program)] = program
    struct.pack_into('<H', ram, 0xBFFC, 0x8000)
    return ram


def snapshot(extension, metadata=b'', mode=0, machine=1):
    ram = diagnostic_ram()
    if extension == 'sna':
        header = bytearray(27)
        struct.pack_into('<H', header, 23, 0xFFFC)
        header[25] = 1
        return header + ram
    if extension == 'z80':
        header = bytearray(30)
        struct.pack_into('<H', header, 6, 0x8000)
        struct.pack_into('<H', header, 8, 0xFFFE)
        header[29] = 1 | (mode << 6)
        return header + ram
    if extension == 'z80v3':
        header = bytearray(87)
        struct.pack_into('<H', header, 8, 0xFFFE)
        header[29] = 0x81
        struct.pack_into('<HH', header, 30, 55, 0x8000)
        for key, value in enumerate((0x0103, 0x0203, 0x0403, 0x0803, 0x1003)):
            struct.pack_into('<H', header, 63 + key * 2, value)
        return header + b''.join(b'\xff\xff' + bytes([page]) + ram[i*16384:(i+1)*16384]
                                  for i, page in enumerate((8, 4, 5)))
    regs = bytearray(37)
    struct.pack_into('<HH', regs, 20, 0xFFFE, 0x8000)
    regs[28] = 1
    return (b'ZXST\x01\x05' + bytes([machine, 0]) + chunk(b'Z80R', regs) + metadata
            + b''.join(chunk(b'RAMP', bytes([0, 0, page]) + ram[i*16384:(i+1)*16384])
                       for i, page in enumerate((5, 2, 0))))


def read_snapshot(path):
    data = path.read_bytes()
    result = bytearray(49152)
    joystick = None
    pos = 8
    while pos < len(data):
        tag, size = struct.unpack_from('<4sI', data, pos)
        body = data[pos + 8:pos + 8 + size]
        pos += 8 + size
        if tag == b'JOY\0':
            joystick = body[4]
        if tag == b'RAMP' and body[2] in (5, 2, 0):
            index = (5, 2, 0).index(body[2])
            page = zlib.decompress(body[3:]) if body[0] & 1 else body[3:]
            result[index*16384:(index+1)*16384] = page
    return result, joystick


def main():
    repo = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=repo / 'build/bin/spectacol')
    parser.add_argument('--output', type=Path, default=repo / 'build/joysticks')
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    joy = lambda value, other=8: chunk(b'JOY\0', bytes([0, 0, 0, 0, value, other]))
    keyboard = lambda value: chunk(b'KEYB', bytes([0, 0, 0, 0, value]))
    # Expected persisted index, SZX interface value, keyboard keys, hardware port sample.
    interfaces = {
        0: (0, 8, [], None),
        1: (1, 2, [0x34, 0x43, 0x40], None),
        2: (2, 0, [], (0x5010, 0x1A)),
        3: (3, 3, [0x44, 0x41, 0x40], None),
        4: (4, 4, [0x30, 0x33, 0x34], None),
        5: (5, 1, [], (0x5011, 0x7A)),
        6: (6, 6, [], (0x5012, 0x7A)),
        7: (7, 7, [], (0x5013, 0x7A)),
    }
    cases = []
    for index, szx in ((i, values[1]) for i, values in interfaces.items()):
        cases.append((f'szx-{index}', 'szx', joy(szx), index, False, 9 if index >= 6 else 1, True))
    cases += [
        ('keyboard-only', 'szx', keyboard(4), 4, False, 1, True),
        ('keyboard-with-disabled-players', 'szx', joy(8) + keyboard(3), 3, False, 1, True),
        ('same-type-twice', 'szx', joy(0, 0), 2, False, 1, True),
        ('missing', 'szx', b'', 3, True, 1, True),
        ('old-keyboard', 'szx', chunk(b'KEYB', bytes(4)), 3, True, 1, True),
        ('spectrum-plus-keys', 'szx', keyboard(5), 3, True, 1, True),
        ('normal-keyboard-with-joystick', 'szx', keyboard(5) + joy(0), 2, False, 1, True),
        ('unknown', 'szx', joy(99), 3, True, 1, True),
        ('programmable', 'szx', joy(5), 3, True, 1, True),
        ('ambiguous', 'szx', joy(0, 4), 3, True, 1, True),
        ('conflicting-keyboard', 'szx', joy(0) + keyboard(4), 3, True, 1, True),
        ('timex-wrong-model', 'szx', joy(6), 3, True, 1, True),
        ('sna-prompt', 'sna', b'', 3, True, 1, True),
        ('prompt-disabled', 'sna', b'', 2, False, 1, False),
        ('metadata-with-prompt-disabled', 'szx', joy(4), 4, False, 1, False),
        ('z80-cursor', 'z80', 0, 1, False, 1, True),
        ('z80-kempston', 'z80', 1, 2, False, 1, True),
        ('z80-sinclair2', 'z80', 2, 4, False, 1, True),
        ('z80-sinclair1', 'z80', 3, 3, False, 1, True),
        ('z80v3-custom-sinclair2', 'z80v3', b'', 4, False, 1, True),
        ('zip-szx', 'szx', joy(1), 5, False, 1, True),
        ('browser-sna', 'sna', b'', 3, True, 1, True),
        ('browser-szx', 'szx', joy(4), 4, False, 1, True),
    ]
    with tempfile.TemporaryDirectory(prefix='session-', dir=output) as temporary:
        session = Path(temporary)

        def run(case):
            name, extension, metadata, selected, prompt, machine, enabled = case
            home = session / name
            config = home / 'config/Licentia'
            config.mkdir(parents=True)
            initial = 2 if prompt or name == 'prompt-disabled' else 1 if selected == 2 else 2
            settings = config / 'Spectacol.conf'
            settings.write_text(f'[General]\nfirst_start=false\nautoSaveOnExit=false\njoystick={initial}\ndataPath={home / "data"}\n[Peripherals]\njoystickPrompt={str(enabled).lower()}\n')
            files = home / 'data/files'
            files.mkdir(parents=True)
            if name.startswith('browser-'):
                with settings.open('a') as config_file:
                    config_file.write(f'[FileBrowser]\npath={files}\n')
            source = files / ('game.' + ('z80' if extension == 'z80v3' else extension))
            source.write_bytes(snapshot(extension, metadata if isinstance(metadata, bytes) else b'',
                                        metadata if isinstance(metadata, int) else 0, machine))
            if name == 'zip-szx':
                archive = home / 'game.zip'
                with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as zipped:
                    zipped.write(source, source.name)
                source = archive
            target = output / (name + '.szx')
            keys = (['Down', 'Return'] if prompt else []) + ['pad:AxisLeftX:-1', 'pad:AxisLeftY:-1', 'pad:ButtonA:down']
            if name.startswith('browser-'):
                keys = ['F6', 'Return', 'wait', 'wait'] + keys
            env = os.environ | {'QT_QPA_PLATFORM': 'offscreen', 'XDG_CONFIG_HOME': str(config.parent)}
            command = [str(args.binary.resolve()), '--machine', '48k', '--quit-after', str(2 + 0.3*len(keys)),
                       '--window-size', '1280x800', '--ui-keys', ','.join(keys), '--screenshot', str(target)]
            if not name.startswith('browser-'):
                command.append(str(source))
            run = subprocess.run(command, env=env, capture_output=True, text=True, timeout=25)
            (output / (name + '.log')).write_text(run.stdout + run.stderr)
            assert run.returncode == 0, (name, run.stderr)
            assert not any(x in run.stderr for x in ('ReferenceError', 'TypeError', 'Error loading media')), (name, run.stderr)
            settings_read = configparser.ConfigParser()
            settings_read.read(settings)
            actual = settings_read.getint('General', 'joystick')
            assert actual == selected, (name, actual, selected, run.stderr)
            ram, exported_joystick = read_snapshot(target)
            _, expected_szx, pressed, hardware = interfaces[selected]
            assert exported_joystick == expected_szx, (name, exported_joystick, expected_szx)
            expected = bytearray([0x1F] * 8)
            for key in pressed:
                expected[key >> 4] &= ~(1 << (key & 15))
            assert ram[0x5000:0x5008] == expected, (name, ram[0x5000:0x5008].hex(), expected.hex())
            if hardware:
                address, expected_value = hardware
                assert ram[address] == expected_value, (name, ram[address], expected_value)
            if selected >= 6:
                other = 0x5013 if selected == 6 else 0x5012
                assert ram[other] == 0xFF, (name, 'other Timex port changed')
            return name

        with ThreadPoolExecutor(max_workers=3) as pool:
            for name in pool.map(run, cases):
                print(name + ': selection, chooser, export and real I/O match')
    print('Snapshot joystick integration checks passed')


if __name__ == '__main__':
    main()
