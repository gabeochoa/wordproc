#pragma once

// Pure scroll math: no backend, no allocations. Shared by the navigation
// system (wheel input), the render system (scrollbar thumb geometry) and
// the unit tests, so all three agree on offset <-> thumb mapping.

namespace scroll_math {

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
