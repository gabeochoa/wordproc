# Gutter Decorations

The vertical strip to the left of the editor content showing per-line information: line numbers, fold markers, git change indicators, and breakpoints.

## Status

Line numbers exist (showLineNumbers in render_system.h). No fold markers, git indicators, or breakpoints.

## Decisions

- **Gutter items**: Line numbers, fold/unfold arrows, git diff indicators (green/blue/red), breakpoint dots
- **Width**: Auto-size based on what's currently shown
- **Composition**: TODO -- decide whether gutter uses per-line slots with components or a single querying widget
- **Click behavior**: TODO -- decide whether clicks are context-sensitive (click icon for action, click number for select) or uniform
