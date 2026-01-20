#!/bin/bash
# Run E2E tests on large documents (War and Peace)
# Tests scrolling, navigation, menus, and editing

WORDPROC="./output/wordproc.exe"
WAR_AND_PEACE="test_files/public_domain/war_and_peace.txt"
SCREENSHOT_DIR="output/e2e_large_doc"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

# Check if app exists
if [ ! -f "$WORDPROC" ]; then
    echo -e "${RED}Error: $WORDPROC not found. Run 'make' first.${NC}"
    exit 1
fi

# Check if test file exists
if [ ! -f "$WAR_AND_PEACE" ]; then
    echo -e "${RED}Error: $WAR_AND_PEACE not found.${NC}"
    exit 1
fi

# Create screenshot directory
mkdir -p "$SCREENSHOT_DIR"

echo "=============================================="
echo "Large Document E2E Tests"
echo "=============================================="
echo "Test file: $WAR_AND_PEACE"
echo ""

passed=0
failed=0

run_test() {
    local test_name=$1
    local script_path=$2
    
    echo -n "  Running $test_name... "
    
    if $WORDPROC "$WAR_AND_PEACE" --test-mode --test-script="$script_path" --screenshot-dir="$SCREENSHOT_DIR" --e2e-timeout=60 2>&1; then
        echo -e "${GREEN}PASS${NC}"
        ((passed++))
    else
        echo -e "${RED}FAIL${NC}"
        ((failed++))
    fi
}

# Run scroll/navigation test
run_test "Scroll and Navigation" "tests/e2e_scripts/e2e_large_doc_scroll.e2e"

# Run menu interactions test
run_test "Menu Interactions" "tests/e2e_scripts/e2e_large_doc_menus.e2e"

echo ""
echo "=============================================="
echo "Summary: $passed passed, $failed failed"
echo "Screenshots saved to: $SCREENSHOT_DIR"
echo "=============================================="

if [ $failed -gt 0 ]; then
    exit 1
fi

exit 0

