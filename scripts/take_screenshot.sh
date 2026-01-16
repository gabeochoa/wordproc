#!/bin/bash
# Script to run wordproc and capture a screenshot for debugging

cd "$(dirname "$0")/.."

# Clean up old screenshots
rm -f debug_screenshot.png 01_startup.png

# Run the app normally (no frame limit) so we can capture screenshot
echo "Starting wordproc..."
./output/wordproc.exe 2>&1 &
APP_PID=$!

# Wait for the app window to appear and render
sleep 1

# Bring wordproc to front using AppleScript
osascript -e 'tell application "wordproc" to activate' 2>/dev/null || true

# Give it a moment to come to front
sleep 0.5

# Use screencapture on macOS to take a screenshot
if command -v screencapture &> /dev/null; then
    echo "Taking screenshot with screencapture..."
    screencapture -x -o debug_screenshot.png
fi

# Kill the app
kill $APP_PID 2>/dev/null || true

# Check screenshots
if [ -f "debug_screenshot.png" ]; then
    echo "Screenshot captured: debug_screenshot.png"
    ls -la debug_screenshot.png
else
    echo "No screenshot captured!"
fi

echo "Done."


