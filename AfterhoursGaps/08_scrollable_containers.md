# Scrollable Containers

## Working Implementation
See these files for a complete working example:
- `src/ecs/components.h` - ScrollComponent struct
- `src/ecs/input_system.h` - Scroll input handling (search for "scroll")
- `src/ecs/render_system.h` - Scroll-aware rendering

## Problem
Afterhours UI does not provide scrollable containers or scroll view components. There is 
no mouse wheel input handling for UI scrolling. The autolayout system has no concept of 
content overflow or scrolling.

## Current Workaround
Custom `ScrollComponent` in `src/ecs/components.h`:

```cpp
struct ScrollComponent : public afterhours::BaseComponent {
    int offset = 0;         // Scroll offset in lines
    int visibleLines = 20;  // Number of visible lines
    int maxScroll = 0;      // Maximum scroll value
    int secondaryOffset = 0;  // Secondary scroll offset for split view
};
```

Manual scroll handling in `input_system.h`:
```cpp
// NavigationSystem::for_each_with() - lines 573-582
float wheelMove = GetMouseWheelMove();  // Direct raylib call
if (wheelMove != 0.0f) {
    int scrollLines = static_cast<int>(-wheelMove * 3);
    if (layout.splitViewEnabled && shift_down) {
        scroll.secondaryOffset += scrollLines;
        clampSecondary();
    } else {
        scroll.offset += scrollLines;
    }
}
```

Rendering clipping is done manually in `render_system.h` by checking Y bounds:
```cpp
if (y > static_cast<int>(textArea.y + textArea.height)) {
    break;
}
```

## What Afterhours Currently Provides

### Input System (`plugins/input_system.h`)
The `afterhours::input` namespace wraps raylib input functions but is missing mouse wheel:

| Function | Wrapped? | Notes |
|----------|----------|-------|
| `get_mouse_position()` | ✅ Yes | With resolution scaling support |
| `is_mouse_button_down()` | ✅ Yes | |
| `is_mouse_button_pressed()` | ✅ Yes | |
| `is_mouse_button_released()` | ✅ Yes | |
| `is_key_pressed()` | ✅ Yes | |
| `is_key_down()` | ✅ Yes | |
| `get_char_pressed()` | ✅ Yes | |
| `GetMouseWheelMove()` | ❌ No | **Must call raylib directly** |
| `GetMouseWheelMoveV()` | ❌ No | For 2D trackpad scrolling |

### Drawing Helpers (`drawing_helpers.h`)
Provides rectangle and text drawing but no scissor/clipping:

| Function | Available? | Notes |
|----------|------------|-------|
| `draw_rectangle()` | ✅ Yes | |
| `draw_rectangle_outline()` | ✅ Yes | |
| `draw_rectangle_rounded()` | ✅ Yes | |
| `draw_text()` | ✅ Yes | |
| `draw_text_ex()` | ✅ Yes | |
| `BeginScissorMode()` | ❌ No | Must call raylib directly |
| `EndScissorMode()` | ❌ No | Must call raylib directly |
| `rlPushMatrix()` | ❌ No | For content translation |
| `rlTranslatef()` | ❌ No | For content translation |
| `rlPopMatrix()` | ❌ No | For content translation |

### Autolayout (`plugins/autolayout.h`)
The `AutoLayout` class handles sizing and positioning but has no scroll awareness:

- Children are positioned within parent bounds
- Content that exceeds parent bounds is not handled
- No `overflow: scroll` or `overflow: hidden` concept
- `UIComponent::rect()` returns the final positioned rectangle
- `UIComponent::bounds()` includes padding and margin

### UI Context (`plugins/ui/context.h`)
The `UIContext<InputAction>` manages focus, hot/active state, and mouse tracking:

- `mouse.pos` - Current mouse position
- `mouse.left_down` - Whether left button is down
- `is_mouse_inside()` - Helper to check if point is in rectangle
- **No scroll offset awareness** - Hit testing doesn't account for scrolled content

### UI Components (`plugins/ui/components.h`)
Existing component patterns to follow:

```cpp
// Example: HasCheckboxState
struct HasCheckboxState : BaseComponent {
  bool changed_since = false;
  bool on;
  HasCheckboxState(bool b) : on(b) {}
};

// Example: HasSliderState
struct HasSliderState : BaseComponent {
  bool changed_since = false;
  float value;
  HasSliderState(float val) : value(val) {}
};
```

## Required Feature: Complete Scroll View API

### 1. Input System Extensions (`plugins/input_system.h`)

Add to the `afterhours::input` struct alongside existing wrappers:

```cpp
#ifdef AFTER_HOURS_USE_RAYLIB
  // Mouse wheel input (vertical)
  static float get_mouse_wheel_move() {
    return raylib::GetMouseWheelMove();
  }
  
  // Mouse wheel input (2D - for trackpads)
  static Vector2Type get_mouse_wheel_move_v() {
    raylib::Vector2 v = raylib::GetMouseWheelMoveV();
    return Vector2Type{v.x, v.y};
  }
#else
  static float get_mouse_wheel_move() { return 0.f; }
  static Vector2Type get_mouse_wheel_move_v() { return {0.f, 0.f}; }
#endif
```

### 2. Drawing Helper Extensions (`drawing_helpers.h`)

Add scissor mode wrappers:

```cpp
#ifdef AFTER_HOURS_USE_RAYLIB
  // Clipping/Scissor support for scroll containers
  inline void begin_scissor_mode(int x, int y, int width, int height) {
    raylib::BeginScissorMode(x, y, width, height);
  }
  
  inline void begin_scissor_mode(const RectangleType& rect) {
    raylib::BeginScissorMode(
      static_cast<int>(rect.x), 
      static_cast<int>(rect.y),
      static_cast<int>(rect.width), 
      static_cast<int>(rect.height)
    );
  }
  
  inline void end_scissor_mode() {
    raylib::EndScissorMode();
  }
  
  // Matrix operations for scroll translation
  inline void push_matrix() { raylib::rlPushMatrix(); }
  inline void pop_matrix() { raylib::rlPopMatrix(); }
  inline void translate(float x, float y, float z = 0.f) { 
    raylib::rlTranslatef(x, y, z); 
  }
#else
  inline void begin_scissor_mode(int, int, int, int) {}
  inline void begin_scissor_mode(const RectangleType&) {}
  inline void end_scissor_mode() {}
  inline void push_matrix() {}
  inline void pop_matrix() {}
  inline void translate(float, float, float = 0.f) {}
#endif
```

### 3. Scroll Component (`plugins/ui/components.h`)

Following Afterhours component patterns (like `HasSliderState`, `HasCheckboxState`):

```cpp
namespace afterhours::ui {

// Scroll state component - manages scroll position and bounds
struct HasScrollView : BaseComponent {
  // Current scroll offset in pixels
  Vector2Type scroll_offset = {0.f, 0.f};
  
  // Total size of scrollable content (set by layout/user)
  Vector2Type content_size = {0.f, 0.f};
  
  // Size of the visible viewport (set by layout from UIComponent)
  Vector2Type viewport_size = {0.f, 0.f};
  
  // Scroll direction enables
  bool horizontal_enabled = false;
  bool vertical_enabled = true;
  
  // Pixels scrolled per mouse wheel tick
  float scroll_speed = 20.0f;
  
  // Whether content changed since last frame (for systems to react)
  bool changed_since = false;
  
  // Computed: Maximum scroll values
  float max_scroll_x() const { 
    return std::max(0.f, content_size.x - viewport_size.x); 
  }
  float max_scroll_y() const { 
    return std::max(0.f, content_size.y - viewport_size.y); 
  }
  
  // Computed: Scroll position as 0-1 ratio
  float scroll_ratio_x() const {
    float max = max_scroll_x();
    return max > 0.f ? scroll_offset.x / max : 0.f;
  }
  float scroll_ratio_y() const {
    float max = max_scroll_y();
    return max > 0.f ? scroll_offset.y / max : 0.f;
  }
  
  // Computed: Whether scroll is possible in each direction
  bool can_scroll_up() const { return scroll_offset.y > 0.f; }
  bool can_scroll_down() const { return scroll_offset.y < max_scroll_y(); }
  bool can_scroll_left() const { return scroll_offset.x > 0.f; }
  bool can_scroll_right() const { return scroll_offset.x < max_scroll_x(); }
  bool can_scroll() const {
    return can_scroll_up() || can_scroll_down() || 
           can_scroll_left() || can_scroll_right();
  }
  
  // Clamp scroll offset to valid range
  void clamp() {
    scroll_offset.x = std::clamp(scroll_offset.x, 0.f, max_scroll_x());
    scroll_offset.y = std::clamp(scroll_offset.y, 0.f, max_scroll_y());
  }
  
  // Scroll to specific position (with clamping)
  void scroll_to(float x, float y) {
    scroll_offset.x = x;
    scroll_offset.y = y;
    clamp();
  }
  
  // Scroll by delta amount (with clamping)
  void scroll_by(float dx, float dy) {
    scroll_offset.x += dx;
    scroll_offset.y += dy;
    clamp();
  }
  
  // Scroll to ensure a rect within content is visible
  void scroll_to_visible(RectangleType rect) {
    // Scroll up/left if rect is before viewport
    if (rect.y < scroll_offset.y) {
      scroll_offset.y = rect.y;
    }
    if (rect.x < scroll_offset.x) {
      scroll_offset.x = rect.x;
    }
    // Scroll down/right if rect is after viewport
    if (rect.y + rect.height > scroll_offset.y + viewport_size.y) {
      scroll_offset.y = rect.y + rect.height - viewport_size.y;
    }
    if (rect.x + rect.width > scroll_offset.x + viewport_size.x) {
      scroll_offset.x = rect.x + rect.width - viewport_size.x;
    }
    clamp();
  }
};

// Optional: Scrollbar appearance configuration
struct HasScrollbarStyle : BaseComponent {
  float width = 12.0f;
  float min_thumb_size = 20.0f;
  
  Color track_color = {200, 200, 200, 100};
  Color thumb_color = {128, 128, 128, 255};
  Color thumb_hover_color = {100, 100, 100, 255};
  Color thumb_active_color = {80, 80, 80, 255};
  
  bool auto_hide = true;           // Hide when content fits
  float auto_hide_delay = 1.5f;    // Seconds of inactivity before hiding
  bool overlay = false;            // Draw over content vs taking space
};

// Tag component for elements that should clip their children
struct ClipsChildren : BaseComponent {};

} // namespace afterhours::ui
```

### 4. Scroll Input System (`plugins/ui/systems.h`)

Following the pattern of `HandleClicks`, `HandleDrags`, etc:

```cpp
namespace afterhours::ui {

template <typename InputAction>
struct HandleScrollInput : SystemWithUIContext<HasScrollView> {
  UIContext<InputAction>* context;
  
  virtual void once(float) override {
    this->context = EntityHelper::get_singleton_cmp<UIContext<InputAction>>();
  }
  
  virtual ~HandleScrollInput() {}
  
  virtual void for_each_with(Entity& entity, UIComponent& cmp,
                             HasScrollView& scroll, float) override {
    if (!cmp.was_rendered_to_screen) return;
    if (cmp.should_hide || entity.has<ShouldHide>()) return;
    
    // Update viewport size from UIComponent
    scroll.viewport_size = {cmp.rect().width, cmp.rect().height};
    
    // Check if mouse is over this scroll container
    RectangleType rect = cmp.rect();
    if (entity.has<HasUIModifiers>()) {
      rect = entity.get<HasUIModifiers>().apply_modifier(rect);
    }
    
    if (!is_mouse_inside(context->mouse.pos, rect)) return;
    
    // Handle mouse wheel
    float wheel = input::get_mouse_wheel_move();
    if (wheel != 0.f) {
      if (scroll.vertical_enabled) {
        scroll.scroll_by(0.f, -wheel * scroll.scroll_speed);
        scroll.changed_since = true;
      } else if (scroll.horizontal_enabled) {
        // If only horizontal enabled, use vertical wheel for horizontal scroll
        scroll.scroll_by(-wheel * scroll.scroll_speed, 0.f);
        scroll.changed_since = true;
      }
    }
    
    // Handle 2D wheel (trackpad)
    Vector2Type wheel_v = input::get_mouse_wheel_move_v();
    if (wheel_v.x != 0.f && scroll.horizontal_enabled) {
      scroll.scroll_by(-wheel_v.x * scroll.scroll_speed, 0.f);
      scroll.changed_since = true;
    }
    if (wheel_v.y != 0.f && scroll.vertical_enabled) {
      scroll.scroll_by(0.f, -wheel_v.y * scroll.scroll_speed);
      scroll.changed_since = true;
    }
    
    // Keyboard scrolling when focused
    if (context->has_focus(entity.id)) {
      if constexpr (magic_enum::enum_contains<InputAction>("WidgetUp")) {
        if (context->pressed(InputAction::WidgetUp)) {
          scroll.scroll_by(0.f, -scroll.scroll_speed);
          scroll.changed_since = true;
        }
      }
      if constexpr (magic_enum::enum_contains<InputAction>("WidgetDown")) {
        if (context->pressed(InputAction::WidgetDown)) {
          scroll.scroll_by(0.f, scroll.scroll_speed);
          scroll.changed_since = true;
        }
      }
      // Page Up/Down could be handled similarly
    }
  }
};

} // namespace afterhours::ui
```

### 5. Scroll-Aware Rendering (`plugins/ui/rendering.h`)

Modify `RenderImm` to handle scroll containers:

```cpp
template <typename InputAction>
struct RenderImm : System<UIContext<InputAction>, FontManager> {
  // ... existing code ...
  
  void render(const UIContext<InputAction>& context,
              const FontManager& font_manager, const Entity& entity) const {
    const UIComponent& cmp = entity.get<UIComponent>();
    if (cmp.should_hide || entity.has<ShouldHide>()) return;
    
    // ... existing font handling ...
    
    // Check if this is a scroll container
    bool is_scroll_container = entity.has<HasScrollView>();
    bool clips_children = entity.has<ClipsChildren>() || is_scroll_container;
    
    // Render the container itself (background, border, etc)
    if (entity.has<HasColor>() || entity.has<HasLabel>() || /* etc */) {
      render_me(context, font_manager, entity);
    }
    
    // Set up clipping if needed
    if (clips_children) {
      RectangleType rect = cmp.rect();
      if (entity.has<HasUIModifiers>()) {
        rect = entity.get<HasUIModifiers>().apply_modifier(rect);
      }
      begin_scissor_mode(rect);
    }
    
    // Set up scroll translation if this is a scroll view
    if (is_scroll_container) {
      const auto& scroll = entity.get<HasScrollView>();
      push_matrix();
      translate(-scroll.scroll_offset.x, -scroll.scroll_offset.y);
    }
    
    // Render children
    for (EntityID child : cmp.children) {
      render(context, font_manager, AutoLayout::to_ent_static(child));
    }
    
    // Clean up scroll translation
    if (is_scroll_container) {
      pop_matrix();
      
      // Render scrollbars on top (not translated)
      if (entity.has<HasScrollbarStyle>()) {
        render_scrollbars(entity, context);
      }
    }
    
    // Clean up clipping
    if (clips_children) {
      end_scissor_mode();
    }
  }
  
  void render_scrollbars(const Entity& entity, 
                         const UIContext<InputAction>& context) const {
    const UIComponent& cmp = entity.get<UIComponent>();
    const HasScrollView& scroll = entity.get<HasScrollView>();
    const HasScrollbarStyle& style = entity.get<HasScrollbarStyle>();
    RectangleType rect = cmp.rect();
    
    // Vertical scrollbar
    if (scroll.vertical_enabled && scroll.can_scroll()) {
      float track_height = rect.height;
      float thumb_height = std::max(
        style.min_thumb_size,
        track_height * (scroll.viewport_size.y / scroll.content_size.y)
      );
      float thumb_y = rect.y + (track_height - thumb_height) * scroll.scroll_ratio_y();
      
      // Track
      RectangleType track = {
        rect.x + rect.width - style.width, rect.y,
        style.width, track_height
      };
      draw_rectangle(track, style.track_color);
      
      // Thumb
      RectangleType thumb = {
        rect.x + rect.width - style.width, thumb_y,
        style.width, thumb_height
      };
      
      Color thumb_color = style.thumb_color;
      if (is_mouse_inside(context.mouse.pos, thumb)) {
        thumb_color = context.mouse.left_down 
          ? style.thumb_active_color 
          : style.thumb_hover_color;
      }
      draw_rectangle(thumb, thumb_color);
    }
    
    // Horizontal scrollbar (similar logic)
    if (scroll.horizontal_enabled && scroll.can_scroll_left() || scroll.can_scroll_right()) {
      // ... similar implementation ...
    }
  }
};
```

### 6. Scroll-Aware Hit Testing

Modify `UIContext` or create helper for scroll-aware hit testing:

```cpp
namespace afterhours::ui {

// Helper to check if point is inside rect, accounting for scroll offset
static inline bool is_mouse_inside_scrolled(
    const input::MousePosition& mouse_pos,
    const RectangleType& rect,
    const HasScrollView* scroll = nullptr) {
  Vector2Type effective_pos = {mouse_pos.x, mouse_pos.y};
  if (scroll) {
    effective_pos.x += scroll->scroll_offset.x;
    effective_pos.y += scroll->scroll_offset.y;
  }
  return effective_pos.x >= rect.x && 
         effective_pos.x <= rect.x + rect.width &&
         effective_pos.y >= rect.y && 
         effective_pos.y <= rect.y + rect.height;
}

// Find the scroll context for a given entity (walks up parent chain)
static inline const HasScrollView* find_scroll_parent(const Entity& entity) {
  const Entity* current = &entity;
  int guard = 0;
  while (current && ++guard < 64) {
    if (current->has<HasScrollView>()) {
      return &current->get<HasScrollView>();
    }
    if (!current->has<UIComponent>()) break;
    EntityID parent_id = current->get<UIComponent>().parent;
    if (parent_id < 0) break;
    current = &EntityHelper::getEntityForIDEnforce(parent_id);
  }
  return nullptr;
}

} // namespace afterhours::ui
```

### 7. Autolayout Integration

Add overflow handling to `AutoLayout`:

```cpp
// In AutoLayout class
void compute_content_size(UIComponent& widget) {
  // After layout is complete, compute content size for scroll containers
  if (!widget.children.empty()) {
    float max_x = 0.f;
    float max_y = 0.f;
    
    for (EntityID child_id : widget.children) {
      UIComponent& child = this->to_cmp(child_id);
      if (child.absolute || child.should_hide) continue;
      
      float child_right = child.computed_rel[Axis::X] + child.computed[Axis::X];
      float child_bottom = child.computed_rel[Axis::Y] + child.computed[Axis::Y];
      
      max_x = std::max(max_x, child_right);
      max_y = std::max(max_y, child_bottom);
    }
    
    // Update scroll component if present
    Entity& ent = to_ent(widget.id);
    if (ent.has<HasScrollView>()) {
      auto& scroll = ent.get<HasScrollView>();
      scroll.content_size = {max_x, max_y};
      scroll.viewport_size = {widget.computed[Axis::X], widget.computed[Axis::Y]};
    }
  }
  
  // Recurse
  for (EntityID child_id : widget.children) {
    compute_content_size(this->to_cmp(child_id));
  }
}
```

## System Registration Order

Add scroll handling to the system registration in correct order:

```cpp
// In your app initialization, after other UI systems:
sm.register_update_system(std::make_unique<HandleScrollInput<InputAction>>());

// Ensure RenderImm handles scroll containers
// (usually already registered for rendering)
```

## Integration Checklist

When implementing scroll support in Afterhours:

1. **Input** (`plugins/input_system.h`)
   - [ ] Add `get_mouse_wheel_move()` wrapper
   - [ ] Add `get_mouse_wheel_move_v()` wrapper for 2D/trackpad

2. **Drawing** (`drawing_helpers.h`)
   - [ ] Add `begin_scissor_mode()` wrapper
   - [ ] Add `end_scissor_mode()` wrapper  
   - [ ] Add `push_matrix()`, `pop_matrix()`, `translate()` wrappers

3. **Components** (`plugins/ui/components.h`)
   - [ ] Add `HasScrollView` component
   - [ ] Add `HasScrollbarStyle` component (optional)
   - [ ] Add `ClipsChildren` tag component

4. **Systems** (`plugins/ui/systems.h`)
   - [ ] Add `HandleScrollInput<InputAction>` system
   - [ ] Add `HandleScrollbarDrag<InputAction>` system (optional)

5. **Rendering** (`plugins/ui/rendering.h`)
   - [ ] Modify `RenderImm::render()` to handle scissor mode
   - [ ] Modify `RenderImm::render()` to handle scroll translation
   - [ ] Add scrollbar rendering

6. **Layout** (`plugins/autolayout.h`)
   - [ ] Add content size calculation after layout
   - [ ] Consider `overflow` property on UIComponent

7. **Hit Testing** (`plugins/ui/context.h`)
   - [ ] Add scroll-aware mouse position helpers
   - [ ] Modify `active_if_mouse_inside()` for scrolled content

## Interactions with Other Gaps

This feature has significant interactions with other Afterhours gaps. Understanding these 
dependencies is critical for proper implementation ordering.

### ⚠️ BLOCKER: 05_render_system_const_constraint.md

**Severity:** Must resolve BEFORE implementing scroll rendering

The scroll system as proposed requires **mutable access during rendering**:

1. **Scrollbar hover state** - `RenderImm::render_scrollbars()` (section 5) checks 
   `is_mouse_inside(context.mouse.pos, thumb)` and needs to update visual state
   
2. **Scroll position updates** - Mouse wheel input during render would ideally update 
   `HasScrollView::scroll_offset` immediately for responsive feel

3. **The const constraint problem:**
   ```cpp
   // Current Afterhours render dispatch (system.h)
   const Entity &e = *entity;  // <- CONST!
   sys.for_each(e, dt);        // <- Calls const for_each_with
   
   // Our proposed scroll rendering needs:
   void for_each_with(Entity& entity,  // <- Needs mutable!
                      HasScrollView& scroll, ...) override {
       scroll.scroll_by(wheel_delta);  // <- Can't do this in const render
   }
   ```

**Resolution options (from 05):**
- **Option A:** Use `register_mutable_render_system()` for scroll-aware render systems
- **Option B:** Move all scroll state updates to update systems, render is purely visual
- **Option C:** Use `const_cast` workaround (current wordproc approach - not ideal)

**Recommendation:** Implement Option A from 05 first, then scroll containers can cleanly 
separate:
- `HandleScrollInput` → update system (mutable, handles wheel/keyboard input)
- `RenderScrollbars` → mutable render system (handles scrollbar dragging feedback)
- `RenderClippedChildren` → const render system (just draws with scissor)

### 🔗 DEPENDENCY: 07_renderer_abstraction.md

**Severity:** Should coordinate implementation

The scissor/clipping functions proposed in section 2 of this document conflict with 07's 
renderer abstraction proposal:

| This Document (08) | Renderer Abstraction (07) |
|--------------------|---------------------------|
| Adds to `drawing_helpers.h` | Replaces with `graphics.h` |
| Direct raylib calls with wrapper | Concept-based backend abstraction |
| `begin_scissor_mode(rect)` | Should be `BeginScissorMode(rect)` in graphics.h |

**Proposed alignment:**

Add to the `GraphicsBackend` concept in 07:

```cpp
template<typename T>
concept GraphicsBackend = requires(T t, /* ... existing ... */
    Rectangle clip_rect) 
{
  // ... existing requirements ...
  
  // Clipping support (for scroll containers)
  { t.begin_scissor_mode(clip_rect) } -> std::same_as<void>;
  { t.end_scissor_mode() } -> std::same_as<void>;
  
  // Matrix operations (for scroll translation)
  { t.push_matrix() } -> std::same_as<void>;
  { t.pop_matrix() } -> std::same_as<void>;
  { t.translate(0.f, 0.f) } -> std::same_as<void>;
};
```

And add to `graphics.h` free functions:

```cpp
inline void BeginScissorMode(Rectangle r) { detail::g_backend.begin_scissor_mode(r); }
inline void EndScissorMode() { detail::g_backend.end_scissor_mode(); }
inline void PushMatrix() { detail::g_backend.push_matrix(); }
inline void PopMatrix() { detail::g_backend.pop_matrix(); }
inline void Translate(float x, float y) { detail::g_backend.translate(x, y); }
```

**If 07 ships first:** Update section 2 of this document to use the new graphics.h API
**If 08 ships first:** Add to drawing_helpers.h as proposed, migrate later when 07 ships

### 🔗 CONSUMER: 03_text_editing_widget.md

**Severity:** Major integration point

The text editor is the primary consumer of scroll functionality:

| Text Editor Need | Scroll Feature |
|------------------|----------------|
| "Scrolling - No horizontal scroll for long text" (line 38) | `HasScrollView.horizontal_enabled` |
| "Multiline editing with word wrap" (line 52) | Vertical scrolling with content height |
| `TextEditorState.scroll_x, scroll_y` (line 79) | Should use `HasScrollView` component |

**Integration approach:**

```cpp
// Instead of custom scroll state in TextEditorState:
struct TextEditorState {
  // float scroll_x = 0, scroll_y = 0;  // REMOVE - redundant
  // ...
};

// Text editor entity has BOTH components:
entity.addComponent<TextEditorState>(...);
entity.addComponent<HasScrollView>({
  .horizontal_enabled = !word_wrap,
  .vertical_enabled = true,
});

// Layout system calculates content size from text layout:
auto& scroll = entity.get<HasScrollView>();
scroll.content_size = {
  text_layout.max_line_width,
  text_layout.total_height
};

// Cursor-following (scroll to keep cursor visible):
scroll.scroll_to_visible(cursor_rect);
```

**03 should depend on 08** - implement scroll containers first, then text editor uses them.

### 🔗 INTEGRATION: 01_test_input_hooks.md

**Severity:** Required for E2E testing of scroll

The test input system in 01 does NOT mention mouse wheel:

```cpp
// From 01_test_input_hooks.md - current API proposal (lines 127-144)
void push_key(int keycode);
void push_char(char32_t c);
void set_mouse_position(float x, float y);
void press_mouse_button(int button);
void hold_mouse_button(int button);
void release_mouse_button(int button);
void scroll_wheel(float delta);  // <-- NEEDS TO BE ADDED
```

**Update 01 to include:**

```cpp
namespace afterhours::ui::test {

// Add to API (line ~134):
void scroll_wheel(float delta);        // Vertical wheel
void scroll_wheel_v(float dx, float dy);  // 2D trackpad

// Internal state addition:
struct MouseState {
  // ...existing...
  float wheel_delta = 0.f;
  Vector2 wheel_delta_v = {0.f, 0.f};
  int frames_until_clear_wheel = 0;  // Same timing issue as clicks
};

}
```

**The `InputProvider` interface from 01 should include:**

```cpp
struct InputProvider {
  // ...existing...
  virtual float get_mouse_wheel_move() = 0;
  virtual Vector2 get_mouse_wheel_move_v() = 0;
};
```

### 🔗 INTEGRATION: 06_action_binding_system.md

**Severity:** Optional enhancement

The action binding system could support scroll-related actions:

```cpp
// Extend ValidInputAction concept for scroll support
template <typename T>
concept ScrollableInputAction = ValidInputAction<T> && requires {
  { T::ScrollUp } -> std::convertible_to<T>;
  { T::ScrollDown } -> std::convertible_to<T>;
  { T::ScrollPageUp } -> std::convertible_to<T>;
  { T::ScrollPageDown } -> std::convertible_to<T>;
  { T::ScrollToTop } -> std::convertible_to<T>;
  { T::ScrollToBottom } -> std::convertible_to<T>;
};
```

Default bindings:
| Action | Default Binding |
|--------|-----------------|
| ScrollUp | Up Arrow (when scroll container focused) |
| ScrollDown | Down Arrow |
| ScrollPageUp | Page Up |
| ScrollPageDown | Page Down |
| ScrollToTop | Ctrl+Home |
| ScrollToBottom | Ctrl+End |

This allows users to rebind scroll navigation keys.

### 🔗 INTEGRATION: 09_modal_dialogs.md

**Severity:** Minor enhancement

Modals may contain scrollable content:

```cpp
// Example: EULA dialog with scrollable text
if (begin_modal(ctx, dialog_id, "License Agreement", {.size = {500, 400}})) {
  
  // Scrollable content area inside modal
  if (begin_scroll_view(ctx, scroll_id, {.height = 300})) {
    text_block(ctx, license_text);  // Long text
    end_scroll_view();
  }
  
  // Buttons outside scroll area
  if (button(ctx, ok_id, "I Agree").clicked) {
    close_modal(ctx, dialog_id, DialogResult::Confirmed);
  }
  end_modal();
}
```

No blocking dependency - just note that modals should support scroll containers as children.

### 🔗 INTEGRATION: 12_e2e_testing_framework.md

**Severity:** Required for scroll testing

The E2E DSL needs scroll commands:

```
# Add to DSL (line ~76):
| `scroll delta` | Scroll wheel | `scroll -3` |
| `scroll_to x y` | Scroll to position | `scroll_to 0 500` |
| `drag_scroll x1 y1 x2 y2` | Scrollbar drag | `drag_scroll 490 100 490 300` |
```

**Implementation in e2e_script.h:**

```cpp
void executeCommand(const Command& cmd) {
  // ...existing cases...
  
  case CommandType::Scroll: {
    float delta = std::stof(cmd.args[0]);
    test_input::scroll_wheel(delta);
    break;
  }
  
  case CommandType::ScrollTo: {
    float x = std::stof(cmd.args[0]);
    float y = std::stof(cmd.args[1]);
    // Find focused scroll container, set scroll position
    auto* scroll = get_focused_scroll_view();
    if (scroll) scroll->scroll_to(x, y);
    break;
  }
}
```

**Example test script:**
```
# Test: Scroll document
type "Line 1\n"
type "Line 2\n"
# ... repeat for many lines ...
wait 5
scroll -10
expect_text "Line 50"
screenshot scrolled_view
scroll 10
expect_text "Line 1"
```

## Implementation Order

Based on the dependency analysis above, the recommended implementation order is:

```
┌─────────────────────────────────────────────────────────────────┐
│ Phase 1: Foundation                                              │
│                                                                  │
│  05_render_system_const_constraint ◄── MUST BE FIRST            │
│       │                                                          │
│       ▼                                                          │
│  07_renderer_abstraction (scissor/matrix methods)                │
│       │                                                          │
│       ▼                                                          │
│  01_test_input_hooks (add scroll_wheel)                          │
└─────────────────────────────────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│ Phase 2: Core Scroll                                             │
│                                                                  │
│  08_scrollable_containers (this document)                        │
│       │                                                          │
│       ├──► HasScrollView component                               │
│       ├──► HandleScrollInput system                              │
│       ├──► Scroll-aware rendering                                │
│       └──► Scrollbar rendering                                   │
└─────────────────────────────────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│ Phase 3: Consumers                                               │
│                                                                  │
│  03_text_editing_widget (uses HasScrollView)                     │
│  09_modal_dialogs (scroll containers in modals)                  │
│  12_e2e_testing_framework (scroll commands)                      │
│  06_action_binding_system (scroll actions - optional)            │
└─────────────────────────────────────────────────────────────────┘
```

## Related Gaps (Quick Reference)

| Gap | Relationship | Notes |
|-----|--------------|-------|
| **05** | ⚠️ BLOCKER | Must resolve const constraint first |
| **07** | 🔗 Coordinate | Align scissor API with graphics abstraction |
| **01** | 🔗 Extend | Add scroll wheel to test input |
| **03** | 🔗 Consumer | Text editor uses scroll |
| **06** | 🔗 Optional | Scroll action bindings |
| **09** | 🔗 Minor | Scrollable modal content |
| **12** | 🔗 Extend | Add scroll to E2E DSL |

## Use Cases
- Document editing area with long content
- File browser / list views
- Settings panels with many options
- Log viewers / console output
- Dropdown menus with many items
- Any content that can exceed viewport

## Notes
- Mouse wheel handling should respect which UI element is hovered (innermost scrollable wins)
- Touch/drag scrolling would be nice for mobile/touch screens
- Inertial/momentum scrolling could be a nice-to-have
- Keyboard navigation (Page Up/Down, arrow keys) should work when focused
- Nested scroll containers need careful handling (event bubbling)
- Scrollbar dragging requires `HandleDrags`-style interaction

