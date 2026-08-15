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

"$cc" \
  -std=c99 \
  -Wall -Wextra -Werror \
  -I"$ROOT_DIR/tests/host/stubs" \
  -I"$ROOT_DIR/System" \
  "$ROOT_DIR/System/ByteRing.c" \
  "$ROOT_DIR/System/Timebase.c" \
  "$ROOT_DIR/tests/host/test_boundaries.c" \
  -o "$BUILD_DIR/boundary_tests"

"$BUILD_DIR/boundary_tests"

"$cc" \
  -std=c99 \
  -Wall -Wextra -Werror \
  "$ROOT_DIR/tests/host/test_format_contract.c" \
  -o "$BUILD_DIR/format_contract_tests"

"$BUILD_DIR/format_contract_tests"
