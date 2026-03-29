# Target Compatibility Decision

This document records the current compatibility decision for the first live `piFartBox` target.

## Observed Target Facts
From the live Raspberry Pi at `192.168.1.26`:

- hostname: `zynthian`
- OS: `Raspbian GNU/Linux 9 (stretch)`
- kernel: `4.14.30+`
- architecture: `armv6l`
- package architecture: `armhf`
- libc: `glibc 2.24`
- Python default: `2.7.13`
- `cmake`: `3.7.2`
- `g++`: `6.3.0`
- `git`: `2.11.0`

Additional warning sign:
- the running `zynthian.service` logs repeated `Illegal instruction` failures from `zynthian_gui.py`

## Current Repo Assumptions
`piFartBox` currently assumes a much newer build environment:

- `cmake_minimum_required(VERSION 3.20)`
- modern C++ baseline
- modern desktop-like development workflow

That means the current target cannot natively configure/build the repo as-is.

## Options Considered
### Option 1: Relax `piFartBox` to the current legacy image
Lower the repo baseline until it builds on:
- `cmake 3.7.2`
- `g++ 6.3.0`
- old Stretch-era userland

Pros:
- simplest story if we insist on native compilation on the Pi
- no cross-toolchain setup needed at first

Cons:
- drags the whole repo architecture backward
- constrains language/library choices early
- ties development to an already-aging host image
- does not address the current `Illegal instruction` warning signs in the image

### Option 2: Full in-place system modernization on the Pi
Attempt to upgrade the running Zynthian image/toolchain in place.

Pros:
- could eventually produce a more capable native build host
- might align better with newer repo assumptions

Cons:
- highest risk to the current working appliance install
- an old Pi 1 B+ is a poor place to rely on heavy native rebuild workflows
- a blanket `apt full-upgrade` on this image is not a reliable toolchain strategy
- could destabilize the already-running Zynthian stack further

### Option 3: Modern host build + legacy target deploy
Treat the Pi as a runtime appliance target, not the primary compiler box.

Pros:
- keeps `piFartBox` free to use a modern toolchain locally
- avoids forcing the whole repo down to Stretch-era constraints
- matches the repo’s existing git-bundle and deploy-oriented workflow
- reduces risk to the current Zynthian image

Cons:
- requires an explicit cross-build/deploy setup
- adds one more build profile to document and maintain

## Recommendation
Recommended path: **Option 3, modern host build + legacy target deploy**.

This should be the repository default for now.

Reasoning:
- the target image is old enough that native compilation should be treated as a compatibility bonus, not the foundation
- the target already shows runtime instability signs
- the Pi 1 B+ is better treated as a constrained ARMv6 runtime appliance than as the primary dev/build machine
- this preserves the most architectural freedom in `piFartBox`

## Explicit Recommendation on Full System Update
Do **not** start with a blind `apt full-upgrade`.

If we choose to modernize the image later, that should be a deliberate migration task with:
- SD-card backup first
- a defined rollback path
- explicit awareness that Zynthian itself may be impacted

For now, a full system update is **not** the recommended first move.

## Repository Decision
For the current project phase:

- the live Pi is a **legacy ARMv6 runtime target**
- `piFartBox` should prefer a **cross-compile + deploy** model
- native build on the current image is not the primary supported path
- toolchain modernization on the box remains an optional later task, not the baseline

## Next Implementation Task
The next concrete task should be:
- `TASK-0007` `Cross-compile and deploy baseline`

That task should define:
- target ABI/triplet assumptions
- cross-build profile or toolchain file
- how built outputs are deployed to `/opt/piFartBox`
- how this integrates with the updater and bootstrap workflow
