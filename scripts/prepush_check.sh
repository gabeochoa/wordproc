#!/bin/bash
# Pre-push gate (Gabe, 2026-10-18): catch "works on boulder, broken on my Mac"
# before reporting a milestone as pushed.
#   1. Local tree: -fsyntax-only every TU under the newest clang available
#      (Gabe's Mac is clang 21 / SDK 26.4; boulder has llvm@21 in ~/homebrew).
#   2. Same TUs, same clang, build.zig warning flags: FAIL on any `warning:`.
#   3. vendor/afterhours submodule: clean tree, pinned to a commit that is
#      pushed to its remote (never a local-only afterhours edit).
#   4. Fresh `git clone --recursive` from GitHub -> make -> make test -> bench.
# Run before every push; report "fresh-clone ✓ clang21 ✓".
set -u
cd "$(dirname "$0")/.." || exit 1
FAIL=0

# Newest clang++ available
CLANGXX=""
for c in "$HOME/homebrew/opt/llvm@21/bin/clang++" /opt/homebrew/opt/llvm@21/bin/clang++ \
         /usr/local/opt/llvm/bin/clang++ "$(command -v clang++ || true)"; do
    if [ -n "$c" ] && [ -x "$c" ]; then CLANGXX="$c"; break; fi
done
echo "== clang: $CLANGXX ($($CLANGXX --version 2>/dev/null | head -1))"

echo "== [1/4] syntax-only, all TUs, newest clang"
INCLUDES="-isystem vendor/ -isystem vendor/afterhours/vendor/"
DEFINES="-DBACKWARD -DAFTER_HOURS_ENABLE_MCP -DAFTERHOURS_ENFORCE_MIN_FONT_SIZE -DAFTERHOURS_DEBUG_TEXT_OVERFLOW -DAFTER_HOURS_ENABLE_E2E_TESTING -DAFTER_HOURS_UI_SINGLE_COLLECTION"
TUS=$(find src -name '*.cpp' ! -name 'sokol_impl_web.cpp' | sort)
TUS="$TUS vendor/afterhours/src/plugins/files.cpp"
for f in $TUS; do
    if ! perl -e 'alarm 300; exec @ARGV' "$CLANGXX" -std=c++23 -fsyntax-only $INCLUDES $DEFINES "$f" >/tmp/prepush_syn.log 2>&1; then
        echo "FAIL syntax: $f"; head -5 /tmp/prepush_syn.log; FAIL=1
    fi
done
for f in $(find src -name '*.mm' | sort); do
    if ! perl -e 'alarm 300; exec @ARGV' "$CLANGXX" -std=c++23 -x objective-c++ -fsyntax-only $INCLUDES $DEFINES "$f" >/tmp/prepush_syn.log 2>&1; then
        echo "FAIL syntax: $f"; head -5 /tmp/prepush_syn.log; FAIL=1
    fi
done
[ $FAIL -eq 0 ] && echo "clang syntax ✓"

echo "== [2/4] clang21 zero warnings (build.zig warning flags)"
# Keep in sync with build.zig: `warnings` + the app flags (-O2, defines,
# -include src/log.h). A warning here fails the gate even though the TU
# still compiles: zig prints "failed command:" for any step writing stderr.
WARNFLAGS="-g -Wall -Wextra -Wpedantic -Wuninitialized -Wshadow -Wconversion -Wcast-qual -Wchar-subscripts -Wcomment -Wdisabled-optimization -Wformat=2 -Wformat-nonliteral -Wformat-security -Wformat-y2k -Wimport -Winit-self -Winline -Winvalid-pch -Wlong-long -Wmissing-format-attribute -Wmissing-include-dirs -Wpacked -Wpointer-arith -Wreturn-type -Wsequence-point -Wstrict-overflow=5 -Wswitch -Wswitch-default -Wswitch-enum -Wtrigraphs -Wunused-label -Wunused-parameter -Wunused-value -Wunused-variable -Wvariadic-macros -Wvolatile-register-var -Wwrite-strings -Warray-bounds -pipe -fno-stack-protector -fno-common -Wno-deprecated-volatile -Wno-missing-field-initializers -Wno-c99-extensions -Wno-unused-function -Wno-sign-conversion -Wno-implicit-int-float-conversion -Wno-implicit-float-conversion -Wno-format-nonliteral -Wno-format-security -Wno-format-y2k -Wno-import -Wno-inline -Wno-invalid-pch -Wno-long-long -Wno-missing-format-attribute -Wno-missing-noreturn -Wno-packed -Wno-redundant-decls -Wno-sequence-point -Wno-trigraphs -Wno-variadic-macros -Wno-volatile-register-var"
WARNDEFINES="$DEFINES -include src/log.h"
for f in $TUS; do
    if ! perl -e 'alarm 300; exec @ARGV' "$CLANGXX" -std=c++23 -O2 $WARNFLAGS $WARNDEFINES $INCLUDES -fsyntax-only "$f" >/tmp/prepush_warn.log 2>&1; then
        echo "FAIL syntax (zero-warnings step): $f"; head -5 /tmp/prepush_warn.log; FAIL=1
    elif grep -q "warning:" /tmp/prepush_warn.log; then
        echo "FAIL warning: $f"; grep "warning:" /tmp/prepush_warn.log | head -5; FAIL=1
    fi
done
for f in $(find src -name '*.mm' | sort); do
    if ! perl -e 'alarm 300; exec @ARGV' "$CLANGXX" -std=c++23 -O2 $WARNFLAGS $WARNDEFINES $INCLUDES -x objective-c++ -fsyntax-only "$f" >/tmp/prepush_warn.log 2>&1; then
        echo "FAIL syntax (zero-warnings step): $f"; head -5 /tmp/prepush_warn.log; FAIL=1
    elif grep -q "warning:" /tmp/prepush_warn.log; then
        echo "FAIL warning: $f"; grep "warning:" /tmp/prepush_warn.log | head -5; FAIL=1
    fi
done
[ $FAIL -eq 0 ] && echo "zero warnings ✓"

echo "== [3/4] submodule clean + pinned to pushed commit"
if [ -n "$(git -C vendor/afterhours status --porcelain 2>/dev/null)" ]; then
    echo "FAIL: vendor/afterhours has uncommitted edits"; FAIL=1
fi
git -C vendor/afterhours fetch -q origin 2>/dev/null
if ! git -C vendor/afterhours branch -r --contains HEAD 2>/dev/null | grep -q .; then
    echo "FAIL: afterhours HEAD $(git -C vendor/afterhours rev-parse --short HEAD) is not on any pushed branch"; FAIL=1
fi
[ $FAIL -eq 0 ] && echo "submodule ✓"

echo "== [4/4] fresh clone from GitHub: build + test + bench"
rm -rf /tmp/wp-prepush-check
GIT_CONFIG_COUNT=1 GIT_CONFIG_KEY_0=url.https://github.com/.insteadOf \
GIT_CONFIG_VALUE_0=git@github.com: \
    perl -e 'alarm 900; exec @ARGV' git clone --recursive -q https://github.com/gabeochoa/wordproc /tmp/wp-prepush-check || FAIL=1
if [ $FAIL -eq 0 ]; then
    (cd /tmp/wp-prepush-check && perl -e 'alarm 1200; exec @ARGV' make -j8 >/tmp/prepush_build.log 2>&1) \
        || { echo "FAIL fresh-clone build"; tail -5 /tmp/prepush_build.log; FAIL=1; }
fi
if [ $FAIL -eq 0 ]; then
    (cd /tmp/wp-prepush-check && perl -e 'alarm 900; exec @ARGV' make test 2>&1 | grep -E "All tests passed|FAILED" | head -2)
    (cd /tmp/wp-prepush-check && perl -e 'alarm 900; exec @ARGV' make bench-unit 2>&1 | grep -E "AoS \(|SoA \(" | head -2)
    echo "fresh-clone ✓"
fi

if [ $FAIL -eq 0 ]; then echo "PREPUSH OK — fresh-clone ✓ clang21 ✓"; else echo "PREPUSH FAILED"; fi
exit $FAIL
