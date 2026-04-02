# Cross-Compile and Deploy

This document defines the first supported `piFartBox` build/deploy baseline for the Raspberry Pi 1 B+ target.

## Decision
The supported baseline is:

- build on a modern Linux host or WSL environment
- cross-compile for `arm-linux-gnueabihf`
- target ARMv6 with hardware floating point
- deploy prebuilt artifacts to the Pi
- keep the source checkout on the Pi for docs, scripts, and repository history

The live Pi is treated as a runtime appliance target, not the primary build machine.

## Sysroot Requirement
For Raspberry Pi 1 B+ deployments, the cross-build should use a target-captured sysroot so the final linked ELF inherits the Pi's actual ARMv6 runtime baseline rather than the host distro's newer ARMhf defaults.

Capture helper:
- `scripts/deploy/capture_target_sysroot.py`

Recommended local location:
- `sysroots/rpi1-trixie/`

## Target ABI / CPU Assumptions
- GNU triplet: `arm-linux-gnueabihf`
- CPU family: ARMv6
- expected flags:
  - `-march=armv6zk`
  - `-mfpu=vfp`
  - `-mfloat-abi=hard`
  - `-marm`

Toolchain file:
- `cmake/toolchains/armv6-rpi1-linux-gnueabihf.cmake`

## Host Prerequisites
Preferred host:
- Linux machine or WSL distro with Debian/Ubuntu-style package management
- current supported Windows-host baseline: `Ubuntu 22.04` under WSL

Expected packages:
- `build-essential`
- `cmake`
- `ninja-build`
- `pkg-config`
- `git`
- `gcc-arm-linux-gnueabihf`
- `g++-arm-linux-gnueabihf`
- `python3`
- `python3-pip`

See:
- `docs/wsl-cross-build-host.md`

Current local validation:
- `Ubuntu 22.04` is installed under WSL2 on the primary Windows workstation
- the ARMv6 cross toolchain is installed and on PATH inside WSL
- `./scripts/build/cross_build_armv6.sh` successfully configures and builds the runtime probe from `/mnt/c/piFartBox`
- the build wrappers now force a fresh configure and fail if the final runtime ELF is not tagged `Tag_CPU_arch: v6`

## Build Target
The first deployable artifact target is:
- `pi_fartbox_runtime_probe`

Purpose:
- verify cross-build plumbing
- verify deployed binary execution on the live Pi
- verify all major subsystem libraries link together

This is a bring-up target, not the final synth runtime.

## Recommended Build Flow
On a Linux/WSL host:

```bash
python3 scripts/deploy/capture_target_sysroot.py --host 192.168.1.26 --user erik --password <password> --name rpi1-trixie
TARGET_SYSROOT=sysroots/rpi1-trixie ./scripts/build/cross_build_armv6.sh
```

Expected output:
- `build-armv6/apps/runtime-probe/pi_fartbox_runtime_probe`
- `build-armv6/apps/runtime/pi_fartbox_runtime`
- a build-time failure if the resulting runtime binary is not tagged for `ARMv6`

## Artifact Packaging
The first artifact package should contain:

```text
piFartBox-runtime-artifact/
  manifest.json
  bin/pi_fartbox_runtime_probe
```

Manifest fields should include:
- `project`
- `target`
- `commit`
- `artifact_name`
- `created_at`

## Deploy Model
Keep two distinct things on the target:

1. source repo checkout
- location: `/opt/piFartBox`
- purpose: docs, scripts, manifests, git history, runtime metadata

2. deployed runtime artifacts
- location: `/opt/piFartBox/runtime/<revision>/`
- purpose: actual executable outputs

Recommended current symlink:
- `/opt/piFartBox/runtime-current`

This keeps artifact deployment separate from source-repo bootstrap/update.

## Relationship to Existing Updater Flow
Current updater docs already support source-repo transfer using git bundles.

Going forward:
- source checkout updates remain bundle-based
- runtime artifact deployment should be an additional step layered on top
- the two should stay coordinated by commit/revision metadata

## Current Recommendation
Do not lower the repo to the old native Pi toolchain.
Do not begin with a blind full system update.

Instead:
- use the documented Linux/WSL cross-build host baseline
- capture a target sysroot from the live Pi
- validate the ARMv6 toolchain file and final ELF attributes
- build the runtime
- deploy and run it on the Pi
