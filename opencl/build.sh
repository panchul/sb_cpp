#!/usr/bin/env bash
set -eu

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
