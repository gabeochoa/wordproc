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
#include "../util/clipboard.h"
#include "../editor/document_io.h"
#include "../editor/export/export_html.h"
#include "../editor/export/export_pdf.h"
#include "../editor/export/export_rtf.h"
#include "../editor/drawing.h"
#include "../editor/equation.h"
#include "../editor/image.h"
#include "../editor/table.h"
#include "../input/action_map.h"
#include "../rl.h"
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

// Draw a page background with shadow (for paged mode)
inline void drawPageBackground(const LayoutComponent& layout) {
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

    // Draw margin guidelines (dotted or light lines)
    float marginScaled = layout.pageMargin * layout.pageScale;
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
            
            // Draw cell border
            afterhours::Color borderColor = afterhours::Color{0, 0, 0, 255};
            switch (cell.borders.top) {
                case BorderStyle::Thin:
                    afterhours::draw_line(static_cast<int>(cellX), static_cast<int>(cellY),
                                    static_cast<int>(cellX + cellW), static_cast<int>(cellY), borderColor);
                    break;
                case BorderStyle::Medium:
                    afterhours::draw_line_ex({cellX, cellY}, {cellX + cellW, cellY}, 2.0f, borderColor);
                    break;
                case BorderStyle::Thick:
                    afterhours::draw_line_ex({cellX, cellY}, {cellX + cellW, cellY}, 3.0f, borderColor);
                    break;
                case BorderStyle::None:
                case BorderStyle::Double:
                case BorderStyle::Dashed:
                case BorderStyle::Dotted:
                default:
                    break;
            }
            switch (cell.borders.bottom) {
                case BorderStyle::Thin:
                    afterhours::draw_line(static_cast<int>(cellX), static_cast<int>(cellY + cellH),
                                    static_cast<int>(cellX + cellW), static_cast<int>(cellY + cellH), borderColor);
                    break;
                case BorderStyle::Medium:
                    afterhours::draw_line_ex({cellX, cellY + cellH}, {cellX + cellW, cellY + cellH}, 2.0f, borderColor);
                    break;
                case BorderStyle::Thick:
                    afterhours::draw_line_ex({cellX, cellY + cellH}, {cellX + cellW, cellY + cellH}, 3.0f, borderColor);
                    break;
                case BorderStyle::None:
                case BorderStyle::Double:
                case BorderStyle::Dashed:
                case BorderStyle::Dotted:
                default:
                    break;
            }
            switch (cell.borders.left) {
                case BorderStyle::Thin:
                    afterhours::draw_line(static_cast<int>(cellX), static_cast<int>(cellY),
                                    static_cast<int>(cellX), static_cast<int>(cellY + cellH), borderColor);
                    break;
                case BorderStyle::Medium:
                    afterhours::draw_line_ex({cellX, cellY}, {cellX, cellY + cellH}, 2.0f, borderColor);
                    break;
                case BorderStyle::Thick:
                    afterhours::draw_line_ex({cellX, cellY}, {cellX, cellY + cellH}, 3.0f, borderColor);
                    break;
                case BorderStyle::None:
                case BorderStyle::Double:
                case BorderStyle::Dashed:
                case BorderStyle::Dotted:
                default:
                    break;
            }
            switch (cell.borders.right) {
                case BorderStyle::Thin:
                    afterhours::draw_line(static_cast<int>(cellX + cellW), static_cast<int>(cellY),
                                    static_cast<int>(cellX + cellW), static_cast<int>(cellY + cellH), borderColor);
                    break;
                case BorderStyle::Medium:
                    afterhours::draw_line_ex({cellX + cellW, cellY}, {cellX + cellW, cellY + cellH}, 2.0f, borderColor);
                    break;
                case BorderStyle::Thick:
                    afterhours::draw_line_ex({cellX + cellW, cellY}, {cellX + cellW, cellY + cellH}, 3.0f, borderColor);
                    break;
                case BorderStyle::None:
                case BorderStyle::Double:
                case BorderStyle::Dashed:
                case BorderStyle::Dotted:
                default:
                    break;
            }
            
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
inline void renderTextBuffer(const TextBuffer& buffer,
                             const LayoutComponent::Rect& textArea,
                             bool caretVisible, int baseFontSize, int baseLineHeight,
                             int scrollOffset, bool showLineNumbers = false,
                             float lineNumberGutterWidth = 50.0f,
                             int tabWidth = 4, float zoomLevel = 1.0f) {
    std::size_t lineCount = buffer.lineCount();
    CaretPosition caret = buffer.caret();
    bool hasSelection = buffer.hasSelection();
    CaretPosition selStart = buffer.selectionStart();
    CaretPosition selEnd = buffer.selectionEnd();
    
    // Calculate gutter offset for text
    int gutterOffset = showLineNumbers ? static_cast<int>(lineNumberGutterWidth) : 0;

    int y = static_cast<int>(textArea.y) + theme::layout::TEXT_PADDING;

    std::size_t startRow = static_cast<std::size_t>(scrollOffset);
    if (startRow >= lineCount) startRow = lineCount > 0 ? lineCount - 1 : 0;

    for (std::size_t row = startRow; row < lineCount; ++row) {
        LineSpan span = buffer.lineSpan(row);
        int baseX = static_cast<int>(textArea.x) + theme::layout::TEXT_PADDING + gutterOffset;
        int availableWidth = static_cast<int>(textArea.width) - 2 * theme::layout::TEXT_PADDING;

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
        
        // Get paragraph style for this line
        ParagraphStyle paraStyle = buffer.lineParagraphStyle(row);
        int lineFontSize = static_cast<int>(paragraphStyleFontSize(paraStyle) * zoomLevel);
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
        
        // Draw page break indicator if present
        if (buffer.hasPageBreakBefore(row)) {
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
        
        // Draw line number in gutter if enabled
        if (showLineNumbers) {
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
        
        // Apply indentation
        int leftIndent = buffer.lineLeftIndent(row);
        int firstLineIndent = buffer.lineFirstLineIndent(row);
        // Note: firstLineIndent only applies to first line of paragraph
        // For now, we treat each line as its own paragraph
        int totalIndent = leftIndent + firstLineIndent;
        
        // Get list properties for this line
        ListType listType = buffer.lineListType(row);
        int listLevel = buffer.lineListLevel(row);
        int listNumber = buffer.lineListNumber(row);
        
        // Calculate list marker indent (each level adds 20px)
        int listIndent = (listType != ListType::None) ? (listLevel + 1) * 20 : 0;
        
        int indentedBaseX = baseX + totalIndent + listIndent;
        int indentedWidth = availableWidth - totalIndent - listIndent;
        
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
        
        // Draw selection highlight (with alignment offset)
        if (hasSelection) {
            bool lineInSelection = (row >= selStart.row && row <= selEnd.row);
            if (lineInSelection) {
                std::size_t startCol =
                    (row == selStart.row) ? selStart.column : 0;
                std::size_t endCol =
                    (row == selEnd.row) ? selEnd.column : span.length;

                if (startCol < endCol && !displayLine.empty()) {
                    std::string beforeSel = displayLine.substr(0, startCol);
                    std::string selectedText =
                        displayLine.substr(startCol, endCol - startCol);

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
            // Register document text for E2E tests
            test_input::register_visible_text(displayLine);
            
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
            break;
        }
    }
}

// Forward declarations - implemented after MenuSystem
void handleMenuActionImpl(int menuResult, DocumentComponent& doc,
                          MenuComponent& menu,
                          LayoutComponent& layout);

// System for rendering the complete editor UI
struct EditorRenderSystem
    : public afterhours::System<DocumentComponent, CaretComponent,
                                ScrollComponent,
                                LayoutComponent, MenuComponent> {
    void once(const float) const override {
        afterhours::graphics::begin_drawing();
        afterhours::graphics::clear_background(theme::WINDOW_BG);
        // Note: Visible text registry is cleared in main.cpp at start of frame
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
                    afterhours::graphics::take_screenshot(pathStr.c_str());
                    // Verify screenshot was taken
                    if (std::filesystem::exists(pathStr)) {
                        LOG_INFO("Screenshot saved successfully");
                    } else {
                        LOG_WARNING("Screenshot file not found after TakeScreenshot");
                    }
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
        afterhours::graphics::end_drawing();
    }

    void for_each_with(afterhours::Entity& /*entity*/,
                       DocumentComponent& doc,
                       CaretComponent& caret,
                       ScrollComponent& scroll,
                       LayoutComponent& layout, MenuComponent& menu,
                       const float) override {
        // F1 to show help window
        if (input::isKeyPressed(afterhours::keys::F1)) {
            menu.showHelpWindow = !menu.showHelpWindow;
            menu.helpScrollOffset = 0;
        }

        // Draw horizontal ruler (after toolbars, before text area)
        if (!layout.focusMode) {
            float rulerY = theme::layout::scale(theme::layout::TITLE_BAR_HEIGHT + 
                                               theme::layout::MENU_BAR_HEIGHT + 
                                               theme::layout::TOOLBAR_HEIGHT + 
                                               theme::layout::FORMATTING_BAR_HEIGHT);
            Rectangle rulerRect = {
                0, rulerY,
                static_cast<float>(layout.screenWidth),
                theme::layout::scale(theme::layout::RULER_HEIGHT)
            };
            afterhours::draw_rectangle(rulerRect, theme::RULER_BG);
            util::drawSunkenBorder(rulerRect);
            
            // Draw ruler marks (inches/centimeters)
            // For simplicity, use 1 inch = 72 pixels (standard DPI)
            int rulerStartX = theme::layout::scaleInt(50);  // Offset for left margin
            int pixelsPerInch = theme::layout::scaleInt(72);
            int maxInches = (layout.screenWidth - rulerStartX) / pixelsPerInch;
            
            for (int inch = 0; inch <= maxInches; ++inch) {
                int x = rulerStartX + inch * pixelsPerInch;
                
                // Draw inch mark (tall line)
                afterhours::draw_line(x, static_cast<int>(rulerY) + theme::layout::scaleInt(12), 
                               x, static_cast<int>(rulerY) + theme::layout::scaleInt(18), 
                               theme::RULER_MARKS);
                
                // Draw inch number
                std::string inchStr = std::to_string(inch);
                int textWidth = theme::MeasureUIText(inchStr.c_str(), 10);
                theme::DrawUIText(inchStr.c_str(), x - textWidth / 2, 
                                static_cast<int>(rulerY) + theme::layout::scaleInt(1), 
                                10, theme::RULER_TEXT);
                
                // Draw half-inch marks
                if (inch < maxInches) {
                    int halfX = x + pixelsPerInch / 2;
                    afterhours::draw_line(halfX, static_cast<int>(rulerY) + theme::layout::scaleInt(14), 
                                   halfX, static_cast<int>(rulerY) + theme::layout::scaleInt(18), 
                                   theme::RULER_MARKS);
                    
                    // Draw quarter-inch marks
                    int quarterX1 = x + pixelsPerInch / 4;
                    int quarterX2 = x + 3 * pixelsPerInch / 4;
                    afterhours::draw_line(quarterX1, static_cast<int>(rulerY) + theme::layout::scaleInt(16), 
                                   quarterX1, static_cast<int>(rulerY) + theme::layout::scaleInt(18), 
                                   theme::RULER_MARKS);
                    afterhours::draw_line(quarterX2, static_cast<int>(rulerY) + theme::layout::scaleInt(16), 
                                   quarterX2, static_cast<int>(rulerY) + theme::layout::scaleInt(18), 
                                   theme::RULER_MARKS);
                }
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

            // Draw the page with shadow and margins
            drawPageBackground(layout);
        } else {
            afterhours::draw_rectangle(textAreaRect, theme::TEXT_AREA_BG);
            util::drawSunkenBorder(textAreaRect);
        }

        // Render text buffer using effective text area (respects page margins)
        TextStyle style = doc.buffer.textStyle();
        int fontSize = std::max(8, static_cast<int>(std::round(style.fontSize * layout.zoomLevel)));
        int lineHeight = fontSize + 4;
        LayoutComponent::Rect effectiveArea = layout::effectiveTextArea(layout);

        if (layout.splitViewEnabled) {
            float splitHeight = effectiveArea.height * 0.5f;
            LayoutComponent::Rect topArea = {effectiveArea.x, effectiveArea.y,
                                             effectiveArea.width, splitHeight - 4.0f};
            LayoutComponent::Rect bottomArea = {effectiveArea.x, effectiveArea.y + splitHeight + 4.0f,
                                                effectiveArea.width, splitHeight - 4.0f};
            renderTextBuffer(doc.buffer, topArea, caret.visible, fontSize,
                             lineHeight, scroll.offset, layout.showLineNumbers,
                             layout.lineNumberGutterWidth, doc.docSettings.tabWidth,
                             layout.zoomLevel);
            renderTextBuffer(doc.buffer, bottomArea, caret.visible, fontSize,
                             lineHeight, scroll.secondaryOffset, layout.showLineNumbers,
                             layout.lineNumberGutterWidth, doc.docSettings.tabWidth,
                             layout.zoomLevel);

            // Split divider
            afterhours::draw_line(static_cast<int>(effectiveArea.x),
                             static_cast<int>(effectiveArea.y + splitHeight),
                             static_cast<int>(effectiveArea.x + effectiveArea.width),
                             static_cast<int>(effectiveArea.y + splitHeight),
                             theme::BORDER_DARK);
        } else {
            renderTextBuffer(doc.buffer, effectiveArea, caret.visible, fontSize,
                             lineHeight, scroll.offset, layout.showLineNumbers,
                             layout.lineNumberGutterWidth, doc.docSettings.tabWidth,
                             layout.zoomLevel);
        }

        // === Draw Vertical Scroll Bar ===
        {
            float sbWidth = 16.0f;
            float sbX = textAreaRect.x + textAreaRect.width - sbWidth;
            float sbY = textAreaRect.y;
            float sbHeight = textAreaRect.height;
            int totalLines = static_cast<int>(doc.buffer.lineCount());
            int visibleLines = scroll.visibleLines > 0 ? scroll.visibleLines : 20;

            // Scroll bar track (sunken background)
            afterhours::draw_rectangle(Rectangle{sbX, sbY, sbWidth, sbHeight},
                                       theme::BUTTON_FACE);
            // Sunken border on track
            afterhours::draw_line(static_cast<int>(sbX), static_cast<int>(sbY),
                             static_cast<int>(sbX), static_cast<int>(sbY + sbHeight),
                             theme::BORDER_DARK);
            afterhours::draw_line(static_cast<int>(sbX), static_cast<int>(sbY),
                             static_cast<int>(sbX + sbWidth), static_cast<int>(sbY),
                             theme::BORDER_DARK);

            // Thumb
            if (totalLines > visibleLines) {
                float thumbRatio = static_cast<float>(visibleLines) / static_cast<float>(totalLines);
                float thumbHeight = std::max(20.0f, sbHeight * thumbRatio);
                float scrollRange = sbHeight - thumbHeight;
                float scrollRatio = (scroll.maxScroll > 0)
                    ? static_cast<float>(scroll.offset) / static_cast<float>(scroll.maxScroll)
                    : 0.0f;
                float thumbY = sbY + scrollRange * scrollRatio;

                // Raised thumb
                afterhours::draw_rectangle(
                    Rectangle{sbX + 1, thumbY, sbWidth - 2, thumbHeight},
                    theme::BUTTON_FACE);
                // 3D border on thumb (raised)
                afterhours::draw_line(static_cast<int>(sbX + 1), static_cast<int>(thumbY),
                                 static_cast<int>(sbX + sbWidth - 2), static_cast<int>(thumbY),
                                 theme::BORDER_LIGHT);
                afterhours::draw_line(static_cast<int>(sbX + 1), static_cast<int>(thumbY),
                                 static_cast<int>(sbX + 1), static_cast<int>(thumbY + thumbHeight),
                                 theme::BORDER_LIGHT);
                afterhours::draw_line(static_cast<int>(sbX + sbWidth - 2), static_cast<int>(thumbY),
                                 static_cast<int>(sbX + sbWidth - 2), static_cast<int>(thumbY + thumbHeight),
                                 theme::BORDER_DARK);
                afterhours::draw_line(static_cast<int>(sbX + 1), static_cast<int>(thumbY + thumbHeight - 1),
                                 static_cast<int>(sbX + sbWidth - 2), static_cast<int>(thumbY + thumbHeight - 1),
                                 theme::BORDER_DARK);
            }
        }

        // Draw comment markers in the right margin
        if (!doc.comments.empty()) {
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

// System for rendering menus and handling interactions
struct MenuSystem
    : public afterhours::System<DocumentComponent, MenuComponent,
                                LayoutComponent> {

    void for_each_with(afterhours::Entity& /*entity*/, DocumentComponent& doc,
                       MenuComponent& menu,
                       LayoutComponent& layout, const float) override {
        renderMenus(doc, menu, layout);
    }

    void renderMenus(DocumentComponent& doc, MenuComponent& menu,
                     LayoutComponent& layout) const {
        // Consume any click results from MenuUISystem and handle actions
        int menuResult = menu.consumeClickedResult();

        if (menuResult >= 0) {
            handleMenuAction(menuResult, doc, menu, layout);
        }

        // Note: About, Word Count, Comment, Template, and Tab Width dialogs
        // are now rendered by MenuUISystem using afterhours modal.h

        // F1 to show help window
        if (input::isKeyPressed(afterhours::keys::F1)) {
            menu.showHelpWindow = !menu.showHelpWindow;
        }
    }

   private:
    void handleMenuAction(int menuResult, DocumentComponent& doc,
                          MenuComponent& menu,
                          LayoutComponent& layout) const {
        handleMenuActionImpl(menuResult, doc, menu, layout);
    }
};

// Implementation of menu action handler (called by both EditorRenderSystem and MenuSystem)
inline void handleMenuActionImpl(int menuResult, DocumentComponent& doc,
                          MenuComponent& menu,
                          LayoutComponent& layout) {
        int menuIndex = menuResult / 100;
        int itemIndex = menuResult % 100;

        if (menuIndex == 0) {  // File menu
            if (itemIndex >= 0 &&
                itemIndex < static_cast<int>(menu.menus[0].items.size())) {
                const std::string& label = menu.menus[0].items[itemIndex].label;
                if (label.rfind("Recent: ", 0) == 0) {
                    std::string path = label.substr(std::string("Recent: ").size());
                    auto result = loadDocumentEx(doc.buffer, doc.docSettings, path);
                    if (result.success) {
                        doc.filePath = path;
                        doc.isDirty = false;
                        doc.comments.clear();
                        doc.revisions.clear();
                        layout.pageMode = doc.docSettings.pageSettings.mode;
                        layout.pageWidth = doc.docSettings.pageSettings.pageWidth;
                        layout.pageHeight = doc.docSettings.pageSettings.pageHeight;
                        layout.pageMargin = doc.docSettings.pageSettings.pageMargin;
                        layout.lineWidthLimit =
                            doc.docSettings.pageSettings.lineWidthLimit;
                        Settings::get().add_recent_file(path);
                        menu.menus = menu_setup::createMenuBar(
                            Settings::get().get_recent_files());
                        menu.recentFilesCount = static_cast<int>(
                            Settings::get().get_recent_files().size());
                        if (doc.trackChangesEnabled &&
                            menu.menus.size() > 1 &&
                            menu.menus[1].items.size() > 3) {
                            menu.menus[1].items[3].mark =
                                win95::MenuMark::Checkmark;
                        }
                        toast_notify::success(
                            "Opened: " + std::filesystem::path(path).filename().string());
                    } else {
                        toast_notify::error("Open failed: " + result.error);
                    }
                    return;
                }
                if (label == "Exit") {
                    return;
                }
            }
            switch (itemIndex) {
                case 0:  // New
                    doc.buffer.setText("");
                    doc.filePath.clear();
                    doc.isDirty = false;
                    doc.comments.clear();
                    doc.revisions.clear();
                    doc.trackChangesBaseline.clear();
                    break;
                case 1:  // New from Template...
                    menu.showTemplateDialog = true;
                    break;
                case 2:  // Open
                {
                    // Load document with settings (document settings saved with
                    // file)
                    auto result = loadDocumentEx(doc.buffer, doc.docSettings,
                                                 doc.defaultPath);
                    if (result.success) {
                        doc.filePath = doc.defaultPath;
                        doc.isDirty = false;
                        doc.comments.clear();
                        doc.revisions.clear();
                        // Sync loaded document settings to layout component
                        layout.pageMode = doc.docSettings.pageSettings.mode;
                        layout.pageWidth =
                            doc.docSettings.pageSettings.pageWidth;
                        layout.pageHeight =
                            doc.docSettings.pageSettings.pageHeight;
                        layout.pageMargin =
                            doc.docSettings.pageSettings.pageMargin;
                        layout.lineWidthLimit =
                            doc.docSettings.pageSettings.lineWidthLimit;
                        Settings::get().add_recent_file(doc.defaultPath);
                        menu.menus = menu_setup::createMenuBar(
                            Settings::get().get_recent_files());
                        menu.recentFilesCount = static_cast<int>(
                            Settings::get().get_recent_files().size());
                        if (doc.trackChangesEnabled &&
                            menu.menus.size() > 1 &&
                            menu.menus[1].items.size() > 3) {
                            menu.menus[1].items[3].mark =
                                win95::MenuMark::Checkmark;
                        }
                        toast_notify::success(
                            "Opened: " + std::filesystem::path(doc.defaultPath)
                                             .filename()
                                             .string());
                    } else {
                        toast_notify::error("Open failed: " + result.error);
                    }
                } break;
                case 3:  // Save
                {
                    std::string savePath =
                        doc.filePath.empty() ? doc.defaultPath : doc.filePath;
                    // Sync layout settings to document settings before save
                    doc.docSettings.textStyle = doc.buffer.textStyle();
                    doc.docSettings.pageSettings.mode = layout.pageMode;
                    doc.docSettings.pageSettings.pageWidth = layout.pageWidth;
                    doc.docSettings.pageSettings.pageHeight = layout.pageHeight;
                    doc.docSettings.pageSettings.pageMargin = layout.pageMargin;
                    doc.docSettings.pageSettings.lineWidthLimit =
                        layout.lineWidthLimit;
                    // Save document with all settings
                    auto result =
                        saveDocumentEx(doc.buffer, doc.docSettings, savePath);
                    if (result.success) {
                        doc.isDirty = false;
                        doc.filePath = savePath;
                        if (!doc.autoSavePath.empty()) {
                            std::filesystem::remove(doc.autoSavePath);
                        }
                        Settings::get().add_recent_file(savePath);
                        menu.menus = menu_setup::createMenuBar(
                            Settings::get().get_recent_files());
                        menu.recentFilesCount = static_cast<int>(
                            Settings::get().get_recent_files().size());
                        if (doc.trackChangesEnabled &&
                            menu.menus.size() > 1 &&
                            menu.menus[1].items.size() > 3) {
                            menu.menus[1].items[3].mark =
                                win95::MenuMark::Checkmark;
                        }
                        toast_notify::success(
                            "Saved: " + std::filesystem::path(savePath)
                                                    .filename()
                                                    .string());
                    } else {
                        toast_notify::error("Save failed: " + result.error);
                    }
                } break;
                case 4:  // Save As...
                {
                    // Pre-fill with current filename or default
                    std::string suggested = doc.filePath.empty() ? "untitled.wdoc" : doc.filePath;
                    menu.saveAsInputStr = suggested;
                    menu.showSaveAsDialog = true;
                } break;
                case 6:  // Export PDF
                {
                    std::filesystem::path basePath =
                        doc.filePath.empty() ? doc.defaultPath : doc.filePath;
                    basePath.replace_extension(".pdf");
                    auto result = exportDocumentPdf(doc.buffer, doc.docSettings,
                                                    basePath.string());
                    if (result.success) {
                        toast_notify::success(
                            "Exported PDF: " + basePath.filename().string());
                    } else {
                        toast_notify::error("Export PDF failed: " + result.error);
                    }
                } break;
                case 7:  // Export HTML
                {
                    std::filesystem::path basePath =
                        doc.filePath.empty() ? doc.defaultPath : doc.filePath;
                    basePath.replace_extension(".html");
                    auto result = exportDocumentHtml(doc.buffer, doc.docSettings,
                                                     basePath.string());
                    if (result.success) {
                        toast_notify::success(
                            "Exported HTML: " + basePath.filename().string());
                    } else {
                        toast_notify::error("Export HTML failed: " + result.error);
                    }
                } break;
                case 8:  // Export RTF
                {
                    std::filesystem::path basePath =
                        doc.filePath.empty() ? doc.defaultPath : doc.filePath;
                    basePath.replace_extension(".rtf");
                    auto result = exportDocumentRtf(doc.buffer, doc.docSettings,
                                                    basePath.string());
                    if (result.success) {
                        toast_notify::success(
                            "Exported RTF: " + basePath.filename().string());
                    } else {
                        toast_notify::error("Export RTF failed: " + result.error);
                    }
                } break;
                case 10:  // Page Setup
                {
                    // Toggle the page setup dialog (for now, just toggle to Paged mode with presets)
                    menu.showPageSetup = !menu.showPageSetup;
                } break;
                default:
                    break;
            }
        } else if (menuIndex == 1) {  // Edit menu
            switch (itemIndex) {
                case 0:  // Undo
                    if (doc.buffer.canUndo()) {
                        doc.buffer.undo();
                        doc.isDirty = true;
                    }
                    break;
                case 1:  // Redo
                    if (doc.buffer.canRedo()) {
                        doc.buffer.redo();
                        doc.isDirty = true;
                    }
                    break;
                case 3:  // Track Changes
                    doc.trackChangesEnabled = !doc.trackChangesEnabled;
                    if (doc.trackChangesEnabled) {
                        doc.trackChangesBaseline = doc.buffer.getText();
                        menu.menus[1].items[3].mark = win95::MenuMark::Checkmark;
                        toast_notify::info("Track Changes: On");
                    } else {
                        menu.menus[1].items[3].mark = win95::MenuMark::None;
                        toast_notify::info("Track Changes: Off");
                    }
                    break;
                case 4:  // Accept All Changes
                    doc.revisions.clear();
                    doc.trackChangesBaseline = doc.buffer.getText();
                    toast_notify::success("All changes accepted");
                    break;
                case 5:  // Reject All Changes
                    if (!doc.trackChangesBaseline.empty()) {
                        doc.buffer.setText(doc.trackChangesBaseline);
                        doc.isDirty = true;
                    }
                    doc.revisions.clear();
                    toast_notify::success("All changes rejected");
                    break;
                case 7:  // Cut
                    if (doc.buffer.hasSelection()) {
                        std::string selected = doc.buffer.getSelectedText();
                        if (!selected.empty()) {
                            app::clipboard::set_text(selected);
                            doc.buffer.deleteSelection();
                            doc.isDirty = true;
                        }
                    }
                    break;
                case 8:  // Copy
                    if (doc.buffer.hasSelection()) {
                        std::string selected = doc.buffer.getSelectedText();
                        if (!selected.empty()) {
                            app::clipboard::set_text(selected);
                        }
                    }
                    break;
                case 9:  // Paste
                {
                    if (app::clipboard::has_text()) {
                        std::string clipText = app::clipboard::get_text();
                        doc.buffer.insertText(clipText);
                        doc.isDirty = true;
                    }
                } break;
                case 11:  // Select All
                    doc.buffer.selectAll();
                    break;
                case 13:  // Find...
                    menu.showFindDialog = true;
                    menu.findReplaceMode = false;
                    toast_notify::info("Find: Ctrl+G next, Ctrl+Shift+G prev");
                    break;
                case 14:  // Find Next
                    if (!menu.lastSearchTerm.empty()) {
                        FindResult result = doc.buffer.findNext(menu.lastSearchTerm, menu.findOptions);
                        if (result.found) {
                            doc.buffer.setCaret(result.start);
                            doc.buffer.setSelectionAnchor(result.start);
                            doc.buffer.setCaret(result.end);
                            doc.buffer.updateSelectionToCaret();
                            toast_notify::info("Found", 2.0f);
                        } else {
                            toast_notify::warning("Not found");
                        }
                    }
                    break;
                case 15:  // Find Previous
                    if (!menu.lastSearchTerm.empty()) {
                        FindResult result = doc.buffer.findPrevious(menu.lastSearchTerm, menu.findOptions);
                        if (result.found) {
                            doc.buffer.setCaret(result.start);
                            doc.buffer.setSelectionAnchor(result.start);
                            doc.buffer.setCaret(result.end);
                            doc.buffer.updateSelectionToCaret();
                            toast_notify::info("Found", 2.0f);
                        } else {
                            toast_notify::warning("Not found");
                        }
                    }
                    break;
                case 16:  // Replace...
                    menu.showFindDialog = true;
                    menu.findReplaceMode = true;
                    toast_notify::info("Replace mode");
                    break;
                case 18:  // Go To Bookmark...
                    menu.showBookmarkListDialog = true;
                    break;
                default:
                    break;
            }
        } else if (menuIndex == 2) {  // View menu
            switch (itemIndex) {
                case 0:  // Pageless Mode
                    layout.pageMode = PageMode::Pageless;
                    layout::updateLayout(layout, layout.screenWidth,
                                         layout.screenHeight);
                    toast_notify::info("Switched to Pageless mode");
                    break;
                case 1:  // Paged Mode
                    layout.pageMode = PageMode::Paged;
                    layout::updateLayout(layout, layout.screenWidth,
                                         layout.screenHeight);
                    toast_notify::info("Switched to Paged mode");
                    break;
                case 3:  // Zoom In
                    layout.zoomLevel = std::min(4.0f, layout.zoomLevel + 0.1f);
                    toast_notify::info("Zoom in");
                    break;
                case 4:  // Zoom Out
                    layout.zoomLevel = std::max(0.5f, layout.zoomLevel - 0.1f);
                    toast_notify::info("Zoom out");
                    break;
                case 5:  // Zoom Reset
                    layout.zoomLevel = 1.0f;
                    toast_notify::info("Zoom reset");
                    break;
                case 7:  // Focus Mode
                    layout.focusMode = !layout.focusMode;
                    layout::updateLayout(layout, layout.screenWidth,
                                         layout.screenHeight);
                    toast_notify::info(layout.focusMode ? "Focus mode: On" : "Focus mode: Off");
                    break;
                case 8:  // Split View
                    layout.splitViewEnabled = !layout.splitViewEnabled;
                    toast_notify::info(layout.splitViewEnabled ? "Split view: On" : "Split view: Off");
                    break;
                case 9:  // Dark Mode
                    theme::applyDarkMode(!theme::DARK_MODE_ENABLED);
                    toast_notify::info(theme::DARK_MODE_ENABLED ? "Dark mode: On" : "Dark mode: Off");
                    break;
                case 11:  // Line Width: Normal (no limit)
                    layout::setLineWidthLimit(layout, 0.0f);
                    toast_notify::info("Line width: Normal");
                    break;
                case 12:  // Line Width: Narrow (60 chars)
                    layout::setLineWidthLimit(layout, 60.0f);
                    toast_notify::info("Line width: Narrow (60 chars)");
                    break;
                case 13:  // Line Width: Wide (100 chars)
                    layout::setLineWidthLimit(layout, 100.0f);
                    toast_notify::info("Line width: Wide (100 chars)");
                    break;
                case 15:  // Show Line Numbers
                    layout.showLineNumbers = !layout.showLineNumbers;
                    toast_notify::info(layout.showLineNumbers ? "Line numbers: On" : "Line numbers: Off");
                    break;
                case 16:  // Show Outline
                    layout.showOutline = !layout.showOutline;
                    toast_notify::info(layout.showOutline ? "Outline: On" : "Outline: Off");
                    break;
                default:
                    break;
            }
        } else if (menuIndex == 4) {  // Format menu
            TextStyle style = doc.buffer.textStyle();
            switch (itemIndex) {
                // Paragraph styles (0-8)
                case 0:  // Normal
                    doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Normal);
                    toast_notify::info("Style: Normal");
                    break;
                case 1:  // Title
                    doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Title);
                    toast_notify::info("Style: Title");
                    break;
                case 2:  // Subtitle
                    doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Subtitle);
                    toast_notify::info("Style: Subtitle");
                    break;
                case 3:  // Heading 1
                    doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Heading1);
                    toast_notify::info("Style: Heading 1");
                    break;
                case 4:  // Heading 2
                    doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Heading2);
                    toast_notify::info("Style: Heading 2");
                    break;
                case 5:  // Heading 3
                    doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Heading3);
                    toast_notify::info("Style: Heading 3");
                    break;
                case 6:  // Heading 4
                    doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Heading4);
                    toast_notify::info("Style: Heading 4");
                    break;
                case 7:  // Heading 5
                    doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Heading5);
                    toast_notify::info("Style: Heading 5");
                    break;
                case 8:  // Heading 6
                    doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Heading6);
                    toast_notify::info("Style: Heading 6");
                    break;
                // (9 is separator)
                case 10:  // Bold
                    style.bold = !style.bold;
                    doc.buffer.setTextStyle(style);
                    break;
                case 11:  // Italic
                    style.italic = !style.italic;
                    doc.buffer.setTextStyle(style);
                    break;
                case 12:  // Underline
                    style.underline = !style.underline;
                    doc.buffer.setTextStyle(style);
                    break;
                case 13:  // Strikethrough
                    style.strikethrough = !style.strikethrough;
                    doc.buffer.setTextStyle(style);
                    break;
                case 14:  // Superscript
                    style.superscript = !style.superscript;
                    if (style.superscript) style.subscript = false;
                    doc.buffer.setTextStyle(style);
                    break;
                case 15:  // Subscript
                    style.subscript = !style.subscript;
                    if (style.subscript) style.superscript = false;
                    doc.buffer.setTextStyle(style);
                    break;
                // (14 is separator)
                // Alignment (15-18)
                case 20:  // Align Left
                    doc.buffer.setCurrentAlignment(TextAlignment::Left);
                    toast_notify::info("Align: Left");
                    break;
                case 21:  // Align Center
                    doc.buffer.setCurrentAlignment(TextAlignment::Center);
                    toast_notify::info("Align: Center");
                    break;
                case 22:  // Align Right
                    doc.buffer.setCurrentAlignment(TextAlignment::Right);
                    toast_notify::info("Align: Right");
                    break;
                case 23:  // Justify
                    doc.buffer.setCurrentAlignment(TextAlignment::Justify);
                    toast_notify::info("Align: Justify");
                    break;
                // (19 is separator)
                // Text colors (20-26)
                case 25:  // Text: Black
                    style.textColor = TextColors::Black;
                    doc.buffer.setTextStyle(style);
                    break;
                case 26:  // Text: Red
                    style.textColor = TextColors::Red;
                    doc.buffer.setTextStyle(style);
                    break;
                case 27:  // Text: Orange
                    style.textColor = TextColors::Orange;
                    doc.buffer.setTextStyle(style);
                    break;
                case 28:  // Text: Green
                    style.textColor = TextColors::Green;
                    doc.buffer.setTextStyle(style);
                    break;
                case 29:  // Text: Blue
                    style.textColor = TextColors::Blue;
                    doc.buffer.setTextStyle(style);
                    break;
                case 30:  // Text: Purple
                    style.textColor = TextColors::Purple;
                    doc.buffer.setTextStyle(style);
                    break;
                case 31:  // Text: Gray
                    style.textColor = TextColors::Gray;
                    doc.buffer.setTextStyle(style);
                    break;
                // (27 is separator)
                // Highlight colors (28-33)
                case 33:  // Highlight: None
                    style.highlightColor = HighlightColors::None;
                    doc.buffer.setTextStyle(style);
                    break;
                case 34:  // Highlight: Yellow
                    style.highlightColor = HighlightColors::Yellow;
                    doc.buffer.setTextStyle(style);
                    break;
                case 35:  // Highlight: Green
                    style.highlightColor = HighlightColors::Green;
                    doc.buffer.setTextStyle(style);
                    break;
                case 36:  // Highlight: Cyan
                    style.highlightColor = HighlightColors::Cyan;
                    doc.buffer.setTextStyle(style);
                    break;
                case 37:  // Highlight: Pink
                    style.highlightColor = HighlightColors::Pink;
                    doc.buffer.setTextStyle(style);
                    break;
                case 38:  // Highlight: Orange
                    style.highlightColor = HighlightColors::Orange;
                    doc.buffer.setTextStyle(style);
                    break;
                // (34 is separator)
                // Fonts (35-36)
                case 40:  // Font: Gaegu
                    style.font = "Gaegu-Bold";
                    doc.buffer.setTextStyle(style);
                    break;
                case 41:  // Font: Garamond
                    style.font = "EBGaramond-Regular";
                    doc.buffer.setTextStyle(style);
                    break;
                // (37 is separator)
                // Font size (38-40)
                case 43:  // Increase Size
                    style.fontSize = std::min(72, style.fontSize + 2);
                    doc.buffer.setTextStyle(style);
                    break;
                case 44:  // Decrease Size
                    style.fontSize = std::max(8, style.fontSize - 2);
                    doc.buffer.setTextStyle(style);
                    break;
                case 45:  // Reset Size
                    style.fontSize = 16;
                    doc.buffer.setTextStyle(style);
                    break;
                // (41 is separator)
                // Alignment (42-45)
                case 47:  // Align Left
                    doc.buffer.setCurrentAlignment(TextAlignment::Left);
                    toast_notify::info("Align: Left");
                    break;
                case 48:  // Align Center
                    doc.buffer.setCurrentAlignment(TextAlignment::Center);
                    toast_notify::info("Align: Center");
                    break;
                case 49:  // Align Right
                    doc.buffer.setCurrentAlignment(TextAlignment::Right);
                    toast_notify::info("Align: Right");
                    break;
                case 50:  // Justify
                    doc.buffer.setCurrentAlignment(TextAlignment::Justify);
                    toast_notify::info("Align: Justify");
                    break;
                // (46 is separator)
                case 52:  // Increase Indent
                    doc.buffer.increaseIndent();
                    toast_notify::info("Indent increased");
                    break;
                case 53:  // Decrease Indent
                    doc.buffer.decreaseIndent();
                    toast_notify::info("Indent decreased");
                    break;
                // (54 is separator)
                case 55:  // Single Spacing
                    doc.buffer.setLineSpacingSingle();
                    toast_notify::info("Line spacing: Single");
                    break;
                case 56:  // 1.5 Line Spacing
                    doc.buffer.setLineSpacing1_5();
                    toast_notify::info("Line spacing: 1.5");
                    break;
                case 57:  // Double Spacing
                    doc.buffer.setLineSpacingDouble();
                    toast_notify::info("Line spacing: Double");
                    break;
                // (58 is separator)
                case 59:  // Bulleted List
                    doc.buffer.toggleBulletedList();
                    toast_notify::info(doc.buffer.currentListType() == ListType::Bulleted ? "Bullets on" : "Bullets off");
                    break;
                case 60:  // Numbered List
                    doc.buffer.toggleNumberedList();
                    toast_notify::info(doc.buffer.currentListType() == ListType::Numbered ? "Numbering on" : "Numbering off");
                    break;
                case 61:  // Increase List Level
                    doc.buffer.increaseListLevel();
                    toast_notify::info("List level increased");
                    break;
                case 62:  // Decrease List Level
                    doc.buffer.decreaseListLevel();
                    toast_notify::info("List level decreased");
                    break;
                case 64:  // Increase Space Before
                    doc.buffer.setCurrentSpaceBefore(doc.buffer.currentSpaceBefore() + 6);
                    toast_notify::info("Space before increased");
                    break;
                case 65:  // Decrease Space Before
                    doc.buffer.setCurrentSpaceBefore(doc.buffer.currentSpaceBefore() - 6);
                    toast_notify::info("Space before decreased");
                    break;
                case 66:  // Increase Space After
                    doc.buffer.setCurrentSpaceAfter(doc.buffer.currentSpaceAfter() + 6);
                    toast_notify::info("Space after increased");
                    break;
                case 67:  // Decrease Space After
                    doc.buffer.setCurrentSpaceAfter(doc.buffer.currentSpaceAfter() - 6);
                    toast_notify::info("Space after decreased");
                    break;
                case 69:  // Drop Cap
                    doc.buffer.toggleCurrentLineDropCap();
                    toast_notify::info(doc.buffer.currentLineHasDropCap() ? "Drop cap: On" : "Drop cap: Off");
                    break;
                case 70:  // Tab Width...
                    menu.showTabWidthDialog = true;
                    break;
                default:
                    break;
            }
        } else if (menuIndex == 3) {  // Insert menu
            switch (itemIndex) {
                case 0:  // Page Break
                    doc.buffer.insertPageBreak();
                    doc.isDirty = true;
                    toast_notify::info("Page break inserted");
                    break;
                case 1:  // Section Break
                    doc.buffer.insertSectionBreak();
                    doc.isDirty = true;
                    toast_notify::info("Section break inserted");
                    break;
                case 3:  // Hyperlink...
                    // For now, just add hyperlink to selection if any
                    if (doc.buffer.hasSelection()) {
                        // Would need a dialog for URL input - placeholder for now
                        if (doc.buffer.addHyperlink("https://example.com")) {
                            doc.isDirty = true;
                            toast_notify::info("Hyperlink added (edit URL)");
                        }
                    } else {
                        toast_notify::error("Select text first");
                    }
                    break;
                case 4:  // Remove Hyperlink
                    if (doc.buffer.hyperlinkAtCaret()) {
                        std::size_t caretOffset = doc.buffer.caretOffset();
                        if (doc.buffer.removeHyperlink(caretOffset)) {
                            doc.isDirty = true;
                            toast_notify::success("Hyperlink removed");
                        }
                    } else {
                        toast_notify::error("No hyperlink at cursor");
                    }
                    break;
                case 5:  // Bookmark...
                {
                    std::string name =
                        std::format("bookmark_{}", doc.buffer.caret().row + 1);
                    if (doc.buffer.addBookmark(name)) {
                        toast_notify::success("Bookmark added");
                    } else {
                        toast_notify::error("Bookmark not added");
                    }
                    break;
                }
                case 6:  // Comment...
                    if (doc.buffer.hasSelection()) {
                        CaretPosition start = doc.buffer.selectionStart();
                        CaretPosition end = doc.buffer.selectionEnd();
                        menu.pendingCommentStart =
                            doc.buffer.offsetForPosition(start);
                        menu.pendingCommentEnd = doc.buffer.offsetForPosition(end);
                        menu.showCommentDialog = true;
                    } else {
                        toast_notify::error("Select text to comment");
                    }
                    break;
                case 8:  // Table...
                {
                    std::size_t currentLine = doc.buffer.caret().row;
                    doc.insertTable(currentLine, 3, 3);
                    doc.isDirty = true;
                    toast_notify::info("Inserted 3x3 table");
                    break;
                }
                case 9:  // Image...
                {
                    // Insert a placeholder image at the current line
                    DocumentImage img;
                    img.anchorLine = doc.buffer.caret().row;
                    img.anchorColumn = doc.buffer.caret().column;
                    img.layoutMode = ImageLayoutMode::Inline;
                    img.displayWidth = 200.0f;
                    img.displayHeight = 150.0f;
                    img.originalWidth = 200.0f;
                    img.originalHeight = 150.0f;
                    img.altText = "Inserted image";
                    img.filename = "placeholder.png";
                    doc.images.addImage(img);
                    doc.isDirty = true;
                    toast_notify::info("Image placeholder inserted");
                    break;
                }
                case 13:  // Shape: Line
                case 14:  // Shape: Rectangle
                case 15:  // Shape: Circle
                case 16:  // Shape: Ellipse
                case 17:  // Shape: Arrow
                case 18:  // Shape: Rounded Rect
                case 19:  // Shape: Triangle
                {
                    DocumentDrawing drawing;
                    drawing.anchorLine = doc.buffer.caret().row;
                    drawing.anchorColumn = doc.buffer.caret().column;
                    drawing.x = 10.0f;
                    drawing.y = 10.0f;
                    drawing.width = 100.0f;
                    drawing.height = 50.0f;
                    drawing.strokeColor = DrawingColors::Black;
                    drawing.fillColor = DrawingColors::LightGray;
                    drawing.strokeWidth = 2.0f;
                    drawing.layoutMode = DrawingLayoutMode::Inline;
                    
                    switch (itemIndex) {
                        case 13: drawing.shapeType = ShapeType::Line; break;
                        case 14: drawing.shapeType = ShapeType::Rectangle; break;
                        case 15: drawing.shapeType = ShapeType::Ellipse; break;  // Circle is an ellipse
                        case 16: drawing.shapeType = ShapeType::Ellipse; break;
                        case 17: drawing.shapeType = ShapeType::Arrow; break;
                        case 18: drawing.shapeType = ShapeType::RoundedRect; break;
                        case 19: drawing.shapeType = ShapeType::Triangle; break;
                        default: break;
                    }
                    
                    doc.drawings.addDrawing(drawing);
                    doc.isDirty = true;
                    toast_notify::info(std::format("{} inserted", shapeTypeName(drawing.shapeType)));
                    break;
                }
                case 21:  // Equation...
                {
                    DocumentEquation eq;
                    eq.anchorLine = doc.buffer.caret().row;
                    eq.anchorColumn = doc.buffer.caret().column;
                    eq.source = "f(x) = x^2";  // Default sample equation
                    eq.style = EquationStyle::Inline;
                    doc.equations.addEquation(eq);
                    doc.isDirty = true;
                    toast_notify::info("Equation inserted");
                    break;
                }
                case 22:  // Footnote
                {
                    // Insert footnote marker as placeholder
                    doc.buffer.insertText("[1]");
                    doc.isDirty = true;
                    toast_notify::info("Footnote marker inserted");
                    break;
                }
                case 25:  // Header
                {
                    // Insert header placeholder text at current position
                    doc.buffer.insertText("[HEADER]");
                    doc.isDirty = true;
                    toast_notify::info("Header placeholder inserted");
                    break;
                }
                case 26:  // Footer
                {
                    // Insert footer placeholder text at current position
                    doc.buffer.insertText("[FOOTER]");
                    doc.isDirty = true;
                    toast_notify::info("Footer placeholder inserted");
                    break;
                }
                case 27:  // Page Number
                {
                    // Insert page number placeholder at current position
                    doc.buffer.insertText("[PAGE #]");
                    doc.isDirty = true;
                    toast_notify::info("Page number placeholder inserted");
                    break;
                }
                case 30:  // Table of Contents
                {
                    doc.buffer.insertTableOfContents();
                    doc.isDirty = true;
                    toast_notify::info("Table of Contents inserted");
                    break;
                }
                default:
                    break;
            }
        } else if (menuIndex == 6) {  // Table menu
            switch (itemIndex) {
                case 0: {  // Insert Table...
                    // Insert a default 3x3 table at current line
                    std::size_t currentLine = doc.buffer.caret().row;
                    doc.insertTable(currentLine, 3, 3);
                    doc.isDirty = true;
                    toast_notify::info("Inserted 3x3 table");
                    break;
                }
                case 2: {  // Insert Row Above
                    std::size_t currentLine = doc.buffer.caret().row;
                    Table* table = doc.tableAtLine(currentLine);
                    if (table) {
                        table->insertRowAbove(table->currentCell().row);
                        doc.isDirty = true;
                        toast_notify::info("Row inserted above");
                    } else {
                        toast_notify::error("No table at cursor");
                    }
                    break;
                }
                case 3: {  // Insert Row Below
                    std::size_t currentLine = doc.buffer.caret().row;
                    Table* table = doc.tableAtLine(currentLine);
                    if (table) {
                        table->insertRowBelow(table->currentCell().row);
                        doc.isDirty = true;
                        toast_notify::info("Row inserted below");
                    } else {
                        toast_notify::error("No table at cursor");
                    }
                    break;
                }
                case 4: {  // Insert Column Left
                    std::size_t currentLine = doc.buffer.caret().row;
                    Table* table = doc.tableAtLine(currentLine);
                    if (table) {
                        table->insertColumnLeft(table->currentCell().col);
                        doc.isDirty = true;
                        toast_notify::info("Column inserted left");
                    } else {
                        toast_notify::error("No table at cursor");
                    }
                    break;
                }
                case 5: {  // Insert Column Right
                    std::size_t currentLine = doc.buffer.caret().row;
                    Table* table = doc.tableAtLine(currentLine);
                    if (table) {
                        table->insertColumnRight(table->currentCell().col);
                        doc.isDirty = true;
                        toast_notify::info("Column inserted right");
                    } else {
                        toast_notify::error("No table at cursor");
                    }
                    break;
                }
                case 7: {  // Delete Row
                    std::size_t currentLine = doc.buffer.caret().row;
                    Table* table = doc.tableAtLine(currentLine);
                    if (table) {
                        table->deleteRow(table->currentCell().row);
                        doc.isDirty = true;
                        toast_notify::info("Row deleted");
                    } else {
                        toast_notify::error("No table at cursor");
                    }
                    break;
                }
                case 8: {  // Delete Column
                    std::size_t currentLine = doc.buffer.caret().row;
                    Table* table = doc.tableAtLine(currentLine);
                    if (table) {
                        table->deleteColumn(table->currentCell().col);
                        doc.isDirty = true;
                        toast_notify::info("Column deleted");
                    } else {
                        toast_notify::error("No table at cursor");
                    }
                    break;
                }
                case 10: {  // Merge Cells
                    std::size_t currentLine = doc.buffer.caret().row;
                    Table* table = doc.tableAtLine(currentLine);
                    if (table && table->hasSelection()) {
                        CellPosition start = table->selectionStart();
                        CellPosition end = table->selectionEnd();
                        // Normalize selection
                        CellPosition topLeft = {std::min(start.row, end.row), std::min(start.col, end.col)};
                        CellPosition bottomRight = {std::max(start.row, end.row), std::max(start.col, end.col)};
                        if (table->mergeCells(topLeft, bottomRight)) {
                            doc.isDirty = true;
                            toast_notify::success("Cells merged");
                        } else {
                            toast_notify::error("Cannot merge cells");
                        }
                    } else {
                        toast_notify::error("Select cells to merge");
                    }
                    break;
                }
                case 11: {  // Split Cell
                    std::size_t currentLine = doc.buffer.caret().row;
                    Table* table = doc.tableAtLine(currentLine);
                    if (table) {
                        if (table->splitCell(table->currentCell())) {
                            doc.isDirty = true;
                            toast_notify::success("Cell split");
                        } else {
                            toast_notify::error("Cell is not merged");
                        }
                    } else {
                        toast_notify::error("No table at cursor");
                    }
                    break;
                }
                default:
                    break;
            }
        } else if (menuIndex == 7) {  // Help menu
            if (itemIndex == 0) {     // Keyboard Shortcuts
                menu.showHelpWindow = true;
            } else if (itemIndex == 2) {  // About (after separator)
                menu.showAboutDialog = true;
            }
        } else if (menuIndex == 5) {  // Tools menu (includes former Settings items)
            switch (itemIndex) {
                case 0:  // Word Count...
                    menu.showWordCountDialog = true;
                    break;
                case 2:  // UI Scale... (was Settings item 0)
                    menu.showSettingsDialog = true;
                    {
                        int currentPercentage = static_cast<int>(Settings::get().get_ui_scale() * 100.0f);
                        menu.uiScaleInputStr = std::to_string(currentPercentage);
                    }
                    break;
                default:
                    break;
            }
        }
    }


}  // namespace ecs
