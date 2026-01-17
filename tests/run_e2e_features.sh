#!/bin/bash
# E2E Feature Validation Test Runner (Batch Mode)
# Runs all E2E test scripts in a single application window for speed
#
# Test naming convention:
#   pass_<name>.e2e - Test expected to pass (exit code 0)
#   fail_<name>.e2e - Test expected to fail (exit code non-zero, validates error handling)
#
# This uses batch mode to avoid window creation overhead between tests.
# The E2E runner automatically handles per-script pass/fail tracking.

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
echo -e "${BLUE}Running all tests in single window (batch mode)...${NC}"
echo ""

# Run all scripts in batch mode from project root (for resource path consistency)
set +e
cd "$PROJECT_DIR" && "$EXECUTABLE" \
    --test-mode \
    --test-script-dir="$E2E_SCRIPTS_DIR" \
    --screenshot-dir="$SCREENSHOT_DIR" \
    2>&1 | tee "$OUTPUT_DIR/e2e_features.log"
exit_code=${PIPESTATUS[0]}
set -e

echo ""
echo "Screenshots saved to: $SCREENSHOT_DIR"
echo "Full log: $OUTPUT_DIR/e2e_features.log"

if [ $exit_code -eq 0 ]; then
    echo ""
    echo -e "${GREEN}All E2E feature tests passed!${NC}"
    exit 0
else
    echo ""
    echo -e "${RED}Some E2E feature tests failed!${NC}"
    exit 1
fi
