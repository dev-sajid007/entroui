#!/usr/bin/env bash
set -euo pipefail

PRESET="${1:-debug}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

cd "${ROOT}"

if [[ ! -f "build/${PRESET}/CMakeCache.txt" ]]; then
    "${ROOT}/scripts/build.sh" "${PRESET}"
fi

echo "==> Running tests (${PRESET})"
ctest --preset "${PRESET}" --output-on-failure
