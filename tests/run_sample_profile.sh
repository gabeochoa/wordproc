#!/bin/bash
# Sampling profiler for startup
# Uses macOS `sample` to capture a short profile of the running app.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
OUTPUT_DIR="$PROJECT_DIR/output"
EXECUTABLE="$OUTPUT_DIR/wordproc.exe"
REPORT_DIR="$OUTPUT_DIR/perf"
PROFILE_FILE="$REPORT_DIR/sample_startup.txt"
APP_LOG="$REPORT_DIR/sample_app.log"
SAMPLE_LOG="$REPORT_DIR/sample_error.log"

# Tunables (override via env)
DURATION_SEC="${DURATION_SEC:-5}"
FRAME_LIMIT="${FRAME_LIMIT:-0}"
LOAD_FILE="${LOAD_FILE:-}"

echo "=== Wordproc Sampling Profile ==="
echo ""

if [ ! -f "$EXECUTABLE" ]; then
    echo "Error: $EXECUTABLE not found. Run 'make' first."
    exit 1
fi

mkdir -p "$REPORT_DIR"

if ! command -v sample >/dev/null 2>&1; then
    echo "Error: 'sample' not found (macOS only)."
    exit 1
fi

SAMPLE_CMD=(sample)
if [ "${SUDO:-0}" = "1" ]; then
    SAMPLE_CMD=(sudo sample)
fi

if [ "$FRAME_LIMIT" -le 0 ]; then
    # Keep the app alive long enough to sample (assume up to ~120 FPS).
    min_frames=$((DURATION_SEC * 120))
    if [ "$min_frames" -lt 300 ]; then
        min_frames=300
    fi
    FRAME_LIMIT="$min_frames"
fi

cmd=("$EXECUTABLE" --test-mode --frame-limit="$FRAME_LIMIT")
if [ -n "$LOAD_FILE" ]; then
    cmd+=("$LOAD_FILE")
fi

echo "Duration: ${DURATION_SEC}s"
echo "Frame limit: ${FRAME_LIMIT}"
if [ -n "$LOAD_FILE" ]; then
    echo "Load file: $LOAD_FILE"
else
    echo "Load file: (none)"
fi
echo ""

rm -f "$PROFILE_FILE" "$SAMPLE_LOG"

"${cmd[@]}" >"$APP_LOG" 2>&1 &
app_pid=$!

cleanup() {
    if kill -0 "$app_pid" >/dev/null 2>&1; then
        kill "$app_pid" >/dev/null 2>&1 || true
    fi
}
trap cleanup EXIT

for _ in {1..10}; do
    if kill -0 "$app_pid" >/dev/null 2>&1; then
        break
    fi
    sleep 0.1
done

echo "Sampling PID $app_pid..."
"${SAMPLE_CMD[@]}" "$app_pid" "$DURATION_SEC" -file "$PROFILE_FILE" >"$SAMPLE_LOG" 2>&1
sample_status=$?

wait "$app_pid" >/dev/null 2>&1 || true

if [ ! -s "$PROFILE_FILE" ]; then
    echo "Error: sample output not created."
    echo "sample exit code: $sample_status"
    if [ -s "$SAMPLE_LOG" ]; then
        echo "sample stderr:"
        tail -n 20 "$SAMPLE_LOG"
    fi
    if grep -q "Operation not permitted" "$SAMPLE_LOG" 2>/dev/null; then
        echo "Hint: grant Developer Tools permission to your terminal app"
        echo "      (System Settings > Privacy & Security > Developer Tools)."
    fi
    if grep -q "try running with \\`sudo\\`" "$SAMPLE_LOG" 2>/dev/null; then
        echo "Hint: if you still can’t sample, try SUDO=1."
    fi
    exit 1
fi

echo ""
echo "Profile saved to: $PROFILE_FILE"
echo "App log saved to: $APP_LOG"
echo "Done."

