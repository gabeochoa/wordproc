# Toast Notifications

Ephemeral notification popups for non-blocking messages. Position, stacking, actions, and auto-dismiss behavior are all configurable per app and per toast.

## Status

Not yet implemented.

## Decisions

- **Position**: Configurable by the app (bottom-right, top-center, etc.)
- **Stacking**: App controls stacking behavior (newest on top, newest on bottom, or replace)
- **Action buttons**: Per-toast choice -- app decides whether each toast has action buttons (e.g., "Undo", "View")
- **Auto-dismiss**: Per-toast choice -- app decides whether each toast auto-dismisses after a timeout or stays until manually closed
- **Severity levels**: TODO -- decide whether to support info/warning/error styling or leave that to the app
