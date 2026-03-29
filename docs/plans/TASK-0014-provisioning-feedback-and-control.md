# TASK-0014 - Provisioning Feedback and Control

## Objective
Make the Raspberry Pi provisioning workflow observable and operator-friendly so long-running package setup no longer appears hung.

## Success Criteria
- The SSH provisioning wrapper streams remote output live instead of buffering until completion.
- The target shell script prints clear step markers for package update, upgrade, install, filesystem setup, and service configuration.
- Operators can choose whether provisioning performs a full upgrade, a package install only, or skips apt work entirely.
- Repository docs explain the new behavior and recommended usage.

## Constraints
- Keep the provisioning model simple enough for first-boot Raspberry Pi setup.
- Do not remove the existing package baseline or service install behavior.
- Preserve compatibility with password-based SSH for the first setup flow.

## Implementation Design
- replace the buffered SSH command helper with a streaming implementation using a PTY-enabled Paramiko channel
- send the sudo password through stdin to the remote command rather than piping with `echo`
- add a reusable shell `log_step()` helper and step-level progress output to `provision_pi_os.sh`
- add an `--apt-mode` argument to the Python wrapper and matching `APT_MODE` handling in the shell script:
  - `full-upgrade`
  - `install-only`
  - `skip`
- document when to use each mode and why `install-only` is often the safest first run on fresh Pi OS

## Interfaces / Types Affected
- `scripts/provision/bootstrap_pi_os.py`
- `scripts/provision/provision_pi_os.sh`
- `docs/platform-provisioning.md`
- `README.md`

## Test Plan
- `python -m py_compile` passes for the Python wrapper
- `bash -n` passes for the shell script
- local wrapper `--help` shows the new apt mode option
- live provisioning shows streamed step output on the terminal instead of waiting silently

## Assumptions / Defaults
- first-boot provisioning may take a long time on Raspberry Pi 1 B+
- `install-only` is a useful operator default for faster first validation
- SSH is already enabled on the target image

## Out of Scope
- replacing Paramiko with another transport
- fully unattended image baking
- artifact deployment or runtime binary rollout
