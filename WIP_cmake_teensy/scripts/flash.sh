#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
HEX_DIR="$ROOT/build/teensy41-release/projects/teensy"
LOADER="$ROOT/tools/teensy_loader_cli/teensy_loader_cli"

usage() {
    echo "Usage: $(basename "$0") <hex-file|keyword>"
    echo
    echo "Flash a Teensy 4.1 with a HEX file."
    echo
    echo "The argument can be:"
    echo "  - The exact HEX filename"
    echo "  - A keyword/substring matching a HEX filename"
    echo
    echo "Examples:"
    echo "  $(basename "$0") projects_teensy_emitter.hex"
    echo "  $(basename "$0") emitter"
    echo "  $(basename "$0") 4channel"
    echo
    echo "Available HEX files:"
    find "$HEX_DIR" -maxdepth 1 -type f -name '*.hex' -printf '  %f\n' | sort
}

if [[ $# -eq 0 ]]; then
    usage
    exit 0
fi

if [[ ! -x "$LOADER" ]]; then
    echo "Error: teensy_loader_cli not found: $LOADER" >&2
    exit 1
fi

if [[ ! -d "$HEX_DIR" ]]; then
    echo "Error: HEX directory not found: $HEX_DIR" >&2
    exit 1
fi

QUERY="$1"

# First try an exact filename match.
EXACT_FILE="$HEX_DIR/$QUERY"

if [[ -f "$EXACT_FILE" ]]; then
    HEX_FILE="$EXACT_FILE"
else
    # Otherwise, look for filenames containing the provided keyword.
    mapfile -t MATCHES < <(
        find "$HEX_DIR" -maxdepth 1 -type f -name '*.hex' -printf '%f\n' |
        grep -iF -- "$QUERY" |
        sort
    )

    if [[ ${#MATCHES[@]} -eq 0 ]]; then
        echo "Error: no HEX files matching '$QUERY'." >&2
        echo >&2
        echo "Available HEX files:" >&2
        find "$HEX_DIR" -maxdepth 1 -type f -name '*.hex' -printf '  %f\n' |
            sort >&2
        exit 1
    fi

    if [[ ${#MATCHES[@]} -gt 1 ]]; then
        echo "Error: '$QUERY' matches multiple HEX files:" >&2
        printf '  %s\n' "${MATCHES[@]}" >&2
        echo >&2
        echo "Please provide a more specific keyword." >&2
        exit 1
    fi

    HEX_FILE="$HEX_DIR/${MATCHES[0]}"
fi

echo "Flashing: $(basename "$HEX_FILE")"

"$LOADER" \
    --mcu=TEENSY41 \
    -w -v `# -r` \
    "$HEX_FILE"
