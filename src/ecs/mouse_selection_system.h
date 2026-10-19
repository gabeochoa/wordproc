#pragma once

#include <algorithm>
#include <cmath>

#include "../../vendor/afterhours/src/core/system.h"
#include "../editor/columns_math.h"
#include "../editor/outline_view.h"
#include "../editor/scroll_math.h"
#include "../editor/text_layout.h"
#include "../rl.h"
#include "../ui/input.h"
#include "../ui/theme.h"
#include "component_helpers.h"
#include "components.h"
#include "document_commands.h"
#include "scrollbar_system.h"

namespace ecs {

// Click / drag text selection via hit testing. Single click places the
// caret, drag selects, shift-click extends, double-click selects the
// word, triple-click selects the paragraph. Dragging past the top or
// bottom edge auto-scrolls one line per frame. Typing replaces a
// selection through TextBuffer::insertChar -> deleteSelection.
// Runs after MenuUISystem/ScrollbarSystem; a press edge is consumed by
// prevDown_ tracking while a menu is open, so it never leaks through.
struct MouseSelectionSystem
    : public afterhours::System<DocumentComponent, CaretComponent, ScrollComponent,
                                LayoutComponent, MenuComponent, DialogState,
                                ToolbarComponent> {
    bool prevDown_ = false;
    bool dragging_ = false;
    // Whether this drag already auto-scrolled on a move frame. The
    // release frame adds a step only if no move frame ever fired (a
    // move frame may have been missed), so the release step never
    // double-counts a step the move frames already took. Move frames
    // still auto-scroll continuously while the pointer is held outside.
    bool autoScrolledThisDrag_ = false;
    // Anchor of the in-progress drag. Kept in the system (not in the
    // buffer) so a plain click leaves hasSelection()==false until the
    // drag actually moves.
    CaretPosition dragAnchor_{0, 0};
    // Cell drag (Batch 8): same idea inside one cell — the
    // anchor offset within the cell being dragged in. A drag
    // never spans cells (see NavigationSystem's cross-cell
    // collapse); movement clamps to the current cell's bounds.
    bool cellDragging_ = false;
    std::size_t dragAnchorCellOffset_ = 0;
    double lastClickTime_ = -1.0;
    int clickCount_ = 0;
    float lastClickX_ = 0.0f;
    float lastClickY_ = 0.0f;

    CaretPosition hitTest(const TextBuffer& buffer, const LayoutComponent& layout,
                          const LayoutComponent::Rect& area, float mx, float my,
                          int scrollOffset, int fontSize, int lineHeight,
                          int columnCount = 1, float columnSpacingPx = 0.0f,
                          int linesPerColumn = 50) const {
        float relY = my - area.y - static_cast<float>(theme::layout::TEXT_PADDING);
        float baseX = area.x + static_cast<float>(theme::layout::TEXT_PADDING);
        if (layout.showLineNumbers) baseX += layout.lineNumberGutterWidth;
        auto measure = [fontSize](const std::string& prefix) {
            return static_cast<float>(
                afterhours::graphics::measure_text(prefix.c_str(), fontSize));
        };
        // Outline mode: the visible row is a heading row; map it to
        // the heading's real buffer row (body rows are not targets).
        // The visible-row estimate uses the same global line height
        // as normal hit-testing (the renderer draws headings taller,
        // so this is approximate for deep outlines — the row mapping
        // clamps to the last heading rather than landing on a body
        // row). The column is measured from the heading's indented
        // start, matching where the renderer draws its text.
        if (layout.showOutline) {
            std::vector<ParagraphStyle> styles;
            for (std::size_t i = 0; i < buffer.lineCount(); ++i) {
                styles.push_back(buffer.lineParagraphStyle(i));
            }
            std::vector<std::size_t> visible =
                outline_view::outlineVisibleRowsFiltered(
                    styles, layout.outlineShowLevel,
                    layout.outlineCollapsedRows);
            if (visible.empty()) return {0, 0};
            int rawVisibleIndex =
                scrollOffset +
                static_cast<int>(
                    std::floor(relY / static_cast<float>(lineHeight)));
            if (rawVisibleIndex < 0) rawVisibleIndex = 0;
            std::size_t row = outline_view::outlineRealRowForVisible(
                visible, static_cast<std::size_t>(rawVisibleIndex),
                buffer.lineCount());
            ParagraphStyle style = buffer.lineParagraphStyle(row);
            // The renderer shifts EVERY heading's text right by
            // the collapse-box width in Outline (a reserved
            // collapse gutter); the column is measured from
            // that same shifted start, matching where the text
            // is drawn.
            int collapseShift = outline_view::kOutlineCollapseShift;
            float indentedX =
                baseX + static_cast<float>(outline_view::outlineIndentPx(style) +
                                            collapseShift);
            std::string line = buffer.lineString(row);
            std::size_t col = hitTestColumn(line, mx - indentedX, measure);
            return {row, col};
        }
        // Columns (paged mode): the click's column comes from its x
        // (column width + spacing, same math as the renderer), the
        // row-in-column from its y, and the real row is pageFirst +
        // column * linesPerColumn + rowInColumn. This is first-page
        // correct and page-consistent for later pages (the renderer
        // shows the page containing scrollOffset).
        if (layout.pageMode == PageMode::Paged && !layout.showOutline &&
            columnCount > 1 && linesPerColumn > 0) {
            int availableWidth = static_cast<int>(area.width) -
                                 2 * theme::layout::TEXT_PADDING;
            float colWidth = columns_math::columnWidth(
                static_cast<float>(availableWidth), columnCount,
                columnSpacingPx);
            float stride = colWidth + columnSpacingPx;
            int column = 0;
            if (stride > 0.0f) {
                column = static_cast<int>(std::floor(
                    (mx - area.x - static_cast<float>(
                                     theme::layout::TEXT_PADDING)) /
                    stride));
            }
            column = std::clamp(column, 0, columnCount - 1);
            float columnX = area.x +
                            static_cast<float>(theme::layout::TEXT_PADDING) +
                            columns_math::columnXOffset(column, colWidth,
                                                        columnSpacingPx);
            int rowInColumn = static_cast<int>(
                std::floor(relY / static_cast<float>(lineHeight)));
            if (rowInColumn < 0) rowInColumn = 0;
            std::size_t pageSize = static_cast<std::size_t>(linesPerColumn) *
                                   static_cast<std::size_t>(columnCount);
            std::size_t pageFirst =
                (static_cast<std::size_t>(std::max(0, scrollOffset)) /
                 pageSize) *
                pageSize;
            std::size_t realRow =
                pageFirst +
                static_cast<std::size_t>(column) *
                    static_cast<std::size_t>(linesPerColumn) +
                static_cast<std::size_t>(rowInColumn);
            std::size_t lineCount = buffer.lineCount();
            if (lineCount == 0) return {0, 0};
            if (realRow >= lineCount) realRow = lineCount - 1;
            std::string line = buffer.lineString(realRow);
            std::size_t col = hitTestColumn(line, mx - columnX, measure);
            return {realRow, col};
        }
        int row = scrollOffset + static_cast<int>(std::floor(relY / static_cast<float>(lineHeight)));
        int lineCount = static_cast<int>(buffer.lineCount());
        if (row < 0) row = 0;
        if (row > lineCount - 1) row = lineCount - 1;
        std::string line = buffer.lineString(static_cast<std::size_t>(row));
        std::size_t col = hitTestColumn(line, mx - baseX, measure);
        return {static_cast<std::size_t>(row), col};
    }

    void for_each_with(afterhours::Entity& /*entity*/, DocumentComponent& doc,
                       CaretComponent& caret, ScrollComponent& scroll,
                       LayoutComponent& layout, MenuComponent& menu,
                       DialogState& dialogs, ToolbarComponent& toolbar,
                       const float) override {
        bool anyMenuOpen = false;
        for (const auto& m : menu.menus) {
            if (m.open) { anyMenuOpen = true; break; }
        }
        bool down = input::isMouseButtonDown(0);
        bool pressed = down && !prevDown_;
        prevDown_ = down;

        if (anyMenuOpen || ScrollbarSystem::anyModalDialogOpen(dialogs)) {
            dragging_ = false;
            return;
        }

        // Click shields (margin comment cards, @ chip popup) are UI
        // elements over the text area whose clicks the UI already
        // handled this frame. A press inside one must not also move
        // the caret: a card click would land the caret on the card's
        // screen position instead of the comment's anchor, and a
        // popup click would move the caret off the chip it just
        // inserted. A press OUTSIDE every shield dismisses the chip
        // popup and then proceeds as a normal text click.
        // The dismiss must NOT fire for inside-shield presses:
        // UI buttons fire on release (is_mouse_click reads
        // just_released), so dismissing on press destroys the
        // popup item before its release can fire — clicking a
        // popup item dismissed the popup without inserting
        // anything (the pre-existing pass_smart_chips tail
        // failure: click_ui chip_item_1 never inserted Tomorrow).
        if (pressed) {
            auto mousePos = input::getMousePosition();
            bool inShield = false;
            for (const auto& shield : dialogs.clickShields) {
                if (mousePos.x >= shield.x &&
                    mousePos.x < shield.x + shield.w &&
                    mousePos.y >= shield.y &&
                    mousePos.y < shield.y + shield.h) {
                    inShield = true;
                    break;
                }
            }
            if (dialogs.showChipPopup && !inShield) {
                dialogs.showChipPopup = false;
            }
            if (inShield) {
                dragging_ = false;
                return;
            }
        }

        // Hit-testing uses the same scaled font as the renderer
        // (displayScale = fit * zoom in paged mode), so a click
        // lands on the column whose glyph is drawn at that x.
        // Pageless displayScale == zoomLevel, so this is unchanged.
        TextStyle style = doc.buffer.textStyle();
        float ds = layout::displayScale(layout);
        int fontSize = ruler_math::scaledFontSize(style.fontSize, ds);
        int lineHeight = fontSize + 4;
        // Column geometry for the hit-test (same helpers as the
        // renderer, so a click lands on the line drawn at that x).
        int columnCount = doc.docSettings.pageSettings.columnCount;
        float columnSpacingPx =
            doc.docSettings.pageSettings.columnSpacing * ds;
        int columnsLinesPerPage = static_cast<int>(
            columns_math::linesPerColumnForPage(
                layout.pageHeight, layout.marginTop, layout.marginBottom,
                ds, lineHeight));
        LayoutComponent::Rect area = layout::effectiveTextArea(layout);
        float sbX = layout.textArea.x + layout.textArea.width - scroll_math::SCROLLBAR_WIDTH;

        auto mouse = input::getMousePosition();
        bool inTextArea = mouse.x >= area.x && mouse.x < sbX && mouse.y >= area.y &&
                          mouse.y < area.y + area.height;

        // Table cells (Batch 8): same origin geometry as
        // renderDocumentTables (effective area + TEXT_PADDING,
        // line * lineHeight from the scrolled line), so a click
        // lands in the cell drawn under it. A press inside a
        // table enters cell editing at the clicked offset;
        // a press outside any table exits it and falls through
        // to the body click below. Not in Print Preview (read-
        // only) or Outline mode (tables are not the outline).
        auto cellOffsetAtMouse = [&](std::size_t tableIndex,
                                     CellPosition cell, float mx,
                                     float my) {
            const Table& table = doc.tables[tableIndex].second;
            std::size_t lineNum =
                doc.tables[tableIndex].first;
            float tableX = area.x +
                           static_cast<float>(theme::layout::TEXT_PADDING);
            float tableY = area.y +
                           static_cast<float>(theme::layout::TEXT_PADDING) +
                           static_cast<float>(
                               static_cast<int>(lineNum) - scroll.offset) *
                               static_cast<float>(lineHeight);
            Table::CellBounds bounds = table.cellBounds(cell);
            const TableCell& cellData = table.cell(cell);
            float relX = mx - (tableX + bounds.x +
                               static_cast<float>(cellData.paddingLeft));
            float relY = my - (tableY + bounds.y +
                               static_cast<float>(cellData.paddingTop));
            auto lines = cell_edit::wrapCellText(
                cellData.content, cell_edit::textWidth(table, cell),
                cellData.textStyle.fontSize, cmd::cellMeasure());
            return cell_edit::caretOffsetAtPoint(
                lines, relX, relY, cellData.textStyle.fontSize,
                cmd::cellMeasure());
        };
        if (pressed && inTextArea && !layout.previewMode &&
            !layout.showOutline) {
            bool hitTable = false;
            for (std::size_t ti = 0; ti < doc.tables.size(); ++ti) {
                const Table& table = doc.tables[ti].second;
                if (table.isEmpty()) continue;
                std::size_t lineNum = doc.tables[ti].first;
                if (lineNum < static_cast<std::size_t>(scroll.offset)) {
                    continue;
                }
                float tableX =
                    area.x +
                    static_cast<float>(theme::layout::TEXT_PADDING);
                float tableY =
                    area.y +
                    static_cast<float>(theme::layout::TEXT_PADDING) +
                    static_cast<float>(
                        static_cast<int>(lineNum) - scroll.offset) *
                        static_cast<float>(lineHeight);
                if (mouse.x < tableX ||
                    mouse.x >= tableX + table.totalWidth() ||
                    mouse.y < tableY ||
                    mouse.y >= tableY + table.totalHeight()) {
                    continue;
                }
                hitTable = true;
                CellPosition clicked = table.cellAtPoint(
                    mouse.x - tableX, mouse.y - tableY);
                clicked = table.getMergeParent(clicked);
                std::size_t off =
                    cellOffsetAtMouse(ti, clicked, mouse.x, mouse.y);
                // Multi-click counting, same rule as the body
                // click below (0.5s, within 4px).
                double now = afterhours::graphics::get_time();
                if (now - lastClickTime_ < 0.5 &&
                    std::abs(mouse.x - lastClickX_) < 4.0f &&
                    std::abs(mouse.y - lastClickY_) < 4.0f) {
                    clickCount_ += 1;
                } else {
                    clickCount_ = 1;
                }
                lastClickTime_ = now;
                lastClickX_ = mouse.x;
                lastClickY_ = mouse.y;
                bool shiftDown =
                    test_input::is_key_down(
                        afterhours::keys::LEFT_SHIFT) ||
                    test_input::is_key_down(
                        afterhours::keys::RIGHT_SHIFT);
                bool sameCell =
                    doc.cellEdit.active &&
                    doc.cellEdit.tableIndex == ti &&
                    doc.cellEdit.cell == clicked;
                if (!sameCell) {
                    cmd::beginCellEdit(doc, ti, clicked, off);
                } else if (shiftDown) {
                    if (!doc.cellEdit.hasSelection) {
                        doc.cellEdit.anchorOffset =
                            doc.cellEdit.caretOffset;
                    }
                    doc.cellEdit.caretOffset = off;
                    doc.cellEdit.hasSelection =
                        doc.cellEdit.anchorOffset != off;
                } else {
                    doc.cellEdit.caretOffset = off;
                    doc.cellEdit.clearSelection();
                }
                if (clickCount_ == 2) {
                    Table& editTable = doc.tables[ti].second;
                    cell_edit::selectWord(editTable, doc.cellEdit);
                    cellDragging_ = false;
                } else if (clickCount_ >= 3) {
                    Table& editTable = doc.tables[ti].second;
                    cell_edit::selectAllInCell(editTable,
                                              doc.cellEdit);
                    cellDragging_ = false;
                } else {
                    dragAnchorCellOffset_ =
                        doc.cellEdit.hasSelection
                            ? doc.cellEdit.anchorOffset
                            : doc.cellEdit.caretOffset;
                    cellDragging_ = true;
                }
                dragging_ = false;
                caret::resetBlink(caret);
                return;
            }
            if (!hitTable && doc.cellEdit.active) {
                cmd::endCellEdit(doc);
                cellDragging_ = false;
            }
        }

        if (pressed && inTextArea) {
            // Format Painter: an armed painter applies the captured
            // style on this click (click or the press that starts a
            // click-drag) through the command layer, then disarms;
            // the click still places the caret / starts the drag.
            // Not in Print Preview: the preview toolbar replaces the
            // Standard toolbar, so the painter cannot be armed there.
            if (!layout.previewMode) {
                cmd::applyFormatPainter(doc, toolbar);
            }
            // Outline collapse box: a press on a heading's +/-
            // box toggles that heading's collapse instead of
            // moving the caret. The row band uses the same
            // global-line-height approximation as hitTest (the
            // renderer draws headings taller, so the band is
            // approximate for deep outlines), and the box x uses
            // the same outlineIndent + collapse-box geometry as
            // the renderer. A press below the last visible row
            // is NOT a box press (hitTest would clamp it to the
            // last heading; toggling there would be a surprise).
            if (layout.showOutline) {
                std::vector<ParagraphStyle> outlineStyles;
                for (std::size_t i = 0; i < doc.buffer.lineCount(); ++i) {
                    outlineStyles.push_back(
                        doc.buffer.lineParagraphStyle(i));
                }
                std::vector<std::size_t> outlineVisible =
                    outline_view::outlineVisibleRowsFiltered(
                        outlineStyles, layout.outlineShowLevel,
                        layout.outlineCollapsedRows);
                float outlineRelY =
                    mouse.y - area.y -
                    static_cast<float>(theme::layout::TEXT_PADDING);
                int outlineVisibleIndex =
                    scroll.offset +
                    static_cast<int>(std::floor(
                        outlineRelY / static_cast<float>(lineHeight)));
                if (outlineVisibleIndex >= 0 &&
                    static_cast<std::size_t>(outlineVisibleIndex) <
                        outlineVisible.size()) {
                    std::size_t outlineRow =
                        outlineVisible[static_cast<std::size_t>(
                            outlineVisibleIndex)];
                    if (outline_view::outlineHasSubordinateHeadings(
                            outlineStyles, outlineRow)) {
                        float outlineBaseX =
                            area.x + static_cast<float>(
                                         theme::layout::TEXT_PADDING);
                        if (layout.showLineNumbers) {
                            outlineBaseX += layout.lineNumberGutterWidth;
                        }
                        float boxX =
                            outlineBaseX +
                            static_cast<float>(outline_view::outlineIndentPx(
                                doc.buffer.lineParagraphStyle(outlineRow)));
                        if (mouse.x >= boxX &&
                            mouse.x < boxX + static_cast<float>(
                                                 outline_view::kOutlineCollapseBoxSize)) {
                            // Move the caret to the heading first,
                            // so the Expand/Collapse toolbar button
                            // and the box agree about the heading
                            // at the caret after a box press.
                            doc.buffer.clearSelection();
                            doc.buffer.setCaret({outlineRow, 0});
                            cmd::toggleOutlineCollapse(doc, layout);
                            caret::resetBlink(caret);
                            return;
                        }
                    }
                }
            }
            bool shiftDown = test_input::is_key_down(afterhours::keys::LEFT_SHIFT) ||
                             test_input::is_key_down(afterhours::keys::RIGHT_SHIFT);
            CaretPosition pos = hitTest(doc.buffer, layout, area, mouse.x, mouse.y,
                                        scroll.offset, fontSize, lineHeight,
                                        columnCount, columnSpacingPx,
                                        columnsLinesPerPage);

            // Multi-click: second press within 0.5s near the same point.
            double now = afterhours::graphics::get_time();
            if (now - lastClickTime_ < 0.5 &&
                std::abs(mouse.x - lastClickX_) < 4.0f &&
                std::abs(mouse.y - lastClickY_) < 4.0f) {
                clickCount_ += 1;
            } else {
                clickCount_ = 1;
            }
            lastClickTime_ = now;
            lastClickX_ = mouse.x;
            lastClickY_ = mouse.y;

            if (clickCount_ == 2) {
                doc.buffer.selectWordAt(pos);
                dragging_ = false;
            } else if (clickCount_ >= 3) {
                // No reset here: the next press in the same burst must
                // still count as >= 3 (resetting to 0 made a triple-click
                // end as a word selection). The time-gap branch above
                // resets to 1 after a 0.5s pause.
                doc.buffer.selectParagraphAt(pos);
                dragging_ = false;
            } else if (shiftDown) {
                if (!doc.buffer.hasSelection()) {
                    doc.buffer.setSelectionAnchor(doc.buffer.caret());
                }
                dragAnchor_ = doc.buffer.selectionStart();
                doc.buffer.setCaret(pos);
                doc.buffer.updateSelectionToCaret();
                dragging_ = true;
                autoScrolledThisDrag_ = false;
            } else {
                doc.buffer.clearSelection();
                doc.buffer.setCaret(pos);
                dragAnchor_ = pos;
                dragging_ = true;
                autoScrolledThisDrag_ = false;
            }
            caret::resetBlink(caret);
            return;
        }

        // Cell drag in progress: extend the within-cell selection
        // to the drag position (clamped to the current cell by
        // cellOffsetAtMouse). A zero-length drag collapses to a
        // plain caret on release, like the body drag below.
        if (cellDragging_) {
            if (!down) {
                if (doc.cellEdit.active) {
                    std::size_t off = cellOffsetAtMouse(
                        doc.cellEdit.tableIndex, doc.cellEdit.cell,
                        mouse.x, mouse.y);
                    doc.cellEdit.anchorOffset = dragAnchorCellOffset_;
                    doc.cellEdit.caretOffset = off;
                    doc.cellEdit.hasSelection =
                        dragAnchorCellOffset_ != off;
                }
                cellDragging_ = false;
                caret::resetBlink(caret);
                return;
            }
            if (doc.cellEdit.active && inTextArea) {
                std::size_t off = cellOffsetAtMouse(
                    doc.cellEdit.tableIndex, doc.cellEdit.cell,
                    mouse.x, mouse.y);
                doc.cellEdit.anchorOffset = dragAnchorCellOffset_;
                doc.cellEdit.caretOffset = off;
                doc.cellEdit.hasSelection =
                    dragAnchorCellOffset_ != off;
                caret::resetBlink(caret);
            }
            return;
        }

        // Drag in progress: extend the selection, auto-scrolling past edges.
        if (dragging_) {
            if (!down) {
                // Release: land the selection at the release position,
                // and guarantee at least one auto-scroll step if the
                // drag ended outside but no move frame ever fired
                // (drag phases racing the auto-release can drop it).
                // The flag only suppresses a duplicate release step:
                // a drag that already auto-scrolled on move frames
                // gets no extra step here.
                bool endedOutside =
                    mouse.y < area.y || mouse.y > area.y + area.height;
                if (endedOutside && !autoScrolledThisDrag_) {
                    int totalLines = static_cast<int>(doc.buffer.lineCount());
                    int maxScroll = std::max(0, totalLines - scroll.visibleLines);
                    if (mouse.y < area.y && scroll.offset > 0) {
                        scroll.offset -= 1;
                    } else if (mouse.y > area.y + area.height &&
                               scroll.offset < maxScroll) {
                        scroll.offset += 1;
                    }
                }
                float clampedY =
                    std::clamp(mouse.y, area.y, area.y + area.height - 1.0f);
                CaretPosition pos = hitTest(doc.buffer, layout, area, mouse.x,
                                            clampedY, scroll.offset, fontSize,
                                            lineHeight, columnCount,
                                            columnSpacingPx,
                                            columnsLinesPerPage);
                if (!doc.buffer.hasSelection()) {
                    doc.buffer.setCaret(dragAnchor_);
                    doc.buffer.setSelectionAnchor(dragAnchor_);
                }
                doc.buffer.setCaret(pos);
                doc.buffer.updateSelectionToCaret();
                caret::resetBlink(caret);
                dragging_ = false;
                // Collapse a zero-length drag to a plain caret.
                if (doc.buffer.hasSelection() &&
                    doc.buffer.getSelectedText().empty()) {
                    doc.buffer.clearSelection();
                }
                return;
            }
            if (!inTextArea) {
                // Dragged past the top/bottom edge: scroll one line per
                // frame toward the pointer, then recompute in bounds.
                int totalLines = static_cast<int>(doc.buffer.lineCount());
                int maxScroll = std::max(0, totalLines - scroll.visibleLines);
                if (mouse.y < area.y && scroll.offset > 0) {
                    scroll.offset -= 1;
                    autoScrolledThisDrag_ = true;
                } else if (mouse.y > area.y + area.height && scroll.offset < maxScroll) {
                    scroll.offset += 1;
                    autoScrolledThisDrag_ = true;
                }
            }
            float clampedY = std::clamp(mouse.y, area.y, area.y + area.height - 1.0f);
            CaretPosition pos = hitTest(doc.buffer, layout, area, mouse.x, clampedY,
                                        scroll.offset, fontSize, lineHeight,
                                        columnCount, columnSpacingPx,
                                        columnsLinesPerPage);
            // First drag move after a plain click: open the selection at
            // the stored anchor. Shift-click already opened it.
            if (!doc.buffer.hasSelection()) {
                doc.buffer.setCaret(dragAnchor_);
                doc.buffer.setSelectionAnchor(dragAnchor_);
            }
            doc.buffer.setCaret(pos);
            doc.buffer.updateSelectionToCaret();
            caret::resetBlink(caret);
        }
    }
};

}  // namespace ecs
