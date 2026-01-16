#pragma once

#include "../../vendor/afterhours/src/plugins/ui.h"

namespace ui_imm {

// Win95 color palette for Afterhours Theme
namespace win95_colors {
constexpr afterhours::Color WINDOW_BG = {192, 192, 192, 255};
constexpr afterhours::Color TITLE_BAR = {0, 0, 128, 255};
constexpr afterhours::Color TEXT = {0, 0, 0, 255};
constexpr afterhours::Color TEXT_WHITE = {255, 255, 255, 255};
constexpr afterhours::Color TEXT_DISABLED = {128, 128, 128, 255};
constexpr afterhours::Color TEXT_AREA = {255, 255, 255, 255};
constexpr afterhours::Color HIGHLIGHT = {0, 0, 128, 255};
constexpr afterhours::Color BUTTON_FACE = {192, 192, 192, 255};
constexpr afterhours::Color BORDER_LIGHT = {255, 255, 255, 255};
constexpr afterhours::Color BORDER_DARK = {128, 128, 128, 255};
constexpr afterhours::Color ERROR_COLOR = {255, 0, 0, 255};
}  // namespace win95_colors

// Create Win95 theme for Afterhours UI
inline afterhours::ui::Theme createWin95Theme() {
    afterhours::ui::Theme theme;

    // Font colors
    theme.font = win95_colors::TEXT_WHITE;
    theme.darkfont = win95_colors::TEXT;
    theme.font_muted = win95_colors::TEXT_DISABLED;

    // Background colors
    theme.background = win95_colors::WINDOW_BG;
    theme.surface = win95_colors::TEXT_AREA;

    // UI element colors
    theme.primary = win95_colors::TITLE_BAR;
    theme.secondary = win95_colors::BUTTON_FACE;
    theme.accent = win95_colors::HIGHLIGHT;
    theme.error = win95_colors::ERROR_COLOR;

    // Win95 has sharp corners (no rounded corners)
    theme.rounded_corners = std::bitset<4>();
    theme.roundness = 0.0f;
    theme.segments = 0;

    return theme;
}

// Initialize the Win95 theme globally
inline void initWin95Theme() {
    auto& themeDefaults = afterhours::ui::imm::ThemeDefaults::get();
    themeDefaults.set_theme(createWin95Theme());
}

// Initialize the Win95 theme (call after Preload::make_singleton())
// Note: Preload::make_singleton() already sets up all singletons.
// This function just applies the Win95 theme.
inline void initUIContext([[maybe_unused]] int screenWidth,
                          [[maybe_unused]] int screenHeight) {
    initWin95Theme();
}

}  // namespace ui_imm
