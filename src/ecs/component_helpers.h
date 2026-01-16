#pragma once

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

    layout.titleBar = {0, 0, static_cast<float>(w), layout.titleBarHeight};
    layout.menuBar = {0, layout.titleBarHeight, static_cast<float>(w),
                      layout.menuBarHeight};
    layout.statusBar = {0, static_cast<float>(h - layout.statusBarHeight),
                        static_cast<float>(w), layout.statusBarHeight};

    float textTop =
        layout.titleBarHeight + layout.menuBarHeight + layout.borderWidth;
    float textHeight = static_cast<float>(h) - layout.titleBarHeight -
                       layout.menuBarHeight - layout.statusBarHeight -
                       2 * layout.borderWidth;
    layout.textArea = {layout.borderWidth, textTop,
                       static_cast<float>(w) - 2 * layout.borderWidth,
                       textHeight};

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
