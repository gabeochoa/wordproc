#!/bin/bash
# Load-time regression benchmark
# Measures cold start time for opening various test files

set -e

# Configuration
WORDPROC="./output/ui_tester.exe"
TEST_DIR="test_files/public_domain"
OUTPUT_DIR="output/perf"
REPORT_FILE="$OUTPUT_DIR/load_times.csv"
BASELINE_FILE="$OUTPUT_DIR/baseline.csv"
TARGET_MS=100

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# Ensure output directory exists
mkdir -p "$OUTPUT_DIR"

# Check if app exists
if [ ! -f "$WORDPROC" ]; then
    echo "Error: $WORDPROC not found. Run 'make' first."
    exit 1
fi

# CSV header
echo "filename,size_bytes,cold_start_ms,ready_to_interact_ms,pass_fail,timestamp" > "$REPORT_FILE"

echo "=============================================="
echo "Load-Time Regression Benchmark"
echo "=============================================="
echo "Target: ${TARGET_MS}ms cold start"
echo "Test files: $TEST_DIR"
echo ""

pass_count=0
fail_count=0
total_count=0

# Process each test file
for file in "$TEST_DIR"/*.txt "$TEST_DIR"/*.md "$TEST_DIR"/*.wpdoc 2>/dev/null; do
    [ -f "$file" ] || continue
    
    filename=$(basename "$file")
    size_bytes=$(stat -f%z "$file" 2>/dev/null || stat -c%s "$file" 2>/dev/null || echo "0")
    
    # Run the app in test mode with frame limit of 5 (enough to measure startup)
    # Capture stdout which includes startup time
    start_time=$(date +%s%N 2>/dev/null || python3 -c "import time; print(int(time.time()*1e9))")
    
    output=$("$WORDPROC" --test-mode --frame-limit 5 --screenshot-dir "$OUTPUT_DIR/screenshots" "$file" 2>&1 || true)
    
    end_time=$(date +%s%N 2>/dev/null || python3 -c "import time; print(int(time.time()*1e9))")
    
    # Extract startup time from app output
    startup_ms=$(echo "$output" | grep "Startup time:" | sed 's/.*: \([0-9]*\) ms.*/\1/' || echo "")
    
    if [ -z "$startup_ms" ]; then
        # Fallback: calculate from wall clock (less accurate due to process overhead)
        elapsed_ns=$((end_time - start_time))
        startup_ms=$((elapsed_ns / 1000000))
    fi
    
    # Ready-to-interact is same as startup for now (first interactive frame)
    ready_ms=$startup_ms
    
    # Check against target
    if [ "$startup_ms" -le "$TARGET_MS" ]; then
        pass_fail="PASS"
        status_color=$GREEN
        ((pass_count++)) || true
    else
        pass_fail="FAIL"
        status_color=$RED
        ((fail_count++)) || true
    fi
    ((total_count++)) || true
    
    # Human-readable size
    if [ "$size_bytes" -ge 1048576 ]; then
        size_human=$(echo "scale=1; $size_bytes / 1048576" | bc)MB
    elif [ "$size_bytes" -ge 1024 ]; then
        size_human=$(echo "scale=1; $size_bytes / 1024" | bc)KB
    else
        size_human="${size_bytes}B"
    fi
    
    # Write to CSV
    timestamp=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
    echo "$filename,$size_bytes,$startup_ms,$ready_ms,$pass_fail,$timestamp" >> "$REPORT_FILE"
    
    # Console output
    printf "  %-35s %10s  %5sms  %b%s%b\n" "$filename" "$size_human" "$startup_ms" "$status_color" "$pass_fail" "$NC"
done

echo ""
echo "=============================================="
echo "Summary: $pass_count/$total_count passed (target: ${TARGET_MS}ms)"
echo "Report saved to: $REPORT_FILE"

# Compare against baseline if it exists
if [ -f "$BASELINE_FILE" ]; then
    echo ""
    echo "Comparing against baseline..."
    
    while IFS=, read -r baseline_file baseline_size baseline_cold baseline_ready baseline_pf baseline_ts; do
        [ "$baseline_file" = "filename" ] && continue
        
        current_cold=$(grep "^$baseline_file," "$REPORT_FILE" | cut -d, -f3)
        if [ -n "$current_cold" ] && [ -n "$baseline_cold" ]; then
            diff=$((current_cold - baseline_cold))
            if [ "$diff" -gt 10 ]; then
                printf "  ${RED}REGRESSION${NC}: %s: %dms -> %dms (+%dms)\n" "$baseline_file" "$baseline_cold" "$current_cold" "$diff"
            elif [ "$diff" -lt -10 ]; then
                printf "  ${GREEN}IMPROVEMENT${NC}: %s: %dms -> %dms (%dms)\n" "$baseline_file" "$baseline_cold" "$current_cold" "$diff"
            fi
        fi
    done < "$BASELINE_FILE"
fi

echo ""
echo "To create a baseline: cp $REPORT_FILE $BASELINE_FILE"

# Exit with failure if any tests failed
if [ "$fail_count" -gt 0 ]; then
    echo ""
    echo -e "${RED}$fail_count file(s) exceeded ${TARGET_MS}ms target${NC}"
    exit 1
fi

exit 0
