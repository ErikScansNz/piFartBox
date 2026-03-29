# TASK-0003 - Reference Docs and SL MkIII SysEx Notes

## Objective
Add a compact reference-doc manifest for the new project and capture the proven SL MkIII SysEx communication approach from the current synth codebase so the new platform starts from verified knowledge.

## Success Criteria
- A compact `docs/reference-docs.md` exists with high-value source links grouped by domain.
- A dedicated SL MkIII communication document exists in the new repo and explains the working SysEx approach used in the current codebase.
- The SL MkIII doc includes the key MIDI/SysEx message model, the current code locations, and the practical implementation lessons we want to carry forward.
- The task is recorded in the repo task system and then moved to completed once done.
- The new repo is committed so the scaffold and docs are under git tracking.

## Constraints
- Keep the reference-doc manifest link-oriented and compact.
- Do not paste large source excerpts into active context files.
- The SL MkIII doc should summarize proven behavior and point back to current source files rather than duplicating the full old implementation.
- Do not treat the current Pico-era CC map as the final future controller spec.

## Implementation Design
### Reference docs
- Create `docs/reference-docs.md`.
- Group links by:
  - Zynthian platform and development
  - Raspberry Pi hardware and audio
  - Linux audio stack
  - Controller integration
- Add a short note for why each source matters.

### SL MkIII documentation
- Create a focused implementation note that records:
  - InControl USB port requirement
  - Web MIDI SysEx permission requirement
  - current SysEx header and command IDs
  - screen layout, property, notification, and LED messaging model
  - relative encoder behavior and button/fader expectations
  - important lessons learned from the working browser implementation
- Reference the existing source files in `pico2w-synth` that demonstrate the behavior.

### Git tracking
- Add the new files to the `piFartBox` repo.
- Create an initial commit that captures the scaffold plus the new documentation.

## Interfaces / Types Affected
- Documentation only.
- No runtime engine or schema interfaces are changed by this task.

## Test Plan
- Verify `docs/reference-docs.md` is concise and link-oriented.
- Verify the SL MkIII note includes enough detail to re-implement the communication layer later.
- Verify the repo has a valid git commit after the task completes.

## Assumptions / Defaults
- The current `pico2w-synth` SL MkIII implementation is the reference behavior for communication patterns, not the final future UX.
- The reference doc list should prefer official and primary sources where available.

## Out of Scope
- Re-implementing the controller integration in `piFartBox`
- Finalizing the future SL MkIII slot/page map
- Downloading and storing all external documentation locally
