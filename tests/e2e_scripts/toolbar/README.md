# Toolbar E2E Tests

These tests validate the functionality of the new toolbar buttons (icons, formatting, etc.).

## Running the Tests

Use the `--param=value` format (not `--param value`) with the wordproc executable:

```bash
# Run a single test
./output/wordproc.exe --test-script=tests/e2e_scripts/toolbar/e2e_toolbar_bold_button.e2e \
  --screenshot-dir=screenshots \
  --frame-limit=600

# Run all toolbar tests (batch mode)
./output/wordproc.exe --test-script-dir=tests/e2e_scripts/toolbar \
  --screenshot-dir=screenshots \
  --frame-limit=1200
```

## Test Files

1. **e2e_toolbar_bold_button.e2e** - Tests the Bold button functionality
2. **e2e_toolbar_italic_button.e2e** - Tests the Italic button functionality
3. **e2e_toolbar_underline_button.e2e** - Tests the Underline button functionality
4. **e2e_toolbar_alignment_buttons.e2e** - Tests all alignment buttons (Left, Center, Right, Justify)
5. **e2e_toolbar_undo_redo.e2e** - Tests Undo/Redo button functionality
6. **e2e_toolbar_new_document.e2e** - Tests the New Document button
7. **e2e_toolbar_cut_copy_paste.e2e** - Tests Cut/Copy/Paste operations
8. **e2e_toolbar_combined_formatting.e2e** - Tests multiple formatting buttons together
9. **e2e_toolbar_visual_check.e2e** - Captures screenshots of various toolbar states for visual verification

## What Each Test Does

The tests use keyboard shortcuts (Ctrl+B, Ctrl+I, etc.) to trigger the toolbar buttons and validate:
- Button state changes (active/inactive)
- Text formatting is applied correctly
- Alignment changes work properly
- Undo/Redo functionality
- Cut/Copy/Paste operations

## Screenshots

Screenshots are saved to the directory specified by `--screenshot-dir` (default: `screenshots/`)

Each test generates screenshots showing:
- Button states (active/inactive)
- Before and after formatting changes
- Visual verification of toolbar appearance

## Frame Limit

The `--frame-limit` parameter controls how many frames to run before timing out:
- Single test: 600 frames (~10 seconds at 60fps)
- All toolbar tests: 1200 frames (~20 seconds)
- If tests timeout, increase the frame limit

## Notes

- The tests validate functionality through keyboard shortcuts, which also update the toolbar button states
- Tests use the e2e testing framework built into the app
- All tests run headlessly and automatically

