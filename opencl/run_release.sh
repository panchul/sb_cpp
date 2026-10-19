#!/usr/bin/env bash
set -eu

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

if [[ -x "$SCRIPT_DIR/build/release/fft_opencl" ]]; then
  exec "$SCRIPT_DIR/build/release/fft_opencl"
fi

echo "Release build not found. Run ./build_release.sh first." >&2
exit 1
