#!/usr/bin/env python3
"""Check RZX progress, stop and desynchronization in the compiled UI.

Copyright (c) 2015-2026, BogDan Vatra. GPL-3.0-or-later.
Requires Pillow and tesseract for reading the rendered status (no downloaded recordings).
"""
import argparse
import os
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import zlib

from PIL import Image, ImageOps


def recording(frames, broken=False):
    def block(kind, data):
        return struct.pack('<BI', kind, len(data) + 5) + data

    # Authored 48K SNA, with PC on the stack: IN A,(31); JP 32768. No ROM bytes.
    header = bytearray(27)
    struct.pack_into('<H', header, 23, 0xfffc)
    ram = bytearray(49152)
    ram[0x4000:0x4005] = bytes.fromhex('db1fc30080')
    struct.pack_into('<H', ram, 0xbffc, 0x8000)
    snapshot = header + ram
    inputs = struct.pack('<HH', 12000, 5999 if broken else 6000) + b'\xff' * (5999 if broken else 6000)
    inputs += struct.pack('<HH', 12000, 65535) * (frames - 1)
    return (b'RZX!\0\r' + bytes(4)
            + block(0x10, struct.pack('<20sHH', b'Spectacol UI test', 1, 0))
            + block(0x30, struct.pack('<I4sI', 2, b'sna', len(snapshot)) + zlib.compress(snapshot))
            + block(0x80, struct.pack('<IBII', frames, 0, 0, 2) + zlib.compress(inputs)))


def main():
    repo = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=repo / 'build/bin/spectacol')
    parser.add_argument('--output', type=Path, default=repo / 'build/rzx')
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='session-', dir=output) as temporary:
        session = Path(temporary)
        config = session / 'config/Licentia'
        config.mkdir(parents=True)
        env = os.environ | {'QT_QPA_PLATFORM': 'offscreen', 'XDG_CONFIG_HOME': str(config.parent),
                            'XDG_DATA_HOME': str(session / 'data')}

        def run(name, frames=500, broken=False, keys='', overlay=False, expected='', live=False, panel=False):
            (config / 'Spectacol.conf').write_text(
                '[General]\nfirst_start=false\nautoSaveOnExit=false\ndataPath=' + str(session / 'data')
                + '\n[Peripherals]\njoystickPrompt=false\n[Ui]\nmode=' + ('1' if overlay else '2') + '\n')
            source = output / (name + '.rzx')
            raw = recording(frames, broken)
            if live:
                source = output / (name + '.sna')
                size = struct.unpack_from('<I', raw, 40)[0]
                raw = zlib.decompress(raw[56:39 + size])
            source.write_bytes(raw)
            screenshot = output / (name + '.png')
            command = [str(args.binary.resolve()), '--machine', '48k' if live else '128k', '--quit-after', str(max(3, 2 + 0.3 * len(keys.split(',')))),
                       '--window-size', '1280x800', '--ui-screenshot', str(screenshot), str(source)]
            if keys:
                command += ['--ui-keys', keys]
            with (output / (name + '.log')).open('w') as log:
                subprocess.run(command, env=env, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=20)
            log = (output / (name + '.log')).read_text()
            assert not any(s in log for s in ('ReferenceError', 'TypeError', 'Unknown key', 'Cannot assign')), log
            with Image.open(screenshot) as image:
                box = (0, 0, image.width, image.height) if panel else (0, 0, image.width, 160) if overlay else (0, image.height - 50, image.width, image.height)
                status = ImageOps.invert(image.convert('RGB').crop(box)).convert('L')
                status = status.resize((status.width * 3, status.height * 3))
                status = status.point(lambda value: 0 if value < 165 else 255)
                ocr = output / (name + '-status.png')
                status.save(ocr)
            text = subprocess.check_output(['tesseract', str(ocr), 'stdout', '--psm', '11' if overlay or panel else '7'],
                                           text=True, stderr=subprocess.DEVNULL)
            (output / (name + '.txt')).write_text(text)
            assert re.sub(r'\s+', '', expected).lower() in re.sub(r'\s+', '', text).lower(), (name, text)
            if broken:
                assert 'input overrun' in log and 'Disassembly' not in text, (log, text)
            if name == 'complete':
                assert re.search(r'5\s*/\s*5\s*frames', text, re.I), text
            print(name + ': rendered status verified', flush=True)
            return text

        run('playing', expected='RZX:')
        run('complete', frames=5, expected='RZX complete:')
        run('stopped', keys='click:50:24,Down,Down,Down,Return', expected='RZX stopped:')
        run('error', broken=True, expected='RZX error:')
        run('overlay', overlay=True, expected='RZX:')
        def cursor(text):
            match = re.search(r'RZX:\s*(\d+)\s*/', text, re.I)
            assert match, text
            return int(match[1])
        text = run('seek-pauses', keys='F8', expected='Press a game key')
        assert cursor(text) == 0, text
        text = run('modifier-stays-paused', keys='F8,down:Shift', expected='Press a game key')
        assert cursor(text) == 0, text
        text = run('key-resumes', keys='F8,wait,A', expected='RZX:')
        assert cursor(text) > 0 and 'pressagamekey' not in re.sub(r'\s+', '', text).lower(), text
        text = run('noise-stays-paused', keys='F8,pad:AxisLeftX:0.1', expected='Press a game key')
        assert cursor(text) == 0, text
        text = run('held-stick-stays-paused', keys='pad:AxisLeftX:0.8,F8,pad:AxisLeftX:0.9', expected='Press a game key')
        assert cursor(text) == 0, text
        text = run('stick-resumes', keys='pad:AxisLeftX:0.8,F8,pad:AxisLeftX:0,pad:AxisLeftX:0.8', expected='RZX:')
        assert cursor(text) > 0 and 'pressagamekey' not in re.sub(r'\s+', '', text).lower(), text
        text = run('button-resumes', keys='F8,pad:ButtonA:down,pad:ButtonA:up', expected='RZX:')
        assert cursor(text) > 0 and 'pressagamekey' not in re.sub(r'\s+', '', text).lower(), text
        text = run('held-button-stays-paused', keys='pad:ButtonA:down,F8,pad:ButtonA:down', expected='Press a game key')
        assert cursor(text) == 0, text
        text = run('fast-forward-pauses', keys='F8,F9', expected='Press a game key')
        assert 240 <= cursor(text) <= 260, text
        text = run('seek-end', keys='F8,Shift+F9', expected='RZX complete:')
        assert cursor(text.replace('RZX complete:', 'RZX:')) == 500, text
        controls = 'click:140:24,Down,Down,Down,Right,Down,Return'
        run('controls-page', keys=controls, panel=True, expected='Playback and recording')
        # At the origin, keyboard navigation skips the disabled Rewind row.
        text = run('controls-skip-disabled', keys='F8,'+controls+',Down,Down,Return', expected='Press a game key')
        assert 240 <= cursor(text) <= 260, text
        # Keep the legacy start menu route stable, then branch through fresh input.
        start = 'click:140:24,Down,Down,Down,Right,Down,Return'
        text = run('recording-seek', live=True, keys=start+',wait,wait,wait,F8', expected='Press a game key')
        assert re.search(r'RZX recording:\s*0\s*/', text, re.I), text
        text = run('recording-resumes', live=True, keys=start+',wait,wait,wait,F8,wait,A', expected='RZX recording:')
        assert 'pressagamekey' not in re.sub(r'\s+', '', text).lower(), text
        assert re.search(r'RZX recording:\s*[1-9]\d* frames', text, re.I), text
        stop = 'click:50:24,Down,Down,Down,Return'
        before = set((session / 'data').rglob('*.rzx'))
        run('recording-stop-at-origin', live=True, keys=start+',wait,wait,wait,F8,wait,'+stop, expected='RZX stopped:')
        files = set((session / 'data').rglob('*.rzx')) - before
        assert len(files) == 1, files
        raw = next(iter(files)).read_bytes()
        position = 10
        total = 0
        while position < len(raw):
            kind, size = struct.unpack_from('<BI', raw, position)
            if kind == 0x80: total += struct.unpack_from('<I', raw, position + 5)[0]
            position += size
        assert total == 0 and position == len(raw), total
    print('RZX compiled UI checks passed')


if __name__ == '__main__':
    main()
