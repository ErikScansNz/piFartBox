# Runtime Entrypoint

## Executable
- runtime binary: `pi_fartbox_runtime`
- deployed path: `/opt/piFartBox/runtime-current/bin/pi_fartbox_runtime`

## Modes
- default:
  - long-running foreground process for `systemd`
- `--oneshot`:
  - print the runtime summary
  - write the runtime status file
  - exit immediately

## Current Responsibilities
- instantiate the current subsystem scaffolding
- print a startup summary
- write a simple runtime status file under `/var/lib/piFartBox`
- remain alive as the service entrypoint until signalled

## Current Status File
- path: `/var/lib/piFartBox/runtime-status.txt`
- json path: `/var/lib/piFartBox/runtime-status.json`

## Notes
- This is a service-compatible scaffold, not the final audio engine runtime.
- ALSA remains the intended first audio backend baseline.
- The control endpoint default is `127.0.0.1:17890` to match the nginx proxy config.
- The Pi-hosted workbench can consume the JSON status export directly through nginx before the full control API is implemented.
