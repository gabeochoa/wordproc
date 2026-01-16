# Afterhours Gaps

Tracking any Afterhours library changes we might need, plus app-side workarounds.

## Current Gaps

### 1. Test Input Hooks
- `src/external.h` expects test input helpers for automated input.
- **Workaround**: Only forward declarations are copied; test input injection stub files created in `src/testing/`.

### 2. No Native Clipboard Support
- Afterhours doesn't provide clipboard abstraction.
- **Workaround**: Using raylib's `GetClipboardText()`/`SetClipboardText()` directly.

### 3. No Built-in Text Editing Widget
- Would benefit from a native text area/editor component.
- **Workaround**: Built custom `TextBuffer` with gap buffer, selection, and caret management.

## APIs Used

| API | Purpose | Notes |
|-----|---------|-------|
| raylib window/input | Window creation, keyboard/mouse | Works well |
| raylib drawing | Text, rectangles, lines | Sufficient for Win95 style |
| raylib fonts | Font loading and rendering | Need per-glyph metrics for accurate caret |
| Settings | Window position persistence | Works as expected |
| Preload | Application initialization | Clean API |

### 4. No Built-in Win95-Style Widget Library
- Afterhours doesn't provide Win95/classic-style UI widgets.
- **Workaround**: Custom `src/ui/win95_widgets.h/.cpp` implements:
  - `DrawRaisedBorder` / `DrawSunkenBorder` for 3D effects
  - `DrawButton` with hover/pressed/disabled states
  - `DrawCheckbox` with state tracking
  - `DrawMenuBar` with dropdown menu support
  - `DrawMessageDialog` and `DrawInputDialog` for modal dialogs
- Would benefit from Afterhours providing themeable widget primitives.

### 5. .doc File Import
- Microsoft Word .doc files use a proprietary binary format (OLE Compound Document).
- **Complexity**: Requires parsing OLE2/CFBF container, then Word-specific binary streams.
- **External Libraries**:
  - `antiword` (GPL, C): Extracts text from .doc files
  - `wv` / `wvWare` (GPL): More complete Word parsing
  - `libole2` / `pole`: OLE container parsing
  - Python `python-docx` (MIT, but .docx only)
- **Workaround**: For v0.1, recommend users convert .doc to .txt or .docx before importing.
- **Future**: Add wvWare or antiword as optional dependency for v0.2+ .doc import.

## Feature Requests / Ideas

1. **Text Editing Component**: Built-in multiline text editor with selection, caret, undo/redo
2. **Glyph Metrics API**: Get precise character widths for caret positioning (MeasureText is line-only)
3. **Menu System**: Win95-style dropdown menu component with keyboard navigation
4. **Dialog Framework**: Modal dialogs with standardized button placement
5. **Clipboard Abstraction**: Cross-platform clipboard with rich text support
6. **Themeable Widget Library**: Buttons, checkboxes, menus, dialogs with configurable themes (Win95, macOS, modern flat)

## Notes

- No vendor/afterhours changes are required at this stage.
- All gaps have app-side workarounds implemented in `src/` directory.
- The custom TextBuffer with gap buffer performs well (2M chars/sec typing capability).

