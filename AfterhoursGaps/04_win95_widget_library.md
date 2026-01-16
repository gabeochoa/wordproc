# Win95-Style Widget Library

## Problem
Afterhours does not provide a Win95 or classic UI widget set for buttons,
menus, dialogs, and other UI elements.

## Current Workaround
- Custom `src/ui/win95_widgets.h/.cpp` implements:
  - Raised and sunken borders.
  - Buttons with hover, pressed, and disabled states.
  - Checkboxes with state tracking.
  - Menu bar with dropdowns.
  - Message and input dialogs.

## Desired Behavior
- Themeable widget primitives for classic UI styles.
- Built-in menu bar and dropdown components with keyboard navigation.
- Standardized dialog layouts and button placement.

## Proposed API Sketch
- `ui::theme::set("win95")` or theme enums.
- `ui::MenuBar`, `ui::Menu`, `ui::MenuItem`.
- `ui::Dialog` with configurable button sets.

## Notes
This would let the app rely on Afterhours UI instead of custom widgets.

