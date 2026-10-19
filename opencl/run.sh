#!/usr/bin/env bash
set -eu

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

if [[ -x "$SCRIPT_DIR/build/fft_opencl" ]]; then
  exec "$SCRIPT_DIR/build/fft_opencl"
fi

if [[ -x "$SCRIPT_DIR/build/release/fft_opencl" ]]; then
  exec "$SCRIPT_DIR/build/release/fft_opencl"
fi

if [[ -x "$SCRIPT_DIR/build/debug/fft_opencl" ]]; then
  exec "$SCRIPT_DIR/build/debug/fft_opencl"
fi

echo "No built FFT binary found. Run build.sh, build_debug.sh, or build_release.sh first." >&2
exit 1
