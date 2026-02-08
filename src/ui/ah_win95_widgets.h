#pragma once

// Afterhours-based Win95 widgets
// These are Afterhours UI components styled to match the Win95 aesthetic
// Used as drop-in replacements for win95_widgets.h functions
// This is the CANONICAL source for all Win95 widget configs.

#include <afterhours/src/plugins/ui.h>
#include <string>
#include <vector>

#include "theme.h"
#include "ui_context.h"

namespace ah_win95 {

using namespace afterhours;
using namespace afterhours::ui;
using namespace afterhours::ui::imm;

// ============================================================
// Win95 button style configuration (general purpose)
// ============================================================
inline ComponentConfig win95ButtonStyle(bool enabled = true, bool pressed = false) {
    afterhours::Color bg = pressed ? ui_imm::win95_colors::BORDER_DARK : ui_imm::win95_colors::BUTTON_FACE;
    afterhours::Color textColor = enabled ? ui_imm::win95_colors::TEXT : ui_imm::win95_colors::TEXT_DISABLED;

    auto config = ComponentConfig{}
        .with_roundness(0.0f)
        .with_custom_background(bg)
        .with_custom_text_color(textColor)
        .with_padding(Padding{
            .top = pixels(4),
            .right = pixels(8),
            .bottom = pixels(4),
            .left = pixels(8)
        })
        .with_margin(Margin{
            .top = pixels(2),
            .right = pixels(2),
            .bottom = pixels(2),
            .left = pixels(2)
        });

    if (!enabled) {
        config.disabled = true;
    }

    return config;
}

// ============================================================
// Win95 toolbar button - square with icon/text, centered label
// ============================================================
inline ComponentConfig win95ToolbarButtonStyle(float size, bool enabled = true, bool pressed = false) {
    afterhours::Color bg = pressed ? ui_imm::win95_colors::BORDER_DARK : ui_imm::win95_colors::BUTTON_FACE;
    afterhours::Color textColor = enabled ? ui_imm::win95_colors::TEXT : ui_imm::win95_colors::TEXT_DISABLED;

    auto config = ComponentConfig{}
        .with_size(ComponentSize{pixels(size), pixels(size)})
        .with_roundness(0.0f)
        .with_custom_background(bg)
        .with_custom_text_color(textColor)
        .with_margin(Margin{.right = pixels(2)})
        .with_alignment(afterhours::ui::TextAlignment::Center);

    if (!enabled) {
        config.disabled = true;
    }

    return config;
}

// ============================================================
// Win95 container/panel style (toolbar background, dialog, etc.)
// ============================================================
inline ComponentConfig win95PanelStyle(float width = 0, float height = 0) {
    auto config = ComponentConfig{}
        .with_roundness(0.0f)
        .with_custom_background(ui_imm::win95_colors::BUTTON_FACE)
        .with_flex_direction(FlexDirection::Row)
        .with_align_items(AlignItems::Center)
        .with_padding(Padding{
            .top = pixels(2),
            .right = pixels(4),
            .bottom = pixels(2),
            .left = pixels(4)
        });

    if (width > 0 && height > 0) {
        config.with_size(ComponentSize{pixels(width), pixels(height)});
    }

    return config;
}

// ============================================================
// Win95 separator (vertical line)
// ============================================================
inline ComponentConfig win95SeparatorStyle(float height) {
    return ComponentConfig{}
        .with_size(ComponentSize{pixels(2), pixels(height)})
        .with_margin(Margin{
            .top = pixels(2),
            .right = pixels(4),
            .bottom = pixels(2),
            .left = pixels(2)
        })
        .with_custom_background(ui_imm::win95_colors::BORDER_DARK)
        .with_roundness(0.0f);
}

// ============================================================
// Win95 dropdown button style
// ============================================================
inline ComponentConfig win95DropdownButtonStyle(float width, float height, bool open) {
    afterhours::Color bg = open ? ui_imm::win95_colors::TEXT_AREA : ui_imm::win95_colors::BUTTON_FACE;
    return ComponentConfig{}
        .with_size(ComponentSize{pixels(width), pixels(height)})
        .with_custom_background(bg)
        .with_custom_text_color(ui_imm::win95_colors::TEXT)
        .with_roundness(0.0f)
        .with_padding(Padding{.top = pixels(2), .right = pixels(4), .bottom = pixels(2), .left = pixels(4)})
        .with_margin(Margin{.right = pixels(4)})
        .with_alignment(afterhours::ui::TextAlignment::Left);
}

// ============================================================
// Win95 dropdown list style (absolute-positioned popup)
// ============================================================
inline ComponentConfig win95DropdownListStyle(float width, int itemCount = 0) {
    auto config = ComponentConfig{}
        .with_roundness(0.0f)
        .with_custom_background(ui_imm::win95_colors::TEXT_AREA)
        .with_border(ui_imm::win95_colors::BORDER_DARK, 1.0f)
        .with_flex_direction(FlexDirection::Column)
        .with_padding(Padding{
            .top = pixels(2),
            .right = pixels(2),
            .bottom = pixels(2),
            .left = pixels(2)
        })
        .with_absolute_position();

    if (itemCount > 0) {
        float itemHeight = theme::layout::scale(20);
        float listHeight = static_cast<float>(itemCount) * itemHeight + theme::layout::scale(4);
        config.with_size(ComponentSize{pixels(width), pixels(listHeight)});
    } else {
        config.with_size(ComponentSize{pixels(width), children()});
    }

    return config;
}

// ============================================================
// Win95 dropdown item style
// ============================================================
inline ComponentConfig win95DropdownItemStyle(bool selected) {
    afterhours::Color bg = selected ? ui_imm::win95_colors::HIGHLIGHT : ui_imm::win95_colors::TEXT_AREA;
    afterhours::Color text = selected ? ui_imm::win95_colors::TEXT_WHITE : ui_imm::win95_colors::TEXT;
    float itemHeight = theme::layout::scale(18);
    return ComponentConfig{}
        .with_size(ComponentSize{percent(1.0f), pixels(itemHeight)})
        .with_custom_background(bg)
        .with_custom_text_color(text)
        .with_roundness(0.0f)
        .with_padding(Padding{.left = pixels(4)})
        .with_alignment(afterhours::ui::TextAlignment::Left);
}

// ============================================================
// Toolbar button helper - returns true if clicked
// ============================================================
template<typename UIContextT>
inline bool toolbarButton(
    UIContextT& ctx,
    Entity& parent,
    int id,
    const std::string& label,
    float size,
    bool enabled = true,
    bool pressed = false
) {
    auto config = win95ToolbarButtonStyle(size, enabled, pressed)
        .with_label(label)
        .with_debug_name("ah_toolbar_btn_" + label);

    return button(ctx, mk(parent, id), config) && enabled;
}

// ============================================================
// Separator helper for toolbar
// ============================================================
template<typename UIContextT>
inline void toolbarSeparator(
    UIContextT& ctx,
    Entity& parent,
    int id,
    float height
) {
    auto config = win95SeparatorStyle(height)
        .with_debug_name("ah_toolbar_sep");

    div(ctx, mk(parent, id), config);
}

}  // namespace ah_win95

