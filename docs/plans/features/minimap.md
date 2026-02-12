# Minimap

A zoomed-out overview of the document displayed at the edge of the editor. Shows the full document structure with a viewport rectangle indicating the currently visible area. Click or drag to navigate.

## Status

Not yet implemented.

## Decisions

- **Position**: Right edge by default, but configurable via AfterHours widget API
- **Rendering**: TODO -- decide rendering approach later (tiny text vs. abstract color blocks)
- **Interaction**: Click to scroll to that position, drag to scrub through the document
- **Viewport indicator**: Semi-transparent rectangle showing the currently visible area
- **Show/hide**: TODO -- decide toggle mechanism
