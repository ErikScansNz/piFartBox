# TASK-0012 - Runtime Entrypoint Scaffold

## Objective
Create the first real `pi_fartbox_runtime` executable so the provisioned systemd service has a concrete runtime binary to launch.

## Success Criteria
- The repo defines a `pi_fartbox_runtime` target under `apps/`.
- The runtime executable links the current subsystem libraries cleanly.
- The runtime stays alive as a long-running process suitable for systemd.
- The runtime reports useful startup metadata and supports a one-shot inspection mode.
- Baseline runtime defaults align with the new platform provisioning assumptions.

## Constraints
- Keep the implementation lightweight and scaffold-oriented.
- Do not introduce browser audio or emulator behavior.
- Do not depend on target-native compilation.
- Preserve the existing runtime probe target.

## Implementation Design
- add `apps/runtime/` with a `pi_fartbox_runtime` executable
- implement:
  - subsystem/config summary output
  - signal-aware main loop for long-running service behavior
  - a `--oneshot` mode for inspection and smoke testing
- align defaults with the new platform baseline:
  - ALSA-first audio backend
  - local control API port matching nginx
  - session/state roots consistent with provisioning
- document the runtime entrypoint behavior for deployment and service use

## Interfaces / Types Affected
- top-level CMake target graph
- runtime executable layout
- subsystem default config surfaces
- deployment/runtime docs

## Test Plan
- build the runtime target with the WSL cross-build path
- run the runtime in `--oneshot` mode locally under WSL
- verify the runtime probe still builds
- verify the runtime service target path matches the produced binary name

## Assumptions / Defaults
- runtime binary path is `/opt/piFartBox/runtime-current/bin/pi_fartbox_runtime`
- first service mode is a long-running foreground process
- structured networking/API implementation can come later behind the same binary

## Out of Scope
- full audio processing startup
- real HTTP API serving
- final systemd validation on the Pi
