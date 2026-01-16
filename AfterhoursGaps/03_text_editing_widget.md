# Text Editing Widget

## Problem
Afterhours does not include a built-in text editor or multiline text area
component with selection, caret, undo, and layout.

## Current Workaround
- Custom `TextBuffer` with gap buffer, selection, and caret management.
- App-specific layout and rendering logic in `src/editor/`.

## Desired Behavior
- Provide a text editor widget that supports:
  - Caret movement and selection.
  - Undo and redo.
  - Clipboard integration.
  - Word wrap and scrolling.
- Allow styling hooks for fonts, sizes, and colors.

## Proposed API Sketch
- `ui::TextEditor` component with data model and callbacks.
- `ui::TextEditor::set_text()` and `ui::TextEditor::get_text()`.
- `ui::TextEditor::set_read_only(bool)`.

## Notes
This would reduce the amount of custom editor code needed in the app.

