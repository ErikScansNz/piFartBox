# Device Architecture

This document captures the canonical device model introduced by `TASK-0002`.

## Core Idea
`piFartBox` uses engine-owned `devices` as the primary building blocks for instruments.

Internally, `device` is the canonical term. `Module` can remain a user-facing concept in the browser or documentation, but the schema should use:
- `DeviceTypeDefinition`
- `DeviceInstance`

## Signal Families
The first schema supports exactly three top-level signal families:
- `audio`
- `mod`
- `event`

### `audio`
- sample/audio-rate streams
- oscillators, filters, VCAs, mixers, FX

### `mod`
- control and modulation values
- envelopes, LFOs, macros, offsets, attenuverters

### `event`
- note on/off
- gate / trigger
- clock / division
- transport
- arp / sequencer timing

Timing belongs to `event`, not `mod`.

## Curated Categories
The first curated device categories are:
- `source`
- `tone`
- `mod`
- `event`
- `mix`
- `fx`
- `utility`
- `composite`

This is a constrained graph, not a free-form modular patch system.

Validation expectations:
- incompatible signal-family routes are rejected
- incompatible event subtypes are rejected
- graphs must be acyclic in v1
- exported controls must reference real device parameters

## Nested Devices
Devices are flat by default.

Nested child devices are only allowed when:
- the parent type explicitly declares a child layout rule
- the allowed child categories are satisfied
- the child count stays within the type limit

Nested devices are real schema objects and not hidden implementation detail.

## Exported Controls
Devices do not expose every parameter by default.

Instead:
- each device type declares what controls are exportable
- each instrument chooses which exports become user-facing
- browser and SL MkIII views consume exported control metadata from the engine schema

## Rich Metadata
Every parameter or exported control can describe:
- label and short label
- section and help text
- range and default
- display format
- knob/fader/button intent
- enum options
- preferred page
- automation visibility
- modulation eligibility

## Instrument Model
An instrument is an assembled graph of:
- device instances
- typed connections
- exported controls
- default pages
- validation metadata

The graph must validate against the curated signal and nesting rules before runtime activation.

## Starter Palette
The first starter palette is subtractive-core focused:
- oscillator
- multimode filter
- VCA
- ADSR envelope
- clock
- delay
- composite voice strip

This starter set can expand later toward richer synth families without changing the core schema model.
