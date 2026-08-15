#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="$ROOT_DIR/tests/host/build"
mkdir -p "$BUILD_DIR"

cc=${CC:-cc}
"$cc" \
  -std=c99 \
  -Wall -Wextra -Werror \
  -I"$ROOT_DIR/tests/host/stubs" \
  -I"$ROOT_DIR/User" \
  -I"$ROOT_DIR/System" \
  "$ROOT_DIR/User/ActionTask.c" \
  "$ROOT_DIR/User/SafetyTask.c" \
  "$ROOT_DIR/tests/host/test_state_machine.c" \
  -lm \
  -o "$BUILD_DIR/state_machine_tests"

"$BUILD_DIR/state_machine_tests"
