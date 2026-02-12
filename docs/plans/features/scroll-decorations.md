# Scroll Decorations

Colored marks in the scrollbar track indicating positions of interest: search matches, errors/warnings, and modified lines. Provides at-a-glance document overview without scrolling.

## Status

Not yet implemented.

## Decisions

- **Location**: In the scrollbar track itself (not the minimap)
- **Decoration types**: Search matches, errors/warnings, modified lines (unsaved changes)
- **Composition**: TODO -- decide whether the scrollbar widget accepts decoration data or each system renders its own marks. Depends on implementation difficulty
