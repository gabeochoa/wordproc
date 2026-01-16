#pragma once

#include <filesystem>
#include <format>

#include "../../vendor/afterhours/src/core/system.h"
#include "../editor/document_io.h"
#include "../editor/table.h"
#include "../input/action_map.h"
#include "../rl.h"
#include "../ui/theme.h"
#include "../ui/win95_widgets.h"
#include "../util/drawing.h"
#include "../util/logging.h"
#include "component_helpers.h"
#include "components.h"

namespace ecs {

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

// Render a single image with its layout mode
inline void renderImage(const DocumentImage& image, float anchorX, float anchorY,
                        bool isSelected = false) {
    // For now, we draw a placeholder rectangle since we don't have actual image loading
    // In a full implementation, this would load and render the actual image texture
    
    DocumentImage::Bounds bounds = image.getBounds(anchorX, anchorY);
    
    // Draw border if present
    if (image.borderWidth > 0.0f) {
        raylib::Color borderColor = {image.borderR, image.borderG, image.borderB, image.borderA};
        raylib::DrawRectangleLinesEx(
            {bounds.x, bounds.y, bounds.width, bounds.height},
            image.borderWidth, borderColor);
    }
    
    // Draw image placeholder (light gray with image icon)
    float imgX = bounds.x + image.marginLeft + image.borderWidth;
    float imgY = bounds.y + image.marginTop + image.borderWidth;
    raylib::Color placeholderColor = {230, 230, 230, 255};
    raylib::DrawRectangle(static_cast<int>(imgX), static_cast<int>(imgY),
                         static_cast<int>(image.displayWidth), 
                         static_cast<int>(image.displayHeight), placeholderColor);
    
    // Draw a simple image icon in the center
    float iconSize = std::min(image.displayWidth, image.displayHeight) * 0.3f;
    float iconX = imgX + (image.displayWidth - iconSize) / 2.0f;
    float iconY = imgY + (image.displayHeight - iconSize) / 2.0f;
    
    // Draw image icon (simplified mountain/sun icon)
    raylib::Color iconColor = {150, 150, 150, 255};
    // Sun (circle in upper right)
    float sunX = iconX + iconSize * 0.7f;
    float sunY = iconY + iconSize * 0.3f;
    raylib::DrawCircle(static_cast<int>(sunX), static_cast<int>(sunY), 
                      iconSize * 0.15f, iconColor);
    // Mountain (triangle)
    raylib::Vector2 v1 = {iconX + iconSize * 0.2f, iconY + iconSize * 0.9f};
    raylib::Vector2 v2 = {iconX + iconSize * 0.5f, iconY + iconSize * 0.3f};
    raylib::Vector2 v3 = {iconX + iconSize * 0.8f, iconY + iconSize * 0.9f};
    raylib::DrawTriangle(v1, v2, v3, iconColor);
    
    // Draw filename/alt text below image if present
    if (!image.filename.empty() && image.displayHeight > 30.0f) {
        int fontSize = 10;
        std::string label = image.filename;
        if (label.length() > 20) {
            label = label.substr(0, 17) + "...";
        }
        int textWidth = raylib::MeasureText(label.c_str(), fontSize);
        float textX = imgX + (image.displayWidth - textWidth) / 2.0f;
        float textY = imgY + image.displayHeight - fontSize - 4.0f;
        raylib::DrawText(label.c_str(), static_cast<int>(textX), 
                        static_cast<int>(textY), fontSize, iconColor);
    }
    
    // Draw selection highlight
    if (isSelected) {
        raylib::DrawRectangleLinesEx(
            {imgX - 2, imgY - 2, image.displayWidth + 4, image.displayHeight + 4},
            2.0f, raylib::Color{0, 120, 215, 255});
        
        // Draw resize handles at corners
        float handleSize = 6.0f;
        raylib::Color handleColor = {0, 120, 215, 255};
        // Top-left
        raylib::DrawRectangle(static_cast<int>(imgX - handleSize/2), 
                             static_cast<int>(imgY - handleSize/2),
                             static_cast<int>(handleSize), static_cast<int>(handleSize), handleColor);
        // Top-right
        raylib::DrawRectangle(static_cast<int>(imgX + image.displayWidth - handleSize/2),
                             static_cast<int>(imgY - handleSize/2),
                             static_cast<int>(handleSize), static_cast<int>(handleSize), handleColor);
        // Bottom-left
        raylib::DrawRectangle(static_cast<int>(imgX - handleSize/2),
                             static_cast<int>(imgY + image.displayHeight - handleSize/2),
                             static_cast<int>(handleSize), static_cast<int>(handleSize), handleColor);
        // Bottom-right
        raylib::DrawRectangle(static_cast<int>(imgX + image.displayWidth - handleSize/2),
                             static_cast<int>(imgY + image.displayHeight - handleSize/2),
                             static_cast<int>(handleSize), static_cast<int>(handleSize), handleColor);
    }
}

// Render all images in a document at their anchor positions
inline void renderDocumentImages(const ImageCollection& images,
                                 const LayoutComponent::Rect& textArea,
                                 int baseLineHeight, int scrollOffset,
                                 std::size_t selectedImageId = 0) {
    for (const auto& image : images.images()) {
        // Calculate anchor Y position based on line number
        if (image.anchorLine < static_cast<std::size_t>(scrollOffset)) continue;
        
        float y = textArea.y + theme::layout::TEXT_PADDING +
                  static_cast<float>(image.anchorLine - scrollOffset) * baseLineHeight;
        float x = textArea.x + theme::layout::TEXT_PADDING;
        
        // For non-inline images, apply alignment
        if (image.layoutMode != ImageLayoutMode::Inline) {
            float availableWidth = textArea.width - 2 * theme::layout::TEXT_PADDING;
            switch (image.alignment) {
                case ImageAlignment::Left:
                    // Already at left
                    break;
                case ImageAlignment::Center:
                    x = textArea.x + (textArea.width - image.displayWidth) / 2.0f;
                    break;
                case ImageAlignment::Right:
                    x = textArea.x + availableWidth - image.displayWidth - theme::layout::TEXT_PADDING;
                    break;
            }
        }
        
        // Check if image is visible
        if (y > textArea.y + textArea.height) continue;
        
        bool isSelected = (image.id == selectedImageId);
        renderImage(image, x, y, isSelected);
    }
}

// Render the text buffer with caret and selection
// Now supports per-line paragraph styles (H1-H6, Title, Subtitle)
// showLineNumbers: if true, draws line numbers in a gutter on the left
inline void renderTextBuffer(const TextBuffer& buffer,
                             const LayoutComponent::Rect& textArea,
                             bool caretVisible, int baseFontSize, int baseLineHeight,
                             int scrollOffset, bool showLineNumbers = false,
                             float lineNumberGutterWidth = 50.0f) {
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
        
        // Get paragraph style for this line
        ParagraphStyle paraStyle = buffer.lineParagraphStyle(row);
        int lineFontSize = paragraphStyleFontSize(paraStyle);
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
        int textWidth = line.empty() ? 0 : raylib::MeasureText(line.c_str(), lineFontSize);
        
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
                    std::string beforeSel = line.substr(0, startCol);
                    std::string selectedText =
                        line.substr(startCol, endCol - startCol);

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
            
            // For headings and titles, draw bold text (simulated by drawing twice with offset)
            if (paragraphStyleIsBold(paraStyle) || globalStyle.bold) {
                // Draw bold effect by drawing text twice with 1px offset
                raylib::DrawText(line.c_str(), x, y, lineFontSize, textColor);
                raylib::DrawText(line.c_str(), x + 1, y, lineFontSize, textColor);
            } else if (paragraphStyleIsItalic(paraStyle) || globalStyle.italic) {
                // For subtitle italic style, draw in a slightly different shade
                raylib::Color italicColor = {static_cast<unsigned char>(textColor.r / 2 + 64),
                                             static_cast<unsigned char>(textColor.g / 2 + 64),
                                             static_cast<unsigned char>(textColor.b / 2 + 64), textColor.a};
                raylib::DrawText(line.c_str(), x, y, lineFontSize, italicColor);
            } else {
                raylib::DrawText(line.c_str(), x, y, lineFontSize, textColor);
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
                line.substr(0, std::min(caret.column, line.length()));
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

// System for rendering the complete editor UI
struct EditorRenderSystem
    : public afterhours::System<DocumentComponent, CaretComponent,
                                ScrollComponent, StatusComponent,
                                LayoutComponent, MenuComponent> {
    void once(const float) const override {
        raylib::BeginDrawing();
        raylib::ClearBackground(theme::WINDOW_BG);
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
        raylib::DrawText(title.c_str(), 4, 4, theme::layout::FONT_SIZE,
                         theme::TITLE_TEXT);

        // Draw menu bar background
        raylib::Rectangle menuBarRect = {layout.menuBar.x, layout.menuBar.y,
                                         layout.menuBar.width,
                                         layout.menuBar.height};
        raylib::DrawRectangleRec(menuBarRect, theme::WINDOW_BG);
        util::drawRaisedBorder(menuBarRect);

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
        int fontSize = style.fontSize;
        int lineHeight = fontSize + 4;
        LayoutComponent::Rect effectiveArea = layout::effectiveTextArea(layout);
        renderTextBuffer(doc.buffer, effectiveArea, caret.visible, fontSize,
                         lineHeight, scroll.offset, layout.showLineNumbers,
                         layout.lineNumberGutterWidth);

        // Draw status bar
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
            std::string statusText = std::format(
                "Ln {}, Col {} | {} | {}{}{}{}| {}pt | {}", caretPos.row + 1,
                caretPos.column + 1, paragraphStyleName(paraStyle),
                style.bold ? "B " : "",
                style.italic ? "I " : "",
                style.underline ? "U " : "",
                style.strikethrough ? "S " : "",
                style.fontSize, style.font);
            raylib::DrawText(
                statusText.c_str(), 4,
                layout.screenHeight - theme::layout::STATUS_BAR_HEIGHT + 2,
                theme::layout::FONT_SIZE - 2, theme::TEXT_COLOR);
        }

        // Draw About dialog if active
        if (menu.showAboutDialog) {
            raylib::Rectangle dialogRect = {
                static_cast<float>(layout.screenWidth / 2 - 150),
                static_cast<float>(layout.screenHeight / 2 - 75), 300, 150};
            win95::DrawMessageDialog(dialogRect, "About Wordproc",
                                     "Wordproc v0.1\n\nA Windows 95 style word "
                                     "processor\nbuilt with Afterhours.",
                                     false);
        }
    }
};

// System for handling menu interactions (needs mutable access)
struct MenuSystem
    : public afterhours::System<DocumentComponent, MenuComponent,
                                StatusComponent, LayoutComponent> {
    void for_each_with(afterhours::Entity& /*entity*/, DocumentComponent& doc,
                       MenuComponent& menu, StatusComponent& status,
                       LayoutComponent& layout, const float) override {
        // Draw interactive menus
        int menuResult =
            win95::DrawMenuBar(menu.menus, theme::layout::TITLE_BAR_HEIGHT,
                               theme::layout::MENU_BAR_HEIGHT);

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

        // F1 to show help window
        if (raylib::IsKeyPressed(raylib::KEY_F1)) {
            menu.showHelpWindow = !menu.showHelpWindow;
            menu.helpScrollOffset = 0;
        }

        // Handle Help window (keybindings)
        if (menu.showHelpWindow) {
            drawHelpWindow(menu, layout);
        }
    }

    void drawHelpWindow(MenuComponent& menu, const LayoutComponent& layout) {
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
        if (raylib::IsMouseButtonPressed(raylib::MOUSE_BUTTON_LEFT)) {
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
        if (raylib::IsKeyPressed(raylib::KEY_ESCAPE)) {
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
        float wheel = raylib::GetMouseWheelMove();
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
        if (raylib::IsMouseButtonPressed(raylib::MOUSE_BUTTON_LEFT)) {
            raylib::Vector2 mousePos = raylib::GetMousePosition();
            if (mousePos.x >= okBtn.x && mousePos.x <= okBtn.x + okBtn.width &&
                mousePos.y >= okBtn.y && mousePos.y <= okBtn.y + okBtn.height) {
                menu.showHelpWindow = false;
            }
        }
    }

   private:
    void handleMenuAction(int menuResult, DocumentComponent& doc,
                          MenuComponent& menu, StatusComponent& status,
                          LayoutComponent& layout) {
        int menuIndex = menuResult / 100;
        int itemIndex = menuResult % 100;

        if (menuIndex == 0) {  // File menu
            switch (itemIndex) {
                case 0:  // New
                    doc.buffer.setText("");
                    doc.filePath.clear();
                    doc.isDirty = false;
                    break;
                case 1:  // Open
                {
                    // Load document with settings (document settings saved with
                    // file)
                    auto result = loadDocumentEx(doc.buffer, doc.docSettings,
                                                 doc.defaultPath);
                    if (result.success) {
                        doc.filePath = doc.defaultPath;
                        doc.isDirty = false;
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
                case 2:  // Save
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
                case 5:  // Exit
                    break;
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
                case 3:  // Cut
                    if (doc.buffer.hasSelection()) {
                        std::string selected = doc.buffer.getSelectedText();
                        if (!selected.empty()) {
                            raylib::SetClipboardText(selected.c_str());
                            doc.buffer.deleteSelection();
                            doc.isDirty = true;
                        }
                    }
                    break;
                case 4:  // Copy
                    if (doc.buffer.hasSelection()) {
                        std::string selected = doc.buffer.getSelectedText();
                        if (!selected.empty()) {
                            raylib::SetClipboardText(selected.c_str());
                        }
                    }
                    break;
                case 5:  // Paste
                {
                    const char* clipText = raylib::GetClipboardText();
                    if (clipText && clipText[0] != '\0') {
                        doc.buffer.insertText(clipText);
                        doc.isDirty = true;
                    }
                } break;
                case 7:  // Select All
                    doc.buffer.selectAll();
                    break;
                case 9:  // Find...
                    menu.showFindDialog = true;
                    menu.findReplaceMode = false;
                    status::set(status, "Find: Ctrl+G next, Ctrl+Shift+G prev");
                    status.expiresAt = raylib::GetTime() + 3.0;
                    break;
                case 10:  // Find Next
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
                case 11:  // Find Previous
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
                case 12:  // Replace...
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
                case 3:  // Line Width: Normal (no limit)
                    layout::setLineWidthLimit(layout, 0.0f);
                    status::set(status, "Line width: Normal");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 4:  // Line Width: Narrow (60 chars)
                    layout::setLineWidthLimit(layout, 60.0f);
                    status::set(status, "Line width: Narrow (60 chars)");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 5:  // Line Width: Wide (100 chars)
                    layout::setLineWidthLimit(layout, 100.0f);
                    status::set(status, "Line width: Wide (100 chars)");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 7:  // Show Line Numbers
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
                // (14 is separator)
                // Alignment (15-18)
                case 15:  // Align Left
                    doc.buffer.setCurrentAlignment(TextAlignment::Left);
                    status::set(status, "Align: Left");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 16:  // Align Center
                    doc.buffer.setCurrentAlignment(TextAlignment::Center);
                    status::set(status, "Align: Center");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 17:  // Align Right
                    doc.buffer.setCurrentAlignment(TextAlignment::Right);
                    status::set(status, "Align: Right");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 18:  // Justify
                    doc.buffer.setCurrentAlignment(TextAlignment::Justify);
                    status::set(status, "Align: Justify");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                // (19 is separator)
                // Text colors (20-26)
                case 20:  // Text: Black
                    style.textColor = TextColors::Black;
                    doc.buffer.setTextStyle(style);
                    break;
                case 21:  // Text: Red
                    style.textColor = TextColors::Red;
                    doc.buffer.setTextStyle(style);
                    break;
                case 22:  // Text: Orange
                    style.textColor = TextColors::Orange;
                    doc.buffer.setTextStyle(style);
                    break;
                case 23:  // Text: Green
                    style.textColor = TextColors::Green;
                    doc.buffer.setTextStyle(style);
                    break;
                case 24:  // Text: Blue
                    style.textColor = TextColors::Blue;
                    doc.buffer.setTextStyle(style);
                    break;
                case 25:  // Text: Purple
                    style.textColor = TextColors::Purple;
                    doc.buffer.setTextStyle(style);
                    break;
                case 26:  // Text: Gray
                    style.textColor = TextColors::Gray;
                    doc.buffer.setTextStyle(style);
                    break;
                // (27 is separator)
                // Highlight colors (28-33)
                case 28:  // Highlight: None
                    style.highlightColor = HighlightColors::None;
                    doc.buffer.setTextStyle(style);
                    break;
                case 29:  // Highlight: Yellow
                    style.highlightColor = HighlightColors::Yellow;
                    doc.buffer.setTextStyle(style);
                    break;
                case 30:  // Highlight: Green
                    style.highlightColor = HighlightColors::Green;
                    doc.buffer.setTextStyle(style);
                    break;
                case 31:  // Highlight: Cyan
                    style.highlightColor = HighlightColors::Cyan;
                    doc.buffer.setTextStyle(style);
                    break;
                case 32:  // Highlight: Pink
                    style.highlightColor = HighlightColors::Pink;
                    doc.buffer.setTextStyle(style);
                    break;
                case 33:  // Highlight: Orange
                    style.highlightColor = HighlightColors::Orange;
                    doc.buffer.setTextStyle(style);
                    break;
                // (34 is separator)
                // Fonts (35-36)
                case 35:  // Font: Gaegu
                    style.font = "Gaegu-Bold";
                    doc.buffer.setTextStyle(style);
                    break;
                case 36:  // Font: Garamond
                    style.font = "EBGaramond-Regular";
                    doc.buffer.setTextStyle(style);
                    break;
                // (37 is separator)
                // Font size (38-40)
                case 38:  // Increase Size
                    style.fontSize = std::min(72, style.fontSize + 2);
                    doc.buffer.setTextStyle(style);
                    break;
                case 39:  // Decrease Size
                    style.fontSize = std::max(8, style.fontSize - 2);
                    doc.buffer.setTextStyle(style);
                    break;
                case 40:  // Reset Size
                    style.fontSize = 16;
                    doc.buffer.setTextStyle(style);
                    break;
                // (41 is separator)
                // Alignment (42-45)
                case 42:  // Align Left
                    doc.buffer.setCurrentAlignment(TextAlignment::Left);
                    status::set(status, "Align: Left");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 43:  // Align Center
                    doc.buffer.setCurrentAlignment(TextAlignment::Center);
                    status::set(status, "Align: Center");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 44:  // Align Right
                    doc.buffer.setCurrentAlignment(TextAlignment::Right);
                    status::set(status, "Align: Right");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 45:  // Justify
                    doc.buffer.setCurrentAlignment(TextAlignment::Justify);
                    status::set(status, "Align: Justify");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                // (46 is separator)
                case 47:  // Increase Indent
                    doc.buffer.increaseIndent();
                    status::set(status, "Indent increased");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 48:  // Decrease Indent
                    doc.buffer.decreaseIndent();
                    status::set(status, "Indent decreased");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                // (49 is separator)
                case 50:  // Single Spacing
                    doc.buffer.setLineSpacingSingle();
                    status::set(status, "Line spacing: Single");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 51:  // 1.5 Line Spacing
                    doc.buffer.setLineSpacing1_5();
                    status::set(status, "Line spacing: 1.5");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 52:  // Double Spacing
                    doc.buffer.setLineSpacingDouble();
                    status::set(status, "Line spacing: Double");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                // (53 is separator)
                case 54:  // Bulleted List
                    doc.buffer.toggleBulletedList();
                    status::set(status, doc.buffer.currentListType() == ListType::Bulleted ? "Bullets on" : "Bullets off");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 55:  // Numbered List
                    doc.buffer.toggleNumberedList();
                    status::set(status, doc.buffer.currentListType() == ListType::Numbered ? "Numbering on" : "Numbering off");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 56:  // Increase List Level
                    doc.buffer.increaseListLevel();
                    status::set(status, "List level increased");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 57:  // Decrease List Level
                    doc.buffer.decreaseListLevel();
                    status::set(status, "List level decreased");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                default:
                    break;
            }
        } else if (menuIndex == 4) {  // Insert menu
            switch (itemIndex) {
                case 0: {  // Insert Image...
                    // Create a placeholder image at the current line
                    DocumentImage img;
                    img.filename = "image.png";
                    img.anchorLine = doc.buffer.caret().row;
                    img.anchorColumn = doc.buffer.caret().column;
                    img.originalWidth = 200.0f;
                    img.originalHeight = 150.0f;
                    img.displayWidth = 200.0f;
                    img.displayHeight = 150.0f;
                    img.layoutMode = ImageLayoutMode::Inline;
                    doc.insertImage(img);
                    doc.isDirty = true;
                    status::set(status, "Image inserted");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                }
                case 2: {  // Insert Table (same as Table menu Insert Table)
                    std::size_t currentLine = doc.buffer.caret().row;
                    doc.insertTable(currentLine, 3, 3);
                    doc.isDirty = true;
                    status::set(status, "Inserted 3x3 table");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                }
                case 4:  // Page Break
                    doc.buffer.insertChar('\f');  // Form feed character as page break
                    doc.isDirty = true;
                    status::set(status, "Page break inserted");
                    status.expiresAt = raylib::GetTime() + 2.0;
                    break;
                case 5:  // Line Break (soft break, not paragraph)
                    doc.buffer.insertChar('\n');
                    doc.isDirty = true;
                    break;
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
        } else if (menuIndex == 5) {  // Help menu
            if (itemIndex == 0) {     // Keyboard Shortcuts
                menu.showHelpWindow = true;
            } else if (itemIndex == 2) {  // About (after separator)
                menu.showAboutDialog = true;
            }
        }
    }
};

}  // namespace ecs
