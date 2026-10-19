#pragma once

#include <string>
#include <string_view>

#include "../../vendor/afterhours/src/plugins/clipboard.h"

namespace app {
namespace clipboard {

namespace detail {
    inline std::string test_clipboard_text;
    inline bool test_mode_enabled = false;
}

// Enable/disable test mode (uses in-memory clipboard instead of system)
inline void enable_test_mode() { detail::test_mode_enabled = true; }
inline void disable_test_mode() { detail::test_mode_enabled = false; }
inline bool is_test_mode() { return detail::test_mode_enabled; }

// Set clipboard text
inline void set_text(std::string_view text) {
    if (detail::test_mode_enabled) {
        detail::test_clipboard_text = std::string(text);
        return;
    }
    afterhours::clipboard::set_text(text);
}

// Get clipboard text
inline std::string get_text() {
    if (detail::test_mode_enabled) {
        return detail::test_clipboard_text;
    }
    return afterhours::clipboard::get_text();
}

// Check if clipboard has text
inline bool has_text() {
    if (detail::test_mode_enabled) {
        return !detail::test_clipboard_text.empty();
    }
    return afterhours::clipboard::has_text();
}

} // namespace clipboard
} // namespace app

