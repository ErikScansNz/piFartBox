# TASK-0007 - Cross-Compile and Deploy Baseline

## Objective
Establish the first supported build/deploy path where `piFartBox` is built on a modern development machine and deployed to the legacy ARMv6 Pi target.

## Success Criteria
- The repo defines a supported cross-compile target for the Pi 1 B+ runtime.
- The deploy flow can transfer built artifacts or a built checkout to the live target cleanly.
- The build baseline no longer depends on the target host having a modern compiler or CMake.
- The chosen cross-build assumptions are documented in the repo.

## Constraints
- The live target is ARMv6 / `arm-linux-gnueabihf`.
- The target host currently has an outdated native toolchain.
- The existing Zynthian runtime should remain usable.

## Implementation Design
- define the target triplet, ABI, and minimum CPU assumptions
- decide whether deployment sends:
  - source checkout + remote build helpers, or
  - prebuilt binaries/artifacts
- align the updater/deploy scripts with the supported cross-build path
- keep `/opt/piFartBox` as the deployed project root

## Interfaces / Types Affected
- build system
- deployment scripts
- target compatibility docs

## Test Plan
- verify the cross-build target can produce deployable artifacts
- verify the live Pi can run the deployed result
- verify the deploy path stays compatible with the USB-first updater story

## Assumptions / Defaults
- this task follows `TASK-0006`
- the first supported deployment target remains the live Pi at `192.168.1.26`

## Out of Scope
- full audio engine implementation
- browser hosting
- final service integration
