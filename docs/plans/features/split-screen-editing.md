# Split-Screen Editing

View two parts of the same document side by side, or view different documents in parallel. Supports horizontal and vertical splits. Each pane scrolls independently. Foundational for code editor use cases and game engine tooling (inspector panels, asset browsers).

## Status

Not yet implemented.

## Decisions

- **Split count**: 2-way split initially, but design the API so N-way recursive splits are possible later
- **Split direction**: Both horizontal and vertical, user chooses
- **Splitter UI**: Draggable divider bar between panes
- **Pane content**: Each pane can show the same document at different positions or different documents
- **Focus model**: Click-to-focus (only one pane receives keyboard input at a time)
- **Active pane indicator**: TODO -- think about how to visually indicate the active pane
- **Implementation**: Build as a reusable AfterHours UI widget (SplitPane component)
- **Minimum size**: Enforce a minimum pane width/height so content is always usable
- **Scroll sync**: Always independent scrolling between panes
- **Drag between panes**: No, use copy/paste between panes
- **Keyboard shortcuts**: Yes, support shortcuts for splitting/closing panes. TODO -- determine shortcut for switching focus between panes
