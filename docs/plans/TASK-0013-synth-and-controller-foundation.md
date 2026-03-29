# TASK-0013 - Synth and Controller Foundation

## Objective
Implement the first integrated synth-runtime and SL MkIII workstation foundation so the engine and controller subsystems share one canonical runtime model.

## Success Criteria
- The engine defines compiled instrument, voice, slot, and runtime graph types.
- The engine can compile a curated subtractive test instrument and assign it to slot 1.
- The runtime owns slot voice pools, note dispatch, focus state, and fixed channel routing for slots 1-4.
- The controller subsystem defines a real workstation controller core with MIDI routing, generated instrument pages, and hand-authored workstation/system pages.
- The runtime binary surfaces the new synth/controller state in `--oneshot` mode.

## Constraints
- Keep the implementation scaffold-oriented and deterministic.
- Do not add a generic modulation matrix yet.
- Do not depend on browser-local state or hardware-local truth.
- Keep slot routing fixed to MIDI channels 1-4 for this milestone.

## Implementation Design
- add engine-owned runtime types for compiled instruments, voices, slots, note events, and runtime graph ordering
- implement a curated subtractive composite instrument compiler path
- implement note routing and voice allocation in `EngineRuntime`
- add `MidiRouter`, `ControllerAction`, `ControllerContext`, and `SlmkiiiControllerCore`
- generate instrument pages from exported controls and add workstation/system pages by hand
- keep SL MkIII message-model helpers limited to controller-runtime scaffolding and state derivation

## Interfaces / Types Affected
- engine runtime and instrument compiler
- controller-mapper public API
- runtime executable summary/output

## Test Plan
- build ARMv6 runtime successfully
- verify `pi_fartbox_runtime --oneshot` shows compiled instrument and slot/controller state
- verify note routing sends channels 1-4 to slots 1-4
- verify generated instrument pages and workstation pages are present in controller context

## Assumptions / Defaults
- first instrument is a subtractive composite module
- first slot/channel policy is fixed 1-4
- first controller target remains Novation SL MkIII InControl

## Out of Scope
- actual MIDI device I/O integration
- actual audio rendering
- browser editor implementation
