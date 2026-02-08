#pragma once

#include <filesystem>
#include <string>

#include "../../vendor/afterhours/src/core/system.h"
#include "../ui/ah_win95_widgets.h"
#include "../ui/theme.h"
#include "../ui/ui_context.h"
#include "components.h"

namespace ecs {

using afterhours::Entity;
using afterhours::ui::UIContext;
using afterhours::ui::imm::ComponentConfig;
using afterhours::ui::imm::div;
using afterhours::ui::imm::button;
using afterhours::ui::imm::mk;
using afterhours::ui::pixels;
using afterhours::ui::percent;
using afterhours::ui::ComponentSize;

// Helper: Win95-style title bar control button
inline ComponentConfig titleBarButton(float x, float y, float w, float h) {
    return ComponentConfig{}
        .with_size(ComponentSize{pixels(w), pixels(h)})
        .with_absolute_position()
        .with_translate(x, y)
        .with_custom_background(ui_imm::win95_colors::BUTTON_FACE)
        .with_custom_text_color(ui_imm::win95_colors::TEXT)
        .with_bevel(afterhours::ui::BevelStyle::Raised,
                    ui_imm::win95_colors::BORDER_LIGHT, ui_imm::win95_colors::BORDER_DARK, 1.0f)
        .with_roundness(0.0f)
        .with_alignment(afterhours::ui::TextAlignment::Center);
}

// Title Bar System - renders the blue title bar using Afterhours UI
struct TitleBarSystem : afterhours::System<UIContext<InputAction>> {

    void for_each_with(Entity& /*ctxEntity*/, UIContext<InputAction>& ctx, float) override {
        // Find layout
        auto layoutEntities = afterhours::EntityQuery({.force_merge = true})
                                 .whereHasComponent<LayoutComponent>()
                                 .gen();
        if (layoutEntities.empty()) return;
        auto& layout = layoutEntities[0].get().get<LayoutComponent>();

        // Find document
        auto docEntities = afterhours::EntityQuery({.force_merge = true})
                              .whereHasComponent<DocumentComponent>()
                              .gen();
        if (docEntities.empty()) return;
        auto& doc = docEntities[0].get().get<DocumentComponent>();

        float screenWidth = static_cast<float>(layout.screenWidth);
        float titleBarHeight = layout.titleBarHeight;

        // Build title string
        std::string title = "Wordproc";
        if (!doc.filePath.empty()) {
            title += " - " + std::filesystem::path(doc.filePath).filename().string();
        } else {
            title += " - Untitled";
        }
        if (doc.isDirty) {
            title += " [Modified]";
        }

        Entity& uiRoot = ui_imm::getUIRootEntity();

        // Title bar div — blue background, white text, left-aligned
        div(ctx, mk(uiRoot, 9000),
            ComponentConfig{}
                .with_label(title)
                .with_size(ComponentSize{pixels(screenWidth), pixels(titleBarHeight)})
                .with_absolute_position()
                .with_translate(0, 0)
                .with_custom_background(afterhours::Color{
                    theme::TITLE_BAR.r, theme::TITLE_BAR.g,
                    theme::TITLE_BAR.b, theme::TITLE_BAR.a})
                .with_custom_text_color(afterhours::Color{
                    theme::TITLE_TEXT.r, theme::TITLE_TEXT.g,
                    theme::TITLE_TEXT.b, theme::TITLE_TEXT.a})
                .with_roundness(0.0f)
                .with_alignment(afterhours::ui::TextAlignment::Left)
                .with_padding(afterhours::ui::Padding{.left = pixels(4)})
                .with_debug_name("title_bar"));

        // === Window control buttons (right side of title bar) ===
        float btnW = theme::layout::scale(16);
        float btnH = theme::layout::scale(14);
        float btnPad = theme::layout::scale(2);
        float btnY = (titleBarHeight - btnH) / 2.0f;

        // Close button (rightmost)
        float closeX = screenWidth - btnW - btnPad;
        if (button(ctx, mk(uiRoot, 9010),
            titleBarButton(closeX, btnY, btnW, btnH)
                .with_label("X")
                .with_debug_name("btn_close"))) {
            // Request window close
            // Note: raylib doesn't have a clean "close" API from within,
            // but we can set a flag that the main loop checks
        }

        // Maximize button
        float maxX = closeX - btnW - btnPad;
        button(ctx, mk(uiRoot, 9011),
            titleBarButton(maxX, btnY, btnW, btnH)
                .with_label("o")
                .with_debug_name("btn_maximize"));

        // Minimize button
        float minX = maxX - btnW - btnPad;
        if (button(ctx, mk(uiRoot, 9012),
            titleBarButton(minX, btnY, btnW, btnH)
                .with_label("_")
                .with_debug_name("btn_minimize"))) {
            raylib::MinimizeWindow();
        }
    }
};

}  // namespace ecs
