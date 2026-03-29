# WSL Cross-Build Host

## Supported Baseline
- Host OS: Windows with WSL enabled
- Supported distro: `Ubuntu 22.04`
- Repository path in Windows: `C:\piFartBox`
- Repository path in WSL: `/mnt/c/piFartBox`
- Validated on: 2026-03-29

## Required Packages
- `build-essential`
- `cmake`
- `ninja-build`
- `pkg-config`
- `git`
- `python3`
- `python3-pip`
- `gcc-arm-linux-gnueabihf`
- `g++-arm-linux-gnueabihf`

## Validation Commands
```bash
cmake --version
ninja --version
arm-linux-gnueabihf-g++ --version
cd /mnt/c/piFartBox
./scripts/build/cross_build_armv6.sh
```

Windows wrapper:
```powershell
.\scripts\build\cross_build_armv6.ps1
```

## Observed Validation Baseline
- `cmake 3.22.1`
- `ninja 1.10.1`
- `arm-linux-gnueabihf-g++ 11.4.0`
- first verified build target:
  - `build-armv6/apps/runtime-probe/pi_fartbox_runtime_probe`
- verified entrypoints:
  - `./scripts/build/cross_build_armv6.sh`
  - `.\scripts\build\cross_build_armv6.ps1`

## Notes
- The WSL host is the preferred local build environment for the first ARMv6 cross-builds.
- The live Pi remains the runtime target and deployment target.
- If the repo moves to a different Windows path, update this document and any helper scripts that assume `/mnt/c/piFartBox`.
