#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

# Priority:
#
# 1. CLI argument
# 2. CMake Tools environment
# 3. Default

if [[ $# -ge 1 ]]; then
    PRESET="$1"
elif [[ -n "${CMAKE_BUILD_PRESET:-}" ]]; then
    PRESET="$CMAKE_BUILD_PRESET"
elif [[ -n "${CMAKE_CONFIGURE_PRESET:-}" ]]; then
    PRESET="$CMAKE_CONFIGURE_PRESET"
else
    PRESET="teensy41-release"
fi

echo "Building preset: $PRESET"

cmake --preset "$PRESET"
cmake --build --preset "$PRESET"