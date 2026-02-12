# Bottom Panel Dock

A resizable panel docked to the bottom of the editor for terminal, output/logs, and other tool panels. Reuses the same dockable panel primitive as the sidebar (same AfterHours widget, configured for bottom orientation).

## Status

Not yet implemented.

## Decisions

- **Composition**: Same dockable panel primitive as the sidebar, configured for bottom dock position
- **Panels**: Terminal and output/logs initially. More panels (problems, debug) can be added later
- **Terminal**: Full interactive terminal (PTY-based) is the goal, but defer the terminal emulator implementation -- focus on the panel layout first
- **Multiple instances**: Support multiple terminal instances (like VS Code's terminal tabs)
- **Panel switching**: Tabs at the top of the bottom panel (same pattern as sidebar panel switcher)
- **Resize**: Draggable top edge
- **Show/hide**: Keyboard shortcut toggle
