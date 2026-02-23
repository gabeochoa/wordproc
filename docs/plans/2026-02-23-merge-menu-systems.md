# Merge MenuSystem into MenuUISystem

**Date:** 2026-02-23
**Type:** Refactoring — reduce system count, fix bug
**Impact:** Eliminates one system, fixes F1 double-toggle bug, ~50 lines removed

## Problem

There are two menu-related systems registered in the pipeline:

1. **`MenuUISystem`** (update phase, `src/ecs/menu_ui_system.h`) — 1200 lines. Renders menu bar, dropdowns, and all modal dialogs via afterhours immediate-mode UI.

2. **`MenuSystem`** (render phase, `src/ecs/render_system.h`) — ~35 lines. Only does two things:
   - Calls `handleMenuActionImpl` when `menu.consumeClickedResult()` returns a value
   - Checks F1 to toggle `menu.showHelpWindow`

Meanwhile, `EditorRenderSystem` (also render phase) independently checks F1:
```cpp
if (input::isKeyPressed(afterhours::keys::F1)) {
    menu.showHelpWindow = !menu.showHelpWindow;
    menu.helpScrollOffset = 0;
}
```

This causes a **double-toggle bug**: both systems fire on the same frame, toggling the help window on and then immediately off (or vice versa), making F1 appear to do nothing.

## Proposed Solution

1. Move `handleMenuActionImpl` dispatch into `MenuUISystem::for_each_with()`, right after the menu item click handling code. The clicked result is already set by `MenuUISystem` — consuming it in the same system is more natural.

2. Remove `MenuSystem` entirely from `render_system.h` and from the system registration in `main.cpp`.

3. Remove the F1 check from `EditorRenderSystem` (keep only the one in `MenuUISystem`, or move it to `KeyboardShortcutSystem` where other shortcuts live).

## Files to Change

| File | Change |
|------|--------|
| `src/ecs/menu_ui_system.h` | Add `handleMenuActionImpl` call after item click handling |
| `src/ecs/render_system.h` | Remove `MenuSystem` struct, remove F1 check from `EditorRenderSystem` |
| `src/main.cpp` | Remove `sm.register_render_system(std::make_unique<ecs::MenuSystem>())` |

## Risks

- `handleMenuActionImpl` currently runs in the render phase; moving it to update phase changes execution order. This should be fine since it only mutates component data (no draw calls), but needs verification.
- The F1 check must exist in exactly one place after the merge.

## Migration Strategy

1. Remove the F1 check from `EditorRenderSystem`
2. Verify help window toggle works (single toggle per press)
3. Move `handleMenuActionImpl` call into `MenuUISystem`
4. Remove `MenuSystem` and its registration
5. Run all E2E tests to verify
