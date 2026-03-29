# Runtime Foundation

This note captures the concrete subsystem boundaries established by `TASK-0001`.

## Runtime Subsystems
### `engine-core`
- owns DSP graph execution
- owns slot rendering and voice allocation
- owns modulation, FX, and audio-facing runtime contracts
- exposes the engine runtime surface consumed by higher-level subsystems

### `session-manager`
- owns session persistence boundaries
- owns slot assignment state
- owns controller profile assignment state
- owns deployed/session path defaults for the runtime

### `control-server`
- owns the Pi-hosted local control API boundary
- receives commands from browser/editor and operator tooling
- forwards validated commands to engine and session subsystems

### `zynthian-adapter`
- owns Zynthian lifecycle and integration concerns
- translates runtime metadata into Zynthian-facing concepts
- should remain thin around the engine and session model

### `controller-mapper`
- owns dedicated controller integration logic
- starts with Novation SL MkIII as the first-class target
- maps slot focus, pages, labels, LEDs, and live control semantics

### `platform/linux`
- owns Linux host concerns
- audio backend selection
- filesystem install roots and update roots
- process/runtime host defaults

## Initial Interface Defaults
- the runtime is monolithic at process level
- slot count defaults to 4
- browser audio is not part of the runtime
- Zynthian integration is treated as a primary platform contract
- controller logic is isolated from generic engine code

## Current Implementation Scope
The current repository state provides:
- concrete native library targets for each subsystem
- include paths and stub interfaces for each subsystem
- explicit dependency edges between the subsystem targets

This task does not yet implement real DSP, persistence, browser routes, or Zynthian runtime glue.
