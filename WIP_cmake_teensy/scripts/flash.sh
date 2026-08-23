#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"

if ! command -v "$ROOT/tools/teensy_loader_cli/teensy_loader_cli" >/dev/null; then
    echo "teensy_loader_cli not found"
    exit 1
fi

# HEX_FILE="projects_teensy_perf.hex"
HEX_FILE="projects_teensy_4channel.hex"
# HEX_FILE="projects_teensy_emitter.hex"

"$ROOT/tools/teensy_loader_cli/teensy_loader_cli" \
    --mcu=TEENSY41 \
    -w -v `# -r`\
    "$ROOT/build/teensy41-release/projects/teensy/$HEX_FILE"