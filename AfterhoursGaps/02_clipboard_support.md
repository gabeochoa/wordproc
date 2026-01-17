# Clipboard Support

## Problem
Afterhours has no clipboard abstraction or UI-facing clipboard API. Apps must
reach into backend-specific functions directly.

## Current Workaround
The app uses raylib's clipboard calls directly:
- `GetClipboardText()` for paste
- `SetClipboardText()` for copy

This works but ties UI code to raylib and blocks testing in headless modes.

## Desired Behavior
Provide a simple cross-platform clipboard API in Afterhours:

```cpp
namespace afterhours::platform {

// Text clipboard
bool has_clipboard_text();
std::string get_clipboard_text();
void set_clipboard_text(const std::string& text);

// Optional future extensions
// bool has_clipboard_rtf();
// std::string get_clipboard_rtf();
// void set_clipboard_rtf(const std::string& rtf);
// bool has_clipboard_html();
// std::string get_clipboard_html();
// void set_clipboard_html(const std::string& html);

} // namespace afterhours::platform
```

## Notes
- Keep the base API text-only for simplicity.
- Rich text (RTF/HTML) can be optional and backend-specific.

