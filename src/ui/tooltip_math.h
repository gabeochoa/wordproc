#pragma once

// Pure tooltip geometry: no backend, no allocations. Shared by the
// toolbar tooltip renderer and the unit tests.

namespace tooltip_math {

// Win95 tooltips appear after a short hover, not instantly.
inline constexpr float HOVER_DELAY_SECONDS = 0.5f;

// Padding inside the tooltip box: ~4px horizontal, ~2px vertical.
inline constexpr float PADDING_X = 4.0f;
inline constexpr float PADDING_Y = 2.0f;

inline float tooltipWidth(float textWidth) { return textWidth + 2.0f * PADDING_X; }
inline float tooltipHeight(float fontSize) { return fontSize + 2.0f * PADDING_Y; }
inline bool delayElapsed(float hoverSeconds) { return hoverSeconds >= HOVER_DELAY_SECONDS; }

}  // namespace tooltip_math
