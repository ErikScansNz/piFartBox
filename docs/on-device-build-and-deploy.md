# On-Device Build And Deploy

## Purpose
This is the fallback deployment path for cases where cross-built artifacts are not yet target-compatible or when a native ARMv6 validation build is needed quickly on the Raspberry Pi itself.

## Operator Script
- `scripts/deploy/build_on_device.py`

## What It Does
- archives the local repository into a single upload artifact
- uploads that archive to the Pi
- checks for native build dependencies
- optionally installs missing dependencies
- extracts a clean native source tree on the Pi
- configures and builds `pi_fartbox_runtime` on-device
- publishes the built runtime into:
  - `/opt/piFartBox/runtime/<commit>-native/`
  - `/opt/piFartBox/runtime-current`

## Recommended Usage
For the current Pi 1 B+ baseline, use conservative settings:

```powershell
python scripts\deploy\build_on_device.py --host 192.168.1.26 --user erik --password <password> --parallel 1 --dependency-mode install
```

## Useful Modes
- preflight only:

```powershell
python scripts\deploy\build_on_device.py --host 192.168.1.26 --user erik --password <password> --preflight-only
```

- restart the runtime service after a successful native build:

```powershell
python scripts\deploy\build_on_device.py --host 192.168.1.26 --user erik --password <password> --parallel 1 --restart-service
```

## Position In The Overall Pipeline
- preferred path:
  - WSL cross-build + deploy
- fallback path:
  - full source copy + native build on device

This fallback exists because the Pi is slow but trustworthy for final ARMv6 compatibility, while the cross-build path still needs target-userland compatibility work.
