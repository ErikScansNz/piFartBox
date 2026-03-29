param(
  [string]$BuildDir = "build-armv6",
  [string]$ToolchainFile = "cmake/toolchains/armv6-rpi1-linux-gnueabihf.cmake",
  [string]$Distro = "Ubuntu-22.04"
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

$bashCommand = @"
set -e
if ! command -v arm-linux-gnueabihf-g++ >/dev/null 2>&1; then
  echo 'error: arm-linux-gnueabihf-g++ not found in WSL' >&2
  exit 1
fi
cd '$linuxRepoPath'
cmake -S . -B '$linuxBuildDir' -G Ninja -DCMAKE_TOOLCHAIN_FILE='$linuxToolchain'
cmake --build '$linuxBuildDir'
"@

wsl -d $Distro -- bash -lc $bashCommand
