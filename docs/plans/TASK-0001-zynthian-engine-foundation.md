# TASK-0001 - Zynthian Engine Foundation

## Objective
Define and scaffold the native runtime foundation for a Linux-first, Zynthian-hosted synth engine that replaces the old Pico-centric architecture.

## Success Criteria
- The repository contains concrete subsystem boundaries for `engine-core`, `session-manager`, `control-server`, `zynthian-adapter`, and `controller-mapper`.
- The engine foundation explicitly targets Raspberry Pi 1 B+ limits and ZynthianOS deployment.
- Runtime ownership boundaries are documented clearly enough that implementation can proceed without re-deciding architecture.
- Native build targets exist for the runtime subsystems that will eventually host real code.

## Constraints
- The runtime must be native C++ and produce all audio on the Pi.
- The browser is control-only and must not become a second synth runtime.
- Zynthian integration is a primary platform contract.
- The first release target is 4 simultaneous slots, not an unbounded engine count.
- Modularity is curated and CPU-budgeted.

## Implementation Design
### Runtime subsystem boundaries
- `engine-core`
  - owns DSP graph compilation/execution, voices, slots, modulation, FX, and render scheduling
- `session-manager`
  - owns session persistence, slot assignment state, controller mappings, and restore behavior
- `control-server`
  - exposes a Pi-hosted local API for browser/editor and administration flows
- `zynthian-adapter`
  - integrates the engine with Zynthian lifecycle, discovery, and runtime hosting expectations
- `controller-mapper`
  - owns Novation SL MkIII slot focus, page routing, display feedback, and LED policies

### Build and ownership scaffold
- Expand the CMake structure so each subsystem becomes a concrete native target.
- Add stub interface headers where needed to make boundaries explicit.
- Document which subsystem owns each runtime concern before real DSP code lands.

### Runtime defaults
- Pi audio output is the only audio path.
- 4 slots are reserved in the initial runtime model.
- The render model should be block-based and CPU-aware.
- Slot switching and session recall must be first-class runtime concerns.

## Interfaces / Types Affected
This task should define ownership for the following future interfaces:
- engine runtime API
- session load/save API
- control command API between browser/controller and engine
- Zynthian adapter lifecycle API
- controller mapping API

The task should avoid inventing detailed JSON schemas for instruments and sessions; that belongs to `TASK-0002`.

## Test Plan
- Verify subsystem boundaries are documented and reflected in the repo structure.
- Verify build targets exist for each native subsystem.
- Verify no Pico-specific assumptions remain in the new repo's top-level docs.
- Verify the runtime architecture is consistent with Pi-hosted browser control and Zynthian-native deployment.

## Assumptions / Defaults
- Deployment target is Raspberry Pi 1 B+ running ZynthianOS.
- The engine remains monolithic at process level even if internally modular.
- Slot count is fixed at 4 for the first release architecture.

## Out of Scope
- DSP algorithm implementation
- Module schema serialization details
- Browser route/component design
- SL MkIII CC-page specifics