# Runtime Entrypoint

## Executable
- runtime binary: `pi_fartbox_runtime`
- deployed path: `/opt/piFartBox/runtime-current/bin/pi_fartbox_runtime`

## Modes
- default:
  - long-running foreground process for `systemd`
  - starts the ALSA playback thread and writes live runtime JSON
- `--oneshot`:
  - print the runtime summary
  - write the runtime status file
  - exit immediately
- `--tone-test <seconds>`:
  - start the ALSA playback thread with the test-tone path enabled
  - hold it open for the requested duration
  - write runtime JSON and exit
- `--synth-demo <seconds>`:
  - start the ALSA playback thread with the starter subtractive instrument render path
  - enable the internal demo note pattern
  - write runtime JSON and exit after the requested duration

## Current Responsibilities
- instantiate the current subsystem scaffolding
- start the first ALSA playback engine using direct `alsa-lib`
- render the starter subtractive instrument through the ALSA callback path
- attempt `SCHED_FIFO` and `mlockall()` for the audio thread
- print a startup summary
- write runtime status files under `/var/lib/piFartBox`
- remain alive as the service entrypoint until signalled

## Current Status File
- path: `/var/lib/piFartBox/runtime-status.txt`
- json path: `/var/lib/piFartBox/runtime-status.json`

## Current Audio Defaults
- device: `default` unless overridden by `PI_FARTBOX_ALSA_DEVICE`
- sample rate: `48000`
- channels: `2`
- format: `S16_LE`
- periods: `3`
- period frames: `256`

## Relevant Environment Overrides
- `PI_FARTBOX_ALSA_DEVICE`
- `PI_FARTBOX_AUDIO_SAMPLE_RATE`
- `PI_FARTBOX_AUDIO_CHANNELS`
- `PI_FARTBOX_AUDIO_PERIOD_FRAMES`
- `PI_FARTBOX_AUDIO_PERIOD_COUNT`
- `PI_FARTBOX_AUDIO_RT_PRIORITY`
- `PI_FARTBOX_AUDIO_TEST_TONE`
- `PI_FARTBOX_AUDIO_TEST_TONE_HZ`
- `PI_FARTBOX_AUDIO_TEST_TONE_LEVEL`
- `PI_FARTBOX_SYNTH_DEMO`

## Notes
- This is still a foundation milestone, not the final synth voice engine.
- ALSA is now the active first audio backend baseline.
- The runtime JSON now includes an active InControl page preview derived from the focused instrument exports.
- The control endpoint default is `127.0.0.1:17890` to match the nginx proxy config.
- The Pi-hosted workbench can consume the JSON status export directly through nginx before the full control API is implemented.
