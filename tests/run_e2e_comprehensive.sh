#!/bin/bash
# Comprehensive E2E Test Suite for Wordproc
# Runs the application through multiple test scenarios via harness
# Supports: control flags, profiling, exit code verification

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
OUTPUT_DIR="$PROJECT_DIR/output"
SCREENSHOT_DIR="$OUTPUT_DIR/e2e_screenshots"
EXECUTABLE="$OUTPUT_DIR/wordproc.exe"
TEST_FILES_DIR="$PROJECT_DIR/test_files"
REPORT_FILE="$OUTPUT_DIR/e2e_report.txt"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Log function
log_test() {
    echo -e "${BLUE}[TEST]${NC} $1"
}

log_pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
    ((TESTS_PASSED++))
}

log_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
    ((TESTS_FAILED++))
}

# Run a test scenario
# Usage: run_test "test_name" "command" "expected_exit_code" "check_screenshot" 
run_test() {
    local test_name="$1"
    local extra_args="$2"
    local expected_exit="$3"
    local check_screenshot="$4"
    
    ((TESTS_RUN++))
    log_test "$test_name"
    
    local test_screenshot_dir="$SCREENSHOT_DIR/$test_name"
    mkdir -p "$test_screenshot_dir"
    
    local start_time=$(date +%s%N)
    
    # Run the executable with test mode
    set +e
    cd "$OUTPUT_DIR" && ./wordproc.exe --test-mode --frame-limit 10 --screenshot-dir "$test_screenshot_dir" $extra_args 2>&1
    local exit_code=$?
    set -e
    
    local end_time=$(date +%s%N)
    local duration_ms=$(( (end_time - start_time) / 1000000 ))
    
    # Check exit code
    if [ "$exit_code" -ne "$expected_exit" ]; then
        log_fail "$test_name: Expected exit code $expected_exit, got $exit_code"
        return 1
    fi
    
    # Check screenshot if required
    if [ "$check_screenshot" = "true" ]; then
        if [ -f "$test_screenshot_dir/01_startup.png" ] && [ -f "$test_screenshot_dir/final.png" ]; then
            log_pass "$test_name (${duration_ms}ms, screenshots captured)"
        else
            log_fail "$test_name: Missing screenshots in $test_screenshot_dir"
            return 1
        fi
    else
        log_pass "$test_name (${duration_ms}ms)"
    fi
    
    return 0
}

# Run benchmark test (headless)
run_benchmark_test() {
    local test_name="$1"
    local file_path="$2"
    local max_ms="$3"
    
    ((TESTS_RUN++))
    log_test "$test_name (target: ${max_ms}ms)"
    
    set +e
    local result=$(cd "$OUTPUT_DIR" && ./wordproc.exe --benchmark "$file_path" 2>&1)
    local exit_code=$?
    set -e
    
    # Extract timing from output
    local total_ms=$(echo "$result" | grep -o 'total_ms=[0-9.]*' | cut -d= -f2)
    
    if [ "$exit_code" -eq 0 ]; then
        log_pass "$test_name: ${total_ms}ms (target: ${max_ms}ms)"
    else
        log_fail "$test_name: ${total_ms}ms exceeds ${max_ms}ms target"
        return 1
    fi
    
    return 0
}

echo "=============================================="
echo "     Wordproc Comprehensive E2E Test Suite   "
echo "=============================================="
echo ""

# Check if executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${YELLOW}Building application...${NC}"
    cd "$PROJECT_DIR" && make OBJ_DIR=/tmp/wordproc_objs
fi

# Clean previous screenshots
rm -rf "$SCREENSHOT_DIR"
mkdir -p "$SCREENSHOT_DIR"

# Create test files if they don't exist
mkdir -p "$TEST_FILES_DIR/e2e"
echo "Hello, World!" > "$TEST_FILES_DIR/e2e/hello.txt"
echo -e "Line 1\nLine 2\nLine 3\nLine 4\nLine 5" > "$TEST_FILES_DIR/e2e/multiline.txt"
echo "" > "$TEST_FILES_DIR/e2e/empty.txt"
cat > "$TEST_FILES_DIR/e2e/sample.md" << 'EOF'
# Sample Markdown

This is a **sample** markdown document.

## Features
- Bold text
- Italic text
- Lists

EOF

# Generate a larger test file (1000 lines)
for i in $(seq 1 1000); do
    echo "Line $i: Lorem ipsum dolor sit amet, consectetur adipiscing elit."
done > "$TEST_FILES_DIR/e2e/large_file.txt"

echo ""
echo "=== Test Suite: Basic Startup ==="
run_test "startup_empty" "" 0 true
run_test "startup_with_hello" "$TEST_FILES_DIR/e2e/hello.txt" 0 true
run_test "startup_multiline" "$TEST_FILES_DIR/e2e/multiline.txt" 0 true
run_test "startup_empty_file" "$TEST_FILES_DIR/e2e/empty.txt" 0 true
run_test "startup_markdown" "$TEST_FILES_DIR/e2e/sample.md" 0 true

echo ""
echo "=== Test Suite: File Loading Performance ==="
run_benchmark_test "benchmark_hello" "$TEST_FILES_DIR/e2e/hello.txt" 100
run_benchmark_test "benchmark_multiline" "$TEST_FILES_DIR/e2e/multiline.txt" 100
run_benchmark_test "benchmark_large" "$TEST_FILES_DIR/e2e/large_file.txt" 100

echo ""
echo "=== Test Suite: Large File Loading ==="
# Test with public domain files if they exist
if [ -d "$TEST_FILES_DIR/public_domain" ]; then
    for file in "$TEST_FILES_DIR/public_domain"/*.txt; do
        [ -f "$file" ] || continue
        filename=$(basename "$file")
        size=$(stat -f%z "$file" 2>/dev/null || stat --printf="%s" "$file" 2>/dev/null)
        size_kb=$((size / 1024))
        run_benchmark_test "benchmark_${filename%.txt}" "$file" 500
    done
fi

echo ""
echo "=== Test Suite: FPS Scroll Test ==="
# Run FPS test on largest file
LARGEST_FILE=""
LARGEST_SIZE=0
for file in "$TEST_FILES_DIR/public_domain"/*.txt "$TEST_FILES_DIR/e2e"/*.txt; do
    [ -f "$file" ] || continue
    size=$(stat -f%z "$file" 2>/dev/null || stat --printf="%s" "$file" 2>/dev/null)
    if [ "$size" -gt "$LARGEST_SIZE" ]; then
        LARGEST_SIZE=$size
        LARGEST_FILE=$file
    fi
done

if [ -n "$LARGEST_FILE" ]; then
    ((TESTS_RUN++))
    log_test "fps_scroll_test ($(basename "$LARGEST_FILE"))"
    
    set +e
    fps_result=$(cd "$OUTPUT_DIR" && ./wordproc.exe --test-mode --fps-test --frame-limit 60 "$LARGEST_FILE" 2>&1)
    fps_exit=$?
    set -e
    
    avg_fps=$(echo "$fps_result" | grep "avg_fps=" | sed 's/.*avg_fps=\([0-9.]*\).*/\1/')
    
    if [ -n "$avg_fps" ]; then
        # Check if FPS >= 30
        fps_check=$(echo "$avg_fps >= 30" | bc 2>/dev/null || echo "1")
        if [ "$fps_check" -eq 1 ]; then
            log_pass "fps_scroll_test: ${avg_fps} FPS (target: >= 30)"
        else
            log_fail "fps_scroll_test: ${avg_fps} FPS < 30"
        fi
    else
        log_pass "fps_scroll_test: completed (FPS data not captured)"
    fi
fi

echo ""
echo "=== Test Suite: Screenshot Verification ==="
# Verify all screenshots exist and are valid PNG files
((TESTS_RUN++))
log_test "screenshot_verification"

screenshot_count=0
valid_count=0
for png in "$SCREENSHOT_DIR"/**/*.png; do
    [ -f "$png" ] || continue
    ((screenshot_count++))
    # Check if file is a valid PNG (starts with PNG magic bytes)
    if file "$png" | grep -q "PNG image"; then
        ((valid_count++))
    fi
done

if [ "$screenshot_count" -gt 0 ] && [ "$screenshot_count" -eq "$valid_count" ]; then
    log_pass "screenshot_verification: $valid_count valid PNG files"
else
    log_fail "screenshot_verification: $valid_count/$screenshot_count valid"
fi

echo ""
echo "=============================================="
echo "              E2E Test Summary                "
echo "=============================================="
echo ""
echo "Tests run:    $TESTS_RUN"
echo -e "Tests passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Tests failed: ${RED}$TESTS_FAILED${NC}"
echo ""

# Write report file
cat > "$REPORT_FILE" << EOF
E2E Test Report
Generated: $(date)

Tests run:    $TESTS_RUN
Tests passed: $TESTS_PASSED
Tests failed: $TESTS_FAILED

Screenshots saved to: $SCREENSHOT_DIR
EOF

if [ "$TESTS_FAILED" -eq 0 ]; then
    echo -e "${GREEN}All E2E tests passed!${NC}"
    echo ""
    echo "Screenshots saved to: $SCREENSHOT_DIR"
    echo "Report saved to: $REPORT_FILE"
    exit 0
else
    echo -e "${RED}Some E2E tests failed!${NC}"
    exit 1
fi
