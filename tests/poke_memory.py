#!/usr/bin/env python3
"""Check writable bank selection and pokes through standard forms and controller navigation.

Copyright (c) 2026, BogDan Vatra. GPL-3.0-or-later.
Pillow and tesseract check the bank picker. All media are authored locally.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import os
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import zlib
from PIL import Image
from debugger import program
from debugger_gamepad import pad
from joysticks import snapshot,read_snapshot

OPEN=['F5']+['Down']*6+['Return']+['Down']*5+['Return','wait','wait']


def ram_page(path,page):
    data=path.read_bytes();pos=8
    while pos<len(data):
        tag,size=struct.unpack_from('<4sI',data,pos);body=data[pos+8:pos+8+size];pos+=8+size
        if tag==b'RAMP' and body[2]==page:return zlib.decompress(body[3:]) if body[0]&1 else body[3:]
    raise AssertionError(('Missing page',page))


def main():
    repo=Path(__file__).resolve().parents[1];parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary',type=Path,default=repo/'build/bin/spectacol')
    parser.add_argument('--output',type=Path,default=repo/'build/poke-memory')
    parser.add_argument('--case', action='append', help='Run only this named scenario')
    args=parser.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    cpu=[pad('Y'),pad('Down'),'input:36864',pad('Down'),'input:255',pad('Down'),pad('A')]
    physical=[pad('Y'),pad('A'),pad('Down'),pad('A'),pad('Down'),'input:65536',pad('Down'),'input:255',pad('Down'),pad('A')]
    toggle=[pad('Down'),pad('Down'),pad('Down'),pad('A')]
    with tempfile.TemporaryDirectory(prefix='session-',dir=out) as temporary:
        def run(case):
            name,keys,machine=case;home=Path(temporary)/name;config=home/'config/Licentia';config.mkdir(parents=True)
            (config/'Spectacol.conf').write_text(f'[General]\nfirst_start=false\nautoSaveOnExit=false\ndataPath={home / "data"}\n[Peripherals]\njoystickPrompt=false\n[Ui]\nmode=2\n')
            source=home/'game.sna';program(source)
            if machine=='128k':source=source.with_suffix('.szx');source.write_bytes(snapshot('szx',machine=2))
            target=out/(name+'.szx');image=out/(name+'.png');commands=OPEN+keys;seconds=2+.3*len(commands)
            result=subprocess.run([str(args.binary.resolve()),'--machine',machine,'--quit-after',str(seconds),'--window-size','1280x800',
                '--ui-keys',','.join(commands),'--screenshot',str(target),'--ui-screenshot',str(image),str(source)],
                env=os.environ|{'QT_QPA_PLATFORM':'offscreen','XDG_CONFIG_HOME':str(config.parent)},capture_output=True,text=True,timeout=seconds+20)
            (out/(name+'.log')).write_text(result.stdout+result.stderr)
            assert result.returncode==0,(name,result.returncode,result.stderr)
            assert not any(error in result.stderr for error in ('ReferenceError','TypeError','Binding loop','Cannot assign','is not a type','Required property','No such method','Unable to assign','Unknown navigation')),(name,result.stderr)
            if name in ('cpu-poke','cpu-toggle-off','cpu-toggle-on','cancel-poke'):
                ram,_=read_snapshot(target);expected=0x43 if name in ('cpu-toggle-off','cancel-poke') else 255
                assert ram[0x5000]==expected,(name,ram[0x5000],expected)
            if name=='cpu-rom-protected':
                ram,_=read_snapshot(target);assert ram[0x5000]==0x43
                text=subprocess.check_output(['tesseract',str(image),'stdout','--psm','11'],stderr=subprocess.DEVNULL,text=True).lower()
                assert 'read-only' in result.stderr and 'add poke' in text,(text,result.stderr)
                pixels=Image.open(image).convert('RGB').tobytes()
                assert sum(1 for r,g,b in zip(pixels[0::3],pixels[1::3],pixels[2::3]) if r>180 and g<140 and b<140)>100
            if name in ('nested-back','restore-focus','invalid-byte'):
                text=subprocess.check_output(['tesseract',str(image),'stdout','--psm','11'],stderr=subprocess.DEVNULL,text=True).lower()
                assert 'add poke' in text,(name,text)
                ram,_=read_snapshot(target);assert ram[0x5000]==0x43,(name,ram[0x5000])
            if name=='physical-128k':assert ram_page(target,4)[0]==255
            if name=='bank-list':
                text=subprocess.check_output(['tesseract',str(image),'stdout','--psm','11'],stderr=subprocess.DEVNULL,text=True).lower()
                assert 'ram' in text and not re.search(r'\brom\b',text),text
            print(name+': passed',flush=True)
        cases=[
            ('cpu-rom-protected',[pad('Y'),pad('Down'),'input:0',pad('Down'),'input:255',pad('Down'),pad('A')],'48k'),
            ('bank-list',[pad('Y'),pad('A'),pad('Down'),pad('A')],'48k'),
            ('cpu-poke',cpu,'48k'),
            ('cpu-toggle-off',cpu+toggle,'48k'),
            ('cpu-toggle-on',cpu+toggle+[pad('A')],'48k'),
            ('physical-128k',physical,'128k'),
            ('cancel-poke',cpu[:-1]+[pad('Select')],'48k'),
            ('nested-back',[pad('Y'),pad('A'),pad('Select')],'48k'),
            ('restore-focus',[pad('Y'),pad('Select'),pad('A')],'48k'),
            ('invalid-byte',cpu[:-3]+['input:256','Return'],'48k'),
        ]
        with ThreadPoolExecutor(max_workers=3) as executor:list(executor.map(run,[case for case in cases if not args.case or case[0] in args.case]))


if __name__=='__main__':main()
