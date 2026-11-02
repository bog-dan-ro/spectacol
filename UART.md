# Next UART modules

Options → Peripherals → Next UART devices chooses the ESP and Pi scripts,
reloads edited files, restores the bundled modules and configures a Pi TCP service.
A script field accepts a local path or an HTTPS download URL. Downloads are
stored by SHA-256 in application data; older cached versions are retained.
Changes apply on the emulation thread without rebuilding Spectacol.

Each runtime QJSEngine, its API objects and all QJSValues are created, invoked and
destroyed on ZxKit's Engine thread. Normal transmit hooks call JavaScript directly.
Debugger single steps made on the GUI thread defer their bytes to the Engine.
There is no JavaScript call on an idle UART status read or every CPU instruction.
Socket objects stay on the GUI thread and exchange bounded commands/events with
the runtime; no socket or filesystem operation runs inside a byte hook.

A module is a JavaScript file containing a factory body. `host` is supplied by
Spectacol; return an object with `apiVersion: 1`. Method `this` is the returned
object. For example, this module echoes raw bytes, including zero and FF:

```javascript
return {
    apiVersion: 1,
    reset: function () {},
    onByte: function (byte) {
        host.receive([byte]);
    }
};
```

Callbacks are optional:

- `reset()` clears protocol state after configuration, machine/UART reset or
  restore. Close connections and cancel owned timers here.
- `onByte(byte)` receives a transmitted byte (0–255).
- `onNetwork(event)` receives `connected`, `data`, `sent`, `closed`, `error` or
  `resolved`, with connection `id` and `bytes`, `message` or `address` as applicable.
- `onTimer(id)` receives a one-shot host timer.

The host API is:

- `receive(bytes)`: queue bytes towards the guest; returns the accepted count.
  Retain and retry any remainder. Calls accept at most 65536 bytes, and ZxKit's
  wire queue is bounded. The bundled modules retry via timers and report an
  exceeded backlog instead of silently truncating their stream.
- `network(operation, id, address, port, bytes, udp)`: `open`, `write`, `close` or
  `resolve`. IDs are 0–4 per UART; closing ID -1 closes all connections. Defaults
  for unused arguments are empty/zero/false. `open` uses TCP unless `udp` is true.
- `configuration()`: the Pi bridge's `{host, port}` settings.
- `setTimer(id, milliseconds)` / `cancelTimer(id)`: one-shot timers, up to 64
  per endpoint, IDs 0–1023 and delays 0–60000 ms. Delivery occurs on an Engine
  poll; paused emulation delivers pending events on resume/refresh.
- `log(message)`: report a module error in the UART settings page.

Runtime calls have a 100 ms watchdog. An exception or interrupted function
turns off that module and exposes the error. Reloading or restoring a bundled
module recovers it. Scripts are application extensions selected by the user;
the Engine is separate from the UI's QML engine.

The bundled ESP module supports AT/ATE, reset/version, station-mode/join/status,
CIPMUX, TCP/UDP CIPSTART, length-delimited binary CIPSEND, CIPCLOSE, CIPSTATUS,
CIPDOMAIN and basic UART/sleep options. It models a virtual access point backed
by host networking, not host Wi-Fi credentials; unsupported commands return ERROR.
The Pi module bridges raw serial bytes to a TCP service (port 0 disables it).
It does not emulate a Raspberry Pi CPU, Linux, GPIO electronics or I2S audio.
Hardware/register changes still belong in ZxKit; external modules update the
protocol and service behavior.

`test_next_uart` uses local TCP/UDP servers to verify binary traffic, Pi bridging,
module timers, method ownership, local/downloaded modules, reload and watchdog recovery. No public service or
controller is required. `tests/next.py` drives the compiled application's Next
keyboard, machine selection and SD picker using generated diagnostic ROM/media.
