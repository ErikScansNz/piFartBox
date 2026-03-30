# TASK-0020 - Pico Reference Controller Guidelines

## Objective
Analyze the previous `pico2w-synth` codebase and documentation to extract the useful synth-control, MIDI-mapping, and SL MkIII InControl interaction patterns, then turn those findings into a concrete implementation guideline for `piFartBox`.

## Success Criteria
- Relevant `pico2w-synth` docs and source areas are reviewed:
  - root README
  - `docs/`
  - `src/synth`
  - `src/midi`
  - `web/js/slmkiii.js`
  - related browser controller code
- A new `piFartBox` guideline doc summarizes:
  - what to carry forward
  - what to leave behind
  - the hybrid dynamic + hard-wired controller model
  - concrete SL MkIII page, slider, knob, and button recommendations
- The guideline is implementation-oriented and usable as the reference for future controller tasks.

## Constraints
- Treat `pico2w-synth` as a reference source only.
- Do not modify files in the old repo.
- Keep the new guideline focused on `piFartBox`, not as a changelog of the old project.

## Implementation Design
- inspect the old project’s README and relevant docs
- inspect the old synth/midi/controller source areas
- extract the best patterns for:
  - dynamic display population
  - reserved/hard-wired controls
  - instrument vs global mixer control
  - InControl display and LED behavior
- write a new `piFartBox` implementation guideline doc in `docs/`

## Interfaces / Types Affected
- documentation only

## Test Plan
- verify the guideline references real old-project source locations
- verify the recommendations are specific enough to implement against
- verify the hybrid slider/page recommendations are explicitly documented

## Assumptions / Defaults
- `pico2w-synth` remains the best current practical reference for SL MkIII interaction
- `piFartBox` will use a hybrid model rather than purely hard-wired pages

## Out of Scope
- implementing the actual MIDI/controller runtime changes
- modifying the old project
