#pragma once

#include <filesystem>
#include <format>

#include "../../vendor/afterhours/src/core/system.h"
#include "../editor/document_io.h"
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

// Render the text buffer with caret and selection
// Now supports per-line paragraph styles (H1-H6, Title, Subtitle)
inline void renderTextBuffer(const TextBuffer& buffer,
                             const LayoutComponent::Rect& textArea,
                             bool caretVisible, int baseFontSize, int baseLineHeight,
                             int scrollOffset) {
    std::size_t lineCount = buffer.lineCount();
    CaretPosition caret = buffer.caret();
    bool hasSelection = buffer.hasSelection();
    CaretPosition selStart = buffer.selectionStart();
    CaretPosition selEnd = buffer.selectionEnd();

    int y = static_cast<int>(textArea.y) + theme::layout::TEXT_PADDING;

    std::size_t startRow = static_cast<std::size_t>(scrollOffset);
    if (startRow >= lineCount) startRow = lineCount > 0 ? lineCount - 1 : 0;

    for (std::size_t row = startRow; row < lineCount; ++row) {
        LineSpan span = buffer.lineSpan(row);
        int x = static_cast<int>(textArea.x) + theme::layout::TEXT_PADDING;

        std::string line = (span.length > 0) ? buffer.lineString(row) : "";
        
        // Get paragraph style for this line
        ParagraphStyle paraStyle = buffer.lineParagraphStyle(row);
        int lineFontSize = paragraphStyleFontSize(paraStyle);
        int lineHeight = lineFontSize + 4;
        
        // Use base font size as minimum if paragraph style would be smaller
        if (lineFontSize < baseFontSize && paraStyle == ParagraphStyle::Normal) {
            lineFontSize = baseFontSize;
            lineHeight = baseLineHeight;
        }

        // Draw selection highlight
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
            // For headings and titles, draw bold text (simulated by drawing twice with offset)
            if (paragraphStyleIsBold(paraStyle)) {
                // Draw bold effect by drawing text twice with 1px offset
                raylib::DrawText(line.c_str(), x, y, lineFontSize, theme::TEXT_COLOR);
                raylib::DrawText(line.c_str(), x + 1, y, lineFontSize, theme::TEXT_COLOR);
            } else if (paragraphStyleIsItalic(paraStyle)) {
                // For subtitle italic style, just draw normally for now
                // (true italics would require a separate font or skewing)
                raylib::DrawText(line.c_str(), x, y, lineFontSize, raylib::DARKGRAY);
            } else {
                raylib::DrawText(line.c_str(), x, y, lineFontSize, theme::TEXT_COLOR);
            }
        }

        // Draw caret
        if (caretVisible && row == caret.row) {
            std::string beforeCaret =
                line.substr(0, std::min(caret.column, line.length()));
            int caretX = x + raylib::MeasureText(beforeCaret.c_str(), lineFontSize);
            raylib::DrawRectangle(caretX, y, 2, lineHeight, theme::CARET_COLOR);
        }

        y += lineHeight;

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

    void for_each_with(const afterhours::Entity& entity,
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
                         lineHeight, scroll.offset);

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
                "Ln {}, Col {} | {} | {}{}| {}pt | {}", caretPos.row + 1,
                caretPos.column + 1, paragraphStyleName(paraStyle),
                style.bold ? "B " : "",
                style.italic ? "I " : "", style.fontSize, style.font);
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
    void for_each_with(afterhours::Entity& entity, DocumentComponent& doc,
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
                // (12 is separator)
                case 13:  // Font: Gaegu
                    style.font = "Gaegu-Bold";
                    doc.buffer.setTextStyle(style);
                    break;
                case 14:  // Font: Garamond
                    style.font = "EBGaramond-Regular";
                    doc.buffer.setTextStyle(style);
                    break;
                // (15 is separator)
                case 16:  // Increase Size
                    style.fontSize = std::min(72, style.fontSize + 2);
                    doc.buffer.setTextStyle(style);
                    break;
                case 17:  // Decrease Size
                    style.fontSize = std::max(8, style.fontSize - 2);
                    doc.buffer.setTextStyle(style);
                    break;
                case 18:  // Reset Size
                    style.fontSize = 16;
                    doc.buffer.setTextStyle(style);
                    break;
                default:
                    break;
            }
        } else if (menuIndex == 4) {  // Help menu
            if (itemIndex == 0) {     // Keyboard Shortcuts
                menu.showHelpWindow = true;
            } else if (itemIndex == 2) {  // About (after separator)
                menu.showAboutDialog = true;
            }
        }
    }
};

}  // namespace ecs
