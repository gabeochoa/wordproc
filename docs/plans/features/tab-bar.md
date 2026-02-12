# Tab Bar

A row of tabs representing open files/documents. Each tab is an entity (TabButton) inside a TabBar container, following AfterHours' ECS composition pattern. Tab names render via a text component so styling (italic for preview, bold, etc.) is handled by the component, not the tab bar widget.

## Status

Not yet implemented. The app currently supports one open document at a time.

## Decisions

- **Composition**: TabBar is a container of TabButton entities. Each TabButton has components for close button, dirty indicator, icon (optional), and label (text component)
- **Position**: App decides where to place the tab bar (top, bottom, etc.)
- **Close button**: Always visible on the focused tab, visible on hover for other tabs
- **Dirty indicator**: Dot replaces the close X when the document has unsaved changes
- **Drag-and-drop**: Full DnD -- reorder within the tab bar, drag to another split group to move there, drag out to create a new split
- **Icons**: Optional per tab -- app provides via the render callback (file type icons, document icons, etc.)
- **New tab button**: No '+' button -- use keyboard shortcut (Cmd+N) only
- **Tab overflow**: TODO -- decide overflow handling later (scroll arrows, shrink + scroll, or dropdown)
- **Preview tabs**: Styling of preview vs. pinned tabs is handled by the text component on the tab label, not by the tab bar widget itself
- **Pin tabs**: TODO -- decide later
- **Tab preview on hover**: TODO -- decide later
- **Right-click context menu**: TODO -- decide later
