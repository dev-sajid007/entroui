#!/usr/bin/env bash
set -euo pipefail

PRESET="${1:-debug}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_TYPE="Debug"
if [[ "${PRESET}" == "release" ]]; then
    BUILD_TYPE="Release"
fi

cd "${ROOT}"

echo "==> Installing Conan dependencies (${PRESET})"
conan install . \
    --build=missing \
    -s build_type="${BUILD_TYPE}" \
    -s compiler.cppstd=20 \
    -of "build/${PRESET}"

echo "==> Configuring CMake (${PRESET})"
cmake --preset "${PRESET}"

echo "==> Building (${PRESET})"
cmake --build --preset "${PRESET}" -j "$(nproc)"

echo "==> Done: build/${PRESET}"
