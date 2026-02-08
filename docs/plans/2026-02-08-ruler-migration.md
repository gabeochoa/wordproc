# Ruler Migration to Afterhours

## Current State

The horizontal ruler is drawn with raw raylib calls in `EditorRenderSystem::for_each_with` (lines 661-713 of `render_system.h`). It renders:

- Background rectangle with sunken border
- Inch marks: tall lines at each inch boundary with numeric labels (0, 1, 2, ...)
- Half-inch marks: medium lines at each half-inch
- Quarter-inch marks: short lines at each quarter-inch
- Calculated from `pixelsPerInch = 72 * scale`, starting at `rulerStartX = 50 * scale`

Total: ~53 lines of raw raylib draw code.

The ruler is positioned between the toolbar/formatting bar and the text area. It's hidden in focus mode.

## What Needs to Happen

1. **Create a `RulerSystem`** as an afterhours update system.
2. **Build the ruler container** as a `div` with the sunken border (using `BevelStyle::Sunken`).
3. **Draw tick marks inside**: This is the tricky part. Afterhours UI is flex-layout based — it doesn't have a "draw arbitrary lines at computed pixel positions" primitive. The tick marks need precise positioning at calculated intervals.

Two approaches:
- **A) Custom render callback**: Create the ruler container with afterhours for layout/positioning, but use a custom render system to draw the tick marks with raw raylib calls inside the container's bounds. This is hybrid — afterhours handles the container, raw code handles the ticks.
- **B) Absolute-positioned elements**: Create many small `div` elements for each tick mark, absolute-positioned at the computed X offset. This is "pure" afterhours but creates dozens of entities per frame for tick marks, which is wasteful.

**Recommendation**: Approach A (hybrid). The ruler is inherently a custom-drawn widget. Fighting the flex system to render 50+ tick marks at precise pixel positions is counterproductive.

## Does It Need Afterhours Changes?

**Depends on approach.**

- **Approach A (hybrid)**: No afterhours changes needed. Just use afterhours for the container `div` and draw ticks in a render system.
- **Approach B (pure)**: Would benefit from afterhours having a "canvas" or "custom draw" component that accepts a draw callback. This doesn't exist today.

A useful afterhours feature that would help both approaches: a `HasCustomDraw` component that takes a `std::function<void(Rectangle bounds)>` callback, invoked during rendering with the element's computed bounds. This would let us create a single afterhours entity that calls our custom tick-drawing code.

## How to Validate

1. **Screenshot comparison**: The ruler is visible in screenshots of tests that don't activate focus mode. Compare before/after for correct tick positions, labels, and border style.
2. **Scaling**: The ruler uses `theme::layout::scale()`. Verify tick positions scale correctly at different zoom levels.
3. **Focus mode**: Verify ruler disappears when focus mode is toggled.

## Open Questions

1. **Is the ruler interactive?** Currently it's display-only. If future plans include draggable margin handles or tab stops (like Word's ruler), the migration approach matters significantly — interactive elements need afterhours hit testing.
2. **Hybrid approach acceptable?** Using afterhours for the container but raw raylib for the ticks inside is pragmatic but not "fully migrated." Is this good enough?
3. **Zoom interaction**: The ruler should reflect the document's zoom level. Currently it uses a fixed `pixelsPerInch = 72 * scale`. Should this account for `layout.zoomLevel`?
