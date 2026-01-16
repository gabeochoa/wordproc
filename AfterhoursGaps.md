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

## Feature Requests / Ideas

1. **Text Editing Component**: Built-in multiline text editor with selection, caret, undo/redo
2. **Glyph Metrics API**: Get precise character widths for caret positioning (MeasureText is line-only)
3. **Menu System**: Win95-style dropdown menu component with keyboard navigation
4. **Dialog Framework**: Modal dialogs with standardized button placement
5. **Clipboard Abstraction**: Cross-platform clipboard with rich text support

## Notes

- No vendor/afterhours changes are required at this stage.
- All gaps have app-side workarounds implemented in `src/` directory.
- The custom TextBuffer with gap buffer performs well (2M chars/sec typing capability).

