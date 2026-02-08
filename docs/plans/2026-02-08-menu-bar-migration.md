# Menu Bar Migration to Afterhours

## Current State

The menu bar has a split implementation:

- **`MenuUISystem`** (update system in `menu_ui_system.h`): Creates afterhours UI entities for the menu bar headers and dropdown items. Renders them as absolute-positioned `button()` and `div()` elements. Also handles modals (About, Word Count, etc.) via `afterhours::modal`.
- **`win95::DrawMenuBar`** (render system in `EditorRenderSystem`): The legacy immediate-mode raylib draw calls that ACTUALLY handle clicks, hover-to-switch, and dropdown rendering. Called in `render_system.h` line 842.
- **`win95_widgets.cpp`**: Contains `DrawMenuBar` (~100 lines), `DrawDropdownMenu` (~120 lines), and supporting hover/click logic.

The result: menu visuals are doubled — afterhours creates UI entities AND legacy code draws on top. Click handling lives in the legacy path. The afterhours buttons exist but don't handle clicks (comment on line 228: "Click handling for menu items is done by win95::DrawDropdownMenu").

## What Needs to Happen

1. **Move click handling into `MenuUISystem`**: The afterhours `button()` calls already return `ElementResult` which is truthy on click. Wire up the click results to set `menu.lastClickedResult` (the encoded `menuIdx * 100 + itemIdx`).
2. **Add hover-to-switch logic**: When any menu is open and mouse hovers a different header button, switch the open menu. Afterhours provides `ctx.is_hot(id)` to detect hover.
3. **Add click-outside-to-close**: Afterhours has `CloseDropdownOnClickOutside` system. Evaluate whether that works for this use case, or add manual detection.
4. **Remove `win95::DrawMenuBar` call** from `EditorRenderSystem::for_each_with` (line 842).
5. **Remove `MenuSystem` render system** entirely — its only job is dispatching `menu.consumeClickedResult()`, which can move into `MenuUISystem`.
6. **Delete `DrawMenuBar` and `DrawDropdownMenu`** from `win95_widgets.cpp`.

## Does It Need Afterhours Changes?

**Probably not.** The existing afterhours primitives (`button`, `div`, absolute positioning, `is_hot`, render layers) are sufficient. Two potential gaps:

- **Mark/checkmark column**: The legacy dropdown reserves 20px for checkmarks (✓, •, -). Afterhours `button()` doesn't natively support a leading icon/mark. **Workaround**: Use a `div` with `FlexDirection::Row` containing a small mark label + the item label, or prepend the mark character to the label string.
- **Separator rendering**: `imm_menu.h` already calls `imm::separator()`. Verify this exists in the current afterhours version and renders a visible line.

## How to Validate

1. **E2E test `pass_file_menu_dropdown`**: Already validates File menu opens and shows New, Open, Save, Exit. After migration, this test must still pass.
2. **E2E test `pass_expect_text_menu`**: Validates all menu labels (File, Edit, View, Format, Insert, Table, Help) are visible.
3. **Visual screenshot review**: Compare before/after screenshots of menu bar and dropdown for correct styling, hover highlight, separator lines, shortcut alignment, and checkmark display.
4. **Manual check**: Hover between menu headers while a dropdown is open — it should switch menus without clicking.
5. **`menu_select` e2e command**: Existing tests use `menu_select` to trigger menu items. These must continue to work.

## Open Questions

1. **Who handles F1 (help toggle)?** Currently duplicated in both `EditorRenderSystem` (line 656) and `MenuSystem` (line 895). Should consolidate into one place — probably `MenuUISystem` or `KeyboardShortcutSystem`.
2. **Menu action handler location**: `handleMenuActionImpl` is 800+ lines of switch/case logic. Should it stay in `render_system.h` or move to its own file? It doesn't render anything — it's pure logic.
3. **`imm_menu.h` — keep or delete?** There's a fully-implemented `imm_menu::renderMenuBar()` in `imm_menu.h` that's never called. It's a parallel implementation to `MenuUISystem`. Decide whether to use it, merge it, or delete it.
4. **`win95::Menu` vs `imm_menu::Menu`**: Two separate Menu structs exist. After migration, we only need one. Which one stays?
5. **Render layer ordering**: Menu dropdowns use `with_render_layer(10)` and items use `11`. Is the afterhours render layer system stable enough for correct z-ordering, especially with toasts at layer 100?
