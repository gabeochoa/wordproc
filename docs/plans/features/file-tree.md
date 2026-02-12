# File Tree / Project Explorer

A sidebar panel showing the project directory structure for browsing, opening, and managing files. Reuses the same composable AfterHours tree widget as the outline (tree handles structure, expand/collapse, DnD, selection -- each node is an entity with components for rendering and behavior, following the same ECS composition pattern as drag and scroll).

## Status

Not yet implemented. The app currently handles single documents with an open file dialog.

## Decisions

- **Content**: Full project directory tree + an "open files" section at the top
- **File operations**: Create, rename, delete files and folders from the tree, plus drag to reorder/move
- **Drag-and-drop**: Drag files between folders to move them
- **Widget reuse**: Same composable tree widget as outline -- not a template-based generic, but ECS composition (tree node entities with components attached)
- **Icons**: App provides icons via the render callback -- the tree widget itself doesn't know about file types
- **Filter**: Text input at the top of the tree that filters as you type
- **Lazy loading**: Async scan to a configurable depth by default, then lazy-load deeper directories on expand
- **File watching**: Watch the filesystem and auto-update the tree when files change externally (git operations, external editors, etc.)
- **Inline rename**: Double-click a file name to enter inline edit mode (text field replaces the label)
- **Open behavior**: Single-click to preview (opens in a temporary/preview tab), double-click to keep open (pinned tab)
- **Gitignore**: Show all files, but dim gitignored files. No toggle to hide them entirely for now
