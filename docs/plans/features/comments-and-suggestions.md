# Comments & Suggestions

Annotate documents with comments anchored to text ranges. Threaded replies with resolution workflow. Pushes AfterHours' anchored overlay and z-ordering capabilities -- useful for annotation tools, game HUDs, and editor inspectors.

## Status

Basic comments exist. Missing: threading, resolution, anchored overlays, and nav pane integration.

## Decisions

- **Display**: Three views working together:
  - Background highlight + small margin icon on commented text in the document
  - Comments tab in the navigation pane (sidebar) listing all comments
  - Inline popover on click for quick interaction
- **Anchoring**: Comments anchor to a text range (highlighted span)
- **Threading**: Threaded replies nested under the original comment, with a "resolve" button to collapse the thread
- **Suggestions mode**: Comments first; add suggestions (proposed edits with accept/reject) later
- **Comment location**: Comments listed in the nav pane's comments tab (not a separate right margin)
- **Overlap handling**: When multiple comments are close together, collapse into a numbered badge in the margin (e.g., "3") that expands into a list on click
- **Implementation**: Build anchored overlay/popover as a reusable AfterHours widget
- **Document highlight**: Background highlight on the anchored text range + a small icon/indicator in the margin
- **Navigation**: Bidirectional -- click highlighted text to scroll nav pane to that comment, click comment in nav pane to scroll document to its anchor
- **Adding comments**: Both right-click context menu ("Add Comment") and keyboard shortcut (e.g., Cmd+Shift+M)
- **Resolution**: Resolved comments move to a separate "Resolved" section in the nav pane (not deleted)
