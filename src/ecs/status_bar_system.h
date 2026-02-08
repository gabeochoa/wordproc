#pragma once

#include <ctime>
#include <format>

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

    void for_each_with(Entity& /*ctxEntity*/, UIContext<InputAction>& ctx, float) override {
        // Find layout
        auto layoutEntities = afterhours::EntityQuery({.force_merge = true})
                                 .whereHasComponent<LayoutComponent>()
                                 .gen();
        if (layoutEntities.empty()) return;
        auto& layout = layoutEntities[0].get().get<LayoutComponent>();

        if (layout.focusMode) return;

        // Find document
        auto docEntities = afterhours::EntityQuery({.force_merge = true})
                              .whereHasComponent<DocumentComponent>()
                              .gen();
        if (docEntities.empty()) return;
        auto& doc = docEntities[0].get().get<DocumentComponent>();

        float screenWidth = static_cast<float>(layout.screenWidth);
        float statusBarHeight = theme::layout::scale(theme::layout::STATUS_BAR_HEIGHT);
        float statusBarY = static_cast<float>(layout.screenHeight) - statusBarHeight;

        Entity& uiRoot = ui_imm::getUIRootEntity();

        // Build the left-side status string
        CaretPosition caretPos = doc.buffer.caret();
        TextStats stats = doc.buffer.stats();

        float inchPos = static_cast<float>(caretPos.column) / 72.0f;
        std::string leftText = std::format(
            "Page 1   Sec 1      {}/{}      At {:.1f}\"      Ln {}   Col {}",
            caretPos.row + 1, stats.lines,
            inchPos,
            caretPos.row + 1, caretPos.column + 1);

        // Build the right-side status string
        std::time_t now = std::time(nullptr);
        std::tm* localTime = std::localtime(&now);
        char timeStr[16];
        std::strftime(timeStr, sizeof(timeStr), "%I:%M %p", localTime);
        std::string rightText = std::format("REC   MRK   EXT   OVR      {}", timeStr);

        // === Status bar background ===
        div(ctx, mk(uiRoot, 8000),
            ComponentConfig{}
                .with_size(ComponentSize{pixels(screenWidth), pixels(statusBarHeight)})
                .with_absolute_position()
                .with_translate(0, statusBarY)
                .with_custom_background(ui_imm::win95_colors::BUTTON_FACE)
                .with_border(ui_imm::win95_colors::BORDER_LIGHT, 1.0f)
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
                .with_debug_name("status_right"));
    }
};

}  // namespace ecs
