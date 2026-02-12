# Document Navigation Pane

A persistent docked sidebar showing document structure for quick navigation. Essential for code editors (file tree, outline, search results) and game engine tooling (scene hierarchy, inspector panels). The headings tab with the depth slider set to H1-only serves as the outline view.

## Status

Not yet implemented as a persistent sidebar. Basic outline view exists but as a separate mode, not a docked panel.

## Decisions

- **Dock side**: Left side of the editor
- **Resize**: Draggable edge to resize panel width
- **Content**: Switchable panels within the sidebar:
  - File tree / project explorer
  - Headings tree (outline)
  - Search results
  - Comments and tracked changes
  - Document elements (bookmarks, figures, tables, footnotes)
- **Panel switcher**: Icons at the top of the sidebar to switch between panels. TODO -- decide between horizontal tabs, vertical icon strip (VS Code), or collapsible sections (Xcode) once the total panel count is clearer
- **Implementation**: Split -- dockable panel primitive in AfterHours, content/tabs in wordproc
- **Cursor sync**: Auto-scroll/highlight the pane to reflect current cursor position in the document
- **Click navigation**: Click an item to scroll the document to that location + briefly highlight the target
- **Show/hide**: Keyboard shortcut toggle
- **Multiple panels**: Single panel for now, design the dock API to support multiple panels later
- **Tree expand state**: Remember the user's expand/collapse state per document
- **Heading depth**: Configurable via a depth slider at the top of the headings tab (slide to choose H1-only through H1-H6)

### Drag-and-Drop in Headings Tree

- **DnD scope**: Drag a heading to reorder it -- all content under it (until the next same-level heading) moves with the heading
- **Drop indicator**: Horizontal line between items + indentation preview showing where the drop will land
- **Undo**: Drag-and-drop reordering is undoable with Cmd+Z
- **Multi-select**: Select multiple headings (Shift+click, Cmd+click) and drag them together
- **Keyboard reorder**: Keyboard shortcuts for moving items up/down (exact keys TBD)
- **Collapsed nodes**: Auto-expand collapsed headings when hovering during drag
- **AH widget**: Build as a general-purpose AfterHours DnD tree widget (reusable for file trees, scene graphs, etc.)
