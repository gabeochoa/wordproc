#pragma once

// Format > Drop Cap: pure helpers. A paragraph's drop cap is
// dropCapLines lines tall (0 = off, 2-4 = dropped). The first
// glyph draws at lineHeight * dropCapLines, and the following
// dropCapLines - 1 rows indent past its width — the wrap indent
// the renderer applies via indentedBaseX, so caret, selection,
// and squiggles follow it like the outline collapse shift.
//
// This renderer draws one buffer line per visual row (no text
// wrapping), so the "following lines" are the next buffer rows:
// the wrap indent applies to the rows a drop-cap paragraph's
// continuation lines would occupy in a wrapping renderer. The
// mouse hit-test does not account for paragraph indents at all
// (pre-existing approximation), so it does not account for this
// indent either.

#include <algorithm>
#include <cstddef>
#include <vector>

namespace drop_cap {

inline constexpr int kOff = 0;
inline constexpr int kMinLines = 2;
inline constexpr int kMaxLines = 4;

// Clamp a dialog value to the supported span. 0/1 (off) stay 0.
inline int clampLines(int lines) {
    if (lines < kMinLines) return kOff;
    return std::min(lines, kMaxLines);
}

inline bool isEnabled(int lines) { return lines >= kMinLines; }

// One row's drop-cap state, for coveringDropCapRow.
struct RowInfo {
    bool hasDropCap = false;
    int dropCapLines = kMinLines;
};

// The drop-cap paragraph covering `row`, or -1: scan back from
// the row above (at most kMaxLines - 1 rows: a span of N covers
// itself + N - 1 following rows) for the NEAREST drop-cap row
// whose span reaches `row`. A nearer drop-cap row that does not
// reach `row` does not block an earlier one that does.
inline int coveringDropCapRow(const std::vector<RowInfo>& rows,
                              std::size_t row) {
    if (row == 0 || rows.empty()) return -1;
    for (std::size_t cap = row; cap-- > 0;) {
        if (row - cap > static_cast<std::size_t>(kMaxLines - 1)) break;
        if (cap >= rows.size()) continue;
        const RowInfo& info = rows[cap];
        if (!info.hasDropCap) continue;
        if (row - cap < static_cast<std::size_t>(
                            std::max(kMinLines, info.dropCapLines))) {
            return static_cast<int>(cap);
        }
    }
    return -1;
}

}  // namespace drop_cap
