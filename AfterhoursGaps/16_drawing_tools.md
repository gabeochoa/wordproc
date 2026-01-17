# Drawing Tools and Shape Editing

## Context
Wordproc uses `src/editor/drawing.h` to model document drawings and shapes:
- Shape types (line, rectangle, ellipse, arrow, rounded rect, triangle, freeform)
- Stroke styles (solid/dashed/dotted/dash-dot), arrowheads, fill colors
- Layout modes (inline/float/behind/in-front)
- Selection/hit-testing via bounding boxes
- Rotation, resize, and anchor positioning

## Problem
Afterhours UI provides immediate-mode layout and rendering primitives, but does not
include a drawing/shape editing layer (selection handles, resizing, rotation, z-order,
freeform capture, hit testing). Implementing these at the app layer is possible but
duplicative across apps (diagrams, whiteboards, word processors, flow tools).

## Relationship to Renderer Abstraction (07_renderer_abstraction.md)

The drawing tools layer sits **above** the `GraphicsBackend` abstraction defined in
**07_renderer_abstraction.md**. The architecture is:

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                         │
│  (wordproc drawing tools, diagram editors, whiteboards)      │
└───────────────────────────┬─────────────────────────────────┘
                            │ uses
                            ▼
┌─────────────────────────────────────────────────────────────┐
│              afterhours::ui::draw / ui::edit                 │
│       (shape primitives, selection, hit-testing, etc.)       │
│            ← THIS DOCUMENT: Drawing Tools Layer              │
└───────────────────────────┬─────────────────────────────────┘
                            │ uses
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                afterhours::graphics.h                        │
│  (DrawRectangle, DrawLine, DrawCircle, etc. free functions)  │
│            ← 07_renderer_abstraction.md                      │
└───────────────────────────┬─────────────────────────────────┘
                            │ delegates to
                            ▼
┌─────────────────────────────────────────────────────────────┐
│              GraphicsBackend (concept/interface)             │
│    RaylibBackend │ SokolBackend │ HeadlessBackend │ etc.    │
└─────────────────────────────────────────────────────────────┘
```

### Missing GraphicsBackend Primitives

The current `GraphicsBackend` concept (see 07_renderer_abstraction.md) is missing
primitives needed by the drawing layer. These should be added to the backend:

```cpp
template<typename T>
concept GraphicsBackend = requires(T t, /* existing params... */
    Vector2 center, float radius_x, float radius_y,
    float start_angle, float end_angle,
    const std::vector<Vector2>& points) 
{
  // ... existing requirements from 07_renderer_abstraction.md ...
  
  // === NEW: Ellipse/Circle ===
  { t.draw_ellipse(center, radius_x, radius_y, color) } -> std::same_as<void>;
  { t.draw_ellipse_lines(center, radius_x, radius_y, thickness, color) } -> std::same_as<void>;
  
  // === NEW: Rounded Rectangle ===
  { t.draw_rounded_rect(rect, corner_radius, color) } -> std::same_as<void>;
  { t.draw_rounded_rect_lines(rect, corner_radius, thickness, color) } -> std::same_as<void>;
  
  // === NEW: Triangle ===
  { t.draw_triangle(v1, v2, v3, color) } -> std::same_as<void>;
  { t.draw_triangle_lines(v1, v2, v3, color) } -> std::same_as<void>;
  
  // === NEW: Polyline / Polygon ===
  { t.draw_line_strip(points, color) } -> std::same_as<void>;
  { t.draw_polygon(points, color) } -> std::same_as<void>;
  { t.draw_polygon_lines(points, thickness, color) } -> std::same_as<void>;
  
  // === NEW: Thick line with end caps ===
  { t.draw_line_ex(v1, v2, thickness, color) } -> std::same_as<void>;
  
  // === NEW: Arc (for arrowhead curves, pie charts) ===
  { t.draw_arc(center, radius, start_angle, end_angle, segments, color) } -> std::same_as<void>;
  { t.draw_arc_lines(center, radius, start_angle, end_angle, segments, thickness, color) } -> std::same_as<void>;
};
```

**Corresponding `graphics.h` free functions to add:**

```cpp
// In afterhours/src/graphics.h - add these alongside existing functions

// Ellipse
inline void DrawEllipse(Vector2 center, float rx, float ry, Color c) {
  detail::g_backend.draw_ellipse(center, rx, ry, c);
}
inline void DrawEllipseLines(Vector2 center, float rx, float ry, float t, Color c) {
  detail::g_backend.draw_ellipse_lines(center, rx, ry, t, c);
}

// Rounded Rectangle
inline void DrawRoundedRect(Rectangle r, float radius, Color c) {
  detail::g_backend.draw_rounded_rect(r, radius, c);
}
inline void DrawRoundedRectLines(Rectangle r, float radius, float t, Color c) {
  detail::g_backend.draw_rounded_rect_lines(r, radius, t, c);
}

// Triangle
inline void DrawTriangle(Vector2 v1, Vector2 v2, Vector2 v3, Color c) {
  detail::g_backend.draw_triangle(v1, v2, v3, c);
}
inline void DrawTriangleLines(Vector2 v1, Vector2 v2, Vector2 v3, Color c) {
  detail::g_backend.draw_triangle_lines(v1, v2, v3, c);
}

// Polyline / Polygon
inline void DrawLineStrip(const std::vector<Vector2>& pts, Color c) {
  detail::g_backend.draw_line_strip(pts, c);
}
inline void DrawPolygon(const std::vector<Vector2>& pts, Color c) {
  detail::g_backend.draw_polygon(pts, c);
}

// Thick line
inline void DrawLineEx(Vector2 a, Vector2 b, float thickness, Color c) {
  detail::g_backend.draw_line_ex(a, b, thickness, c);
}

// Arc
inline void DrawArc(Vector2 center, float r, float start, float end, int segs, Color c) {
  detail::g_backend.draw_arc(center, r, start, end, segs, c);
}
```

### Raylib Backend Implementation for New Primitives

```cpp
// In backends/raylib_backend.h - add to RaylibBackend struct

void draw_ellipse(Vector2 center, float rx, float ry, Color c) {
  raylib::DrawEllipse(center.x, center.y, rx, ry, to_raylib(c));
}

void draw_ellipse_lines(Vector2 center, float rx, float ry, float /*thickness*/, Color c) {
  // Note: raylib's DrawEllipseLines doesn't support thickness natively
  // Could implement with line segments for thick outlines
  raylib::DrawEllipseLines(center.x, center.y, rx, ry, to_raylib(c));
}

void draw_rounded_rect(Rectangle r, float radius, Color c) {
  raylib::DrawRectangleRounded({r.x, r.y, r.width, r.height}, 
                                radius / std::min(r.width, r.height), 
                                8, to_raylib(c));
}

void draw_rounded_rect_lines(Rectangle r, float radius, float thickness, Color c) {
  raylib::DrawRectangleRoundedLines({r.x, r.y, r.width, r.height},
                                     radius / std::min(r.width, r.height),
                                     8, thickness, to_raylib(c));
}

void draw_triangle(Vector2 v1, Vector2 v2, Vector2 v3, Color c) {
  raylib::DrawTriangle({v1.x, v1.y}, {v2.x, v2.y}, {v3.x, v3.y}, to_raylib(c));
}

void draw_triangle_lines(Vector2 v1, Vector2 v2, Vector2 v3, Color c) {
  raylib::DrawTriangleLines({v1.x, v1.y}, {v2.x, v2.y}, {v3.x, v3.y}, to_raylib(c));
}

void draw_line_strip(const std::vector<Vector2>& points, Color c) {
  if (points.size() < 2) return;
  for (size_t i = 0; i < points.size() - 1; ++i) {
    raylib::DrawLine(points[i].x, points[i].y, 
                     points[i+1].x, points[i+1].y, to_raylib(c));
  }
}

void draw_line_ex(Vector2 a, Vector2 b, float thickness, Color c) {
  raylib::DrawLineEx({a.x, a.y}, {b.x, b.y}, thickness, to_raylib(c));
}
```

## Requested Afterhours Features

### 1) Shape rendering primitives
Provide reusable draw helpers for common shapes. These use the `graphics.h` free
functions internally but add higher-level features like line styles:

```cpp
namespace afterhours::ui::draw {
  // Line style enum (handled at this layer, not in backend)
  enum class LineStyle { Solid, Dashed, Dotted, DashDot };
  enum class ArrowStyle { None, Standard, Open, Diamond, Circle };

  // Core shape drawing - delegates to graphics.h with style processing
  void line(Vector2 a, Vector2 b, Color color, float thickness, LineStyle style);
  void rect(Rect r, Color stroke, float thickness, LineStyle style);
  void rect_filled(Rect r, Color fill);
  void rounded_rect(Rect r, float radius, Color stroke, float thickness, LineStyle style);
  void rounded_rect_filled(Rect r, float radius, Color fill);
  void ellipse(Rect bounds, Color stroke, float thickness, LineStyle style);
  void ellipse_filled(Rect bounds, Color fill);
  void triangle(Vector2 a, Vector2 b, Vector2 c, Color stroke, float thickness, LineStyle style);
  void triangle_filled(Vector2 a, Vector2 b, Vector2 c, Color fill);
  void arrow(Vector2 a, Vector2 b, Color stroke, float thickness,
             ArrowStyle start, ArrowStyle end);
}
```

**Implementation note:** Line styles (dashed/dotted/dash-dot) are implemented at the
`ui::draw` layer by breaking lines into segments, NOT at the `GraphicsBackend` level.
This keeps backends simple while providing rich styling at the higher layer:

```cpp
// Implementation in afterhours/src/ui/draw.h (header-only)
namespace afterhours::ui::draw {

inline void line(Vector2 a, Vector2 b, Color color, float thickness, LineStyle style) {
  if (style == LineStyle::Solid) {
    // Delegate directly to backend via graphics.h
    afterhours::DrawLineEx(a, b, thickness, color);
    return;
  }
  
  // For styled lines, compute segments
  float dx = b.x - a.x, dy = b.y - a.y;
  float length = std::sqrt(dx*dx + dy*dy);
  if (length < 0.001f) return;
  
  Vector2 dir = {dx / length, dy / length};
  float dash_len = 8.0f, gap_len = 4.0f, dot_len = 2.0f;
  
  switch (style) {
    case LineStyle::Dashed: {
      float t = 0;
      while (t < length) {
        float end_t = std::min(t + dash_len, length);
        afterhours::DrawLineEx(
          {a.x + dir.x * t, a.y + dir.y * t},
          {a.x + dir.x * end_t, a.y + dir.y * end_t},
          thickness, color);
        t = end_t + gap_len;
      }
      break;
    }
    case LineStyle::Dotted: {
      float t = 0;
      while (t < length) {
        afterhours::DrawCircle({a.x + dir.x * t, a.y + dir.y * t}, 
                               thickness * 0.5f, color);
        t += gap_len;
      }
      break;
    }
    case LineStyle::DashDot: {
      float t = 0;
      bool do_dash = true;
      while (t < length) {
        if (do_dash) {
          float end_t = std::min(t + dash_len, length);
          afterhours::DrawLineEx(
            {a.x + dir.x * t, a.y + dir.y * t},
            {a.x + dir.x * end_t, a.y + dir.y * end_t},
            thickness, color);
          t = end_t + gap_len;
        } else {
          afterhours::DrawCircle({a.x + dir.x * t, a.y + dir.y * t},
                                 thickness * 0.5f, color);
          t += gap_len;
        }
        do_dash = !do_dash;
      }
      break;
    }
    default: break;
  }
}

inline void arrow(Vector2 a, Vector2 b, Color color, float thickness,
                  ArrowStyle start, ArrowStyle end) {
  // Draw the main line
  afterhours::DrawLineEx(a, b, thickness, color);
  
  // Calculate arrow direction
  float dx = b.x - a.x, dy = b.y - a.y;
  float len = std::sqrt(dx*dx + dy*dy);
  if (len < 0.001f) return;
  Vector2 dir = {dx / len, dy / len};
  Vector2 perp = {-dir.y, dir.x};
  
  float head_size = thickness * 4.0f;
  
  // Draw end arrowhead
  if (end == ArrowStyle::Standard) {
    Vector2 tip = b;
    Vector2 left = {tip.x - dir.x * head_size + perp.x * head_size * 0.5f,
                    tip.y - dir.y * head_size + perp.y * head_size * 0.5f};
    Vector2 right = {tip.x - dir.x * head_size - perp.x * head_size * 0.5f,
                     tip.y - dir.y * head_size - perp.y * head_size * 0.5f};
    afterhours::DrawTriangle(tip, left, right, color);
  }
  // ... handle other arrow styles ...
}

} // namespace afterhours::ui::draw
```

### 2) Shape editing / interaction helpers
Generic utilities for selection and editing:
- Hit-testing for shapes (line distance, ellipse bounds, polygon)
- Selection handles (8-way + rotation handle)
- Drag/resize/rotate helpers returning updated geometry
- Snapping to grid/angles (0/15/45/90 degrees)

These are **pure logic utilities** that don't depend on the graphics backend for
computation, but use `graphics.h` for visual feedback (handles, guides):

```cpp
namespace afterhours::ui::edit {

  // === Handle identifiers ===
  enum class Handle : int {
    None = -1,
    TopLeft = 0, Top, TopRight,
    Left, Right,
    BottomLeft, Bottom, BottomRight,
    Rotate  // Handle above top-center for rotation
  };

  // === Edit state for a single shape ===
  struct EditState {
    bool selected = false;
    bool dragging = false;
    bool resizing = false;
    bool rotating = false;
    Handle active_handle = Handle::None;
    Vector2 drag_start{};      // Mouse position when drag started
    Rect original_bounds{};    // Bounds when edit started
    float original_rotation{}; // Rotation when edit started
  };

  // === Hit testing (pure math, no rendering) ===
  inline bool hit_test_rect(Rect r, Vector2 p) {
    return p.x >= r.x && p.x <= r.x + r.width &&
           p.y >= r.y && p.y <= r.y + r.height;
  }

  inline bool hit_test_line(Vector2 a, Vector2 b, Vector2 p, float tolerance) {
    float dx = b.x - a.x, dy = b.y - a.y;
    float len_sq = dx*dx + dy*dy;
    if (len_sq < 0.0001f) return false;
    float t = std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / len_sq, 0.f, 1.f);
    float closest_x = a.x + t * dx, closest_y = a.y + t * dy;
    float dist_sq = (p.x - closest_x) * (p.x - closest_x) + 
                    (p.y - closest_y) * (p.y - closest_y);
    return dist_sq <= tolerance * tolerance;
  }

  inline bool hit_test_ellipse(Rect bounds, Vector2 p) {
    float cx = bounds.x + bounds.width * 0.5f;
    float cy = bounds.y + bounds.height * 0.5f;
    float rx = bounds.width * 0.5f, ry = bounds.height * 0.5f;
    if (rx < 0.0001f || ry < 0.0001f) return false;
    float nx = (p.x - cx) / rx, ny = (p.y - cy) / ry;
    return (nx * nx + ny * ny) <= 1.0f;
  }

  inline bool hit_test_triangle(Vector2 v1, Vector2 v2, Vector2 v3, Vector2 p) {
    auto sign = [](Vector2 a, Vector2 b, Vector2 c) {
      return (a.x - c.x) * (b.y - c.y) - (b.x - c.x) * (a.y - c.y);
    };
    float d1 = sign(p, v1, v2), d2 = sign(p, v2, v3), d3 = sign(p, v3, v1);
    bool has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    bool has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
    return !(has_neg && has_pos);
  }

  // === Handle positions (pure math) ===
  inline std::array<Vector2, 9> get_handle_positions(Rect bounds, float handle_offset = 20.f) {
    float cx = bounds.x + bounds.width * 0.5f;
    return {{
      {bounds.x, bounds.y},                                    // TopLeft
      {cx, bounds.y},                                          // Top
      {bounds.x + bounds.width, bounds.y},                     // TopRight
      {bounds.x, bounds.y + bounds.height * 0.5f},             // Left
      {bounds.x + bounds.width, bounds.y + bounds.height * 0.5f}, // Right
      {bounds.x, bounds.y + bounds.height},                    // BottomLeft
      {cx, bounds.y + bounds.height},                          // Bottom
      {bounds.x + bounds.width, bounds.y + bounds.height},     // BottomRight
      {cx, bounds.y - handle_offset}                           // Rotate handle
    }};
  }

  inline Handle hit_test_handles(Rect bounds, Vector2 p, float handle_size = 8.f) {
    auto positions = get_handle_positions(bounds);
    for (int i = 0; i < 9; ++i) {
      Rect handle_rect = {positions[i].x - handle_size * 0.5f,
                          positions[i].y - handle_size * 0.5f,
                          handle_size, handle_size};
      if (hit_test_rect(handle_rect, p)) {
        return static_cast<Handle>(i);
      }
    }
    return Handle::None;
  }

  // === Drawing selection handles (uses graphics.h) ===
  inline void draw_selection_handles(Rect bounds, Color handle_color, 
                                     Color line_color, float handle_size = 8.f) {
    // Draw bounding box with dashed line
    afterhours::ui::draw::rect(bounds, line_color, 1.0f, 
                                afterhours::ui::draw::LineStyle::Dashed);
    
    // Draw handles as filled squares
    auto positions = get_handle_positions(bounds);
    for (int i = 0; i < 9; ++i) {
      Rect hr = {positions[i].x - handle_size * 0.5f,
                 positions[i].y - handle_size * 0.5f,
                 handle_size, handle_size};
      afterhours::DrawRectangle(hr, handle_color);
      afterhours::DrawRectangleLines(hr, 1.0f, line_color);
    }
    
    // Draw line from top-center to rotation handle
    afterhours::DrawLine(positions[1], positions[8], line_color);
  }

  // === Drag/resize/rotate updates (pure math, returns new geometry) ===
  inline bool update_drag(EditState& state, Vector2 mouse, Rect& bounds) {
    if (!state.dragging) return false;
    float dx = mouse.x - state.drag_start.x;
    float dy = mouse.y - state.drag_start.y;
    bounds.x = state.original_bounds.x + dx;
    bounds.y = state.original_bounds.y + dy;
    return true;
  }

  inline bool update_resize(EditState& state, Vector2 mouse, Rect& bounds,
                            bool maintain_aspect = false) {
    if (!state.resizing) return false;
    float dx = mouse.x - state.drag_start.x;
    float dy = mouse.y - state.drag_start.y;
    
    // Resize based on which handle is active
    switch (state.active_handle) {
      case Handle::TopLeft:
        bounds.x = state.original_bounds.x + dx;
        bounds.y = state.original_bounds.y + dy;
        bounds.width = state.original_bounds.width - dx;
        bounds.height = state.original_bounds.height - dy;
        break;
      case Handle::BottomRight:
        bounds.width = state.original_bounds.width + dx;
        bounds.height = state.original_bounds.height + dy;
        break;
      // ... other handles ...
      default: break;
    }
    
    // Clamp minimum size
    bounds.width = std::max(bounds.width, 10.f);
    bounds.height = std::max(bounds.height, 10.f);
    return true;
  }

  inline bool update_rotate(EditState& state, Vector2 mouse, 
                            Vector2 center, float& degrees) {
    if (!state.rotating) return false;
    float dx = mouse.x - center.x, dy = mouse.y - center.y;
    degrees = std::atan2(dy, dx) * (180.f / 3.14159f) + 90.f;
    return true;
  }

  // === Snapping utilities ===
  inline float snap_angle(float degrees, float snap_increment = 15.f) {
    return std::round(degrees / snap_increment) * snap_increment;
  }

  inline Vector2 snap_to_grid(Vector2 p, float grid_size) {
    return {std::round(p.x / grid_size) * grid_size,
            std::round(p.y / grid_size) * grid_size};
  }
}
```

### 3) Freeform drawing capture
Provide a "capture stroke" helper that collects points with smoothing/decimation.
This combines input handling with `graphics.h` for real-time visual feedback:

```cpp
namespace afterhours::ui::draw {

  struct Stroke {
    std::vector<Vector2> points;
    Color color = {0, 0, 0, 255};
    float thickness = 2.0f;
    
    void clear() { points.clear(); }
    bool empty() const { return points.empty(); }
  };

  // Capture state - tracks ongoing stroke capture
  struct StrokeCaptureState {
    bool capturing = false;
    Stroke current_stroke;
    float min_distance_sq = 4.0f;  // Minimum distance² between points
  };

  // Process input during capture - call each frame while drawing
  // Returns true if stroke is complete (mouse released)
  inline bool capture_stroke_update(StrokeCaptureState& state, 
                                    Vector2 mouse_pos, bool mouse_down) {
    if (mouse_down && !state.capturing) {
      // Start new stroke
      state.capturing = true;
      state.current_stroke.clear();
      state.current_stroke.points.push_back(mouse_pos);
      return false;
    }
    
    if (mouse_down && state.capturing) {
      // Add point if far enough from last point
      if (!state.current_stroke.points.empty()) {
        Vector2 last = state.current_stroke.points.back();
        float dx = mouse_pos.x - last.x, dy = mouse_pos.y - last.y;
        if (dx*dx + dy*dy >= state.min_distance_sq) {
          state.current_stroke.points.push_back(mouse_pos);
        }
      }
      return false;
    }
    
    if (!mouse_down && state.capturing) {
      // Stroke complete
      state.capturing = false;
      return true;
    }
    
    return false;
  }

  // Draw stroke in progress (uses graphics.h)
  inline void draw_stroke(const Stroke& stroke) {
    if (stroke.points.size() < 2) return;
    // Use DrawLineStrip from graphics.h (which we proposed adding above)
    for (size_t i = 0; i < stroke.points.size() - 1; ++i) {
      afterhours::DrawLineEx(stroke.points[i], stroke.points[i+1],
                             stroke.thickness, stroke.color);
    }
  }

  // Smooth/decimate stroke using Ramer-Douglas-Peucker algorithm
  inline void simplify_stroke(Stroke& stroke, float epsilon = 2.0f) {
    // Implementation of RDP line simplification
    // Reduces point count while preserving shape
    if (stroke.points.size() < 3) return;
    
    std::function<void(size_t, size_t, std::vector<bool>&)> rdp;
    std::vector<bool> keep(stroke.points.size(), false);
    keep[0] = keep[stroke.points.size() - 1] = true;
    
    rdp = [&](size_t start, size_t end, std::vector<bool>& marks) {
      if (end <= start + 1) return;
      
      float max_dist = 0;
      size_t max_idx = start;
      
      Vector2 a = stroke.points[start], b = stroke.points[end];
      float dx = b.x - a.x, dy = b.y - a.y;
      float len_sq = dx*dx + dy*dy;
      
      for (size_t i = start + 1; i < end; ++i) {
        Vector2 p = stroke.points[i];
        float dist;
        if (len_sq < 0.0001f) {
          dist = std::sqrt((p.x - a.x)*(p.x - a.x) + (p.y - a.y)*(p.y - a.y));
        } else {
          float t = std::clamp(((p.x - a.x)*dx + (p.y - a.y)*dy) / len_sq, 0.f, 1.f);
          float proj_x = a.x + t * dx, proj_y = a.y + t * dy;
          dist = std::sqrt((p.x - proj_x)*(p.x - proj_x) + (p.y - proj_y)*(p.y - proj_y));
        }
        if (dist > max_dist) { max_dist = dist; max_idx = i; }
      }
      
      if (max_dist > epsilon) {
        marks[max_idx] = true;
        rdp(start, max_idx, marks);
        rdp(max_idx, end, marks);
      }
    };
    
    rdp(0, stroke.points.size() - 1, keep);
    
    std::vector<Vector2> simplified;
    for (size_t i = 0; i < stroke.points.size(); ++i) {
      if (keep[i]) simplified.push_back(stroke.points[i]);
    }
    stroke.points = std::move(simplified);
  }
}
```

### 4) Z-order and layering utilities
Helpers to manage stacking order for floating drawings. These are pure data
utilities—no rendering backend dependency:

```cpp
namespace afterhours::ui::layer {

  // Layer position constants
  constexpr int LAYER_BEHIND = -1000;     // Behind text
  constexpr int LAYER_DEFAULT = 0;        // Normal layer
  constexpr int LAYER_INFRONT = 1000;     // In front of text

  // Item with z-order
  template<typename T>
  struct LayeredItem {
    T data;
    int z_order = 0;
    size_t id = 0;
  };

  // Z-order operations on a vector of layered items
  template<typename T>
  void bring_to_front(std::vector<LayeredItem<T>>& items, size_t id) {
    int max_z = 0;
    for (const auto& item : items) max_z = std::max(max_z, item.z_order);
    for (auto& item : items) {
      if (item.id == id) { item.z_order = max_z + 1; break; }
    }
  }

  template<typename T>
  void send_to_back(std::vector<LayeredItem<T>>& items, size_t id) {
    int min_z = 0;
    for (const auto& item : items) min_z = std::min(min_z, item.z_order);
    for (auto& item : items) {
      if (item.id == id) { item.z_order = min_z - 1; break; }
    }
  }

  template<typename T>
  void bring_forward(std::vector<LayeredItem<T>>& items, size_t id) {
    for (auto& item : items) {
      if (item.id == id) { item.z_order += 1; break; }
    }
  }

  template<typename T>
  void send_backward(std::vector<LayeredItem<T>>& items, size_t id) {
    for (auto& item : items) {
      if (item.id == id) { item.z_order -= 1; break; }
    }
  }

  // Sort items by z-order for rendering
  template<typename T>
  void sort_by_z(std::vector<LayeredItem<T>>& items) {
    std::stable_sort(items.begin(), items.end(), 
                     [](const auto& a, const auto& b) { return a.z_order < b.z_order; });
  }

  // Hit-test in z-order (front to back, returns first hit)
  template<typename T, typename HitTest>
  LayeredItem<T>* hit_test_layered(std::vector<LayeredItem<T>>& items, 
                                    Vector2 point, HitTest&& test) {
    // Iterate in reverse (highest z first)
    for (auto it = items.rbegin(); it != items.rend(); ++it) {
      if (test(it->data, point)) return &(*it);
    }
    return nullptr;
  }
}
```

### 5) Text wrap helpers around shapes
Utility to compute exclusion regions for text flow. This is pure layout math,
no rendering involved:

```cpp
namespace afterhours::layout {

  struct Exclusion {
    Rect bounds;
    float margin = 4.0f;  // Extra space around shape
  };

  // Get exclusion rect with margins
  inline Rect exclusion_rect(const Exclusion& ex) {
    return {ex.bounds.x - ex.margin, ex.bounds.y - ex.margin,
            ex.bounds.width + ex.margin * 2, ex.bounds.height + ex.margin * 2};
  }

  // Compute exclusions from shapes for a line of text
  // Returns adjusted x-ranges that text can occupy
  struct TextRange { float x_start; float x_end; };

  inline std::vector<TextRange> available_ranges_for_line(
      float line_y, float line_height,
      float line_x_start, float line_x_end,
      const std::vector<Exclusion>& exclusions) {
    
    // Collect exclusions that intersect this line
    std::vector<std::pair<float, float>> blocked;
    for (const auto& ex : exclusions) {
      Rect r = exclusion_rect(ex);
      // Check vertical intersection
      if (line_y + line_height > r.y && line_y < r.y + r.height) {
        blocked.push_back({r.x, r.x + r.width});
      }
    }
    
    if (blocked.empty()) {
      return {{line_x_start, line_x_end}};
    }
    
    // Sort blocked ranges by start
    std::sort(blocked.begin(), blocked.end());
    
    // Merge overlapping blocked ranges
    std::vector<std::pair<float, float>> merged;
    for (const auto& b : blocked) {
      if (merged.empty() || merged.back().second < b.first) {
        merged.push_back(b);
      } else {
        merged.back().second = std::max(merged.back().second, b.second);
      }
    }
    
    // Compute available ranges (gaps between blocked)
    std::vector<TextRange> result;
    float x = line_x_start;
    for (const auto& m : merged) {
      if (x < m.first) {
        result.push_back({x, std::min(m.first, line_x_end)});
      }
      x = m.second;
    }
    if (x < line_x_end) {
      result.push_back({x, line_x_end});
    }
    
    return result;
  }
}
```

## App-Side Workarounds (Current)
- Wordproc implements shape data and rendering in app code (`src/editor/drawing.h` + 
  render system).
- Hit-testing uses simple bounding box checks in `DocumentDrawing::containsPoint()`.
- No selection handles, rotation, or styled line rendering yet.
- Drawings can be inserted via menu but rendering is not fully implemented.

## Integration Summary

| Layer | Namespace | Backend Dependency | Purpose |
|-------|-----------|-------------------|---------|
| Shape Drawing | `afterhours::ui::draw` | Uses `graphics.h` | Line styles, arrows, styled shapes |
| Shape Editing | `afterhours::ui::edit` | Uses `graphics.h` for handles | Hit-testing, resize, rotate |
| Stroke Capture | `afterhours::ui::draw` | Uses `graphics.h` for preview | Freeform drawing |
| Z-Order | `afterhours::ui::layer` | None (pure data) | Layer management |
| Text Flow | `afterhours::layout` | None (pure math) | Exclusion computation |

## Why This Belongs in Afterhours
- Reusable across multiple tools (diagram editors, whiteboards, report editors).
- Reduces duplicated logic for selection handles, shape rendering, and interaction.
- Aligns with Afterhours goals of reusable UI primitives.
- Builds naturally on top of the `GraphicsBackend` abstraction (07_renderer_abstraction.md).
- Header-only implementation keeps it consistent with Afterhours' ~90% header-only design.

## Related Gaps
- **07_renderer_abstraction.md** - Base graphics layer this builds upon
- **08_scrollable_containers.md** - Uses similar scissor/clipping for drawing canvas bounds

