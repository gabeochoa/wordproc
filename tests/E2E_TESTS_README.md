# E2E Test Organization

## Directory Structure

Tests are organized by functionality into the following categories:

### Menu Tests (`menu/`)
Tests for menu bar functionality, organized by top-level menu:

- **`menu/file/`** - File menu operations (New, Open, Save, Save As, etc.)
- **`menu/edit/`** - Edit menu operations (Undo, Redo, Find, Go To Bookmark, etc.)
- **`menu/view/`** - View menu options (Page modes, Line width, Line numbers, etc.)
- **`menu/format/`** - Format menu operations (Styles, Fonts, Font size, Spacing, etc.)
- **`menu/insert/`** - Insert menu operations (future expansion)
- **`menu/table/`** - Table menu operations (Delete, Merge/Split, etc.)
- **`menu/help/`** - Help menu operations (future expansion)
- **`menu/tools/`** - Tools menu operations (future expansion)

### Formatting Tests (`formatting/`)
Text and paragraph formatting operations:
- Bold, Italic, Underline, Strikethrough
- Text color, Highlight color
- Paragraph styles, Alignment, Indentation
- Line spacing, Drop caps
- Bulleted/Numbered/Multi-level lists

### Editing Tests (`editing/`)
Text editing and manipulation:
- Selection (Select All, Mouse selection)
- Clipboard (Copy, Cut, Paste)
- Undo/Redo
- Find/Replace, Regex search
- Bookmarks, Comments
- Mouse interactions (Click, Double-click, Drag)

### Table Tests (`tables/`)
Table creation and manipulation:
- Insert table, Table navigation
- Add rows/columns

### Insert Tests (`insert/`)
Inserting special elements:
- Page breaks, Section breaks
- Hyperlinks, Footnotes
- Special characters, Equations
- Images, Drawings

### Document Tests (`document/`)
Document-level features:
- Page setup, Headers/Footers
- Outline view, Table of Contents

### View Tests (`view/`)
UI and view options:
- Status bar, Help window
- Zoom, Dark mode
- Focus mode, Split view

### File Operations (`files/`)
File handling and management:
- Export (PDF, HTML, RTF)
- Recent files, Autosave
- Templates, Track changes
- Word count

### Basic Tests (`basic/`)
Fundamental functionality:
- Basic typing, Multiline text
- Smart quotes, Tab width
- Large document handling

### Test Validation (`test/`)
Test infrastructure validation:
- Pass/Fail test examples
- Command validation tests

## Running Tests

### New Test Runner (`run_e2e.sh`)

The main test runner with full flexibility:

```bash
# Run all tests in e2e_scripts/ (non-recursive)
./tests/run_e2e.sh

# Recursively run ALL tests in all subdirectories
./tests/run_e2e.sh -r

# Run all menu tests recursively
./tests/run_e2e.sh -r menu/

# Run only file menu tests
./tests/run_e2e.sh -r menu/file/

# Run all formatting tests
./tests/run_e2e.sh -r formatting/

# Run a single test
./tests/run_e2e.sh basic/e2e_basic_typing.e2e

# Custom timeout (default 60s)
./tests/run_e2e.sh -r -t 120 menu/
```

### Menu Test Runner (`run_e2e_menu_tests.sh`)

Specialized runner for the 14 comprehensive menu tests:

```bash
./tests/run_e2e_menu_tests.sh
```

This runs the curated set of 14 menu validation tests that cover:
- File: New, Open, Save, Save As
- Edit: Go To Bookmark
- View: Page modes, Line width, Line numbers
- Format: Styles, Fonts, Font size, Spacing
- Table: Delete, Merge/Split

## Test Results

All 14 comprehensive menu tests: ✅ PASSING

To see detailed results:
```bash
cat output/e2e_tests.log | grep -E "\[PASS\]|\[FAIL\]"
```

## Adding New Tests

1. Create your test file with `.e2e` extension
2. Place it in the appropriate category folder
3. Run with `./tests/run_e2e.sh -r <category>/`

## Test File Format

E2E tests use a simple command-based DSL:

```e2e
# Comment
type "Hello"          # Type text
key CTRL+S            # Press keyboard shortcut
menu_open File        # Open menu
menu_select Save      # Select menu item
validate text=Hello   # Assert property value
wait 1                # Wait N seconds
screenshot test_name  # Take screenshot
```

For full command reference, see `src/testing/e2e_commands.h` and `vendor/afterhours/src/plugins/e2e_testing/`.

