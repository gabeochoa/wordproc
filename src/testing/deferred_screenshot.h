#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "../external.h"

// Screenshots must be captured AFTER end_drawing(): reading back the
// Metal render texture mid-frame (before the pass commits) deadlocks in
// metal_copy_to_shared's waitUntilCompleted. Producers queue a request;
// the main loop flushes once per frame after end_drawing().
namespace deferred_screenshot {

inline std::vector<std::filesystem::path>& queue() {
    static std::vector<std::filesystem::path> q;
    return q;
}

inline void request(const std::filesystem::path& path) {
    std::filesystem::create_directories(path.parent_path());
    queue().push_back(path);
}

inline void flush() {
    for (auto& path : queue()) {
        if (!afterhours::graphics::capture_frame(path))
            afterhours::graphics::take_screenshot(path.string().c_str());
    }
    queue().clear();
}

}  // namespace deferred_screenshot
