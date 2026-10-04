#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

arch="$(uname -m)"
if [[ "${arch}" == "arm64" ]]; then
  BREW_PREFIX="/opt/homebrew"
else
  BREW_PREFIX="/usr/local"
fi

export VK_ICD_FILENAMES="${BREW_PREFIX}/opt/molten-vk/etc/vulkan/icd.d/MoltenVK_icd.json"
export VK_LAYER_PATH="${BREW_PREFIX}/opt/vulkan-validationlayers/share/vulkan/explicit_layer.d"

"${BUILD_DIR}/vulkan_sandbox"
