#!/bin/bash
# Launch-time benchmark (interactive)
# Measures wall-clock startup time by launching the app in test mode.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
OUTPUT_DIR="$PROJECT_DIR/output"
EXECUTABLE="$OUTPUT_DIR/wordproc.exe"
TEST_DIR="$PROJECT_DIR/test_files/public_domain"
REPORT_DIR="$OUTPUT_DIR/perf"
REPORT_FILE="$REPORT_DIR/launch_times.csv"

# Tunables (override via env)
# Note: Wall time includes process startup, rendering frames, and cleanup
# Internal startup is ~400-550ms, but wall time is higher due to:
#   - Process initialization overhead
#   - Frame rendering (2 frames)
#   - Window and resource cleanup
# 2000ms is realistic for wall time, accounting for cold cache variance
TARGET_MS="${TARGET_MS:-2000}"
ITERATIONS="${ITERATIONS:-3}"
FRAME_LIMIT="${FRAME_LIMIT:-2}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=== Wordproc Launch Benchmark ==="
echo ""

# Check if executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${YELLOW}Building application...${NC}"
    cd "$PROJECT_DIR" && make
fi

mkdir -p "$REPORT_DIR"

# Create a small test file
mkdir -p "$OUTPUT_DIR/test_files"
SMALL_FILE="$OUTPUT_DIR/test_files/small.txt"
echo "Hello, World!" > "$SMALL_FILE"

# Find the largest test file
LARGEST_FILE=""
LARGEST_SIZE=0
for file in "$TEST_DIR"/*.txt "$TEST_DIR"/*.md "$TEST_DIR"/*.wpdoc; do
    [ -f "$file" ] || continue
    size=$(stat -f%z "$file" 2>/dev/null || stat --printf="%s" "$file" 2>/dev/null)
    if [ "$size" -gt "$LARGEST_SIZE" ]; then
        LARGEST_SIZE=$size
        LARGEST_FILE=$file
    fi
done

if [ -z "$LARGEST_FILE" ]; then
    echo -e "${YELLOW}No test files found in $TEST_DIR; skipping large-file scenario.${NC}"
fi

echo "Target wall time: ${TARGET_MS}ms"
echo "Iterations: $ITERATIONS"
echo "Frame limit: $FRAME_LIMIT"
echo ""

EXECUTABLE="$EXECUTABLE" \
OUTPUT_DIR="$OUTPUT_DIR" \
REPORT_FILE="$REPORT_FILE" \
TARGET_MS="$TARGET_MS" \
ITERATIONS="$ITERATIONS" \
FRAME_LIMIT="$FRAME_LIMIT" \
SMALL_FILE="$SMALL_FILE" \
LARGEST_FILE="$LARGEST_FILE" \
python3 - <<'PY'
import csv
import os
import re
import statistics
import subprocess
import time
from pathlib import Path

exec_path = Path(os.environ["EXECUTABLE"])
output_dir = Path(os.environ["OUTPUT_DIR"])
report_file = Path(os.environ["REPORT_FILE"])
target_ms = float(os.environ["TARGET_MS"])
iterations = int(os.environ["ITERATIONS"])
frame_limit = int(os.environ["FRAME_LIMIT"])
small_file = os.environ.get("SMALL_FILE", "")
largest_file = os.environ.get("LARGEST_FILE", "")
screenshot_dir = output_dir / "perf" / "launch_screenshots"

timing_re = re.compile(r"^\[INFO\]\s+(?P<name>.+?) took (?P<ms>[0-9.]+) ms$")

def parse_timers(output: str) -> dict:
    timers = {}
    for line in output.splitlines():
        match = timing_re.match(line.strip())
        if match:
            timers[match.group("name")] = float(match.group("ms"))
    return timers

scenarios = [
    {"name": "blank", "file": ""},
    {"name": "small", "file": small_file},
]
if largest_file:
    scenarios.append({"name": "large", "file": largest_file})

report_file.parent.mkdir(parents=True, exist_ok=True)
with report_file.open("w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow([
        "scenario",
        "file",
        "size_bytes",
        "iteration",
        "exit_code",
        "wall_ms",
        "settings_ms",
        "preload_ms",
        "ui_ms",
        "pass_fail",
    ])

    failures = 0
    summary = {}

    for scenario in scenarios:
        name = scenario["name"]
        file_path = scenario["file"]
        size_bytes = 0
        if file_path:
            try:
                size_bytes = Path(file_path).stat().st_size
            except FileNotFoundError:
                size_bytes = 0

        wall_times = []
        for i in range(1, iterations + 1):
            cmd = [
                str(exec_path),
                "--test-mode",
                f"--frame-limit={frame_limit}",
                f"--screenshot-dir={screenshot_dir}",
            ]
            if file_path:
                cmd.append(file_path)

            start = time.perf_counter()
            proc = subprocess.run(
                cmd,
                cwd=output_dir,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
            end = time.perf_counter()
            wall_ms = (end - start) * 1000.0
            timers = parse_timers(proc.stdout)

            settings_ms = timers.get("Settings load", "")
            preload_ms = timers.get("Preload and singletons", "")
            ui_ms = timers.get("UI context init", "")

            pass_fail = "PASS" if wall_ms <= target_ms and proc.returncode == 0 else "FAIL"
            if pass_fail == "FAIL":
                failures += 1

            writer.writerow([
                name,
                file_path,
                size_bytes,
                i,
                proc.returncode,
                f"{wall_ms:.3f}",
                settings_ms if settings_ms != "" else "",
                preload_ms if preload_ms != "" else "",
                ui_ms if ui_ms != "" else "",
                pass_fail,
            ])

            wall_times.append(wall_ms)

        summary[name] = wall_times

    print("=== Launch Timing Summary ===")
    summary_failures = 0
    for name, times in summary.items():
        avg = statistics.mean(times)
        p50 = statistics.median(times)
        p90 = statistics.quantiles(times, n=10)[8] if len(times) >= 2 else times[0]
        # Use median (p50) for pass/fail - more robust to outliers than average
        status = "PASS" if p50 <= target_ms else "FAIL"
        if status == "FAIL":
            summary_failures += 1
        print(
            f"  {name:6s} avg={avg:.1f}ms p50={p50:.1f}ms p90={p90:.1f}ms "
            f"(target {target_ms:.0f}ms) {status}"
        )

    print("")
    print(f"Report saved to: {report_file}")

    # Exit based on summary pass/fail (median-based), not individual iteration failures
    if summary_failures > 0:
        raise SystemExit(1)
PY

exit 0

