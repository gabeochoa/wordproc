#pragma once

// Afterhours-based Win95 widgets
// These are Afterhours UI components styled to match the Win95 aesthetic
// Used as drop-in replacements for win95_widgets.h functions
// This is the CANONICAL source for all Win95 widget configs.

#include <afterhours/src/plugins/modal.h>
#include <afterhours/src/plugins/ui.h>
#include <afterhours/src/plugins/ui/text_input/text_input.h>
#include <algorithm>
#include <string>
#include <utility>
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
        .with_bevel(pressed ? afterhours::ui::BevelStyle::Sunken : afterhours::ui::BevelStyle::Raised,
                    ui_imm::win95_colors::BORDER_LIGHT, ui_imm::win95_colors::BORDER_DARK, 2.0f)
        .with_cursor(CursorType::Pointer)
        .with_alignment(afterhours::ui::TextAlignment::Center)
        // Label inset ~8px horizontal, centered. Padding on a label is a
        // no-op in afterhours (it warns); text_inset is the mechanism
        // that actually insets the label.
        .with_text_inset(8.0f, 4.0f)
        .with_margin(Margin{
            .top = pixels(2),
            .right = pixels(2),
            .bottom = pixels(2),
            .left = pixels(2)
        });

    if (pressed) {
        // The framework has no label-only offset (text_inset is
        // symmetric, so it cannot shift a centered label), so the whole
        // face translates 1px down/right: the visible Win95 pressed
        // effect (label moves with the sunken face).
        config.with_translate(1.0f, 1.0f);
    }

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
        .with_cursor(CursorType::Pointer)
        .with_bevel(pressed ? afterhours::ui::BevelStyle::Sunken : afterhours::ui::BevelStyle::Raised,
                    ui_imm::win95_colors::BORDER_LIGHT, ui_imm::win95_colors::BORDER_DARK, 2.0f)
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
        .with_cursor(CursorType::Pointer)
        .with_bevel(open ? afterhours::ui::BevelStyle::Sunken : afterhours::ui::BevelStyle::Raised,
                    ui_imm::win95_colors::BORDER_LIGHT, ui_imm::win95_colors::BORDER_DARK, 1.0f)
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
        .with_absolute_position()
        // Win95 floats a menu above the content with a hard 2px shadow.
        .with_hard_shadow(ui_imm::WIN95_POPUP_SHADOW_OFFSET,
                          ui_imm::WIN95_POPUP_SHADOW_OFFSET,
                          ui_imm::WIN95_POPUP_SHADOW_COLOR);

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
        .with_cursor(CursorType::Pointer)
        .with_custom_hover_bg(ui_imm::win95_colors::HIGHLIGHT)
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
// Win95 dialog shell (Word 95 dialogs: Spelling, AutoCorrect,
// Word Count). Square BUTTON_FACE panel, raised bevel, no
// backdrop dimming. The modal title stays empty so the library
// does not emit its own rounded header; each dialog draws its
// own navy title bar as the first content row via
// win95DialogTitleBar().
// ============================================================
inline afterhours::ModalConfig win95DialogConfig(
    afterhours::ui::Size width, afterhours::ui::Size height,
    float screenWidth, float screenHeight) {
    // Resolve the panel size the same way modal_impl does: a
    // ScreenPercent size (h720/w1280) scales with screen height.
    auto resolve = [](afterhours::ui::Size size, float screenWidth,
                      float screenHeight) {
        switch (size.dim) {
            case afterhours::ui::Dim::Pixels: return size.value;
            case afterhours::ui::Dim::ScreenPercent:
                return size.value * screenHeight;
            case afterhours::ui::Dim::Percent: return size.value * screenWidth;
            default: return size.value;
        }
    };
    float widthPx = resolve(width, screenWidth, screenHeight);
    float heightPx = resolve(height, screenWidth, screenHeight);
    float x = std::max(0.0f, (screenWidth - widthPx) / 2.0f);
    float y = std::max(0.0f, (screenHeight - heightPx) / 2.0f);
    return afterhours::ModalConfig{}
        .with_size(width, height)
        .with_backdrop_color(afterhours::Color{0, 0, 0, 0})
        .with_title("")
        .with_show_close_button(false)
        .with_panel(ComponentConfig{}
                        .with_size(ComponentSize{pixels(widthPx), pixels(heightPx)})
                        .with_absolute_position(x, y)
                        .with_flex_direction(FlexDirection::Column)
                        .with_no_wrap()
                        .with_roundness(0.0f)
                        .with_custom_background(ui_imm::win95_colors::BUTTON_FACE)
                        .with_bevel(afterhours::ui::BevelStyle::Raised,
                                    ui_imm::win95_colors::BORDER_LIGHT,
                                    ui_imm::win95_colors::BORDER_DARK, 2.0f)
                        .with_padding(Padding::all(pixels(4)))
                        .with_debug_name("win95_dialog_panel"));
}

// Navy caption bar for a Win95 dialog: white bold small title on
// TITLE_BAR blue. `epPair` is the dialog's content parent, e.g.
// mk(result.ent(), <unique id>).
template<typename UIContextT>
inline auto win95DialogTitleBar(UIContextT& ctx,
                                afterhours::ui::imm::EntityParent epPair,
                                const std::string& title, int renderLayer) {
    return div(ctx, epPair,
               ComponentConfig{}
                   .with_label(title)
                   .with_size(ComponentSize{percent(1.0f), afterhours::ui::h720(20)})
                   .with_custom_background(ui_imm::win95_colors::TITLE_BAR)
                   .with_custom_text_color(ui_imm::win95_colors::TEXT_WHITE)
                   .with_font_weight(afterhours::colors::FontWeight::Bold)
                   .with_font_size(afterhours::ui::h720(14))
                   .with_roundness(0.0f)
                   .with_alignment(afterhours::ui::TextAlignment::Left)
                   .with_padding(Padding{.left = pixels(4)})
                   .with_render_layer(renderLayer)
                   .with_debug_name("win95_dialog_title"));
}

// Win95 text-field edge for `text_input` (shared widget point: every
// dialog field goes through win95TextInput below, never a per-dialog
// border).
//
// Why a border, not the bevel: `text_input` builds its field div
// with `inherit_from(config)`, which copies `border_config` but
// drops `bevel_config`, and with a custom (white) background it
// skips its own default border. So a `win95SunkenStyle()` field
// renders as flat white with no edge at all when unfocused, while
// the focused field gets upstream's hardcoded 2px black
// `HasBorder`. The per-side border below is the 1px form of the
// same sunken edge — dark top/left, light bottom/right — and it
// survives `inherit_from` onto the field entity. A caller-set
// border is left alone. Focused rendering is untouched: upstream
// overwrites the field's `HasBorder` with the black 2px border
// every frame after the div, so focus still shows the black
// square.
inline ComponentConfig win95TextInputConfig(ComponentConfig config) {
    if (!config.has_border()) {
        config.with_border_top(ui_imm::win95_colors::BORDER_DARK, pixels(1.0f))
            .with_border_left(ui_imm::win95_colors::BORDER_DARK, pixels(1.0f))
            .with_border_bottom(ui_imm::win95_colors::BORDER_LIGHT, pixels(1.0f))
            .with_border_right(ui_imm::win95_colors::BORDER_LIGHT, pixels(1.0f));
    }
    return config;
}

// Win95 single-line text field: upstream `text_input` with the
// sunken unfocused edge above. All dialog text fields call this,
// not `afterhours::text_input::text_input` directly.
template <typename UIContextT>
inline auto win95TextInput(UIContextT& ctx,
                           afterhours::ui::imm::EntityParent epPair,
                           std::string& text, ComponentConfig config) {
    return afterhours::text_input::text_input(
        ctx, epPair, text, win95TextInputConfig(std::move(config)));
}

// Sunken white field / list box (Not-in-dictionary field,
// Suggestions list, AutoCorrect Replace list). 2px bevel: the
// 1px version rendered but was too faint to read as sunken at
// dialog scale (dark top/left, light bottom/right).
inline ComponentConfig win95SunkenStyle() {
    return ComponentConfig{}
        .with_roundness(0.0f)
        .with_custom_background(ui_imm::win95_colors::TEXT_AREA)
        .with_custom_text_color(ui_imm::win95_colors::TEXT)
        .with_bevel(afterhours::ui::BevelStyle::Sunken,
                    ui_imm::win95_colors::BORDER_LIGHT,
                    ui_imm::win95_colors::BORDER_DARK, 2.0f);
}

// Standard Win95 dialog button: compact gray raised button,
// ~90px wide (Word Count OK, Spelling right column, AutoCorrect
// OK/Cancel). Callers add debug name, label, render layer.
// Widths use w1280, not h720: h720 resolves against the layout
// axis, so h720(90) as a width is 90/720*1280 = 160px (that is
// why these buttons used to render ~174px wide).
inline ComponentConfig win95DialogButtonStyle(bool enabled = true) {
    return win95ButtonStyle(enabled).with_size(
        ComponentSize{w1280(90), afterhours::ui::h720(28)});
}

// Win95 option checkbox: a small (14px) white sunken square box
// with the afterhours checkbox widget inside it, immediately left
// of its label on BUTTON_FACE. The checkbox widget owns state,
// focus, and the check mark; the sunken box is a separate div
// because the widget's config merge drops bevel_config, so a
// bevel passed to checkbox() would never reach its toggle.
// The checkbox itself is transparent so the box shows through.
template<typename UIContextT>
inline void win95Checkbox(UIContextT& ctx,
                          afterhours::ui::imm::EntityParent epPair,
                          bool& value, const std::string& debugName,
                          const std::string& label, int renderLayer) {
    using namespace afterhours::ui;
    using namespace afterhours::ui::imm;
    auto row = div(ctx, epPair,
                   ComponentConfig{}
                       .with_size(ComponentSize{percent(1.0f), h720(22)})
                       .with_flex_direction(FlexDirection::Row)
                       .with_align_items(AlignItems::Center)
                       .with_gap(pixels(6))
                       .with_custom_background(
                           ui_imm::win95_colors::BUTTON_FACE)
                       .with_render_layer(renderLayer));
    if (!row) return;
    auto box = div(ctx, mk(row.ent(), 0),
                   win95SunkenStyle()
                       // Square: width in w1280, height in h720.
                       .with_size(ComponentSize{w1280(14), h720(14)})
                       .with_render_layer(renderLayer));
    if (box) {
        checkbox(ctx, mk(box.ent(), 0), value,
                 ComponentConfig{}
                     .with_debug_name(debugName)
                     .with_size(ComponentSize{percent(1.0f), percent(1.0f)})
                     .with_transparent_bg()
                     .with_custom_text_color(ui_imm::win95_colors::TEXT)
                     .with_render_layer(renderLayer));
    }
    div(ctx, mk(row.ent(), 1),
        ComponentConfig{}
            .with_label(label)
            .with_size(ComponentSize{children(), h720(20)})
            .with_custom_background(ui_imm::win95_colors::BUTTON_FACE)
            .with_custom_text_color(ui_imm::win95_colors::TEXT)
            .with_alignment(afterhours::ui::TextAlignment::Left)
            .with_render_layer(renderLayer));
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

