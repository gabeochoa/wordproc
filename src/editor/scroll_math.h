#pragma once

#include <cstddef>

// Pure scroll math: no backend, no allocations. Shared by the navigation
// system (wheel input), the render system (scrollbar thumb geometry) and
// the unit tests, so all three agree on offset <-> thumb mapping.

namespace scroll_math {

// Width of the vertical scrollbar in pixels (Win95: 16px).
inline constexpr float SCROLLBAR_WIDTH = 16.0f;

// Press-hold auto-repeat for the scrollbar arrow buttons: first repeat
// after ~400ms, then every ~50ms while held. Pure functions over the
// (dir, next) pair stored on ScrollComponent so unit tests can drive
// them with fake clocks.
inline constexpr double ARROW_REPEAT_INITIAL_DELAY = 0.4;
inline constexpr double ARROW_REPEAT_INTERVAL = 0.05;

inline void arrowRepeatPress(int& dir, double& next, int d, double now) {
    dir = d;
    next = now + ARROW_REPEAT_INITIAL_DELAY;
}

inline bool arrowRepeatDue(int dir, double& next, double now) {
    if (dir == 0 || now < next) return false;
    next = now + ARROW_REPEAT_INTERVAL;
    return true;
}

inline void arrowRepeatRelease(int& dir) { dir = 0; }

// Fold one discrete wheel event into the sub-line remainder and return the
// whole lines to apply this frame. Small deltas accumulate across frames
// (ten 0.1-notch events sum to 3 lines); a large flick applies at once.
// wheelMove follows raylib sign: positive = wheel up.
inline int accumulateWheel(float& remainder, float wheelMove,
                           float linesPerNotch = 3.0f) {
    remainder += -wheelMove * linesPerNotch;
    int lines = static_cast<int>(remainder);  // truncates toward zero
    remainder -= static_cast<float>(lines);
    // Snap float dust (e.g. ten 0.1-notch events) back to an exact zero.
    if (remainder > -1e-6f && remainder < 1e-6f) remainder = 0.0f;
    return lines;
}

// Win95 vertical scrollbar geometry. The track runs between the two
// 16px arrow buttons; the thumb is sized viewport/total with a minimum
// size, and positioned proportionally to offset/maxScroll.
struct ScrollbarGeometry {
    float upArrowY = 0.0f;
    float arrowHeight = 16.0f;
    float trackY = 0.0f;
    float trackHeight = 0.0f;
    float thumbY = 0.0f;
    float thumbHeight = 0.0f;
    bool thumbVisible = false;
};

inline ScrollbarGeometry scrollbarGeometry(float barY, float barHeight,
                                           int totalLines, int visibleLines,
                                           int maxScroll, int offset,
                                           float minThumbHeight = 20.0f) {
    ScrollbarGeometry g;
    g.upArrowY = barY;
    g.trackY = barY + g.arrowHeight;
    g.trackHeight = barHeight - 2.0f * g.arrowHeight;
    if (g.trackHeight < 0.0f) g.trackHeight = 0.0f;
    g.thumbVisible = totalLines > visibleLines && g.trackHeight > 0.0f;
    if (!g.thumbVisible) {
        g.thumbY = g.trackY;
        g.thumbHeight = g.trackHeight;
        return g;
    }
    float ratio = static_cast<float>(visibleLines) / static_cast<float>(totalLines);
    g.thumbHeight = g.trackHeight * ratio;
    if (g.thumbHeight < minThumbHeight) g.thumbHeight = minThumbHeight;
    if (g.thumbHeight > g.trackHeight) g.thumbHeight = g.trackHeight;
    float scrollRatio = maxScroll > 0 ? static_cast<float>(offset) / static_cast<float>(maxScroll) : 0.0f;
    if (scrollRatio < 0.0f) scrollRatio = 0.0f;
    if (scrollRatio > 1.0f) scrollRatio = 1.0f;
    g.thumbY = g.trackY + (g.trackHeight - g.thumbHeight) * scrollRatio;
    return g;
}

// --- Print Preview pagination (estimate — see below) ---
// Wordproc has no real pagination: the renderer draws buffer lines top
// to bottom and clips at the text-area bottom, and the PDF exporter
// writes a single page. Print Preview therefore ESTIMATES page breaks
// from geometry: the number of rendered lines that fit in one page's
// text height. Headings (larger fonts), paragraph spacing, and explicit
// page-break indicators shift the true breaks, so "Page X of Y" is an
// estimate, not a paginated layout. These pure helpers keep the
// estimate identical in the preview toolbar, the renderer stride, and
// the unit tests.
inline int previewLinesPerPage(float textHeightPx, float lineHeightPx) {
    if (textHeightPx <= 0.0f || lineHeightPx <= 0.0f) return 1;
    int lines = static_cast<int>(textHeightPx / lineHeightPx);
    return lines < 1 ? 1 : lines;
}

inline int previewPageCount(std::size_t totalLines, int linesPerPage) {
    if (linesPerPage <= 0) linesPerPage = 1;
    if (totalLines == 0) return 1;
    return static_cast<int>((totalLines + static_cast<std::size_t>(linesPerPage) - 1) /
                            static_cast<std::size_t>(linesPerPage));
}

// First buffer line shown on a preview page (0-based page index).
inline int previewFirstLine(int previewPage, int linesPerPage) {
    if (previewPage < 0) previewPage = 0;
    if (linesPerPage < 1) linesPerPage = 1;
    return previewPage * linesPerPage;
}

inline int previewPageClamp(int previewPage, int pageCount) {
    if (previewPage < 0) return 0;
    if (pageCount < 1) pageCount = 1;
    if (previewPage > pageCount - 1) return pageCount - 1;
    return previewPage;
}

// Inverse of scrollbarGeometry: thumb top (in track coords) -> scroll offset.
inline int offsetForThumbY(float thumbY, float trackY, float trackHeight,
                           float thumbHeight, int maxScroll) {
    float travel = trackHeight - thumbHeight;
    if (travel <= 0.0f || maxScroll <= 0) return 0;
    float ratio = (thumbY - trackY) / travel;
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;
    return static_cast<int>(ratio * static_cast<float>(maxScroll) + 0.5f);
}

}  // namespace scroll_math
