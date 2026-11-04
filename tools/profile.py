#!/usr/bin/env python3
"""Measure warmed Spectacol resource use on Linux; raw data stays in the build tree.
Copyright (c) 2015-2026, BogDan Vatra. GPL-3.0-or-later.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import time


def snapshot(pid):
    root = Path('/proc') / str(pid)
    threads = {}
    for task in (root / 'task').iterdir():
        try:
            fields = (task / 'stat').read_text().rsplit(')', 1)[1].split()
            status = dict(line.split(':', 1) for line in (task / 'status').read_text().splitlines() if ':' in line)
            threads[task.name] = dict(name=status['Name'].strip(), ticks=int(fields[11]) + int(fields[12]),
                                     voluntary=int(status['voluntary_ctxt_switches']),
                                     involuntary=int(status['nonvoluntary_ctxt_switches']))
        except FileNotFoundError:
            pass
    memory = dict(line.split(':', 1) for line in (root / 'smaps_rollup').read_text().splitlines() if ':' in line)
    stat = (root / 'stat').read_text().rsplit(')', 1)[1].split()
    return dict(time=time.monotonic(), ticks=int(stat[11]) + int(stat[12]), threads=threads,
                rss_kib=int(memory['Rss'].split()[0]), pss_kib=int(memory['Pss'].split()[0]))


def main():
    repo = Path(__file__).resolve().parents[1]
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary', type=Path, default=repo / 'build-production/bin/spectacol')
    p.add_argument('--output', type=Path, default=repo / 'build/profiling/run')
    p.add_argument('--media', type=Path)
    p.add_argument('--machine', default='48k', help='Machine ID passed explicitly to Spectacol')
    p.add_argument('--filter', default='normal')
    p.add_argument('--state', choices=['gameplay', 'menu', 'background'], default='gameplay')
    p.add_argument('--seconds', type=float, default=15)
    p.add_argument('--warmup', type=float, default=5)
    p.add_argument('--historical', action='store_true')
    p.add_argument('--software', action='store_true')
    p.add_argument('--sink', default='spectacol_profile', help='PULSE_SINK hint; native PipeWire may use the system default instead')
    p.add_argument('--prefix', nargs=argparse.REMAINDER, default=[])
    argv = sys.argv[1:]
    prefix_index = argv.index('--prefix') if '--prefix' in argv else len(argv)
    args = p.parse_args(argv[:prefix_index])
    args.prefix = argv[prefix_index + 1:] if prefix_index < len(argv) else []
    if args.seconds <= 0 or args.warmup < 1:
        p.error("seconds must be positive and warmup at least one second")
    if args.historical and args.state == 'menu':
        p.error("menu automation needs a current binary; historical builds lack --ui-keys")
    if args.software and args.state == 'background':
        p.error("background measurements require a window on a real display")
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    config = out / 'config/Licentia'
    config.mkdir(parents=True, exist_ok=True)
    (config / 'Spectacol.conf').write_text(
        '[General]\nfirst_start=false\nautoSaveOnExit=false\nemulationSpeed=100\n'
        f'dataPath={out}/data/\n[Screen]\nsoftwareFilter={args.filter}\n'
        'borderLeft=32\nborderRight=32\nborderTop=32\nborderBottom=32\n'
        '[Sound]\nsoundEnabled=true\nspeakerType=Unfiltered\n'
        '[Peripherals]\njoystickPrompt=false\n')
    env = os.environ | dict(XDG_CONFIG_HOME=str(config.parent), XDG_DATA_HOME=str(out / 'data'), XDG_CACHE_HOME=str(out / 'cache'),
                            QT_QPA_PLATFORM='offscreen' if args.software else 'xcb',
                            QSG_RHI_BACKEND='opengl', QSG_INFO='1', PULSE_SINK=args.sink)
    command = [str(args.binary.resolve()), '--machine', args.machine, '--quit-after', str(args.warmup + args.seconds + (3 if args.prefix else 1))]
    if not args.historical:
        command += ['--window-size', '960x768']
        if args.state == 'menu':
            command += ['--ui-keys', 'F5']
    if args.media:
        command.append(str(args.media.resolve()))
    command = args.prefix + command
    (out / 'command.json').write_text(json.dumps(dict(command=command, env={k: env[k] for k in
        ['XDG_CONFIG_HOME', 'XDG_DATA_HOME', 'XDG_CACHE_HOME', 'QT_QPA_PLATFORM', 'QSG_RHI_BACKEND', 'PULSE_SINK']}), indent=2))
    with (out / 'stderr.log').open('w') as log:
        app = subprocess.Popen(command, env=env, stdout=log, stderr=subprocess.STDOUT)
        # Profilers can be wrappers; resolve the application child for /proc sampling.
        pid = app.pid
        if args.prefix:
            time.sleep(1)
            children = Path(f'/proc/{pid}/task/{pid}/children').read_text().split()
            if children:
                pid = int(children[-1])
        time.sleep(1)
        if not args.software:
            win = subprocess.check_output(['xdotool', 'search', '--sync', '--onlyvisible', '--pid', str(pid)], text=True, timeout=10).splitlines()[-1]
            subprocess.run(['xdotool', 'windowsize', win, '960', '768'], check=True)
            subprocess.run(['xdotool', 'windowactivate', '--sync', win], check=True)
            if args.state == 'background':
                subprocess.run(['xdotool', 'windowminimize', win], check=True)
        # Record actual server routing: Qt's native PipeWire backend can ignore
        # PULSE_SINK when opening an explicitly resolved default device.
        try:
            def pulse_list(kind):
                return json.loads(subprocess.check_output(['pactl', '-f', 'json', 'list', kind], stderr=subprocess.DEVNULL))
            clients = pulse_list('clients')
            owned = [x for x in clients if x.get('properties', {}).get('application.process.id') == str(pid)]
            client_ids = {str(x['index']) for x in owned} | {str(x.get('properties', {}).get('object.id')) for x in owned}
            streams = [x for x in pulse_list('sink-inputs') if str(x.get('client')) in client_ids
                       or str(x.get('properties', {}).get('client.id')) in client_ids]
            sinks = {x['index']: x['name'] for x in pulse_list('sinks')}
            routing = [dict(index=x['index'], sink=x['sink'], sink_name=sinks.get(x['sink']),
                            sample_specification=x.get('sample_specification')) for x in streams]
            (out / 'audio-routing.json').write_text(json.dumps(routing, indent=2))
        except (OSError, subprocess.CalledProcessError, ValueError):
            pass  # Optional server introspection; CPU/process sampling still works.
        time.sleep(max(0, args.warmup - 1))
        start = snapshot(pid)
        samples = []
        end_at = time.monotonic() + args.seconds
        while time.monotonic() < end_at:
            time.sleep(min(1, max(0, end_at - time.monotonic())))
            samples.append(snapshot(pid))
        end = samples[-1]
        wall = end['time'] - start['time']
        hz = os.sysconf('SC_CLK_TCK')
        cpu = (end['ticks'] - start['ticks']) / hz
        result = dict(pid=pid, cpu_seconds=cpu, wall_seconds=wall, one_core_percent=100 * cpu / wall,
                      machine_percent=100 * cpu / wall / os.cpu_count(), logical_cpus=os.cpu_count(),
                      rss_kib=end['rss_kib'], peak_sampled_rss_kib=max(s['rss_kib'] for s in samples),
                      pss_kib=end['pss_kib'], threads=[])
        for tid, t in end['threads'].items():
            a = start['threads'].get(tid)
            if a:
                result['threads'].append(dict(tid=tid, name=t['name'], role='GUI' if tid == str(pid) else t['name'], cpu_seconds=(t['ticks'] - a['ticks']) / hz,
                    voluntary_switches_per_second=(t['voluntary'] - a['voluntary']) / wall,
                    involuntary_switches_per_second=(t['involuntary'] - a['involuntary']) / wall))
        result['threads'].sort(key=lambda t: t['cpu_seconds'], reverse=True)
        # Let --quit-after close cleanly so profiler traces and destructors finish.
        result['exit_code'] = app.wait(timeout=15)
        (out / 'samples.json').write_text(json.dumps(dict(start=start, samples=samples), indent=2))
        (out / 'result.json').write_text(json.dumps(result, indent=2))
        print(json.dumps(dict(output=str(out), **result)))
        if result['exit_code']:
            raise SystemExit(result['exit_code'])


if __name__ == '__main__':
    main()
