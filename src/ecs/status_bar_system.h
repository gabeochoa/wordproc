#pragma once

#include <cmath>
#include <cstdint>
#include <ctime>
#include <format>
#include <limits>

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

    // Cached buffer stats (lines/words/chars). TextBuffer::stats() is
    // O(n); the status bar renders every frame, so recompute only
    // when the buffer version changes (any content or style edit).
    // The cache lives on the system object, which the app constructs
    // once, so it persists across frames.
    std::uint64_t cachedStatsVersion_ =
        std::numeric_limits<std::uint64_t>::max();
    TextStats cachedStats_{};

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
            // stats() is O(n): recompute only when the buffer version
            // changed (see cachedStatsVersion_). The "Words: N"
            // segment below reads the same cached stats, as does the
            // line count in the 3rd field.
            if (cachedStatsVersion_ != doc.buffer.version()) {
                cachedStats_ = doc.buffer.stats();
                cachedStatsVersion_ = doc.buffer.version();
            }
            TextStats stats = cachedStats_;

            float inchPos = static_cast<float>(caretPos.column) / 72.0f;
            // Page from the scroll offset, not the caret: the bar
            // reports the page on screen, and the caret can sit on
            // a later page than the first visible line (e.g. after
            // a wheel scroll with the caret left behind). The
            // lines-per-page is the same estimate the renderer and
            // the Normal-view break lines use (page text height /
            // scaled line height), times the column count in the
            // paged columns view, so the page the bar names is the
            // page the renderer draws. Sec stays 1: one section.
            float ds = layout::displayScale(layout);
            int lineHeight = ruler_math::scaledLineHeight(
                doc.buffer.textStyle().fontSize, ds);
            int linesPerColumn = static_cast<int>(
                columns_math::linesPerColumnForPage(
                    layout.pageHeight, layout.marginTop,
                    layout.marginBottom, ds, lineHeight));
            bool columnsActive =
                layout.pageMode == PageMode::Paged &&
                doc.docSettings.pageSettings.columnCount > 1;
            int linesPerPage =
                linesPerColumn *
                (columnsActive
                     ? doc.docSettings.pageSettings.columnCount
                     : 1);
            int scrollOffset =
                cache_.scroll ? cache_.scroll->offset : 0;
            int page = scroll_math::pageFromScrollOffset(
                scrollOffset, linesPerPage, stats.lines);
            leftText = std::format(
                "Page {}   Sec 1      {}/{}      At {:.1f}\"      Ln {}   Col {}      Words: {}",
                page, caretPos.row + 1, stats.lines,
                inchPos,
                caretPos.row + 1, caretPos.column + 1, stats.words);

            // Build the right-side status string
            std::time_t now = std::time(nullptr);
            std::tm* localTime = std::localtime(&now);
            char timeStr[16];
            std::strftime(timeStr, sizeof(timeStr), "%I:%M %p", localTime);
            // Word 95 fields: REC TRK MRK EXT OVR. TRK is dim
            // like the rest when tracking is off; while
            // trackChangesEnabled it is overpainted below in bold
            // TEXT colour (the mode indicator).
            rightText = std::format("REC   TRK   MRK   EXT   OVR      {}", timeStr);
        }

        // Transient status message (e.g. "Page Layout" after a View >
        // Page Layout switch) replaces the stats line until it expires.
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

        // === Right-aligned status text: REC TRK MRK EXT OVR + time ===
        // A flex row of text-hugging segments, right-justified, so
        // TRK is a real segment the layout engine positions with
        // the same font/size it renders with. (A first attempt
        // overlaid TRK at an x computed with
        // graphics::measure_text at 16px; UI divs render at
        // TypographyScale::base() = h720(20), so the overlay
        // landed ~33px left, on top of REC.)
        // TRK is dim like the rest when tracking is off; while
        // trackChangesEnabled it renders bold TEXT colour — the
        // mode indicator. Preview shows no right block (as
        // before: rightText is empty there).
        auto rightRow = div(
            ctx, mk(uiRoot, 8002),
            ComponentConfig{}
                .with_size(ComponentSize{
                    pixels(screenWidth * 0.4f), pixels(statusBarHeight)})
                .with_absolute_position()
                .with_translate(screenWidth * 0.6f, statusBarY)
                .with_flex_direction(FlexDirection::Row)
                .with_justify_content(JustifyContent::FlexEnd)
                .with_align_items(AlignItems::Center)
                .with_roundness(0.0f)
                .with_debug_name("status_right"));
        if (rightRow && !layout.previewMode) {
            const auto segFontSize =
                afterhours::ui::imm::TypographyScale::base();
            auto seg = [&](int id, const char* debugName,
                           const std::string& label, bool highlight) {
                div(ctx, mk(rightRow.ent(), id),
                    ComponentConfig{}
                        .with_label(label)
                        .with_fit_content(pixels(screenWidth * 0.4f),
                                         segFontSize)
                        .with_custom_text_color(
                            highlight ? ui_imm::win95_colors::TEXT
                                      : ui_imm::win95_colors::TEXT_DISABLED)
                        .with_font_weight(highlight
                                              ? afterhours::colors::FontWeight::
                                                    Bold
                                              : afterhours::colors::FontWeight::
                                                    Regular)
                        .with_roundness(0.0f)
                        .with_alignment(
                            afterhours::ui::TextAlignment::Left)
                        .with_debug_name(debugName));
            };
            const std::string kPrefix = "REC   TRK";
            const std::string rest =
                rightText.size() > kPrefix.size()
                    ? rightText.substr(kPrefix.size())
                    : "";
            seg(8003, "status_rec", "REC   ", false);
            seg(8004, "status_trk", "TRK", doc.trackChangesEnabled);
            seg(8005, "status_rest", rest, false);
        }
    }
};

}  // namespace ecs
