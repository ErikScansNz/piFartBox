# TASK-0015 - Native On-Device Build Pipeline

## Objective
Add a robust fallback deployment path that copies the repository to the Pi, installs missing native build dependencies, builds on-device, and promotes the resulting runtime artifact into the standard runtime deployment layout.

## Success Criteria
- A single operator script can package the local repo, upload it to the Pi, and extract it into a native build source root.
- The script performs preflight checks and installs missing native build dependencies on the Pi when requested.
- The script configures and builds `pi_fartbox_runtime` on-device with low-risk defaults for Raspberry Pi 1 B+.
- The resulting runtime is copied into `/opt/piFartBox/runtime/<revision>-native/` and linked via `/opt/piFartBox/runtime-current`.
- The flow is documented in the repo and validated at least through preflight and command-line help.

## Constraints
- Keep this as a fallback path, not the new primary build model.
- Avoid relying on git clone or heavy network chatter on the Pi; prefer a single uploaded archive.
- Preserve the existing runtime layout and systemd expectations.
- Default to conservative build settings for a 1-core, low-memory target.

## Implementation Design
- create a Python deploy helper that:
  - archives the local repo while excluding build output, caches, sysroots, and VCS noise
  - uploads the archive to a staging directory on the Pi
  - runs a streamed remote shell session under sudo
  - installs missing native build packages if requested
  - extracts the source tree into a dedicated native source root
  - configures a native build directory with `cmake` and `Ninja`
  - builds `pi_fartbox_runtime` with a default parallelism of `1`
  - publishes the built runtime into the normal runtime artifact layout
- add a companion doc describing when to use this fallback versus the preferred cross-build path

## Interfaces / Types Affected
- `scripts/deploy/build_on_device.py`
- deployment and build docs
- task tracking docs

## Test Plan
- `python -m py_compile` passes for the new deploy helper
- the helper `--help` output is correct
- a live preflight run completes on the target Pi
- if practical, a full native build run produces a promoted runtime artifact

## Assumptions / Defaults
- target host remains `192.168.1.26`
- target runtime root remains `/opt/piFartBox/runtime`
- the default native build generator is `Ninja`
- default parallelism is `1`

## Out of Scope
- replacing the primary cross-build baseline
- browser UI implementation
- ALSA runtime integration details
