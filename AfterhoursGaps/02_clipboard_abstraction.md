# Clipboard Abstraction

## Problem
Afterhours does not provide a cross-platform clipboard API.
The app uses raylib's `GetClipboardText()` and `SetClipboardText()` directly.

## Current Workaround
- Call raylib clipboard functions in editor actions.
- No centralized abstraction for future renderer or runtime changes.

## Desired Behavior
- Platform-agnostic clipboard interface with UTF-8 text support.
- Optional rich-text payload support for future versions.
- Clear ownership and lifetime rules for returned strings.

## Proposed API Sketch
- `clipboard::set_text(std::string_view text)`
- `clipboard::get_text() -> std::string`
- `clipboard::has_text() -> bool`

## Notes
This would remove a core dependency on raylib in the editor layer.
# Clipboard Abstraction

## Problem
Afterhours does not provide a cross-platform clipboard API.
The app uses raylib's `GetClipboardText()`/`SetClipboardText()` directly.

## Current Workaround
- Call raylib clipboard functions in editor actions.
- No centralized abstraction for future renderer/runtime changes.

## Desired Behavior
- A platform-agnostic clipboard interface with UTF-8 text support.
- Optional rich-text payload support (future).
- Clear ownership and lifetime rules for returned strings.

## Proposed API Sketch
- `clipboard::set_text(std::string_view text)`
- `clipboard::get_text() -> std::string`
- `clipboard::has_text() -> bool`

## Notes
This would remove a core dependency on raylib in the editor layer.

