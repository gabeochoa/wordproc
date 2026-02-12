# Command Palette

Universal command palette for searching and executing any app command without leaving the keyboard. Covers block insertion, formatting, navigation, file operations, settings -- everything. Critical for code editor and game engine tooling use cases.

## Status

Not yet implemented.

## Decisions

- **Trigger**: Keyboard shortcut only (no inline `/` trigger). Default: Cmd+Shift+P (macOS) / Ctrl+Shift+P (Win/Linux)
- **Search**: Fuzzy matching (e.g., "ofil" matches "Open File")
- **Scope**: Universal -- all app commands are searchable
- **Implementation**: Split between AfterHours (overlay/popup primitive) and wordproc (command registry)
- **Results**: Grouped by category (File, Edit, Format, Insert, etc.)
- **Keyboard shortcuts**: Show each command's shortcut next to it in the results (e.g., "Bold  Cmd+B")
- **Ranking**: By search relevance for now. TODO -- investigate ranking by recent/frequent usage
- **Preview**: Live preview of command effect before executing (e.g., preview heading style change)
- **Navigation**: Flat actions only -- no nested sub-menus, use search to narrow down
- **Appearance**: Centered on screen with a subtle pop-in animation (macOS Spotlight style)
