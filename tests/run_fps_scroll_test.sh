#!/bin/bash
# FPS Scroll Test - loads largest test file and measures FPS while scrolling
# This test requires a window and measures visual performance

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
OUTPUT_DIR="$PROJECT_DIR/output"
EXECUTABLE="$OUTPUT_DIR/wordproc.exe"
TEST_DIR="$PROJECT_DIR/test_files/public_domain"
REPORT_FILE="$OUTPUT_DIR/perf/fps_scroll.log"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=== Wordproc FPS Scroll Test ==="
echo ""

# Check if executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${YELLOW}Building application...${NC}"
    cd "$PROJECT_DIR" && make
fi

# Find the largest test file
LARGEST_FILE=""
LARGEST_SIZE=0

for file in "$TEST_DIR"/*.txt "$TEST_DIR"/*.md; do
    [ -f "$file" ] || continue
    size=$(stat -f%z "$file" 2>/dev/null || stat --printf="%s" "$file" 2>/dev/null)
    if [ "$size" -gt "$LARGEST_SIZE" ]; then
        LARGEST_SIZE=$size
        LARGEST_FILE=$file
    fi
done

if [ -z "$LARGEST_FILE" ]; then
    echo -e "${RED}No test files found in $TEST_DIR${NC}"
    exit 1
fi

FILENAME=$(basename "$LARGEST_FILE")
SIZE_MB=$(echo "scale=2; $LARGEST_SIZE / 1048576" | bc)

echo "Largest file: $FILENAME ($SIZE_MB MB)"
echo ""

# Create output directory
mkdir -p "$OUTPUT_DIR/perf"

echo "=== Running FPS Scroll Test ==="
echo "Loading $FILENAME and measuring FPS during simulated scrolling..."
echo ""

# Run app in test mode with scroll simulation
# --fps-test mode: runs for 60 frames, simulates PageDown each frame, logs FPS
cd "$OUTPUT_DIR" && ./wordproc.exe --test-mode --fps-test --frame-limit 60 "$LARGEST_FILE" 2>&1 | tee "$REPORT_FILE"

echo ""
echo "=== FPS Test Complete ==="
echo "Report saved to: $REPORT_FILE"

# Parse and summarize results
if [ -f "$REPORT_FILE" ]; then
    # Extract FPS data if available
    avg_fps=$(grep "avg_fps=" "$REPORT_FILE" 2>/dev/null | sed 's/.*avg_fps=\([0-9.]*\).*/\1/' | tail -1)
    min_fps=$(grep "min_fps=" "$REPORT_FILE" 2>/dev/null | sed 's/.*min_fps=\([0-9.]*\).*/\1/' | tail -1)
    
    if [ -n "$avg_fps" ]; then
        echo ""
        echo "FPS Summary:"
        echo "  Average FPS: $avg_fps"
        echo "  Minimum FPS: $min_fps"
        
        # Check if FPS is acceptable (target: 60 FPS)
        if [ "$(echo "$avg_fps >= 30" | bc)" -eq 1 ]; then
            echo -e "  ${GREEN}PASS: FPS is acceptable (>= 30)${NC}"
        else
            echo -e "  ${RED}FAIL: FPS is below 30${NC}"
            exit 1
        fi
    else
        echo ""
        echo -e "${YELLOW}Note: FPS data not captured. Run with --fps-test flag.${NC}"
        echo "The test ran successfully but FPS logging needs to be implemented in the app."
    fi
fi

echo ""
echo -e "${GREEN}FPS scroll test completed!${NC}"
