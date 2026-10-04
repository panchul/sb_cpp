#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build/debug"
EXECUTABLE="${BUILD_DIR}/vulkan_sandbox"

if [[ ! -x "${EXECUTABLE}" ]]; then
  echo "Debug executable not found: ${EXECUTABLE}" >&2
  echo "Run ./build_debug.sh first." >&2
  exit 1
fi

OS_NAME="$(uname -s)"
if [[ "${OS_NAME}" == "Darwin" ]]; then
  arch="$(uname -m)"
  if [[ "${arch}" == "arm64" ]]; then
    BREW_PREFIX="/opt/homebrew"
  else
    BREW_PREFIX="/usr/local"
  fi

  export VK_ICD_FILENAMES="${BREW_PREFIX}/opt/molten-vk/etc/vulkan/icd.d/MoltenVK_icd.json"
  export VK_LAYER_PATH="${BREW_PREFIX}/opt/vulkan-validationlayers/share/vulkan/explicit_layer.d"
elif [[ "${OS_NAME}" == "Linux" ]]; then
  :
else
  echo "This launcher is for macOS or Linux. On Windows, use ./run_debug.ps1 instead." >&2
  exit 1
fi

exec "${EXECUTABLE}"
