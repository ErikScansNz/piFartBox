# TASK-0002 - Engine Schema and Session Model

## Objective
Define the canonical engine-owned schema for modules, instruments, slots, sessions, and controller profiles so every other subsystem consumes one source of truth.

## Success Criteria
- Canonical model families are defined for `ModuleDefinition`, `InstrumentDefinition`, `SlotState`, `SessionDefinition`, and `ControllerProfile`.
- The schema design is explicit enough to drive both persistence and browser UI generation.
- Curated graph rules and typed connection categories are captured.
- Versioning and validation strategy are defined for future persistence files.

## Constraints
- The engine schema is authoritative; browser code must not invent parallel schemas.
- The model must support 4 concurrent slots.
- The graph is curated rather than fully free-form.
- The schema must be friendly to live editing without destabilizing the runtime.

## Implementation Design
### Canonical model families
- `ModuleDefinition`
  - stable module instance id
  - module type id
  - parameter spec/value set
  - typed ports
  - CPU class and UI metadata
- `InstrumentDefinition`
  - instrument id and version
  - module instances
  - approved graph connections
  - exposed macros/control mappings
  - default controller page metadata
- `SlotState`
  - slot id
  - assigned instrument id
  - enable/mute/focus state
  - MIDI/channel zone settings
  - mix, pan, and send state
- `SessionDefinition`
  - session id/version
  - four slot assignments
  - global performance state
  - controller focus and transport state
- `ControllerProfile`
  - device/profile id
  - page definitions
  - binding targets
  - LED/display metadata
  - slot navigation actions

### Persistence defaults
- Use versioned on-disk definitions.
- Keep IDs stable and explicit.
- Separate reusable instrument definitions from live session state.
- Validate connections against curated graph rules before runtime activation.

### Connection model defaults
- Typed connections should distinguish at least:
  - audio
  - control/modulation
  - note/gate/event
- Curated graph validation should reject unsupported routes before load/activation.

## Interfaces / Types Affected
This task should produce the canonical contracts later consumed by:
- the C++ engine runtime
- session persistence
- browser/editor metadata export
- controller mapping and slot interaction
- Zynthian-facing preset/session discovery

## Test Plan
- Verify schema definitions cover all canonical model families.
- Verify instruments and sessions can be represented without ambiguity.
- Verify curated graph validation rules are documented.
- Verify the schema can support browser UI generation without a duplicate browser-owned contract.

## Assumptions / Defaults
- Versioned structured data will be used for persistence.
- Reusable instruments and live sessions remain separate objects.
- Slot count is fixed at 4 in the first release model.

## Out of Scope
- Actual file parser implementation
- Control-server endpoint definitions
- Real DSP module implementations
- Zynthian runtime glue code