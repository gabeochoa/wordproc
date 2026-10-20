#!/bin/bash
# Lean-build guard: `zig build -De2e=false` drops src/testing/e2e_runner.cpp
# and omits -DAFTER_HOURS_ENABLE_E2E_TESTING, so src/main.cpp must not leave
# dangling references to the E2E runner. This check compiles main.cpp with
# the lean flag set and asserts the object has no undefined E2E symbols
# (pre-gating main.cpp left 2: e2e::initializeRunner / initializeRunnerBatch).
# -O0 and no -g are check-only speedups: the assertion is about symbols,
# not codegen. The other flags mirror build.zig's app flags minus the E2E
# define; keep in sync with build.zig.
set -euo pipefail
cd "$(dirname "$0")/.."

grep -q 'b.option(bool, "e2e"' build.zig || {
  echo "FAIL: build.zig declares no e2e option"
  exit 1
}
grep -q 'if (e2e) flags.append' build.zig || {
  echo "FAIL: build.zig does not gate AFTER_HOURS_ENABLE_E2E_TESTING on e2e"
  exit 1
}
grep -q 'if (!e2e and std.mem.eql(u8, src, "src/testing/e2e_runner.cpp")) continue' build.zig || {
  echo "FAIL: build.zig does not drop e2e_runner.cpp when e2e=false"
  exit 1
}

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
"${CXX:-clang++}" -std=c++23 -O0 -DBACKWARD -include src/log.h \
  -DAFTERHOURS_ENFORCE_MIN_FONT_SIZE -DAFTER_HOURS_UI_SINGLE_COLLECTION \
  -DAFTER_HOURS_ENABLE_MCP -DAFTERHOURS_DEBUG_TEXT_OVERFLOW \
  -isystem vendor -isystem vendor/afterhours/vendor \
  -c src/main.cpp -o "$TMP/main.o"

if nm -u "$TMP/main.o" | grep -E 'E2ERunner|_ZN3e2e' > "$TMP/e2e_syms.txt"; then
  echo "FAIL: main.cpp references E2E symbols in a lean build:"
  cat "$TMP/e2e_syms.txt"
  exit 1
fi
echo "OK: lean main.cpp has no dangling E2E symbols"
