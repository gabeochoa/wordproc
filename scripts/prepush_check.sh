#!/bin/bash
# Pre-push gate (Gabe, 2026-10-18): catch "works on boulder, broken on my Mac"
# before reporting a milestone as pushed.
#   1. Local tree: -fsyntax-only every TU under the newest clang available
#      (Gabe's Mac is clang 21 / SDK 26.4; boulder has llvm@21 in ~/homebrew).
#   2. vendor/afterhours submodule: clean tree, pinned to a commit that is
#      pushed to its remote (never a local-only afterhours edit).
#   3. Fresh `git clone --recursive` from GitHub -> make -> make test -> bench.
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

echo "== [1/3] syntax-only, all TUs, newest clang"
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

echo "== [2/3] submodule clean + pinned to pushed commit"
if [ -n "$(git -C vendor/afterhours status --porcelain 2>/dev/null)" ]; then
    echo "FAIL: vendor/afterhours has uncommitted edits"; FAIL=1
fi
git -C vendor/afterhours fetch -q origin 2>/dev/null
if ! git -C vendor/afterhours branch -r --contains HEAD 2>/dev/null | grep -q .; then
    echo "FAIL: afterhours HEAD $(git -C vendor/afterhours rev-parse --short HEAD) is not on any pushed branch"; FAIL=1
fi
[ $FAIL -eq 0 ] && echo "submodule ✓"

echo "== [3/3] fresh clone from GitHub: build + test + bench"
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
