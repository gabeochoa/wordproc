#!/bin/bash
# Load-time benchmark for Wordproc
# Measures cold start time for each test file

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
OUTPUT_DIR="$PROJECT_DIR/output"
BENCHMARK_DIR="$OUTPUT_DIR/benchmarks"
EXECUTABLE="$OUTPUT_DIR/ui_tester.exe"
TEST_FILES_DIR="$PROJECT_DIR/test_files/public_domain"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Target: 100ms cold start
TARGET_MS=100

echo "=== Wordproc Load-Time Benchmark ==="
echo ""

# Check if executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${YELLOW}Building application...${NC}"
    cd "$PROJECT_DIR" && make
fi

# Create benchmark output directory
mkdir -p "$BENCHMARK_DIR"

# CSV report header
REPORT_FILE="$BENCHMARK_DIR/load_times_$(date +%Y%m%d_%H%M%S).csv"
echo "filename,size_bytes,startup_time_ms,pass_fail" > "$REPORT_FILE"

echo "Target: <= ${TARGET_MS}ms cold start"
echo ""
echo "Running benchmarks..."
echo ""

TOTAL_PASS=0
TOTAL_FAIL=0

# Test each file in the public_domain directory
for file in "$TEST_FILES_DIR"/*.txt; do
    if [ -f "$file" ]; then
        FILENAME=$(basename "$file")
        FILESIZE=$(stat -f%z "$file" 2>/dev/null || stat -c%s "$file" 2>/dev/null)
        
        # Run the app in test mode and capture output
        cd "$OUTPUT_DIR"
        OUTPUT=$(./ui_tester.exe --test-mode --frame-limit 3 "$file" 2>&1 || true)
        
        # Extract startup time from output
        STARTUP_TIME=$(echo "$OUTPUT" | grep "Startup time:" | sed 's/.*: \([0-9]*\) ms/\1/')
        
        if [ -z "$STARTUP_TIME" ]; then
            STARTUP_TIME="0"
        fi
        
        # Determine pass/fail
        if [ "$STARTUP_TIME" -le "$TARGET_MS" ]; then
            PASS_FAIL="PASS"
            TOTAL_PASS=$((TOTAL_PASS + 1))
            STATUS="${GREEN}PASS${NC}"
        else
            PASS_FAIL="FAIL"
            TOTAL_FAIL=$((TOTAL_FAIL + 1))
            STATUS="${RED}FAIL${NC}"
        fi
        
        # Write to CSV
        echo "$FILENAME,$FILESIZE,$STARTUP_TIME,$PASS_FAIL" >> "$REPORT_FILE"
        
        # Print result
        printf "  %-30s %6d bytes  %4d ms  %b\n" "$FILENAME" "$FILESIZE" "$STARTUP_TIME" "$STATUS"
    fi
done

echo ""
echo "=== Summary ==="
echo "  Passed: $TOTAL_PASS"
echo "  Failed: $TOTAL_FAIL"
echo ""
echo "Report saved to: $REPORT_FILE"

# Compare with baseline if it exists
BASELINE_FILE="$BENCHMARK_DIR/baseline.csv"
if [ -f "$BASELINE_FILE" ]; then
    echo ""
    echo "=== Baseline Comparison ==="
    echo "(Comparing with $BASELINE_FILE)"
    
    # Simple comparison - just check if any times increased
    tail -n +2 "$REPORT_FILE" | while IFS=',' read -r fname size time status; do
        BASELINE_TIME=$(grep "^$fname," "$BASELINE_FILE" | cut -d',' -f3)
        if [ -n "$BASELINE_TIME" ] && [ "$time" -gt "$((BASELINE_TIME + 10))" ]; then
            echo -e "  ${YELLOW}REGRESSION${NC}: $fname ($BASELINE_TIME ms -> $time ms)"
        fi
    done
fi

echo ""
if [ "$TOTAL_FAIL" -gt 0 ]; then
    echo -e "${RED}Some tests failed the ${TARGET_MS}ms target!${NC}"
    exit 1
else
    echo -e "${GREEN}All tests passed!${NC}"
fi
