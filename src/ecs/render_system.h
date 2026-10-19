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
#include "../editor/columns_math.h"
#include "../editor/outline_view.h"
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
// Uses the UI font for consistent rendering across all UI elements
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
    // on — by default the page is plain white. Margin from the ONE
    // scale source: pageMargin(points) * displayScale, so the
    // margin lines sit exactly on the ruler's 1" marks.
    if (!showTextBoundaries) return;
    float marginScaled = layout.pageMargin * layout::displayScale(layout);
    afterhours::Color marginColor = afterhours::Color{200, 200, 200, 100};

    // Left margin
    afterhours::draw_line(static_cast<int>(layout.pageOffsetX + marginScaled),
                     static_cast<int>(pageY),
                     static_cast<int>(layout.pageOffsetX + marginScaled),
                     static_cast<int>(pageY + layout.pageDisplayHeight),
                     marginColor);

    // Right margin
    afterhours::draw_line(static_cast<int>(layout.pageOffsetX +
                                      layout.pageDisplayWidth - marginScaled),
                     static_cast<int>(pageY),
                     static_cast<int>(layout.pageOffsetX +
                                      layout.pageDisplayWidth - marginScaled),
                     static_cast<int>(pageY + layout.pageDisplayHeight),
                     marginColor);

    // Top margin
    afterhours::draw_line(
        static_cast<int>(layout.pageOffsetX),
        static_cast<int>(pageY + marginScaled),
        static_cast<int>(layout.pageOffsetX + layout.pageDisplayWidth),
        static_cast<int>(pageY + marginScaled), marginColor);

    // Bottom margin
    afterhours::draw_line(
        static_cast<int>(layout.pageOffsetX),
        static_cast<int>(pageY + layout.pageDisplayHeight - marginScaled),
        static_cast<int>(layout.pageOffsetX + layout.pageDisplayWidth),
        static_cast<int>(pageY + layout.pageDisplayHeight - marginScaled),
        marginColor);
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

// Render a table at a specific position
inline void renderTable(const Table& table, float tableX, float tableY, 
                        CellPosition currentCell, bool isEditing) {
    if (table.isEmpty()) return;
    
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
            
            // Draw cell borders
            afterhours::Color borderColor = afterhours::Color{0, 0, 0, 255};
            drawBorderEdge(cellX, cellY, cellX + cellW, cellY, cell.borders.top, borderColor);
            drawBorderEdge(cellX, cellY + cellH, cellX + cellW, cellY + cellH, cell.borders.bottom, borderColor);
            drawBorderEdge(cellX, cellY, cellX, cellY + cellH, cell.borders.left, borderColor);
            drawBorderEdge(cellX + cellW, cellY, cellX + cellW, cellY + cellH, cell.borders.right, borderColor);
            
            // Draw cell content
            if (!cell.content.empty()) {
                int textX = static_cast<int>(cellX) + cell.paddingLeft;
                int textY = static_cast<int>(cellY) + cell.paddingTop;
                int fontSize = cell.textStyle.fontSize;
                afterhours::Color textColor = {cell.textStyle.textColor.r, cell.textStyle.textColor.g,
                                          cell.textStyle.textColor.b, cell.textStyle.textColor.a};
                afterhours::draw_text(cell.content.c_str(), textX, textY, fontSize, textColor);
            }
            
            // Highlight current cell if editing
            if (isEditing && row == currentCell.row && col == currentCell.col) {
                afterhours::draw_rectangle_outline(
                    Rectangle{cellX, cellY, cellW, cellH},
                    afterhours::Color{0, 120, 215, 255}, 2.0f);  // Blue highlight
            }
        }
    }
}

// Render all tables in a document at their line positions
inline void renderDocumentTables(const std::vector<std::pair<std::size_t, Table>>& tables,
                                 const LayoutComponent::Rect& textArea,
                                 int baseLineHeight, int scrollOffset,
                                 std::size_t editingLine = std::numeric_limits<std::size_t>::max(),
                                 CellPosition currentCell = {0, 0}) {
    for (const auto& [lineNum, table] : tables) {
        // Calculate Y position based on line number
        if (lineNum < static_cast<std::size_t>(scrollOffset)) continue;
        
        int y = static_cast<int>(textArea.y) + theme::layout::TEXT_PADDING +
                static_cast<int>(lineNum - scrollOffset) * baseLineHeight;
        int x = static_cast<int>(textArea.x) + theme::layout::TEXT_PADDING;
        
        // Check if table is visible
        if (y > static_cast<int>(textArea.y + textArea.height)) continue;
        
        bool isEditing = (lineNum == editingLine);
        renderTable(table, static_cast<float>(x), static_cast<float>(y), currentCell, isEditing);
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
                             int linesPerColumn = 50) {
    // Cursor into the squiggle list (sorted by row). Advanced as rows pass.
    std::size_t squiggleIdx = 0;
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

    // Outline mode: iterate the heading rows only (Title /
    // Subtitle / Heading 1-6); scrollOffset counts visible heading
    // rows, not buffer rows, so body paragraphs are never drawn.
    std::vector<std::size_t> outlineRows;
    if (outlineMode) {
        for (std::size_t i = 0; i < lineCount; ++i) {
            if (outline_view::isHeadingStyle(buffer.lineParagraphStyle(i))) {
                outlineRows.push_back(i);
            }
        }
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
        int baseX = static_cast<int>(textArea.x) +
                    theme::layout::TEXT_PADDING + gutterOffset +
                    (columnsActive
                         ? static_cast<int>(columns_math::columnXOffset(
                               column, static_cast<float>(columnWidthPx),
                               columnSpacingPx))
                         : 0);
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
        int indentedBaseX = baseX + totalIndent + listIndent + outlineIndent;
        int indentedWidth =
            std::max(0, availableWidth - totalIndent - listIndent -
                            rightIndent - outlineIndent);
        
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

        // Draw text with paragraph style applied
        if (!displayLine.empty()) {
            // Get global text style for underline/strikethrough/colors
            TextStyle globalStyle = buffer.textStyle();
            
            // Convert TextColor to backend-agnostic Color
            afterhours::Color textColor = {globalStyle.textColor.r, globalStyle.textColor.g,
                                       globalStyle.textColor.b, globalStyle.textColor.a};
            
            // Draw highlight background if set
            if (!globalStyle.highlightColor.isNone()) {
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

            // Drop cap support: draw first character larger
            if (span.hasDropCap && !textToDraw.empty()) {
                std::string dropChar = textToDraw.substr(0, 1);
                int dropFontSize = lineFontSize * span.dropCapLines;
                afterhours::draw_text(dropChar.c_str(), x, y - lineFontSize / 2,
                                 dropFontSize, textColor);
                int dropWidth = afterhours::graphics::measure_text(dropChar.c_str(), dropFontSize);
                textToDraw = textToDraw.substr(1);
                if (!textToDraw.empty()) {
                    x += dropWidth + 4;
                }
            }

            // For headings and titles, draw bold text (simulated by drawing twice with offset)
            if (paragraphStyleIsBold(paraStyle) || globalStyle.bold) {
                // Draw bold effect by drawing text twice with 1px offset
                afterhours::draw_text(textToDraw.c_str(), x, y + textYOffset, textFontSize, textColor);
                afterhours::draw_text(textToDraw.c_str(), x + 1, y + textYOffset, textFontSize, textColor);
            } else if (paragraphStyleIsItalic(paraStyle) || globalStyle.italic) {
                // For subtitle italic style, draw in a slightly different shade
                afterhours::Color italicColor = {static_cast<unsigned char>(textColor.r / 2 + 64),
                                             static_cast<unsigned char>(textColor.g / 2 + 64),
                                             static_cast<unsigned char>(textColor.b / 2 + 64), textColor.a};
                afterhours::draw_text(textToDraw.c_str(), x, y + textYOffset, textFontSize, italicColor);
            } else {
                afterhours::draw_text(textToDraw.c_str(), x, y + textYOffset, textFontSize, textColor);
            }
            
            // Draw underline if enabled
            if (globalStyle.underline) {
                int underlineY = y + lineFontSize + 1;
                afterhours::draw_line(x, underlineY, x + textWidth, underlineY, textColor);
            }
            
            // Draw strikethrough if enabled
            if (globalStyle.strikethrough) {
                int strikeY = y + lineFontSize / 2;
                afterhours::draw_line(x, strikeY, x + textWidth, strikeY, textColor);
            }

            // Win95 inverse selection: the whole-line draw above left the
            // selected text black-on-navy. Re-draw the selected segment in
            // white (SELECTION_TEXT) over the navy background, matching
            // bold / super / subscript / underline / strikethrough.
            // Drop-cap lines skip this: their x/text are shifted, so the
            // segment math would misalign (bg rect still shows).
            if (hasSelection && !span.hasDropCap) {
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
                float x1 = static_cast<float>(x) +
                           static_cast<float>(afterhours::graphics::measure_text(
                               displayLine.substr(0, dispStart).c_str(), lineFontSize));
                float x2 = static_cast<float>(x) +
                           static_cast<float>(afterhours::graphics::measure_text(
                               displayLine.substr(0, dispEnd).c_str(), lineFontSize));
                if (x2 > x1) drawSpellSquiggle(x1, x2, waveY);
                ++squiggleIdx;
            }
        }

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

        // Advance y by line height plus paragraph spacing after
        int paragraphSpaceAfter = buffer.lineSpaceAfter(row);
        y += lineHeight + paragraphSpaceAfter;

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
            afterhours::draw_rectangle(
                Rectangle{g.whiteLeft, rulerY, g.whiteRight - g.whiteLeft,
                          rulerH},
                theme::RULER_BG);
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
                if (q % 4 == 0 && q > 0 && x >= g.whiteLeft &&
                    x <= g.whiteRight) {
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
        // Columns (Format > Columns...): paged editing view only,
        // never Print Preview (which uses its own line stride).
        bool columnsActive =
            layout.pageMode == PageMode::Paged && !layout.previewMode &&
            doc.docSettings.pageSettings.columnCount > 1;
        int columnsLinesPerPage = static_cast<int>(
            columns_math::linesPerColumnForPage(
                layout.pageHeight, layout.pageMargin, ds, lineHeight));
        float columnsSpacingPx =
            columnsActive
                ? doc.docSettings.pageSettings.columnSpacing * ds
                : 0.0f;

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
                             columnsSpacingPx, columnsLinesPerPage);
            renderTextBuffer(doc.buffer, bottomArea, caretVisible, fontSize,
                             lineHeight, scroll.secondaryOffset, showLineNumbers,
                             layout.lineNumberGutterWidth, doc.docSettings.tabWidth,
                             ds, squiggleCache,
                             layout::displayScale(layout),
                             layout.previewMode, showParagraphMarks, outlineMode,
                             columnsActive ? doc.docSettings.pageSettings.columnCount : 1,
                             columnsSpacingPx, columnsLinesPerPage);

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
                             columnsSpacingPx, columnsLinesPerPage);
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

        // Draw comment markers in the right margin (skipped in Print
        // Preview: marker Y math uses scroll.offset, not the preview page).
        if (!layout.previewMode && !doc.comments.empty()) {
            for (const auto& comment : doc.comments) {
                CaretPosition pos = doc.buffer.positionForOffset(comment.startOffset);
                if (pos.row < static_cast<std::size_t>(scroll.offset)) continue;
                int markerY = static_cast<int>(effectiveArea.y) + theme::layout::TEXT_PADDING +
                              static_cast<int>(pos.row - scroll.offset) * lineHeight;
                int markerX = static_cast<int>(effectiveArea.x + effectiveArea.width) - 8;
                if (markerY > static_cast<int>(effectiveArea.y + effectiveArea.height)) continue;
                afterhours::draw_rectangle(
                    Rectangle{static_cast<float>(markerX), static_cast<float>(markerY), 6.0f, 6.0f},
                    afterhours::Color{255, 200, 0, 255});
            }
        }


        // Note: About, Word Count, and Help dialogs now rendered by MenuUISystem using afterhours modal.h
    }
};


}  // namespace ecs
