#!/usr/bin/env bash
set -euo pipefail

BASELINE="4de02c1aeec5a34f0744a97213499441c508d565"
TARGET="keyboards/sekigon/keyboard_quantizer/mini/keymaps/vial/keymap.h"

# The Feather project was bootstrapped directly from this upstream commit.
# Fail early if the expected baseline is not in repository history.
git cat-file -e "${BASELINE}^{commit}"

# Current pinned upstream already contains the Vial keymap header. Do not
# synthesize or restore files into the protected upstream tree during CI.
if [[ ! -f "$TARGET" ]]; then
  echo "Expected upstream file is missing: $TARGET" >&2
  exit 1
fi

arm-none-eabi-gcc --version | head -n 1 || true
make sekigon/keyboard_quantizer/mini:vial:uf2
