#pragma once

#include <cmath>
#include <ctime>
#include <format>

#include "../../vendor/afterhours/src/core/system.h"
#include "../ui/ah_win95_widgets.h"
#include "../ui/theme.h"
#include "../ui/ui_context.h"
#include "../external.h"
#include "component_helpers.h"
#include "components.h"
#include "editor_entity_cache.h"

namespace ecs {

using afterhours::Entity;
using afterhours::ui::UIContext;
using afterhours::ui::imm::ComponentConfig;
using afterhours::ui::imm::div;
using afterhours::ui::imm::mk;
using afterhours::ui::pixels;
using afterhours::ui::percent;
using afterhours::ui::FlexDirection;
using afterhours::ui::AlignItems;
using afterhours::ui::JustifyContent;
using afterhours::ui::ComponentSize;
using afterhours::ui::Padding;

// Status Bar System - renders Word 6.0-style status bar using Afterhours UI
struct StatusBarSystem : afterhours::System<UIContext<InputAction>> {
    EditorEntityCache cache_;

    void for_each_with(Entity& /*ctxEntity*/, UIContext<InputAction>& ctx, float) override {
        cache_.resolve();
        if (!cache_.resolved()) return;
        auto& layout = *cache_.layout;
        auto& doc = *cache_.doc;

        if (layout.focusMode) return;

        float screenWidth = static_cast<float>(layout.screenWidth);
        float statusBarHeight = theme::layout::scale(theme::layout::STATUS_BAR_HEIGHT);
        float statusBarY = static_cast<float>(layout.screenHeight) - statusBarHeight;

        Entity& uiRoot = ui_imm::getUIRootEntity();

        // Build the left-side status string. In Print Preview the
        // caret fields (Ln/Col/At) are meaningless — the preview page
        // has no caret — so the bar shows "Page X of Y" plus the
        // preview zoom instead, and the REC/MRK/EXT/OVR block on the
        // right is suppressed. Exiting preview restores both sides.
        std::string leftText;
        std::string rightText;
        if (layout.previewMode) {
            // Displayed zoom is the EFFECTIVE zoom (fit * user),
            // not the raw user zoom: at fit the page renders at
            // ~54% while user zoom is 100%.
            int zoomPct = static_cast<int>(
                std::lround(layout::effectiveZoom(layout) * 100.0f));
            leftText = std::format("Page {} of {}      {}%",
                                    layout.previewPage + 1,
                                    layout.previewPageCount, zoomPct);
            rightText = "";
        } else {
            CaretPosition caretPos = doc.buffer.caret();
            TextStats stats = doc.buffer.stats();

            float inchPos = static_cast<float>(caretPos.column) / 72.0f;
            leftText = std::format(
                "Page 1   Sec 1      {}/{}      At {:.1f}\"      Ln {}   Col {}",
                caretPos.row + 1, stats.lines,
                inchPos,
                caretPos.row + 1, caretPos.column + 1);

            // Build the right-side status string
            std::time_t now = std::time(nullptr);
            std::tm* localTime = std::localtime(&now);
            char timeStr[16];
            std::strftime(timeStr, sizeof(timeStr), "%I:%M %p", localTime);
            rightText = std::format("REC   MRK   EXT   OVR      {}", timeStr);
        }

        // Transient status message (e.g. "Paged mode" after a View >
        // Paged Mode switch) replaces the stats line until it expires.
        // This is the StatusComponent channel, rendered here — it was
        // previously created but never read.
        if (cache_.status &&
            status::hasMessage(*cache_.status,
                               afterhours::graphics::get_time())) {
            leftText = cache_.status->text;
        }

        // Register status bar text for E2E testing
        test_input::register_visible_text(leftText);
        test_input::register_visible_text(rightText);

        // === Status bar background ===
        div(ctx, mk(uiRoot, 8000),
            ComponentConfig{}
                .with_size(ComponentSize{pixels(screenWidth), pixels(statusBarHeight)})
                .with_absolute_position()
                .with_translate(0, statusBarY)
                .with_custom_background(ui_imm::win95_colors::BUTTON_FACE)
                .with_bevel(afterhours::ui::BevelStyle::Raised,
                            ui_imm::win95_colors::BORDER_LIGHT, ui_imm::win95_colors::BORDER_DARK, 1.0f)
                .with_roundness(0.0f)
                .with_debug_name("status_bar_bg"));

        // === Left-aligned status text ===
        div(ctx, mk(uiRoot, 8001),
            ComponentConfig{}
                .with_label(leftText)
                .with_size(ComponentSize{pixels(screenWidth * 0.6f), pixels(statusBarHeight)})
                .with_absolute_position()
                .with_translate(4.0f, statusBarY)
                .with_custom_text_color(ui_imm::win95_colors::TEXT)
                .with_roundness(0.0f)
                .with_alignment(afterhours::ui::TextAlignment::Left)
                .with_text_overflow(afterhours::ui::TextOverflow::Ellipsis)
                .with_debug_name("status_left"));

        // === Right-aligned status text ===
        div(ctx, mk(uiRoot, 8002),
            ComponentConfig{}
                .with_label(rightText)
                .with_size(ComponentSize{pixels(screenWidth * 0.4f), pixels(statusBarHeight)})
                .with_absolute_position()
                .with_translate(screenWidth * 0.6f, statusBarY)
                .with_custom_text_color(ui_imm::win95_colors::TEXT_DISABLED)
                .with_roundness(0.0f)
                .with_alignment(afterhours::ui::TextAlignment::Right)
                .with_text_overflow(afterhours::ui::TextOverflow::Ellipsis)
                .with_debug_name("status_right"));
    }
};

}  // namespace ecs
