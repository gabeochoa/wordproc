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

## Requested Afterhours Features

### 1) Shape rendering primitives
Provide reusable draw helpers for common shapes:
```cpp
namespace afterhours::ui::draw {
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

### 2) Shape editing / interaction helpers
Generic utilities for selection and editing:
- Hit-testing for shapes (line distance, ellipse bounds, polygon)
- Selection handles (8-way + rotation handle)
- Drag/resize/rotate helpers returning updated geometry
- Snapping to grid/angles (0/15/45/90 degrees)

Proposed API:
```cpp
namespace afterhours::ui::edit {
  struct EditState {
    bool selected = false;
    bool dragging = false;
    bool resizing = false;
    bool rotating = false;
    int handle = -1; // which handle is active
  };

  bool hit_test_rect(Rect r, Vector2 p);
  bool hit_test_line(Vector2 a, Vector2 b, Vector2 p, float tolerance);
  bool hit_test_ellipse(Rect bounds, Vector2 p);

  void draw_selection_handles(Rect bounds, Color handleColor);
  bool update_drag(EditState& state, Vector2 mouse, Rect& bounds);
  bool update_resize(EditState& state, Vector2 mouse, Rect& bounds);
  bool update_rotate(EditState& state, Vector2 mouse, Vector2 center, float& degrees);
}
```

### 3) Freeform drawing capture
Provide a "capture stroke" helper that collects points with smoothing/decimation:
```cpp
namespace afterhours::ui::draw {
  struct Stroke { std::vector<Vector2> points; };
  bool capture_stroke(Stroke& out, float minDistance, bool smooth);
}
```

### 4) Z-order and layering utilities
Helpers to manage stacking order for floating drawings:
- `bring_to_front`, `send_to_back`, `bring_forward`, `send_backward`
- Hit-test across layered items

### 5) Text wrap helpers around shapes
Utility to compute exclusion regions for text flow:
```cpp
namespace afterhours::layout {
  struct Exclusion { Rect bounds; };
  std::vector<Exclusion> exclusions_for_shapes(...);
}
```

## App-Side Workarounds (Current)
- Wordproc implements shape data and rendering in app code (`drawing.h` + render system).
- Hit-testing and editing behavior are app-specific and limited.

## Why This Belongs in Afterhours
- Reusable across multiple tools (diagram editors, whiteboards, report editors).
- Reduces duplicated logic for selection handles, shape rendering, and interaction.
- Aligns with Afterhours goals of reusable UI primitives.

