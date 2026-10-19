#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <sstream>

#include "../../vendor/afterhours/src/core/system.h"
#include <afterhours/src/drawing_helpers.h>
#include "../editor/borders_shading.h"
#include "../editor/columns_math.h"
#include "../editor/drop_cap.h"
#include "../editor/text_layout.h"
#include "../editor/normal_view.h"
#include "../editor/outline_view.h"
#include "../editor/revisions.h"
#include "../editor/scroll_math.h"
#include "../editor/table.h"
#include "../input/action_map.h"
#include "../rl.h"
#include "../testing/deferred_screenshot.h"
#include "../settings.h"
// test_input:: available via rl.h -> external.h
#include "../ui/theme.h"
#include "../ui/ui_context.h"  // for toast_notify
#include "../ui/win95_widgets.h"
#include "../ui/menu_setup.h"
#include "../ui/input.h"    // Centralized afterhours input wrappers
#include <afterhours/src/drawing_helpers.h>  // Direct afterhours drawing functions
#include "../util/drawing.h"
#include "../util/logging.h"
#include "component_helpers.h"
#include "components.h"

namespace ecs {

// Helper to draw text and register it for E2E testing
// Uses the UI font for consistent rendering across all UI elements.
// NOTE: render-phase registrations are cleared at the start of
// the next frame, before E2E checks read the registry, so
// expect_text CANNOT see text registered only here — document
// text must also go through DocumentTextRegistration (update
// phase, e2e_commands.h). This registration is best-effort.
inline void drawTextWithRegistry(const char* text, int x, int y, int fontSize,
                                  afterhours::Color color) {
    theme::DrawUIText(text, x, y, fontSize, color);
    test_input::register_visible_text(text);
}

// Helper to draw text with font and register it for E2E testing
inline void drawTextExWithRegistry(afterhours::Font font, const char* text, 
                                    vec2 pos, float fontSize, 
                                    float spacing, afterhours::Color color) {
    afterhours::draw_text_ex(font, text, pos, fontSize, spacing,
                             afterhours::Color{color.r, color.g, color.b, color.a});
    test_input::register_visible_text(text);
}

// Draw a page background with shadow (for paged mode).
// showTextBoundaries gates the faint margin guides: Word 95 shows
// them only with Tools > Options > View "Text boundaries" enabled.
inline void drawPageBackground(const LayoutComponent& layout,
                               bool showTextBoundaries = false) {
    if (layout.pageMode != PageMode::Paged) return;

    float pageY = layout.textArea.y + 10.0f;  // 10px margin from top

    // Draw page shadow
    Rectangle shadowRect = {layout.pageOffsetX + 4.0f, pageY + 4.0f,
                                    layout.pageDisplayWidth,
                                    layout.pageDisplayHeight};
    afterhours::draw_rectangle(shadowRect, {100, 100, 100, 128});

    // Draw page (white background)
    Rectangle pageRect = {layout.pageOffsetX, pageY,
                                  layout.pageDisplayWidth,
                                  layout.pageDisplayHeight};
    afterhours::draw_rectangle(pageRect, {255, 255, 255, 255});

    // Draw page border
    afterhours::draw_rectangle_outline(pageRect, {80, 80, 80, 255}, 1.0f);

    // Margin boundary guides (Word 95 "Text boundaries"): faint
    // lines at the text-area edges. Drawn only when the option is
    // on — by default the page is plain white. Margins from the ONE
    // scale source: margin(points) * displayScale, so the margin
    // lines sit exactly on the ruler's inch marks.
    if (!showTextBoundaries) return;
    float ds = layout::displayScale(layout);
    float leftScaled = layout.marginLeft * ds;
    float rightScaled = layout.marginRight * ds;
    float topScaled = layout.marginTop * ds;
    float bottomScaled = layout.marginBottom * ds;
    afterhours::Color marginColor = afterhours::Color{200, 200, 200, 100};

    // Left margin
    afterhours::draw_line(static_cast<int>(layout.pageOffsetX + leftScaled),
                     static_cast<int>(pageY),
                     static_cast<int>(layout.pageOffsetX + leftScaled),
                     static_cast<int>(pageY + layout.pageDisplayHeight),
                     marginColor);

    // Right margin
    afterhours::draw_line(static_cast<int>(layout.pageOffsetX +
                                      layout.pageDisplayWidth - rightScaled),
                     static_cast<int>(pageY),
                     static_cast<int>(layout.pageOffsetX +
                                      layout.pageDisplayWidth - rightScaled),
                     static_cast<int>(pageY + layout.pageDisplayHeight),
                     marginColor);

    // Top margin
    afterhours::draw_line(
        static_cast<int>(layout.pageOffsetX),
        static_cast<int>(pageY + topScaled),
        static_cast<int>(layout.pageOffsetX + layout.pageDisplayWidth),
        static_cast<int>(pageY + topScaled), marginColor);

    // Bottom margin
    afterhours::draw_line(
        static_cast<int>(layout.pageOffsetX),
        static_cast<int>(pageY + layout.pageDisplayHeight - bottomScaled),
        static_cast<int>(layout.pageOffsetX + layout.pageDisplayWidth),
        static_cast<int>(pageY + layout.pageDisplayHeight - bottomScaled),
        marginColor);
}

// Dashed rectangle outline (Word 95 header/footer editing
// boundary): 6px-on / 4px-off segments via draw_line, the
// same segmented-line technique as drawTableGridlineEdge
// below, but dashes (not dots) and a caller-chosen color.
// Far edges sit 1px inside the rect, matching a 1px
// draw_rectangle_outline of the same rect.
inline void drawDashedRectOutline(float x, float y, float w, float h,
                                  afterhours::Color color) {
    float x2 = x + w - 1.0f;
    float y2 = y + h - 1.0f;
    for (float px = x; px < x2; px += 10.0f) {
        float end = std::min(px + 6.0f, x2);
        afterhours::draw_line(static_cast<int>(px), static_cast<int>(y),
                              static_cast<int>(end), static_cast<int>(y),
                              color);
        afterhours::draw_line(static_cast<int>(px), static_cast<int>(y2),
                              static_cast<int>(end), static_cast<int>(y2),
                              color);
    }
    for (float py = y; py < y2; py += 10.0f) {
        float end = std::min(py + 6.0f, y2);
        afterhours::draw_line(static_cast<int>(x), static_cast<int>(py),
                              static_cast<int>(x), static_cast<int>(end),
                              color);
        afterhours::draw_line(static_cast<int>(x2), static_cast<int>(py),
                              static_cast<int>(x2), static_cast<int>(end),
                              color);
    }
}

// Draw one header / footer band (Batch 9): the buffer's text,
// wrapped at the band width (cell_edit::wrapCellText, the same
// wrap the mouse hit-test uses), left-aligned in the margin
// band from layout::headerFooterAreas; a page-number field
// (HeaderFooter::center.showPageNumber) draws "Page N" (real
// per-page index, HeaderFooter::formatPageNumber) centered
// on the FIRST line's baseline, sharing it with the band
// text (Word 95: left text + centered Page N on one line) —
// not on its own line after the text. differentFirstPage
// skips the band on page 1 — except the ACTIVE band while
// editing it (typing into an invisible band would be
// unusable). While editing, the active band gets a dashed
// light-gray outline, the selection highlight, and the
// blinking caret. Pageless / Normal never reach here (the
// caller gates on pageMode == Paged): Word Normal shows
// headers/footers only in Page Layout.
inline void renderHeaderFooter(const DocumentComponent& doc,
                               const LayoutComponent& layout, int page,
                               int totalPages, bool caretVisible) {
    if (layout.pageMode != PageMode::Paged) return;
    float ds = layout::displayScale(layout);
    auto [headerArea, footerArea] = layout::headerFooterAreas(layout);
    auto measure = [](const std::string& text, int fontSize) {
        return static_cast<float>(
            afterhours::graphics::measure_text(text.c_str(), fontSize));
    };
    const afterhours::Color textColor{0, 0, 0, 255};
    for (int target = 0; target < 2; ++target) {
        bool isFooter = target == 1;
        HeaderEditTarget bandTarget = isFooter ? HeaderEditTarget::Footer
                                               : HeaderEditTarget::Header;
        const TextBuffer& hfBuffer =
            isFooter ? doc.footerBuffer : doc.headerBuffer;
        const HeaderFooter& config = isFooter ? doc.docSettings.footer
                                              : doc.docSettings.header;
        const LayoutComponent::Rect& band =
            isFooter ? footerArea : headerArea;
        bool editing = layout.headerEditTarget == bandTarget;
        if (page == 1 && config.differentFirstPage && !editing) {
            continue;
        }
        int fontSize = ruler_math::scaledFontSize(
            hfBuffer.textStyle().fontSize, ds);
        int lineHeight = fontSize + 4;
        float textX = band.x +
                      static_cast<float>(theme::layout::TEXT_PADDING);
        float textY = band.y + 4.0f;
        float wrapWidth =
            band.width -
            2.0f * static_cast<float>(theme::layout::TEXT_PADDING);
        auto lines = cell_edit::wrapCellText(hfBuffer.getText(), wrapWidth,
                                             fontSize, measure);
        // Page-number field (Batch 9 fix round): shares the
        // FIRST line's baseline with the band text — it used
        // to draw on its own line after the text, ~one
        // lineHeight lower. Computed before the line loop so
        // the first line can truncate before it: Page N keeps
        // the center and the text truncates on a collision,
        // never the reverse. Same font size / line height as
        // the band text (the footer buffer's), never a
        // separately computed y.
        //
        // Page N geometry uses the DRAWN measure
        // (theme::MeasureUIText, the same scaled measure
        // the draw call uses), not the band's wrap/caret
        // `measure`: the draw scales the font by ui_scale,
        // so an unscaled width would center Page N ~off by
        // the scale error and the truncated line would
        // still overrun it at ui_scale != 1. Wrap, caret,
        // and selection keep the unscaled measure — the
        // mouse hit-test shares it, and re-plumbing the
        // global measure convention is out of scope.
        std::string pageText;
        float pageX = 0.0f;
        bool showPageNumber = config.center.showPageNumber;
        if (showPageNumber) {
            pageText = "Page " + HeaderFooter::formatPageNumber(
                                     page, config.center.format);
            if (config.center.showTotalPages) {
                pageText += " of " + std::to_string(totalPages);
            }
            float pageW = static_cast<float>(
                theme::MeasureUIText(pageText.c_str(), fontSize));
            pageX = band.x + (band.width - pageW) / 2.0f;
        }
        std::size_t selStartOff = 0;
        std::size_t selEndOff = 0;
        if (editing && hfBuffer.hasSelection()) {
            selStartOff = hfBuffer.offsetForPosition(
                hfBuffer.selectionStart());
            selEndOff =
                hfBuffer.offsetForPosition(hfBuffer.selectionEnd());
        }
        float y = textY;
        for (std::size_t lineIdx = 0; lineIdx < lines.size(); ++lineIdx) {
            const auto& line = lines[lineIdx];
            if (y + static_cast<float>(lineHeight) >
                band.y + band.height) {
                break;
            }
            // First-line text truncates before the centered
            // Page N (8px gap, in drawn space); later lines
            // cannot collide with it. Pop whole UTF-8
            // characters only. The truncation measure is
            // the drawn measure (see above): with the
            // unscaled wrap measure, the drawn line would
            // still overrun Page N at ui_scale != 1.
            std::string drawText = line.text;
            if (lineIdx == 0 && showPageNumber) {
                float avail = pageX - 8.0f - textX;
                while (!drawText.empty() &&
                       static_cast<float>(theme::MeasureUIText(
                           drawText.c_str(), fontSize)) > avail) {
                    drawText.pop_back();
                    while (!drawText.empty() &&
                           (static_cast<unsigned char>(drawText.back()) &
                            0xC0) == 0x80) {
                        drawText.pop_back();
                    }
                }
            }
            if (editing && hfBuffer.hasSelection() &&
                line.endOffset > selStartOff &&
                line.startOffset < selEndOff) {
                std::size_t segBegin =
                    std::max(line.startOffset, selStartOff);
                std::size_t segEnd =
                    std::min(line.endOffset, selEndOff);
                // Selection highlight covers only the drawn
                // (possibly truncated) text, never Page N.
                if (lineIdx == 0 && showPageNumber) {
                    segEnd = std::min(
                        segEnd, line.startOffset + drawText.size());
                }
                if (segEnd > segBegin) {
                    std::string beforeSeg = line.text.substr(
                        0, std::min(segBegin - line.startOffset,
                                    line.text.size()));
                    std::string segText = line.text.substr(
                        std::min(segBegin - line.startOffset,
                                 line.text.size()),
                        std::min(segEnd - segBegin,
                                 line.text.size() -
                                     std::min(segBegin - line.startOffset,
                                              line.text.size())));
                    float selX =
                        textX + measure(beforeSeg, fontSize);
                    float selW = measure(segText, fontSize);
                    afterhours::draw_rectangle(
                        Rectangle{selX, y, selW,
                                  static_cast<float>(lineHeight)},
                        theme::SELECTION_BG);
                }
            }
            if (!drawText.empty()) {
                drawTextWithRegistry(drawText.c_str(),
                                     static_cast<int>(textX),
                                     static_cast<int>(y), fontSize,
                                     textColor);
            }
            y += static_cast<float>(lineHeight);
        }
        // Page N on the FIRST line's baseline (textY): the
        // same y the band text starts on, from the same font
        // size / line height — not the y left over after the
        // text lines.
        if (showPageNumber &&
            textY + static_cast<float>(lineHeight) <=
                band.y + band.height) {
            drawTextWithRegistry(pageText.c_str(),
                                 static_cast<int>(pageX),
                                 static_cast<int>(textY), fontSize,
                                 textColor);
        }
        if (editing) {
            // Word 95 shows the active header/footer area
            // with a light dashed boundary, not a hard
            // rectangle.
            drawDashedRectOutline(band.x, band.y, band.width,
                                  band.height,
                                  afterhours::Color{160, 160, 160, 255});
        }
        if (editing && caretVisible) {
            std::size_t caretOff = hfBuffer.caretOffset();
            std::size_t lineIdx =
                lines.empty() ? 0 : lines.size() - 1;
            for (std::size_t li = 0; li < lines.size(); ++li) {
                if (caretOff < lines[li].endOffset) {
                    lineIdx = li;
                    break;
                }
                if (caretOff == lines[li].endOffset &&
                    (li + 1 >= lines.size() ||
                     lines[li + 1].startOffset > caretOff)) {
                    lineIdx = li;
                    break;
                }
            }
            const auto& caretLine = lines[lineIdx];
            std::size_t inLine = caretOff >= caretLine.startOffset
                                     ? caretOff - caretLine.startOffset
                                     : 0;
            if (inLine > caretLine.text.size()) {
                inLine = caretLine.text.size();
            }
            float caretX =
                textX + measure(caretLine.text.substr(0, inLine),
                                fontSize);
            float caretY =
                textY + static_cast<float>(lineIdx) *
                            static_cast<float>(lineHeight);
            afterhours::draw_rectangle(
                Rectangle{caretX, caretY, 2.0f,
                          static_cast<float>(lineHeight)},
                theme::CARET_COLOR);
        }
    }
}

// Draw a single border edge between two points using the given style
inline void drawBorderEdge(float x1, float y1, float x2, float y2,
                           BorderStyle style, afterhours::Color color) {
    float thickness = borderStyleThickness(style);
    if (thickness <= 0.0f) return;
    if (thickness <= 1.0f) {
        afterhours::draw_line(static_cast<int>(x1), static_cast<int>(y1),
                              static_cast<int>(x2), static_cast<int>(y2), color);
    } else {
        afterhours::draw_line_ex({x1, y1}, {x2, y2}, thickness, color);
    }
}

// Dotted gridline edge for table cells (Word 95 gridlines):
// #808080, 2px on / 2px off along the axis-aligned edge. Used
// for None/Thin sides (tableBorderIsGridline); explicitly-set
// sides keep the black border path below.
inline void drawTableGridlineEdge(float x1, float y1, float x2, float y2) {
    const afterhours::Color gridColor{128, 128, 128, 255};  // #808080
    if (y1 == y2) {  // Horizontal edge
        for (float px = x1; px < x2; px += 4.0f) {
            float end = std::min(px + 2.0f, x2);
            afterhours::draw_line(static_cast<int>(px), static_cast<int>(y1),
                                  static_cast<int>(end), static_cast<int>(y1),
                                  gridColor);
        }
    } else {  // Vertical edge
        for (float py = y1; py < y2; py += 4.0f) {
            float end = std::min(py + 2.0f, y2);
            afterhours::draw_line(static_cast<int>(x1), static_cast<int>(py),
                                  static_cast<int>(x1), static_cast<int>(end),
                                  gridColor);
        }
    }
}

// Render a table at a specific position. cellEdit is the
// active cell caret for THIS table (nullptr for the others);
// caretVisible is the blink state — the cell caret blinks
// with the same timer as the body caret.
inline void renderTable(const Table& table, float tableX, float tableY,
                        const CellEditState* cellEdit,
                        bool caretVisible) {
    if (table.isEmpty()) return;
    auto measure = [](const std::string& text, int fontSize) {
        return static_cast<float>(
            afterhours::graphics::measure_text(text.c_str(), fontSize));
    };

    // Draw table cells
    for (std::size_t row = 0; row < table.rowCount(); ++row) {
        for (std::size_t col = 0; col < table.colCount(); ++col) {
            const TableCell& cell = table.cell(row, col);
            
            // Skip cells that are part of a merge (not the parent)
            if (cell.isMerged) continue;
            
            // Get cell bounds
            Table::CellBounds bounds = table.cellBounds({row, col});
            float cellX = tableX + bounds.x;
            float cellY = tableY + bounds.y;
            float cellW = bounds.width;
            float cellH = bounds.height;
            
            // Draw cell background
            afterhours::Color bgColor = {cell.backgroundColor.r, cell.backgroundColor.g,
                                     cell.backgroundColor.b, cell.backgroundColor.a};
            afterhours::draw_rectangle(Rectangle{cellX, cellY, cellW, cellH}, bgColor);
            
            // Draw cell borders: default/gridline sides draw the
            // light-gray dotted gridline; explicitly-set sides
            // keep the black border. (renderDocumentTables
            // delegates here; Double/Dashed/Dotted still draw
            // nothing — borderStyleThickness returns 0 for them,
            // a pre-existing limitation, not changed here.)
            afterhours::Color borderColor = afterhours::Color{0, 0, 0, 255};
            auto drawTableBorder = [&](float x1, float y1, float x2, float y2,
                                       BorderStyle style) {
                if (tableBorderIsGridline(style)) {
                    drawTableGridlineEdge(x1, y1, x2, y2);
                } else {
                    drawBorderEdge(x1, y1, x2, y2, style, borderColor);
                }
            };
            drawTableBorder(cellX, cellY, cellX + cellW, cellY, cell.borders.top);
            drawTableBorder(cellX, cellY + cellH, cellX + cellW, cellY + cellH, cell.borders.bottom);
            drawTableBorder(cellX, cellY, cellX, cellY + cellH, cell.borders.left);
            drawTableBorder(cellX + cellW, cellY, cellX + cellW, cellY + cellH, cell.borders.right);
            
            // Draw cell content, wrapped inside the cell (the
            // row height was recomputed from these same wrapped
            // lines at edit time, so the text fits its row).
            int textX = static_cast<int>(cellX) + cell.paddingLeft;
            int textY = static_cast<int>(cellY) + cell.paddingTop;
            int fontSize = cell.textStyle.fontSize;
            int lineHeight = cellLineHeight(fontSize);
            afterhours::Color textColor = {cell.textStyle.textColor.r, cell.textStyle.textColor.g,
                                      cell.textStyle.textColor.b, cell.textStyle.textColor.a};
            auto lines = cell_edit::wrapCellText(
                cell.content, cell_edit::textWidth(table, {row, col}),
                fontSize, measure);
            for (std::size_t li = 0; li < lines.size(); ++li) {
                if (lines[li].text.empty()) continue;
                afterhours::draw_text(
                    lines[li].text.c_str(), textX,
                    textY + static_cast<int>(li) * lineHeight, fontSize,
                    textColor);
            }

            // Active cell: within-cell selection (navy bg + white
            // glyphs, like the body's inverse selection) and the
            // blinking caret bar at the caret offset.
            bool isActiveCell =
                cellEdit != nullptr && cellEdit->active &&
                cellEdit->cell.row == row && cellEdit->cell.col == col;
            if (isActiveCell && cellEdit->hasSelection) {
                std::size_t selStart = cellEdit->selectionStart();
                std::size_t selEnd = cellEdit->selectionEnd();
                for (std::size_t li = 0; li < lines.size(); ++li) {
                    std::size_t ovStart =
                        std::max(selStart, lines[li].startOffset);
                    std::size_t ovEnd =
                        std::min(selEnd, lines[li].endOffset);
                    if (ovStart >= ovEnd) continue;
                    std::string before =
                        lines[li].text.substr(
                            0, ovStart - lines[li].startOffset);
                    std::string selected =
                        lines[li].text.substr(
                            ovStart - lines[li].startOffset,
                            ovEnd - ovStart);
                    float selX = static_cast<float>(textX) +
                                 measure(before, fontSize);
                    float selW = measure(selected, fontSize);
                    float selY = static_cast<float>(textY) +
                                 static_cast<float>(li) *
                                     static_cast<float>(lineHeight);
                    afterhours::draw_rectangle(
                        Rectangle{selX, selY, selW,
                                  static_cast<float>(lineHeight)},
                        theme::SELECTION_BG);
                    afterhours::draw_text(
                        selected.c_str(), static_cast<int>(selX),
                        static_cast<int>(selY), fontSize,
                        theme::SELECTION_TEXT);
                }
            }
            if (isActiveCell && caretVisible) {
                // Visual line holding the caret (same boundary
                // rule as cell_edit::moveUp/Down).
                std::size_t lineIdx = lines.size() - 1;
                for (std::size_t li = 0; li < lines.size(); ++li) {
                    if (cellEdit->caretOffset < lines[li].endOffset) {
                        lineIdx = li;
                        break;
                    }
                    if (cellEdit->caretOffset == lines[li].endOffset &&
                        (li + 1 >= lines.size() ||
                         lines[li + 1].startOffset >
                             cellEdit->caretOffset)) {
                        lineIdx = li;
                        break;
                    }
                }
                std::size_t caretInLine =
                    cellEdit->caretOffset < lines[lineIdx].startOffset
                        ? 0
                        : cellEdit->caretOffset -
                              lines[lineIdx].startOffset;
                std::string beforeCaret = cell.content.substr(
                    lines[lineIdx].startOffset, caretInLine);
                float caretX = static_cast<float>(textX) +
                               measure(beforeCaret, fontSize);
                float caretY = static_cast<float>(textY) +
                               static_cast<float>(lineIdx) *
                                   static_cast<float>(lineHeight);
                afterhours::draw_rectangle(
                    Rectangle{caretX, caretY, 2.0f,
                              static_cast<float>(lineHeight)},
                    theme::CARET_COLOR);
            }

        }
    }

    // Active editing cell outline: navy #000080, 1px — the Word 95
    // selection colour, not electric blue. Drawn AFTER the cell
    // loop, not per-cell: neighbouring cells share the active
    // cell's right/bottom edges, and their dotted gridlines are
    // drawn in row-major order AFTER the active cell, so a
    // per-cell outline came out navy on the top/left and
    // navy/gray-dotted on the right/bottom (the neighbours'
    // gridlines overpainted it). Deferred, all four edges stay
    // solid navy.
    if (cellEdit != nullptr && cellEdit->active &&
        cellEdit->cell.row < table.rowCount() &&
        cellEdit->cell.col < table.colCount() &&
        !table.cell(cellEdit->cell).isMerged) {
        Table::CellBounds bounds = table.cellBounds(cellEdit->cell);
        afterhours::draw_rectangle_outline(
            Rectangle{tableX + bounds.x, tableY + bounds.y, bounds.width,
                      bounds.height},
            afterhours::Color{0, 0, 128, 255}, 1.0f);
    }
}

// Render all tables in a document at their line positions.
// cellEdit is the active cell caret (drawn only in its own
// table); caretVisible is the blink state.
inline void renderDocumentTables(
    const std::vector<std::pair<std::size_t, Table>>& tables,
    const LayoutComponent::Rect& textArea, int baseLineHeight,
    int scrollOffset, const CellEditState* cellEdit = nullptr,
    bool caretVisible = true, const TextBuffer* buffer = nullptr) {
    for (std::size_t ti = 0; ti < tables.size(); ++ti) {
        const auto& [lineNum, table] = tables[ti];
        // Calculate Y position based on line number
        if (lineNum < static_cast<std::size_t>(scrollOffset)) continue;

        // A table after a drop-cap paragraph sits below the glyph:
        // the offset accumulates each prior row's reserved extent
        // (text_layout::yOffsetForRow), not a bare row stride.
        // buffer == nullptr keeps the old naive stride.
        int rowOffset =
            buffer
                ? yOffsetForRow(*buffer,
                                static_cast<std::size_t>(
                                    std::max(0, scrollOffset)),
                                lineNum, baseLineHeight)
                : static_cast<int>(lineNum - scrollOffset) * baseLineHeight;
        int y = static_cast<int>(textArea.y) + theme::layout::TEXT_PADDING +
                rowOffset;
        int x = static_cast<int>(textArea.x) + theme::layout::TEXT_PADDING;

        // Check if table is visible
        if (y > static_cast<int>(textArea.y + textArea.height)) continue;

        const CellEditState* thisTableEdit =
            (cellEdit != nullptr && cellEdit->active &&
             cellEdit->tableIndex == ti)
                ? cellEdit
                : nullptr;
        renderTable(table, static_cast<float>(x), static_cast<float>(y),
                    thisTableEdit, caretVisible);
    }
}

// Render the text buffer with caret and selection
// Now supports per-line paragraph styles (H1-H6, Title, Subtitle)
// showLineNumbers: if true, draws line numbers in a gutter on the left
// Draw a red wavy underline from x1 to x2 at the given y (Word 95 squiggle).
inline void drawSpellSquiggle(float x1, float x2, float waveY) {
    const afterhours::Color red{255, 0, 0, 255};
    for (float px = x1; px < x2; px += 4.0f) {
        float midX = std::min(px + 2.0f, x2);
        float endX = std::min(px + 4.0f, x2);
        afterhours::draw_line(static_cast<int>(px), static_cast<int>(waveY),
                              static_cast<int>(midX), static_cast<int>(waveY - 2.0f),
                              red);
        afterhours::draw_line(static_cast<int>(midX), static_cast<int>(waveY - 2.0f),
                              static_cast<int>(endX), static_cast<int>(waveY), red);
    }
}

inline void renderTextBuffer(const TextBuffer& buffer,
                             const LayoutComponent::Rect& textArea,
                             bool caretVisible, int baseFontSize, int baseLineHeight,
                             int scrollOffset, bool showLineNumbers = false,
                             float lineNumberGutterWidth = 50.0f,
                             int tabWidth = 4, float displayScale = 1.0f,
                             const SpellSquiggleCache* spellCache = nullptr,
                             float indentScale = 1.0f,
                             bool suppressSelection = false,
                             bool showParagraphMarks = false,
                             bool outlineMode = false,
                             int columnCount = 1,
                             float columnSpacingPx = 0.0f,
                             int linesPerColumn = 50,
                             int outlineShowLevel = 0,
                             const std::unordered_set<std::size_t>*
                                 outlineCollapsedRows = nullptr,
                             // Margin comments (Docs-era): the anchored
                             // ranges highlight pale yellow; the cards
                             // themselves are UI in MenuUISystem.
                             const std::vector<Comment>* comments = nullptr,
                             // Tracked revisions (suggesting mode):
                             // inserts render green + underlined,
                             // live Delete marks render red +
                             // strikethrough, while the recorded
                             // text still matches the buffer at the
                             // offset (see revisions.h).
                             const std::vector<Revision>* revisions = nullptr,
                             // @ smart chips: raised BUTTON_FACE box
                             // behind the label while the buffer still
                             // holds it at the recorded offset.
                             const std::vector<Chip>* chips = nullptr,
                             // View > Normal: dashed indicator at
                             // computed page boundaries (see
                             // normal_view::isPageBoundary).
                             // normalLinesPerPage is the preview
                             // estimate (page text height / line
                             // height), passed in by the caller.
                             bool normalMode = false,
                             int normalLinesPerPage = 50,
                             // Paged editing view (Page Layout):
                             // the paragraph dialog's pageBreakBefore
                             // / keepWithNext attributes reflow the
                             // page here. In pageless / preview
                             // (normalMode / suppressSelection) they
                             // do not.
                             bool pagedMode = false) {
    // Cursor into the squiggle list (sorted by row). Advanced as rows pass.
    std::size_t squiggleIdx = 0;

    // Verified tracked inserts / deletes, computed once per
    // render (not per line): only revisions whose text still
    // matches the buffer at their offset render; a stale
    // revision draws nothing rather than colouring the wrong
    // text. Classification lives in
    // revisions::verifiedInsertRanges / verifiedDeleteRanges /
    // offsetInInsertRanges / offsetInDeleteRanges so unit tests
    // pin it without rendering.
    const std::vector<std::pair<std::size_t, std::size_t>>
        verifiedInserts =
            revisions
                ? revisions::verifiedInsertRanges(buffer, *revisions)
                : std::vector<std::pair<std::size_t, std::size_t>>{};
    const std::vector<std::pair<std::size_t, std::size_t>>
        verifiedDeletes =
            revisions
                ? revisions::verifiedDeleteRanges(buffer, *revisions)
                : std::vector<std::pair<std::size_t, std::size_t>>{};

    // Verified chips, same once-per-render pattern: only chips
    // whose label still matches the buffer render a box.
    std::vector<std::pair<std::size_t, std::size_t>> verifiedChips;
    if (chips) {
        for (const auto& chip : *chips) {
            if (chips::bufferMatches(buffer, chip)) {
                verifiedChips.emplace_back(chip.startOffset, chip.endOffset);
            }
        }
    }
    const std::vector<SpellSquiggle>* squiggles =
        spellCache ? &spellCache->squiggles : nullptr;
    std::size_t lineCount = buffer.lineCount();
    CaretPosition caret = buffer.caret();
    // suppressSelection (Print Preview): printable content only — no
    // selection highlight, no inverse-selection redraw, no squiggle/
    // selection interaction. Callers also pass spellCache=nullptr in
    // preview so no squiggles render at all.
    bool hasSelection = buffer.hasSelection() && !suppressSelection;
    CaretPosition selStart = buffer.selectionStart();
    CaretPosition selEnd = buffer.selectionEnd();
    
    // Columns (paged mode only; Outline wins when both are on,
    // which cannot happen through the menus — Outline forces
    // Pageless): the page's lines flow column by column. Scroll
    // shows the page containing scrollOffset, page-at-a-time.
    bool columnsActive = !outlineMode && columnCount > 1;
    if (linesPerColumn < 1) linesPerColumn = 1;
    if (columnSpacingPx < 0.0f) columnSpacingPx = 0.0f;
    std::vector<std::pair<std::size_t, int>> columnOrder;
    if (columnsActive) {
        std::size_t pageSize = static_cast<std::size_t>(linesPerColumn) *
                               static_cast<std::size_t>(columnCount);
        std::size_t pageFirst =
            (static_cast<std::size_t>(scrollOffset) / pageSize) * pageSize;
        std::size_t pageEnd =
            std::min(lineCount, pageFirst + pageSize);
        for (int col = 0; col < columnCount; ++col) {
            std::size_t colFirst = pageFirst +
                static_cast<std::size_t>(col) *
                    static_cast<std::size_t>(linesPerColumn);
            std::size_t colEnd = std::min(
                pageEnd,
                colFirst + static_cast<std::size_t>(linesPerColumn));
            for (std::size_t r = colFirst; r < colEnd; ++r) {
                columnOrder.emplace_back(r, col);
            }
        }
    }
    int availableWidthBase =
        static_cast<int>(textArea.width) - 2 * theme::layout::TEXT_PADDING;
    int columnWidthPx =
        columnsActive
            ? static_cast<int>(columns_math::columnWidth(
                  static_cast<float>(availableWidthBase), columnCount,
                  columnSpacingPx))
            : availableWidthBase;

    // Calculate gutter offset for text (line numbers are suppressed
    // in columns mode: the gutter would collide across columns).
    int gutterOffset =
        (showLineNumbers && !columnsActive)
            ? static_cast<int>(lineNumberGutterWidth)
            : 0;

    int y = static_cast<int>(textArea.y) + theme::layout::TEXT_PADDING;
    const int startY = y;
    // Whether a row has already been drawn in this call: the
    // pageBreakBefore / keepWithNext pushes below only apply
    // past the first drawn row — the first drawn row is already
    // at this page's top.
    bool drawnAnyRow = false;
    // Painted bottom of the previous drawn row (its top +
    // fontSize + 2, glyphs + squiggle): the Normal-view break
    // line sits midway between it and the next row's top edge
    // (normal_view::breakLineY). -1 when no row has been drawn
    // yet in this call.
    int prevPaintedBottom = -1;

    // Outline mode: iterate the heading rows only (Title /
    // Subtitle / Heading 1-6), after the Outlining toolbar's
    // filters (Show Level + collapsed headings); scrollOffset
    // counts visible heading rows, not buffer rows, so body
    // paragraphs are never drawn. The filter is the ONE mapping
    // in outline_view.h -- the hit-test and the E2E
    // outline_items count use the same function, so a click on a
    // visible row lands on the heading drawn there.
    // Per-row style list for the row filter and the collapse
    // boxes below (styles by buffer row, in document order).
    std::vector<ParagraphStyle> outlineAllStyles;
    std::vector<std::size_t> outlineRows;
    if (outlineMode) {
        outlineAllStyles.reserve(lineCount);
        for (std::size_t i = 0; i < lineCount; ++i) {
            outlineAllStyles.push_back(buffer.lineParagraphStyle(i));
        }
        static const std::unordered_set<std::size_t> kNoCollapsed;
        outlineRows = outline_view::outlineVisibleRowsFiltered(
            outlineAllStyles, outlineShowLevel,
            outlineCollapsedRows ? *outlineCollapsedRows : kNoCollapsed);
    }
    std::size_t visibleCount = columnsActive
                                     ? columnOrder.size()
                                     : (outlineMode ? outlineRows.size()
                                                    : lineCount);
    std::size_t startVisible = static_cast<std::size_t>(scrollOffset);
    if (startVisible >= visibleCount) {
        startVisible = visibleCount > 0 ? visibleCount - 1 : 0;
    }
    if (columnsActive) startVisible = 0;
    int prevColumn = -1;

    for (std::size_t visibleIndex = startVisible;
         visibleIndex < visibleCount; ++visibleIndex) {
        std::size_t row = columnsActive
                              ? columnOrder[visibleIndex].first
                              : (outlineMode ? outlineRows[visibleIndex]
                                             : visibleIndex);
        int column = columnsActive ? columnOrder[visibleIndex].second : 0;
        // A new column restarts at the top of the page; the squiggle
        // cursor (rows ascending within a column) re-seeks to this
        // column's first row.
        if (columnsActive && column != prevColumn) {
            y = startY;
            prevPaintedBottom = -1;
            prevColumn = column;
            squiggleIdx = 0;
            if (squiggles) {
                while (squiggleIdx < squiggles->size() &&
                       (*squiggles)[squiggleIdx].row < row) {
                    ++squiggleIdx;
                }
            }
        }
        LineSpan span = buffer.lineSpan(row);
        // One origin for text and squiggles, through the columns
        // math (see columns_math::columnLineOriginX): a squiggle
        // from a single-column x would land under the wrong
        // column's word.
        int baseX = static_cast<int>(columns_math::columnLineOriginX(
            static_cast<float>(textArea.x), theme::layout::TEXT_PADDING,
            static_cast<float>(gutterOffset), columnsActive ? column : 0,
            static_cast<float>(columnWidthPx), columnSpacingPx));
        int availableWidth =
            columnsActive ? columnWidthPx : availableWidthBase;

        // Zero-copy line access: use lineView() to avoid allocation on most lines.
        // Only the line currently being edited (where the gap is) needs a copy.
        auto view = buffer.lineView(row);
        std::string lineStorage;  // Only used if view fails or tabs need expanding
        const char* lineData;
        std::size_t lineLen;
        if (view) {
            lineData = view.data;
            lineLen = view.length;
        } else if (span.length > 0) {
            lineStorage = buffer.lineString(row);
            lineData = lineStorage.data();
            lineLen = lineStorage.size();
        } else {
            lineData = "";
            lineLen = 0;
        }

        // Expand tabs — only allocates if there are actual tab characters
        auto expandTabs = [tabWidth](const char* data, std::size_t len) -> std::string {
            if (tabWidth <= 0) return std::string(data, len);
            // Fast check: any tabs?
            if (!std::memchr(data, '\t', len)) return std::string(data, len);
            std::string expanded;
            expanded.reserve(len);
            int col = 0;
            for (std::size_t i = 0; i < len; ++i) {
                char ch = data[i];
                if (ch == '\t') {
                    int spaces = tabWidth - (col % tabWidth);
                    expanded.append(static_cast<std::size_t>(spaces), ' ');
                    col += spaces;
                } else {
                    expanded.push_back(ch);
                    col += 1;
                }
            }
            return expanded;
        };
        std::string displayLine = expandTabs(lineData, lineLen);
        
        // Get paragraph style for this line. The paragraph font
        // scales by displayScale (fit * zoom in paged mode), so
        // text, caret height, line height, and squiggle offsets
        // all live in the page's coordinate scale. In pageless
        // mode displayScale == zoomLevel, so this is unchanged.
        ParagraphStyle paraStyle = buffer.lineParagraphStyle(row);
        int lineFontSize = ruler_math::scaledParagraphFontSize(
            paragraphStyleFontSize(paraStyle), displayScale);
        int baseLineHeightForStyle = lineFontSize + 4;
        
        // Use base font size as minimum if paragraph style would be smaller
        if (lineFontSize < baseFontSize && paraStyle == ParagraphStyle::Normal) {
            lineFontSize = baseFontSize;
            baseLineHeightForStyle = baseLineHeight;
        }
        
        // Apply line spacing multiplier
        float spacingMultiplier = buffer.lineSpacing(row);
        int lineHeight = static_cast<int>(baseLineHeightForStyle * spacingMultiplier);
        
        // Apply paragraph spacing before
        int paragraphSpaceBefore = buffer.lineSpaceBefore(row);
        y += paragraphSpaceBefore;

        // Paragraph dialog "Page break before"
        // (LineSpan::pageBreakBefore, distinct from the Ctrl+Enter
        // manual break below): in the paged editing view the
        // paragraph starts a new page. This viewport shows one
        // page (textArea); ending the loop here leaves the rest
        // of this page blank, and the paragraph renders at the
        // top once scrolled to. Skipped in columns (rows are
        // assigned to columns by index, not by y), outline, and
        // Print Preview (suppressSelection: preview pagination
        // is a line-stride estimate that does not reflow).
        if (pagedMode && !columnsActive && !outlineMode &&
            !suppressSelection && span.pageBreakBefore && drawnAnyRow) {
            break;
        }
        
        // Draw page break indicator if present (editing adornment:
        // hidden in Print Preview, which shows printable content only).
        if (buffer.hasPageBreakBefore(row) && !suppressSelection) {
            // In paged mode, this would force a new page
            // In pageless mode, we show a visual indicator
            int breakY = y - 8;  // Position above the line
            int lineStart = static_cast<int>(textArea.x) + 20;
            int lineEnd = static_cast<int>(textArea.x + textArea.width) - 20;
            
            // Draw a dashed line to indicate page break
            afterhours::Color breakColor = {128, 128, 128, 255};  // Gray
            for (int px = lineStart; px < lineEnd; px += 8) {
                afterhours::draw_line(px, breakY, px + 4, breakY, breakColor);
            }
            
            // Draw "Page Break" text in center
            const char* breakText = "Page Break";
            int textWidth = afterhours::graphics::measure_text(breakText, 10);
            int textX = lineStart + (lineEnd - lineStart - textWidth) / 2;
            
            // Draw background for text
            afterhours::draw_rectangle(
                Rectangle{static_cast<float>(textX - 4), static_cast<float>(breakY - 6),
                                  static_cast<float>(textWidth + 8), 12.0f},
                afterhours::Color{255, 255, 255, 255});
            theme::DrawUIText(breakText, textX, breakY - 5, 10, breakColor);
            
            y += 20;  // Add space for the page break indicator
        }

        // View > Normal: dashed indicator at each COMPUTED page
        // boundary (normal_view::isPageBoundary — every
        // normalLinesPerPage-th row, the same estimate Print
        // Preview uses) and at each paragraph-dialog "Page break
        // before" row (LineSpan::pageBreakBefore). Word 95 Normal
        // shows page breaks as dashed lines across the text;
        // unlike an explicit Page Break (above) this adds no
        // vertical space and no label — the rows keep their
        // positions, so the hit-test math is untouched. Skipped
        // where an explicit break already drew: two lines on top
        // of each other would read as one thick line. Editing
        // adornment: hidden in Print Preview (normalMode is false
        // there), like the explicit break.
        if (normalMode && !suppressSelection &&
            (normal_view::isPageBoundary(row, normalLinesPerPage) ||
             span.pageBreakBefore) &&
            !buffer.hasPageBreakBefore(row)) {
            // y is the current line's top (paragraph space-before
            // already added): the line sits at the top edge minus
            // half the inter-line gap (midway between the
            // previous row's painted bottom and this row's top),
            // never on either row's glyphs or squiggle.
            int breakY = normal_view::breakLineY(y, prevPaintedBottom);
            int lineStart = static_cast<int>(textArea.x) + 20;
            int lineEnd =
                static_cast<int>(textArea.x + textArea.width) - 20;
            afterhours::Color breakColor = {128, 128, 128, 255};
            for (int px = lineStart; px < lineEnd; px += 8) {
                afterhours::draw_line(px, breakY, px + 4, breakY,
                                      breakColor);
            }
        }

        // Paragraph dialog "Keep with next"
        // (LineSpan::keepWithNext): if this row is the last one
        // that fits on this page and a next row exists, move this
        // row to the next page with it — either the next row
        // would overflow this page (the loop's own break check
        // below would end the page after this row), or the next
        // row starts a new page itself (pageBreakBefore). Same
        // one-page viewport and same skips as pageBreakBefore
        // above.
        if (pagedMode && !columnsActive && !outlineMode &&
            !suppressSelection && span.keepWithNext && drawnAnyRow &&
            row + 1 < lineCount) {
            int spaceAfter = buffer.lineSpaceAfter(row);
            bool nextStartsNewPage =
                buffer.lineSpan(row + 1).pageBreakBefore;
            // The page-overflow check uses the whole reserved extent
            // of a drop-cap paragraph, not one line of it: a cap
            // paragraph starting at the page bottom must move with
            // its next row, not spill its glyph past the page edge.
            int reservedHeight =
                static_cast<int>(drop_cap::reservedRows(
                    buffer.lineDropCapLines(row), 1)) *
                lineHeight;
            if (nextStartsNewPage ||
                y + reservedHeight + spaceAfter >
                    static_cast<int>(textArea.y + textArea.height)) {
                break;
            }
        }

        // Previous row's painted bottom for the next row's
        // Normal-view break line: this row's top + fontSize + 2
        // (glyphs + the squiggle below them). y is final for
        // this row's text here (space-before and the explicit
        // break bump already applied).
        prevPaintedBottom = y + lineFontSize + 2;

        // Draw line number in gutter if enabled (not in columns
        // mode: the gutter would collide across columns).
        if (showLineNumbers && !columnsActive) {
            int lineNum = static_cast<int>(row + 1);  // 1-based line numbers
            char lineNumStr[16];
            std::snprintf(lineNumStr, sizeof(lineNumStr), "%d", lineNum);
            
            // Measure line number text to right-align in gutter
            int numWidth = afterhours::graphics::measure_text(lineNumStr, 14);
            int gutterX = static_cast<int>(textArea.x) + static_cast<int>(lineNumberGutterWidth) - numWidth - 8;
            
            // Draw line number in gray
            afterhours::Color lineNumColor = {128, 128, 128, 255};
            theme::DrawUIText(lineNumStr, gutterX, y, 14, lineNumColor);
        }
        
        // Apply indentation, scaled by the ONE display scale so the
        // text indent lands on the same ruler tick as its marker:
        // stored indents are 72-dpi units, display = stored * indentScale.
        int leftIndent = static_cast<int>(
            std::lround(buffer.lineLeftIndent(row) * indentScale));
        int firstLineIndent = static_cast<int>(
            std::lround(buffer.lineFirstLineIndent(row) * indentScale));
        int rightIndent = static_cast<int>(
            std::lround(buffer.lineRightIndent(row) * indentScale));
        // Note: firstLineIndent only applies to first line of paragraph
        // For now, we treat each line as its own paragraph
        int totalIndent = leftIndent + firstLineIndent;
        
        // Get list properties for this line
        ListType listType = buffer.lineListType(row);
        int listLevel = buffer.lineListLevel(row);
        int listNumber = buffer.lineListNumber(row);
        
        // Calculate list marker indent (each level adds 20px)
        int listIndent = (listType != ListType::None) ? (listLevel + 1) * 20 : 0;
        
        // Outline indent: (level - 1) * 24px on top of the
        // paragraph's own indents.
        int outlineIndent =
            outlineMode ? outline_view::outlineIndentPx(paraStyle) : 0;
        // Collapse box: drawn at the heading's indented start
        // when it has subordinate headings. EVERY heading's text
        // shifts right by the box + gap -- not only headings
        // with a box -- so the (level - 1) * 24px indent
        // difference between a parent and a child is preserved
        // (a box-only shift would eat 16px of the child's 24px
        // indent and leave it only 8px right of its parent).
        // The shift is part of indentedBaseX, so text, caret,
        // selection, and squiggles all move with it (and the
        // hit-test uses the same shift).
        bool outlineHasCollapseBox =
            outlineMode &&
            outline_view::outlineHasSubordinateHeadings(outlineAllStyles,
                                                        row);
        int outlineCollapseShift =
            outlineMode ? outline_view::kOutlineCollapseShift : 0;
        int indentedBaseX = baseX + totalIndent + listIndent +
                            outlineIndent + outlineCollapseShift;
        int indentedWidth =
            std::max(0, availableWidth - totalIndent - listIndent -
                            rightIndent - outlineIndent -
                            outlineCollapseShift);
        // Drop Cap wrap indent: none here. Each buffer row IS its
        // own paragraph, so a following buffer row is never a
        // wrapped continuation of a drop-cap paragraph — the
        // old scan of the previous rows ("the nearest drop-cap
        // row whose span reaches this row") indented paragraphs
        // 2 and 3 under a drop cap on paragraph 1 (Batch 11 fix
        // round). The indent belongs only to wrapped lines of
        // the SAME paragraph (drop_cap::wrapIndentApplies,
        // line-index-within-paragraph in [1, N)), which the
        // wrapping layout (text_layout) governs; this renderer
        // draws one buffer line per visual row, so there are no
        // such lines here. The cap paragraph's own row draws its
        // first glyph enlarged below, never as an indent.
        // Draw the +/- collapse box (before the text, at the
        // pre-shift indented start). Collapsed shows '+'
        // (subordinates hidden), expanded shows '-'.
        if (outlineHasCollapseBox) {
            int boxX = outline_view::outlineCollapseBoxX(
                indentedBaseX - outlineCollapseShift);
            int boxY = y + (lineFontSize + 4 -
                            outline_view::kOutlineCollapseBoxSize) /
                               2;
            bool isCollapsed =
                outlineCollapsedRows &&
                outlineCollapsedRows->count(row) > 0;
            afterhours::draw_rectangle(
                Rectangle{static_cast<float>(boxX),
                          static_cast<float>(boxY),
                          static_cast<float>(
                              outline_view::kOutlineCollapseBoxSize),
                          static_cast<float>(
                              outline_view::kOutlineCollapseBoxSize)},
                afterhours::Color{255, 255, 255, 255});
            afterhours::draw_rectangle_outline(
                Rectangle{static_cast<float>(boxX),
                          static_cast<float>(boxY),
                          static_cast<float>(
                              outline_view::kOutlineCollapseBoxSize),
                          static_cast<float>(
                              outline_view::kOutlineCollapseBoxSize)},
                afterhours::Color{0, 0, 0, 255}, 1.0f);
            int midY = boxY + outline_view::kOutlineCollapseBoxSize / 2;
            afterhours::draw_line(
                boxX + 2, midY,
                boxX + outline_view::kOutlineCollapseBoxSize - 3, midY,
                afterhours::Color{0, 0, 0, 255});
            if (isCollapsed) {
                int midX = boxX + outline_view::kOutlineCollapseBoxSize / 2;
                afterhours::draw_line(
                    midX, boxY + 2, midX,
                    boxY + outline_view::kOutlineCollapseBoxSize - 3,
                    afterhours::Color{0, 0, 0, 255});
            }
        }
        
        // Calculate text width for alignment
        int textWidth = displayLine.empty() ? 0 : afterhours::graphics::measure_text(displayLine.c_str(), lineFontSize);
        
        // Apply text alignment (within the indented area)
        TextAlignment alignment = buffer.lineAlignment(row);
        int x = indentedBaseX;
        switch (alignment) {
            case TextAlignment::Left:
            default:
                x = indentedBaseX;
                break;
            case TextAlignment::Center:
                x = indentedBaseX + (indentedWidth - textWidth) / 2;
                break;
            case TextAlignment::Right:
                x = indentedBaseX + indentedWidth - textWidth;
                break;
            case TextAlignment::Justify:
                // Justify is same as left for now (requires word spacing adjustments)
                x = indentedBaseX;
                break;
        }
        // The line's layout x (column origin + indents + alignment).
        // Squiggles below use this, not the drawing cursor `x`,
        // which the drop-cap block below mutates.
        const int layoutTextX = x;
        
        // Draw list marker (bullet or number) before text
        if (listType != ListType::None) {
            // Calculate bullet position (hanging indent style)
            int markerX = baseX + totalIndent + (listLevel * 20);
            
            TextStyle globalStyle = buffer.textStyle();
            afterhours::Color textColor = {globalStyle.textColor.r, globalStyle.textColor.g,
                                       globalStyle.textColor.b, globalStyle.textColor.a};
            
            if (listType == ListType::Bulleted) {
                const char* bullet = bulletForLevel(listLevel);
                afterhours::draw_text(bullet, markerX, y, lineFontSize, textColor);
            } else if (listType == ListType::Numbered) {
                char numberStr[16];
                std::snprintf(numberStr, sizeof(numberStr), "%d.", listNumber);
                afterhours::draw_text(numberStr, markerX, y, lineFontSize, textColor);
            }
        }
        
        // Draw paragraph shading + borders (Format > Borders and
        // Shading...). Shading fills the paragraph rect behind the
        // text, BEFORE the selection highlight below (a selection
        // still paints its navy bg over the shading) and before the
        // squiggles, which draw after the text. The rect spans the
        // indented paragraph width expanded by the
        // borders_shading padding (7px horizontal / 5px vertical:
        // base 4/2 + 3 expansion) so glyphs never touch the
        // stroke; consecutive lines
        // with the identical border + shading triple merge into one
        // box (no internal horizontal edge), except in columns /
        // outline mode where each visible line draws its own edges.
        // Borders reuse drawBorderEdge, the table-border helper.
        if (borders_shading::hasParagraphShading(span.shadingColorIndex) ||
            borders_shading::hasParagraphBorder(span.borderEdges,
                                                span.borderStyle)) {
            bool mergeGroups = !columnsActive && !outlineMode;
            auto sameTriple = [&](std::size_t other) {
                LineSpan otherSpan = buffer.lineSpan(other);
                return borders_shading::sameBorderShading(
                    span.borderEdges, span.borderStyle,
                    span.shadingColorIndex, otherSpan.borderEdges,
                    otherSpan.borderStyle, otherSpan.shadingColorIndex);
            };
            bool sameAsPrev =
                mergeGroups && row > 0 && sameTriple(row - 1);
            bool sameAsNext = mergeGroups && row + 1 < lineCount &&
                              sameTriple(row + 1);
            borders_shading::BorderBox box =
                borders_shading::borderBoxRect(
                    static_cast<float>(baseX),
                    static_cast<float>(leftIndent),
                    static_cast<float>(availableWidth),
                    static_cast<float>(rightIndent),
                    static_cast<float>(y),
                    static_cast<float>(lineHeight));
            float boxX0 = box.x0;
            float boxX1 = box.x1;
            float boxY0 = box.y0;
            float boxY1 = box.y1;
            if (borders_shading::hasParagraphShading(
                    span.shadingColorIndex)) {
                TextColor fill =
                    borders_shading::shadingColorForIndex(
                        span.shadingColorIndex);
                afterhours::draw_rectangle(
                    Rectangle{boxX0, boxY0, boxX1 - boxX0, boxY1 - boxY0},
                    afterhours::Color{fill.r, fill.g, fill.b, fill.a});
            }
            borders_shading::EdgesToDraw toDraw =
                borders_shading::edgesToDraw(span.borderEdges,
                                             span.borderStyle, sameAsPrev,
                                             sameAsNext);
            afterhours::Color borderColor{0, 0, 0, 255};
            if (toDraw.top)
                drawBorderEdge(boxX0, boxY0, boxX1, boxY0,
                               BorderStyle::Thin, borderColor);
            if (toDraw.bottom)
                drawBorderEdge(boxX0, boxY1, boxX1, boxY1,
                               BorderStyle::Thin, borderColor);
            if (toDraw.left)
                drawBorderEdge(boxX0, boxY0, boxX0, boxY1,
                               BorderStyle::Thin, borderColor);
            if (toDraw.right)
                drawBorderEdge(boxX1, boxY0, boxX1, boxY1,
                               BorderStyle::Thin, borderColor);
        }

        // Draw selection highlight (with alignment offset).
        // Selection columns are raw-line coords; map through tab
        // expansion to display columns so the bg lines up with the text.
        if (hasSelection) {
            bool lineInSelection = (row >= selStart.row && row <= selEnd.row);
            if (lineInSelection) {
                std::size_t startCol =
                    (row == selStart.row) ? selStart.column : 0;
                std::size_t endCol =
                    (row == selEnd.row) ? selEnd.column : span.length;
                std::size_t dispStart = rawColumnToDisplayColumn(
                    lineData, lineLen, startCol, tabWidth);
                std::size_t dispEnd = rawColumnToDisplayColumn(
                    lineData, lineLen, endCol, tabWidth);
                dispStart = std::min(dispStart, displayLine.size());
                dispEnd = std::min(std::max(dispEnd, dispStart), displayLine.size());

                if (dispStart < dispEnd && !displayLine.empty()) {
                    std::string beforeSel = displayLine.substr(0, dispStart);
                    std::string selectedText =
                        displayLine.substr(dispStart, dispEnd - dispStart);

                    int selX =
                        x + afterhours::graphics::measure_text(beforeSel.c_str(), lineFontSize);
                    int selWidth =
                        afterhours::graphics::measure_text(selectedText.c_str(), lineFontSize);
                    afterhours::draw_rectangle(
                        Rectangle{static_cast<float>(selX), static_cast<float>(y),
                                          static_cast<float>(selWidth), static_cast<float>(lineHeight)},
                        theme::SELECTION_BG);
                }
            }
        }

        // Draw margin-comment anchor highlight: pale yellow behind
        // the anchored range on this line (same column mapping as
        // the selection highlight above). Zero-length anchors and
        // anchors whose range misses this line draw nothing.
        if (comments && !displayLine.empty()) {
            for (const auto& comment : *comments) {
                std::size_t lineBegin = span.offset;
                std::size_t lineEnd = span.offset + span.length;
                std::size_t segBegin =
                    std::max(comment.startOffset, lineBegin);
                std::size_t segEnd = std::min(comment.endOffset, lineEnd);
                if (segBegin >= segEnd) continue;
                std::size_t dispBegin = rawColumnToDisplayColumn(
                    lineData, lineLen, segBegin - lineBegin, tabWidth);
                std::size_t dispEnd = rawColumnToDisplayColumn(
                    lineData, lineLen, segEnd - lineBegin, tabWidth);
                dispBegin = std::min(dispBegin, displayLine.size());
                dispEnd = std::min(std::max(dispEnd, dispBegin),
                                   displayLine.size());
                if (dispBegin >= dispEnd) continue;
                int hlX = x + afterhours::graphics::measure_text(
                                  displayLine.substr(0, dispBegin).c_str(),
                                  lineFontSize);
                int hlW = afterhours::graphics::measure_text(
                    displayLine.substr(dispBegin, dispEnd - dispBegin)
                        .c_str(),
                    lineFontSize);
                afterhours::draw_rectangle(
                    Rectangle{static_cast<float>(hlX),
                              static_cast<float>(y),
                              static_cast<float>(hlW),
                              static_cast<float>(lineHeight)},
                    afterhours::Color{255, 255, 180, 255});
            }
        }

        // Draw @ chip boxes: raised BUTTON_FACE rect behind the
        // chip's label on this line (same column mapping as the
        // comment highlight above). Rendered before the text so
        // the glyphs draw on top of the box. Neighbouring chips
        // get their touching sides shrunk (chips::chipBoxRect)
        // so adjacent boxes stay visibly separate: with the old
        // fixed inset the date + person boxes merged into one
        // continuous rectangle.
        if (!verifiedChips.empty() && !displayLine.empty()) {
            struct ChipSeg {
                std::size_t begin = 0;
                std::size_t end = 0;
                int labelX = 0;
                int labelW = 0;
            };
            std::vector<ChipSeg> chipSegs;
            for (const auto& range : verifiedChips) {
                std::size_t lineBegin = span.offset;
                std::size_t lineEnd = span.offset + span.length;
                std::size_t segBegin = std::max(range.first, lineBegin);
                std::size_t segEnd = std::min(range.second, lineEnd);
                if (segBegin >= segEnd) continue;
                std::size_t dispBegin = rawColumnToDisplayColumn(
                    lineData, lineLen, segBegin - lineBegin, tabWidth);
                std::size_t dispEnd = rawColumnToDisplayColumn(
                    lineData, lineLen, segEnd - lineBegin, tabWidth);
                dispBegin = std::min(dispBegin, displayLine.size());
                dispEnd = std::min(std::max(dispEnd, dispBegin),
                                   displayLine.size());
                if (dispBegin >= dispEnd) continue;
                const int labelX =
                    x + afterhours::graphics::measure_text(
                            displayLine.substr(0, dispBegin).c_str(),
                            lineFontSize);
                const int labelW =
                    afterhours::graphics::measure_text(
                        displayLine.substr(dispBegin, dispEnd - dispBegin)
                            .c_str(),
                        lineFontSize);
                chipSegs.push_back(ChipSeg{segBegin, segEnd, labelX,
                                           labelW});
            }
            for (std::size_t si = 0; si < chipSegs.size(); ++si) {
                const ChipSeg& seg = chipSegs[si];
                int leftDist = -1;
                int rightDist = -1;
                for (std::size_t oi = 0; oi < chipSegs.size(); ++oi) {
                    if (oi == si) continue;
                    const ChipSeg& other = chipSegs[oi];
                    if (other.end == seg.begin) {
                        leftDist = 0;
                    } else if (other.end + 1 == seg.begin) {
                        leftDist = 1;
                    }
                    if (other.begin == seg.end) {
                        rightDist = 0;
                    } else if (other.begin == seg.end + 1) {
                        rightDist = 1;
                    }
                }
                const chips::ChipBox box = chips::chipBoxRect(
                    seg.labelX, seg.labelW, leftDist, rightDist);
                afterhours::draw_rectangle(
                    Rectangle{static_cast<float>(box.x),
                              static_cast<float>(y),
                              static_cast<float>(box.width),
                              static_cast<float>(lineHeight)},
                    theme::BUTTON_FACE);
                afterhours::draw_rectangle_outline(
                    Rectangle{static_cast<float>(box.x),
                              static_cast<float>(y),
                              static_cast<float>(box.width),
                              static_cast<float>(lineHeight)},
                    afterhours::Color{128, 128, 128, 255}, 1.0f);
            }
        }

        // Draw text with paragraph style applied
        if (!displayLine.empty()) {
            // Get global text style for underline/strikethrough/colors
            TextStyle globalStyle = buffer.textStyle();
            
            // Convert TextColor to backend-agnostic Color
            afterhours::Color textColor = {globalStyle.textColor.r, globalStyle.textColor.g,
                                       globalStyle.textColor.b, globalStyle.textColor.a};
            
            // Per-range spans on this line? A span overrides the
            // global style for its chars only (Batch 12); drop-cap
            // lines keep the old whole-line draw (their x/text are
            // shifted, so per-segment math would misalign there).
            bool lineHasCharSpans = false;
            if (!span.hasDropCap) {
                std::size_t lineBegin = span.offset;
                std::size_t lineEnd = span.offset + span.length;
                for (const auto& cs : buffer.charStyleSpans()) {
                    if (cs.startOffset < lineEnd &&
                        cs.endOffset > lineBegin) {
                        lineHasCharSpans = true;
                        break;
                    }
                }
            }

            // Draw highlight background if set (whole-line: only
            // when no per-range spans — the span path below draws
            // one highlight rect per segment).
            if (!globalStyle.highlightColor.isNone() &&
                !lineHasCharSpans) {
                afterhours::Color highlightColor = {globalStyle.highlightColor.r, globalStyle.highlightColor.g,
                                                globalStyle.highlightColor.b, globalStyle.highlightColor.a};
                afterhours::draw_rectangle(
                    Rectangle{static_cast<float>(x), static_cast<float>(y),
                                      static_cast<float>(textWidth), static_cast<float>(lineHeight)},
                    highlightColor);
            }
            
            int textFontSize = lineFontSize;
            int textYOffset = 0;
            if (globalStyle.superscript || globalStyle.subscript) {
                textFontSize = std::max(8, static_cast<int>(lineFontSize * 0.75f));
                textYOffset = globalStyle.superscript ? -lineFontSize / 3 : lineFontSize / 4;
            }

            std::string textToDraw = displayLine;

            // Drop cap support: draw first character larger, in
            // the effective style at offset 0 of its paragraph
            // (span-aware, Batch 13 — the same effectiveStyleAt
            // the per-segment path uses), not the global style:
            // a bold / italic / red first letter drops bold /
            // italic / red. Paragraph-style bold / italic still
            // apply, as for the rest of the line; the rest of
            // the drop-cap line keeps the whole-line global draw.
            if (span.hasDropCap && !textToDraw.empty()) {
                std::string dropChar = textToDraw.substr(0, 1);
                int dropFontSize = lineFontSize * span.dropCapLines;
                const TextStyle dropStyle =
                    buffer.effectiveStyleAt(span.offset);
                const afterhours::Color dropColor{
                    dropStyle.textColor.r, dropStyle.textColor.g,
                    dropStyle.textColor.b, dropStyle.textColor.a};
                const bool dropBold =
                    paragraphStyleIsBold(paraStyle) || dropStyle.bold;
                const bool dropItalic =
                    paragraphStyleIsItalic(paraStyle) || dropStyle.italic;
                const afterhours::Color dropDrawColor =
                    dropItalic
                        ? afterhours::Color{
                              static_cast<unsigned char>(dropColor.r / 2 + 64),
                              static_cast<unsigned char>(dropColor.g / 2 + 64),
                              static_cast<unsigned char>(dropColor.b / 2 + 64),
                              dropColor.a}
                        : dropColor;
                if (dropBold) {
                    afterhours::draw_text(dropChar.c_str(), x,
                                          y - lineFontSize / 2, dropFontSize,
                                          dropDrawColor);
                    afterhours::draw_text(dropChar.c_str(), x + 1,
                                          y - lineFontSize / 2, dropFontSize,
                                          dropDrawColor);
                } else {
                    afterhours::draw_text(dropChar.c_str(), x,
                                          y - lineFontSize / 2, dropFontSize,
                                          dropDrawColor);
                }
                int dropWidth = afterhours::graphics::measure_text(dropChar.c_str(), dropFontSize);
                textToDraw = textToDraw.substr(1);
                if (!textToDraw.empty()) {
                    x += dropWidth + 4;
                }
            }

            // Tracked inserts render GREEN (#008000, Word revision
            // colour) + underlined, winning over the spell squiggle
            // (the squiggle pass below skips spans). Drop-cap lines
            // keep the old whole-line draw: their x/text are
            // shifted, so per-segment math would misalign there.
            // A 1px green underline alone was invisible next to the
            // red squiggle (the pre-fix screenshot) — the green
            // glyphs are what makes the span visible.
            const afterhours::Color revisionGreen{0, 128, 0, 255};
            const bool isBold =
                paragraphStyleIsBold(paraStyle) || globalStyle.bold;
            const bool isItalic =
                paragraphStyleIsItalic(paraStyle) || globalStyle.italic;
            const afterhours::Color italicColor = {
                static_cast<unsigned char>(textColor.r / 2 + 64),
                static_cast<unsigned char>(textColor.g / 2 + 64),
                static_cast<unsigned char>(textColor.b / 2 + 64),
                textColor.a};
            if (span.hasDropCap) {
                // For headings and titles, draw bold text (simulated by drawing twice with offset)
                if (isBold) {
                    afterhours::draw_text(textToDraw.c_str(), x, y + textYOffset, textFontSize, textColor);
                    afterhours::draw_text(textToDraw.c_str(), x + 1, y + textYOffset, textFontSize, textColor);
                } else if (isItalic) {
                    afterhours::draw_text(textToDraw.c_str(), x, y + textYOffset, textFontSize, italicColor);
                } else {
                    afterhours::draw_text(textToDraw.c_str(), x, y + textYOffset, textFontSize, textColor);
                }

                // Draw underline if enabled
                if (globalStyle.underline) {
                    int underlineY = y + lineFontSize + 1;
                    afterhours::draw_line(x, underlineY, x + textWidth, underlineY, textColor);
                }
            } else if (lineHasCharSpans) {
                // Per-range path: split the line into segments
                // where the effective style (span or global) or
                // the tracked-insert flag changes, and draw each
                // segment with its own bold / italic / color /
                // underline / strikethrough / highlight /
                // super-sub sizing. Unspanned chars use the
                // global style, so a bold word A next to plain
                // word C draws bold only for A.
                std::vector<std::size_t> displayOffsets;
                displayOffsets.reserve(displayLine.size());
                if (tabWidth <= 0) {
                    for (std::size_t i = 0; i < lineLen; ++i) {
                        displayOffsets.push_back(span.offset + i);
                    }
                } else {
                    int col = 0;
                    for (std::size_t i = 0; i < lineLen; ++i) {
                        if (lineData[i] == '\t') {
                            int spaces = tabWidth - (col % tabWidth);
                            for (int s = 0; s < spaces; ++s) {
                                displayOffsets.push_back(span.offset + i);
                            }
                            col += spaces;
                        } else {
                            displayOffsets.push_back(span.offset + i);
                            ++col;
                        }
                    }
                }
                const std::size_t segCount =
                    std::min(displayOffsets.size(), displayLine.size());
                std::size_t segStart = 0;
                int cursorX = x;
                while (segStart < segCount) {
                    const TextStyle segStyle =
                        buffer.effectiveStyleAt(displayOffsets[segStart]);
                    const bool segInsert =
                        revisions::offsetInInsertRanges(
                            displayOffsets[segStart], verifiedInserts);
                    const bool segDelete =
                        revisions::offsetInDeleteRanges(
                            displayOffsets[segStart], verifiedDeletes);
                    std::size_t segEnd = segStart + 1;
                    while (segEnd < segCount &&
                           revisions::offsetInInsertRanges(
                               displayOffsets[segEnd], verifiedInserts) ==
                                   segInsert &&
                           revisions::offsetInDeleteRanges(
                               displayOffsets[segEnd], verifiedDeletes) ==
                                   segDelete &&
                           buffer.effectiveStyleAt(displayOffsets[segEnd]) ==
                               segStyle) {
                        ++segEnd;
                    }
                    const std::string segText = displayLine.substr(
                        segStart, segEnd - segStart);
                    int segFontSize = lineFontSize;
                    int segYOffset = 0;
                    if (segStyle.superscript || segStyle.subscript) {
                        segFontSize = std::max(
                            8, static_cast<int>(lineFontSize * 0.75f));
                        segYOffset = segStyle.superscript
                                         ? -lineFontSize / 3
                                         : lineFontSize / 4;
                    }
                    const int segW = afterhours::graphics::measure_text(
                        segText.c_str(), segFontSize);
                    if (!segStyle.highlightColor.isNone()) {
                        afterhours::draw_rectangle(
                            Rectangle{static_cast<float>(cursorX),
                                      static_cast<float>(y),
                                      static_cast<float>(segW),
                                      static_cast<float>(lineHeight)},
                            afterhours::Color{
                                segStyle.highlightColor.r,
                                segStyle.highlightColor.g,
                                segStyle.highlightColor.b,
                                segStyle.highlightColor.a});
                    }
                    const bool segBold =
                        paragraphStyleIsBold(paraStyle) || segStyle.bold;
                    const bool segItalic =
                        paragraphStyleIsItalic(paraStyle) ||
                        segStyle.italic;
                    const afterhours::Color segTextColor{
                        segStyle.textColor.r, segStyle.textColor.g,
                        segStyle.textColor.b, segStyle.textColor.a};
                    const afterhours::Color segItalicColor{
                        static_cast<unsigned char>(segTextColor.r / 2 + 64),
                        static_cast<unsigned char>(segTextColor.g / 2 + 64),
                        static_cast<unsigned char>(segTextColor.b / 2 + 64),
                        segTextColor.a};
                    // Delete wins over insert: inserted-then-
                    // deleted text reads as deleted (red +
                    // strikethrough, no underline).
                    const afterhours::Color revisionRed{255, 0, 0, 255};
                    const afterhours::Color segColor =
                        segDelete ? revisionRed
                                  : (segInsert ? revisionGreen
                                               : (segItalic ? segItalicColor
                                                            : segTextColor));
                    if (segBold) {
                        afterhours::draw_text(segText.c_str(), cursorX,
                                              y + segYOffset, segFontSize,
                                              segColor);
                        afterhours::draw_text(segText.c_str(), cursorX + 1,
                                              y + segYOffset, segFontSize,
                                              segColor);
                    } else {
                        afterhours::draw_text(segText.c_str(), cursorX,
                                              y + segYOffset, segFontSize,
                                              segColor);
                    }
                    if ((segInsert && !segDelete) ||
                        segStyle.underline) {
                        const int underlineY = y + lineFontSize + 1;
                        afterhours::draw_line(cursorX, underlineY,
                                              cursorX + segW, underlineY,
                                              segColor);
                    }
                    if (segDelete || segStyle.strikethrough) {
                        const int strikeY = y + lineFontSize / 2;
                        afterhours::draw_line(cursorX, strikeY,
                                              cursorX + segW, strikeY,
                                              segColor);
                    }
                    cursorX += segW;
                    segStart = segEnd;
                }
                // Selection redraw for span lines (the whole-line
                // block below is skipped for these lines): the
                // selected sub-segments redraw in SELECTION_TEXT
                // with each sub-segment's own bold / underline /
                // strikethrough.
                if (hasSelection && segCount > 0) {
                    bool lineInSelection =
                        (row >= selStart.row && row <= selEnd.row);
                    if (lineInSelection) {
                        std::size_t rawStart =
                            (row == selStart.row) ? selStart.column : 0;
                        std::size_t rawEnd =
                            (row == selEnd.row) ? selEnd.column
                                                : span.length;
                        std::size_t dispStart = rawColumnToDisplayColumn(
                            lineData, lineLen, rawStart, tabWidth);
                        std::size_t dispEnd = rawColumnToDisplayColumn(
                            lineData, lineLen, rawEnd, tabWidth);
                        dispStart =
                            std::min(dispStart, displayLine.size());
                        dispEnd = std::min(
                            std::max(dispEnd, dispStart),
                            displayLine.size());
                        std::size_t subStart = dispStart;
                        int subX = x;
                        // Re-measure up to dispStart with per-segment
                        // font sizes (super/sub segments are narrower).
                        {
                            std::size_t acc = 0;
                            while (acc < dispStart && acc < segCount) {
                                const TextStyle accStyle =
                                    buffer.effectiveStyleAt(
                                        displayOffsets[acc]);
                                int accSize = lineFontSize;
                                if (accStyle.superscript ||
                                    accStyle.subscript) {
                                    accSize = std::max(
                                        8, static_cast<int>(
                                                lineFontSize * 0.75f));
                                }
                                std::size_t accEnd = acc + 1;
                                while (accEnd < dispStart &&
                                       accEnd < segCount &&
                                       buffer.effectiveStyleAt(
                                           displayOffsets[accEnd]) ==
                                           accStyle) {
                                    ++accEnd;
                                }
                                subX += afterhours::graphics::measure_text(
                                    displayLine.substr(acc, accEnd - acc)
                                        .c_str(),
                                    accSize);
                                acc = accEnd;
                            }
                        }
                        while (subStart < dispEnd) {
                            const TextStyle subStyle =
                                buffer.effectiveStyleAt(
                                    displayOffsets[std::min(
                                        subStart, segCount - 1)]);
                            const bool subInsert =
                                revisions::offsetInInsertRanges(
                                    displayOffsets[std::min(
                                        subStart, segCount - 1)],
                                    verifiedInserts);
                            const bool subDelete =
                                revisions::offsetInDeleteRanges(
                                    displayOffsets[std::min(
                                        subStart, segCount - 1)],
                                    verifiedDeletes);
                            std::size_t subEnd = subStart + 1;
                            while (subEnd < dispEnd &&
                                   revisions::offsetInInsertRanges(
                                       displayOffsets[std::min(
                                           subEnd, segCount - 1)],
                                       verifiedInserts) == subInsert &&
                                   revisions::offsetInDeleteRanges(
                                       displayOffsets[std::min(
                                           subEnd, segCount - 1)],
                                       verifiedDeletes) == subDelete &&
                                   buffer.effectiveStyleAt(
                                       displayOffsets[std::min(
                                           subEnd, segCount - 1)]) ==
                                       subStyle) {
                                ++subEnd;
                            }
                            const std::string subText =
                                displayLine.substr(subStart,
                                                   subEnd - subStart);
                            int subSize = lineFontSize;
                            int subYOff = 0;
                            if (subStyle.superscript ||
                                subStyle.subscript) {
                                subSize = std::max(
                                    8, static_cast<int>(
                                            lineFontSize * 0.75f));
                                subYOff = subStyle.superscript
                                              ? -lineFontSize / 3
                                              : lineFontSize / 4;
                            }
                            const int subW =
                                afterhours::graphics::measure_text(
                                    subText.c_str(), subSize);
                            const bool subBold =
                                paragraphStyleIsBold(paraStyle) ||
                                subStyle.bold;
                            if (subBold) {
                                afterhours::draw_text(
                                    subText.c_str(), subX, y + subYOff,
                                    subSize, theme::SELECTION_TEXT);
                                afterhours::draw_text(
                                    subText.c_str(), subX + 1, y + subYOff,
                                    subSize, theme::SELECTION_TEXT);
                            } else {
                                afterhours::draw_text(
                                    subText.c_str(), subX, y + subYOff,
                                    subSize, theme::SELECTION_TEXT);
                            }
                            if ((subInsert && !subDelete) ||
                                subStyle.underline) {
                                const int underlineY =
                                    y + lineFontSize + 1;
                                afterhours::draw_line(
                                    subX, underlineY, subX + subW,
                                    underlineY, theme::SELECTION_TEXT);
                            }
                            if (subDelete || subStyle.strikethrough) {
                                const int strikeY = y + lineFontSize / 2;
                                afterhours::draw_line(
                                    subX, strikeY, subX + subW, strikeY,
                                    theme::SELECTION_TEXT);
                            }
                            subX += subW;
                            subStart = subEnd;
                        }
                    }
                }
            } else if (!verifiedInserts.empty() ||
                       !verifiedDeletes.empty()) {
                // Split the line into insert / delete / plain
                // segments. Each display char maps to the buffer
                // offset of the raw char it came from (a tab's
                // expanded spaces all map to the tab itself),
                // mirroring expandTabs above.
                std::vector<std::size_t> displayOffsets;
                displayOffsets.reserve(displayLine.size());
                if (tabWidth <= 0) {
                    for (std::size_t i = 0; i < lineLen; ++i) {
                        displayOffsets.push_back(span.offset + i);
                    }
                } else {
                    int col = 0;
                    for (std::size_t i = 0; i < lineLen; ++i) {
                        if (lineData[i] == '\t') {
                            int spaces = tabWidth - (col % tabWidth);
                            for (int s = 0; s < spaces; ++s) {
                                displayOffsets.push_back(span.offset + i);
                            }
                            col += spaces;
                        } else {
                            displayOffsets.push_back(span.offset + i);
                            ++col;
                        }
                    }
                }
                const std::size_t segCount =
                    std::min(displayOffsets.size(), displayLine.size());
                std::size_t segStart = 0;
                while (segStart < segCount) {
                    const bool segInsert =
                        revisions::offsetInInsertRanges(
                            displayOffsets[segStart], verifiedInserts);
                    const bool segDelete =
                        revisions::offsetInDeleteRanges(
                            displayOffsets[segStart], verifiedDeletes);
                    std::size_t segEnd = segStart + 1;
                    while (segEnd < segCount &&
                           revisions::offsetInInsertRanges(
                               displayOffsets[segEnd], verifiedInserts) ==
                                   segInsert &&
                           revisions::offsetInDeleteRanges(
                               displayOffsets[segEnd], verifiedDeletes) ==
                                   segDelete) {
                        ++segEnd;
                    }
                    const std::string segText = displayLine.substr(
                        segStart, segEnd - segStart);
                    const int segX =
                        x + afterhours::graphics::measure_text(
                                displayLine.substr(0, segStart).c_str(),
                                textFontSize);
                    const afterhours::Color revisionRed{255, 0, 0, 255};
                    const afterhours::Color segColor =
                        segDelete ? revisionRed
                                  : (segInsert ? revisionGreen
                                               : (isItalic ? italicColor
                                                           : textColor));
                    if (isBold) {
                        afterhours::draw_text(segText.c_str(), segX,
                                              y + textYOffset, textFontSize,
                                              segColor);
                        afterhours::draw_text(segText.c_str(), segX + 1,
                                              y + textYOffset, textFontSize,
                                              segColor);
                    } else {
                        afterhours::draw_text(segText.c_str(), segX,
                                              y + textYOffset, textFontSize,
                                              segColor);
                    }
                    // Insert spans are always underlined (green);
                    // Delete spans are always struck through (red);
                    // other segments follow the global underline flag.
                    if ((segInsert && !segDelete) ||
                        globalStyle.underline) {
                        const int underlineY = y + lineFontSize + 1;
                        const int segW =
                            afterhours::graphics::measure_text(
                                segText.c_str(), textFontSize);
                        afterhours::draw_line(segX, underlineY, segX + segW,
                                              underlineY, segColor);
                    }
                    if (segDelete) {
                        const int strikeY = y + lineFontSize / 2;
                        const int segW =
                            afterhours::graphics::measure_text(
                                segText.c_str(), textFontSize);
                        afterhours::draw_line(segX, strikeY, segX + segW,
                                              strikeY, segColor);
                    }
                    segStart = segEnd;
                }
            } else {
                // No tracked inserts on this line: the old whole-line draw.
                if (isBold) {
                    // Draw bold effect by drawing text twice with 1px offset
                    afterhours::draw_text(textToDraw.c_str(), x, y + textYOffset, textFontSize, textColor);
                    afterhours::draw_text(textToDraw.c_str(), x + 1, y + textYOffset, textFontSize, textColor);
                } else if (isItalic) {
                    // For subtitle italic style, draw in a slightly different shade
                    afterhours::draw_text(textToDraw.c_str(), x, y + textYOffset, textFontSize, italicColor);
                } else {
                    afterhours::draw_text(textToDraw.c_str(), x, y + textYOffset, textFontSize, textColor);
                }

                // Draw underline if enabled
                if (globalStyle.underline) {
                    int underlineY = y + lineFontSize + 1;
                    afterhours::draw_line(x, underlineY, x + textWidth, underlineY, textColor);
                }
            }
            
            // Draw strikethrough if enabled (span lines draw it
            // per segment above).
            if (globalStyle.strikethrough && !lineHasCharSpans) {
                int strikeY = y + lineFontSize / 2;
                afterhours::draw_line(x, strikeY, x + textWidth, strikeY, textColor);
            }

            // Win95 inverse selection: the whole-line draw above left the
            // selected text black-on-navy. Re-draw the selected segment in
            // white (SELECTION_TEXT) over the navy background, matching
            // bold / super / subscript / underline / strikethrough.
            // Drop-cap lines skip this: their x/text are shifted, so the
            // segment math would misalign (bg rect still shows).
            // Span lines skip this too: the span path above already
            // redrew the selection per segment.
            if (hasSelection && !span.hasDropCap && !lineHasCharSpans) {
                bool lineInSelection = (row >= selStart.row && row <= selEnd.row);
                if (lineInSelection) {
                    std::size_t rawStart =
                        (row == selStart.row) ? selStart.column : 0;
                    std::size_t rawEnd =
                        (row == selEnd.row) ? selEnd.column : span.length;
                    std::size_t dispStart = rawColumnToDisplayColumn(
                        lineData, lineLen, rawStart, tabWidth);
                    std::size_t dispEnd = rawColumnToDisplayColumn(
                        lineData, lineLen, rawEnd, tabWidth);
                    dispStart = std::min(dispStart, displayLine.size());
                    dispEnd = std::min(std::max(dispEnd, dispStart), displayLine.size());
                    if (dispStart < dispEnd) {
                        std::string selText =
                            displayLine.substr(dispStart, dispEnd - dispStart);
                        int selX = x + afterhours::graphics::measure_text(
                                           displayLine.substr(0, dispStart).c_str(),
                                           textFontSize);
                        int selW = afterhours::graphics::measure_text(
                            selText.c_str(), textFontSize);
                        if (paragraphStyleIsBold(paraStyle) || globalStyle.bold) {
                            afterhours::draw_text(selText.c_str(), selX, y + textYOffset,
                                                 textFontSize, theme::SELECTION_TEXT);
                            afterhours::draw_text(selText.c_str(), selX + 1, y + textYOffset,
                                                 textFontSize, theme::SELECTION_TEXT);
                        } else {
                            afterhours::draw_text(selText.c_str(), selX, y + textYOffset,
                                                 textFontSize, theme::SELECTION_TEXT);
                        }
                        if (globalStyle.underline) {
                            int underlineY = y + lineFontSize + 1;
                            afterhours::draw_line(selX, underlineY, selX + selW, underlineY,
                                                  theme::SELECTION_TEXT);
                        }
                        if (globalStyle.strikethrough) {
                            int strikeY = y + lineFontSize / 2;
                            afterhours::draw_line(selX, strikeY, selX + selW, strikeY,
                                                  theme::SELECTION_TEXT);
                        }
                        // A selected Delete mark keeps its
                        // strikethrough (in selection white), so
                        // the mark stays visible under the
                        // highlight: split the selection by the
                        // delete flag, same tab mapping as the
                        // segmented draw above.
                        if (!verifiedDeletes.empty()) {
                            std::vector<std::size_t> selOffsets;
                            selOffsets.reserve(displayLine.size());
                            if (tabWidth <= 0) {
                                for (std::size_t i = 0; i < lineLen; ++i) {
                                    selOffsets.push_back(span.offset + i);
                                }
                            } else {
                                int col = 0;
                                for (std::size_t i = 0; i < lineLen; ++i) {
                                    if (lineData[i] == '\t') {
                                        int spaces = tabWidth - (col % tabWidth);
                                        for (int s = 0; s < spaces; ++s) {
                                            selOffsets.push_back(span.offset + i);
                                        }
                                        col += spaces;
                                    } else {
                                        selOffsets.push_back(span.offset + i);
                                        ++col;
                                    }
                                }
                            }
                            const std::size_t selCount = std::min(
                                selOffsets.size(), displayLine.size());
                            std::size_t subStart = dispStart;
                            while (subStart < dispEnd &&
                                   subStart < selCount) {
                                const bool subDelete =
                                    revisions::offsetInDeleteRanges(
                                        selOffsets[subStart], verifiedDeletes);
                                std::size_t subEnd = subStart + 1;
                                while (subEnd < dispEnd &&
                                       subEnd < selCount &&
                                       revisions::offsetInDeleteRanges(
                                           selOffsets[subEnd],
                                           verifiedDeletes) == subDelete) {
                                    ++subEnd;
                                }
                                if (subDelete) {
                                    const int subX =
                                        x + afterhours::graphics::measure_text(
                                                displayLine.substr(0, subStart)
                                                    .c_str(),
                                                textFontSize);
                                    const int subW =
                                        afterhours::graphics::measure_text(
                                            displayLine.substr(subStart,
                                                               subEnd - subStart)
                                                .c_str(),
                                            textFontSize);
                                    const int strikeY = y + lineFontSize / 2;
                                    afterhours::draw_line(
                                        subX, strikeY, subX + subW, strikeY,
                                        theme::SELECTION_TEXT);
                                }
                                subStart = subEnd;
                            }
                        }
                    }
                }
            }
        }

        // Show/Hide ¶: a pilcrow glyph at the end of each line
        // (each line is its own paragraph in this model). Skipped
        // on drop-cap lines (x/text are shifted there) and in
        // Print Preview (callers pass false there).
        if (showParagraphMarks && !span.hasDropCap) {
            afterhours::draw_text("\xC2\xB6",
                                  x + textWidth, y, lineFontSize,
                                  afterhours::Color{128, 128, 128, 255});
        }

        // Draw red squiggles under misspelled words (spell check as you type).
        // Squiggle columns are raw-line coords; map through tab expansion to
        // display columns. Drop-cap lines are skipped: the drop cap shifts
        // `x` and strips the first char, so per-word x math would be wrong.
        if (squiggles && !span.hasDropCap) {
            while (squiggleIdx < squiggles->size() &&
                   (*squiggles)[squiggleIdx].row < row) {
                ++squiggleIdx;
            }
            float waveY = static_cast<float>(y + lineFontSize + 2);
            while (squiggleIdx < squiggles->size() &&
                   (*squiggles)[squiggleIdx].row == row) {
                const SpellSquiggle& sq = (*squiggles)[squiggleIdx];
                // Tracked inserts win over squiggles: skip any
                // squiggle overlapping a verified insert span on
                // this row — the span already renders green +
                // underlined, and a red squiggle under it was the
                // pre-fix screenshot (black text + red squiggle,
                // green 1px line invisible beside it).
                if (!verifiedInserts.empty()) {
                    const std::size_t sqBegin = span.offset + sq.startCol;
                    const std::size_t sqEnd = span.offset + sq.endCol;
                    bool overlapsInsert = false;
                    for (const auto& range : verifiedInserts) {
                        if (sqBegin < range.second &&
                            sqEnd > range.first) {
                            overlapsInsert = true;
                            break;
                        }
                    }
                    if (overlapsInsert) {
                        ++squiggleIdx;
                        continue;
                    }
                }
                // Tracked deletes win over squiggles the same
                // way: a struck-through mark already reads as a
                // deletion, and a red squiggle under it would
                // fake a spelling error on deleted text.
                if (!verifiedDeletes.empty()) {
                    const std::size_t sqBegin = span.offset + sq.startCol;
                    const std::size_t sqEnd = span.offset + sq.endCol;
                    bool overlapsDelete = false;
                    for (const auto& range : verifiedDeletes) {
                        if (sqBegin < range.second &&
                            sqEnd > range.first) {
                            overlapsDelete = true;
                            break;
                        }
                    }
                    if (overlapsDelete) {
                        ++squiggleIdx;
                        continue;
                    }
                }
                // @ chips win over squiggles, same as tracked
                // inserts: a chip label is a structured name/date,
                // not free text to proof ("Ochoa" is not in the
                // dictionary, but the Gabe Ochoa chip must not
                // carry a red squiggle).
                if (!verifiedChips.empty()) {
                    const std::size_t sqBegin = span.offset + sq.startCol;
                    const std::size_t sqEnd = span.offset + sq.endCol;
                    bool overlapsChip = false;
                    for (const auto& range : verifiedChips) {
                        if (sqBegin < range.second &&
                            sqEnd > range.first) {
                            overlapsChip = true;
                            break;
                        }
                    }
                    if (overlapsChip) {
                        ++squiggleIdx;
                        continue;
                    }
                }
                // Selection wins over squiggles: skip any squiggle that
                // overlaps the selected range on this row.
                if (hasSelection && row >= selStart.row && row <= selEnd.row) {
                    std::size_t selRawStart =
                        (row == selStart.row) ? selStart.column : 0;
                    std::size_t selRawEnd =
                        (row == selEnd.row) ? selEnd.column : span.length;
                    if (sq.startCol < selRawEnd && sq.endCol > selRawStart) {
                        ++squiggleIdx;
                        continue;
                    }
                }
                std::size_t dispStart = rawColumnToDisplayColumn(
                    lineData, lineLen, sq.startCol, tabWidth);
                std::size_t dispEnd = rawColumnToDisplayColumn(
                    lineData, lineLen, sq.endCol, tabWidth);
                dispStart = std::min(dispStart, displayLine.size());
                dispEnd = std::min(std::max(dispEnd, dispStart), displayLine.size());
                // Squiggle x from the line's layout x (column
                // origin via columnLineOriginX + indents +
                // alignment) plus the word's offset in the line:
                // the same origin text rendering used, so the
                // underline sits under its word in every column.
                float x1 = static_cast<float>(layoutTextX) +
                           static_cast<float>(afterhours::graphics::measure_text(
                               displayLine.substr(0, dispStart).c_str(), lineFontSize));
                float x2 = static_cast<float>(layoutTextX) +
                           static_cast<float>(afterhours::graphics::measure_text(
                               displayLine.substr(0, dispEnd).c_str(), lineFontSize));
                if (x2 > x1) drawSpellSquiggle(x1, x2, waveY);
                ++squiggleIdx;
            }
        }

        // (Tracked-insert underlines are drawn with the segmented
        // text above, in the same pass as the green glyphs — the
        // old separate underline block drew a 1px line that was
        // invisible next to the red squiggle.)

        // Draw caret
        if (caretVisible && row == caret.row) {
            std::string beforeCaret =
                displayLine.substr(0, std::min(caret.column, displayLine.length()));
            int caretX = x + afterhours::graphics::measure_text(beforeCaret.c_str(), lineFontSize);
            afterhours::draw_rectangle(
                Rectangle{static_cast<float>(caretX), static_cast<float>(y), 2.0f,
                                  static_cast<float>(lineHeight)},
                theme::CARET_COLOR);
        }

        // Advance y by the paragraph's RESERVED height plus paragraph
        // spacing after. A drop-cap paragraph reserves
        // drop_cap::reservedRows(cap, 1) rows (this renderer draws
        // one visual row per paragraph), so the next paragraph
        // begins below the glyph instead of beside it — its first
        // word used to print through the glyph's lower half. The
        // glyph itself draws top-aligned inside this box (above).
        int paragraphSpaceAfter = buffer.lineSpaceAfter(row);
        y += static_cast<int>(drop_cap::reservedRows(
                 buffer.lineDropCapLines(row), 1)) *
                 lineHeight +
             paragraphSpaceAfter;
        drawnAnyRow = true;

        if (y > static_cast<int>(textArea.y + textArea.height)) {
            // In columns mode an overflowing row (e.g. a tall heading)
            // must not drop the remaining columns.
            if (columnsActive) continue;
            break;
        }
    }
}

// System for rendering the complete editor UI
// Filled triangle via horizontal scanlines (no draw_triangle in the
// backend-agnostic drawing API). Apex (ax, ay), base endpoints
// (ax - halfWidth, baseY) and (ax + halfWidth, baseY).
inline void drawFilledTriangle(float ax, float ay, float baseY, float halfWidth,
                               afterhours::Color color) {
    int top = static_cast<int>(std::min(ay, baseY));
    int bottom = static_cast<int>(std::max(ay, baseY));
    for (int y = top; y <= bottom; ++y) {
        float t = (bottom == top)
                      ? 1.0f
                      : (static_cast<float>(y - top) /
                         static_cast<float>(bottom - top));
        float half = (ay < baseY) ? halfWidth * t : halfWidth * (1.0f - t);
        int x1 = static_cast<int>(ax - half);
        int x2 = static_cast<int>(ax + half);
        afterhours::draw_line(x1, y, x2, y, color);
    }
}

// Word 95 ruler marker: pale raised/beveled triangle — light face,
// dark edge lines, with the top/left edges highlighted (light comes
// from the top-left, as in the Win95 scrollbar-thumb bevel). Not a
// solid-black glyph: the face is BUTTON_FACE gray, the outline is
// dark, the top/left edges are white.
inline void drawBeveledTriangle(const ruler_math::Triangle& t) {
    const afterhours::Color face = theme::BUTTON_FACE;
    const afterhours::Color dark = theme::RULER_MARKS;
    const afterhours::Color light = theme::BORDER_LIGHT;
    drawFilledTriangle(t.apexX, t.apexY, t.baseY, t.halfWidth, face);
    int ax = static_cast<int>(t.apexX);
    int ay = static_cast<int>(t.apexY);
    int baseY = static_cast<int>(t.baseY);
    int leftX = static_cast<int>(t.apexX - t.halfWidth);
    int rightX = static_cast<int>(t.apexX + t.halfWidth);
    // Left (top-left-lit) edge: white. Right edge: dark.
    afterhours::draw_line(ax, ay, leftX, baseY, light);
    afterhours::draw_line(ax, ay, rightX, baseY, dark);
    // Base: light when it is the top edge (apex down), dark when it
    // is the bottom edge (apex up).
    afterhours::draw_line(leftX, baseY, rightX, baseY,
                          t.baseY < t.apexY ? light : dark);
}

// Word 95 hanging-marker rectangle base: same raised bevel as the
// triangles (light top/left, dark bottom/right, gray face).
inline void drawBeveledRect(float x, float y, float w, float h) {
    afterhours::draw_rectangle(Rectangle{x, y, w, h}, theme::BUTTON_FACE);
    int xi = static_cast<int>(x);
    int yi = static_cast<int>(y);
    int xe = static_cast<int>(x + w);
    int ye = static_cast<int>(y + h);
    afterhours::draw_line(xi, yi, xe, yi, theme::BORDER_LIGHT);
    afterhours::draw_line(xi, yi, xi, ye, theme::BORDER_LIGHT);
    afterhours::draw_line(xe, yi, xe, ye, theme::RULER_MARKS);
    afterhours::draw_line(xi, ye, xe, ye, theme::RULER_MARKS);
}

struct EditorRenderSystem
    : public afterhours::System<DocumentComponent, CaretComponent,
                                ScrollComponent,
                                LayoutComponent> {
    void once(const float) const override {
        // Note: begin_drawing() and clear_background() are now called in app_frame()
        // before sm.run(dt), so that all render systems (including RenderImm for UI)
        // draw within the same GPU pass.
    }

    void after(const float) const override {
        // Take screenshots before EndDrawing (must be done while buffer is valid)
        auto testConfigs = afterhours::EntityQuery({.force_merge = true})
                               .whereHasComponent<TestConfigComponent>()
                               .gen();
        for (auto& ref : testConfigs) {
            auto& testConfig = ref.get().get<TestConfigComponent>();
            if (testConfig.enabled) {
                testConfig.frameCount++;
                // Take screenshot on frame 2 (frame 1 might not have rendered yet)
                if (testConfig.frameCount == 2) {
                    // Create directory and take screenshot
                    std::filesystem::create_directories(testConfig.screenshotDir);
                    std::string pathStr = testConfig.screenshotDir + "/01_startup.png";
                    LOG_INFO("Taking startup screenshot: %s", pathStr.c_str());
                    deferred_screenshot::request(std::filesystem::path(pathStr));
                    // File lands at deferred_screenshot::flush() after end_drawing
                }
            }
            
            // Draw E2E debug overlay if enabled
            if (testConfig.e2eDebugOverlay && !testConfig.e2eCurrentCommand.empty()) {
                int screenWidth = afterhours::graphics::get_screen_width();
                int overlayWidth = 400;
                int overlayHeight = 50;
                int overlayX = screenWidth - overlayWidth - 10;
                int overlayY = 10;
                
                // Draw semi-transparent background
                afterhours::draw_rectangle(
                    Rectangle{static_cast<float>(overlayX), static_cast<float>(overlayY),
                                      static_cast<float>(overlayWidth), static_cast<float>(overlayHeight)},
                    afterhours::Color{0, 0, 0, 200});
                afterhours::draw_rectangle_outline(
                    Rectangle{static_cast<float>(overlayX), static_cast<float>(overlayY),
                                      static_cast<float>(overlayWidth), static_cast<float>(overlayHeight)},
                    afterhours::Color{255, 255, 0, 255}, 1.0f);
                
                // Draw current command
                std::string cmdText = testConfig.e2eCurrentCommand;
                if (cmdText.length() > 40) {
                    cmdText = cmdText.substr(0, 37) + "...";
                }
                theme::DrawUIText(cmdText.c_str(), overlayX + 5, overlayY + 5, 14, 
                                 afterhours::Color{255, 255, 255, 255});
                
                // Draw timeout countdown
                std::string timeoutText;
                if (testConfig.e2eTimeoutSeconds >= 0) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "<%.1fs>", testConfig.e2eTimeoutSeconds);
                    timeoutText = buf;
                } else {
                    timeoutText = "<no timeout>";
                }
                theme::DrawUIText(timeoutText.c_str(), overlayX + 5, overlayY + 25, 14, 
                                 afterhours::Color{255, 200, 100, 255});
            }
        }
        // Note: end_drawing() is now called in app_frame() after sm.run(dt)
    }

    void for_each_with(afterhours::Entity& /*entity*/,
                       DocumentComponent& doc,
                       CaretComponent& caret,
                       ScrollComponent& scroll,
                       LayoutComponent& layout,
                       const float) override {
        // Draw horizontal ruler (Word 95 style): the band sits at
        // textArea.y - rulerHeight, directly below the Formatting
        // toolbar (component_helpers reserves it above the text area).
        // Gray outside the margins, white between them, inch numbers +
        // ticks from the left margin, indent triangles for the caret
        // paragraph, L-shaped default tab-stop marks.
        if (ruler::visible(layout)) {
            LayoutComponent::Rect r = ruler::rect(layout);
            ruler_math::Geometry g = ruler::geometry(layout);
            Rectangle rulerRect = {r.x, r.y, r.width, r.height};
            float rulerY = r.y;
            float rulerH = r.height;

            // Gray outside the margins, white between left/right margins.
            afterhours::draw_rectangle(
                rulerRect, afterhours::Color{192, 192, 192, 255});
            // Columns mode: per-column white spans with the gray
            // gutter between them (the ruler margin shading uses
            // the column boxes, not one 6.5in span). The padding
            // zones outside the first/last column stay white.
            // The tick loop below also uses these spans to keep
            // inch numbers off the gray gutters.
            const bool columnsRuler =
                layout.pageMode == PageMode::Paged &&
                !layout.previewMode &&
                doc.docSettings.pageSettings.columnCount > 1;
            const float columnsSpacingPx =
                columnsRuler ? doc.docSettings.pageSettings.columnSpacing *
                                   layout::displayScale(layout)
                             : 0.0f;
            const auto columnsSpans = ruler_math::columnWhiteSpans(
                g,
                columnsRuler ? doc.docSettings.pageSettings.columnCount : 1,
                columnsSpacingPx);
            if (columnsRuler) {
                afterhours::draw_rectangle(
                    Rectangle{g.whiteLeft, rulerY, g.originX - g.whiteLeft,
                              rulerH},
                    theme::RULER_BG);
                for (const auto& span : columnsSpans) {
                    afterhours::draw_rectangle(
                        Rectangle{span.first, rulerY, span.second - span.first,
                                  rulerH},
                        theme::RULER_BG);
                }
                if (!columnsSpans.empty()) {
                    afterhours::draw_rectangle(
                        Rectangle{columnsSpans.back().second, rulerY,
                                  g.whiteRight - columnsSpans.back().second,
                                  rulerH},
                        theme::RULER_BG);
                }
            } else {
                afterhours::draw_rectangle(
                    Rectangle{g.whiteLeft, rulerY, g.whiteRight - g.whiteLeft,
                              rulerH},
                    theme::RULER_BG);
            }
            util::drawSunkenBorder(rulerRect);

            // Ticks every quarter inch from the left margin (inch 0),
            // running both ways across the ruler. Numbers 1.. only
            // inside the white band.
            auto drawTick = [&](float x, int quarter) {
                int tickTop;
                if (quarter % 4 == 0) {
                    tickTop = static_cast<int>(rulerY + rulerH - 8);
                } else if (quarter % 2 == 0) {
                    tickTop = static_cast<int>(rulerY + rulerH - 6);
                } else {
                    tickTop = static_cast<int>(rulerY + rulerH - 4);
                }
                afterhours::draw_line(static_cast<int>(x), tickTop,
                                      static_cast<int>(x),
                                      static_cast<int>(rulerY + rulerH - 1),
                                      theme::RULER_MARKS);
            };
            // Tick spacing is g.pixelsPerInch — the ONE scale source —
            // so in paged mode (fit ~0.59) ticks sit ~42px apart and
            // the page right edge spans exactly 8.5" from the page
            // left edge (origin - margin), not 72px apart.
            for (int q = 0;; ++q) {
                float x = g.originX + static_cast<float>(q) *
                                         g.pixelsPerInch / 4.0f;
                if (x > static_cast<float>(layout.screenWidth)) break;
                drawTick(x, q);
                // Inch numbers sit on white only: in columns mode
                // a tick in a gray gutter gets no number. The
                // padding zones count as white (same as the spans
                // above).
                bool onWhite = x >= g.whiteLeft && x <= g.whiteRight;
                if (columnsRuler) {
                    onWhite = x < g.originX || x > columnsSpans.back().second;
                    for (const auto& span : columnsSpans) {
                        if (x >= span.first && x <= span.second) {
                            onWhite = true;
                        }
                    }
                }
                if (q % 4 == 0 && q > 0 && onWhite) {
                    std::string inchStr = std::to_string(q / 4);
                    int textWidth = theme::MeasureUIText(inchStr.c_str(), 10);
                    theme::DrawUIText(inchStr.c_str(),
                                      static_cast<int>(x) - textWidth / 2,
                                      static_cast<int>(rulerY) + 1, 10,
                                      theme::RULER_TEXT);
                }
                if (q == 0) {
                    // Mirror ticks left of inch 0, into the gray margin.
                    for (int ql = -1;; --ql) {
                        float xl = g.originX +
                                   static_cast<float>(ql) *
                                       g.pixelsPerInch / 4.0f;
                        if (xl < 0.0f) break;
                        drawTick(xl, ql);
                    }
                }
            }

            // Default tab stops: small L marks along the bottom edge.
            // The space width is measured at the scaled font, so
            // tab stops land where the scaled text's tabs land.
            {
                TextStyle tabStyle = doc.buffer.textStyle();
                int tabFontSize = ruler_math::scaledFontSize(
                    tabStyle.fontSize, layout::displayScale(layout));
                float spaceWidth = static_cast<float>(
                    afterhours::graphics::measure_text(" ", tabFontSize));
                for (float x : ruler_math::tabStopXs(
                         g, doc.docSettings.tabWidth, spaceWidth)) {
                    afterhours::draw_line(static_cast<int>(x),
                                          static_cast<int>(rulerY + rulerH - 6),
                                          static_cast<int>(x),
                                          static_cast<int>(rulerY + rulerH - 1),
                                          theme::RULER_MARKS);
                    afterhours::draw_line(static_cast<int>(x),
                                          static_cast<int>(rulerY + rulerH - 1),
                                          static_cast<int>(x) + 4,
                                          static_cast<int>(rulerY + rulerH - 1),
                                          theme::RULER_MARKS);
                }
            }

            // Indent markers for the caret paragraph: Word 95
            // beveled hourglass pair + rectangle base + right
            // triangle, from ruler_math::markerShapes (same
            // positions the drag hit-test uses). Directions point
            // toward the centre; faces are pale gray, edges dark.
            {
                std::size_t row = doc.buffer.caret().row;
                auto shapes = ruler_math::markerShapes(
                    g, doc.buffer.lineLeftIndent(row),
                    doc.buffer.lineFirstLineIndent(row),
                    doc.buffer.lineRightIndent(row));
                drawBeveledTriangle(shapes.firstLine);
                drawBeveledTriangle(shapes.left);
                drawBeveledRect(shapes.rectX, shapes.rectY, shapes.rectW,
                                shapes.rectH);
                drawBeveledTriangle(shapes.right);
            }
        }

        // Draw text area background
        Rectangle textAreaRect = {layout.textArea.x, layout.textArea.y,
                                          layout.textArea.width,
                                          layout.textArea.height};

        // In paged mode, draw a gray background; in pageless mode, draw white
        if (layout.pageMode == PageMode::Paged) {
            afterhours::draw_rectangle(textAreaRect,
                                     afterhours::Color{128, 128, 128, 255});
            util::drawSunkenBorder(textAreaRect);

            // Draw the page with shadow; margin boundary guides
            // only when the Text boundaries option is enabled.
            drawPageBackground(layout, doc.docSettings.showTextBoundaries);
        } else {
            afterhours::draw_rectangle(textAreaRect, theme::TEXT_AREA_BG);
            util::drawSunkenBorder(textAreaRect);
        }

        // Render text buffer using effective text area (respects page margins).
        // The document font scales by displayScale (fit * zoom in
        // paged mode): at the ~0.58 paged fit a 16px base renders
        // at 9px, so the text sits in the page's coordinate scale
        // instead of overflowing the 6.5" text width at full size.
        // Pageless displayScale == zoomLevel, so this is unchanged.
        TextStyle style = doc.buffer.textStyle();
        float ds = layout::displayScale(layout);
        int fontSize = ruler_math::scaledFontSize(style.fontSize, ds);
        int lineHeight = fontSize + 4;
        LayoutComponent::Rect effectiveArea = layout::effectiveTextArea(layout);

        // Print Preview: the rendered rows start at the current preview
        // page (previewPage * previewLinesPerPage), not at scroll.offset;
        // the caret is hidden (read-only mode) and split view is ignored.
        int renderScrollOffset = scroll.offset;
        bool caretVisible = caret.visible;
        if (layout.previewMode) {
            renderScrollOffset = scroll_math::previewFirstLine(
                layout.previewPage, layout.previewLinesPerPage);
            caretVisible = false;
        }

        // Columns (Format > Columns...): paged editing view only,
        // never Print Preview (which uses its own line stride).
        // Computed here because the squiggle cache below needs the
        // page size, and the render calls below reuse all three.
        bool columnsActive =
            layout.pageMode == PageMode::Paged && !layout.previewMode &&
            doc.docSettings.pageSettings.columnCount > 1;
        int columnsLinesPerPage = static_cast<int>(
            columns_math::linesPerColumnForPage(
                layout.pageHeight, layout.marginTop, layout.marginBottom,
                ds, lineHeight));
        float columnsSpacingPx =
            columnsActive
                ? doc.docSettings.pageSettings.columnSpacing * ds
                : 0.0f;

        // Refresh the spell-squiggle cache for the visible rows (union of
        // both panes in split view). No-op when the buffer version and the
        // visible range are unchanged, so this costs nothing per frame.
        // Skipped in Print Preview: preview shows printable content
        // only, so no squiggles are computed or rendered there.
        // Also gated by Tools > Options "Spell check as you type".
        bool wantSquiggles =
            !layout.previewMode && doc.docSettings.spellCheckAsYouType;
        if (wantSquiggles) {
            int visible = scroll.visibleLines > 0 ? scroll.visibleLines : 20;
            std::size_t firstRow = renderScrollOffset > 0
                                       ? static_cast<std::size_t>(renderScrollOffset)
                                       : 0;
            std::size_t lastRow = firstRow + static_cast<std::size_t>(visible) + 1;
            if (layout.splitViewEnabled) {
                std::size_t secondFirst = scroll.secondaryOffset > 0
                                              ? static_cast<std::size_t>(
                                                    scroll.secondaryOffset)
                                              : 0;
                firstRow = std::min(firstRow, secondFirst);
                lastRow = std::max(lastRow,
                                   secondFirst + static_cast<std::size_t>(visible) + 1);
            }
            // Columns: the rendered page holds pageSize rows ACROSS
            // all columns (pageFirst..pageFirst+pageSize), not the
            // single-column visible window [scrollOffset,
            // +visible). The cache must cover the whole page or the
            // squiggles for rows outside that window silently
            // vanish: in the batch screenshot column 1 was clean
            // and column 2 was squiggled from mid-page, because
            // the cache only held the scrolled window.
            if (columnsActive) {
                std::size_t pageSize =
                    static_cast<std::size_t>(columnsLinesPerPage) *
                    static_cast<std::size_t>(
                        doc.docSettings.pageSettings.columnCount);
                if (pageSize > 0) {
                    auto pageRange = [&](int offset) {
                        std::size_t off = offset > 0 ? static_cast<std::size_t>(offset) : 0;
                        std::size_t pageFirst = (off / pageSize) * pageSize;
                        return std::pair{pageFirst, pageFirst + pageSize};
                    };
                    auto range = pageRange(renderScrollOffset);
                    firstRow = range.first;
                    lastRow = range.second;
                    if (layout.splitViewEnabled) {
                        auto second = pageRange(scroll.secondaryOffset);
                        firstRow = std::min(firstRow, second.first);
                        lastRow = std::max(lastRow, second.second);
                    }
                }
            }
            doc.spellSquiggles.update(doc.buffer, doc.spellChecker, firstRow, lastRow);
        }
        const SpellSquiggleCache* squiggleCache =
            wantSquiggles ? &doc.spellSquiggles : nullptr;
        // Line numbers are an editing adornment: hidden in preview.
        bool showLineNumbers =
            layout.showLineNumbers && !layout.previewMode;
        // Same for ¶ marks: shown only in the editing view.
        bool showParagraphMarks =
            layout.showParagraphMarks && !layout.previewMode;
        // Outline mode: heading rows only (not in Print Preview:
        // preview shows printable content, not the outline).
        bool outlineMode = layout.showOutline && !layout.previewMode;
        // Normal view (View > Normal): pageless continuous text
        // plus dashed computed page-break lines. The lines-per-
        // page is columnsLinesPerPage above: page text height /
        // line height, the same estimate Print Preview uses
        // (in Normal displayScale == zoomLevel, so the fit
        // factor is 1). The ruler stays: pageless text uses
        // the full text-area width, not a page frame.
        bool normalMode = layout::isNormalView(layout);
        // columnsActive / columnsLinesPerPage / columnsSpacingPx
        // were computed above (the squiggle cache needs them).

        // Tables: draw on the (empty) text line they sit on,
        // after the text. The active cell caret (Batch 8) draws
        // in its table, and the body caret hides while a cell
        // edit is active — the cell caret is the caret.
        // Positions are the helper's naive line offsets: exact
        // for a fresh document's line 0; paged multi-page
        // alignment is approximate.
        const CellEditState* cellEdit =
            (doc.cellEdit.active && !layout.previewMode)
                ? &doc.cellEdit
                : nullptr;
        if (doc.cellEdit.active) caretVisible = false;
        // The body caret hides while a header/footer is active
        // — the header/footer caret is the caret.
        if (layout.headerEditTarget != HeaderEditTarget::None) {
            caretVisible = false;
        }
        auto drawTables = [&](const LayoutComponent::Rect& area,
                              int scrollOffset) {
            renderDocumentTables(doc.tables, area, lineHeight,
                                 scrollOffset, cellEdit, caret.visible,
                                 &doc.buffer);
        };

        if (!layout.previewMode && layout.splitViewEnabled) {
            float splitHeight = effectiveArea.height * 0.5f;
            LayoutComponent::Rect topArea = {effectiveArea.x, effectiveArea.y,
                                             effectiveArea.width, splitHeight - 4.0f};
            LayoutComponent::Rect bottomArea = {effectiveArea.x, effectiveArea.y + splitHeight + 4.0f,
                                                effectiveArea.width, splitHeight - 4.0f};
            renderTextBuffer(doc.buffer, topArea, caretVisible, fontSize,
                             lineHeight, renderScrollOffset, showLineNumbers,
                             layout.lineNumberGutterWidth, doc.docSettings.tabWidth,
                             ds, squiggleCache,
                             layout::displayScale(layout),
                             layout.previewMode, showParagraphMarks, outlineMode,
                             columnsActive ? doc.docSettings.pageSettings.columnCount : 1,
                             columnsSpacingPx, columnsLinesPerPage,
                             layout.outlineShowLevel,
                             &layout.outlineCollapsedRows,
                             layout.previewMode ? nullptr : &doc.comments,
                             layout.previewMode ? nullptr : &doc.revisions,
                             layout.previewMode ? nullptr : &doc.chips,
                             normalMode, columnsLinesPerPage,
                             layout.pageMode == PageMode::Paged);
            renderTextBuffer(doc.buffer, bottomArea, caretVisible, fontSize,
                             lineHeight, scroll.secondaryOffset, showLineNumbers,
                             layout.lineNumberGutterWidth, doc.docSettings.tabWidth,
                             ds, squiggleCache,
                             layout::displayScale(layout),
                             layout.previewMode, showParagraphMarks, outlineMode,
                             columnsActive ? doc.docSettings.pageSettings.columnCount : 1,
                             columnsSpacingPx, columnsLinesPerPage,
                             layout.outlineShowLevel,
                             &layout.outlineCollapsedRows,
                             layout.previewMode ? nullptr : &doc.comments,
                             layout.previewMode ? nullptr : &doc.revisions,
                             layout.previewMode ? nullptr : &doc.chips,
                             normalMode, columnsLinesPerPage,
                             layout.pageMode == PageMode::Paged);
            drawTables(topArea, renderScrollOffset);
            drawTables(bottomArea, scroll.secondaryOffset);

            // Split divider
            afterhours::draw_line(static_cast<int>(effectiveArea.x),
                             static_cast<int>(effectiveArea.y + splitHeight),
                             static_cast<int>(effectiveArea.x + effectiveArea.width),
                             static_cast<int>(effectiveArea.y + splitHeight),
                             theme::BORDER_DARK);
        } else {
            renderTextBuffer(doc.buffer, effectiveArea, caretVisible, fontSize,
                             lineHeight, renderScrollOffset, showLineNumbers,
                             layout.lineNumberGutterWidth, doc.docSettings.tabWidth,
                             ds, squiggleCache,
                             layout::displayScale(layout),
                             layout.previewMode, showParagraphMarks, outlineMode,
                             columnsActive ? doc.docSettings.pageSettings.columnCount : 1,
                             columnsSpacingPx, columnsLinesPerPage,
                             layout.outlineShowLevel,
                             &layout.outlineCollapsedRows,
                             layout.previewMode ? nullptr : &doc.comments,
                             layout.previewMode ? nullptr : &doc.revisions,
                             layout.previewMode ? nullptr : &doc.chips,
                             normalMode, columnsLinesPerPage,
                             layout.pageMode == PageMode::Paged);
            drawTables(effectiveArea, renderScrollOffset);
        }

        // Header / footer (Batch 9): paged editing + Print
        // Preview only, one page (the page the renderer drew).
        // Page number from the same lines-per-page estimate
        // the status bar / preview use; in preview the current
        // preview page, in editing the page of the first
        // rendered line.
        if (layout.pageMode == PageMode::Paged && !layout.showOutline) {
            int hfTotalPages =
                layout.previewMode
                    ? layout.previewPageCount
                    : scroll_math::previewPageCount(
                          doc.buffer.lineCount(), columnsLinesPerPage);
            int hfPage = layout.previewMode
                             ? layout.previewPage + 1
                             : scroll_math::pageFromScrollOffset(
                                   renderScrollOffset,
                                   columnsLinesPerPage,
                                   doc.buffer.lineCount());
            renderHeaderFooter(doc, layout, hfPage, hfTotalPages,
                               caret.visible && !layout.previewMode);
        }

        // === Draw Vertical Scroll Bar (Win95) ===
        // Geometry identical to ScrollbarSystem via scroll_math, so the
        // thumb the user drags is the thumb that is drawn.
        // Skipped in Print Preview: the thumb tracks scroll.offset,
        // not the preview page (Next/Prev page there).
        if (!layout.previewMode) {
            float sbWidth = scroll_math::SCROLLBAR_WIDTH;
            float sbX = textAreaRect.x + textAreaRect.width - sbWidth;
            float sbY = textAreaRect.y;
            float sbHeight = textAreaRect.height;
            int totalLines = static_cast<int>(doc.buffer.lineCount());
            int visibleLines = scroll.visibleLines > 0 ? scroll.visibleLines : 20;
            int maxScroll = std::max(0, totalLines - visibleLines);
            auto g = scroll_math::scrollbarGeometry(sbY, sbHeight, totalLines, visibleLines,
                                                   maxScroll, scroll.offset);

            auto drawArrowButton = [&](float y, bool up, bool pressed) {
                // Raised bevel button; sunken + darker while pressed.
                afterhours::draw_rectangle(Rectangle{sbX, y, sbWidth, g.arrowHeight},
                                           pressed ? theme::BUTTON_PRESSED_BG : theme::BUTTON_FACE);
                afterhours::Color topLeft = pressed ? theme::BORDER_DARK : theme::BORDER_LIGHT;
                afterhours::Color bottomRight = pressed ? theme::BORDER_LIGHT : theme::BORDER_DARK;
                afterhours::draw_line(static_cast<int>(sbX), static_cast<int>(y),
                                      static_cast<int>(sbX + sbWidth - 1), static_cast<int>(y), topLeft);
                afterhours::draw_line(static_cast<int>(sbX), static_cast<int>(y),
                                      static_cast<int>(sbX), static_cast<int>(y + g.arrowHeight - 1), topLeft);
                afterhours::draw_line(static_cast<int>(sbX + sbWidth - 1), static_cast<int>(y),
                                      static_cast<int>(sbX + sbWidth - 1), static_cast<int>(y + g.arrowHeight - 1),
                                      bottomRight);
                afterhours::draw_line(static_cast<int>(sbX), static_cast<int>(y + g.arrowHeight - 1),
                                      static_cast<int>(sbX + sbWidth - 1), static_cast<int>(y + g.arrowHeight - 1),
                                      bottomRight);
                // Triangle glyph.
                float cx = sbX + sbWidth / 2.0f;
                afterhours::Color glyph = theme::BUTTON_TEXT;
                if (up) {
                    afterhours::draw_line(static_cast<int>(cx), static_cast<int>(y + 4),
                                          static_cast<int>(cx - 4), static_cast<int>(y + 11), glyph);
                    afterhours::draw_line(static_cast<int>(cx), static_cast<int>(y + 4),
                                          static_cast<int>(cx + 4), static_cast<int>(y + 11), glyph);
                    afterhours::draw_line(static_cast<int>(cx - 4), static_cast<int>(y + 11),
                                          static_cast<int>(cx + 4), static_cast<int>(y + 11), glyph);
                } else {
                    afterhours::draw_line(static_cast<int>(cx), static_cast<int>(y + g.arrowHeight - 4),
                                          static_cast<int>(cx - 4), static_cast<int>(y + g.arrowHeight - 11), glyph);
                    afterhours::draw_line(static_cast<int>(cx), static_cast<int>(y + g.arrowHeight - 4),
                                          static_cast<int>(cx + 4), static_cast<int>(y + g.arrowHeight - 11), glyph);
                    afterhours::draw_line(static_cast<int>(cx - 4), static_cast<int>(y + g.arrowHeight - 11),
                                          static_cast<int>(cx + 4), static_cast<int>(y + g.arrowHeight - 11), glyph);
                }
            };

            drawArrowButton(g.upArrowY, true, scroll.scrollbarRepeatDir == -1);
            drawArrowButton(sbY + sbHeight - g.arrowHeight, false, scroll.scrollbarRepeatDir == 1);

            // Track (sunken). No stipple pattern in theme, so the track is
            // flat BUTTON_FACE with a sunken bevel; stipple would go here.
            afterhours::draw_rectangle(Rectangle{sbX, g.trackY, sbWidth, g.trackHeight},
                                       theme::BUTTON_FACE);
            afterhours::draw_line(static_cast<int>(sbX), static_cast<int>(g.trackY),
                                  static_cast<int>(sbX), static_cast<int>(g.trackY + g.trackHeight),
                                  theme::BORDER_DARK);
            afterhours::draw_line(static_cast<int>(sbX + sbWidth - 1), static_cast<int>(g.trackY),
                                  static_cast<int>(sbX + sbWidth - 1), static_cast<int>(g.trackY + g.trackHeight),
                                  theme::BORDER_LIGHT);

            // Thumb (raised, darker while dragging).
            if (g.thumbVisible) {
                bool dragging = scroll.scrollbarDragging;
                afterhours::draw_rectangle(
                    Rectangle{sbX + 1, g.thumbY, sbWidth - 2, g.thumbHeight},
                    dragging ? theme::BUTTON_PRESSED_BG : theme::BUTTON_FACE);
                afterhours::Color topLeft = dragging ? theme::BORDER_DARK : theme::BORDER_LIGHT;
                afterhours::Color bottomRight = dragging ? theme::BORDER_LIGHT : theme::BORDER_DARK;
                afterhours::draw_line(static_cast<int>(sbX + 1), static_cast<int>(g.thumbY),
                                      static_cast<int>(sbX + sbWidth - 2), static_cast<int>(g.thumbY),
                                      topLeft);
                afterhours::draw_line(static_cast<int>(sbX + 1), static_cast<int>(g.thumbY),
                                      static_cast<int>(sbX + 1), static_cast<int>(g.thumbY + g.thumbHeight),
                                      topLeft);
                afterhours::draw_line(static_cast<int>(sbX + sbWidth - 2), static_cast<int>(g.thumbY),
                                      static_cast<int>(sbX + sbWidth - 2), static_cast<int>(g.thumbY + g.thumbHeight),
                                      bottomRight);
                afterhours::draw_line(static_cast<int>(sbX + 1), static_cast<int>(g.thumbY + g.thumbHeight - 1),
                                      static_cast<int>(sbX + sbWidth - 2), static_cast<int>(g.thumbY + g.thumbHeight - 1),
                                      bottomRight);
            }
        }

        // Comment cards are UI in MenuUISystem (clickable, with a
        // Resolve button); the old 6px margin marker is gone. The
        // anchor highlight is drawn in renderTextBuffer above.

        // Note: About, Word Count, and Help dialogs now rendered by MenuUISystem using afterhours modal.h
    }
};


}  // namespace ecs
