# Breadcrumbs

A horizontal bar of navigable path segments below the tab bar. App provides the segments (file path, heading hierarchy, symbol tree, or anything else). Click a segment to open a dropdown showing siblings at that level for quick navigation.

## Status

Not yet implemented.

## Decisions

- **Content**: App provides breadcrumb segments (the widget doesn't assume they're file paths)
- **Click behavior**: Click a segment to open a dropdown showing siblings at that level (e.g., other files in the folder, other headings at the same level)
- **Position**: Below the tab bar, above the editor content
- **Overflow**: Collapse middle segments into "..." when the path is too long to fit
