# AfterHours UI Primitives

New composable UI primitives needed in AfterHours to support wordproc features, code editor use cases, and game engine tooling. Each primitive should follow AfterHours' existing patterns: ECS composition, immediate-mode API, ComponentConfig builder, flexbox layout.

## What Already Exists

These are already in AfterHours and should be built on, not duplicated:

- **Layout**: `div()`, `scroll_view()`, `separator()`, flexbox (Row/Column, justify, align, wrap)
- **Buttons**: `button()`, `button_group()`, `image_button()`, `checkbox()`, `radio_group()`, `toggle_switch()`
- **Input**: `text_input()`, `text_area()`, `slider()`, `dropdown()`
- **Navigation**: `tab_container()`, `navigation_bar()`, `pagination()`
- **Display**: `progress_bar()`, `circular_progress()`, `image()`, `sprite()`, `icon_row()`
- **Composite**: `setting_row()`
- **Interaction**: `HasClickListener`, `HasDragListener`, `HasLeftRightListener`
- **Scroll**: `HasScrollView`, `HasClipChildren`
- **Modals**: `modal` plugin (stacking, focus trapping, dialog results)
- **Toasts**: `toast` plugin (auto-dismiss, severity levels)
- **Animations**: Declarative per-component (on_click, on_appear, on_hover, loop)
- **Theming**: `Theme`, `ComponentConfig`, color utilities

---

## New Primitives

### 1. Draggable Divider

A resize handle between two regions. The user drags it to redistribute space. This is the core building block for split panes and resizable panels.

**Inputs:**
- Orientation (horizontal or vertical)
- Minimum size for each side
- Current split ratio or pixel position

**Outputs:**
- New split position on drag

**Behavior:**
- Renders a thin bar (or invisible hit zone with a visible grab line)
- Changes cursor to resize cursor on hover
- Clamps to minimum sizes on each side
- Emits new position via callback or component state

**Used by:** SplitPane, sidebar resize, bottom panel resize

---

### 2. Dockable Panel

A panel that docks to an edge of a parent container. Combines a content area with a draggable divider for resizing. Can be shown/hidden.

**Inputs:**
- Dock edge (left, right, top, bottom)
- Initial size
- Minimum size
- Visible/hidden state

**Outputs:**
- Current size (after resize)
- Visibility state

**Behavior:**
- Docks to the specified edge of its parent
- Draggable divider on the inner edge for resizing
- Collapsible (shortcut or API toggle)
- Content is provided by the app as child entities

**Composition:** `div()` + Draggable Divider + `HasClipChildren`

**Used by:** Sidebar (left dock), bottom panel (bottom dock)

---

### 3. Split Pane

Two content regions separated by a draggable divider. Each region is an independent container that the app fills with content.

**Inputs:**
- Orientation (horizontal or vertical)
- Initial split ratio
- Minimum pane size

**Outputs:**
- Current split ratio

**Behavior:**
- Two child containers with a draggable divider between them
- Click-to-focus model (one pane is "active" at a time)
- Designed for 2-way initially, API should allow nesting for N-way splits

**Composition:** `div()` + `div()` + Draggable Divider

**Used by:** Split-screen editing, diff viewer (side-by-side mode)

---

### 4. Tree Node

A collapsible node in a hierarchical tree. The tree itself is just a scrollable container of tree nodes. Each node is an entity; the app provides the render callback for the node's content.

**Inputs:**
- Depth level (for indentation)
- Expanded/collapsed state
- Whether the node has children
- App-provided content (child entities for the node row)

**Outputs:**
- Expand/collapse toggle events
- Selection state
- DnD events (drag start, drag over, drop)

**Behavior:**
- Indentation based on depth
- Expand/collapse arrow (if has children)
- Selectable (click to select, Shift/Cmd+click for multi-select)
- Draggable (if DnD is enabled)
- Drop target (with line indicator showing drop position)
- Keyboard navigation (arrow keys to move, Enter to expand/collapse, Space to select)

**Composition:** `div()` with depth-based padding + `HasClickListener` + optional `HasDragListener` + app-provided child entities

**The tree widget does NOT know about files, headings, or any domain-specific content.** The app creates tree node entities, attaches content components, and the tree handles structure.

**Used by:** File tree, document outline, scene hierarchy, search results grouping

---

### 5. Anchored Popup

A floating container positioned relative to an anchor point in the document or UI. Stays attached to its anchor as the view scrolls. Auto-positions to avoid going off-screen.

**Inputs:**
- Anchor position (pixel coordinates or entity reference)
- Preferred placement (above, below, left, right)
- Content (child entities)

**Outputs:**
- Computed position (after screen-edge adjustment)
- Dismiss events

**Behavior:**
- Floats above other content (z-ordered)
- Repositions if anchor moves (e.g., scroll)
- Flips placement if it would go off-screen
- Optional: dismiss on click-outside, dismiss on Escape

**Composition:** `div()` with absolute positioning + z-ordering + screen bounds clamping

**Used by:** Autocomplete dropdown, hover cards, comment popovers, context menus, breadcrumb dropdowns

---

### 6. Command Palette

A centered overlay with a text input and a scrollable filtered results list. The app registers commands; the palette handles fuzzy search, keyboard navigation, and selection.

**Inputs:**
- List of items (each with: label, category, shortcut hint, callback)
- Trigger shortcut

**Outputs:**
- Selected item callback
- Dismiss events

**Behavior:**
- Centered on screen with pop-in animation
- Text input with fuzzy matching
- Results grouped by category
- Keyboard navigation (Up/Down to move, Enter to select, Escape to dismiss)
- Shortcut hints displayed next to each item
- Live preview callback (optional, app provides)

**Composition:** Anchored Popup (centered) + `text_input()` + `scroll_view()` + filtered list of `div()` rows

**Used by:** Command palette (Cmd+Shift+P), file finder, symbol search

---

### 7. Tab Strip

A horizontal row of tab entities. Each tab is an entity the app creates with content components (label, icon, close button, dirty indicator). The strip handles ordering, DnD, overflow, and active state.

**Inputs:**
- Tab entities (app creates them with whatever content components they want)
- Active tab ID

**Outputs:**
- Tab selected events
- Tab closed events
- Tab reorder events (from DnD)
- Tab detach events (drag out of strip)

**Behavior:**
- Horizontal row layout
- Click to select a tab
- Close button (always visible on active tab, hover on others)
- DnD reorder within the strip
- DnD detach (drag out to create a split or new window)
- Dirty indicator (dot replaces close button -- app controls via component state)

**Composition:** `div()` (Row) containing tab entity children, each with `HasClickListener` + `HasDragListener` + app-provided label/icon/close components

**Used by:** Tab bar (open files), bottom panel tab switching, sidebar panel switching

---

### 8. Breadcrumb Bar

A horizontal row of clickable segments with separator icons. Each segment can open a dropdown showing siblings. Collapses middle segments into "..." when too long.

**Inputs:**
- Ordered list of segments (each with: label, dropdown items callback)

**Outputs:**
- Segment click events
- Dropdown item selection events

**Behavior:**
- Horizontal row with separator characters between segments
- Click a segment to open a dropdown of siblings at that level
- Overflow: collapse middle segments into "..." (keep first and last visible)
- Keyboard accessible

**Composition:** `div()` (Row) + per-segment `button()` + Anchored Popup for dropdowns

**Used by:** File path breadcrumbs, heading hierarchy breadcrumbs

---

### 9. Minimap

A scaled-down rendering of document content in a narrow vertical strip. Shows a viewport rectangle indicating the currently visible area. Click or drag to navigate.

**Inputs:**
- Document content (app provides a render callback for the minimap content)
- Viewport position and size (relative to total document)

**Outputs:**
- Navigation events (click position, drag position)

**Behavior:**
- Renders a scaled overview of the content
- Semi-transparent rectangle shows the current viewport
- Click to jump to a position
- Drag to scrub through the document
- Configurable position (right edge by default)

**Composition:** `div()` with custom render callback + `HasDragListener` + viewport overlay `div()`

**Used by:** Editor minimap, document overview

---

### 10. Gutter

A vertical strip alongside a scrollable content area, providing per-line slots for decorations (line numbers, icons, indicators). Each slot aligns with a content line.

**Inputs:**
- Line count
- Line height (must match the content area)
- Slot content per line (app provides via components)

**Outputs:**
- Click events per line/slot

**Behavior:**
- Renders aligned with the adjacent content area's scroll position
- Auto-sizes width based on what's shown (line number digit count, icon columns)
- Each line can have multiple columns of decorations
- Click handling per slot (e.g., toggle breakpoint, toggle fold)

**Composition:** `div()` (Column) synced to content scroll + per-line `div()` rows with app-provided content

**Used by:** Line numbers, fold markers, breakpoints, git change indicators

---

### 11. Scroll Decoration Layer

Colored marks rendered within the scrollbar track showing positions of interest. An extension to the existing `HasScrollView`.

**Inputs:**
- List of marks (each with: position as fraction of document, color)

**Outputs:**
- (Display only, no interaction)

**Behavior:**
- Renders small colored rectangles at proportional positions within the scrollbar track
- Multiple marks can overlap (render in order)
- Updates when marks change

**Composition:** Extension to `scroll_view()` -- additional render pass over the scrollbar area

**Used by:** Search match positions, error/warning locations, modified line indicators

---

### 12. Collapsible Region

A content area that can be collapsed to show only a summary line. Click the summary or an arrow to expand/collapse. Animatable.

**Inputs:**
- Summary content (child entities for the collapsed state)
- Expanded content (child entities for the expanded state)
- Initial expanded/collapsed state

**Outputs:**
- Expand/collapse toggle events

**Behavior:**
- Shows summary when collapsed, full content when expanded
- Arrow indicator rotates on toggle
- Optional animation (height transition)
- Can be nested

**Composition:** `div()` + `HasClickListener` on the summary row + conditional child rendering

**Used by:** Code folding (inline ellipsis), tree node expand/collapse, settings categories, search result file groups

---

### 13. Fuzzy Matcher

Not a UI component -- a utility algorithm. Takes a query string and a list of candidate strings, returns ranked matches with match highlights.

**Inputs:**
- Query string
- List of candidates (each with: text, optional metadata)

**Outputs:**
- Ranked list of matches with match character indices (for highlighting)

**Behavior:**
- Fuzzy substring matching (non-contiguous characters)
- Scoring based on match quality (consecutive chars, word boundaries, prefix match)
- Fast enough for real-time filtering (< 1ms for thousands of candidates)

**Used by:** Command palette, file tree filter, settings search, autocomplete dropdown

---

## Composition Map

How these primitives compose into the features we've planned:

```
Split-Screen Editing
  └── Split Pane
       ├── Draggable Divider
       ├── Content Pane (div)
       └── Content Pane (div)

Sidebar (Nav Pane)
  └── Dockable Panel (left)
       ├── Draggable Divider (right edge)
       ├── Tab Strip (panel switcher, top)
       └── Panel Content (scroll_view)
            ├── File Tree → Tree Node (recursive)
            ├── Outline  → Tree Node (recursive)
            ├── Search   → Tree Node (grouped results)
            └── Comments → scroll_view + comment entities

Bottom Panel
  └── Dockable Panel (bottom)
       ├── Draggable Divider (top edge)
       ├── Tab Strip (terminal/output switcher)
       └── Panel Content

Tab Bar
  └── Tab Strip
       └── Tab entities (app-provided content)

Command Palette
  └── Anchored Popup (centered)
       ├── text_input() + Fuzzy Matcher
       └── scroll_view() + filtered results

Autocomplete
  └── Anchored Popup (below cursor)
       ├── scroll_view() + filtered results
       └── Optional docs pane (div)

Hover Cards
  └── Anchored Popup (near target)
       └── App-provided content

Context Menus
  └── Anchored Popup (at cursor)
       └── Menu items (recursive for submenus)

Breadcrumbs
  └── Breadcrumb Bar
       └── Anchored Popup (per segment dropdown)

Comments
  └── Anchored Popup (at text range)
       └── Comment content + thread

Diff Viewer
  └── Split Pane
       ├── Editor (read-only, left)
       └── Editor (editable, right)
       + Gutter decorations for change markers

Code Folding
  └── Collapsible Region (per fold range)
       + Gutter slot (fold arrow)

Minimap
  └── Minimap widget
       + Scroll Decoration Layer (optional)

Settings Editor
  └── scroll_view()
       ├── text_input() (search filter) + Fuzzy Matcher
       └── setting_row() entries (already exists)
```

## Implementation Order

Suggested order based on dependency chain (each primitive unlocks the features listed):

1. **Draggable Divider** → unlocks Split Pane, Dockable Panel
2. **Dockable Panel** → unlocks sidebar, bottom panel
3. **Tree Node** → unlocks file tree, outline with DnD
4. **Tab Strip** → unlocks tab bar, panel switchers
5. **Anchored Popup** → unlocks autocomplete, hover cards, context menus, command palette
6. **Fuzzy Matcher** → unlocks command palette, file tree filter, settings search
7. **Command Palette** → unlocks Cmd+Shift+P
8. **Collapsible Region** → unlocks code folding, tree nodes (shares expand/collapse)
9. **Breadcrumb Bar** → unlocks path navigation
10. **Gutter** → unlocks line numbers, fold markers, breakpoints, git indicators
11. **Scroll Decoration Layer** → unlocks scrollbar marks
12. **Minimap** → unlocks document overview
