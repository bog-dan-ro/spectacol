#!/usr/bin/env python3
"""Exercise the debugger through Qt Gamepad signals, with standard fields and simulated OS input-method commits.

Copyright (c) 2026, BogDan Vatra. GPL-3.0-or-later.
Requires Pillow and tesseract for checking the rendered view and pause reasons.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import os
from pathlib import Path
import subprocess
import tempfile
from PIL import Image
from debugger import OPEN, pc, program
from joysticks import read_snapshot


def pad(name):
    return 'nav:Button'+name


def main():
    repo=Path(__file__).resolve().parents[1]
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary',type=Path,default=repo/'build/bin/spectacol')
    parser.add_argument('--output',type=Path,default=repo/'build/debugger-gamepad')
    parser.add_argument('--case', action='append', help='Run only this named scenario')
    args=parser.parse_args();output=args.output.resolve();output.mkdir(parents=True,exist_ok=True)
    # Navigation uses real manager signals. input: commits like an OS keyboard;
    # Return represents its Enter/Done key. No controller text-entry widget.
    memory=[pad('X'),pad('Down'),pad('Down'),pad('A')]
    address=[pad('Y'),'input:9000','Return']
    editor=[pad('Y'),pad('Down'),pad('Down'),pad('A')]
    with tempfile.TemporaryDirectory(prefix='session-',dir=output) as temporary:
        def run(case):
            name,keys,expect,screen=case
            home=Path(temporary)/name;config=home/'config/Licentia';config.mkdir(parents=True)
            (config/'Spectacol.conf').write_text(f'[General]\nfirst_start=false\nautoSaveOnExit=false\ndataPath={home / "data"}\n[Peripherals]\njoystickPrompt=false\n[Ui]\nmode=2\n')
            source=home/'debug.sna';program(source)
            target=output/(name+'.szx');image=output/(name+'.png');commands=OPEN+keys
            seconds=2+0.3*len(commands)
            result=subprocess.run([str(args.binary.resolve()),'--machine','48k','--quit-after',str(seconds),'--window-size',screen,
                '--ui-keys',','.join(commands),'--screenshot',str(target),'--ui-screenshot',str(image),str(source)],
                env=os.environ|{'QT_QPA_PLATFORM':'offscreen','XDG_CONFIG_HOME':str(config.parent)},capture_output=True,text=True,timeout=seconds+20)
            (output/(name+'.log')).write_text(result.stdout+result.stderr)
            assert result.returncode==0,(name,result.stderr)
            assert not any(error in result.stderr for error in ('ReferenceError','TypeError','Binding loop','Cannot assign','is not a type','Required property','No such method','Unable to assign','Unknown navigation')),(name,result.stderr)
            ram,_=read_snapshot(target)
            if name in ('poke-zero','poke-zero-phone','poke-max','cancel-value'):
                assert ram[0x5000]==(0 if name.startswith('poke-zero') else 255 if name=='poke-max' else 0x43),(name,ram[0x5000])
            if name=='asm-cancel': assert ram[0x4004]==0xC3,(name,ram[0x4004])
            if name in ('watch-form','watch-control'): assert pc(target)==0x8018,(name,hex(pc(target)))
            if name=='step-over-out': assert pc(target)==0x8004,(name,hex(pc(target)))
            status=output/(name+'-ocr.png')
            Image.open(image).save(status)
            text=subprocess.check_output(['tesseract',str(status),'stdout','--psm','11'],stderr=subprocess.DEVNULL,text=True).lower()
            if expect: assert expect in text,(name,text)
            print(name+': passed',flush=True)
        at8010=[pad('Y'),'input:8010','Return','F8']
        cases=[
            ('asm-cancel',[pad('A'),'input:NOP',pad('B')],'disassembly','1280x800'),
            ('watch-form',[pad('X')]+[pad('Down')]*3+[pad('A'),pad('Down'),pad('A'),pad('A'),pad('Down'),pad('Down'),pad('A'),pad('Down'),'input:9000']+[pad('Down')]*3+[pad('A'),'wait',pad('R1')],'watchpoint','1280x800'),
            ('watch-control',memory+address+[pad('Y')]+[pad('Down')]*4+[pad('A'),'wait',pad('R1')],'watchpoint','1280x800'),
            ('views-memory',memory,'cpu address space','1280x800'),
            ('poke-zero',memory+address+editor+['input:00','Return'],'memory','1280x800'),
            ('poke-zero-phone',memory+address+editor+['input:00','Return'],'cpu address space','480x640'),
            ('poke-max',memory+address+editor+['input:FF','Return'],'cpu address space','1280x800'),
            ('number-editor',memory+address+editor+[pad('A')],'edit byte at $9000','1280x800'),
            ('cancel-value',memory+address+editor+['input:00',pad('B')],'memory','1280x800'),
            ('back-button',memory+address+editor+[pad('Select')],'cpu address space','1280x800'),
            ('restore-focus',memory+address+editor+[pad('B'),pad('A')],'edit byte at $9000','1280x800'),
            ('step-over-out',at8010+[pad('L1'),pad('R2'),pad('L2')],'step complete','1280x800'),
            ('stick-views', [pad('X'),'nav:AxisLeftY:1','wait','wait','wait','nav:AxisLeftY:0',pad('A')],'labels / source mappings','1280x800'),
        ]
        with ThreadPoolExecutor(max_workers=3) as executor: list(executor.map(run,[case for case in cases if not args.case or case[0] in args.case]))


if __name__=='__main__': main()
