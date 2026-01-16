#!/bin/bash
# E2E Feature Validation Test Runner
# Runs all E2E test scripts to validate that features actually work
#
# Test naming convention:
#   pass_<name>.e2e - Test expected to pass (exit code 0)
#   fail_<name>.e2e - Test expected to fail (exit code non-zero, validates error handling)

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
OUTPUT_DIR="$PROJECT_DIR/output"
E2E_SCRIPTS_DIR="$SCRIPT_DIR/e2e_scripts"
SCREENSHOT_DIR="$OUTPUT_DIR/e2e_features"
EXECUTABLE="$OUTPUT_DIR/wordproc.exe"

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
FAILED_TESTS=""

echo "=============================================="
echo "   Wordproc E2E Feature Validation Suite     "
echo "=============================================="
echo ""

# Check if executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${YELLOW}Building application...${NC}"
    cd "$PROJECT_DIR" && make
fi

# Clean previous screenshots
rm -rf "$SCREENSHOT_DIR"
mkdir -p "$SCREENSHOT_DIR"

# Check if E2E scripts exist
if [ ! -d "$E2E_SCRIPTS_DIR" ]; then
    echo -e "${RED}Error: E2E scripts directory not found: $E2E_SCRIPTS_DIR${NC}"
    exit 1
fi

# Count scripts
SCRIPT_COUNT=$(find "$E2E_SCRIPTS_DIR" -name "*.e2e" | wc -l | tr -d ' ')
echo "Found $SCRIPT_COUNT E2E test scripts"
echo ""

# Run each E2E script
for script in "$E2E_SCRIPTS_DIR"/*.e2e; do
    [ -f "$script" ] || continue
    
    script_name=$(basename "$script" .e2e)
    ((TESTS_RUN++))
    
    # Determine expected outcome based on prefix
    if [[ "$script_name" == pass_* ]]; then
        expected_to_pass=true
        display_name="${script_name#pass_}"
        echo -e "${BLUE}[TEST]${NC} Running: $display_name (expect: pass)"
    elif [[ "$script_name" == fail_* ]]; then
        expected_to_pass=false
        display_name="${script_name#fail_}"
        echo -e "${BLUE}[TEST]${NC} Running: $display_name (expect: fail)"
    else
        # Legacy format - assume pass
        expected_to_pass=true
        display_name="$script_name"
        echo -e "${BLUE}[TEST]${NC} Running: $display_name"
    fi
    
    # Create script-specific screenshot dir
    script_screenshot_dir="$SCREENSHOT_DIR/$script_name"
    mkdir -p "$script_screenshot_dir"
    
    # Run the test
    set +e
    output=$(cd "$OUTPUT_DIR" && ./wordproc.exe \
        --test-mode \
        --test-script="$script" \
        --screenshot-dir="$script_screenshot_dir" \
        2>&1)
    exit_code=$?
    set -e
    
    # Check result based on expected outcome
    if [ "$expected_to_pass" = true ]; then
        # Test should pass (exit code 0)
        if [ $exit_code -eq 0 ]; then
            echo -e "${GREEN}[PASS]${NC} $display_name"
            ((TESTS_PASSED++))
        else
            echo -e "${RED}[FAIL]${NC} $display_name (expected pass, got exit code: $exit_code)"
            echo "Output:"
            echo "$output" | sed 's/^/  /'
            ((TESTS_FAILED++))
            FAILED_TESTS="$FAILED_TESTS\n  - $display_name (expected pass)"
        fi
    else
        # Test should fail (exit code non-zero)
        if [ $exit_code -ne 0 ]; then
            echo -e "${GREEN}[PASS]${NC} $display_name (correctly detected errors)"
            ((TESTS_PASSED++))
        else
            echo -e "${RED}[FAIL]${NC} $display_name (expected fail, but passed)"
            echo "Output:"
            echo "$output" | sed 's/^/  /'
            ((TESTS_FAILED++))
            FAILED_TESTS="$FAILED_TESTS\n  - $display_name (expected fail)"
        fi
    fi
done

echo ""
echo "=============================================="
echo "           E2E Feature Test Summary           "
echo "=============================================="
echo ""
echo "Tests run:    $TESTS_RUN"
echo -e "Tests passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Tests failed: ${RED}$TESTS_FAILED${NC}"

if [ $TESTS_FAILED -gt 0 ]; then
    echo ""
    echo -e "${RED}Failed tests:${NC}"
    echo -e "$FAILED_TESTS"
fi

echo ""
echo "Screenshots saved to: $SCREENSHOT_DIR"

if [ $TESTS_FAILED -eq 0 ]; then
    echo ""
    echo -e "${GREEN}All E2E feature tests passed!${NC}"
    exit 0
else
    echo ""
    echo -e "${RED}Some E2E feature tests failed!${NC}"
    exit 1
fi
