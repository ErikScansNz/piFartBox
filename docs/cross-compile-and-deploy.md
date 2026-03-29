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

Expected packages:
- `cmake`
- `ninja-build`
- `g++`
- `gcc-arm-linux-gnueabihf`
- `g++-arm-linux-gnueabihf`
- `python3`

Current local fact:
- this Windows machine does not currently have WSL installed
- this Windows machine does not currently have a direct ARM Linux cross compiler on PATH

So this task establishes the supported baseline and scripts, but host-side validation still depends on installing a suitable build host/toolchain.

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
cmake -S . -B build-armv6 -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/armv6-rpi1-linux-gnueabihf.cmake

cmake --build build-armv6
```

Expected output:
- `build-armv6/apps/runtime-probe/pi_fartbox_runtime_probe`

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
- install or provision a Linux/WSL cross-build host
- validate the ARMv6 toolchain file
- build the runtime probe
- deploy and run it on the Pi
