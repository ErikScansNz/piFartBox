#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build-armv6}"
GENERATOR="${GENERATOR:-Ninja}"
TOOLCHAIN_FILE="${TOOLCHAIN_FILE:-cmake/toolchains/armv6-rpi1-linux-gnueabihf.cmake}"
TARGET_SYSROOT="${TARGET_SYSROOT:-}"

if ! command -v cmake >/dev/null 2>&1; then
  echo "error: cmake is required on the build host" >&2
  exit 1
fi

if ! command -v arm-linux-gnueabihf-g++ >/dev/null 2>&1; then
  echo "error: arm-linux-gnueabihf-g++ is required on the build host" >&2
  exit 1
fi

READELF="${READELF:-}"
if [[ -z "$READELF" ]]; then
  if command -v arm-linux-gnueabihf-readelf >/dev/null 2>&1; then
    READELF="arm-linux-gnueabihf-readelf"
  else
    READELF="readelf"
  fi
fi

cmake -E rm -rf "$BUILD_DIR"

cmake_args=(
  -S .
  -B "$BUILD_DIR"
  -G "$GENERATOR"
  -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE"
)

if [[ -n "$TARGET_SYSROOT" ]]; then
  cmake_args+=("-DPI_FARTBOX_TARGET_SYSROOT=$TARGET_SYSROOT")
fi

cmake "${cmake_args[@]}"
cmake --build "$BUILD_DIR"

RUNTIME_BIN="$BUILD_DIR/apps/runtime/pi_fartbox_runtime"
if [[ -f "$RUNTIME_BIN" ]]; then
  if ! "$READELF" -A "$RUNTIME_BIN" | grep -q "Tag_CPU_arch: v6"; then
    echo "error: built runtime is not tagged for ARMv6" >&2
    "$READELF" -A "$RUNTIME_BIN" >&2 || true
    exit 2
  fi
fi
