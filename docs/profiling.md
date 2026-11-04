# CPU and resource investigation

This investigation precedes CI and UI polish. The original htop observations (about
0.2% after the port, 0.6% without filtering, 0.9–1.5% with filters) did not specify
CPU accounting, media or window workload. They are observations, not benchmark
results. Process CPU measured here sums all threads: 100% means one logical core;
whole-machine CPU divides that by 16 on the Ryzen 7 5800H used here. The local htop
manual distinguishes `CPU%` (one logical core) and `NCPU%` (normalized across
CPUs). The user's configuration selects `PERCENT_CPU` and shows user threads.
The exact row and original game state are unknown, so do not reinterpret the
reported numbers as whole-machine percentages or compare them to process totals
without qualification. The idle 48K scenario is confirmed by the user.

## Reproducing measurements

Build production and QML instrumentation separately, both optimized, with the
same Qt 6.12.0 desktop kit, GCC 16.2 and dependency checkouts:

```sh
cmake -S . -B build-production -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_PREFIX_PATH=/path/to/Qt/6.12.0/gcc_64 -DSPECTACOL_BUILD_TESTS=ON
cmake --build build-production --parallel
cmake -S . -B build-qml-profile -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_PREFIX_PATH=/path/to/Qt/6.12.0/gcc_64 -DCMAKE_CXX_FLAGS=-DQT_QML_DEBUG
cmake --build build-qml-profile --parallel
pactl load-module module-null-sink sink_name=spectacol_profile \
    sink_properties=device.description=SpectacolProfile
python3 tools/profile.py --media /path/to/workload.z80 --output build/profiling/normal
python3 tools/profile.py --media /path/to/workload.z80 --filter hq:4 \
    --output build/profiling/hq4
python3 tools/profile.py --machine 128k --media /path/to/dizzy4-playing.z80 \
    --output build/profiling/dizzy4-128k
python3 tools/profile.py --state menu --output build/profiling/menu
python3 tools/profile.py --state background --output build/profiling/background
python3 tools/profile.py --media /path/to/workload.z80 --output build/profiling/native \
    --prefix perf record -D 5000 -F 499 --call-graph dwarf,8192 --switch-events \
    -o build/profiling/spectacol.data --
mkdir -p build-qml-profile/profiler/traces
python3 tools/profile.py --binary build-qml-profile/bin/spectacol \
    --media /path/to/workload.z80 --output build/profiling/qml \
    --prefix /path/to/Qt/6.12.0/gcc_64/bin/qmlprofiler \
    -o build-qml-profile/profiler/traces/spectacol.qtd --
```

The final comparison uses an isolated KWin virtual output with its own D-Bus
session and Xwayland server. `kwin_wayland --virtual --xwayland --no-lockscreen
--width 1280 --height 1024 --no-global-shortcuts --no-kactivities` supplies a real
AMD OpenGL renderer without depending on desktop focus. Earlier desktop results
remain in the artifacts, but the run interrupted by screen locking and its failed
successor are excluded. Do not merge values from the two display environments.

The sampler is Linux-specific and requires a running X display and `xdotool` for
GPU measurements. It sets a 960×768 window, XCB/OpenGL, 100% speed, the requested machine (48K by default), 32-pixel
borders and unfiltered speaker audio; it activates the window or minimizes it for
the background case. Menu uses the real F5 drawer. Configuration, data and cache
are isolated under the output directory. The process quits through its normal
`--quit-after` path. `--software` is useful for correctness checks but does not
measure GPU texture uploads. Use the returned module ID to unload only the private
audio sink afterwards; do not change the system default device.

Each run saves its command, environment, settings, stderr, `/proc` samples and
summary JSON. CPU uses process ticks over a defined measurement window, excludes
startup and includes all application threads; RSS/PSS include native libraries and
the driver. Voluntary switches indicate blocking activity, not exact wakeups.
The glibc allocation interposer in ZxKit counts allocation traffic in separate
runs; it does not measure live GPU memory or the external audio server.

For this session, four seconds preceded each ten-second comparison window. The
rewind ring finishes filling at roughly six seconds; final RSS/PSS and the sampled
plateau include a full ring. Run longer warmups for strict steady-state studies.
The complete matrix uses shorter four-second windows and is an overview; repeat
individual presets for decisions about small differences. CPU tick resolution is
10 ms. No builds or test suites run concurrently with the measurements.

## Historical comparison and cause

Detached worktrees were created for the original port (`6fa12b7b`) and the then
available initial ZxKit (`e12c5fe`). The port did not pin a library revision; this
pair is reconstructed from Git history and chronology. `76e7a739` is the later UI
rework immediately before filters (`b94d9d66`), so it must not be confused with the
original port. The comparison builds use today's same Qt/compiler/dependencies,
not the original toolchain. Before changes were Spectacol `af2629af` and ZxKit
`bead4cd`. All application revision references remain in this repository.

The border refactor (`fc751d11`) removed `presentFrame()`'s return when no pixel row
changed, so even a static ROM screen started waking the GUI and render threads at
about 50 Hz. Restore that condition while still retaining the newest source frame
and emitting geometry notifications. Cropping a paused frame and changing a
filter still request updates. `image_updates` checks unchanged frames, dirty rows,
paused crop changes and immutable images retained by pending texture uploads.

The filter matrix also found bilinear 4x at 14.68 CPU ms/frame. Constant scale
specializations and reused horizontal sampling coordinates reduce that to
10.11 ms/frame (31%, three paired runs). Original integer rounding, clipping and
RGBA output remain fixed by pre-change fingerprints for all six scales. This
retains significant filter cost; high-resolution bilinear and xBRZ still need
much more CPU than Normal or simple masks.

Native Engine stacks also identified ULA raster work. ZxKit now hoists division
and row lookup out of the clock loop. Fetch timing, memory writes, borders, audio
feeding and the owner's screen/filter UI retain their existing behavior.

## Production results

Three isolated-display runs per case, each with a four-second warmup and a
ten-second CPU measurement window:

| Workload | Historical one core / machine | Before one core / machine | After one core / machine |
|---|---:|---:|---:|
| 48K ROM idle | 2.80% / 0.175% | 7.40% / 0.462% | 3.30% / 0.206% |
| Moving display + beeper | 6.80% / 0.425% | 7.89% / 0.493% | 7.79% / 0.487% |

Idle CPU falls about 55%, restoring the original suppression of redundant
presentation requests. Gameplay-proxy CPU shows no clear end-to-end improvement.
Idle before ranged from 5.10–7.59% of one core; after ranged 3.20–3.30%. Moving
before ranged 7.79–8.09%; after 7.60–7.80%. These measurements explain a real idle
regression but cannot establish the exact original user's workloads or accounting.
The historical/current median RSS was about 231/234 MiB for ROM idle and 245/251
MiB for the moving workload. Final idle RSS remained about 233 MiB; this change
reduces repeated work, not retained UI/native storage.

Menu and minimized background states used 0.40% and 0.30% of one core (0.025% and
0.019% whole-machine). The standalone Engine sleeps when paused; remaining UI
process CPU is mainly the audio backend's approximately 94 callbacks/s. Changing
audio lifecycle to remove those callbacks would require a separate resume/device
study; the independent audio feeder is preserved.

The complete matrix covers all 36 UI presets. Representative final costs on the
animated test snapshot (whole-machine percentages): Normal 0.50%, HQ2x about
1.2%, xBRZ4x 1.53%, bilinear 4x 3.93%, and HQ4x+TV4x 6.55%. Bilinear 4x was
5.09% before the kernel change. These short windows rank costs; the paired native
kernel runs establish the 31% reduction more reliably. The largest chain uses
about 420 MiB RSS and presents about 30 frames/s; QML tracing gives a 36.53 ms p95
renderer gap, with 129 gaps above 33 ms and none above 50 ms in ten seconds.
Unfiltered/xBRZ4x captures remain near 50 presentations/s. The extra copying and
upload volume at high scales remain material costs; quality/preset output is
preserved. Filtering executes outside the image handoff lock and audio feeder.

Dizzy gameplay was checked with explicit local 48K Dizzy 1 and Fantasy World
Dizzy snapshots, prepared once by starting play and saving a snapshot. Captured
windows confirm active rooms, rather than title menus. Every historical/current
run loads that same prepared file; original sources, commands and checksums stay
in ignored `build/profiling/media` output. These are supporting single runs:

| Game | Historical | Before | After | After HQ2x | After xBRZ4x |
|---|---:|---:|---:|---:|---:|
| Dizzy 1, machine % | 0.337 | 0.443 | 0.381 | 0.899 | 1.099 |
| Fantasy World Dizzy, machine % | 0.394 | 0.462 | 0.450 | 1.187 | 1.674 |

Multiply these values by 16 for summed one-core CPU%. There is a modest
historical/current increase on these game states, with no reproduction of a
threefold total-process regression. The exact original game versions/htop row
remain unknown. Required builds and tests do not depend on these local games.

## Evidence and limits

The coordinated session retains raw native data, all commands, results and
before/after binaries in the library checkout's `build/profiling/`; the application
QML traces and diagnostic reports are in `build-qml-profile/profiler/`.
Standalone Engine/audio/filter drivers depend only on ZxKit's own bundled ROMs.
The generated animated/beeper snapshot is authored and reproducible. Idle uses
the bundled 48K ROM. Additional Dizzy inputs are explicit local developer media,
kept only in ignored profiling output rather than required fixtures.

The desktop was not isolated from normal system services, and dynamic boost was
left enabled. Report ranges and medians rather than treating small differences
as significant. The QML parser's animation-derived duration covers only active animation events,
not the entire trace. `tools/summarize_qml_scenegraph.py` supplements it with actual
trace duration, Qt 6 scene graph timing fields and warmed renderer-event gaps.
The gameplay capture has 501 renderer events in ten seconds (p95 20.53 ms,
p99 20.84 ms, maximum 21.26 ms; none above 33 ms). Menu and background have no
renderer events in that warmed window. QML hotspots are mainly startup creation
and compilation, not recurring binding/signal storms. GC pages peak around
1.05–1.11 MiB, with no failed pixmap loads. These traces do not measure native RSS.

Native sampling, QML tracing and allocation interception each
have overhead; their CPU results are labelled separately from production.
Nested QML ranges are not additive CPU time, and QML GC memory is not process RSS.
Kernel scheduler wakeup tracepoints and package energy counters were permission
denied. The host was on AC power; no battery-duration, wattage, Android, Bluetooth
or physical-speaker quality claim follows from these captures.

The measurement environment sets PULSE_SINK, but routing inspection confirmed
Qt 6.12's native PipeWire backend explicitly opens the unchanged system default
(alsa_output.pci-0000_03_00.6.analog-stereo, 48 kHz stereo). Production comparisons
therefore use that same physical output. `audio-routing.json` records actual
routing when supported. The PULSE_SINK hint alone does not select a device for
this backend. No system default was changed.

Live audio checks explicitly select a private sink: the existing independent audio feeder
survives repeated 500 ms GUI stalls and changes of speed. Image hashes and the
complete Debug/Release hardware suites protect output and accuracy. Existing
hardware limits remain listed in ZxKit's fixture README.


## Dizzy IV on 128K

A follow-up uses the owner's `~/Spectacol/Dizzy/Dizzy-4 MagiclandDizzy(Spectrum).tap`,
loaded through the bundled 128K ROMs, with separate title/music and Weirdhenge
gameplay snapshots. The tape reaches its end; both snapshots retain all eight
RAM banks, 128K hardware type, unlocked paging and active AY registers. The
sampler's `--machine 128k` makes model selection explicit. Unattended runs disable
snapshot joystick-choice prompts in their isolated settings.

Three runs per version/state use six seconds of warmup and twelve seconds of
production sampling. Native perf, allocation interception and QML tracing use
separate captures. Source media, preparation steps, screenshots, commands and
results are retained locally under `build/profiling/dizzy4-128k/` in the library
checkout; no user game becomes a required build/test fixture. The dedicated report
contains the title/gameplay comparisons and representative filter costs.

Repeated results (medians of three runs; CPU summed across threads):

| 128K state | Historical, one core | Before fixes, one core | Fixed, one core | Fixed, whole machine |
|---|---:|---:|---:|---:|
| Title with AY music | 3.25% | 7.08% | 3.66% | 0.229% |
| Weirdhenge gameplay | 5.16% | 7.25% | 6.00% | 0.375% |

The fixes reduce title CPU about 48% and gameplay about 17%. A residual increase
relative to the original port remains; this case does not reproduce a threefold
summed-process regression after the fixes. Current HQ2x, xBRZ4x and bilinear 4x
use approximately 0.807%, 1.114% and 2.410% of the whole machine on this gameplay
snapshot (single supporting runs). Unfiltered RSS remains roughly 264 MiB before
and after these fixes. Version/toolchain and game-state limits apply as above.
