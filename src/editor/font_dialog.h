#pragma once

// Format > Font... dialog model (Word 95 parity).
// Pure selection <-> TextStyle mapping, unit-tested; the dialog in
// MenuUISystem holds a FontDialogSelection and applies it through
// the same applyStyleToCurrentRange path the formatting toolbar
// uses (a selection gets a per-range span, a bare caret sets the
// global typing style).

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include "document_settings.h"

namespace font_dialog {

// Preview font size in pixels: the pending point size scaled by the
// UI scale (Settings::get().get_ui_scale(), the same factor
// theme::layout::scale() applies to every other UI text size). The
// old preview used pixels(pendingPt), which ignored that scale, so
// at the default ui_scale 1.2 a 16pt preview rendered 16px while
// its neighbouring dialog text was scaled up -- the preview looked
// small for its size. At ui_scale 1.0 this is the identity.
inline int previewFontPx(int pendingPt, float uiScale) {
    return std::max(1, static_cast<int>(
                           std::lround(static_cast<float>(pendingPt) * uiScale)));
}

// The dialog's working selection.
struct FontDialogSelection {
    int fontIndex = 0;   // index into the toolbar fonts vector
    int styleIndex = 0;  // 0 Regular, 1 Bold, 2 Italic, 3 Bold Italic
    int size = 16;       // point size (list pick or custom value)
    bool underline = false;
    bool strikethrough = false;
    bool superscript = false;
    bool subscript = false;
    int colorIndex = 0;  // index into fontDialogColors()
};

// The Font color dropdown's entries: the existing text colors.
// The dialog has no color swatch row (the dropdown is the only
// color control); this list is its single source of entries.
inline std::vector<std::pair<std::string, TextColor>> fontDialogColors() {
    return {{"Black", TextColors::Black},   {"White", TextColors::White},
            {"Red", TextColors::Red},       {"Orange", TextColors::Orange},
            {"Green", TextColors::Green},   {"Blue", TextColors::Blue},
            {"Purple", TextColors::Purple}, {"Gray", TextColors::Gray}};
}

// Seed the dialog from the current style: font/style/size/effects/
// color. An unknown font or color falls back to the first entry.
inline FontDialogSelection fontDialogSelectionFromStyle(
    const TextStyle& style, const std::vector<std::string>& fonts) {
    FontDialogSelection sel;
    sel.fontIndex = 0;
    for (std::size_t i = 0; i < fonts.size(); ++i) {
        if (fonts[i] == style.font) sel.fontIndex = static_cast<int>(i);
    }
    sel.styleIndex = (style.bold ? 1 : 0) + (style.italic ? 2 : 0);
    sel.size = style.fontSize;
    sel.underline = style.underline;
    sel.strikethrough = style.strikethrough;
    // Superscript/subscript are mutually exclusive in the toggles:
    // a style with both (impossible via the UI) reads as superscript.
    sel.superscript = style.superscript;
    sel.subscript = style.subscript && !style.superscript;
    const auto colors = fontDialogColors();
    sel.colorIndex = 0;
    for (std::size_t i = 0; i < colors.size(); ++i) {
        if (colors[i].second == style.textColor) {
            sel.colorIndex = static_cast<int>(i);
        }
    }
    return sel;
}

// Build the style to apply from a dialog selection.
inline TextStyle fontDialogApplyStyle(
    const TextStyle& base, const FontDialogSelection& sel,
    const std::vector<std::string>& fonts) {
    TextStyle style = base;
    if (sel.fontIndex >= 0 &&
        sel.fontIndex < static_cast<int>(fonts.size())) {
        style.font = fonts[static_cast<std::size_t>(sel.fontIndex)];
    }
    style.bold = sel.styleIndex == 1 || sel.styleIndex == 3;
    style.italic = sel.styleIndex == 2 || sel.styleIndex == 3;
    style.fontSize = std::clamp(sel.size, 8, 72);
    style.underline = sel.underline;
    style.strikethrough = sel.strikethrough;
    style.superscript = sel.superscript;
    // Mutual exclusion, as in the toggles.
    style.subscript = sel.subscript && !sel.superscript;
    const auto colors = fontDialogColors();
    if (sel.colorIndex >= 0 &&
        sel.colorIndex < static_cast<int>(colors.size())) {
        style.textColor =
            colors[static_cast<std::size_t>(sel.colorIndex)].second;
    }
    return style;
}

}  // namespace font_dialog
