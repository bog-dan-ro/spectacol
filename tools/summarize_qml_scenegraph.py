#!/usr/bin/env python3
"""Supplement QTD range summaries with Qt 6 scene graph timings and real frame gaps.
Copyright (c) 2015-2026, BogDan Vatra. GPL-3.0-or-later.
"""
import argparse
import json
from collections import defaultdict
from pathlib import Path
import xml.etree.ElementTree as ET

# QQmlProfilerDefinitions::SceneGraphFrameType, Qt 6.12.
NAMES = ['Renderer', 'Adaptation layer', 'Context', 'Render loop', 'Texture preparation',
         'Texture deletion', 'Polish and sync', 'Windows render show', 'Windows animations', 'Polish']


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('trace', type=Path)
    p.add_argument('--start-seconds', type=float, default=4)
    p.add_argument('--end-seconds', type=float, default=14)
    a = p.parse_args()
    root = ET.parse(a.trace).getroot()
    definitions = {e.get('index'): int(e.findtext('sgEventType'))
                   for e in root.findall('./eventData/event') if e.findtext('type') == 'SceneGraph'}
    events = defaultdict(list)
    for event in root.findall('./profilerDataModel/range'):
        index = event.get('eventIndex')
        if index in definitions and a.start_seconds * 1e9 <= int(event.get('startTime')) <= a.end_seconds * 1e9:
            events[definitions[index]].append(event)
    result = dict(trace_seconds=(int(root.get('traceEnd')) - int(root.get('traceStart'))) / 1e9,
                  window_seconds=a.end_seconds - a.start_seconds, scenegraph=[])
    for kind, ranges in sorted(events.items()):
        result['scenegraph'].append(dict(kind=kind, name=NAMES[kind] if kind < len(NAMES) else str(kind),
            count=len(ranges), timing_fields_ms={f'timing{i}': round(sum(int(e.get(f'timing{i}', 0)) for e in ranges) / 1e6, 4) for i in range(1, 6)}))
    starts = sorted(int(e.get('startTime')) for e in events[0])
    gaps = sorted((b - a) / 1e6 for a, b in zip(starts, starts[1:]))
    if gaps:
        def pct(q):
            return gaps[min(len(gaps) - 1, int((len(gaps) - 1) * q))]
        result['renderer_frame_gaps_ms'] = dict(count=len(starts), p50=pct(.5), p95=pct(.95), p99=pct(.99),
            maximum=max(gaps), over_33ms=sum(x > 33 for x in gaps), over_50ms=sum(x > 50 for x in gaps))
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
