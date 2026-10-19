#pragma once

#include <algorithm>

#include "../editor/ruler_math.h"
#include "../editor/scroll_math.h"
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

// Transient status-bar message: replaces the status bar's left
// text until `now + duration` (STATUS_MESSAGE_DURATION, 3s), then
// the bar reverts to the caret/stats line. This is the Word 95
// channel for mode switches — no floating toast.
inline void flash(StatusComponent& status, const std::string& msg,
                  double now, double duration = 3.0) {
    status.text = msg;
    status.isError = false;
    status.expiresAt = now + duration;
}

}  // namespace status

namespace layout {

// ONE scale source — definitions below updateLayout, declared here
// because updateLayout uses them.
inline float displayScale(const LayoutComponent& layout);
inline float pixelsPerInch(const LayoutComponent& layout);

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
    // Print Preview shows ONE compact toolbar row: the Formatting
    // toolbar is gone in preview, and the ruler is hidden in preview
    // (Word shows a ruler in preview only via its toggle; default
    // off), so both heights are reclaimed by the text area.
    float formattingBarHeight =
        (layout.focusMode || layout.previewMode)
            ? 0.0f
            : theme::layout::scale(layout.formattingBarHeight);
    float rulerHeight = (layout.focusMode || !layout.showRuler ||
                         layout.previewMode)
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

    // Calculate page display dimensions for paged mode.
    // pageScale is the FIT factor only (no zoom); displayScale()
    // below folds zoom in, so pageDisplayWidth = pageWidth(points) *
    // displayScale = (pageWidth/72) * pixelsPerInch = 8.5 * ppi.
    // updateLayout runs every frame (LayoutUpdateSystem), so a zoom
    // change is picked up without a separate invalidation call.
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

        float ds = displayScale(layout);
        layout.pageDisplayWidth = layout.pageWidth * ds;
        layout.pageDisplayHeight = layout.pageHeight * ds;
        layout.pageOffsetX =
            layout.textArea.x +
            (layout.textArea.width - layout.pageDisplayWidth) / 2.0f;
    }
}

// ONE scale source for page, ruler, margins, indents, and text.
//   displayScale = (paged ? pageScale(fit) : 1) * zoomLevel
//   pixelsPerInch = 72 * displayScale
// Every consumer — drawPageBackground, effectiveTextArea, ruler
// geometry/ticks, indent marker/text positions, document font
// sizes (ruler_math::scaledFontSize) — derives from these,
// so the page width is exactly 8.5 * pixelsPerInch and a stored
// indent of 72 always lands on the 1" ruler tick.
inline float displayScale(const LayoutComponent& layout) {
    float fit = layout.pageMode == PageMode::Paged ? layout.pageScale : 1.0f;
    return fit * layout.zoomLevel;
}

inline float pixelsPerInch(const LayoutComponent& layout) {
    return ruler_math::PIXELS_PER_INCH * displayScale(layout);
}

// Effective (displayed) zoom: fit * user zoom. In pageless mode
// fit is 1, so this is just zoomLevel; in paged / Print Preview
// the page is fit-scaled (~54% at 1280x720) and the Zoom control
// and status bar must show THIS, not the raw user zoom — showing
// 100% while the page renders at ~54% is dishonest. Choosing an
// effective zoom sets the user zoom so fit * user == target:
// at effective 100% the page renders at 72ppi * 1.0, larger than
// the viewport, and the existing wheel/PageUp/PageDown paging
// (and, in the editing view, scroll) reaches the rest.
inline float effectiveZoom(const LayoutComponent& layout) {
    return displayScale(layout);
}

inline void setEffectiveZoom(LayoutComponent& layout, float effective) {
    float fit = layout.pageMode == PageMode::Paged ? layout.pageScale : 1.0f;
    if (fit <= 0.0f) fit = 1.0f;
    // User zoom, clamped to a sane range (effective 200% at a
    // 30% fit is a 6.7x user zoom — let the dropdown reach it).
    layout.zoomLevel = std::clamp(effective / fit, 0.1f, 10.0f);
}

inline LayoutComponent::Rect effectiveTextArea(const LayoutComponent& layout) {
    // Margins in display pixels: margin points (72/in), so
    // marginPx = (margin/72) * pixelsPerInch =
    // margin * displayScale — the same scale as the page, ruler,
    // and indents. Per-side: left/right set x/width, top/bottom
    // set y/height. Pageless therefore also gets a real margin band:
    // the ruler is gray outside it, white inside it, instead of
    // white edge-to-edge.
    float ds = displayScale(layout);
    float leftPx = layout.marginLeft * ds;
    float rightPx = layout.marginRight * ds;
    if (layout.pageMode == PageMode::Pageless) {
        float x = layout.textArea.x + leftPx;
        float w = layout.textArea.width - leftPx - rightPx;
        if (w < 0.0f) w = 0.0f;
        // Apply line width limit if set, centered inside the margins.
        if (layout.lineWidthLimit > 0.0f) {
            float limitedWidth =
                layout.lineWidthLimit * 8.0f;  // Approximate char width
            if (limitedWidth < w) {
                float offset = (w - limitedWidth) / 2.0f;
                return {x + offset, layout.textArea.y, limitedWidth,
                        layout.textArea.height};
            }
        }
        return {x, layout.textArea.y, w, layout.textArea.height};
    }

    // Paged mode: return area within page margins
    float topPx = layout.marginTop * ds;
    float bottomPx = layout.marginBottom * ds;
    return {layout.pageOffsetX + leftPx,
            layout.textArea.y + 10.0f + topPx,  // 10px for page shadow
            layout.pageDisplayWidth - leftPx - rightPx,
            layout.pageDisplayHeight - topPx - bottomPx};
}

}  // namespace layout

namespace ruler {

// The ruler is visible only outside focus mode and when not toggled
// off (View > Ruler). Hidden in Print Preview (Word default): the
// showRuler flag itself is untouched, so exiting preview restores it.
// When hidden, updateLayout reclaims its height.
inline bool visible(const LayoutComponent& layout) {
    return !layout.focusMode && layout.showRuler && !layout.previewMode;
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
                                effective.x, effective.width, gutter,
                                layout::pixelsPerInch(layout));
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

// View > Normal (Word 95 parity): Normal IS the pageless view,
// renamed — continuous text, no page frame. It is the one view
// state derived from pageMode, never a second flag: pageMode ==
// Pageless outside Outline / Print Preview. Outline forces
// Pageless but is not Normal (it hides body rows); preview forces
// Paged. The renderer, the View-menu checkmarks, and the E2E
// `normal_view` property all read this one predicate.
inline bool isNormalView(const LayoutComponent& layout) {
    return layout.pageMode == PageMode::Pageless &&
           !layout.showOutline && !layout.previewMode;
}

// --- Print Preview (File > Print Preview) ---
// Enters preview: remembers the current page mode, forces Paged so the
// paged renderer draws ONE page on the gray workspace, and estimates
// pagination from geometry (see scroll_math.h: no real pagination
// exists, so the page count is an estimate). Exiting restores the
// remembered mode. Idempotent: entering twice is a no-op (the second
// enter must not overwrite previewPrevPageMode with Paged).
// Recompute the preview pagination estimate from the current
// geometry (zoom changes the scaled line height, hence the lines
// per page). No-op outside preview. Clamps the current page into
// the new page count.
inline void refreshPreviewPagination(LayoutComponent& layout,
                                     std::size_t totalLines,
                                     int baseFontSize) {
    if (!layout.previewMode) return;
    // Lines per page from the paged geometry: page text height (page
    // height minus both margins, in display px) divided by the scaled
    // line height. Uses the Normal-paragraph font; headings/spacing
    // shift individual breaks (estimate, not pagination).
    float ds = displayScale(layout);
    float textHeightPx =
        layout.pageDisplayHeight -
        (layout.marginTop + layout.marginBottom) * ds;
    int fontSize = ruler_math::scaledFontSize(baseFontSize, ds);
    int lineHeight = fontSize + 4;
    layout.previewLinesPerPage =
        scroll_math::previewLinesPerPage(textHeightPx, static_cast<float>(lineHeight));
    layout.previewPageCount =
        scroll_math::previewPageCount(totalLines, layout.previewLinesPerPage);
    layout.previewPage = scroll_math::previewPageClamp(
        layout.previewPage, layout.previewPageCount);
}

inline void enterPrintPreview(LayoutComponent& layout, std::size_t totalLines,
                              int baseFontSize) {
    if (layout.previewMode) return;
    layout.previewPrevPageMode = layout.pageMode;
    // Preview zoom is preview-local: choosing an effective 100%
    // in preview must not leave the editing view at ~185% user
    // zoom (1 / fit) when preview closes.
    layout.previewPrevZoomLevel = layout.zoomLevel;
    layout.previewMode = true;
    layout.previewPage = 0;
    layout.pageMode = PageMode::Paged;
    updateLayout(layout, layout.screenWidth, layout.screenHeight);
    refreshPreviewPagination(layout, totalLines, baseFontSize);
}

inline void exitPrintPreview(LayoutComponent& layout) {
    if (!layout.previewMode) return;
    layout.previewMode = false;
    layout.pageMode = layout.previewPrevPageMode;
    layout.zoomLevel = layout.previewPrevZoomLevel;
    layout.previewPage = 0;
    layout.previewPageCount = 1;
    layout.previewLinesPerPage = 50;
    updateLayout(layout, layout.screenWidth, layout.screenHeight);
}

inline void previewNextPage(LayoutComponent& layout) {
    if (!layout.previewMode) return;
    layout.previewPage = scroll_math::previewPageClamp(
        layout.previewPage + 1, layout.previewPageCount);
}

inline void previewPrevPage(LayoutComponent& layout) {
    if (!layout.previewMode) return;
    layout.previewPage = scroll_math::previewPageClamp(
        layout.previewPage - 1, layout.previewPageCount);
}

}  // namespace layout

}  // namespace ecs
