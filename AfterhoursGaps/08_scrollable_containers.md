# Scrollable Containers

## Problem
Afterhours UI does not provide scrollable containers or scroll view components. There is 
no mouse wheel input handling for UI scrolling.

## Current Workaround
Custom `ScrollComponent` in `src/ecs/components.h`:

```cpp
struct ScrollComponent : public afterhours::BaseComponent {
    int offset = 0;         // Scroll offset in lines
    int visibleLines = 20;  // Number of visible lines
    int maxScroll = 0;      // Maximum scroll value
};
```

Manual scroll handling in `input_system.h` and rendering clipping in `render_system.h`.

## What Afterhours Currently Provides
- No scrollable container component
- No mouse wheel input detection (`GetMouseWheelMove()` not wrapped)
- No content clipping for overflow handling
- Autolayout does not support scroll overflow

## Required Feature: Scroll View API

### Proposed API for `input_system.h`

```cpp
namespace afterhours::input {

// Mouse wheel input
static float get_mouse_wheel_move();
static Vector2 get_mouse_wheel_move_v();  // For 2D scroll (trackpads)

} // namespace afterhours::input
```

### Proposed Component for `components.h`

```cpp
namespace afterhours::ui {

// Scroll state component
struct HasScrollView : BaseComponent {
  Vector2 scroll_offset = {0, 0};   // Current scroll position
  Vector2 content_size = {0, 0};    // Size of scrollable content
  Vector2 viewport_size = {0, 0};   // Size of visible area
  
  bool horizontal_enabled = false;  // Allow horizontal scrolling
  bool vertical_enabled = true;     // Allow vertical scrolling
  
  float scroll_speed = 20.0f;       // Pixels per scroll tick
  
  // Computed properties
  float max_scroll_x() const { 
    return std::max(0.f, content_size.x - viewport_size.x); 
  }
  float max_scroll_y() const { 
    return std::max(0.f, content_size.y - viewport_size.y); 
  }
  
  bool can_scroll_up() const { return scroll_offset.y > 0; }
  bool can_scroll_down() const { return scroll_offset.y < max_scroll_y(); }
  bool can_scroll_left() const { return scroll_offset.x > 0; }
  bool can_scroll_right() const { return scroll_offset.x < max_scroll_x(); }
};

// Optional: Scrollbar styling
struct ScrollbarStyle {
  float width = 12.0f;
  Color track_color = {200, 200, 200, 255};
  Color thumb_color = {128, 128, 128, 255};
  Color thumb_hover_color = {100, 100, 100, 255};
  bool auto_hide = true;
};

} // namespace afterhours::ui
```

### Proposed Immediate Mode API

```cpp
namespace afterhours::ui::imm {

// Create a scrollable region
template <typename InputAction>
ScrollResult scroll_view(UIContext<InputAction>& ctx, 
                         EntityID id,
                         Vector2 viewport_size) {
  // Returns info about scroll state
  // Content inside is positioned relative to scroll offset
}

// End the scrollable region
void end_scroll_view();

// Or builder pattern:
auto scroll = scroll_view(ctx, id)
    .with_size({400, 300})
    .with_horizontal(false)
    .with_vertical(true)
    .with_scrollbar_style(ScrollbarStyle{...});

} // namespace afterhours::ui::imm
```

### Proposed System for Scroll Input

```cpp
namespace afterhours::ui {

template <typename InputAction>
struct ScrollInputSystem : System<UIContext<InputAction>, HasScrollView> {
  void for_each_with(Entity& entity, UIContext<InputAction>& ctx, 
                     HasScrollView& scroll, float dt) override {
    // Check if mouse is over this scroll view
    if (!is_mouse_over(entity)) return;
    
    // Handle mouse wheel
    float wheel = input::get_mouse_wheel_move();
    if (wheel != 0 && scroll.vertical_enabled) {
      scroll.scroll_offset.y -= wheel * scroll.scroll_speed;
      scroll.scroll_offset.y = std::clamp(scroll.scroll_offset.y, 
                                          0.f, scroll.max_scroll_y());
    }
    
    // Handle horizontal (shift + wheel or trackpad)
    // Handle scrollbar dragging
    // Handle keyboard (Page Up/Down, arrow keys when focused)
  }
};

} // namespace afterhours::ui
```

### Rendering with Content Clipping

The rendering system needs to:
1. Enable scissor/clip rectangle for the viewport
2. Offset child rendering by `-scroll_offset`
3. Disable scissor after rendering children

```cpp
// In rendering.h
if (entity.has<HasScrollView>()) {
  const auto& scroll = entity.get<HasScrollView>();
  
  // Enable clipping
  raylib::BeginScissorMode(rect.x, rect.y, rect.width, rect.height);
  
  // Translate content
  raylib::rlPushMatrix();
  raylib::rlTranslatef(-scroll.scroll_offset.x, -scroll.scroll_offset.y, 0);
  
  // Render children...
  
  raylib::rlPopMatrix();
  raylib::EndScissorMode();
  
  // Render scrollbars on top
  if (show_scrollbar) {
    render_scrollbar(rect, scroll);
  }
}
```

## Use Cases
- Document editing area with long content
- File browser / list views
- Settings panels with many options
- Log viewers / console output
- Any content that can exceed viewport

## Notes
- Mouse wheel handling should respect which UI element is hovered
- Touch/drag scrolling would be nice for mobile/touch screens
- Inertial/momentum scrolling could be a nice-to-have
- Keyboard navigation (Page Up/Down) should work when focused

