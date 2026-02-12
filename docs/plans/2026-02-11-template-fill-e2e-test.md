# Template Fill E2E Test

## Goal

Add an E2E test that exercises a realistic solo-writer workflow: open a pre-built outline template, navigate to placeholder positions, type replacement text, and save the result. Pass/fail only — no timing measurement.

## Deliverables

### 1. New E2E command: `open_file`

A new command in `src/testing/e2e_commands.h` that opens a `.wpdoc` file by path, bypassing the native file dialog.

**Usage in test scripts:**

```
open_file test_files/outline_template.wpdoc
```

**Implementation:**

- Add `HandleOpenFileCommand` struct following the existing pattern (`HandleMenuOpenCommand`, etc.)
- Check `cmd.is("open_file")`, validate 1 argument (the file path)
- Use the existing document I/O logic to load the file into `doc_comp->buffer`
- Call `cmd.consume()` on success, `cmd.fail()` on error
- Register in `register_app_commands()`

### 2. Template file: `test_files/outline_template.wpdoc`

A `.wpdoc` v1 JSON file with placeholder text:

```json
{
  "version": 1,
  "text": "[TITLE]\n\n[INTRO]\n\nBackground\n[SECTION_1]\n\nRecommendations\n[SECTION_2]\n\nSummary\n[CONCLUSION]",
  "style": {
    "font": "EBGaramond-Regular",
    "fontSize": 16
  }
}
```

The section headings ("Background", "Recommendations", "Summary") are real content that stays. The bracketed placeholders (`[TITLE]`, `[INTRO]`, etc.) are what the test replaces.

Note: The current `.wpdoc` v1 format doesn't support per-line paragraph styles, so headings are just text. This is fine for the test's purpose.

### 3. E2E test script: `tests/e2e_scripts/files/e2e_template_fill.e2e`

```
# Test: Load outline template, fill placeholders, save
#
# Opens a pre-built template via the new open_file command,
# navigates to each placeholder line, selects it, types
# replacement text, then saves.

# Open template
open_file test_files/outline_template.wpdoc

# Go to start of document
key CTRL+HOME

# Line 1: [TITLE] — select and replace
key HOME
key SHIFT+END
type "Quarterly Business Review"

# Line 2: empty line — skip
key DOWN
key DOWN

# Line 3: [INTRO] — select and replace
key HOME
key SHIFT+END
type "This document summarizes the key findings and recommendations from the past quarter."

# Line 4: empty line — skip
key DOWN
key DOWN

# Line 5: "Background" heading — skip (keep as-is)
key DOWN

# Line 6: [SECTION_1] — select and replace
key HOME
key SHIFT+END
type "Revenue grew by 12% compared to the previous quarter, driven primarily by new customer acquisition."

# Line 7: empty line — skip
key DOWN
key DOWN

# Line 8: "Recommendations" heading — skip
key DOWN

# Line 9: [SECTION_2] — select and replace
key HOME
key SHIFT+END
type "Three strategic initiatives are recommended for the next quarter to sustain growth momentum."

# Line 10: empty line — skip
key DOWN
key DOWN

# Line 11: "Summary" heading — skip
key DOWN

# Line 12: [CONCLUSION] — select and replace
key HOME
key SHIFT+END
type "The outlook remains positive with continued investment in core product development."

# Save
key CTRL+S

# Validate the document was filled out
validate word_count>20
validate line_count=12

screenshot e2e_template_fill
```

## Scope boundaries

- **In scope:** `open_file` command, template `.wpdoc` file, E2E test script
- **Out of scope:** Performance measurement, per-paragraph styles in `.wpdoc`, Find & Replace based navigation, file dialog automation
