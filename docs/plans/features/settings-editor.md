# Settings Editor

A visual settings UI that reads/writes a settings file. Searchable, with app-defined categories. Supports all standard control types (toggles, dropdowns, text fields, color pickers, etc.).

## Status

App settings exist (settings.h/.cpp) but with no visual editor UI. Settings are configured via code or basic dialogs.

## Decisions

- **Format**: Both -- visual UI that reads/writes a backing settings file (JSON or similar)
- **Search**: Searchable settings via a filter bar at the top
- **Categories**: App defines the category structure
- **Control types**: Toggles (booleans), dropdowns (enums), text fields (strings/numbers), color pickers, sliders, key binding editors -- all standard input types
- **Composition**: Each setting entry is an entity with a label component + the appropriate input control component. The settings editor is a scrollable list of these entities
