# Provisioning Usage

## Why Provisioning Was Reworked
The original SSH wrapper buffered remote stdout and stderr until the provisioning command finished. On a Raspberry Pi 1 B+, long `apt` operations can take a long time, which made the process look frozen even when it was still working.

## Current Behavior
- the Python wrapper now streams remote output live
- the target shell script prints explicit step markers
- the operator can choose the apt behavior:
  - `full-upgrade`
  - `install-only`
  - `skip`

## Recommended First Run
For a fresh Raspberry Pi OS image, start with `install-only` so we can validate package install, service setup, nginx, and filesystem layout without waiting for a full distro upgrade.

```powershell
python scripts\provision\bootstrap_pi_os.py --host <pi-ip> --user <user> --password <password> --apt-mode install-only
```

## Apt Modes
- `full-upgrade`
  - runs `apt-get update`, `apt-get -y full-upgrade`, then installs the package baseline
- `install-only`
  - runs `apt-get update`, then installs the package baseline
- `skip`
  - skips apt work entirely and only applies filesystem, nginx, and systemd scaffolding

## Typical Follow-Up
After provisioning succeeds, deploy a runtime artifact and verify:
- `/opt/piFartBox`
- `/var/lib/piFartBox`
- `/var/log/piFartBox`
- `nginx`
- `pifartbox-runtime.service`
