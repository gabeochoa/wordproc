#!/bin/bash
# Load-time regression benchmark
# Measures cold start time for opening various test files
# Uses --benchmark mode (headless, no window)

set -e

# Configuration
WORDPROC="./output/wordproc.exe"
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
echo "filename,size_bytes,lines,chars,load_ms,total_ms,pass_fail,timestamp" > "$REPORT_FILE"

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
shopt -s nullglob 2>/dev/null || true  # For bash compatibility
for file in "$TEST_DIR"/*.txt "$TEST_DIR"/*.md "$TEST_DIR"/*.wpdoc; do
    [ -f "$file" ] || continue
    
    filename=$(basename "$file")
    
    # Run the app in benchmark mode (headless, no window)
    output=$("$WORDPROC" --benchmark "$file" 2>&1 || true)
    
    # Parse the output
    # Format: file=...,size=...,lines=...,chars=...,load_ms=...,total_ms=...,target=100,pass=true/false
    size_bytes=$(echo "$output" | sed 's/.*size=\([0-9]*\).*/\1/')
    lines=$(echo "$output" | sed 's/.*lines=\([0-9]*\).*/\1/')
    chars=$(echo "$output" | sed 's/.*chars=\([0-9]*\).*/\1/')
    load_ms=$(echo "$output" | sed 's/.*load_ms=\([0-9.]*\).*/\1/')
    total_ms=$(echo "$output" | sed 's/.*total_ms=\([0-9.]*\).*/\1/')
    pass_raw=$(echo "$output" | sed 's/.*pass=\([a-z]*\).*/\1/')
    
    # Human-readable size
    if [ "$size_bytes" -ge 1048576 ]; then
        size_human=$(echo "scale=1; $size_bytes / 1048576" | bc)MB
    elif [ "$size_bytes" -ge 1024 ]; then
        size_human=$(echo "scale=1; $size_bytes / 1024" | bc)KB
    else
        size_human="${size_bytes}B"
    fi
    
    # Check against target
    if [ "$pass_raw" = "true" ]; then
        pass_fail="PASS"
        status_color=$GREEN
        ((pass_count++)) || true
    else
        pass_fail="FAIL"
        status_color=$RED
        ((fail_count++)) || true
    fi
    ((total_count++)) || true
    
    # Write to CSV
    timestamp=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
    echo "$filename,$size_bytes,$lines,$chars,$load_ms,$total_ms,$pass_fail,$timestamp" >> "$REPORT_FILE"
    
    # Console output
    printf "  %-35s %10s  %8.2fms  %b%s%b\n" "$filename" "$size_human" "$total_ms" "$status_color" "$pass_fail" "$NC"
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
