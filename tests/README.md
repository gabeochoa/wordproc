# Testing

This project uses [Catch2 v2](https://github.com/catchorg/Catch2/tree/v2.x) for unit testing and custom E2E testing with screenshot capture.

## Running Tests

```bash
# Run all unit tests
make test

# Run tests with verbose output (shows all assertions)
make test-verbose

# Run E2E tests with screenshot capture (requires display)
make e2e

# Run interactive launch-time benchmark (requires display)
make launch-benchmark
```

## E2E / Screenshot Testing

The application supports a test mode for automated E2E testing:

```bash
# Run with test mode enabled
./output/wordproc.exe --test-mode --frame-limit 10 --screenshot-dir output/screenshots
```

Options:
- `--test-mode` - Enables test mode, captures screenshots and logs startup time
- `--frame-limit N` - Exit after N frames (0 = run forever)
- `--screenshot-dir DIR` - Directory to save screenshots

Screenshots are saved at key points:
- `01_startup.png` - First frame after startup
- `final.png` - Last frame before exit

### Manual Visual Verification

After running E2E tests, visually verify screenshots:
```bash
open output/screenshots/
```

Check that:
1. Win95-style window chrome is visible (blue title bar, gray background)
2. Text area has sunken 3D border
3. Status bar shows line/column info
4. Caret is visible and positioned correctly

## Launch-Time Benchmark (Interactive)

The launch benchmark runs the app in test mode and measures wall-clock time:

```bash
make launch-benchmark
```

Environment overrides:
- `TARGET_MS` - wall-time target per run (default: 200)
- `ITERATIONS` - repeats per scenario (default: 3)
- `FRAME_LIMIT` - frames to render before exit (default: 2)

Results are saved to `output/perf/launch_times.csv`.

## Test Structure

- `tests/test_main.cpp` - Catch2 main entry point
- `tests/test_text_buffer.cpp` - Unit tests for TextBuffer (insert, delete, caret, selection)
- `tests/test_text_layout.cpp` - Unit tests for line wrapping/layout
- `tests/test_document_io.cpp` - Unit tests for save/load functionality

## Adding New Tests

1. Create a new `tests/test_*.cpp` file
2. Include `"catch2/catch.hpp"`
3. Write TEST_CASE blocks with REQUIRE/CHECK assertions
4. The makefile will automatically pick up new test files

## Test Output

Tests run in the terminal. A successful run shows:

```
===============================================================================
All tests passed (X assertions in Y test cases)
```

## Coverage (Optional)

To build with coverage instrumentation:

```bash
make test COVERAGE=1
```

This enables `-fprofile-instr-generate -fcoverage-mapping` on macOS or `--coverage` on Linux.
