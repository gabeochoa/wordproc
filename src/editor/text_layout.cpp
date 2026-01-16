#include "text_layout.h"

#include <algorithm>

std::vector<WrappedLine> layoutWrappedLines(const TextBuffer &buffer,
                                            std::size_t max_columns) {
    std::vector<WrappedLine> result;
    if (max_columns == 0) {
        return result;
    }

    const auto &lines = buffer.lines();
    for (std::size_t row = 0; row < lines.size(); ++row) {
        const std::string &line = lines[row];
        if (line.empty()) {
            result.push_back(WrappedLine{row, 0, 0, ""});
            continue;
        }

        std::size_t start = 0;
        while (start < line.size()) {
            std::size_t len = std::min(max_columns, line.size() - start);
            result.push_back(
                WrappedLine{row, start, len, line.substr(start, len)});
            start += len;
        }
    }

    return result;
}

// SoA layout implementation - avoids string copies
// Uses parallel arrays for better cache locality during iteration
LayoutResult layoutWrappedLinesSoA(const TextBuffer &buffer,
                                   std::size_t max_columns) {
    LayoutResult result;
    if (max_columns == 0) {
        return result;
    }

    const auto &lines = buffer.lines();

    // Pre-compute total wrapped lines for reservation
    std::size_t estimated_count = 0;
    for (const auto &line : lines) {
        estimated_count +=
            line.empty() ? 1 : (line.size() + max_columns - 1) / max_columns;
    }
    result.reserve(estimated_count);

    for (std::size_t row = 0; row < lines.size(); ++row) {
        const std::string &line = lines[row];
        if (line.empty()) {
            result.push_back(row, 0, 0);
            continue;
        }

        std::size_t start = 0;
        while (start < line.size()) {
            std::size_t len = std::min(max_columns, line.size() - start);
            result.push_back(row, start, len);
            start += len;
        }
    }

    return result;
}

// ============================================================================
// RenderCache implementation - caches layout to avoid per-frame recomputation
// ============================================================================

bool RenderCache::needsRebuild(std::uint64_t buffer_version, int font_size,
                               int text_area_width, int text_area_height,
                               int line_height) const {
    if (buffer_version != cached_buffer_version_) return true;
    if (font_size != cached_font_size_) return true;
    if (text_area_width != cached_text_area_width_) return true;
    if (text_area_height != cached_text_area_height_) return true;
    if (line_height != cached_line_height_) return true;

    // Cache hit
    cache_hit_count_++;
    return false;
}

void RenderCache::rebuild(const TextBuffer &buffer,
                          std::uint64_t buffer_version, int font_size,
                          int text_area_x, int text_area_y, int text_area_width,
                          int text_area_height, int line_height,
                          int text_padding) {
    rebuild_count_++;
    visible_lines_.clear();

    // Store cache parameters
    cached_buffer_version_ = buffer_version;
    cached_font_size_ = font_size;
    cached_text_area_width_ = text_area_width;
    cached_text_area_height_ = text_area_height;
    cached_line_height_ = line_height;

    std::size_t lineCount = buffer.lineCount();
    if (lineCount == 0) {
        first_visible_row_ = 0;
        last_visible_row_ = 0;
        return;
    }

    // Calculate how many lines fit in the visible area
    int available_height = text_area_height - 2 * text_padding;
    std::size_t maxVisibleLines =
        (available_height > 0)
            ? static_cast<std::size_t>(available_height / line_height) + 1
            : lineCount;

    // Reserve space for visible lines
    visible_lines_.reserve(std::min(maxVisibleLines, lineCount));

    int y = text_area_y + text_padding;
    int max_y = text_area_y + text_area_height;

    first_visible_row_ = 0;  // TODO: Add scrolling support
    last_visible_row_ = 0;

    for (std::size_t row = 0; row < lineCount && y < max_y; ++row) {
        LineSpan span = buffer.lineSpan(row);

        CachedLine cached;
        cached.source_row = row;
        cached.start_column = 0;  // No horizontal scrolling yet
        cached.y_position = y;

        // Pre-cache the line text (only once per rebuild, not per frame)
        if (span.length > 0) {
            cached.text = buffer.lineString(row);
        }

        visible_lines_.push_back(std::move(cached));
        last_visible_row_ = row;

        y += line_height;
    }
}
