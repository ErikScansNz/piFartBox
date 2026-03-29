# TASK-0008 - WSL Cross-Build Host Provisioning

## Objective
Provision the first supported local Linux build host under WSL so the Windows workstation can cross-compile `piFartBox` for the ARMv6 Pi target.

## Success Criteria
- A supported WSL distro is installed and launches successfully.
- The WSL host has the packages needed to run the repo ARMv6 cross-build flow.
- The repo documents the chosen WSL baseline and package set.
- The cross-build wrapper can at least configure the host environment without missing-tool failures.

## Constraints
- The live Pi remains a runtime target, not the primary build host.
- The Windows machine currently has WSL enabled but no distro installed.
- Provisioning should prefer a stable, mainstream Linux distro with good package availability.

## Implementation Design
- install `Ubuntu 22.04` as the supported WSL distro baseline
- provision cross-build packages:
  - `build-essential`
  - `cmake`
  - `ninja-build`
  - `pkg-config`
  - `git`
  - `python3`
  - `python3-pip`
  - `gcc-arm-linux-gnueabihf`
  - `g++-arm-linux-gnueabihf`
- document how Windows paths map into WSL for this repo
- verify the WSL host can run the repo cross-build wrapper script

## Interfaces / Types Affected
- build host documentation
- cross-build workflow docs
- task tracking docs

## Test Plan
- verify `wsl --list --verbose` shows the installed distro
- verify the distro launches with `bash -lc`
- verify the cross compiler binaries are on PATH inside WSL
- verify the repo build wrapper reaches CMake configure without missing-tool errors

## Assumptions / Defaults
- `Ubuntu-22.04` is the supported WSL distro baseline
- WSL filesystem access to `C:\piFartBox` is sufficient for the first build pass
- the repo continues to target `arm-linux-gnueabihf`

## Out of Scope
- producing a final production binary
- deploying the built artifact to the Pi
- changing the target CPU or ABI baseline
