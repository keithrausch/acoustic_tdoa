#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

PRESET="${1:-host-debug}"

echo "Testing preset: $PRESET"

# Configure the build directory if necessary
cmake --preset "$PRESET"

# Build the tests
cmake --build --preset "$PRESET"

# Run the tests
ctest --preset "$PRESET" --output-on-failure