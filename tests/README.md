# Testing

This project uses [Catch2 v2](https://github.com/catchorg/Catch2/tree/v2.x) for unit testing and a custom E2E script system for integration testing.

## Running Tests

```bash
# Run all unit tests
make test

# Run tests with verbose output (shows all assertions)
make test-verbose

# Run E2E feature tests
./tests/run_e2e_features.sh

# Run performance benchmarks
./tests/run_benchmark.sh

# Run FPS scroll test
./tests/run_fps_scroll_test.sh

# Run launch-time benchmark
./tests/run_launch_benchmark.sh
```

## E2E Script Testing

The E2E test system uses `.e2e` script files to automate testing with simulated keyboard and mouse input.

### Running E2E Tests

```bash
# Run all E2E feature tests (batch mode - fast, single window)
./tests/run_e2e_batch.sh

# Run all E2E feature tests (individual mode - slower, separate window per test)
./tests/run_e2e_features.sh

# Run a single E2E test
./output/wordproc.exe --test-mode --test-script="tests/e2e_scripts/pass_basic_typing.e2e" --screenshot-dir="output/e2e_test"

# Run all tests in a directory (batch mode)
./output/wordproc.exe --test-mode --test-script-dir="tests/e2e_scripts" --screenshot-dir="output/e2e_batch"
```

### E2E Script Commands

| Command | Description | Example |
|---------|-------------|---------|
| `type "text"` | Type text characters | `type "Hello World"` |
| `key MODIFIER+KEY` | Press keyboard shortcut | `key CTRL+B` |
| `select_all` | Select all text (Ctrl+A) | `select_all` |
| `click x y` | Mouse click at coordinates | `click 400 300` |
| `double_click x y` | Double-click at coordinates | `double_click 200 150` |
| `drag x1 y1 x2 y2` | Mouse drag between points | `drag 100 100 300 100` |
| `mouse_move x y` | Move mouse cursor | `mouse_move 250 200` |
| `wait N` | Wait N frames | `wait 5` |
| `validate prop=value` | Assert document property | `validate text=Hello` |
| `screenshot name` | Capture screenshot | `screenshot 01_test` |
| `dump_document path` | Dump document to file | `dump_document debug.txt` |
| `clear` | Clear document (for batch tests) | `clear` |

### Supported Key Modifiers

- `CTRL+` or `CMD+` - Control/Command key
- `SHIFT+` - Shift key
- `ALT+` - Alt/Option key

Modifiers can be combined: `CTRL+SHIFT+S`

### Validation Properties

| Property | Description |
|----------|-------------|
| `text` | Full document text content |
| `line_count` | Number of lines in document |
| `caret_line` | Current line number (0-indexed) |
| `caret_column` | Current column number |
| `has_selection` | Whether text is selected ("true"/"false") |
| `selected_text` | Currently selected text |
| `is_bold` | Bold formatting state |
| `is_italic` | Italic formatting state |
| `is_underline` | Underline formatting state |
| `alignment` | Text alignment ("Left"/"Center"/"Right"/"Justify") |
| `paragraph_style` | Paragraph style ("Normal"/"Heading 1"/etc.) |
| `indent_level` | Indentation level |
| `list_type` | List type ("None"/"Bulleted"/"Numbered") |
| `line_spacing` | Line spacing multiplier |

### Example E2E Script

```
# Test: Bold formatting
# Validates Ctrl+B toggles bold

type "Bold Test"
wait 2

# Select all text
select_all
wait 2

# Apply bold
key CTRL+B
wait 2

# Validate bold is applied
validate is_bold=true

screenshot bold_test
```

### Writing New E2E Tests

1. Create a new `.e2e` file in `tests/e2e_scripts/`
2. Use the naming convention:
   - `pass_<name>.e2e` - Test expected to pass (validates feature works)
   - `fail_<name>.e2e` - Test expected to fail (validates error detection)
3. Add commands to test the feature
4. Run with `./tests/run_e2e_features.sh`

### Test Naming Convention

| Prefix | Expected Outcome | Use Case |
|--------|------------------|----------|
| `pass_` | Exit code 0 | Validate features work correctly |
| `fail_` | Exit code non-zero | Validate error handling works |

Example:
- `pass_bold_formatting.e2e` - Validates Ctrl+B applies bold (should pass)
- `fail_unknown_command.e2e` - Validates unknown commands are detected (should fail)

## Unit Tests

Unit tests use Catch2 and test C++ code directly without running the application.

### Test Files

- `test_main.cpp` - Catch2 main entry point
- `test_text_buffer.cpp` - TextBuffer operations
- `test_text_layout.cpp` - Line wrapping/layout
- `test_document_io.cpp` - Save/load functionality
- `test_table.cpp` - Table operations
- `test_bookmark.cpp` - Bookmark functionality
- `test_hyperlink.cpp` - Hyperlink functionality
- `test_spellcheck.cpp` - Spell/grammar checking
- `test_drawing.cpp` - Shape/drawing operations
- `test_image.cpp` - Image handling
- `test_outline.cpp` - Outline/TOC generation
- `test_menu_setup.cpp` - Menu configuration
- `test_format_validator.cpp` - Format validation

### Adding New Unit Tests

1. Create a new `tests/test_*.cpp` file
2. Include `"catch2/catch.hpp"`
3. Write TEST_CASE blocks with REQUIRE/CHECK assertions
4. The makefile will automatically pick up new test files

## Performance Testing

### Load-Time Benchmark

Measures cold start time for loading various files:

```bash
./tests/run_benchmark.sh
```

Results saved to `output/perf/load_times.csv`.

### FPS Scroll Test

Tests rendering performance while scrolling large files:

```bash
./tests/run_fps_scroll_test.sh
```

### Launch Benchmark

Interactive launch timing with multiple iterations:

```bash
./tests/run_launch_benchmark.sh
```

Environment overrides:
- `TARGET_MS` - wall-time target per run (default: 200)
- `ITERATIONS` - repeats per scenario (default: 3)
- `FRAME_LIMIT` - frames to render before exit (default: 2)

### Startup Profiling (macOS)

Capture a sampling profile of startup:

```bash
./tests/run_sample_profile.sh
```

Results saved to `output/perf/sample_startup.txt`.

## Test Output

Unit tests run in the terminal. A successful run shows:

```
===============================================================================
All tests passed (X assertions in Y test cases)
```

E2E tests show:

```
==============================================
           E2E Feature Test Summary           
==============================================

Tests run:    14
Tests passed: 14
Tests failed: 0

All E2E feature tests passed!
```
