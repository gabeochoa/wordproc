#!/bin/bash
# Wordproc E2E Test Runner
# Supports recursive test discovery and selective test running

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
OUTPUT_DIR="$PROJECT_DIR/output"
E2E_SCRIPTS_DIR="$SCRIPT_DIR/e2e_scripts"
SCREENSHOT_DIR="$OUTPUT_DIR/screenshots/e2e_tests"
EXECUTABLE="$OUTPUT_DIR/wordproc.exe"
LOG_FILE="$OUTPUT_DIR/e2e_tests.log"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
RECURSIVE=false
TEST_PATH=""
TIMEOUT=300
SPEED=5

# Parse arguments
usage() {
    echo "Usage: $0 [OPTIONS] [PATH]"
    echo ""
    echo "Run e2e tests for the wordproc application"
    echo ""
    echo "OPTIONS:"
    echo "  -r, --recursive     Recursively find all .e2e files in subdirectories"
    echo "  -t, --timeout SEC   Set timeout in seconds (default: 60)"
    echo "  -s, --speed MULT    Speed multiplier for waits (default: 10, 1=realtime)"
    echo "  -h, --help          Show this help message"
    echo ""
    echo "PATH:"
    echo "  Optional path to a specific test file or directory"
    echo "  If not provided, runs tests from e2e_scripts/"
    echo ""
    echo "EXAMPLES:"
    echo "  $0                          # Run all tests in e2e_scripts/"
    echo "  $0 -r                       # Recursively run all tests"
    echo "  $0 -r menu/                 # Run all menu tests recursively"
    echo "  $0 menu/file/               # Run all file menu tests"
    echo "  $0 basic/e2e_typing.e2e     # Run single test"
    exit 0
}

while [[ $# -gt 0 ]]; do
    case $1 in
        -r|--recursive)
            RECURSIVE=true
            shift
            ;;
        -t|--timeout)
            TIMEOUT="$2"
            shift 2
            ;;
        -s|--speed)
            SPEED="$2"
            shift 2
            ;;
        -h|--help)
            usage
            ;;
        *)
            TEST_PATH="$1"
            shift
            ;;
    esac
done

echo "=============================================="
echo "   Wordproc E2E Test Runner"
echo "=============================================="
echo ""

# Check for display/GUI access
check_display_access() {
    local os_type="$(uname -s)"
    
    if [ "$os_type" = "Darwin" ]; then
        # macOS: Check if we can access the window server
        # Try to get window list - this will fail if no GUI access
        if ! /usr/bin/osascript -e 'tell application "System Events" to return name of first process' &>/dev/null; then
            # Fallback: check if we're in a SSH session without display forwarding
            if [ -n "$SSH_CONNECTION" ] && [ -z "$DISPLAY" ]; then
                echo -e "${RED}ERROR: No GUI access available.${NC}"
                echo ""
                echo "E2E tests require a graphical display to run."
                echo "You appear to be in an SSH session without display forwarding."
                echo ""
                echo "Options:"
                echo "  1. Run tests locally on a machine with a display"
                echo "  2. Use SSH with X11 forwarding: ssh -X user@host"
                echo "  3. Use a virtual display (e.g., Xvfb on Linux)"
                echo ""
                return 1
            fi
        fi
    elif [ "$os_type" = "Linux" ]; then
        # Linux: Check if DISPLAY is set
        if [ -z "$DISPLAY" ] && [ -z "$WAYLAND_DISPLAY" ]; then
            echo -e "${RED}ERROR: No display available (DISPLAY and WAYLAND_DISPLAY not set).${NC}"
            echo ""
            echo "E2E tests require a graphical display to run."
            echo ""
            echo "Options:"
            echo "  1. Run tests on a machine with a display"
            echo "  2. Use Xvfb for headless testing:"
            echo "     Xvfb :99 -screen 0 1024x768x24 &"
            echo "     export DISPLAY=:99"
            echo "     $0 $@"
            echo ""
            return 1
        fi
    fi
    
    return 0
}

# Verify GUI access before proceeding
if ! check_display_access; then
    exit 1
fi

# Check if executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${YELLOW}Building application...${NC}"
    cd "$PROJECT_DIR" && make
fi

# Clean previous screenshots
rm -rf "$SCREENSHOT_DIR"
mkdir -p "$SCREENSHOT_DIR"

# Create temporary directory for tests
TEMP_TEST_DIR=$(mktemp -d)

# Determine which tests to copy
if [ -z "$TEST_PATH" ]; then
    # No path specified, use default e2e_scripts directory
    TEST_SOURCE="$E2E_SCRIPTS_DIR"
elif [ -f "$E2E_SCRIPTS_DIR/$TEST_PATH" ]; then
    # Single file specified
    cp "$E2E_SCRIPTS_DIR/$TEST_PATH" "$TEMP_TEST_DIR/"
    TEST_SOURCE=""
elif [ -d "$E2E_SCRIPTS_DIR/$TEST_PATH" ]; then
    # Directory specified
    TEST_SOURCE="$E2E_SCRIPTS_DIR/$TEST_PATH"
else
    echo -e "${RED}Error: Test path not found: $TEST_PATH${NC}"
    rm -rf "$TEMP_TEST_DIR"
    exit 1
fi

# Copy tests based on recursive flag
if [ -n "$TEST_SOURCE" ]; then
    if [ "$RECURSIVE" = true ]; then
        echo -e "${BLUE}Recursively copying tests from: $TEST_SOURCE${NC}"
        # Flatten all .e2e files into temp dir (app doesn't recurse subdirs)
        find "$TEST_SOURCE" -name "*.e2e" -type f -exec cp {} "$TEMP_TEST_DIR/" \;
    else
        echo -e "${BLUE}Copying tests from: $TEST_SOURCE${NC}"
        # Only copy tests from top level
        find "$TEST_SOURCE" -maxdepth 1 -name "*.e2e" -type f -exec cp {} "$TEMP_TEST_DIR/" \;
    fi
fi

# Count tests
SCRIPT_COUNT=$(find "$TEMP_TEST_DIR" -name "*.e2e" -type f | wc -l | tr -d ' ')

if [ "$SCRIPT_COUNT" -eq 0 ]; then
    echo -e "${YELLOW}No test files found!${NC}"
    rm -rf "$TEMP_TEST_DIR"
    exit 0
fi

echo "Found $SCRIPT_COUNT test script(s)"
echo ""
echo -e "${BLUE}Running tests with ${TIMEOUT}s timeout...${NC}"
echo "Screenshots will be saved to: $SCREENSHOT_DIR"
echo ""

# Run all scripts in batch mode from project root
set +e
cd "$PROJECT_DIR"

# Run with timeout wrapper to catch hung processes
if command -v timeout &>/dev/null; then
    # Use GNU timeout if available
    TIMEOUT_CMD="timeout $((TIMEOUT + 30))"
else
    # No timeout command, run directly
    TIMEOUT_CMD=""
fi

$TIMEOUT_CMD "$EXECUTABLE" \
    --test-mode \
    --test-script-dir="$TEMP_TEST_DIR" \
    --screenshot-dir="$SCREENSHOT_DIR" \
    --e2e-timeout="$TIMEOUT" \
    --e2e-speed="$SPEED" \
    2>&1 | tee "$LOG_FILE"
exit_code=${PIPESTATUS[0]}
set -e

# Check for permission-related failures
if [ $exit_code -eq 124 ]; then
    echo ""
    echo -e "${RED}ERROR: Test execution timed out.${NC}"
    echo "This may indicate the application couldn't open a window."
    echo ""
    echo "Possible causes:"
    echo "  - Running in a sandboxed environment without display access"
    echo "  - Screen recording/accessibility permissions not granted (macOS)"
    echo "  - No window manager running"
    echo ""
fi

# Check if log file is empty or shows no test output
if [ -f "$LOG_FILE" ]; then
    if ! grep -q "\[PASS\]\|\[FAIL\]\|E2E" "$LOG_FILE" 2>/dev/null; then
        if [ $exit_code -ne 0 ]; then
            echo ""
            echo -e "${YELLOW}WARNING: No test output detected in log.${NC}"
            echo "The application may have failed to start properly."
            echo "Check if GUI/display permissions are configured correctly."
            echo ""
        fi
    fi
fi

# Clean up temp directory
rm -rf "$TEMP_TEST_DIR"

echo ""
echo "Screenshots saved to: $SCREENSHOT_DIR"
echo "Full log: $LOG_FILE"

if [ $exit_code -eq 0 ]; then
    echo ""
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo ""
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi

