# Extract Table Border Rendering Helper

**Date:** 2026-02-23
**Type:** Refactoring — reduce code duplication
**Impact:** ~100 lines removed, unblocks implementing missing border styles

## Problem

`renderTable` in `src/ecs/render_system.h` (lines 116–233) has 4 nearly identical switch statements for drawing cell borders (top, bottom, left, right). Each switch handles `BorderStyle::Thin`, `Medium`, `Thick` with the same pattern — only the start/end coordinates differ:

```cpp
switch (cell.borders.top) {
    case BorderStyle::Thin:
        afterhours::draw_line(cellX, cellY, cellX + cellW, cellY, borderColor);
        break;
    case BorderStyle::Medium:
        afterhours::draw_line_ex({cellX, cellY}, {cellX + cellW, cellY}, 2.0f, borderColor);
        break;
    case BorderStyle::Thick:
        afterhours::draw_line_ex({cellX, cellY}, {cellX + cellW, cellY}, 3.0f, borderColor);
        break;
    case BorderStyle::None:
    case BorderStyle::Double:
    case BorderStyle::Dashed:
    case BorderStyle::Dotted:
    default:
        break;
}
// ... repeated 3 more times for bottom, left, right
```

`Double`, `Dashed`, and `Dotted` styles are defined in the enum but not implemented because adding them to all 4 switches would further bloat the function.

## Proposed Solution

Extract a helper function:

```cpp
inline void drawBorderEdge(vec2 start, vec2 end, BorderStyle style, afterhours::Color color) {
    switch (style) {
        case BorderStyle::None:
            break;
        case BorderStyle::Thin:
            afterhours::draw_line(
                static_cast<int>(start.x), static_cast<int>(start.y),
                static_cast<int>(end.x), static_cast<int>(end.y), color);
            break;
        case BorderStyle::Medium:
            afterhours::draw_line_ex(start, end, 2.0f, color);
            break;
        case BorderStyle::Thick:
            afterhours::draw_line_ex(start, end, 3.0f, color);
            break;
        case BorderStyle::Double:
            // Two thin lines with 2px gap
            afterhours::draw_line_ex(start, end, 1.0f, color);
            // offset perpendicular by 3px...
            break;
        case BorderStyle::Dashed:
            // Draw dashed segments
            break;
        case BorderStyle::Dotted:
            // Draw dotted segments
            break;
    }
}
```

Then the 4 switches become 4 calls:

```cpp
drawBorderEdge({cellX, cellY}, {cellX + cellW, cellY}, cell.borders.top, borderColor);
drawBorderEdge({cellX, cellY + cellH}, {cellX + cellW, cellY + cellH}, cell.borders.bottom, borderColor);
drawBorderEdge({cellX, cellY}, {cellX, cellY + cellH}, cell.borders.left, borderColor);
drawBorderEdge({cellX + cellW, cellY}, {cellX + cellW, cellY + cellH}, cell.borders.right, borderColor);
```

## Files to Change

| File | Change |
|------|--------|
| `src/ecs/render_system.h` | Extract `drawBorderEdge`, replace 4 switches with 4 calls |

## Risks

- None significant. This is a pure mechanical extraction.

## Migration Strategy

1. Extract helper, replace switches
2. Optionally implement `Double`/`Dashed`/`Dotted` while the code is open
3. Verify table rendering E2E tests still pass
