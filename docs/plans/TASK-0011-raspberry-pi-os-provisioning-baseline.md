# TASK-0011 - Raspberry Pi OS Provisioning Baseline

## Objective
Define and scaffold the first clean provisioning path for a fresh Raspberry Pi OS target, including package install/update flow, install roots, service layout, and the initial audio subsystem baseline.

## Success Criteria
- The repo documents the chosen Raspberry Pi OS deployment baseline clearly.
- A repeatable provisioning script exists for package updates and package installation.
- The repo includes local service scaffolding for the runtime and local web/control serving path.
- The repo defines the initial audio subsystem baseline for the fresh Pi image.
- The new provisioning workflow is reflected in repo docs and task tracking.

## Constraints
- Target hardware remains Raspberry Pi 1 B+.
- The build host remains WSL-based cross-compile, not native target build.
- The target setup should be simple enough to reproduce from a freshly flashed SD card.
- The provisioning flow should not assume ZynthianOS-specific services or directories.

## Implementation Design
- choose fresh `Raspberry Pi OS Lite 32-bit` as the target base
- define the target package/update baseline for:
  - SSH/connectivity
  - deployment/update support
  - audio runtime support
  - local static web serving
- create a provisioning script that:
  - updates apt metadata
  - performs an upgrade
  - installs the required packages
  - creates install directories
  - installs systemd unit files
  - installs the local web server config
- keep the initial audio path ALSA-first
- install service scaffolding that points at `/opt/piFartBox/runtime-current`

## Interfaces / Types Affected
- repository operating assumptions
- provisioning scripts
- service definitions
- platform and deployment docs

## Test Plan
- validate the provisioning script syntax locally
- validate the remote wrapper can upload and invoke the provisioning script
- verify the installed service files and web config land in the expected locations
- verify the package baseline matches the documented assumptions

## Assumptions / Defaults
- target OS is fresh `Raspberry Pi OS Lite 32-bit`
- install root remains `/opt/piFartBox`
- runtime symlink remains `/opt/piFartBox/runtime-current`
- static web root is served from the repository install tree

## Out of Scope
- final engine binary deployment
- final audio latency tuning
- PREEMPT_RT kernel work
