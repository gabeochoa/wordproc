# Autocomplete Dropdown

A popup list of completion suggestions anchored to the cursor position. The app provides completion items (from LSP, dictionary, custom sources) -- the widget handles display, filtering, keyboard navigation, and selection.

## Status

Not yet implemented.

## Decisions

- **Trigger**: Auto-trigger after typing a configurable number of characters (e.g., 1-3 chars)
- **Data source**: External -- the app provides completion items. The widget doesn't generate suggestions itself
- **Layout**: Dropdown list below the cursor. Optional documentation preview pane to the side (app decides whether to show it)
- **Navigation**: Configurable keys for navigating, accepting, and dismissing (default: arrow keys, Tab/Enter to accept, Escape to dismiss)
- **Filtering**: Fuzzy match against typed text to narrow the list as the user types
