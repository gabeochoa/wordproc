#!/bin/bash
# E2E Menu Test Runner
# Runs the new comprehensive menu tests in batch mode

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
OUTPUT_DIR="$PROJECT_DIR/output"
E2E_SCRIPTS_DIR="$SCRIPT_DIR/e2e_scripts"
SCREENSHOT_DIR="$OUTPUT_DIR/screenshots/menu_tests"
EXECUTABLE="$OUTPUT_DIR/wordproc.exe"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "=============================================="
echo "   Wordproc Menu E2E Test Suite     "
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

# Create a temporary directory with just the new comprehensive menu tests
TEMP_MENU_TESTS=$(mktemp -d)
# Copy only the new comprehensive menu tests (not the old validation-only tests)
for test in \
    e2e_menu_file_new.e2e \
    e2e_menu_file_open.e2e \
    e2e_menu_file_save.e2e \
    e2e_menu_view_page_modes.e2e \
    e2e_menu_view_line_width.e2e \
    e2e_menu_view_line_numbers.e2e \
    e2e_menu_format_styles.e2e \
    e2e_menu_format_fonts.e2e \
    e2e_menu_format_font_size.e2e \
    e2e_menu_format_para_spacing.e2e \
    e2e_menu_table_delete.e2e \
    e2e_menu_table_merge_split.e2e
do
    if [ -f "$E2E_SCRIPTS_DIR/$test" ]; then
        cp "$E2E_SCRIPTS_DIR/$test" "$TEMP_MENU_TESTS/"
    fi
done

# Count scripts
SCRIPT_COUNT=$(find "$TEMP_MENU_TESTS" -name "*.e2e" | wc -l | tr -d ' ')
echo "Found $SCRIPT_COUNT menu test scripts"
echo ""
echo -e "${BLUE}Running menu tests...${NC}"
echo "Screenshots will be saved to: $SCREENSHOT_DIR"
echo ""

# Run all menu test scripts in batch mode from project root (with 60s timeout)
set +e
cd "$PROJECT_DIR" && "$EXECUTABLE" \
    --test-mode \
    --test-script-dir="$TEMP_MENU_TESTS" \
    --screenshot-dir="$SCREENSHOT_DIR" \
    --e2e-timeout=60 \
    2>&1 | tee "$OUTPUT_DIR/e2e_menu_tests.log"
exit_code=${PIPESTATUS[0]}
set -e

# Clean up temp directory
rm -rf "$TEMP_MENU_TESTS"

echo ""
echo "Screenshots saved to: $SCREENSHOT_DIR"
echo "Full log: $OUTPUT_DIR/e2e_menu_tests.log"

if [ $exit_code -eq 0 ]; then
    echo ""
    echo -e "${GREEN}All menu tests passed!${NC}"
    exit 0
else
    echo ""
    echo -e "${RED}Some menu tests failed!${NC}"
    exit 1
fi

