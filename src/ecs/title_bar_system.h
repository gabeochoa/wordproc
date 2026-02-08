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
using afterhours::ui::imm::mk;
using afterhours::ui::pixels;
using afterhours::ui::percent;
using afterhours::ui::ComponentSize;

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
    }
};

}  // namespace ecs
