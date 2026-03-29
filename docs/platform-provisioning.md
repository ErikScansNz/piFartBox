# Platform Provisioning

## Base OS
- Target image: `Raspberry Pi OS Lite 32-bit`
- Build model: cross-compile on WSL, deploy artifacts to the Pi
- Target install root: `/opt/piFartBox`

## Provisioning Goals
- update the base system packages
- install the minimum runtime/admin package set
- create the `piFartBox` install, state, and log directories
- install a runtime systemd unit
- install an nginx site for the local browser surface

## Package Baseline
- `ca-certificates`
- `curl`
- `git`
- `jq`
- `rsync`
- `unzip`
- `python3`
- `python3-venv`
- `avahi-daemon`
- `alsa-utils`
- `libasound2`
- `nginx-light`
- `udisks2`
- `dosfstools`
- `exfatprogs`
- `ntfs-3g`

## Audio Baseline
- first audio subsystem: direct ALSA
- first utility package: `alsa-utils`
- JACK and PREEMPT_RT tuning are deferred until the runtime is stable on the fresh OS baseline

## Installed Paths
- source checkout: `/opt/piFartBox`
- deployed runtime artifacts: `/opt/piFartBox/runtime/<revision>`
- active runtime symlink: `/opt/piFartBox/runtime-current`
- state root: `/var/lib/piFartBox`
- log root: `/var/log/piFartBox`
- nginx web root: `/opt/piFartBox/web`

## Installed Services
- `pifartbox-runtime.service`
  - points at `/opt/piFartBox/runtime-current/bin/pi_fartbox_runtime`
  - starts only when a deployed runtime exists
  - runs as the configured target login user from provisioning
- `nginx`
  - serves the local browser surface
  - reserves `/api/` for the local control/runtime API

## Provisioning Entry Points
- remote wrapper:
  - `scripts/provision/bootstrap_pi_os.py`
- target shell script:
  - `scripts/provision/provision_pi_os.sh`

## Notes
- The provisioning step intentionally installs scaffolding before the final runtime exists.
- This allows package, service, and web-serving setup to be validated as soon as the SD card is ready.
