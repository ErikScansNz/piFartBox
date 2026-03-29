# TASK-0006 - Target Toolchain Compatibility Baseline

## Objective
Align `piFartBox` build assumptions with the actual Raspberry Pi target toolchain, or define the minimum required toolchain upgrade path if the current host image is too old.

## Success Criteria
- The exact target toolchain versions and incompatibilities are documented.
- The repo decides whether to:
  - lower local build requirements, or
  - upgrade the target toolchain/environment, or
  - support both paths explicitly.
- The CMake and C++ baseline for `piFartBox` becomes explicit and validated against the real target.
- The target can perform at least a successful configure/build for the current foundation code.

## Constraints
- The target is currently Raspbian stretch on ARMv6.
- The observed target toolchain is:
  - `cmake 3.7.2`
  - `g++ 6.3.0`
- Existing Zynthian installation should remain usable.

## Implementation Design
- Capture the real target facts in docs.
- Compare them to current repo assumptions:
  - `cmake_minimum_required(VERSION 3.20)`
  - C++20 requirement
- Decide the supported baseline:
  - modernize the target toolchain
  - relax repo build requirements
  - or separate host-build and target-build profiles
- Validate the chosen approach on the live Pi.

## Interfaces / Types Affected
- top-level build system
- compiler/language standard assumptions
- platform docs and target bootstrap notes

## Test Plan
- verify `cmake` configure succeeds on the target
- verify the current scaffold builds on the target
- verify the chosen baseline is documented in the repo

## Assumptions / Defaults
- the target host from `TASK-0005` remains the first live deployment reference
- toolchain compatibility must be solved before deeper runtime bring-up on the Pi

## Out of Scope
- full audio-engine implementation
- browser deployment
- controller runtime integration
