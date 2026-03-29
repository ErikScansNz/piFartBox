# TASK-0018 - ALSA Runtime Foundation

## Objective
Implement the first real Linux audio path for `piFartBox` using direct ALSA playback, a realtime-friendly audio render thread, and runtime telemetry that can be consumed from the Pi-hosted workbench UI.

## Success Criteria
- `pi_fartbox_runtime` can open an ALSA playback device on the Pi and run a render loop.
- The runtime attempts realtime-friendly startup behavior using `SCHED_FIFO` and `mlockall()`.
- Runtime JSON includes concrete ALSA configuration and health details:
  - requested device
  - active device
  - format
  - sample rate
  - period size
  - period count
  - xruns
  - realtime/memory-lock status
- A simple audio test path exists for validation on device.
- The Pi-hosted web UI displays the new ALSA runtime details.

## Constraints
- Keep the first milestone ALSA-first and do not introduce JACK or PipeWire.
- Keep the runtime architecture simple enough for Raspberry Pi 1 B+.
- Avoid heap allocation, filesystem I/O, and logging in the hot audio loop.
- Preserve the current runtime/service/deploy flow.

## Implementation Design
- add a platform-owned ALSA playback engine in `platform/linux`
- start with blocking `RW_INTERLEAVED` PCM access and `S16_LE`
- use one dedicated audio thread
- attempt `mlockall(MCL_CURRENT | MCL_FUTURE)` and `SCHED_FIFO`
- render silence by default, with an optional sine-wave test tone path for validation
- surface ALSA status into `runtime-status.json`
- extend the workbench UI to show actual runtime audio health rather than only `/proc/asound` probe data
- update the native on-device build helper to ensure ALSA development headers are present

## Interfaces / Types Affected
- `platform/linux/include/pi_fartbox/platform/linux_host.hpp`
- `platform/linux/src/linux_host.cpp`
- `apps/runtime/src/main.cpp`
- `deploy/systemd/pifartbox-runtime.service`
- `web/index.html`
- `scripts/deploy/build_on_device.py`

## Test Plan
- build `pi_fartbox_runtime` on device
- run `pi_fartbox_runtime --oneshot` and verify ALSA status JSON generation
- run `pi_fartbox_runtime --tone-test 2` on device and confirm the test path executes
- verify `pifartbox-runtime.service` starts and stays alive
- verify the Pi-hosted workbench shows ALSA runtime details and accepted device settings

## Assumptions / Defaults
- target OS remains Raspberry Pi OS Lite 32-bit
- deployment validation will use the current Pi at `192.168.1.26`
- default ALSA backend remains `default` device path unless overridden
- first render format is stereo `S16_LE` at `48000` Hz

## Out of Scope
- full synth voice rendering into ALSA
- controller-driven live parameter edits into the audio engine
- JACK, PipeWire, or PREEMPT_RT integration
