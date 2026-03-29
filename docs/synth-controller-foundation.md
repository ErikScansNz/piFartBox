# Synth and Controller Foundation

## Summary
`TASK-0013` establishes the first shared runtime model for `piFartBox` so the synth engine and Novation SL MkIII workstation layer derive from the same canonical slot and instrument state.

## Runtime Foundation
- `EngineRuntime` now owns slot runtime state, slot focus, fixed channel routing, compiled instruments, and per-slot voice pools.
- `InstrumentCompiler` now produces a `CompiledInstrument` with:
  - runtime graph ordering for `event`, `mod`, and `audio` device classes
  - a `VoiceDefinition` for the starter subtractive composite voice
  - generated controller pages grouped from exported controls
- `starter_subtractive_instrument_definition()` provides the first curated test instrument for slot 1.

## Controller Foundation
- `MidiRouter` normalizes note input and high-level workstation control actions.
- `SlmkiiiControllerCore` builds:
  - hand-authored `Workstation` and `System` pages
  - generated `Instrument` pages from compiled instrument exports
  - basic SL MkIII SysEx helpers for notifications and RGB LED updates
- Slot routing is fixed for this milestone:
  - slot 1 -> MIDI channel 1
  - slot 2 -> MIDI channel 2
  - slot 3 -> MIDI channel 3
  - slot 4 -> MIDI channel 4

## Scope Boundaries
This foundation intentionally does **not** yet implement:
- realtime MIDI device I/O
- audio rendering
- browser authoring/editor features
- a generic modulation matrix

The goal of this milestone is to make the engine/session runtime the single source of truth so later ALSA, MIDI I/O, browser, and SL MkIII work all attach to the same model.
