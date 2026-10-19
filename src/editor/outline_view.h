#pragma once

// View > Outline mode helpers (Word 95 parity).
// Pure functions over paragraph styles so the visible-rows mapping
// is unit-testable; the renderer, the click hit-test, and the
// Outline toggle all share these (no second copy of the mapping).
// Outline shows ONLY Title / Subtitle / Heading 1-6 paragraphs:
// body (Normal) paragraphs are not rendered, scroll counts visible
// heading rows, and a click on visible row V lands on the heading's
// real buffer row.

#include <cstddef>
#include <optional>
#include <unordered_set>
#include <utility>
#include <vector>

#include "document_settings.h"

namespace outline_view {

// Outline level of a style, matching TextBuffer::getOutline:
// Title 0, Subtitle/Heading 1 = 1, Heading 2 = 2, ..., Heading 6
// = 6. Normal is not an outline row: -1.
inline int outlineLevel(ParagraphStyle style) {
    switch (style) {
        case ParagraphStyle::Title: return 0;
        case ParagraphStyle::Subtitle:
        case ParagraphStyle::Heading1: return 1;
        case ParagraphStyle::Heading2: return 2;
        case ParagraphStyle::Heading3: return 3;
        case ParagraphStyle::Heading4: return 4;
        case ParagraphStyle::Heading5: return 5;
        case ParagraphStyle::Heading6: return 6;
        case ParagraphStyle::Normal:
        case ParagraphStyle::Count:
        default: return -1;
    }
}

inline bool isHeadingStyle(ParagraphStyle style) {
    return outlineLevel(style) >= 0;
}

// Indent of a heading row in the outline: (level - 1) * 24px, so
// Title and Heading 1 sit at 0, Heading 2 at 24, Heading 3 at 48...
inline int outlineIndentPx(ParagraphStyle style) {
    int level = outlineLevel(style);
    if (level <= 1) return 0;
    return (level - 1) * 24;
}

// Real buffer rows of the outline rows, in document order, for a
// per-row style list (index = buffer row).
inline std::vector<std::size_t> outlineVisibleRows(
    const std::vector<ParagraphStyle>& styles) {
    std::vector<std::size_t> rows;
    for (std::size_t i = 0; i < styles.size(); ++i) {
        if (isHeadingStyle(styles[i])) rows.push_back(i);
    }
    return rows;
}

// Promote / Demote one step on the caret paragraph's heading level.
// Promote clamps at the top: Heading 1 stays Heading 1 (Title is
// a separate style, not a promoted Heading 1; Subtitle promotes to
// Title). Demote clamps at Heading 6. Normal is not an outline row
// and is unchanged. Demote is the inverse of Promote everywhere
// except Title -> Subtitle -> Title (promoting Subtitle lands on
// Title, demoting Title lands back on Subtitle) and Heading 1
// (promote is a no-op there).
inline ParagraphStyle promoteParagraphStyle(ParagraphStyle style) {
    switch (style) {
        case ParagraphStyle::Title: return ParagraphStyle::Title;
        case ParagraphStyle::Subtitle: return ParagraphStyle::Title;
        case ParagraphStyle::Heading1: return ParagraphStyle::Heading1;
        case ParagraphStyle::Heading2: return ParagraphStyle::Heading1;
        case ParagraphStyle::Heading3: return ParagraphStyle::Heading2;
        case ParagraphStyle::Heading4: return ParagraphStyle::Heading3;
        case ParagraphStyle::Heading5: return ParagraphStyle::Heading4;
        case ParagraphStyle::Heading6: return ParagraphStyle::Heading5;
        case ParagraphStyle::Normal:
        case ParagraphStyle::Count:
        default: return style;
    }
}

inline ParagraphStyle demoteParagraphStyle(ParagraphStyle style) {
    switch (style) {
        case ParagraphStyle::Title: return ParagraphStyle::Subtitle;
        case ParagraphStyle::Subtitle: return ParagraphStyle::Heading2;
        case ParagraphStyle::Heading1: return ParagraphStyle::Heading2;
        case ParagraphStyle::Heading2: return ParagraphStyle::Heading3;
        case ParagraphStyle::Heading3: return ParagraphStyle::Heading4;
        case ParagraphStyle::Heading4: return ParagraphStyle::Heading5;
        case ParagraphStyle::Heading5: return ParagraphStyle::Heading6;
        case ParagraphStyle::Heading6: return ParagraphStyle::Heading6;
        case ParagraphStyle::Normal:
        case ParagraphStyle::Count:
        default: return style;
    }
}

// End (exclusive) of the block headed by `start`: the heading at
// start plus its subordinate paragraphs (deeper headings and any
// body rows), stopping at the next heading whose level is <=
// the start heading's level, or the end of the document. Body
// rows in Outline are already hidden, but they belong to the
// block (a Move carries them with their heading).
inline std::size_t outlineBlockEnd(
    const std::vector<ParagraphStyle>& styles, std::size_t start) {
    if (start >= styles.size() || !isHeadingStyle(styles[start])) {
        return start;
    }
    int level = outlineLevel(styles[start]);
    for (std::size_t i = start + 1; i < styles.size(); ++i) {
        if (isHeadingStyle(styles[i]) && outlineLevel(styles[i]) <= level) {
            return i;
        }
    }
    return styles.size();
}

// Whether the heading at `row` has subordinate headings (a
// deeper heading inside its block). Only such headings get a
// +/- collapse box: collapsing a heading with no subordinate
// headings would hide nothing visible in Outline.
inline bool outlineHasSubordinateHeadings(
    const std::vector<ParagraphStyle>& styles, std::size_t row) {
    if (row >= styles.size() || !isHeadingStyle(styles[row])) return false;
    int level = outlineLevel(styles[row]);
    std::size_t end = outlineBlockEnd(styles, row);
    for (std::size_t i = row + 1; i < end; ++i) {
        if (isHeadingStyle(styles[i]) && outlineLevel(styles[i]) > level) {
            return true;
        }
    }
    return false;
}

// Sibling blocks for Move Up / Down: the previous / next heading
// at the SAME level as the heading at `row`, with that heading's
// whole block. Returns nullopt when there is no same-level
// sibling in that direction (a heading with no previous
// same-level sibling does not move past its parent heading, and
// a last sibling does not move past its parent's next heading:
// the blocks are adjacent in both directions, so a Move swaps
// the two contiguous ranges [prevStart, row) and [row, blockEnd)
// or [row, blockEnd) and [blockEnd, nextEnd)).
inline std::optional<std::pair<std::size_t, std::size_t>>
outlinePrevSiblingBlock(const std::vector<ParagraphStyle>& styles,
                        std::size_t row) {
    if (row >= styles.size() || !isHeadingStyle(styles[row])) {
        return std::nullopt;
    }
    int level = outlineLevel(styles[row]);
    for (std::size_t i = row; i-- > 0;) {
        if (isHeadingStyle(styles[i]) && outlineLevel(styles[i]) == level) {
            return std::pair{i, outlineBlockEnd(styles, i)};
        }
        // A heading shallower than the caret heading is its
        // ancestor: no same-level sibling can sit beyond it.
        if (isHeadingStyle(styles[i]) && outlineLevel(styles[i]) < level) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

inline std::optional<std::pair<std::size_t, std::size_t>>
outlineNextSiblingBlock(const std::vector<ParagraphStyle>& styles,
                        std::size_t row) {
    if (row >= styles.size() || !isHeadingStyle(styles[row])) {
        return std::nullopt;
    }
    int level = outlineLevel(styles[row]);
    std::size_t end = outlineBlockEnd(styles, row);
    for (std::size_t i = end; i < styles.size(); ++i) {
        if (isHeadingStyle(styles[i]) && outlineLevel(styles[i]) == level) {
            return std::pair{i, outlineBlockEnd(styles, i)};
        }
        // A heading shallower than the caret heading ends the
        // parent block: no same-level sibling can sit beyond it.
        if (isHeadingStyle(styles[i]) && outlineLevel(styles[i]) < level) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

// Visible outline rows after the Outlining toolbar's filters:
// Show Level N (maxLevel > 0) hides headings deeper than N, and
// a collapsed heading hides its subordinate headings (but not
// itself). `maxLevel == 0` means All Levels. A heading hidden by
// Show Level still ends the block of a shallower heading in the
// stack sense below, because the stack pops on level, not on
// visibility -- a H2 after a hidden H3 is not a subordinate of
// that H3.
inline std::vector<std::size_t> outlineVisibleRowsFiltered(
    const std::vector<ParagraphStyle>& styles, int maxLevel,
    const std::unordered_set<std::size_t>& collapsedRows) {
    std::vector<std::size_t> rows;
    // Open headings, shallowest first: (level, collapsed).
    std::vector<std::pair<int, bool>> stack;
    for (std::size_t i = 0; i < styles.size(); ++i) {
        if (!isHeadingStyle(styles[i])) continue;
        int level = outlineLevel(styles[i]);
        while (!stack.empty() && stack.back().first >= level) {
            stack.pop_back();
        }
        bool hidden = false;
        for (const auto& open : stack) {
            if (open.second) hidden = true;
        }
        if (maxLevel > 0 && level > maxLevel) hidden = true;
        if (!hidden) rows.push_back(i);
        stack.emplace_back(level, collapsedRows.count(i) > 0);
    }
    return rows;
}

// Collapse-box geometry, shared by the renderer (which draws
// the box) and the click hit-test (which toggles it): a 12px
// box at the heading's indented start. EVERY heading's text is
// shifted right by the box + a 4px gap (a reserved collapse
// gutter), whether or not that heading gets a box -- a
// box-only shift would eat 16px of a child's 24px indent.
inline constexpr int kOutlineCollapseBoxSize = 12;
inline constexpr int kOutlineCollapseShift =
    kOutlineCollapseBoxSize + 4;

inline int outlineCollapseBoxX(int indentedBaseX) {
    return indentedBaseX;
}

// Map a visible-row index to its real buffer row, clamping a
// past-the-end index (e.g. from an approximate hit-test row) to the
// last heading. Returns lineCount (no such row) when there are no
// outline rows.
inline std::size_t outlineRealRowForVisible(
    const std::vector<std::size_t>& visibleRows, std::size_t visibleIndex,
    std::size_t lineCount) {
    if (visibleRows.empty()) return lineCount;
    if (visibleIndex >= visibleRows.size()) {
        return visibleRows.back();
    }
    return visibleRows[visibleIndex];
}

}  // namespace outline_view
