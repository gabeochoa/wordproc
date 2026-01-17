# Text Editing Widget

## Status: Partially Addressed

Afterhours now has a basic `text_input` component, but it's a **single-line input**, 
not a full multiline text editor. A full text editor widget is still needed.

## Working Implementation
See these files for a complete working example:
- `src/editor/text_buffer.h` - TextBuffer with gap buffer, selection, undo/redo
- `src/editor/text_buffer.cpp` - Implementation
- `src/editor/text_layout.h` - Text layout and rendering
- `src/editor/text_layout.cpp` - Implementation

## What Afterhours Now Has (`text_input`)

The `text_input` component in `vendor/afterhours/src/plugins/ui/`:
- `text_input_utils.h` - UTF-8 text manipulation utilities
- `imm_components.h` - The `text_input()` immediate-mode component
- `components.h` - `HasTextInputStateT<Storage>` with custom storage backend support

**Features present:**
- ✅ Single-line text input with cursor
- ✅ UTF-8 support (CJK, emoji, multi-byte chars)
- ✅ Cursor movement (left/right by UTF-8 char)
- ✅ Backspace/delete with UTF-8 awareness
- ✅ Cursor blinking (configurable rate)
- ✅ Custom storage backend via `TextStorage` concept (can plug in gap buffer, rope)
- ✅ `on_change` and `on_submit` callbacks
- ✅ Max length limiting

**Features still missing:**
- ❌ **Selection** - No selection_start/selection_end, no shift+arrow
- ❌ **Undo/redo** - No command history
- ❌ **Clipboard** - No copy/cut/paste integration
- ❌ **Multiline** - Single-line only
- ❌ **Word wrap** - N/A for single-line
- ❌ **Scrolling** - No horizontal scroll for long text
- ❌ **Styling hooks** - No per-character or per-range styling

## Problem
Afterhours has a basic single-line `text_input` but no multiline text editor 
component with selection, undo/redo, clipboard, and layout.

## Current Workaround
- Custom `TextBuffer` with gap buffer, selection, and caret management.
- App-specific layout and rendering logic in `src/editor/`.
- These could potentially be extracted back to Afterhours as a `TextEditor` widget.

## Desired Behavior
A full `ui::TextEditor` widget that supports:
- Multiline editing with word wrap
- Text selection (shift+arrow, shift+click, double-click word, triple-click line)
- Undo/redo with command pattern
- Clipboard integration (Ctrl+C/X/V)
- Vertical and horizontal scrolling
- Styling hooks for fonts, sizes, colors (per-range formatting)
- Read-only mode

## Proposed API Sketch

```cpp
namespace afterhours::ui {

// Multiline text editor (vs single-line text_input)
template<TextStorage Storage = GapBuffer>
struct TextEditorState {
  Storage storage;
  size_t cursor_position = 0;
  size_t selection_anchor = 0;  // Selection start (cursor is end)
  bool has_selection() const { return cursor_position != selection_anchor; }
  
  // Undo/redo
  std::vector<EditCommand> undo_stack;
  std::vector<EditCommand> redo_stack;
  
  // Layout
  float scroll_x = 0, scroll_y = 0;
  bool word_wrap = true;
  bool read_only = false;
};

// Immediate-mode component
ElementResult text_editor(HasUIContext auto& ctx, EntityParent ep,
                          TextEditorState& state, 
                          const ComponentConfig& config = {});

} // namespace afterhours::ui
```

## Integration with 08_scrollable_containers.md

The text editor should use `HasScrollView` from **08_scrollable_containers.md** rather 
than implementing custom scroll state:

```cpp
// Instead of custom scroll in TextEditorState:
template<TextStorage Storage = GapBuffer>
struct TextEditorState {
  Storage storage;
  size_t cursor_position = 0;
  size_t selection_anchor = 0;
  
  // REMOVE redundant scroll state:
  // float scroll_x = 0, scroll_y = 0;  // Use HasScrollView instead
  
  bool word_wrap = true;
  bool read_only = false;
};

// Text editor entity composition:
Entity& editor = create_entity();
editor.addComponent<TextEditorState>(initial_text);
editor.addComponent<HasScrollView>({
  .horizontal_enabled = !word_wrap,  // Only if no word wrap
  .vertical_enabled = true,
  .scroll_speed = line_height,  // Scroll by line
});
editor.addComponent<UIComponent>(...);

// In layout system, after text layout:
auto& scroll = editor.get<HasScrollView>();
auto& layout = editor.get<TextLayoutCache>();
scroll.content_size = {layout.max_line_width, layout.total_height};
scroll.viewport_size = {editor.get<UIComponent>().rect().width, ...};

// Scroll to keep cursor visible:
Rectangle cursor_rect = layout.get_cursor_rect(state.cursor_position);
scroll.scroll_to_visible(cursor_rect);
```

**Dependency:** This integration depends on **08_scrollable_containers.md** being 
implemented first.

## Notes
- The existing `text_input` is good for form fields, search boxes, etc.
- A separate `text_editor` component for multiline editing would complement it
- Wordproc's `TextBuffer` + `TextLayout` could be extracted as a starting point
- The `TextStorage` concept in Afterhours already supports custom backends

