#pragma once

#include "../editor/ruler_math.h"
#include "../ui/theme.h"
#include "components.h"

// Pure helper functions for ECS components
// Components should only have fields; logic lives here or in systems

namespace ecs {

namespace caret {

inline void updateBlink(CaretComponent& caret, float dt) {
    caret.blinkTimer += dt;
    if (caret.blinkTimer >= CaretComponent::BLINK_INTERVAL) {
        caret.blinkTimer = 0.0;
        caret.visible = !caret.visible;
    }
}

inline void resetBlink(CaretComponent& caret) {
    caret.visible = true;
    caret.blinkTimer = 0.0;
}

}  // namespace caret

namespace scroll {

inline void clamp(ScrollComponent& scroll, int lineCount) {
    scroll.maxScroll = lineCount - scroll.visibleLines;
    if (scroll.maxScroll < 0) scroll.maxScroll = 0;
    if (scroll.offset < 0) scroll.offset = 0;
    if (scroll.offset > scroll.maxScroll) scroll.offset = scroll.maxScroll;
}

inline void scrollToRow(ScrollComponent& scroll, int row) {
    if (row < scroll.offset) {
        scroll.offset = row;
    } else if (row >= scroll.offset + scroll.visibleLines) {
        scroll.offset = row - scroll.visibleLines + 1;
    }
}

}  // namespace scroll

namespace status {

inline void set(StatusComponent& status, const std::string& msg,
                bool error = false) {
    status.text = msg;
    status.isError = error;
    // Note: expiresAt should be set by caller with current time + duration
}

inline bool hasMessage(const StatusComponent& status, double currentTime) {
    return !status.text.empty() && currentTime < status.expiresAt;
}

}  // namespace status

namespace layout {

inline void updateLayout(LayoutComponent& layout, int w, int h) {
    layout.screenWidth = w;
    layout.screenHeight = h;

    // Layout fields are BASE sizes ("will be scaled in updateLayout"):
    // the afterhours UI divs (title/menu/toolbars/status) are drawn
    // with theme::layout::scale(), so the flow must scale the same
    // way or the divs paint over layout-flow elements — the ruler
    // (108..128) sat 18px under the scaled formatting bar (92..126)
    // and only its bottom 2px of ticks showed.
    float titleHeight =
        layout.focusMode ? 0.0f : theme::layout::scale(layout.titleBarHeight);
    float menuHeight =
        layout.focusMode ? 0.0f : theme::layout::scale(layout.menuBarHeight);
    float toolbarHeight =
        layout.focusMode ? 0.0f : theme::layout::scale(layout.toolbarHeight);
    float formattingBarHeight = layout.focusMode
                                    ? 0.0f
                                    : theme::layout::scale(layout.formattingBarHeight);
    float rulerHeight = (layout.focusMode || !layout.showRuler)
                            ? 0.0f
                            : theme::layout::scale(layout.rulerHeight);
    float statusHeight =
        layout.focusMode ? 0.0f : theme::layout::scale(layout.statusBarHeight);
    float borderWidth = theme::layout::scale(layout.borderWidth);

    layout.titleBar = {0, 0, static_cast<float>(w), titleHeight};
    layout.menuBar = {0, titleHeight, static_cast<float>(w), menuHeight};
    layout.statusBar = {0, static_cast<float>(h - statusHeight),
                        static_cast<float>(w), statusHeight};

    float textTop = titleHeight + menuHeight + toolbarHeight +
                    formattingBarHeight + rulerHeight + borderWidth;
    float textHeight = static_cast<float>(h) - titleHeight - menuHeight -
                       toolbarHeight - formattingBarHeight - rulerHeight -
                       statusHeight - 2 * borderWidth;
    layout.textArea = {borderWidth, textTop,
                       static_cast<float>(w) - 2 * borderWidth, textHeight};

    // Calculate page display dimensions for paged mode
    if (layout.pageMode == PageMode::Paged) {
        float availableWidth =
            layout.textArea.width - 20.0f;  // Margin for page shadow
        float availableHeight = layout.textArea.height - 20.0f;

        // Scale to fit horizontally
        layout.pageScale = availableWidth / layout.pageWidth;
        if (layout.pageScale * layout.pageHeight > availableHeight * 0.9f) {
            // If page would be too tall, scale to fit vertically
            layout.pageScale = (availableHeight * 0.9f) / layout.pageHeight;
        }

        layout.pageDisplayWidth = layout.pageWidth * layout.pageScale;
        layout.pageDisplayHeight = layout.pageHeight * layout.pageScale;
        layout.pageOffsetX =
            layout.textArea.x +
            (layout.textArea.width - layout.pageDisplayWidth) / 2.0f;
    }
}

inline LayoutComponent::Rect effectiveTextArea(const LayoutComponent& layout) {
    if (layout.pageMode == PageMode::Pageless) {
        // Apply line width limit if set
        if (layout.lineWidthLimit > 0.0f) {
            float limitedWidth =
                layout.lineWidthLimit * 8.0f;  // Approximate char width
            if (limitedWidth < layout.textArea.width) {
                float offset = (layout.textArea.width - limitedWidth) / 2.0f;
                return {layout.textArea.x + offset, layout.textArea.y,
                        limitedWidth, layout.textArea.height};
            }
        }
        return layout.textArea;
    }

    // Paged mode: return area within page margins
    float marginScaled = layout.pageMargin * layout.pageScale;
    return {layout.pageOffsetX + marginScaled,
            layout.textArea.y + 10.0f + marginScaled,  // 10px for page shadow
            layout.pageDisplayWidth - 2.0f * marginScaled,
            layout.pageDisplayHeight - 2.0f * marginScaled};
}

}  // namespace layout

namespace ruler {

// The ruler is visible only outside focus mode and when not toggled
// off (View > Ruler). When hidden, updateLayout reclaims its height.
inline bool visible(const LayoutComponent& layout) {
    return !layout.focusMode && layout.showRuler;
}

// Screen rect of the ruler band: immediately above the text area,
// below the Formatting toolbar (textArea.y already reserves it).
inline LayoutComponent::Rect rect(const LayoutComponent& layout) {
    if (!visible(layout)) {
        return {0.0f, layout.textArea.y, layout.textArea.width, 0.0f};
    }
    float rulerHeight = theme::layout::scale(layout.rulerHeight);
    return {0.0f, layout.textArea.y - rulerHeight,
            static_cast<float>(layout.screenWidth), rulerHeight};
}

// Shared marker geometry for the render pass and the drag system,
// from the same effective text area the text renderer uses.
inline ruler_math::Geometry geometry(const LayoutComponent& layout) {
    LayoutComponent::Rect effective = layout::effectiveTextArea(layout);
    float gutter = layout.showLineNumbers ? layout.lineNumberGutterWidth : 0.0f;
    return ruler_math::geometry(layout.textArea.y,
                                theme::layout::scale(layout.rulerHeight),
                                effective.x, effective.width, gutter);
}

inline void toggleRuler(LayoutComponent& layout) {
    layout.showRuler = !layout.showRuler;
    layout::updateLayout(layout, layout.screenWidth, layout.screenHeight);
}

}  // namespace ruler

namespace layout {

inline void togglePageMode(LayoutComponent& layout) {
    layout.pageMode = (layout.pageMode == PageMode::Pageless)
                          ? PageMode::Paged
                          : PageMode::Pageless;
    updateLayout(layout, layout.screenWidth, layout.screenHeight);
}

inline void setLineWidthLimit(LayoutComponent& layout, float chars) {
    layout.lineWidthLimit = chars;
}

}  // namespace layout

}  // namespace ecs
