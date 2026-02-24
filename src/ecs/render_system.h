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

// System for rendering the complete editor UI
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
        // Note: end_drawing() is now called in app_frame() after sm.run(dt)
    }

    void for_each_with(afterhours::Entity& /*entity*/,
                       DocumentComponent& doc,
                       CaretComponent& caret,
                       ScrollComponent& scroll,
                       LayoutComponent& layout,
                       const float) override {
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


}  // namespace ecs
