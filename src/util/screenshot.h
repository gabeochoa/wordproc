#pragma once

#include <filesystem>
#include <vector>
#include <afterhours/src/graphics.h>

namespace app::capture {
inline std::vector<std::filesystem::path> pending;
inline bool headless = false;

inline void request(const std::filesystem::path& path) {
    pending.push_back(path);
}

inline bool flush() {
    bool success = true;
    for (const auto& path : pending) {
        if (headless) success = afterhours::graphics::capture_frame(path) && success;
        else afterhours::graphics::take_screenshot(path.c_str());
        success = std::filesystem::is_regular_file(path) && success;
    }
    pending.clear();
    return success;
}
}
