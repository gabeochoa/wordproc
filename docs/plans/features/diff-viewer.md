# Diff Viewer

Side-by-side or inline/unified view of text differences. Composed from existing primitives (SplitPane + two editor instances + gutter decorations for change markers) rather than a monolithic widget.

## Status

Not yet implemented.

## Decisions

- **Layout**: Both side-by-side and inline/unified modes, user toggles between them
- **Navigation**: Jump between changes via buttons or shortcuts, plus free scrolling
- **Editing**: Right (new) side is editable in place; left (old) side is read-only
- **Composition**: Built from existing primitives -- SplitPane, editor instances, gutter decorations for change markers. Not a standalone widget
- **Change highlighting**: TODO -- decide coloring scheme for additions/deletions/modifications
