#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <sstream>

#include "../../vendor/afterhours/src/core/system.h"
#include "../../vendor/afterhours/src/plugins/clipboard.h"
#include "../editor/document_io.h"
#include "../editor/export/export_html.h"
#include "../editor/export/export_pdf.h"
#include "../editor/export/export_rtf.h"
#include "../editor/image.h"
#include "../editor/table.h"
#include "../input/action_map.h"
#include "../rl.h"
#include "../settings.h"
#include "../testing/test_input.h"
#include "../ui/theme.h"
#include "../ui/win95_widgets.h"
#include "../ui/menu_setup.h"
#include "../util/drawing.h"
#include "../util/logging.h"
#include "component_helpers.h"
#include "components.h"

namespace ecs {

// Helper to draw text and register it for E2E testing
inline void drawTextWithRegistry(const char* text, int x, int y, int fontSize, 
                                  raylib::Color color) {
    raylib::DrawText(text, x, y, fontSize, color);
    test_input::registerVisibleText(text);
}

// Helper to draw text with font and register it for E2E testing
inline void drawTextExWithRegistry(raylib::Font font, const char* text, 
                                    raylib::Vector2 pos, float fontSize, 
                                    float spacing, raylib::Color color) {
    raylib::DrawTextEx(font, text, pos, fontSize, spacing, color);
    test_input::registerVisibleText(text);
}

// Draw a page background with shadow (for paged mode)
inline void drawPageBackground(const LayoutComponent& layout) {
    if (layout.pageMode != PageMode::Paged) return;

    float pageY = layout.textArea.y + 10.0f;  // 10px margin from top

    // Draw page shadow
    raylib::Rectangle shadowRect = {layout.pageOffsetX + 4.0f, pageY + 4.0f,
                                    layout.pageDisplayWidth,
                                    layout.pageDisplayHeight};
    raylib::DrawRectangleRec(shadowRect, raylib::Color{100, 100, 100, 128});

    // Draw page (white background)
    raylib::Rectangle pageRect = {layout.pageOffsetX, pageY,
                                  layout.pageDisplayWidth,
                                  layout.pageDisplayHeight};
    raylib::DrawRectangleRec(pageRect, raylib::WHITE);

    // Draw page border
    raylib::DrawRectangleLinesEx(pageRect, 1.0f, raylib::DARKGRAY);

    // Draw margin guidelines (dotted or light lines)
    float marginScaled = layout.pageMargin * layout.pageScale;
    raylib::Color marginColor = raylib::Color{200, 200, 200, 100};

    // Left margin
    raylib::DrawLine(static_cast<int>(layout.pageOffsetX + marginScaled),
                     static_cast<int>(pageY),
                     static_cast<int>(layout.pageOffsetX + marginScaled),
                     static_cast<int>(pageY + layout.pageDisplayHeight),
                     marginColor);

    // Right margin
    raylib::DrawLine(static_cast<int>(layout.pageOffsetX +
                                      layout.pageDisplayWidth - marginScaled),
                     static_cast<int>(pageY),
                     static_cast<int>(layout.pageOffsetX +
                                      layout.pageDisplayWidth - marginScaled),
                     static_cast<int>(pageY + layout.pageDisplayHeight),
                     marginColor);

    // Top margin
    raylib::DrawLine(
        static_cast<int>(layout.pageOffsetX),
        static_cast<int>(pageY + marginScaled),
        static_cast<int>(layout.pageOffsetX + layout.pageDisplayWidth),
        static_cast<int>(pageY + marginScaled), marginColor);

    // Bottom margin
    raylib::DrawLine(
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
            raylib::Color bgColor = {cell.backgroundColor.r, cell.backgroundColor.g,
                                     cell.backgroundColor.b, cell.backgroundColor.a};
            raylib::DrawRectangle(static_cast<int>(cellX), static_cast<int>(cellY),
                                 static_cast<int>(cellW), static_cast<int>(cellH), bgColor);
            
            // Draw cell border
            raylib::Color borderColor = raylib::BLACK;
            switch (cell.borders.top) {
                case BorderStyle::Thin:
                    raylib::DrawLine(static_cast<int>(cellX), static_cast<int>(cellY),
                                    static_cast<int>(cellX + cellW), static_cast<int>(cellY), borderColor);
                    break;
                case BorderStyle::Medium:
                    raylib::DrawLineEx({cellX, cellY}, {cellX + cellW, cellY}, 2.0f, borderColor);
                    break;
                case BorderStyle::Thick:
                    raylib::DrawLineEx({cellX, cellY}, {cellX + cellW, cellY}, 3.0f, borderColor);
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
                    raylib::DrawLine(static_cast<int>(cellX), static_cast<int>(cellY + cellH),
                                    static_cast<int>(cellX + cellW), static_cast<int>(cellY + cellH), borderColor);
                    break;
                case BorderStyle::Medium:
                    raylib::DrawLineEx({cellX, cellY + cellH}, {cellX + cellW, cellY + cellH}, 2.0f, borderColor);
                    break;
                case BorderStyle::Thick:
                    raylib::DrawLineEx({cellX, cellY + cellH}, {cellX + cellW, cellY + cellH}, 3.0f, borderColor);
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
                    raylib::DrawLine(static_cast<int>(cellX), static_cast<int>(cellY),
                                    static_cast<int>(cellX), static_cast<int>(cellY + cellH), borderColor);
                    break;
                case BorderStyle::Medium:
                    raylib::DrawLineEx({cellX, cellY}, {cellX, cellY + cellH}, 2.0f, borderColor);
                    break;
                case BorderStyle::Thick:
                    raylib::DrawLineEx({cellX, cellY}, {cellX, cellY + cellH}, 3.0f, borderColor);
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
                    raylib::DrawLine(static_cast<int>(cellX + cellW), static_cast<int>(cellY),
                                    static_cast<int>(cellX + cellW), static_cast<int>(cellY + cellH), borderColor);
                    break;
                case BorderStyle::Medium:
                    raylib::DrawLineEx({cellX + cellW, cellY}, {cellX + cellW, cellY + cellH}, 2.0f, borderColor);
                    break;
                case BorderStyle::Thick:
                    raylib::DrawLineEx({cellX + cellW, cellY}, {cellX + cellW, cellY + cellH}, 3.0f, borderColor);
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
                raylib::Color textColor = {cell.textStyle.textColor.r, cell.textStyle.textColor.g,
                                          cell.textStyle.textColor.b, cell.textStyle.textColor.a};
                raylib::DrawText(cell.content.c_str(), textX, textY, fontSize, textColor);
            }
            
            // Highlight current cell if editing
            if (isEditing && row == currentCell.row && col == currentCell.col) {
                raylib::DrawRectangleLinesEx(
                    {cellX, cellY, cellW, cellH}, 2.0f, 
                    raylib::Color{0, 120, 215, 255});  // Blue highlight
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

        std::string line = (span.length > 0) ? buffer.lineString(row) : "";
        auto expandTabs = [tabWidth](const std::string& input) {
            if (tabWidth <= 0) return input;
            std::string expanded;
            expanded.reserve(input.size());
            int col = 0;
            for (char ch : input) {
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
        std::string displayLine = expandTabs(line);
        
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
            raylib::Color breakColor = {128, 128, 128, 255};  // Gray
            for (int px = lineStart; px < lineEnd; px += 8) {
                raylib::DrawLine(px, breakY, px + 4, breakY, breakColor);
            }
            
            // Draw "Page Break" text in center
            const char* breakText = "Page Break";
            int textWidth = raylib::MeasureText(breakText, 10);
            int textX = lineStart + (lineEnd - lineStart - textWidth) / 2;
            
            // Draw background for text
            raylib::DrawRectangle(textX - 4, breakY - 6, textWidth + 8, 12, 
                                 raylib::Color{255, 255, 255, 255});
            raylib::DrawText(breakText, textX, breakY - 5, 10, breakColor);
            
            y += 20;  // Add space for the page break indicator
        }
        
        // Draw line number in gutter if enabled
        if (showLineNumbers) {
            int lineNum = static_cast<int>(row + 1);  // 1-based line numbers
            char lineNumStr[16];
            std::snprintf(lineNumStr, sizeof(lineNumStr), "%d", lineNum);
            
            // Measure line number text to right-align in gutter
            int numWidth = raylib::MeasureText(lineNumStr, 14);
            int gutterX = static_cast<int>(textArea.x) + static_cast<int>(lineNumberGutterWidth) - numWidth - 8;
            
            // Draw line number in gray
            raylib::Color lineNumColor = {128, 128, 128, 255};
            raylib::DrawText(lineNumStr, gutterX, y, 14, lineNumColor);
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
        int textWidth = displayLine.empty() ? 0 : raylib::MeasureText(displayLine.c_str(), lineFontSize);
        
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
            raylib::Color textColor = {globalStyle.textColor.r, globalStyle.textColor.g,
                                       globalStyle.textColor.b, globalStyle.textColor.a};
            
            if (listType == ListType::Bulleted) {
                const char* bullet = bulletForLevel(listLevel);
                raylib::DrawText(bullet, markerX, y, lineFontSize, textColor);
            } else if (listType == ListType::Numbered) {
                char numberStr[16];
                std::snprintf(numberStr, sizeof(numberStr), "%d.", listNumber);
                raylib::DrawText(numberStr, markerX, y, lineFontSize, textColor);
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

                if (startCol < endCol && !line.empty()) {
                    std::string beforeSel = expandTabs(line.substr(0, startCol));
                    std::string selectedText =
                        expandTabs(line.substr(startCol, endCol - startCol));

                    int selX =
                        x + raylib::MeasureText(beforeSel.c_str(), lineFontSize);
                    int selWidth =
                        raylib::MeasureText(selectedText.c_str(), lineFontSize);
                    raylib::DrawRectangle(selX, y, selWidth, lineHeight,
                                          theme::SELECTION_BG);
                }
            }
        }

        // Draw text with paragraph style applied
        if (!line.empty()) {
            // Register document text for E2E tests
            test_input::registerVisibleText(displayLine);
            
            // Get global text style for underline/strikethrough/colors
            TextStyle globalStyle = buffer.textStyle();
            
            // Convert TextColor to raylib::Color
            raylib::Color textColor = {globalStyle.textColor.r, globalStyle.textColor.g,
                                       globalStyle.textColor.b, globalStyle.textColor.a};
            
            // Draw highlight background if set
            if (!globalStyle.highlightColor.isNone()) {
                raylib::Color highlightColor = {globalStyle.highlightColor.r, globalStyle.highlightColor.g,
                                                globalStyle.highlightColor.b, globalStyle.highlightColor.a};
                raylib::DrawRectangle(x, y, textWidth, lineHeight, highlightColor);
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
                raylib::DrawText(dropChar.c_str(), x, y - lineFontSize / 2,
                                 dropFontSize, textColor);
                int dropWidth = raylib::MeasureText(dropChar.c_str(), dropFontSize);
                textToDraw = textToDraw.substr(1);
                if (!textToDraw.empty()) {
                    x += dropWidth + 4;
                }
            }

            // For headings and titles, draw bold text (simulated by drawing twice with offset)
            if (paragraphStyleIsBold(paraStyle) || globalStyle.bold) {
                // Draw bold effect by drawing text twice with 1px offset
                raylib::DrawText(textToDraw.c_str(), x, y + textYOffset, textFontSize, textColor);
                raylib::DrawText(textToDraw.c_str(), x + 1, y + textYOffset, textFontSize, textColor);
            } else if (paragraphStyleIsItalic(paraStyle) || globalStyle.italic) {
                // For subtitle italic style, draw in a slightly different shade
                raylib::Color italicColor = {static_cast<unsigned char>(textColor.r / 2 + 64),
                                             static_cast<unsigned char>(textColor.g / 2 + 64),
                                             static_cast<unsigned char>(textColor.b / 2 + 64), textColor.a};
                raylib::DrawText(textToDraw.c_str(), x, y + textYOffset, textFontSize, italicColor);
            } else {
                raylib::DrawText(textToDraw.c_str(), x, y + textYOffset, textFontSize, textColor);
            }
            
            // Draw underline if enabled
            if (globalStyle.underline) {
                int underlineY = y + lineFontSize + 1;
                raylib::DrawLine(x, underlineY, x + textWidth, underlineY, textColor);
            }
            
            // Draw strikethrough if enabled
            if (globalStyle.strikethrough) {
                int strikeY = y + lineFontSize / 2;
                raylib::DrawLine(x, strikeY, x + textWidth, strikeY, textColor);
            }
        }

        // Draw caret
        if (caretVisible && row == caret.row) {
            std::string beforeCaret =
                expandTabs(line.substr(0, std::min(caret.column, line.length())));
            int caretX = x + raylib::MeasureText(beforeCaret.c_str(), lineFontSize);
            raylib::DrawRectangle(caretX, y, 2, lineHeight, theme::CARET_COLOR);
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
                          MenuComponent& menu, StatusComponent& status,
                          LayoutComponent& layout);
void drawHelpWindowImpl(MenuComponent& menu, const LayoutComponent& layout);

// System for rendering the complete editor UI
struct EditorRenderSystem
    : public afterhours::System<DocumentComponent, CaretComponent,
                                ScrollComponent, StatusComponent,
                                LayoutComponent, MenuComponent> {
    void once(const float) const override {
        raylib::BeginDrawing();
        raylib::ClearBackground(theme::WINDOW_BG);
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
                    raylib::TakeScreenshot(pathStr.c_str());
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
                int screenWidth = raylib::GetScreenWidth();
                int overlayWidth = 400;
                int overlayHeight = 50;
                int overlayX = screenWidth - overlayWidth - 10;
                int overlayY = 10;
                
                // Draw semi-transparent background
                raylib::DrawRectangle(overlayX, overlayY, overlayWidth, overlayHeight, 
                                      raylib::Color{0, 0, 0, 200});
                raylib::DrawRectangleLines(overlayX, overlayY, overlayWidth, overlayHeight, 
                                           raylib::Color{255, 255, 0, 255});
                
                // Draw current command
                std::string cmdText = testConfig.e2eCurrentCommand;
                if (cmdText.length() > 40) {
                    cmdText = cmdText.substr(0, 37) + "...";
                }
                raylib::DrawText(cmdText.c_str(), overlayX + 5, overlayY + 5, 14, 
                                 raylib::Color{255, 255, 255, 255});
                
                // Draw timeout countdown
                std::string timeoutText;
                if (testConfig.e2eTimeoutSeconds >= 0) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "<%.1fs>", testConfig.e2eTimeoutSeconds);
                    timeoutText = buf;
                } else {
                    timeoutText = "<no timeout>";
                }
                raylib::DrawText(timeoutText.c_str(), overlayX + 5, overlayY + 25, 14, 
                                 raylib::Color{255, 200, 100, 255});
            }
        }
        raylib::EndDrawing();
    }

    void for_each_with(const afterhours::Entity& /*entity*/,
                       const DocumentComponent& doc,
                       const CaretComponent& caret,
                       const ScrollComponent& scroll,
                       const StatusComponent& status,
                       const LayoutComponent& layout, const MenuComponent& menu,
                       const float) const override {
        // Draw title bar
        raylib::Rectangle titleBarRect = {layout.titleBar.x, layout.titleBar.y,
                                          layout.titleBar.width,
                                          layout.titleBar.height};
        raylib::DrawRectangleRec(titleBarRect, theme::TITLE_BAR);

        std::string title = "Wordproc";
        if (!doc.filePath.empty()) {
            title +=
                " - " + std::filesystem::path(doc.filePath).filename().string();
        } else {
            title += " - Untitled";
        }
        if (doc.isDirty) {
            title += " *";
        }
        drawTextWithRegistry(title.c_str(), 4, 4, theme::layout::FONT_SIZE,
                             theme::TITLE_TEXT);

        // Draw menu bar background (menus are drawn later after text area)
        if (!layout.focusMode) {
            raylib::Rectangle menuBarRect = {layout.menuBar.x, layout.menuBar.y,
                                             layout.menuBar.width,
                                             layout.menuBar.height};
            raylib::DrawRectangleRec(menuBarRect, theme::WINDOW_BG);
            util::drawRaisedBorder(menuBarRect);
        }

        // Get mutable refs for later use
        auto& mutableDoc = const_cast<DocumentComponent&>(doc);
        auto& mutableMenu = const_cast<MenuComponent&>(menu);
        auto& mutableStatus = const_cast<StatusComponent&>(status);
        auto& mutableLayout = const_cast<LayoutComponent&>(layout);

        // F1 to show help window
        if (IsKeyPressed(raylib::KEY_F1)) {
            mutableMenu.showHelpWindow = !mutableMenu.showHelpWindow;
            mutableMenu.helpScrollOffset = 0;
        }

        // Draw text area background
        raylib::Rectangle textAreaRect = {layout.textArea.x, layout.textArea.y,
                                          layout.textArea.width,
                                          layout.textArea.height};

        // In paged mode, draw a gray background; in pageless mode, draw white
        if (layout.pageMode == PageMode::Paged) {
            raylib::DrawRectangleRec(textAreaRect,
                                     raylib::Color{128, 128, 128, 255});
            util::drawSunkenBorder(textAreaRect);

            // Draw the page with shadow and margins
            drawPageBackground(layout);
        } else {
            raylib::DrawRectangleRec(textAreaRect, theme::TEXT_AREA_BG);
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
            raylib::DrawLine(static_cast<int>(effectiveArea.x),
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

        // Draw comment markers in the right margin
        if (!doc.comments.empty()) {
            for (const auto& comment : doc.comments) {
                CaretPosition pos = doc.buffer.positionForOffset(comment.startOffset);
                if (pos.row < static_cast<std::size_t>(scroll.offset)) continue;
                int markerY = static_cast<int>(effectiveArea.y) + theme::layout::TEXT_PADDING +
                              static_cast<int>(pos.row - scroll.offset) * lineHeight;
                int markerX = static_cast<int>(effectiveArea.x + effectiveArea.width) - 8;
                if (markerY > static_cast<int>(effectiveArea.y + effectiveArea.height)) continue;
                raylib::DrawRectangle(markerX, markerY, 6, 6, raylib::Color{255, 200, 0, 255});
            }
        }

        // Draw status bar
        if (!layout.focusMode) {
            raylib::Rectangle statusBarRect = {
                layout.statusBar.x, layout.statusBar.y, layout.statusBar.width,
                layout.statusBar.height};
            raylib::DrawRectangleRec(statusBarRect, theme::STATUS_BAR);
            util::drawRaisedBorder(statusBarRect);

            double currentTime = raylib::GetTime();
            if (!status.text.empty() && currentTime < status.expiresAt) {
                raylib::Color msgColor =
                    status.isError ? theme::STATUS_ERROR : theme::STATUS_SUCCESS;
                raylib::DrawText(
                    status.text.c_str(), 4,
                    layout.screenHeight - theme::layout::STATUS_BAR_HEIGHT + 2,
                    theme::layout::FONT_SIZE - 2, msgColor);
            } else {
                CaretPosition caretPos = doc.buffer.caret();
                ParagraphStyle paraStyle = doc.buffer.currentParagraphStyle();
                TextStats stats = doc.buffer.stats();
                std::string statusText = std::format(
                    "Ln {}, Col {} | {} | {}{}{}{}| {}pt | {} | Words: {} | Zoom: {}%",
                    caretPos.row + 1, caretPos.column + 1,
                    paragraphStyleName(paraStyle),
                    style.bold ? "B " : "",
                    style.italic ? "I " : "",
                    style.underline ? "U " : "",
                    style.strikethrough ? "S " : "",
                    style.fontSize, style.font, stats.words,
                    static_cast<int>(layout.zoomLevel * 100.0f));
                drawTextWithRegistry(
                    statusText.c_str(), 4,
                    layout.screenHeight - theme::layout::STATUS_BAR_HEIGHT + 2,
                    theme::layout::FONT_SIZE - 2, theme::TEXT_COLOR);
            }
        }

        if (!layout.focusMode) {
            // Draw interactive menus ON TOP of everything except dialogs
            // (drawn last so dropdowns appear above the text area)
            int menuResult =
                win95::DrawMenuBar(mutableMenu.menus, theme::layout::TITLE_BAR_HEIGHT,
                                   theme::layout::MENU_BAR_HEIGHT);
            if (menuResult >= 0) {
                handleMenuActionImpl(menuResult, mutableDoc, mutableMenu, mutableStatus, mutableLayout);
            }
        }

        // Draw About dialog if active
        if (menu.showAboutDialog) {
            raylib::Rectangle dialogRect = {
                static_cast<float>(layout.screenWidth / 2 - 150),
                static_cast<float>(layout.screenHeight / 2 - 75), 300, 150};
            int result = win95::DrawMessageDialog(dialogRect, "About Wordproc",
                                     "Wordproc v0.1\n\nA Windows 95 style word "
                                     "processor\nbuilt with Afterhours.",
                                     false);
            if (result >= 0) {
                mutableMenu.showAboutDialog = false;
            }
        }

        // Draw Help window if active
        if (menu.showHelpWindow) {
            drawHelpWindowImpl(mutableMenu, layout);
        }
    }
};

// System for rendering menus and handling interactions
// Note: Render systems in Afterhours call the const version of for_each_with.
// We use const_cast to allow immediate-mode UI state updates during rendering.
struct MenuSystem
    : public afterhours::System<DocumentComponent, MenuComponent,
                                StatusComponent, LayoutComponent> {

    // Const version called by render systems - uses const_cast for immediate-mode UI
    void for_each_with(const afterhours::Entity& /*entity*/,
                       const DocumentComponent& docConst,
                       const MenuComponent& menuConst,
                       const StatusComponent& statusConst,
                       const LayoutComponent& layoutConst,
                       const float) const override {
        // const_cast for immediate-mode UI that needs to update state during draw
        auto& doc = const_cast<DocumentComponent&>(docConst);
        auto& menu = const_cast<MenuComponent&>(menuConst);
        auto& status = const_cast<StatusComponent&>(statusConst);
        auto& layout = const_cast<LayoutComponent&>(layoutConst);
        renderMenus(doc, menu, status, layout);
    }

    // Mutable version (not called by render systems, but kept for compatibility)
    void for_each_with(afterhours::Entity& /*entity*/, DocumentComponent& doc,
                       MenuComponent& menu, StatusComponent& status,
                       LayoutComponent& layout, const float) override {
        renderMenus(doc, menu, status, layout);
    }

    void renderMenus(DocumentComponent& doc, MenuComponent& menu,
                     StatusComponent& status, LayoutComponent& layout) const {
        // Menu bar is now rendered by MenuUISystem using Afterhours UI
        // Just consume any click results and handle actions here
        int menuResult = menu.consumeClickedResult();

        if (menuResult >= 0) {
            handleMenuAction(menuResult, doc, menu, status, layout);
        }

        // Handle About dialog dismissal
        if (menu.showAboutDialog) {
            raylib::Rectangle dialogRect = {
                static_cast<float>(raylib::GetScreenWidth() / 2 - 150),
                static_cast<float>(raylib::GetScreenHeight() / 2 - 75), 300,
                150};
            int result = win95::DrawMessageDialog(
                dialogRect, "About Wordproc",
                "Wordproc v0.1\n\nA Windows 95 style word processor\nbuilt "
                "with Afterhours.",
                false);
            if (result >= 0) {
                menu.showAboutDialog = false;
            }
        }

        // Word count dialog
        if (menu.showWordCountDialog) {
            TextStats stats = doc.buffer.stats();
            std::string msg = std::format(
                "Words: {}\nCharacters: {}\nLines: {}\nParagraphs: {}\nSentences: {}",
                stats.words, stats.characters, stats.lines, stats.paragraphs,
                stats.sentences);
            raylib::Rectangle dialogRect = {
                static_cast<float>(raylib::GetScreenWidth() / 2 - 160),
                static_cast<float>(raylib::GetScreenHeight() / 2 - 90), 320,
                180};
            int result = win95::DrawMessageDialog(dialogRect, "Word Count",
                                                  msg.c_str(), false);
            if (result >= 0) {
                menu.showWordCountDialog = false;
            }
        }

        // Comment dialog
        if (menu.showCommentDialog) {
            raylib::Rectangle dialogRect = {
                static_cast<float>(raylib::GetScreenWidth() / 2 - 180),
                static_cast<float>(raylib::GetScreenHeight() / 2 - 90), 360,
                180};
            int result = win95::DrawInputDialog(
                dialogRect, "Add Comment", "Comment:", menu.commentInputBuffer,
                static_cast<int>(sizeof(menu.commentInputBuffer)));
            if (result == 0) {
                Comment comment;
                comment.startOffset = menu.pendingCommentStart;
                comment.endOffset = menu.pendingCommentEnd;
                comment.author = "User";
                comment.text = menu.commentInputBuffer;
                comment.createdAt = std::time(nullptr);
                doc.comments.push_back(comment);
                menu.commentInputBuffer[0] = '\0';
                menu.showCommentDialog = false;
                status::set(status, "Comment added");
                status.expiresAt = raylib::GetTime() + 2.0;
            } else if (result > 0) {
                menu.commentInputBuffer[0] = '\0';
                menu.showCommentDialog = false;
            }
        }

        // Template dialog
        if (menu.showTemplateDialog) {
            raylib::Rectangle dialogRect = {
                static_cast<float>(raylib::GetScreenWidth() / 2 - 180),
                static_cast<float>(raylib::GetScreenHeight() / 2 - 90), 360,
                180};
            int result = win95::DrawInputDialog(
                dialogRect, "New from Template",
                "Template (letter/memo/report/resume/essay):",
                menu.templateInputBuffer,
                static_cast<int>(sizeof(menu.templateInputBuffer)));
            if (result == 0) {
                std::string name = menu.templateInputBuffer;
                for (auto& ch : name) ch = static_cast<char>(std::tolower(ch));
                std::filesystem::path templatePath =
                    std::filesystem::current_path() / "resources/templates" /
                    (name + ".txt");
                if (std::filesystem::exists(templatePath)) {
                    std::ifstream ifs(templatePath);
                    std::stringstream buffer;
                    buffer << ifs.rdbuf();
                    doc.buffer.setText(buffer.str());
                    doc.isDirty = true;
                    status::set(status, "Template loaded: " + name);
                } else {
                    status::set(status, "Template not found: " + name, true);
                }
                status.expiresAt = raylib::GetTime() + 2.0;
                menu.templateInputBuffer[0] = '\0';
                menu.showTemplateDialog = false;
            } else if (result > 0) {
                menu.templateInputBuffer[0] = '\0';
                menu.showTemplateDialog = false;
            }
        }

        // Tab width dialog
        if (menu.showTabWidthDialog) {
            raylib::Rectangle dialogRect = {
                static_cast<float>(raylib::GetScreenWidth() / 2 - 160),
                static_cast<float>(raylib::GetScreenHeight() / 2 - 90), 320,
                180};
            int result = win95::DrawInputDialog(
                dialogRect, "Tab Width", "Spaces per tab:",
                menu.tabWidthInputBuffer,
                static_cast<int>(sizeof(menu.tabWidthInputBuffer)));
            if (result == 0) {
                int width = std::atoi(menu.tabWidthInputBuffer);
                if (width >= 1 && width <= 16) {
                    doc.docSettings.tabWidth = width;
                    status::set(status, "Tab width set");
                } else {
                    status::set(status, "Tab width must be 1-16", true);
                }
                status.expiresAt = raylib::GetTime() + 2.0;
                menu.tabWidthInputBuffer[0] = '\0';
                menu.showTabWidthDialog = false;
            } else if (result > 0) {
                menu.tabWidthInputBuffer[0] = '\0';
                menu.showTabWidthDialog = false;
            }
        }

        // F1 to show help window
        if (IsKeyPressed(raylib::KEY_F1)) {
            menu.showHelpWindow = !menu.showHelpWindow;
            menu.helpScrollOffset = 0;
        }

        // Handle Help window (keybindings)
        if (menu.showHelpWindow) {
            drawHelpWindow(menu, layout);
        }
    }

    void drawHelpWindow(MenuComponent& menu, const LayoutComponent& layout) const {
        drawHelpWindowImpl(menu, layout);
    }

   private:
    void handleMenuAction(int menuResult, DocumentComponent& doc,
                          MenuComponent& menu, StatusComponent& status,
                          LayoutComponent& layout) const {
        handleMenuActionImpl(menuResult, doc, menu, status, layout);
    }
};

// Implementation of menu action handler (called by both EditorRenderSystem and MenuSystem)
inline void handleMenuActionImpl(int menuResult, DocumentComponent& doc,
                          MenuComponent& menu, StatusComponent& status,
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
                        ecs::status::set(
                            status,
                            "Opened: " + std::filesystem::path(path).filename().string());
                        status.expiresAt = raylib::GetTime() + 3.0;
                    } else {
                        ecs::status::set(status, "Open failed: " + result.error, true);
                        status.expiresAt = raylib::GetTime() + 3.0;
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
                        ecs::status::set(
                            status,
                            "Opened: " + std::filesystem::path(doc.defaultPath)
                                             .filename()
                                             .string());
                        status.expiresAt = raylib::GetTime() + 3.0;
                    } else {
                        ecs::status::set(status, "Open failed: " + result.error,
                                         true);
                        status.expiresAt = raylib::GetTime() + 3.0;
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
                        ecs::status::set(
                            status, "Saved: " + std::filesystem::path(savePath)
                                                    .filename()
                                                    .string());
                        status.expiresAt = raylib::GetTime() + 3.0;
                    } else {
                        ecs::status::set(status, "Save failed: " + result.error,
                                         true);
                        status.expiresAt = raylib::GetTime() + 3.0;
                    }
                } break;
                case 6:  // Export PDF
                {
                    std::filesystem::path basePath =
                        doc.filePath.empty() ? doc.defaultPath : doc.filePath;
                    basePath.replace_extension(".pdf");
                    auto result = exportDocumentPdf(doc.buffer, doc.docSettings,
                                                    basePath.string());
                    if (result.success) {
                        ecs::status::set(
                            status,
                            "Exported PDF: " + basePath.filename().string());
                    } else {
                        ecs::status::set(status, "Export PDF failed: " + result.error,
                                         true);
                    }
                    status.expiresAt = raylib::GetTime() + 3.0;
                } break;
                case 7:  // Export HTML
                {
                    std::filesystem::path basePath =
                        doc.filePath.empty() ? doc.defaultPath : doc.filePath;
                    basePath.replace_extension(".html");
                    auto result = exportDocumentHtml(doc.buffer, doc.docSettings,
                                                     basePath.string());
                    if (result.success) {
                        ecs::status::set(
                            status,
                            "Exported HTML: " + basePath.filename().string());
                    } else {
                        ecs::status::set(status, "Export HTML failed: " + result.error,
                                         true);
                    }
                    status.expiresAt = raylib::GetTime() + 3.0;
                } break;
                case 8:  // Export RTF
                {
                    std::filesystem::path basePath =
                        doc.filePath.empty() ? doc.defaultPath : doc.filePath;
                    basePath.replace_extension(".rtf");
                    auto result = exportDocumentRtf(doc.buffer, doc.docSettings,
                                                    basePath.string());
                    if (result.success) {
                        ecs::status::set(
                            status,
                            "Exported RTF: " + basePath.filename().string());
                    } else {
                        ecs::status::set(status, "Export RTF failed: " + result.error,
                                         true);
                    }
                    status.expiresAt = raylib::GetTime() + 3.0;
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
                        status::set(status, "Track Changes: On");
                    } else {
                        menu.menus[1].items[3].mark = win95::MenuMark::None;
                        status::set(status, "Track Changes: Off");
                    }
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 4:  // Accept All Changes
                    doc.revisions.clear();
                    doc.trackChangesBaseline = doc.buffer.getText();
                    status::set(status, "All changes accepted");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 5:  // Reject All Changes
                    if (!doc.trackChangesBaseline.empty()) {
                        doc.buffer.setText(doc.trackChangesBaseline);
                        doc.isDirty = true;
                    }
                    doc.revisions.clear();
                    status::set(status, "All changes rejected");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 7:  // Cut
                    if (doc.buffer.hasSelection()) {
                        std::string selected = doc.buffer.getSelectedText();
                        if (!selected.empty()) {
                            afterhours::clipboard::set_text(selected);
                            doc.buffer.deleteSelection();
                            doc.isDirty = true;
                        }
                    }
                    break;
                case 8:  // Copy
                    if (doc.buffer.hasSelection()) {
                        std::string selected = doc.buffer.getSelectedText();
                        if (!selected.empty()) {
                            afterhours::clipboard::set_text(selected);
                        }
                    }
                    break;
                case 9:  // Paste
                {
                    if (afterhours::clipboard::has_text()) {
                        std::string clipText = afterhours::clipboard::get_text();
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
                    status::set(status, "Find: Ctrl+G next, Ctrl+Shift+G prev");
                    status.expiresAt = raylib::GetTime() + 3.0;
                    break;
                case 14:  // Find Next
                    if (!menu.lastSearchTerm.empty()) {
                        FindResult result = doc.buffer.findNext(menu.lastSearchTerm, menu.findOptions);
                        if (result.found) {
                            doc.buffer.setCaret(result.start);
                            doc.buffer.setSelectionAnchor(result.start);
                            doc.buffer.setCaret(result.end);
                            doc.buffer.updateSelectionToCaret();
                            status::set(status, "Found");
                        } else {
                            status::set(status, "Not found");
                        }
                        status.expiresAt = raylib::GetTime() + 2.0;
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
                            status::set(status, "Found");
                        } else {
                            status::set(status, "Not found");
                        }
                        status.expiresAt = raylib::GetTime() + 2.0;
                    }
                    break;
                case 16:  // Replace...
                    menu.showFindDialog = true;
                    menu.findReplaceMode = true;
                    status::set(status, "Replace mode");
                    status.expiresAt = raylib::GetTime() + 2.0;
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
                    status::set(status, "Switched to Pageless mode");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 1:  // Paged Mode
                    layout.pageMode = PageMode::Paged;
                    layout::updateLayout(layout, layout.screenWidth,
                                         layout.screenHeight);
                    status::set(status, "Switched to Paged mode");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 3:  // Zoom In
                    layout.zoomLevel = std::min(4.0f, layout.zoomLevel + 0.1f);
                    status::set(status, "Zoom in");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 4:  // Zoom Out
                    layout.zoomLevel = std::max(0.5f, layout.zoomLevel - 0.1f);
                    status::set(status, "Zoom out");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 5:  // Zoom Reset
                    layout.zoomLevel = 1.0f;
                    status::set(status, "Zoom reset");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 7:  // Focus Mode
                    layout.focusMode = !layout.focusMode;
                    layout::updateLayout(layout, layout.screenWidth,
                                         layout.screenHeight);
                    status::set(status, layout.focusMode ? "Focus mode: On" : "Focus mode: Off");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 8:  // Split View
                    layout.splitViewEnabled = !layout.splitViewEnabled;
                    status::set(status, layout.splitViewEnabled ? "Split view: On" : "Split view: Off");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 9:  // Dark Mode
                    theme::applyDarkMode(!theme::DARK_MODE_ENABLED);
                    status::set(status, theme::DARK_MODE_ENABLED ? "Dark mode: On" : "Dark mode: Off");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 11:  // Line Width: Normal (no limit)
                    layout::setLineWidthLimit(layout, 0.0f);
                    status::set(status, "Line width: Normal");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 12:  // Line Width: Narrow (60 chars)
                    layout::setLineWidthLimit(layout, 60.0f);
                    status::set(status, "Line width: Narrow (60 chars)");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 13:  // Line Width: Wide (100 chars)
                    layout::setLineWidthLimit(layout, 100.0f);
                    status::set(status, "Line width: Wide (100 chars)");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 15:  // Show Line Numbers
                    layout.showLineNumbers = !layout.showLineNumbers;
                    status::set(status, layout.showLineNumbers ? "Line numbers: On" : "Line numbers: Off");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                default:
                    break;
            }
        } else if (menuIndex == 3) {  // Format menu
            TextStyle style = doc.buffer.textStyle();
            switch (itemIndex) {
                // Paragraph styles (0-8)
                case 0:  // Normal
                    doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Normal);
                    status::set(status, "Style: Normal");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 1:  // Title
                    doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Title);
                    status::set(status, "Style: Title");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 2:  // Subtitle
                    doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Subtitle);
                    status::set(status, "Style: Subtitle");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 3:  // Heading 1
                    doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Heading1);
                    status::set(status, "Style: Heading 1");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 4:  // Heading 2
                    doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Heading2);
                    status::set(status, "Style: Heading 2");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 5:  // Heading 3
                    doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Heading3);
                    status::set(status, "Style: Heading 3");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 6:  // Heading 4
                    doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Heading4);
                    status::set(status, "Style: Heading 4");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 7:  // Heading 5
                    doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Heading5);
                    status::set(status, "Style: Heading 5");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 8:  // Heading 6
                    doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Heading6);
                    status::set(status, "Style: Heading 6");
                    status.expiresAt = raylib::GetTime() + 2.0;
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
                    status::set(status, "Align: Left");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 21:  // Align Center
                    doc.buffer.setCurrentAlignment(TextAlignment::Center);
                    status::set(status, "Align: Center");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 22:  // Align Right
                    doc.buffer.setCurrentAlignment(TextAlignment::Right);
                    status::set(status, "Align: Right");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 23:  // Justify
                    doc.buffer.setCurrentAlignment(TextAlignment::Justify);
                    status::set(status, "Align: Justify");
                    status.expiresAt = raylib::GetTime() + 2.0;
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
                    status::set(status, "Align: Left");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 48:  // Align Center
                    doc.buffer.setCurrentAlignment(TextAlignment::Center);
                    status::set(status, "Align: Center");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 49:  // Align Right
                    doc.buffer.setCurrentAlignment(TextAlignment::Right);
                    status::set(status, "Align: Right");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 50:  // Justify
                    doc.buffer.setCurrentAlignment(TextAlignment::Justify);
                    status::set(status, "Align: Justify");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                // (46 is separator)
                case 52:  // Increase Indent
                    doc.buffer.increaseIndent();
                    status::set(status, "Indent increased");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 53:  // Decrease Indent
                    doc.buffer.decreaseIndent();
                    status::set(status, "Indent decreased");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                // (54 is separator)
                case 55:  // Single Spacing
                    doc.buffer.setLineSpacingSingle();
                    status::set(status, "Line spacing: Single");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 56:  // 1.5 Line Spacing
                    doc.buffer.setLineSpacing1_5();
                    status::set(status, "Line spacing: 1.5");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 57:  // Double Spacing
                    doc.buffer.setLineSpacingDouble();
                    status::set(status, "Line spacing: Double");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                // (58 is separator)
                case 59:  // Bulleted List
                    doc.buffer.toggleBulletedList();
                    status::set(status, doc.buffer.currentListType() == ListType::Bulleted ? "Bullets on" : "Bullets off");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 60:  // Numbered List
                    doc.buffer.toggleNumberedList();
                    status::set(status, doc.buffer.currentListType() == ListType::Numbered ? "Numbering on" : "Numbering off");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 61:  // Increase List Level
                    doc.buffer.increaseListLevel();
                    status::set(status, "List level increased");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 62:  // Decrease List Level
                    doc.buffer.decreaseListLevel();
                    status::set(status, "List level decreased");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 64:  // Increase Space Before
                    doc.buffer.setCurrentSpaceBefore(doc.buffer.currentSpaceBefore() + 6);
                    status::set(status, "Space before increased");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 65:  // Decrease Space Before
                    doc.buffer.setCurrentSpaceBefore(doc.buffer.currentSpaceBefore() - 6);
                    status::set(status, "Space before decreased");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 66:  // Increase Space After
                    doc.buffer.setCurrentSpaceAfter(doc.buffer.currentSpaceAfter() + 6);
                    status::set(status, "Space after increased");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 67:  // Decrease Space After
                    doc.buffer.setCurrentSpaceAfter(doc.buffer.currentSpaceAfter() - 6);
                    status::set(status, "Space after decreased");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 69:  // Drop Cap
                    doc.buffer.toggleCurrentLineDropCap();
                    status::set(status, doc.buffer.currentLineHasDropCap() ? "Drop cap: On" : "Drop cap: Off");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 70:  // Tab Width...
                    menu.showTabWidthDialog = true;
                    break;
                default:
                    break;
            }
        } else if (menuIndex == 4) {  // Insert menu
            switch (itemIndex) {
                case 0:  // Page Break
                    doc.buffer.insertPageBreak();
                    doc.isDirty = true;
                    status::set(status, "Page break inserted");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 1:  // Section Break
                    doc.buffer.insertSectionBreak();
                    doc.isDirty = true;
                    status::set(status, "Section break inserted");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 3:  // Hyperlink...
                    // For now, just add hyperlink to selection if any
                    if (doc.buffer.hasSelection()) {
                        // Would need a dialog for URL input - placeholder for now
                        if (doc.buffer.addHyperlink("https://example.com")) {
                            doc.isDirty = true;
                            status::set(status, "Hyperlink added (edit URL)");
                        }
                    } else {
                        status::set(status, "Select text first", true);
                    }
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 4:  // Remove Hyperlink
                    if (doc.buffer.hyperlinkAtCaret()) {
                        std::size_t caretOffset = doc.buffer.caretOffset();
                        if (doc.buffer.removeHyperlink(caretOffset)) {
                            doc.isDirty = true;
                            status::set(status, "Hyperlink removed");
                        }
                    } else {
                        status::set(status, "No hyperlink at cursor", true);
                    }
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 5:  // Bookmark...
                {
                    std::string name =
                        std::format("bookmark_{}", doc.buffer.caret().row + 1);
                    if (doc.buffer.addBookmark(name)) {
                        status::set(status, "Bookmark added");
                    } else {
                        status::set(status, "Bookmark not added", true);
                    }
                    status.expiresAt = raylib::GetTime() + 2.0;
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
                        status::set(status, "Select text to comment", true);
                        status.expiresAt = raylib::GetTime() + 2.0;
                    }
                    break;
                case 8:  // Table...
                {
                    std::size_t currentLine = doc.buffer.caret().row;
                    doc.insertTable(currentLine, 3, 3);
                    doc.isDirty = true;
                    status::set(status, "Inserted 3x3 table");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                }
                case 10:  // Image...
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
                    status::set(status, "Image placeholder inserted");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                }
                default:
                    break;
            }
        } else if (menuIndex == 5) {  // Table menu
            switch (itemIndex) {
                case 0: {  // Insert Table...
                    // Insert a default 3x3 table at current line
                    std::size_t currentLine = doc.buffer.caret().row;
                    doc.insertTable(currentLine, 3, 3);
                    doc.isDirty = true;
                    status::set(status, "Inserted 3x3 table");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                }
                case 2: {  // Insert Row Above
                    std::size_t currentLine = doc.buffer.caret().row;
                    Table* table = doc.tableAtLine(currentLine);
                    if (table) {
                        table->insertRowAbove(table->currentCell().row);
                        doc.isDirty = true;
                        status::set(status, "Row inserted above");
                    } else {
                        status::set(status, "No table at cursor", true);
                    }
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                }
                case 3: {  // Insert Row Below
                    std::size_t currentLine = doc.buffer.caret().row;
                    Table* table = doc.tableAtLine(currentLine);
                    if (table) {
                        table->insertRowBelow(table->currentCell().row);
                        doc.isDirty = true;
                        status::set(status, "Row inserted below");
                    } else {
                        status::set(status, "No table at cursor", true);
                    }
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                }
                case 4: {  // Insert Column Left
                    std::size_t currentLine = doc.buffer.caret().row;
                    Table* table = doc.tableAtLine(currentLine);
                    if (table) {
                        table->insertColumnLeft(table->currentCell().col);
                        doc.isDirty = true;
                        status::set(status, "Column inserted left");
                    } else {
                        status::set(status, "No table at cursor", true);
                    }
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                }
                case 5: {  // Insert Column Right
                    std::size_t currentLine = doc.buffer.caret().row;
                    Table* table = doc.tableAtLine(currentLine);
                    if (table) {
                        table->insertColumnRight(table->currentCell().col);
                        doc.isDirty = true;
                        status::set(status, "Column inserted right");
                    } else {
                        status::set(status, "No table at cursor", true);
                    }
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                }
                case 7: {  // Delete Row
                    std::size_t currentLine = doc.buffer.caret().row;
                    Table* table = doc.tableAtLine(currentLine);
                    if (table) {
                        table->deleteRow(table->currentCell().row);
                        doc.isDirty = true;
                        status::set(status, "Row deleted");
                    } else {
                        status::set(status, "No table at cursor", true);
                    }
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                }
                case 8: {  // Delete Column
                    std::size_t currentLine = doc.buffer.caret().row;
                    Table* table = doc.tableAtLine(currentLine);
                    if (table) {
                        table->deleteColumn(table->currentCell().col);
                        doc.isDirty = true;
                        status::set(status, "Column deleted");
                    } else {
                        status::set(status, "No table at cursor", true);
                    }
                    status.expiresAt = raylib::GetTime() + 2.0;
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
                            status::set(status, "Cells merged");
                        } else {
                            status::set(status, "Cannot merge cells", true);
                        }
                    } else {
                        status::set(status, "Select cells to merge", true);
                    }
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                }
                case 11: {  // Split Cell
                    std::size_t currentLine = doc.buffer.caret().row;
                    Table* table = doc.tableAtLine(currentLine);
                    if (table) {
                        if (table->splitCell(table->currentCell())) {
                            doc.isDirty = true;
                            status::set(status, "Cell split");
                        } else {
                            status::set(status, "Cell is not merged", true);
                        }
                    } else {
                        status::set(status, "No table at cursor", true);
                    }
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                }
                default:
                    break;
            }
        } else if (menuIndex == 6) {  // Help menu
            if (itemIndex == 0) {     // Keyboard Shortcuts
                menu.showHelpWindow = true;
            } else if (itemIndex == 2) {  // About (after separator)
                menu.showAboutDialog = true;
            }
        } else if (menuIndex == 7) {  // Tools menu
            if (itemIndex == 0) {
                menu.showWordCountDialog = true;
            }
        }
    }

// Implementation of help window drawing
inline void drawHelpWindowImpl(MenuComponent& menu, const LayoutComponent& layout) {
    float windowWidth = 400.0f;
    float windowHeight = 400.0f;
    raylib::Rectangle dialogRect = {
        static_cast<float>(layout.screenWidth / 2) - windowWidth / 2,
        static_cast<float>(layout.screenHeight / 2) - windowHeight / 2,
        windowWidth, windowHeight};

    // Draw window background with raised border
    raylib::DrawRectangleRec(dialogRect, theme::WINDOW_BG);
    win95::DrawRaisedBorder(dialogRect);

    // Draw title bar
    raylib::Rectangle titleBar = {dialogRect.x + 2, dialogRect.y + 2,
                                  windowWidth - 4, 20};
    raylib::DrawRectangleRec(titleBar, theme::TITLE_BAR);
    raylib::DrawText("Keyboard Shortcuts", static_cast<int>(titleBar.x + 4),
                     static_cast<int>(titleBar.y + 3), 14,
                     theme::TITLE_TEXT);

    // Draw close button
    raylib::Rectangle closeBtn = {titleBar.x + titleBar.width - 18,
                                  titleBar.y + 2, 16, 16};
    win95::DrawRaisedBorder(closeBtn);
    raylib::DrawText("X", static_cast<int>(closeBtn.x + 4),
                     static_cast<int>(closeBtn.y + 2), 12,
                     theme::TEXT_COLOR);

    // Handle close button click
    if (IsMouseButtonPressed(raylib::MOUSE_BUTTON_LEFT)) {
        raylib::Vector2 mousePos = raylib::GetMousePosition();
        if (mousePos.x >= closeBtn.x &&
            mousePos.x <= closeBtn.x + closeBtn.width &&
            mousePos.y >= closeBtn.y &&
            mousePos.y <= closeBtn.y + closeBtn.height) {
            menu.showHelpWindow = false;
            return;
        }
    }

    // Escape to close
    if (IsKeyPressed(raylib::KEY_ESCAPE)) {
        menu.showHelpWindow = false;
        return;
    }

    // Draw content area with sunken border
    raylib::Rectangle contentArea = {dialogRect.x + 8, dialogRect.y + 28,
                                     windowWidth - 16, windowHeight - 64};
    raylib::DrawRectangleRec(contentArea, raylib::WHITE);
    win95::DrawSunkenBorder(contentArea);

    // Get keybindings
    input::ActionMap defaultMap = input::createDefaultActionMap();
    auto bindings = input::getBindingsList(defaultMap);

    // Handle scrolling
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        menu.helpScrollOffset -= static_cast<int>(wheel * 3);
        if (menu.helpScrollOffset < 0) menu.helpScrollOffset = 0;
        int maxScroll = static_cast<int>(bindings.size()) - 15;
        if (maxScroll < 0) maxScroll = 0;
        if (menu.helpScrollOffset > maxScroll)
            menu.helpScrollOffset = maxScroll;
    }

    // Draw keybindings list
    int lineHeight = 18;
    int y = static_cast<int>(contentArea.y) + 4;
    int visibleLines =
        static_cast<int>((contentArea.height - 8) / lineHeight);

    // Headers
    raylib::DrawText("Action", static_cast<int>(contentArea.x) + 8, y, 12,
                     raylib::DARKGRAY);
    raylib::DrawText("Shortcut", static_cast<int>(contentArea.x) + 200, y,
                     12, raylib::DARKGRAY);
    y += lineHeight;

    // Separator line
    raylib::DrawLine(
        static_cast<int>(contentArea.x) + 4, y,
        static_cast<int>(contentArea.x + contentArea.width) - 4, y,
        raylib::LIGHTGRAY);
    y += 4;

    // Draw bindings
    int startIdx = menu.helpScrollOffset;
    int endIdx = std::min(static_cast<int>(bindings.size()),
                          startIdx + visibleLines - 2);

    for (int i = startIdx; i < endIdx; ++i) {
        const auto& binding = bindings[static_cast<size_t>(i)];
        raylib::DrawText(binding.actionName.c_str(),
                         static_cast<int>(contentArea.x) + 8, y, 12,
                         theme::TEXT_COLOR);
        raylib::DrawText(binding.bindingStr.c_str(),
                         static_cast<int>(contentArea.x) + 200, y, 12,
                         theme::TEXT_COLOR);
        y += lineHeight;
    }

    // Draw OK button
    raylib::Rectangle okBtn = {dialogRect.x + windowWidth / 2 - 40,
                               dialogRect.y + windowHeight - 30, 80, 22};
    win95::DrawRaisedBorder(okBtn);
    raylib::DrawText("OK", static_cast<int>(okBtn.x + 30),
                     static_cast<int>(okBtn.y + 4), 14, theme::TEXT_COLOR);

    // Handle OK button click
    if (IsMouseButtonPressed(raylib::MOUSE_BUTTON_LEFT)) {
        raylib::Vector2 mousePos = raylib::GetMousePosition();
        if (mousePos.x >= okBtn.x && mousePos.x <= okBtn.x + okBtn.width &&
            mousePos.y >= okBtn.y && mousePos.y <= okBtn.y + okBtn.height) {
            menu.showHelpWindow = false;
        }
    }
}

}  // namespace ecs
