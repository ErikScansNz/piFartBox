# TASK-0017 - Low-Level Audio Runtime Research

## Objective
Research and document the best practical low-level audio stack for `piFartBox` on Raspberry Pi 1 B+, with a focus on realtime scheduling, low-latency playback, and the Linux subsystems that best fit a dedicated synth appliance.

## Success Criteria
- The repo contains a compact decision note covering ALSA, PipeWire, JACK, and PREEMPT_RT tradeoffs.
- The note recommends a near-term audio stack for the current `piFartBox` phase.
- The note includes concrete guidance for realtime scheduling, memory locking, and service configuration.
- The note ends with a clear implementation order for the actual ALSA runtime.

## Constraints
- Use primary sources wherever practical.
- Keep the note implementation-oriented.
- Focus on Raspberry Pi OS Lite 32-bit and Raspberry Pi 1 B+ hardware limits.

## Implementation Design
- review current Raspberry Pi audio guidance
- review ALSA PCM and direct-access documentation
- review Linux scheduling and memory-locking references
- review PipeWire and JACK realtime requirements
- record a compact repository decision note with recommended defaults

## Interfaces / Types Affected
- documentation only

## Test Plan
- verify source links are current
- ensure the note makes one clear near-term recommendation
- ensure the note contains concrete next implementation steps for ALSA runtime work

## Assumptions / Defaults
- current target remains Raspberry Pi 1 B+
- current OS baseline remains Raspberry Pi OS Lite 32-bit
- current product remains a dedicated synth workstation rather than a general desktop-audio host

## Out of Scope
- ALSA engine code
- PREEMPT_RT kernel build work
- JACK or PipeWire integration code
