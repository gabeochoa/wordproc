#include "text_layout.h"

#include <algorithm>

#include "drop_cap.h"

// Row -> y mapping for the renderer's one-visual-row-per-paragraph
// model. Each paragraph's band is reservedRows(cap, 1) * lineHeight:
// a 1-line paragraph with a 3-line cap holds 3 rows of y, so a
// following row starts 3 rows below the cap paragraph's top.
int yOffsetForRow(const TextBuffer& buffer, std::size_t firstRow,
                  std::size_t targetRow, int lineHeight) {
    if (lineHeight <= 0 || targetRow <= firstRow) return 0;
    const std::size_t last =
        std::min(targetRow, buffer.lineCount());
    int offset = 0;
    for (std::size_t row = firstRow; row < last; ++row) {
        offset += static_cast<int>(
                      drop_cap::reservedRows(buffer.lineDropCapLines(row),
                                             1)) *
                  lineHeight;
    }
    return offset;
}

std::size_t rowForYOffset(const TextBuffer& buffer, std::size_t firstRow,
                          float relY, int lineHeight) {
    const std::size_t lineCount = buffer.lineCount();
    if (lineCount == 0) return 0;
    if (firstRow >= lineCount) return lineCount - 1;
    if (lineHeight <= 0 || relY <= 0.0f) return firstRow;
    float cursor = 0.0f;
    for (std::size_t row = firstRow; row < lineCount; ++row) {
        cursor += static_cast<float>(
                      drop_cap::reservedRows(buffer.lineDropCapLines(row),
                                             1)) *
                  static_cast<float>(lineHeight);
        if (relY < cursor) return row;
    }
    return lineCount - 1;
}

std::size_t rowForYOffsetInList(const TextBuffer& buffer,
                                const std::vector<std::size_t>& rows,
                                std::size_t firstVisibleIndex, float relY,
                                int lineHeight) {
    if (rows.empty()) return 0;
    if (firstVisibleIndex >= rows.size()) return rows.size() - 1;
    if (lineHeight <= 0 || relY <= 0.0f) return firstVisibleIndex;
    float cursor = 0.0f;
    for (std::size_t i = firstVisibleIndex; i < rows.size(); ++i) {
        cursor += static_cast<float>(
                      drop_cap::reservedRows(
                          buffer.lineDropCapLines(rows[i]), 1)) *
                  static_cast<float>(lineHeight);
        if (relY < cursor) return i;
    }
    return rows.size() - 1;
}

std::vector<WrappedLine> layoutWrappedLines(const TextBuffer &buffer,
                                            std::size_t max_columns) {
    std::vector<WrappedLine> result;
    if (max_columns == 0) {
        return result;
    }

    std::size_t numLines = buffer.lineCount();
    for (std::size_t row = 0; row < numLines; ++row) {
        LineSpan span = buffer.lineSpan(row);
        if (span.length == 0) {
            result.push_back(WrappedLine{row, 0, 0, ""});
            continue;
        }

        // Only materialize the string for the WrappedLine.text field
        std::string line = buffer.lineString(row);
        std::size_t start = 0;
        while (start < span.length) {
            std::size_t len = std::min(max_columns, span.length - start);
            result.push_back(
                WrappedLine{row, start, len, line.substr(start, len)});
            start += len;
        }
    }

    return result;
}

// SoA layout implementation - zero allocation
// Uses parallel arrays for better cache locality during iteration
LayoutResult layoutWrappedLinesSoA(const TextBuffer &buffer,
                                   std::size_t max_columns) {
    LayoutResult result;
    if (max_columns == 0) {
        return result;
    }

    std::size_t numLines = buffer.lineCount();

    // Pre-compute total wrapped lines for reservation
    std::size_t estimated_count = 0;
    for (std::size_t row = 0; row < numLines; ++row) {
        LineSpan span = buffer.lineSpan(row);
        estimated_count +=
            span.length == 0 ? 1 : (span.length + max_columns - 1) / max_columns;
    }
    result.reserve(estimated_count);

    for (std::size_t row = 0; row < numLines; ++row) {
        LineSpan span = buffer.lineSpan(row);
        if (span.length == 0) {
            result.push_back(row, 0, 0);
            continue;
        }

        std::size_t start = 0;
        while (start < span.length) {
            std::size_t len = std::min(max_columns, span.length - start);
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
                          int /*text_area_x*/, int text_area_y, int text_area_width,
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

        // A drop-cap paragraph reserves its cap span (same formula
        // as renderTextBuffer), so the next row's cached top sits
        // below the glyph.
        y += static_cast<int>(
                 drop_cap::reservedRows(buffer.lineDropCapLines(row), 1)) *
             line_height;
    }
}
