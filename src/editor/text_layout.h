#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "text_buffer.h"

// SoA layout: WrappedLine stores spans (offsets) rather than copied strings
// This avoids string allocations during layout computation
struct WrappedLine {
    std::size_t source_row = 0;    // Which line in the buffer
    std::size_t start_column = 0;  // Start offset within that line
    std::size_t length = 0;        // Character count for this wrapped segment

    // For backwards compatibility, we still store text
    // TODO: Phase 2 - remove this and use string_view at render time
    std::string text;
};

// SoA-style layout result: parallel arrays for better cache locality
struct LayoutResult {
    std::vector<std::size_t> source_rows;    // SoA: which buffer line
    std::vector<std::size_t> start_columns;  // SoA: start offset in line
    std::vector<std::size_t> lengths;        // SoA: segment length

    std::size_t size() const { return source_rows.size(); }
    bool empty() const { return source_rows.empty(); }

    void clear() {
        source_rows.clear();
        start_columns.clear();
        lengths.clear();
    }

    void reserve(std::size_t n) {
        source_rows.reserve(n);
        start_columns.reserve(n);
        lengths.reserve(n);
    }

    void push_back(std::size_t row, std::size_t col, std::size_t len) {
        source_rows.push_back(row);
        start_columns.push_back(col);
        lengths.push_back(len);
    }
};

// Original API for backwards compatibility
std::vector<WrappedLine> layoutWrappedLines(const TextBuffer& buffer,
                                            std::size_t max_columns);

// New SoA API - avoids string copies, better for large documents
LayoutResult layoutWrappedLinesSoA(const TextBuffer& buffer,
                                   std::size_t max_columns);

// Cached line data for rendering - avoids per-frame allocations
struct CachedLine {
    std::size_t source_row = 0;
    std::size_t start_column = 0;
    std::string text;    // Pre-computed text (avoids per-frame alloc)
    int y_position = 0;  // Pre-computed y position in pixels
};

// Render cache - stores pre-computed layout for efficient frame rendering
// Invalidate when buffer changes, font size changes, or window resizes
class RenderCache {
   public:
    RenderCache() = default;

    // Check if cache needs rebuild based on buffer version and settings
    bool needsRebuild(std::uint64_t buffer_version, int font_size,
                      int text_area_width, int text_area_height,
                      int line_height) const;

    // Rebuild the cache from the buffer
    void rebuild(const TextBuffer& buffer, std::uint64_t buffer_version,
                 int font_size, int text_area_x, int text_area_y,
                 int text_area_width, int text_area_height, int line_height,
                 int text_padding);

    // Access cached lines for rendering (only visible lines)
    const std::vector<CachedLine>& visibleLines() const {
        return visible_lines_;
    }

    // Get first visible line's source row (for scrolling calculations)
    std::size_t firstVisibleRow() const { return first_visible_row_; }
    std::size_t lastVisibleRow() const { return last_visible_row_; }

    // Performance stats
    std::size_t rebuildCount() const { return rebuild_count_; }
    std::size_t cacheHitCount() const { return cache_hit_count_; }
    void resetStats() {
        rebuild_count_ = 0;
        cache_hit_count_ = 0;
    }

   private:
    std::vector<CachedLine> visible_lines_;
    std::uint64_t cached_buffer_version_ = 0;
    int cached_font_size_ = 0;
    int cached_text_area_width_ = 0;
    int cached_text_area_height_ = 0;
    int cached_line_height_ = 0;
    std::size_t first_visible_row_ = 0;
    std::size_t last_visible_row_ = 0;
    std::size_t rebuild_count_ = 0;
    mutable std::size_t cache_hit_count_ = 0;
};
