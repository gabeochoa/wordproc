# Context Menu Design

## Goal

Add right-click context menus to the document editor. No afterhours changes for now -- use existing modal + tray primitives and a custom E2E command handler. Generic parts documented in afterhours feature requests for future upstream.

## Approach: No afterhours changes

### `right_click` E2E command (wordproc-side workaround)

The afterhours runner's fallback parser already handles unknown commands by parsing remaining tokens as args. So `right_click 50% 50%` parses as name=`"right_click"`, args=`["50%", "50%"]`.

We register a custom E2E command handler in wordproc that:
1. Reads the x/y coordinates from args
2. Directly sets `ContextMenuState.open = true` with the position
3. Skips the real input simulation path entirely

This means the E2E test doesn't go through `is_mouse_button_pressed(1)` -- it takes a shortcut. For real (non-test) usage, we detect right-click via the backend's `is_mouse_button_pressed(1)` which already works.

TODO: Upstream proper right-click support to afterhours (see afterhours-feature-requests.md #14).

### Context menu rendering (existing afterhours primitives)

Use `modal` with `ClosedBy::Any` + `tray()` + `button()` directly in wordproc code. No new afterhours primitive needed. The modal is positioned absolutely at the right-click coordinates, clamped to screen bounds.

## Implementation

### ContextMenuState

Add fields to `DialogState` in `components.h`:

```cpp
bool showContextMenu = false;
float contextMenuX = 0, contextMenuY = 0;
```

### Right-click detection (real usage)

In `input_system.h`, detect `is_mouse_button_pressed(1)` when no dialog is open. Set `dialogs.showContextMenu = true` and store mouse position.

### Context menu rendering

In `menu_ui_system.h`, when `dialogs.showContextMenu` is true, render a modal at `(contextMenuX, contextMenuY)` containing a tray with:
- Cut (Ctrl+X)
- Copy (Ctrl+C)
- Paste (Ctrl+V)
- separator
- Select All (Ctrl+A)

Wire each item to existing document commands.

### Custom E2E command handler

In `e2e_runner.cpp` or `e2e_commands.h`, register a handler for `right_click` that sets `ContextMenuState` directly.

### Dismiss behavior

Modal's `ClosedBy::Any` provides:
- Click outside -> dismissed
- Escape -> cancelled
- Item click -> close after action (manual)

## Files to change

**wordproc only (no vendor/ changes):**
- `src/ecs/components.h` -- context menu fields on `DialogState`
- `src/ecs/input_system.h` -- right-click detection
- `src/ecs/menu_ui_system.h` -- context menu rendering
- `src/testing/e2e_runner.cpp` -- custom `right_click` command handler + reset
- `src/testing/e2e_commands.h` -- `HandleRightClickCommand` system

## Not in scope

- Context-aware items (different items for hyperlinks, tables, images)
- Nested submenus
- Custom icons or marks
- These can be added later by extending the item list
