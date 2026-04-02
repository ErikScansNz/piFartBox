param(
  [string]$BuildDir = "build-armv6",
  [string]$ToolchainFile = "cmake/toolchains/armv6-rpi1-linux-gnueabihf.cmake",
  [string]$Distro = "Ubuntu-22.04",
  [string]$TargetSysroot = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$wslCmd = Get-Command wsl -ErrorAction SilentlyContinue
if (-not $wslCmd) {
  throw "WSL is not installed. Install WSL or use a Linux host with arm-linux-gnueabihf-g++."
}

$repoPath = (Resolve-Path ".").Path
$linuxRepoPath = "/mnt/" + $repoPath.Substring(0,1).ToLower() + $repoPath.Substring(2).Replace("\","/")
$linuxBuildDir = $BuildDir
$linuxToolchain = $ToolchainFile
$linuxTargetSysroot = ""
if ($TargetSysroot) {
  $resolvedSysroot = (Resolve-Path $TargetSysroot).Path
  $linuxTargetSysroot = "/mnt/" + $resolvedSysroot.Substring(0,1).ToLower() + $resolvedSysroot.Substring(2).Replace("\","/")
}

$bashCommand = @"
set -e
if ! command -v arm-linux-gnueabihf-g++ >/dev/null 2>&1; then
  echo 'error: arm-linux-gnueabihf-g++ not found in WSL' >&2
  exit 1
fi
READELF=readelf
if command -v arm-linux-gnueabihf-readelf >/dev/null 2>&1; then
  READELF=arm-linux-gnueabihf-readelf
fi
cd '$linuxRepoPath'
cmake -E rm -rf '$linuxBuildDir'
cmake -S . -B '$linuxBuildDir' -G Ninja -DCMAKE_TOOLCHAIN_FILE='$linuxToolchain'$(if ($linuxTargetSysroot) { " -DPI_FARTBOX_TARGET_SYSROOT='$linuxTargetSysroot'" })
cmake --build '$linuxBuildDir'
if [ -f '$linuxBuildDir/apps/runtime/pi_fartbox_runtime' ]; then
  if ! "$READELF" -A '$linuxBuildDir/apps/runtime/pi_fartbox_runtime' | grep -q 'Tag_CPU_arch: v6'; then
    echo 'error: built runtime is not tagged for ARMv6' >&2
    "$READELF" -A '$linuxBuildDir/apps/runtime/pi_fartbox_runtime' >&2 || true
    exit 2
  fi
fi
"@

wsl -d $Distro -- bash -lc $bashCommand
