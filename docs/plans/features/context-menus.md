# Context Menus

Right-click context menus with nested submenus. Built on a similar primitive as the menu bar dropdowns but with right-click trigger and cursor-relative positioning.

## Status

Menu bar with dropdown menus exists (menu_ui_system.h, imm_menu.h). No right-click context menus yet.

## Decisions

- **Nesting**: Full nested submenus (hover to expand)
- **Item types**: Labels with keyboard shortcut hints, icons, separators, checkmarks for toggle items -- and anything else the app wants to put in a menu item (composable)
- **Positioning**: Open at mouse cursor position, adjust to stay on screen
- **Relationship to menu bar**: Similar underlying primitive as the menu bar dropdowns, with different trigger (right-click) and positioning (cursor-relative)
