#!/usr/bin/env bash
set -euo pipefail

TARGET="keyboards/sekigon/keyboard_quantizer/mini/keymaps/vial/keymap.h"
RESTORE_COMMIT="f786d43769"
CREATED=0

cleanup() {
  if [[ "$CREATED" == "1" ]]; then
    rm -f "$TARGET"
  fi
}
trap cleanup EXIT

if [[ ! -f "$TARGET" ]]; then
  mkdir -p "$(dirname "$TARGET")"
  git show "${RESTORE_COMMIT}:${TARGET}" > "$TARGET"
  CREATED=1
fi

arm-none-eabi-gcc --version | head -n 1 || true
make sekigon/keyboard_quantizer/mini:vial:uf2
