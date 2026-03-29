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
