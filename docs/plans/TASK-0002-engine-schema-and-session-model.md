# TASK-0002 - Engine Schema and Session Model

## Objective
Define the canonical engine-owned device schema for modules, instruments, slots, sessions, and controller profiles so every other subsystem consumes one source of truth.

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
- `DeviceTypeDefinition`
  - stable type id
  - category
  - description
  - declared input/output ports
  - parameter definitions
  - exported control definitions
  - supported nesting rules
  - CPU class
  - display/controller metadata
- `DeviceInstance`
  - stable instance id
  - referenced device type id
  - parameter values
  - local overrides for exported controls
  - optional child device instances when the type supports nesting
- `InstrumentDefinition`
  - instrument id and version
  - device instances
  - typed approved graph connections
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
- Prefer `device` as the canonical internal term and treat `module` as user-facing language where useful.

### Connection model defaults
- Typed connections should distinguish at least:
  - `audio`
  - `mod`
  - `event`
- Event timing must live under `event`, not under `mod`.
- Curated graph validation should reject unsupported routes before load/activation.

### Starter palette defaults
- Start with a subtractive-core device palette:
  - oscillators, noise, sub, mixer, VCA, pan
  - multimode filter and drive
  - ADSR, LFO, constant/offset, attenuverter
  - MIDI note input, gate/trigger, clock/divider, arp/sequencer timing
  - chorus, delay, reverb
  - router, selector, macro source

### Nesting defaults
- Normal devices are flat.
- Only explicitly nestable `composite` or approved device types may contain child devices.
- Nested devices are real schema objects.
- Nesting rules must describe:
  - allowed child categories
  - max child count
  - child visibility
  - export bubbling behavior

## Interfaces / Types Affected
This task should produce the canonical contracts later consumed by:
- the C++ engine runtime
- graph validation and registry APIs
- session persistence
- browser/editor metadata export
- controller mapping and slot interaction
- Zynthian-facing preset/session discovery

## Test Plan
- Verify schema definitions cover all canonical model families.
- Verify instruments and sessions can be represented without ambiguity.
- Verify curated graph validation rules are documented and represented in code.
- Verify the schema can support browser UI generation without a duplicate browser-owned contract.
- Verify a simple subtractive voice graph validates.
- Verify invalid `audio -> event` and `event -> audio` routes are rejected.
- Verify nested-device rules are enforced for composite types only.

## Assumptions / Defaults
- Versioned structured data will be used for persistence.
- Reusable instruments and live sessions remain separate objects.
- Slot count is fixed at 4 in the first release model.
- Browser and controller metadata live in engine-owned schema.
- The first graph language is curated, not free-form modular patching.

## Out of Scope
- Actual file parser implementation
- Control-server endpoint definitions
- Real DSP module implementations
- Zynthian runtime glue code
