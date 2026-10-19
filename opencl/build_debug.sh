#!/usr/bin/env bash
set -eu

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug --config Debug
