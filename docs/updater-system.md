# Updater System

This document defines the repository-supported update path for deployed `piFartBox` systems.

## Summary
The primary updater workflow is:

1. export an offline update bundle from the development machine
2. copy the bundle to a USB flash drive
3. insert the USB drive into the deployed Pi
4. run the Pi-side updater script
5. verify the new deployed revision

This is a USB-first update system, not a network-first or USB gadget/device workflow.

## Why USB Flash Drive Instead of Direct USB Device Upload
For Raspberry Pi 1 B+, the practical simple USB path is USB mass storage media.

`piFartBox` should not rely on plugging the Pi into another computer as a USB gadget/device for code upload. The Pi 1 B+ model is not a good foundation for that workflow, so the updater must assume:

- the Pi is a USB host
- updates arrive on removable USB storage

See also:
- [reference-docs.md](./reference-docs.md)
- Raspberry Pi OTG / hardware references listed there

## Update Payload Format
The expected USB drive layout is:

```text
piFartBox-update/
  manifest.json
  piFartBox.bundle
```

### `manifest.json`
The manifest should contain:
- `project`
- `branch`
- `commit`
- `bundle`
- `created_at`

Example:

```json
{
  "project": "piFartBox",
  "branch": "main",
  "commit": "e90213b",
  "bundle": "piFartBox.bundle",
  "created_at": "2026-03-29T17:00:00Z"
}
```

### Git bundle
The bundle should be a standard git bundle created from the source repository. This allows:
- offline transport
- history-preserving updates
- simpler auditability than copying loose working files

## Default Paths
- development repo root:
  - developer-local clone
- deployed repo root:
  - `/opt/piFartBox`
- mounted USB update root:
  - `/media/usb0/piFartBox-update`

These paths may be overridden in scripts, but these are the repository defaults.

## Host-Side Workflow
Use the host-side bundle-export script to:
- create a git bundle from the selected branch
- write `manifest.json`
- copy both files to the chosen USB target path

Planned host-side script:
- `scripts/update/create_git_bundle.ps1`

Expected host operator flow:

```text
1. update local source repo
2. run bundle export script
3. point the script at the mounted USB drive
4. eject the USB drive cleanly
```

## Pi-Side Workflow
Use the Pi-side install script to:
- locate the mounted USB update folder
- validate required files
- record current deployed HEAD
- create a backup branch or tag
- import the git bundle
- update the deployed branch to the bundled revision
- write a deployed revision record

Planned Pi-side script:
- `scripts/update/install_from_usb.sh`

Expected Pi operator flow:

```text
1. insert USB drive
2. mount or confirm mount at /media/usb0
3. run install_from_usb.sh
4. review deployed revision output
5. restart services if required
```

## Backup and Rollback Rules
Before switching revisions, the updater should preserve the current deployed state.

Minimum rollback requirements:
- record the current commit hash
- create a local backup ref before moving the deployed branch
- keep the previous update media until the new deployment is verified

Recommended local backup ref format:

```text
backup/pre-usb-update-YYYYMMDD-HHMMSS
```

## Future Extensions
This updater system may later gain:
- service stop/start hooks
- post-update verification checks
- signed manifests
- optional network update path

Those are additive. The repository standard remains USB-first for the initial deployed system.
