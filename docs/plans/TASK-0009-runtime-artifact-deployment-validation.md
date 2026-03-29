# TASK-0009 - Runtime Artifact Deployment Validation

## Objective
Validate the first concrete end-to-end deployment path by building the ARMv6 runtime probe locally, deploying it to the live Pi, and confirming it executes correctly from the deployed runtime artifact location.

## Success Criteria
- The runtime probe is built successfully from the supported WSL host baseline.
- The artifact deployment script transfers the built binary to the live Pi.
- The live Pi exposes the deployed artifact under the expected runtime directory and symlink layout.
- The deployed runtime probe executes successfully on the Pi and reports expected subsystem metadata.
- The validated deployment flow is documented in the repo.

## Constraints
- Preserve the existing Zynthian installation and services.
- Do not rely on native build tooling on the Pi.
- Keep deployed runtime artifacts separate from the source checkout.
- Use the current live Pi target at `192.168.1.26`.

## Implementation Design
- build `pi_fartbox_runtime_probe` using the documented WSL cross-build baseline
- deploy the binary with `scripts/deploy/deploy_runtime_artifact.py`
- verify:
  - runtime artifact directory creation
  - `runtime-current` symlink update
  - remote manifest and binary presence
  - successful probe execution on the Pi
- document the validated operator flow and any adjustments discovered during deployment

## Interfaces / Types Affected
- deployment scripts
- cross-compile and deployment docs
- task tracking docs

## Test Plan
- run the PowerShell WSL cross-build wrapper successfully
- run the runtime artifact deploy script successfully
- execute the deployed runtime probe remotely on the Pi
- verify the remote output includes expected subsystem summary fields

## Assumptions / Defaults
- build host baseline from `TASK-0008` is available
- deploy root remains `/opt/piFartBox`
- runtime artifact root remains `/opt/piFartBox/runtime`

## Out of Scope
- service registration or systemd integration
- final synth runtime deployment
- browser/control-api serving setup
