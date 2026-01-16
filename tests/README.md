# Testing

This project uses [Catch2 v2](https://github.com/catchorg/Catch2/tree/v2.x) for unit testing.

## Running Tests

```bash
# Run all tests
make test

# Run tests with verbose output (shows all assertions)
make test-verbose
```

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
