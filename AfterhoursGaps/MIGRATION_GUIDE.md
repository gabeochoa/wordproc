# Wordproc → Afterhours Migration Guide

**Purpose**: Step-by-step guide for migrating wordproc features to use afterhours plugins.

---

## Table of Contents

1. [Text Utilities (Low-Risk)](#1-text-utilities-low-risk)
2. [Command History (Medium-Risk)](#2-command-history-medium-risk)
3. [Icon Registry (Blocked)](#3-icon-registry-blocked)
4. [Migration Priority Matrix](#4-migration-priority-matrix)

---

## 1. Text Utilities (Low-Risk)

### What Can Be Migrated

**Target**: Word and paragraph navigation functions in `src/editor/text_buffer.cpp`

**Afterhours Location**: `vendor/afterhours/src/plugins/ui/text_input/utils.h`

### Before (Wordproc Custom)

```cpp
// src/editor/text_buffer.cpp
void TextBuffer::moveWordLeft(bool extend_selection) {
  std::string text = getText();
  size_t offset = positionToOffset(caret_);
  
  // Custom word boundary detection
  while (offset > 0) {
    offset--;
    char c = text[offset];
    if (std::isspace(c) || std::ispunct(c)) {
      offset++;
      break;
    }
  }
  
  caret_ = offsetToPosition(offset);
  if (!extend_selection) clearSelection();
}
```

### After (Using Afterhours)

```cpp
// src/editor/text_buffer.cpp
#include "vendor/afterhours/src/plugins/ui/text_input/utils.h"

void TextBuffer::moveWordLeft(bool extend_selection) {
  std::string text = getText();
  size_t offset = positionToOffset(caret_);
  
  // Use afterhours utility - handles UTF-8 correctly
  size_t new_offset = afterhours::text_input::find_word_start(text, offset);
  
  caret_ = offsetToPosition(new_offset);
  if (!extend_selection) clearSelection();
}
```

### Migration Steps

1. **Add include**:
```cpp
#include "vendor/afterhours/src/plugins/ui/text_input/utils.h"
```

2. **Replace word navigation**:

| Wordproc Function | Replace With | Benefit |
|-------------------|--------------|---------|
| Custom word boundary logic | `afterhours::text_input::find_word_start()` | UTF-8 aware |
| Custom word end logic | `afterhours::text_input::find_word_end()` | Consistent behavior |
| Double-click selection | `afterhours::text_input::select_word_at()` | One-liner |
| UTF-8 char length | `afterhours::text_input::utf8_char_length()` | Standards-compliant |

3. **Update `moveWordLeft()`**:
```cpp
void TextBuffer::moveWordLeft(bool extend_selection) {
  std::string text = getText();
  size_t offset = positionToOffset(caret_);
  size_t new_offset = afterhours::text_input::find_word_start(text, offset);
  caret_ = offsetToPosition(new_offset);
  if (!extend_selection) clearSelection();
}
```

4. **Update `moveWordRight()`**:
```cpp
void TextBuffer::moveWordRight(bool extend_selection) {
  std::string text = getText();
  size_t offset = positionToOffset(caret_);
  size_t new_offset = afterhours::text_input::find_word_end(text, offset);
  caret_ = offsetToPosition(new_offset);
  if (!extend_selection) clearSelection();
}
```

5. **Update `selectWordAtCursor()`**:
```cpp
void TextBuffer::selectWordAtCursor() {
  std::string text = getText();
  size_t offset = positionToOffset(caret_);
  auto [start, end] = afterhours::text_input::select_word_at(text, offset);
  setCaret(offsetToPosition(start));
  setSelectionAnchor(caret_);
  setCaret(offsetToPosition(end));
}
```

### What NOT to Migrate

**Do NOT replace**:
- `LineSpan` with `LineIndex` (wordproc needs paragraph styles)
- `CaretPosition` with `TextSelection` (wordproc's formatting is tightly coupled)
- `line_spans_` tracking (wordproc stores indent/spacing/lists per line)

**Why**: Wordproc's rich text features (alignment, indentation, lists, page breaks) require custom line metadata that afterhours doesn't support.

### Testing

1. ✅ Test word navigation (Ctrl+Left/Right)
2. ✅ Test double-click word selection
3. ✅ Test with UTF-8 text (emoji, CJK characters)
4. ✅ Test word boundary detection (spaces, punctuation, symbols)

### Risk Assessment

- **Risk**: ⬜ Low
- **Effort**: ⬜ Low (1-2 hours)
- **Benefit**: ⬜ Medium (better UTF-8 support, less maintenance)
- **Breaking**: ❌ No
- **Recommendation**: ✅ Do it

---

## 2. Command History (Medium-Risk)

### What Can Be Migrated

**Target**: `EditCommand` and `CommandHistory` classes in `src/editor/text_buffer.h`

**Afterhours Location**: `vendor/afterhours/src/plugins/command_history.h`

### Key API Differences

| Aspect | Wordproc | Afterhours |
|--------|----------|------------|
| Command base | `EditCommand` | `Command<State>` (template) |
| State param | `execute(TextBuffer&)` | `execute(State&)` |
| History | `CommandHistory` | `CommandHistory<State>` (template) |
| Return types | `void undo()` | `bool undo()` (returns success) |
| Merging | ❌ Not implemented | ✅ `can_merge_with()` / `merge_with()` |

### Migration Steps

#### Step 1: Change Command Base Class

**Before**:
```cpp
class EditCommand {
public:
  virtual ~EditCommand() = default;
  virtual void execute(TextBuffer& buffer) = 0;
  virtual void undo(TextBuffer& buffer) = 0;
  virtual std::string description() const = 0;
};
```

**After**:
```cpp
#include "vendor/afterhours/src/plugins/command_history.h"

// No need to define EditCommand - use afterhours::Command<TextBuffer>
using TextBufferCommand = afterhours::Command<TextBuffer>;
```

#### Step 2: Update Concrete Commands

**Before**:
```cpp
class InsertCharCommand : public EditCommand {
public:
  InsertCharCommand(CaretPosition pos, char ch) : position_(pos), char_(ch) {}
  void execute(TextBuffer& buffer) override;
  void undo(TextBuffer& buffer) override;
  std::string description() const override { return "Insert char"; }
private:
  CaretPosition position_;
  char char_;
};
```

**After**:
```cpp
class InsertCharCommand : public afterhours::Command<TextBuffer> {
public:
  InsertCharCommand(CaretPosition pos, char ch) : position_(pos), char_(ch) {}
  
  void execute(TextBuffer& buffer) override {
    buffer.insertCharAt(position_, char_);
  }
  
  void undo(TextBuffer& buffer) override {
    buffer.deleteCharAt(position_);
    buffer.setCaret(position_);
  }
  
  std::string description() const override { return "Insert char"; }
  
  // NEW: Enable command merging for consecutive typing
  bool can_merge_with(const Command<TextBuffer>& other) const override {
    auto* other_insert = dynamic_cast<const InsertCharCommand*>(&other);
    if (!other_insert) return false;
    
    // Merge if typing consecutively
    return (other_insert->position_.row == position_.row &&
            other_insert->position_.column == position_.column + 1 &&
            !std::isspace(char_) && !std::isspace(other_insert->char_));
  }
  
  void merge_with(Command<TextBuffer>& other) override {
    // Merge handled by coalescing commands
    // (This is optional; you can leave empty if you prefer)
  }
  
private:
  CaretPosition position_;
  char char_;
};
```

#### Step 3: Update CommandHistory Usage

**Before**:
```cpp
class TextBuffer {
  CommandHistory history_;
  
  void insertChar(char ch) {
    auto cmd = std::make_unique<InsertCharCommand>(caret_, ch);
    history_.execute(std::move(cmd), *this);
  }
  
  void undo() {
    if (history_.canUndo()) {
      history_.undo(*this);
    }
  }
};
```

**After**:
```cpp
#include "vendor/afterhours/src/plugins/command_history.h"

class TextBuffer {
  afterhours::CommandHistory<TextBuffer> history_;  // ← Template parameter
  
  void insertChar(char ch) {
    auto cmd = std::make_unique<InsertCharCommand>(caret_, ch);
    history_.execute(std::move(cmd), *this);  // Same API!
  }
  
  void undo() {
    if (history_.can_undo()) {  // ← Changed from canUndo()
      history_.undo(*this);
    }
  }
  
  void redo() {
    if (history_.can_redo()) {  // ← Changed from canRedo()
      history_.redo(*this);
    }
  }
};
```

#### Step 4: Update All Command Classes

Apply the same pattern to:
- ✅ `InsertCharCommand`
- ✅ `DeleteCharCommand`
- ✅ `DeleteSelectionCommand`
- ✅ Any other custom commands

### Benefits of Migration

1. **Command Merging** - Type multiple chars → one undo step
2. **Return Values** - `undo()` returns `bool` for error handling
3. **Consistency** - Use the same pattern as other afterhours projects
4. **Generic** - Can reuse the pattern for other state types (document settings, etc.)

### Migration Risks

- **Medium Risk**: Changes public API of commands
- **Moderate Effort**: Need to update ~3-5 command classes
- **Breaking**: ⚠️ Yes, but internal to TextBuffer
- **Testing**: ✅ Extensive undo/redo tests already exist

### Testing Checklist

1. ✅ Single character undo/redo
2. ✅ Multi-character typing (should merge into one undo)
3. ✅ Delete operations
4. ✅ Selection deletion
5. ✅ Redo after undo
6. ✅ Clear history after new action
7. ✅ Max history depth (100 commands)
8. ✅ Undo/redo descriptions for UI

### Recommendation

⚠️ **Consider migrating** - Benefits outweigh risks if you have good test coverage.

**Timeline**: 4-6 hours for implementation + testing

---

## 3. Icon Registry (Blocked)

### Status: ⚠️ Cannot Migrate Yet

**Blocker**: Afterhours doesn't have IconRegistry plugin.

**Dependency**: Need to contribute ECS-based `IconRegistry` to afterhours first (see `00_RESEARCH_FINDINGS.md` section 3).

### What Needs to Happen

1. **Redesign API** - Convert from traditional singleton to ECS-based plugin
2. **Upstream PR** - Contribute `ProvidesIconRegistry` component to afterhours
3. **Update Wordproc** - Migrate from `icons().get()` → `icon_registry::get()`

### Estimated Timeline

- **API Redesign**: 2-3 hours
- **Upstream PR Review**: 1-2 weeks (afterhours review cycle)
- **Wordproc Migration**: 1-2 hours (after PR merged)

### Current Workaround

Keep using `src/ui/icon_registry.h` until afterhours accepts the PR.

---

## 4. Migration Priority Matrix

| Feature | Risk | Effort | Benefit | Blocked | Priority |
|---------|------|--------|---------|---------|----------|
| **Text Utilities** | Low | Low | Medium | ❌ No | 🟢 High |
| **Command History** | Medium | Medium | High | ❌ No | 🟡 Medium |
| **Icon Registry** | N/A | High | High | ✅ Yes | 🔴 Low (wait for upstream) |

### Recommended Order

1. ✅ **Phase 1**: Migrate text utilities (low-hanging fruit)
2. ⚠️ **Phase 2**: Migrate command history (requires testing)
3. 🔴 **Phase 3**: Wait for IconRegistry to be accepted upstream

---

## General Migration Principles

### Do:
- ✅ Start with utility functions (lowest risk)
- ✅ Test thoroughly after each migration
- ✅ Keep custom code where afterhours can't handle it (rich text metadata)
- ✅ Contribute back to afterhours when you hit gaps

### Don't:
- ❌ Force-fit afterhours components where custom is better
- ❌ Break existing functionality for the sake of "using afterhours"
- ❌ Remove features to simplify migration

### When in Doubt:
- Ask: "Does afterhours' version handle my use case?"
- Ask: "Is the migration effort worth the maintenance reduction?"
- Ask: "Will this block future afterhours updates?"

---

## Questions or Issues?

**Found a bug in afterhours?** → Open an issue on the afterhours repo  
**Need a feature afterhours doesn't have?** → Check `AfterhoursGaps/` docs, then propose upstream  
**Migration broke something?** → Revert and document why in this guide


