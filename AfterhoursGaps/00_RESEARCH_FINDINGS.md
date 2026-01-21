# Afterhours Migration Research Findings

**Date**: 2025-01-21  
**Purpose**: Detailed analysis of what features exist in afterhours vs. what wordproc uses, with migration recommendations.

---

## Executive Summary

After thorough investigation of the afterhours codebase (`vendor/afterhours/`), we found:

### ✅ Already in Afterhours (Wordproc NOT using them yet)
1. **Command History** - Template-based `Command<State>` system
2. **Text Editing Primitives** - Complete infrastructure (TextSelection, LineIndex, TextLayoutCache, text utils)

### 🎯 Ready to Contribute
1. **Icon Registry** - Missing from afterhours, BUT needs API redesign to match afterhours patterns

### 🔴 Blocking Issues
1. **Render System Const Constraint** - Important architectural limitation

### ✅ Already Resolved
1. **Modal Dialogs** - `plugins/modal.h` exists and wordproc uses it
2. **Status Notifications** - `plugins/toast.h` exists and wordproc uses it

---

## 1. Command History - Template vs Non-Template

### Afterhours Implementation
**Location**: `vendor/afterhours/src/plugins/command_history.h`

```cpp
template <typename State>
struct Command {
  virtual void execute(State &state) = 0;
  virtual void undo(State &state) = 0;
  virtual std::string description() const { return "Command"; }
  virtual bool can_merge_with(const Command &) const { return false; }
  virtual void merge_with(Command &) {}
};

template <typename State>
struct CommandHistory {
  using CommandPtr = std::unique_ptr<Command<State>>;
  std::vector<CommandPtr> undo_stack;
  std::vector<CommandPtr> redo_stack;
  size_t max_depth = 100;
  
  void execute(CommandPtr cmd, State &state);
  bool undo(State &state);
  bool redo(State &state);
  // ...
};
```

### Wordproc Implementation
**Location**: `src/editor/text_buffer.h`

```cpp
class EditCommand {
public:
  virtual void execute(TextBuffer& buffer) = 0;
  virtual void undo(TextBuffer& buffer) = 0;
  virtual std::string description() const = 0;
};

class CommandHistory {
public:
  void execute(std::unique_ptr<EditCommand> cmd, TextBuffer& buffer);
  void undo(TextBuffer& buffer);
  void redo(TextBuffer& buffer);
private:
  std::vector<std::unique_ptr<EditCommand>> undoStack_;
  std::vector<std::unique_ptr<EditCommand>> redoStack_;
};
```

### Key Differences
| Aspect | Afterhours | Wordproc |
|--------|------------|----------|
| API | Template `Command<State>` | Non-template `EditCommand` |
| State Passing | Parameter: `execute(State&)` | Hardcoded: `execute(TextBuffer&)` |
| Flexibility | Works with any state type | Specific to TextBuffer |
| Merging | Has `can_merge_with()`/`merge_with()` | Missing |
| Return values | `bool undo(State&)` returns success | `void undo(TextBuffer&)` |

### Recommendation
✅ **Keep afterhours' template version** - It's more general and already implemented.

**Migration Path for Wordproc**:
```cpp
// Change from:
class InsertCharCommand : public EditCommand {
    void execute(TextBuffer& buffer) override { /*...*/ }
    void undo(TextBuffer& buffer) override { /*...*/ }
};

// To:
class InsertCharCommand : public afterhours::Command<TextBuffer> {
    void execute(TextBuffer& buffer) override { /*...*/ }
    void undo(TextBuffer& buffer) override { /*...*/ }
};

// And use:
afterhours::CommandHistory<TextBuffer> history_;
```

**Impact**: Wordproc needs to refactor command classes but gains command merging capability.

---

## 2. Text Editing Primitives - Already Complete!

### What Afterhours Has
**Location**: `vendor/afterhours/src/plugins/ui/text_input/`

#### TextSelection (`selection.h`)
```cpp
namespace afterhours::text_input {
  struct TextSelection {
    size_t anchor = 0;  // Where selection started
    size_t cursor = 0;  // Current cursor position
    
    bool has_selection() const;
    size_t start() const;
    size_t end() const;
    void set_cursor(size_t pos, bool extend_selection);
    void collapse_to_cursor();
    void select_all(size_t text_length);
    // ...
  };
  
  struct HasTextSelection : BaseComponent {
    TextSelection selection;
  };
}
```

#### LineIndex (`line_index.h`)
```cpp
namespace afterhours::text_input {
  struct LineIndex {
    struct Position { size_t row; size_t column; };
    
    void rebuild(std::string_view text);
    size_t line_count() const;
    size_t line_start(size_t row) const;
    size_t line_end(size_t row) const;
    size_t line_length(size_t row) const;
    Position offset_to_position(size_t offset) const;
    size_t position_to_offset(size_t row, size_t column) const;
    size_t clamp_column(size_t row, size_t column) const;
  };
  
  struct HasLineIndex : BaseComponent {
    LineIndex index;
  };
}
```

#### TextLayoutCache (`text_layout.h`)
```cpp
namespace afterhours::text_input {
  struct VisualLine {
    size_t source_offset;  // Byte offset in source text
    size_t length;         // Bytes in this visual line
    float y_position;      // Pixel Y from top
    float width;           // Pixel width
  };
  
  struct TextLayoutCache {
    template <typename MeasureFn>
    void rebuild(std::string_view text, float wrap_width, 
                 float line_height, MeasureFn measure_fn);
    
    const std::vector<VisualLine>& lines() const;
    float total_height() const;
    float max_width() const;
    size_t line_at_offset(size_t offset) const;
    size_t line_at_y(float y, float line_height) const;
    float y_for_offset(size_t offset) const;
  };
}
```

#### Text Utils (`utils.h`)
```cpp
namespace afterhours::text_input {
  // UTF-8 utilities
  size_t utf8_char_length(const std::string &str, size_t pos);
  size_t utf8_prev_char_start(const std::string &str, size_t pos);
  std::string codepoint_to_utf8(int cp);
  
  // Word navigation
  bool is_word_separator(char c);
  size_t find_word_start(std::string_view text, size_t pos);
  size_t find_word_end(std::string_view text, size_t pos);
  std::pair<size_t, size_t> select_word_at(std::string_view text, size_t pos);
  
  // Multiline navigation
  void move_cursor_up(AnyTextAreaState auto &s);
  void move_cursor_down(AnyTextAreaState auto &s);
  void move_to_line_start(AnyTextAreaState auto &s);
  void move_to_line_end(AnyTextAreaState auto &s);
  bool insert_newline(AnyTextAreaState auto &s);
  bool delete_before_cursor_multiline(AnyTextAreaState auto &s);
}
```

### What Wordproc Currently Uses
**Location**: `src/editor/text_buffer.h` and `text_buffer.cpp`

- Custom `CaretPosition` struct with `{row, column}`
- Custom `rebuildLineIndex()` function that builds `line_spans_`
- Custom `positionToOffset()` and `offsetToPosition()` functions
- Custom `LineSpan` struct with rich paragraph metadata (styles, alignment, indentation, lists, page breaks, drop caps)

### Key Differences
| Feature | Afterhours | Wordproc |
|---------|------------|----------|
| Line tracking | Simple `LineIndex` with offsets | Rich `LineSpan` with paragraph styles |
| Selection | `TextSelection` component | Built-in `anchor_` and `caret_` |
| Word wrap | `TextLayoutCache` generic | Custom layout in render system |
| Metadata | None (just positions) | Extensive (alignment, spacing, lists, etc.) |

### Recommendation
⚠️ **Partial migration possible, but challenging**

**What Wordproc COULD Use**:
- `text_input::utf8_char_length()` and UTF-8 utilities ✅
- `text_input::find_word_start()` / `find_word_end()` for word navigation ✅
- `text_input::select_word_at()` for double-click selection ✅

**What Wordproc CANNOT Easily Use**:
- `LineIndex` - Too simple; wordproc needs paragraph styles, indentation, lists, page breaks
- `TextSelection` - Wordproc's selection is tightly integrated with formatting
- `TextLayoutCache` - Wordproc's layout includes rich text rendering

**Migration Path**:
```cpp
// Wordproc can adopt the utility functions:
#include <afterhours/src/plugins/ui/text_input/utils.h>

void TextBuffer::moveWordLeft() {
  std::string text = getText();
  size_t offset = positionToOffset(caret_);
  size_t new_offset = afterhours::text_input::find_word_start(text, offset);
  caret_ = offsetToPosition(new_offset);
}

void TextBuffer::selectWordAtCursor() {
  std::string text = getText();
  size_t offset = positionToOffset(caret_);
  auto [start, end] = afterhours::text_input::select_word_at(text, offset);
  setCaret(offsetToPosition(start));
  setSelectionAnchor(caret_);
  setCaret(offsetToPosition(end));
}
```

**Impact**: Low-risk incremental adoption of utility functions without breaking wordproc's rich text features.

---

## 3. Icon Registry - API Redesign Needed

### Wordproc's Design (Traditional Singleton)
**Location**: `src/extracted/icon_registry.h`

```cpp
namespace afterhours::ui {
  class IconRegistry {
  public:
    static IconRegistry& instance() {  // ⚠️ Traditional singleton
      static IconRegistry inst;
      return inst;
    }
    
    void register_icon(const std::string& id, const std::string& name, ...);
    std::optional<IconInfo> get(const std::string& id) const;
    bool has_icon(const std::string& id) const;
    char get_symbol(const std::string& id) const;
    
  private:
    IconRegistry() = default;  // Private constructor
    std::unordered_map<std::string, IconInfo> icons_;
  };
  
  // Convenience function
  inline IconRegistry& icons() {
    return IconRegistry::instance();
  }
}
```

### Afterhours Pattern (ECS-Based Singleton)
**Examples**: `files.h`, `settings.h`, `window_manager.h`

```cpp
namespace afterhours::icon_registry {
  
  // 1. Component that holds the data
  struct ProvidesIconRegistry : BaseComponent {
    std::unordered_map<std::string, IconInfo> icons;
    
    void register_icon(const std::string& id, const IconInfo& info);
    std::optional<IconInfo> get(const std::string& id) const;
    bool has_icon(const std::string& id) const;
  };
  
  // 2. Plugin initialization functions
  static void add_singleton_components(Entity &entity) {
    entity.addComponent<ProvidesIconRegistry>();
    EntityHelper::registerSingleton<ProvidesIconRegistry>(entity);
  }
  
  static void enforce_singletons(SystemManager &sm) {
    sm.register_update_system(
      std::make_unique<developer::EnforceSingleton<ProvidesIconRegistry>>());
  }
  
  // 3. API functions that access the singleton
  static ProvidesIconRegistry* get_provider() {
    return EntityHelper::get_singleton_cmp<ProvidesIconRegistry>();
  }
  
  static void init() {
    if (EntityHelper::get_default_collection()
            .has_singleton<ProvidesIconRegistry>()) {
      log_warn("Icon registry already initialized");
      return;
    }
    Entity &entity = EntityHelper::createPermanentEntity();
    add_singleton_components(entity);
    EntityHelper::merge_entity_arrays();
  }
  
  static void register_icon(const std::string& id, const IconInfo& info) {
    auto* provider = get_provider();
    if (!provider) {
      log_error("Icon registry not initialized. Call icon_registry::init()");
      return;
    }
    provider->register_icon(id, info);
  }
  
  static std::optional<IconInfo> get(const std::string& id) {
    auto* provider = get_provider();
    if (!provider) return std::nullopt;
    return provider->get(id);
  }
}
```

### Why This Matters

**Afterhours Philosophy**:
- All persistent state lives in ECS components
- Singleton components are registered via `EntityHelper`
- Systems enforce singleton constraints
- Testable (can create temporary entities for testing)
- Can serialize/deserialize with the rest of the ECS

**Wordproc's Singleton**:
- Global static instance
- Not integrated with ECS
- Cannot be easily serialized
- Hidden state separate from game state

### Recommendation
🔄 **Redesign IconRegistry to follow afterhours ECS patterns**

**Benefits**:
1. ✅ Consistent with afterhours architecture
2. ✅ Can be reset/tested easily
3. ✅ Can be serialized with game state
4. ✅ No hidden global state
5. ✅ Fits the `PluginCore` concept

**Changes Needed**:
1. Convert `IconRegistry` class → `ProvidesIconRegistry` component
2. Add `init()`, `add_singleton_components()`, `enforce_singletons()`
3. Change API from `IconRegistry::instance().get()` → `icon_registry::get()`
4. Add `IconInfo` struct to the namespace (currently in class)

**Migration Impact**:
- **Wordproc**: Change from `icons().register_icon()` → `icon_registry::register_icon()`
- **Users**: Must call `icon_registry::init()` during startup
- **API Surface**: Similar but follows afterhours conventions

---

## 4. Render System Const Constraint

### The Problem
**Location**: `vendor/afterhours/src/core/system.h` lines 393-410

```cpp
void SystemManager::render(const Entities &entities, const float dt) {
  for (const auto &system : render_systems_) {
    const SystemBase &sys = *system;     // ⚠️ CONST cast
    sys.once(dt);
    for (std::shared_ptr<Entity> entity : entities) {
      const Entity &e = *entity;         // ⚠️ CONST cast
      sys.for_each(e, dt);                // Calls const version
    }
    sys.after(dt);
  }
}
```

This means render systems can ONLY implement:
```cpp
void for_each_with(const Entity& e, const Component& c, float dt) const override
```

### Why This Is Problematic

**Immediate-Mode UI Patterns Require Mutation**:
1. **Closing dialogs** - Button click should set `modal.active = false`
2. **Scrollbar dragging** - Mouse input should update `scroll.offset`
3. **Text cursor blinking** - Timer should toggle `cursor.visible`
4. **Hover states** - Mouse hover should set `button.hovered = true`

### Current Wordproc Workaround
**Location**: `src/ecs/render_system.h`

```cpp
void for_each_with(const Entity& /*entity*/,
                   const DocumentComponent& docConst,
                   const MenuComponent& menuConst,
                   const float) const override {
    // ⚠️ const_cast - technically undefined behavior!
    auto& doc = const_cast<DocumentComponent&>(docConst);
    auto& menu = const_cast<MenuComponent&>(menuConst);
    renderMenus(doc, menu, status, layout);
}
```

### Proposed Solution (from gap 05)

**Add mutable render system registration**:
```cpp
// In SystemManager
std::vector<bool> render_system_is_mutable_;

void register_mutable_render_system(std::unique_ptr<SystemBase> system) {
  render_systems_.push_back(std::move(system));
  render_system_is_mutable_.push_back(true);
}

void render(Entities &entities, const float dt) {  // Non-const entities
  for (size_t i = 0; i < render_systems_.size(); ++i) {
    auto &system = render_systems_[i];
    
    if (render_system_is_mutable_[i]) {
      // Mutable path - same as tick()
      system->for_each(*entity, dt);  // Calls non-const for_each_with
    } else {
      // Const path - existing behavior
      const SystemBase &sys = *system;
      const Entity &e = *entity;
      sys.for_each(e, dt);  // Calls const for_each_with
    }
  }
}
```

### Recommendation
⚠️ **Important but requires careful design**

**Why It's Important**:
- Blocks scrollable containers (can't update scroll offset)
- Blocks modal interactions (can't close on button click)
- Blocks text editor cursor updates (can't blink cursor)
- Forces widespread use of `const_cast` (undefined behavior)

**Why It's Not Urgent**:
- `const_cast` workaround is functional (though technically UB)
- Changing this affects ALL afterhours users
- Need to ensure backward compatibility
- Need input from afterhours maintainers

**Alternative: Use Update Systems**:
- Move ALL state mutations to update systems
- Render systems become purely visual (no state changes)
- More ECS-compliant but breaks immediate-mode UI patterns

---

## 5. Drawing Primitives Gap

### Missing from Afterhours
**Checked**: `vendor/afterhours/src/drawing_helpers.h`

Currently missing (wordproc calls raylib directly):
- `draw_line()` / `draw_line_ex()`
- `draw_circle()` / `draw_circle_filled()`
- `draw_ellipse()` / `draw_ellipse_lines()`
- `draw_triangle()` / `draw_triangle_filled()`
- `draw_polygon()` / `draw_line_strip()`

### Wordproc Workaround
**Location**: `src/ui/drawing.h`

```cpp
namespace draw {
  // Routes through afterhours where available
  inline void text(...) { afterhours::draw_text(...); }
  inline void rectangle(...) { afterhours::draw_rectangle(...); }
  
  // Still calls raylib directly
  inline void line(float x1, float y1, float x2, float y2, Color color) {
    raylib::DrawLine(x1, y1, x2, y2, color);  // ⚠️ Direct raylib call
  }
  
  inline void circle(float x, float y, float radius, Color color) {
    raylib::DrawCircle(x, y, radius, color);  // ⚠️ Direct raylib call
  }
}
```

### Recommendation
➡️ **Add missing primitives to afterhours**

**Impact**: Low risk, high value
- Completes the drawing abstraction
- Enables backend swapping (raylib, sokol, headless)
- Small API surface (5-6 functions)
- No breaking changes

**Priority**: Medium (wordproc workaround is functional)

---

## Summary & Action Items

### ✅ Documentation Updates (Immediate)
1. Update `03_text_editing_widget.md` to mark as ✅ RESOLVED
2. Update `10_command_history.md` to mark as ✅ RESOLVED
3. Add note that features exist but wordproc hasn't migrated yet

### 📝 Migration Guides (Next)
1. Create guide: "Migrating to afterhours::CommandHistory<T>"
2. Create guide: "Using afterhours text_input utilities"
3. Document IconRegistry API redesign

### 🎯 Upstream Contributions (When Ready)
1. **Icon Registry** - After API redesign to ECS pattern
2. **Drawing Primitives** - Add missing line/circle/ellipse functions
3. **Mutable Render Systems** - After community discussion

### 🔍 Questions for Afterhours Maintainers
1. Is the template `Command<State>` API the preferred approach?
2. Would they accept IconRegistry as an ECS-based plugin?
3. What's their position on mutable render systems?
4. Any concerns about adding basic drawing primitives?


