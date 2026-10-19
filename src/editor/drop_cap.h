#pragma once

// Format > Drop Cap: pure helpers. A paragraph's drop cap is
// dropCapLines lines tall (0 = off, 2-4 = dropped). The first
// glyph draws at lineHeight * dropCapLines, and the following
// dropCapLines - 1 WRAPPED lines of THE SAME PARAGRAPH indent past
// its width — the wrap indent the renderer applies via
// indentedBaseX, so caret, selection, and squiggles follow it
// like the outline collapse shift.
//
// Scoping (Batch 11 fix round): the indent is keyed by
// (paragraph, line-index-within-paragraph), never by "the N rows
// after a drop-cap row". The old covering-row scan treated the
// next buffer rows as the cap paragraph's continuations, but
// each buffer row IS its own paragraph (the renderer draws one
// buffer line per visual row, no wrapping), so a drop cap on
// paragraph 1 indented paragraphs 2 and 3 as well. Now no
// following buffer row ever inherits another paragraph's cap:
// a wrapped line indents only when its own paragraph carries
// the cap and its index within that paragraph is in [1, N).
// Wrapped lines are the text_layout splits of one paragraph
// (layoutWrappedLines); a paragraph that does not wrap has no
// continuation lines and no indent.
//
// The mouse hit-test does not account for paragraph indents at all
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

// One row's drop-cap state, for coveringDropCapParagraph.
struct RowInfo {
    bool hasDropCap = false;
    int dropCapLines = kMinLines;
};

// Does the wrapped line at `lineIndexWithinParagraph` (0 = the
// line carrying the dropped glyph) of a paragraph with this
// dropCapLines indent past the glyph? Only the first N - 1
// FOLLOWING lines of the same paragraph: indices [1, N).
inline bool wrapIndentApplies(int dropCapLines,
                              std::size_t lineIndexWithinParagraph) {
    if (!isEnabled(dropCapLines)) return false;
    return lineIndexWithinParagraph >= 1 &&
           lineIndexWithinParagraph <
               static_cast<std::size_t>(dropCapLines);
}

// The drop-cap paragraph covering a wrapped line: `paragraphRow`
// itself when that paragraph carries a cap whose span reaches
// this wrapped line, else -1. A wrapped line of any OTHER
// paragraph is never covered, however close it sits after a
// drop-cap paragraph — that was the scope leak.
inline int coveringDropCapParagraph(const std::vector<RowInfo>& rows,
                                    std::size_t paragraphRow,
                                    std::size_t lineIndexWithinParagraph) {
    if (paragraphRow >= rows.size()) return -1;
    const RowInfo& info = rows[paragraphRow];
    if (!info.hasDropCap) return -1;
    if (!wrapIndentApplies(info.dropCapLines, lineIndexWithinParagraph)) {
        return -1;
    }
    return static_cast<int>(paragraphRow);
}

}  // namespace drop_cap
