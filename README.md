# piFartBox

`piFartBox` is the new Linux-native synth workstation codebase for Raspberry Pi 1 B+.

## Product Direction
- Deployment base: fresh `Raspberry Pi OS Lite 32-bit`
- Audio runtime: native C++ engine only
- Browser role: Pi-hosted editor/control surface only
- Product model: curated modular instruments assigned to 4 live slots
- Primary controller: Novation SL MkIII

## Non-Goals
- No Raspberry Pi Pico target in this repository
- No PWM audio backend
- No browser-side synth emulator or browser audio engine
- No Web Serial-first architecture

## Runtime Subsystems
- `engine-core`: DSP graph execution, slots, voices, modulation, FX, load/save hooks
- `session-manager`: sessions, slot assignments, mixer state, controller mappings
- `control-server`: Pi-hosted API for browser and controller/admin actions
- `controller-mapper`: SL MkIII pages, LEDs, display, and slot workflow

## Canonical Model
- `Module`: curated building block with typed ports and known CPU class
- `Instrument`: saved approved module graph
- `Slot`: live host for one instrument instance
- `Session`: full workstation state containing 4 slots and controller focus

## Repository Layout
- `engine/`
- `session-manager/`
- `platform/linux/`
- `integrations/zynthian/`
- `control-api/`
- `controller-mapper/`
- `web/`
- `content/`
- `docs/`

## Planning Workflow
- Full plans live in `docs/plans/`
- Active and planned work is indexed in `docs/todo.md`
- Completed work is moved to `docs/completedTasks.md`
- Detailed task context belongs in plan files, not in root-level docs

## Update Workflow
- The primary deployed-system update path is USB-first.
- Updates are expected to travel on USB mass storage as a git bundle plus manifest.
- See `docs/updater-system.md` and `scripts/update/`.

## Build Baseline
- The first live Pi target is treated as a legacy ARMv6 runtime appliance.
- The preferred build model is modern-host cross-compile plus deploy.
- A native on-device full-copy build pipeline now exists as a fallback when ARMv6 compatibility needs to be validated directly on the Pi.
- The first supported Windows-host baseline is `Ubuntu 22.04` under WSL.
- Native compilation on the Pi is not the primary supported path.
- The first target OS baseline is fresh `Raspberry Pi OS Lite 32-bit`.
- The first target audio baseline is ALSA-first.
- See `docs/platform-provisioning.md`, `docs/target-compatibility-decision.md`, `docs/cross-compile-and-deploy.md`, and `docs/wsl-cross-build-host.md`.

## Current Status
Repository bootstrap and governance scaffolding are complete.
Runtime foundation, schema, and the first ARMv6 cross-build baseline are in place.
Provisioning for fresh Raspberry Pi OS targets supports streamed output and selectable apt modes.
The first ALSA runtime foundation is now live on the Pi, including runtime JSON telemetry and a Pi-hosted workbench view of accepted audio settings.
The starter subtractive instrument now renders through the ALSA callback path, and the runtime exports an active SL MkIII InControl page preview derived from focused instrument controls.
See `docs/todo.md` for the next active tasks.
