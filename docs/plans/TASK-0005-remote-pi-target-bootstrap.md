# TASK-0005 - Remote Pi Target Bootstrap

## Objective
Bootstrap the first live `piFartBox` deployment root on the flashed Raspberry Pi target without disturbing the existing Zynthian installation.

## Success Criteria
- The target host facts are recorded in a compact bootstrap note.
- The repository includes a repeatable remote bootstrap script.
- The live Pi receives an initial `piFartBox` checkout under `/opt/piFartBox`.
- The deployment path uses repository-native git history transfer rather than loose manual copies.
- The bootstrap does not modify the existing `/zynthian` runtime tree.

## Constraints
- Preserve the active Zynthian installation and services.
- Use the provided network host and credentials only for bootstrap work.
- Prefer repository-native transport that matches the updater story.
- Keep the remote install rooted at `/opt/piFartBox`.

## Implementation Design
### Remote facts capture
- Record the observed host facts:
  - hostname
  - OS/kernel
  - architecture
  - available toolchain
  - current Zynthian directories and service state

### Deployment transport
- Create a local git bundle from the current `piFartBox` repo.
- Upload the bundle and manifest to the target over SSH/SFTP.
- Initialize or update `/opt/piFartBox` from the uploaded bundle.
- Use a user-writable remote staging path under `/home/pi` for uploaded bundle assets.

### Initial remote setup
- Ensure `/opt/piFartBox` exists and is a git repo on the target.
- Ensure ownership is usable by the runtime/admin workflow.
- Write a deployed revision marker after bootstrap.

### Verification
- Confirm the expected repo files exist on the Pi.
- Confirm `git`, `cmake`, and `g++` are present on the host.
- If practical, run a first `cmake` configure/build on the Pi after bootstrap.

## Interfaces / Types Affected
- Deployment scripts
- Bootstrap documentation
- Task index / completed-task history

## Test Plan
- Verify SSH connectivity and sudo access.
- Verify bundle upload and checkout to `/opt/piFartBox`.
- Verify the deployed repo revision matches the local source revision.
- Verify bootstrap leaves `/zynthian` untouched.

## Assumptions / Defaults
- Target host is `192.168.1.26`.
- Default SSH credentials are `pi` / `raspberry`.
- Install root is `/opt/piFartBox`.

## Out of Scope
- Full service integration with Zynthian
- Audio-engine runtime bring-up
- Browser deployment
