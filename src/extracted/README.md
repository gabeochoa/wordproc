# Extracted Code for Afterhours PR

This folder contains clean, standalone implementations of features that could be
contributed to the Afterhours library. Each file is designed to be:

1. **Self-contained** - Minimal dependencies, easy to integrate
2. **Well-documented** - Includes usage examples and integration notes
3. **PR-ready** - Clean code following Afterhours conventions

## Files

### `bevel_border.h`
3D beveled borders for Win95/retro UI styles. Provides `BevelStyle` enum,
`BevelBorder` struct, and drawing functions for raised/sunken borders.

**Gap doc**: `AfterhoursGaps/04_win95_widget_library.md`

### `command_history.h`
Generic undo/redo system using the Command pattern. Useful for level editors,
paint tools, forms, and any application with reversible actions.

**Gap doc**: `AfterhoursGaps/10_command_history.md`

### `status_notifications.h`
Timed notification/toast system for status messages. Supports info, success,
warning, and error levels with auto-dismiss.

**Gap doc**: `AfterhoursGaps/11_status_notifications.md`

### `action_binding.h`
Remappable keyboard shortcuts with modifier key support (Ctrl, Shift, Alt).
Useful for games with rebindable controls and productivity apps.

**Gap doc**: `AfterhoursGaps/06_action_binding_system.md`

### `icon_registry.h`
Centralized icon management with:
- Map action/item IDs to icon resources
- Fallback text symbols when icons unavailable
- Mirrored icon pairs (undo/redo, left/right arrows)
- Common icon presets for file/edit/view operations

**Use cases**: Inventory systems, skill icons, status effects, toolbar icons

**Gap doc**: `AfterhoursGaps/13_icon_registry.md`

## How to Create a PR

1. Copy the relevant `.h` file to `afterhours/src/plugins/`
2. Add any necessary includes
3. Test in a sample project
4. Update Afterhours documentation
5. Submit PR with link to gap doc for context

## Integration Pattern

Each file follows a similar pattern:
- Structs/enums for data
- Component class (e.g., `HasBevelBorder`, `ProvidesNotifications`)
- Helper functions for common operations
- Usage examples in comments

Most features are header-only for easy integration.

