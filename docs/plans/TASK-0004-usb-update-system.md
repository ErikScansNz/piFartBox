# TASK-0004 - USB-First Codebase Updater System

## Objective
Define and scaffold a simple, repeatable updater path for deployed `piFartBox` systems, using USB media as the primary offline transport for codebase updates.

## Success Criteria
- The repository contains a documented updater workflow for deployed Pi systems.
- The updater explicitly uses USB mass storage as the primary field-update transport.
- The repo includes host-side and Pi-side update script scaffolds.
- `AGENTS.md` documents the updater requirement and maintenance rules.
- The task is tracked in the repo task system.

## Constraints
- Raspberry Pi 1 B+ should not be treated as a USB gadget/device update target.
- The workflow should not depend on network access for normal field updates.
- The workflow should preserve a backup/rollback path.
- The updater should work with the repository being git-tracked.

## Implementation Design
### Transport choice
- Use a USB flash drive as the default update transport.
- Use a git bundle as the primary update payload so updates preserve repository history and can be applied offline.

### Host-side flow
- Export a bundle from the development machine onto a USB drive.
- Write a small manifest file alongside the bundle to record branch, commit, and bundle name.

### Pi-side flow
- Mount the USB drive.
- Validate the presence of the expected bundle and manifest.
- Backup the current deployed repo state.
- Fetch/import the git bundle into the deployed repo.
- Fast-forward or reset the deployed branch to the bundled revision in a controlled way.
- Record the deployed revision.

### Fallbacks
- Keep the workflow compatible with future network-based update flows, but do not make them the primary path.

## Interfaces / Types Affected
- Deployment and maintenance docs
- Updater shell/PowerShell scripts
- Update manifest format

## Test Plan
- Verify the host-side script writes a bundle and manifest to a chosen USB path.
- Verify the Pi-side script validates missing-path and missing-bundle cases cleanly.
- Verify the update doc describes backup and rollback expectations.

## Assumptions / Defaults
- Default deployed repo root will be `/opt/piFartBox`.
- Default update media root will contain a `piFartBox-update/` folder.
- Git is available on the deployed Pi.

## Out of Scope
- Full service restart integration
- Package-manager-based updates
- OTA update infrastructure
