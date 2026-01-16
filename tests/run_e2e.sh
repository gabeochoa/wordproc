#!/bin/bash
# E2E test runner for Wordproc
# Runs the application in test mode to capture screenshots for visual verification

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
OUTPUT_DIR="$PROJECT_DIR/output"
SCREENSHOT_DIR="$OUTPUT_DIR/screenshots"
EXECUTABLE="$OUTPUT_DIR/ui_tester.exe"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=== Wordproc E2E Test Runner ==="
echo ""

# Check if executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${YELLOW}Building application...${NC}"
    cd "$PROJECT_DIR" && make
fi

# Clean previous screenshots
if [ -d "$SCREENSHOT_DIR" ]; then
    echo "Cleaning previous screenshots..."
    rm -rf "$SCREENSHOT_DIR"
fi
mkdir -p "$SCREENSHOT_DIR"

# Create test files
mkdir -p "$OUTPUT_DIR/test_files"
echo "Hello, World!" > "$OUTPUT_DIR/test_files/hello.txt"
echo -e "Line 1\nLine 2\nLine 3" > "$OUTPUT_DIR/test_files/multiline.txt"

echo ""
echo "=== Test 1: Basic Startup ==="
echo "Running app in test mode (5 frames)..."
cd "$OUTPUT_DIR" && ./ui_tester.exe --test-mode --frame-limit 5 --screenshot-dir "$SCREENSHOT_DIR"

# Check if screenshots were created
if [ -f "$SCREENSHOT_DIR/01_startup.png" ]; then
    echo -e "${GREEN}✓ Startup screenshot captured${NC}"
else
    echo -e "${RED}✗ Startup screenshot not found${NC}"
    exit 1
fi

if [ -f "$SCREENSHOT_DIR/final.png" ]; then
    echo -e "${GREEN}✓ Final screenshot captured${NC}"
else
    echo -e "${RED}✗ Final screenshot not found${NC}"
    exit 1
fi

echo ""
echo "=== Test Summary ==="
echo "Screenshots saved to: $SCREENSHOT_DIR"
echo ""
echo "Please visually verify the following:"
echo "  1. 01_startup.png - Window renders with Win95 styling"
echo "  2. final.png - Application renders correctly"
echo ""
echo "To view screenshots:"
echo "  open $SCREENSHOT_DIR"
echo ""
echo -e "${GREEN}E2E tests completed successfully!${NC}"
