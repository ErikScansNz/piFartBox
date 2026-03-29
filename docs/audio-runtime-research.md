# Low-Level Audio Runtime Research

## Summary
For the current `piFartBox` phase on Raspberry Pi 1 B+, the best low-level audio stack is:

- direct `ALSA` via `alsa-lib`
- one dedicated audio render thread using `SCHED_FIFO`
- `mlockall(MCL_CURRENT | MCL_FUTURE)` after major startup allocations
- explicit ALSA period/buffer configuration and underrun recovery
- no `PipeWire` or `JACK` in the critical path for the first milestone

The best longer-term path is:

- keep the dedicated ALSA engine design
- move from read/write PCM access to ALSA direct `mmap` only after the basic render loop is stable
- evaluate `PREEMPT_RT` only after real latency and underrun measurements on the Pi

## Research Date
- 2026-03-29

## Recommended Stack

### 1. Use ALSA First
For a dedicated Pi-hosted synth workstation, direct ALSA is the best starting point.

Why:
- Raspberry Pi’s own current audio whitepaper says Raspberry Pi OS Lite still uses ALSA and does not include PipeWire by default.
- ALSA is the lowest practical userspace audio API we need for a dedicated appliance.
- It gives us direct control over device choice, sample rate, format, period size, buffer size, start threshold, and underrun handling.

Recommendation:
- use `alsa-lib` PCM directly
- prefer a real device path such as `hw:` for dedicated deployment
- keep `plughw:` as a compatibility fallback during bring-up

## Why Not PipeWire First
PipeWire is useful for desktop-style audio routing, but it adds a userspace daemon and another policy layer between the synth and the hardware.

Its own realtime module docs show that it depends on realtime limits such as `RLIMIT_RTPRIO` and can fall back to portal or RTKit paths when those are not already configured.

Recommendation:
- do not put PipeWire in the first runtime audio path
- revisit it only if we later need desktop interoperability or multiple shared audio clients

## Why Not JACK First
JACK is strong when inter-application routing is the goal, but it is extra moving parts for a single dedicated synth appliance.

Its own Linux realtime guidance centers around configuring privileged realtime scheduling and memory locking for JACK users.

Recommendation:
- do not make JACK part of the first audio milestone
- revisit only if we later want external audio graph routing between multiple Linux audio apps

## Raspberry Pi Audio Hardware Guidance
Raspberry Pi’s audio whitepaper says the Pi 1/1 B+ style 3.5 mm analogue path is a low-quality analogue output generated from PCM, while I2S adapter boards provide digital audio via the GPIO header.

Recommendation:
- bring the first ALSA engine up on whatever device is immediately available
- prefer an I2S DAC or other digital output path for serious synth use
- treat the analogue headphone output as acceptable for bring-up but not as the best long-term audio target

## ALSA API Recommendation

### First implementation
Start with:
- `snd_pcm_open()`
- blocking playback
- `SND_PCM_ACCESS_RW_INTERLEAVED`
- explicit period size and buffer size
- `snd_pcm_recover()` on xrun/suspend style errors

This is the best correctness-first path on Pi 1 B+.

### Later optimization
Once the engine is stable:
- evaluate `SND_PCM_ACCESS_MMAP_INTERLEAVED`
- use ALSA direct-buffer functions such as `snd_pcm_mmap_begin()` / `snd_pcm_mmap_commit()`

ALSA’s PCM docs and direct-access docs confirm the ring-buffer model and the direct `mmap` path for zero-copy style access.

Recommendation:
- first milestone: `RW_INTERLEAVED`
- second milestone: optional `MMAP_INTERLEAVED`

## Scheduling Recommendation
Linux realtime scheduling docs make `SCHED_FIFO` the right primitive for the audio render thread.

Important behavior:
- realtime threads outrank normal `SCHED_OTHER` work
- a runnable `SCHED_FIFO` thread will preempt normal threads immediately
- `SCHED_FIFO` has no timeslicing, so badly behaved realtime code can starve the machine

Recommendation:
- dedicate exactly one render thread as `SCHED_FIFO`
- keep web, control, logging, file I/O, and network work off that thread
- keep helper threads on normal scheduling initially
- start with an audio thread priority around `60`
- raise or lower only after measuring xruns and starvation risk

Important Pi-specific note:
- on a single-core Pi 1 B+, CPU isolation and affinity tricks are much less useful than simply keeping the audio thread short, deterministic, and free of blocking calls

## Memory Locking Recommendation
Linux `mlockall()` exists specifically to keep process memory resident and avoid paging delays that hurt realtime execution.

Recommendation:
- call `mlockall(MCL_CURRENT | MCL_FUTURE)` during startup
- do it after major fixed allocations and before entering steady-state audio
- pre-touch stack and hot buffers before the render loop
- avoid heap allocation and `fork()` after memory locking

## Runtime Service Recommendation
The runtime should run as a `systemd` service with permissions that allow realtime scheduling and memory locking.

Recommended service defaults for the first ALSA milestone:
- `LimitRTPRIO=95`
- `LimitMEMLOCK=infinity`
- `CPUSchedulingPolicy=fifo`
- `CPUSchedulingPriority=60`

Important implementation rule:
- still set thread policy explicitly in-process
- do not rely only on service-level defaults

## PREEMPT_RT Recommendation
Current Linux kernel docs describe `PREEMPT_RT` as a kernel realtime-preemption model for lower-latency behavior inside the kernel itself.

Recommendation:
- do not make `PREEMPT_RT` a prerequisite for the first playable milestone
- first measure actual behavior with:
  - direct ALSA
  - `SCHED_FIFO`
  - `mlockall()`
  - disciplined no-allocation/no-blocking audio code
- only consider `PREEMPT_RT` if real Pi measurements still show unacceptable jitter or underruns

This keeps the initial platform simpler while still leaving a valid later optimization path.

## Concrete Runtime Recommendations For piFartBox

### Threading
- one dedicated audio render thread
- one control/MIDI thread or event worker
- web/control API threads remain non-realtime
- no logging, JSON writing, or filesystem work on the audio thread

### ALSA defaults to try first
- sample rate: `48000`
- channels: `2`
- format: `S16_LE` first, with `S32_LE` later if justified
- periods: start with `3`
- period size: start around `128` or `256` frames
- access: `RW_INTERLEAVED`

These are starting assumptions, not final truths. The correct values need measurement on the live Pi and chosen DAC/device.

### Error handling
- use `snd_pcm_recover()` for xrun/suspend recovery
- expose xrun counters and active ALSA settings in runtime JSON
- add startup validation that reports the actual accepted ALSA format/rate/period settings, not just the requested ones

### Device selection
- start with the default ALSA playback device or an explicit configured device
- prefer a dedicated `hw:` device name once the chosen output hardware is stable
- if both analogue and I2S are present, test both, but bias toward I2S for quality

## Recommended Implementation Order
1. Add ALSA device open/close and device enumeration to `platform/linux`.
2. Build a minimal stereo test-tone playback path in `pi_fartbox_runtime`.
3. Add ALSA config reporting to `/api/runtime-status.json`:
   - device
   - format
   - sample rate
   - period size
   - period count
   - xruns
4. Add `SCHED_FIFO` + `mlockall()` bring-up with explicit failure reporting.
5. Add underrun recovery and runtime counters.
6. Only after that, evaluate ALSA `mmap`.
7. Only after real measurements, decide whether `PREEMPT_RT` is worth the maintenance cost.

## Bottom Line
For `piFartBox` on Raspberry Pi 1 B+, the best foundation is:

- direct ALSA
- one disciplined `SCHED_FIFO` audio thread
- locked memory
- explicit ALSA tuning
- no desktop audio server in the hot path

That is the simplest path to a reliable, testable synth appliance on this hardware.

## Sources
- Raspberry Pi audio options whitepaper: https://pip-assets.raspberrypi.com/categories/1259-audio-camera-and-display/documents/RP-008124-WP-1-Choosing%20an%20Audio%20option.pdf
- Raspberry Pi I2S whitepaper: https://pip-assets.raspberrypi.com/categories/1259-audio-camera-and-display/documents/RP-009699-WP-1-Using%20the%20I2S%20peripherals%20on%20Raspberry%20Pi%20SBCs.pdf
- ALSA PCM docs: https://www.alsa-project.org/alsa-doc/alsa-lib/pcm.html
- ALSA PCM direct/mmap docs: https://www.alsa-project.org/alsa-doc/alsa-lib/group___p_c_m___direct.html
- Linux scheduling docs: https://man7.org/linux/man-pages/man7/sched.7.html
- Linux memory locking docs: https://man7.org/linux/man-pages/man2/mlock.2.html
- PipeWire realtime module docs: https://docs.pipewire.org/page_module_rt.html
- JACK Linux realtime configuration FAQ: https://jackaudio.org/faq/linux_rt_config.html
- Linux kernel realtime preemption docs: https://www.kernel.org/doc/html/latest/core-api/real-time/index.html
