#pragma once

#include <string>

namespace window_menu {

// Shell command that launches a second instance of this executable
// onto the same document (Window > New Window). The caller runs it
// with std::system. Empty when there is no executable path (e.g.
// Emscripten, where a new OS window is impossible) — the caller
// then does nothing.
//   POSIX:   'exe' 'file' >/dev/null 2>&1 &  (detached, no block)
//   Windows: start "" "exe" "file"           (start detaches)
// Single-quote escaping on POSIX (' -> '\''), double-quote escaping
// on Windows, so paths with spaces/specials survive.
inline std::string shellQuote(const std::string& s) {
#if defined(_WIN32)
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else out += c;
    }
    out += "\"";
    return out;
#else
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') out += "'\\''";
        else out += c;
    }
    out += "'";
    return out;
#endif
}

inline std::string newWindowCommand(const std::string& exePath,
                                    const std::string& filePath) {
#if defined(__EMSCRIPTEN__)
    (void)exePath;
    (void)filePath;
    return "";
#else
    if (exePath.empty()) return "";
    std::string cmd;
#if defined(_WIN32)
    cmd = "start \"\" " + shellQuote(exePath);
    if (!filePath.empty()) cmd += " " + shellQuote(filePath);
#else
    cmd = shellQuote(exePath);
    if (!filePath.empty()) cmd += " " + shellQuote(filePath);
    cmd += " >/dev/null 2>&1 &";
#endif
    return cmd;
#endif
}

}  // namespace window_menu
