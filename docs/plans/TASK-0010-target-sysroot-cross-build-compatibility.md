# TASK-0010 - Target Sysroot Cross-Build Compatibility

## Objective
Make the ARMv6 cross-build output ABI-compatible with the live Pi image by introducing a target-synced sysroot into the cross-build flow.

## Success Criteria
- The repo can capture a target-compatible sysroot from the live Pi.
- The ARMv6 toolchain file can build against that sysroot.
- The rebuilt runtime probe no longer requires newer GLIBC or GLIBCXX versions than the Pi provides.
- The runtime probe executes successfully on the live Pi after redeployment.
- The repo documents the validated sysroot-based deployment baseline.

## Constraints
- Preserve the existing Zynthian installation and running services.
- Keep the Pi as a runtime appliance, not the primary build host.
- Avoid requiring a full system upgrade on the Pi.
- Keep the sysroot workflow explicit and versioned in repo docs/scripts.

## Implementation Design
- create a script that captures the relevant target sysroot content from the Pi into a local `sysroots/` directory
- extend the ARMv6 toolchain file to accept a `PI_FARTBOX_TARGET_SYSROOT`
- prefer target-compatible linking and lookup paths driven by the captured sysroot
- rebuild the runtime probe against the synced sysroot
- redeploy and verify the binary runs on the Pi

## Interfaces / Types Affected
- cross-build toolchain configuration
- deployment and sysroot helper scripts
- deployment documentation
- task tracking docs

## Test Plan
- capture a sysroot snapshot from the live Pi
- verify the cross-build configure step sees the sysroot
- verify the rebuilt binary links against target-compatible runtime versions
- verify remote execution succeeds on the Pi

## Assumptions / Defaults
- live target remains `192.168.1.26`
- deploy root remains `/opt/piFartBox`
- sysroot snapshots live under a local `sysroots/` directory

## Out of Scope
- building a complete SDK or toolchain from source
- changing the Pi OS image
- final service registration
