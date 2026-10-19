#pragma once

// App wiring for MeasureCache: the one cached front end for
// `afterhours::graphics::measure_text` (the active-font ink-bounds
// measure the document renderer uses for squiggle, selection, caret,
// and line-width x positions).
//
// The font key folds in everything that measure depends on besides
// (text, size): the active fontstash id and the DPI scale. A font
// reload mints a new fontstash id and a display move changes the
// scale, so both land in a different key rather than a stale hit.
// Buffer edits need no invalidation: widths are keyed on the text
// itself, so an edited line measures under its new text.
//
// One process-wide cache: the render path is single-threaded, and
// every caller measures through the same active font.

#include <cstdint>
#include <string>
#include <string_view>

#include "../editor/measure_cache.h"

#ifdef AFTER_HOURS_USE_METAL
#include <afterhours/src/backends/sokol/metal_state.h>
#endif

namespace ecs {

// Opaque font key for the current `graphics::measure_text` inputs.
// 0 when the backend does not expose an active font (never the case
// on the Metal build; the fallback key is still consistent within a
// run because the active font never changes mid-frame).
inline std::uint64_t renderMeasureFontKey() {
#ifdef AFTER_HOURS_USE_METAL
    const int fontId = afterhours::graphics::metal_detail::g_active_font;
    const float dpi = afterhours::graphics::metal_detail::dpi_scale();
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(fontId)) << 32) |
           static_cast<std::uint64_t>(dpi * 1024.0f);
#else
    return 0;
#endif
}

inline int cachedMeasureText(std::string_view text, int fontSize) {
    static MeasureCache cache{[](std::string_view measured, int size) {
        return afterhours::graphics::measure_text(std::string(measured).c_str(), size);
    }};
    return cache.measure(text, fontSize, renderMeasureFontKey());
}

}  // namespace ecs
