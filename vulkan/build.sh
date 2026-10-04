#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MODE="${1:-debug}"
MODE_LOWER="$(printf '%s' "${MODE}" | tr '[:upper:]' '[:lower:]')"

case "${MODE_LOWER}" in
  debug)
    exec "${SCRIPT_DIR}/build_debug.sh"
    ;;
  release)
    exec "${SCRIPT_DIR}/build_release.sh"
    ;;
  *)
    echo "Usage: $0 [debug|release]" >&2
    exit 1
    ;;
esac
