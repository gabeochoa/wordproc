# Text Editing Composable Components

## Status: Partially Addressed

Afterhours provides building blocks for text editing. Applications like wordproc 
compose these into full text editors. Afterhours does NOT provide a complete 
text editor widget - that's the application's job.

## Design Philosophy

**Afterhours provides LEGO bricks. Applications build castles.**

A game engine should NOT include a full text editor widget because:
1. Games have wildly different text editing needs (chat vs code vs notes vs console)
2. Full editors are complex and opinionated
3. Most games need only `text_input` (single-line)
4. Applications that need more can compose primitives

Instead, Afterhours provides:
- **Composable components** - Small, focused, combinable
- **Concepts/interfaces** - Contracts that apps implement
- **Utility functions** - Helpers for common operations
- **Higher-order patterns** - Ways to enhance basic components

---

## What Afterhours Already Has

### `text_input` (Single-line)
Location: `vendor/afterhours/src/plugins/ui/`
- `text_input_utils.h` - UTF-8 manipulation utilities
- `imm_components.h` - The `text_input()` immediate-mode component
- `components.h` - `HasTextInputStateT<Storage>` with pluggable storage

**Features:**
- ✅ UTF-8 support (CJK, emoji, multi-byte)
- ✅ Cursor movement (left/right by UTF-8 char)
- ✅ Backspace/delete with UTF-8 awareness
- ✅ Cursor blinking (configurable)
- ✅ `TextStorage` concept for custom backends
- ✅ `on_change` and `on_submit` callbacks

### `HasScrollView` (Scrolling)
Location: `vendor/afterhours/src/plugins/ui/components.h`
- Scroll offset tracking
- Content size vs viewport size
- `scroll_to_visible(rect)` helper
- Mouse wheel input handling

### `clipboard` (System Clipboard)
Location: `vendor/afterhours/src/plugins/clipboard.h`

**Already implemented:**
```cpp
namespace afterhours::clipboard {
  void set_text(std::string_view text);  // Copy to clipboard
  std::string get_text();                 // Paste from clipboard
  bool has_text();                        // Check if clipboard has content
}
```

Wordproc extends this with test mode support in `src/util/clipboard.h`:
```cpp
namespace app::clipboard {
  void enable_test_mode();   // Use in-memory clipboard for E2E tests
  void disable_test_mode();
  bool is_test_mode();
  // Same API: set_text(), get_text(), has_text()
}
```

**What's missing:** The `text_input` widget doesn't wire up Ctrl+C/V/X to 
the clipboard. This could be added via a config option:

```cpp
// Proposed API - opt-in clipboard shortcuts
text_input(ctx, mk(parent), text,
    ComponentConfig{}
        .with_size(...)
        .enable_keyboard_shortcuts<InputAction>()  // Enables clipboard actions
);

// Requires these actions in the InputAction enum:
enum struct InputAction {
  // ... existing ...
  WidgetTextCopy,       // Ctrl+C / Cmd+C
  WidgetTextCut,        // Ctrl+X / Cmd+X
  WidgetTextPaste,      // Ctrl+V / Cmd+V
  WidgetTextSelectAll,  // Ctrl+A / Cmd+A
};
```

**Dependency: 06_action_binding_system.md**

Modifier key combos (Ctrl+C, etc.) require the `ActionMap` with `Modifiers` support
from **06_action_binding_system.md**. That document proposes:

```cpp
// From 06 - KeyBinding with modifiers
namespace afterhours::input {
  enum class Modifiers : uint8_t {
    None = 0, Ctrl = 1, Shift = 2, Alt = 4, Meta = 8
  };
  
  struct KeyBinding {
    KeyCode key;
    Modifiers modifiers;
    [[nodiscard]] bool is_pressed() const;
  };
}

// Binding clipboard actions
actions.bind({KEY_C, Modifiers::Ctrl}, InputAction::WidgetTextCopy);
actions.bind({KEY_V, Modifiers::Ctrl}, InputAction::WidgetTextPaste);
```

**Implementation order:**
1. Implement `ActionMap` with modifier support (06)
2. Add `enable_keyboard_shortcuts()` config to `text_input` (this doc)
3. Wire up clipboard actions when that config is set

---

## Plugin Organization

Not all text-editing features belong in the same plugin. Here's the recommended organization:

### Standalone Plugins (Useful Beyond UI)

| Plugin | Purpose | Why Standalone? |
|--------|---------|-----------------|
| `clipboard.h` | System clipboard access | Platform-specific, used by CLI tools, games, etc. |
| `command_history.h` | Generic undo/redo stack | Useful for level editors, settings, game state - not just text |

### UI Plugin (`plugins/ui/`)

| Component | Purpose | Why In UI? |
|-----------|---------|------------|
| `TextSelection` | Selection anchor + cursor | Composes with `text_input`, UI-specific |
| `LineIndex` | Offset ↔ row/column mapping | Text layout for UI rendering |
| `TextLayoutCache` | Word wrap layout | Text rendering in UI viewports |
| `text_utils::*` | Word/paragraph navigation | Extends existing `text_input_utils.h` |

### Application Layer (NOT in Afterhours)

| Feature | Why Not In Afterhours? |
|---------|------------------------|
| Rich text / formatting spans | Application-specific styling needs |
| Text buffer implementations (gap buffer, rope) | Apps choose their own data structures |
| Find/replace | UI/UX varies widely between apps |
| Syntax highlighting | Domain-specific (code, markdown, etc.) |
| Document save/load | File formats are app-specific |

### Plugin Dependency Graph

```
                    ┌─────────────────┐
                    │  command_history │  (standalone - no dependencies)
                    └─────────────────┘
                    
                    ┌─────────────────┐
                    │    clipboard     │  (standalone - platform deps only)
                    └─────────────────┘
                    
┌───────────────────────────────────────────────────────────────┐
│                        plugins/ui/                             │
│  ┌─────────────┐  ┌─────────────┐  ┌──────────────────────┐  │
│  │TextSelection│  │ LineIndex   │  │ TextLayoutCache      │  │
│  └─────────────┘  └─────────────┘  └──────────────────────┘  │
│         │                │                    │               │
│         └────────────────┴────────────────────┘               │
│                          │                                    │
│                          ▼                                    │
│  ┌──────────────────────────────────────────────────────┐    │
│  │              text_input + text_input_utils            │    │
│  │         (existing single-line input component)        │    │
│  └──────────────────────────────────────────────────────┘    │
└───────────────────────────────────────────────────────────────┘
```

### Why `CommandHistory` is Standalone

The undo/redo pattern is useful far beyond text editing:

```cpp
// Level editor - undo tile placements
CommandHistory<LevelState> level_history;

// Settings panel - undo option changes  
CommandHistory<GameSettings> settings_history;

// Text editor - undo text edits
CommandHistory<TextBuffer> text_history;

// Character creator - undo appearance changes
CommandHistory<CharacterAppearance> character_history;
```

Making it a standalone plugin means games can use undo/redo without 
pulling in UI dependencies.

---

## Composable Components to Add

These are the building blocks applications need to construct text editors.

### 1. `TextSelection` - Selection State

A simple data structure for tracking text selection. No behavior, just state.

```cpp
namespace afterhours::ui {

/// Text selection state - tracks anchor and cursor positions.
/// The anchor is where selection started, cursor is current position.
/// When anchor == cursor, there is no selection.
///
/// This is just DATA - no editing logic. Applications use this
/// with their own text buffer to implement selection behavior.
struct TextSelection {
  size_t anchor = 0;   // Byte offset where selection started
  size_t cursor = 0;   // Byte offset of current cursor position
  
  // Query
  bool has_selection() const { return anchor != cursor; }
  bool is_empty() const { return !has_selection(); }
  
  // Get ordered range (start <= end)
  size_t start() const { return std::min(anchor, cursor); }
  size_t end() const { return std::max(anchor, cursor); }
  size_t length() const { return end() - start(); }
  
  // Mutations
  void collapse_to_cursor() { anchor = cursor; }
  void collapse_to_start() { cursor = anchor = start(); }
  void collapse_to_end() { cursor = anchor = end(); }
  void select_all(size_t text_length) { anchor = 0; cursor = text_length; }
  
  // Set cursor, optionally extending selection
  void set_cursor(size_t pos, bool extend_selection) {
    cursor = pos;
    if (!extend_selection) anchor = pos;
  }
};

/// Component wrapper for ECS usage
struct HasTextSelection : BaseComponent {
  TextSelection selection;
  
  HasTextSelection() = default;
  explicit HasTextSelection(size_t initial_cursor) 
    : selection{initial_cursor, initial_cursor} {}
};

} // namespace afterhours::ui
```

**Usage in wordproc:**
```cpp
// Wordproc's TextBuffer uses this for selection
class TextBuffer {
  afterhours::ui::TextSelection selection_;
  GapBuffer chars_;
  
  void move_cursor_right(bool extend) {
    size_t new_pos = next_utf8_pos(selection_.cursor);
    selection_.set_cursor(new_pos, extend);
  }
  
  std::string get_selected_text() const {
    if (!selection_.has_selection()) return "";
    return chars_.substr(selection_.start(), selection_.length());
  }
};
```

---

### 2. `CommandHistory<T>` - Generic Undo/Redo

A generic undo/redo stack that works with ANY state type, not just text.
Games can use this for level editors, settings, etc.

```cpp
namespace afterhours {

/// Base interface for reversible commands.
/// Template parameter T is the state type being modified.
template <typename State>
struct Command {
  virtual ~Command() = default;
  
  /// Execute the command, modifying state
  virtual void execute(State& state) = 0;
  
  /// Reverse the command, restoring previous state
  virtual void undo(State& state) = 0;
  
  /// Human-readable description for UI
  virtual std::string_view description() const = 0;
  
  /// Can this command merge with another? (e.g., typing multiple chars)
  virtual bool can_merge_with(const Command& other) const { return false; }
  
  /// Merge another command into this one
  virtual void merge_with(Command& other) {}
};

/// Generic undo/redo history.
/// Works with any state type - text buffers, game state, settings, etc.
template <typename State>
class CommandHistory {
public:
  using CommandPtr = std::unique_ptr<Command<State>>;
  
  explicit CommandHistory(size_t max_depth = 100) : max_depth_(max_depth) {}
  
  /// Execute a command and record it for undo
  void execute(CommandPtr cmd, State& state) {
    cmd->execute(state);
    
    // Try merging with previous command
    if (!undo_stack_.empty() && undo_stack_.back()->can_merge_with(*cmd)) {
      undo_stack_.back()->merge_with(*cmd);
    } else {
      undo_stack_.push_back(std::move(cmd));
      if (undo_stack_.size() > max_depth_) {
        undo_stack_.erase(undo_stack_.begin());
      }
    }
    
    // Clear redo stack on new action
    redo_stack_.clear();
  }
  
  /// Record a command without executing (for external mutations)
  void record(CommandPtr cmd) {
    undo_stack_.push_back(std::move(cmd));
    if (undo_stack_.size() > max_depth_) {
      undo_stack_.erase(undo_stack_.begin());
    }
    redo_stack_.clear();
  }
  
  bool can_undo() const { return !undo_stack_.empty(); }
  bool can_redo() const { return !redo_stack_.empty(); }
  
  void undo(State& state) {
    if (!can_undo()) return;
    auto cmd = std::move(undo_stack_.back());
    undo_stack_.pop_back();
    cmd->undo(state);
    redo_stack_.push_back(std::move(cmd));
  }
  
  void redo(State& state) {
    if (!can_redo()) return;
    auto cmd = std::move(redo_stack_.back());
    redo_stack_.pop_back();
    cmd->execute(state);
    undo_stack_.push_back(std::move(cmd));
  }
  
  void clear() {
    undo_stack_.clear();
    redo_stack_.clear();
  }
  
  size_t undo_depth() const { return undo_stack_.size(); }
  size_t redo_depth() const { return redo_stack_.size(); }

private:
  std::vector<CommandPtr> undo_stack_;
  std::vector<CommandPtr> redo_stack_;
  size_t max_depth_;
};

/// ECS component wrapper
template <typename State>
struct HasCommandHistory : BaseComponent {
  CommandHistory<State> history;
  
  HasCommandHistory() = default;
  explicit HasCommandHistory(size_t max_depth) : history(max_depth) {}
};

} // namespace afterhours
```

**Usage in wordproc:**
```cpp
// Define text-specific commands
struct InsertTextCommand : afterhours::Command<TextBuffer> {
  size_t offset;
  std::string text;
  
  InsertTextCommand(size_t off, std::string txt) 
    : offset(off), text(std::move(txt)) {}
  
  void execute(TextBuffer& buf) override {
    buf.insert_at(offset, text);
  }
  
  void undo(TextBuffer& buf) override {
    buf.erase_at(offset, text.size());
  }
  
  std::string_view description() const override { return "Insert"; }
  
  bool can_merge_with(const Command<TextBuffer>& other) const override {
    auto* ins = dynamic_cast<const InsertTextCommand*>(&other);
    return ins && ins->offset == offset + text.size() && text.size() < 50;
  }
  
  void merge_with(Command<TextBuffer>& other) override {
    text += static_cast<InsertTextCommand&>(other).text;
  }
};

// Use in TextBuffer
class TextBuffer {
  afterhours::CommandHistory<TextBuffer> history_;
  
  void insert_char(char ch) {
    auto cmd = std::make_unique<InsertTextCommand>(caret_, std::string(1, ch));
    history_.execute(std::move(cmd), *this);
  }
  
  void undo() { history_.undo(*this); }
  void redo() { history_.redo(*this); }
};
```

---

### 4. `LineIndex` - Line/Column ↔ Offset Mapping

Efficiently maps between byte offsets and row/column positions.
Essential for multiline text navigation.

```cpp
namespace afterhours::ui {

/// Maps between byte offsets and row/column positions in text.
/// Caches line start positions for efficient lookups.
/// 
/// This is a UTILITY, not tied to any specific text storage.
/// Applications use it with their own buffer.
class LineIndex {
public:
  /// Rebuild index from text. Call after text changes.
  void rebuild(std::string_view text) {
    line_starts_.clear();
    line_starts_.push_back(0);  // Line 0 starts at offset 0
    
    for (size_t i = 0; i < text.size(); ++i) {
      if (text[i] == '\n') {
        line_starts_.push_back(i + 1);
      }
    }
    text_size_ = text.size();
  }
  
  /// Number of lines (always >= 1)
  size_t line_count() const { 
    return line_starts_.empty() ? 1 : line_starts_.size(); 
  }
  
  /// Get byte offset of line start
  size_t line_start(size_t row) const {
    if (row >= line_starts_.size()) return text_size_;
    return line_starts_[row];
  }
  
  /// Get byte offset of line end (before newline or at text end)
  size_t line_end(size_t row) const {
    if (row + 1 < line_starts_.size()) {
      return line_starts_[row + 1] - 1;  // Before the \n
    }
    return text_size_;
  }
  
  /// Get line length in bytes
  size_t line_length(size_t row) const {
    return line_end(row) - line_start(row);
  }
  
  /// Convert byte offset to row/column
  struct Position { size_t row; size_t column; };
  Position offset_to_position(size_t offset) const {
    // Binary search for the line containing offset
    auto it = std::upper_bound(line_starts_.begin(), line_starts_.end(), offset);
    size_t row = (it == line_starts_.begin()) ? 0 : std::distance(line_starts_.begin(), it) - 1;
    size_t column = offset - line_starts_[row];
    return {row, column};
  }
  
  /// Convert row/column to byte offset
  size_t position_to_offset(size_t row, size_t column) const {
    if (row >= line_starts_.size()) {
      return text_size_;
    }
    size_t line_start = line_starts_[row];
    size_t max_col = line_length(row);
    return line_start + std::min(column, max_col);
  }
  
  /// Clamp column to valid range for a row
  size_t clamp_column(size_t row, size_t column) const {
    return std::min(column, line_length(row));
  }

private:
  std::vector<size_t> line_starts_;
  size_t text_size_ = 0;
};

/// ECS component wrapper
struct HasLineIndex : BaseComponent {
  LineIndex index;
};

} // namespace afterhours::ui
```

**Usage in wordproc:**
```cpp
class TextBuffer {
  afterhours::ui::LineIndex line_index_;
  GapBuffer chars_;
  
  void on_text_changed() {
    line_index_.rebuild(chars_.toString());
  }
  
  void move_cursor_up() {
    auto pos = line_index_.offset_to_position(caret_);
    if (pos.row > 0) {
      size_t new_col = line_index_.clamp_column(pos.row - 1, pos.column);
      caret_ = line_index_.position_to_offset(pos.row - 1, new_col);
    }
  }
  
  void move_cursor_to_line_start() {
    auto pos = line_index_.offset_to_position(caret_);
    caret_ = line_index_.line_start(pos.row);
  }
};
```

---

### 5. Text Navigation Utilities

Free functions for common text navigation operations.
These extend the existing `text_input_utils.h`.

```cpp
namespace afterhours::ui::text_utils {

// ============ UTF-8 Utilities (already exist in text_input_utils.h) ============
// utf8_char_length(), utf8_prev_char_start(), codepoint_to_utf8()

// ============ Word Boundary Detection ============

/// Check if character is a word separator
inline bool is_word_separator(char c) {
  return std::isspace(static_cast<unsigned char>(c)) || 
         std::ispunct(static_cast<unsigned char>(c));
}

/// Find start of word containing or before position
inline size_t find_word_start(std::string_view text, size_t pos) {
  if (pos == 0 || text.empty()) return 0;
  
  // Move back past any separators
  size_t p = pos;
  while (p > 0 && is_word_separator(text[p - 1])) --p;
  
  // Move back to start of word
  while (p > 0 && !is_word_separator(text[p - 1])) --p;
  
  return p;
}

/// Find end of word containing or after position
inline size_t find_word_end(std::string_view text, size_t pos) {
  if (pos >= text.size()) return text.size();
  
  // Move forward past any separators
  size_t p = pos;
  while (p < text.size() && is_word_separator(text[p])) ++p;
  
  // Move forward to end of word
  while (p < text.size() && !is_word_separator(text[p])) ++p;
  
  return p;
}

/// Select the word at position (for double-click)
inline std::pair<size_t, size_t> select_word_at(std::string_view text, size_t pos) {
  if (text.empty()) return {0, 0};
  pos = std::min(pos, text.size() - 1);
  
  // If on a separator, select just that separator
  if (is_word_separator(text[pos])) {
    return {pos, pos + 1};
  }
  
  // Find word boundaries
  size_t start = pos;
  while (start > 0 && !is_word_separator(text[start - 1])) --start;
  
  size_t end = pos;
  while (end < text.size() && !is_word_separator(text[end])) ++end;
  
  return {start, end};
}

/// Select entire line at position (for triple-click)
inline std::pair<size_t, size_t> select_line_at(std::string_view text, size_t pos,
                                                 const LineIndex& lines) {
  auto position = lines.offset_to_position(pos);
  return {lines.line_start(position.row), lines.line_end(position.row)};
}

// ============ Paragraph Navigation ============

/// Find start of paragraph (empty line or document start)
inline size_t find_paragraph_start(std::string_view text, size_t pos,
                                    const LineIndex& lines) {
  auto position = lines.offset_to_position(pos);
  
  // Move up until we find an empty line or reach line 0
  while (position.row > 0) {
    if (lines.line_length(position.row - 1) == 0) {
      return lines.line_start(position.row);
    }
    position.row--;
  }
  return 0;
}

/// Find end of paragraph (empty line or document end)
inline size_t find_paragraph_end(std::string_view text, size_t pos,
                                  const LineIndex& lines) {
  auto position = lines.offset_to_position(pos);
  
  // Move down until we find an empty line or reach last line
  while (position.row < lines.line_count() - 1) {
    if (lines.line_length(position.row + 1) == 0) {
      return lines.line_end(position.row);
    }
    position.row++;
  }
  return text.size();
}

} // namespace afterhours::ui::text_utils
```

**Usage in wordproc:**
```cpp
class TextBuffer {
  void move_word_left() {
    using namespace afterhours::ui::text_utils;
    caret_ = find_word_start(chars_.toString(), caret_);
  }
  
  void move_word_right() {
    using namespace afterhours::ui::text_utils;
    caret_ = find_word_end(chars_.toString(), caret_);
  }
  
  void select_word_at_cursor() {
    using namespace afterhours::ui::text_utils;
    auto [start, end] = select_word_at(chars_.toString(), caret_);
    selection_.anchor = start;
    selection_.cursor = end;
  }
};
```

---

### 6. `TextLayoutCache` - Word Wrap Layout

Caches word-wrapped line layout for efficient rendering.
Applications use this to map text to visual positions.

```cpp
namespace afterhours::ui {

/// A visual line after word wrapping.
/// Multiple VisualLines may correspond to one source line.
struct VisualLine {
  size_t source_offset;  // Byte offset in source text
  size_t length;         // Bytes in this visual line
  float y_position;      // Pixel Y from top of text area
  float width;           // Pixel width of this line
};

/// Caches word-wrapped layout for efficient rendering.
/// Rebuild when text changes, wrap width changes, or font changes.
class TextLayoutCache {
public:
  /// Rebuild layout from text.
  /// @param text The source text
  /// @param wrap_width Maximum line width in pixels (0 = no wrap)
  /// @param measure_fn Function to measure text width: (string_view) -> float
  template <typename MeasureFn>
  void rebuild(std::string_view text, float wrap_width, float line_height,
               MeasureFn measure_fn) {
    lines_.clear();
    total_height_ = 0.f;
    max_width_ = 0.f;
    
    float y = 0.f;
    size_t pos = 0;
    
    while (pos <= text.size()) {
      // Find end of this source line
      size_t line_end = text.find('\n', pos);
      if (line_end == std::string_view::npos) line_end = text.size();
      
      std::string_view source_line = text.substr(pos, line_end - pos);
      
      if (wrap_width > 0 && !source_line.empty()) {
        // Word wrap this line
        wrap_line(source_line, pos, wrap_width, line_height, y, measure_fn);
      } else {
        // No wrapping - single visual line
        float width = source_line.empty() ? 0.f : measure_fn(source_line);
        lines_.push_back({pos, source_line.size(), y, width});
        max_width_ = std::max(max_width_, width);
        y += line_height;
      }
      
      if (line_end >= text.size()) break;
      pos = line_end + 1;  // Skip past \n
    }
    
    // Ensure at least one line
    if (lines_.empty()) {
      lines_.push_back({0, 0, 0.f, 0.f});
      y = line_height;
    }
    
    total_height_ = y;
  }
  
  // Accessors
  const std::vector<VisualLine>& lines() const { return lines_; }
  float total_height() const { return total_height_; }
  float max_width() const { return max_width_; }
  size_t line_count() const { return lines_.size(); }
  
  /// Find visual line containing byte offset
  size_t line_at_offset(size_t offset) const {
    for (size_t i = 0; i < lines_.size(); ++i) {
      if (offset >= lines_[i].source_offset &&
          offset < lines_[i].source_offset + lines_[i].length) {
        return i;
      }
    }
    return lines_.empty() ? 0 : lines_.size() - 1;
  }
  
  /// Find visual line at pixel Y position
  size_t line_at_y(float y, float line_height) const {
    if (y < 0 || lines_.empty()) return 0;
    size_t line = static_cast<size_t>(y / line_height);
    return std::min(line, lines_.size() - 1);
  }
  
  /// Get Y position for a byte offset
  float y_for_offset(size_t offset) const {
    size_t line = line_at_offset(offset);
    return line < lines_.size() ? lines_[line].y_position : 0.f;
  }

private:
  template <typename MeasureFn>
  void wrap_line(std::string_view line, size_t base_offset,
                 float wrap_width, float line_height, float& y,
                 MeasureFn measure_fn) {
    size_t pos = 0;
    
    while (pos < line.size()) {
      // Find how much fits on this visual line
      size_t end = pos;
      size_t last_break = pos;
      
      while (end < line.size()) {
        // Try adding the next word
        size_t word_end = end;
        while (word_end < line.size() && !std::isspace(line[word_end])) ++word_end;
        while (word_end < line.size() && std::isspace(line[word_end])) ++word_end;
        
        std::string_view segment = line.substr(pos, word_end - pos);
        if (measure_fn(segment) > wrap_width && end > pos) {
          // Doesn't fit - break at last break point
          break;
        }
        
        last_break = end;
        end = word_end;
      }
      
      // If no break found, force break at wrap_width
      if (end == pos && pos < line.size()) {
        end = pos + 1;  // At least one character
        while (end < line.size() && 
               measure_fn(line.substr(pos, end - pos)) <= wrap_width) {
          ++end;
        }
        if (end > pos + 1) --end;  // Back up one
      }
      
      std::string_view visual_line = line.substr(pos, end - pos);
      float width = measure_fn(visual_line);
      lines_.push_back({base_offset + pos, end - pos, y, width});
      max_width_ = std::max(max_width_, width);
      y += line_height;
      pos = end;
    }
    
    // Handle empty line
    if (pos == 0) {
      lines_.push_back({base_offset, 0, y, 0.f});
      y += line_height;
    }
  }

  std::vector<VisualLine> lines_;
  float total_height_ = 0.f;
  float max_width_ = 0.f;
};

/// ECS component wrapper
struct HasTextLayoutCache : BaseComponent {
  TextLayoutCache cache;
  uint64_t cached_version = 0;  // For invalidation
  float cached_wrap_width = 0.f;
};

} // namespace afterhours::ui
```

---

## Summary: What Goes Where

### Afterhours Already Provides

| Component | Purpose | File |
|-----------|---------|------|
| `text_input()` | Single-line text input widget | `imm_components.h` |
| `HasTextInputStateT<T>` | Text input state with pluggable storage | `components.h` |
| `TextStorage` concept | Backend abstraction for text storage | `components.h` |
| `text_input_utils::*` | UTF-8 utilities | `text_input_utils.h` |
| `HasScrollView` | Scroll state and viewport management | `components.h` |
| `scroll_view()` | Scrollable container widget | `imm_components.h` |
| `clipboard::*` | System clipboard access | `clipboard.h` |

### Afterhours Should Add (Composable Primitives)

| Component | Purpose | File | Plugin |
|-----------|---------|------|--------|
| `TextSelection` | Selection state (anchor + cursor) | `ui/components.h` | UI |
| `CommandHistory<T>` | Generic undo/redo stack | `command_history.h` | **Standalone** |
| `LineIndex` | Offset ↔ row/column mapping | `ui/text_input_utils.h` | UI |
| `TextLayoutCache` | Word wrap layout | `ui/text_layout.h` | UI |
| `text_utils::*` | Word/paragraph navigation | `ui/text_input_utils.h` | UI |

### Wordproc Composes (Application Layer)

| Wordproc Component | Uses From Afterhours |
|-------------------|----------------------|
| `TextBuffer` | `TextSelection`, `LineIndex`, `CommandHistory<TextBuffer>` |
| `TextLayout` | `TextLayoutCache`, `text_utils::*` |
| Editor input handling | `ClipboardProvider`, `text_utils::*` |
| Undo/Redo commands | `Command<TextBuffer>` base class |

---

## File Organization

```
vendor/afterhours/src/plugins/
├── clipboard.h               # ✅ EXISTS: System clipboard access (standalone)
├── command_history.h         # NEW: CommandHistory<T>, Command<T> (standalone)
│
└── ui/
    ├── components.h          # Add: TextSelection, HasTextSelection
    ├── text_input_utils.h    # Extend: LineIndex, word/paragraph navigation
    ├── text_layout.h         # NEW: TextLayoutCache
    └── imm_components.h      # Unchanged (text_input only)
```

**Key decision:** `command_history.h` is at the `plugins/` level, NOT inside `ui/`,
because undo/redo is useful for non-UI game state (level editors, settings, etc.).

---

## NOT Included in Afterhours

These are application concerns, not game engine concerns:

- ❌ Complete `text_area()` or `text_editor()` widgets
- ❌ Text buffer implementations (gap buffer, rope)
- ❌ Rich text/formatting spans
- ❌ Find/replace
- ❌ Syntax highlighting
- ❌ Document save/load

Applications like wordproc build these using the composable primitives above.

---

## Implementation Phases

### Phase 1: Selection + Line Index
**Files:** `components.h`, `text_input_utils.h`
- Add `TextSelection`, `HasTextSelection`
- Add `LineIndex`, `HasLineIndex`
- Add word/paragraph navigation to `text_utils`

### Phase 2: Command History
**Files:** `command_history.h` (new)
- Generic `Command<T>` interface
- `CommandHistory<T>` with undo/redo/merge

### Phase 3: Text Layout
**Files:** `text_layout.h` (new)
- `TextLayoutCache` with word wrap
- Visual line → source offset mapping
- Hit testing (pixel → text position)

---

## Relationship to Working Implementation

Wordproc already has implementations of these concepts:
- `src/editor/text_buffer.h` - Selection, undo/redo, line navigation
- `src/editor/text_layout.h` - Layout caching

These can be refactored to USE the Afterhours primitives:
1. Replace wordproc's selection tracking with `TextSelection`
2. Replace wordproc's `CommandHistory` with generic version
3. Keep wordproc's `GapBuffer` (application-specific)
4. Keep wordproc's rich text, hyperlinks, etc. (application-specific)

This allows wordproc to benefit from shared code while keeping 
its domain-specific features.
