#pragma once

#include "components.h"
#include "../rl.h"
#include "../ui/theme.h"
#include "../ui/win95_widgets.h"
#include "../util/drawing.h"
#include "../../vendor/afterhours/src/core/system.h"

#include <filesystem>
#include <format>

namespace ecs {

// Draw a page background with shadow (for paged mode)
inline void drawPageBackground(const LayoutComponent& layout) {
  if (layout.pageMode != PageMode::Paged) return;
  
  float pageY = layout.textArea.y + 10.0f;  // 10px margin from top
  
  // Draw page shadow
  raylib::Rectangle shadowRect = {
    layout.pageOffsetX + 4.0f,
    pageY + 4.0f,
    layout.pageDisplayWidth,
    layout.pageDisplayHeight
  };
  raylib::DrawRectangleRec(shadowRect, raylib::Color{100, 100, 100, 128});
  
  // Draw page (white background)
  raylib::Rectangle pageRect = {
    layout.pageOffsetX,
    pageY,
    layout.pageDisplayWidth,
    layout.pageDisplayHeight
  };
  raylib::DrawRectangleRec(pageRect, raylib::WHITE);
  
  // Draw page border
  raylib::DrawRectangleLinesEx(pageRect, 1.0f, raylib::DARKGRAY);
  
  // Draw margin guidelines (dotted or light lines)
  float marginScaled = layout.pageMargin * layout.pageScale;
  raylib::Color marginColor = raylib::Color{200, 200, 200, 100};
  
  // Left margin
  raylib::DrawLine(
    static_cast<int>(layout.pageOffsetX + marginScaled),
    static_cast<int>(pageY),
    static_cast<int>(layout.pageOffsetX + marginScaled),
    static_cast<int>(pageY + layout.pageDisplayHeight),
    marginColor
  );
  
  // Right margin
  raylib::DrawLine(
    static_cast<int>(layout.pageOffsetX + layout.pageDisplayWidth - marginScaled),
    static_cast<int>(pageY),
    static_cast<int>(layout.pageOffsetX + layout.pageDisplayWidth - marginScaled),
    static_cast<int>(pageY + layout.pageDisplayHeight),
    marginColor
  );
  
  // Top margin
  raylib::DrawLine(
    static_cast<int>(layout.pageOffsetX),
    static_cast<int>(pageY + marginScaled),
    static_cast<int>(layout.pageOffsetX + layout.pageDisplayWidth),
    static_cast<int>(pageY + marginScaled),
    marginColor
  );
  
  // Bottom margin
  raylib::DrawLine(
    static_cast<int>(layout.pageOffsetX),
    static_cast<int>(pageY + layout.pageDisplayHeight - marginScaled),
    static_cast<int>(layout.pageOffsetX + layout.pageDisplayWidth),
    static_cast<int>(pageY + layout.pageDisplayHeight - marginScaled),
    marginColor
  );
}

// Render the text buffer with caret and selection
inline void renderTextBuffer(const TextBuffer& buffer, 
                             const LayoutComponent::Rect& textArea,
                             bool caretVisible, int fontSize, int lineHeight,
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

    // Draw selection highlight
    if (hasSelection) {
      bool lineInSelection = (row >= selStart.row && row <= selEnd.row);
      if (lineInSelection) {
        std::size_t startCol = (row == selStart.row) ? selStart.column : 0;
        std::size_t endCol = (row == selEnd.row) ? selEnd.column : span.length;
        
        if (startCol < endCol && !line.empty()) {
          std::string beforeSel = line.substr(0, startCol);
          std::string selectedText = line.substr(startCol, endCol - startCol);
          
          int selX = x + raylib::MeasureText(beforeSel.c_str(), fontSize);
          int selWidth = raylib::MeasureText(selectedText.c_str(), fontSize);
          raylib::DrawRectangle(selX, y, selWidth, lineHeight, theme::SELECTION_BG);
        }
      }
    }

    // Draw text
    if (!line.empty()) {
      raylib::DrawText(line.c_str(), x, y, fontSize, theme::TEXT_COLOR);
    }

    // Draw caret
    if (caretVisible && row == caret.row) {
      std::string beforeCaret = line.substr(0, std::min(caret.column, line.length()));
      int caretX = x + raylib::MeasureText(beforeCaret.c_str(), fontSize);
      raylib::DrawRectangle(caretX, y, 2, lineHeight, theme::CARET_COLOR);
    }

    y += lineHeight;
    
    if (y > static_cast<int>(textArea.y + textArea.height)) {
      break;
    }
  }
}

// System for rendering the complete editor UI
struct EditorRenderSystem : public afterhours::System<DocumentComponent, CaretComponent, 
                                                       ScrollComponent, StatusComponent,
                                                       LayoutComponent, MenuComponent> {
  void once(const float) const override {
    raylib::BeginDrawing();
    raylib::ClearBackground(theme::WINDOW_BG);
  }
  
  void after(const float) const override {
    raylib::EndDrawing();
  }
  
  void for_each_with(const afterhours::Entity& entity,
                     const DocumentComponent& doc,
                     const CaretComponent& caret,
                     const ScrollComponent& scroll,
                     const StatusComponent& status,
                     const LayoutComponent& layout,
                     const MenuComponent& menu,
                     const float) const override {
    // Draw title bar
    raylib::Rectangle titleBarRect = {
      layout.titleBar.x, layout.titleBar.y,
      layout.titleBar.width, layout.titleBar.height
    };
    raylib::DrawRectangleRec(titleBarRect, theme::TITLE_BAR);
    
    std::string title = "Wordproc";
    if (!doc.filePath.empty()) {
      title += " - " + std::filesystem::path(doc.filePath).filename().string();
    } else {
      title += " - Untitled";
    }
    if (doc.isDirty) {
      title += " *";
    }
    raylib::DrawText(title.c_str(), 4, 4, theme::layout::FONT_SIZE, theme::TITLE_TEXT);

    // Draw menu bar background
    raylib::Rectangle menuBarRect = {
      layout.menuBar.x, layout.menuBar.y,
      layout.menuBar.width, layout.menuBar.height
    };
    raylib::DrawRectangleRec(menuBarRect, theme::WINDOW_BG);
    util::drawRaisedBorder(menuBarRect);

    // Draw text area background
    raylib::Rectangle textAreaRect = {
      layout.textArea.x, layout.textArea.y,
      layout.textArea.width, layout.textArea.height
    };
    
    // In paged mode, draw a gray background; in pageless mode, draw white
    if (layout.pageMode == PageMode::Paged) {
      raylib::DrawRectangleRec(textAreaRect, raylib::Color{128, 128, 128, 255});
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
    LayoutComponent::Rect effectiveArea = layout.effectiveTextArea();
    renderTextBuffer(doc.buffer, effectiveArea, caret.visible, 
                     fontSize, lineHeight, scroll.offset);

    // Draw status bar
    raylib::Rectangle statusBarRect = {
      layout.statusBar.x, layout.statusBar.y,
      layout.statusBar.width, layout.statusBar.height
    };
    raylib::DrawRectangleRec(statusBarRect, theme::STATUS_BAR);
    util::drawRaisedBorder(statusBarRect);
    
    double currentTime = raylib::GetTime();
    if (status.hasMessage(currentTime)) {
      raylib::Color msgColor = status.isError ? theme::STATUS_ERROR : theme::STATUS_SUCCESS;
      raylib::DrawText(status.text.c_str(), 4, 
                       layout.screenHeight - theme::layout::STATUS_BAR_HEIGHT + 2,
                       theme::layout::FONT_SIZE - 2, msgColor);
    } else {
      CaretPosition caretPos = doc.buffer.caret();
      std::string statusText = std::format("Ln {}, Col {} | {}{}| {}pt | {}",
                    caretPos.row + 1, caretPos.column + 1,
                    style.bold ? "B " : "",
                    style.italic ? "I " : "",
                    style.fontSize,
                    style.font);
      raylib::DrawText(statusText.c_str(), 4,
                       layout.screenHeight - theme::layout::STATUS_BAR_HEIGHT + 2,
                       theme::layout::FONT_SIZE - 2, theme::TEXT_COLOR);
    }

    // Draw About dialog if active
    if (menu.showAboutDialog) {
      raylib::Rectangle dialogRect = {
        static_cast<float>(layout.screenWidth / 2 - 150),
        static_cast<float>(layout.screenHeight / 2 - 75),
        300, 150
      };
      win95::DrawMessageDialog(dialogRect, "About Wordproc",
        "Wordproc v0.1\n\nA Windows 95 style word processor\nbuilt with Afterhours.", false);
    }
  }
};

// System for handling menu interactions (needs mutable access)
struct MenuSystem : public afterhours::System<DocumentComponent, MenuComponent, StatusComponent, LayoutComponent> {
  void for_each_with(afterhours::Entity& entity,
                     DocumentComponent& doc,
                     MenuComponent& menu,
                     StatusComponent& status,
                     LayoutComponent& layout,
                     const float) override {
    // Draw interactive menus
    int menuResult = win95::DrawMenuBar(menu.menus, 
                                        theme::layout::TITLE_BAR_HEIGHT, 
                                        theme::layout::MENU_BAR_HEIGHT);
    
    if (menuResult >= 0) {
      handleMenuAction(menuResult, doc, menu, status, layout);
    }
    
    // Handle About dialog dismissal
    if (menu.showAboutDialog) {
      raylib::Rectangle dialogRect = {
        static_cast<float>(raylib::GetScreenWidth() / 2 - 150),
        static_cast<float>(raylib::GetScreenHeight() / 2 - 75),
        300, 150
      };
      int result = win95::DrawMessageDialog(dialogRect, "About Wordproc",
        "Wordproc v0.1\n\nA Windows 95 style word processor\nbuilt with Afterhours.", false);
      if (result >= 0) {
        menu.showAboutDialog = false;
      }
    }
  }
  
private:
  void handleMenuAction(int menuResult, DocumentComponent& doc, 
                        MenuComponent& menu, StatusComponent& status,
                        LayoutComponent& layout) {
    int menuIndex = menuResult / 100;
    int itemIndex = menuResult % 100;
    
    if (menuIndex == 0) { // File menu
      switch (itemIndex) {
        case 0: // New
          doc.buffer.setText("");
          doc.filePath.clear();
          doc.isDirty = false;
          break;
        case 1: // Open
          {
            auto result = loadTextFileEx(doc.buffer, doc.defaultPath);
            if (result.success) {
              doc.filePath = doc.defaultPath;
              doc.isDirty = false;
              status.set("Opened: " + std::filesystem::path(doc.defaultPath).filename().string());
              status.expiresAt = raylib::GetTime() + 3.0;
            } else {
              status.set("Open failed: " + result.error, true);
              status.expiresAt = raylib::GetTime() + 3.0;
            }
          }
          break;
        case 2: // Save
          {
            std::string savePath = doc.filePath.empty() ? doc.defaultPath : doc.filePath;
            auto result = saveTextFileEx(doc.buffer, savePath);
            if (result.success) {
              doc.isDirty = false;
              doc.filePath = savePath;
              status.set("Saved: " + std::filesystem::path(savePath).filename().string());
              status.expiresAt = raylib::GetTime() + 3.0;
            } else {
              status.set("Save failed: " + result.error, true);
              status.expiresAt = raylib::GetTime() + 3.0;
            }
          }
          break;
        case 5: // Exit
          break;
        default:
          break;
      }
    } else if (menuIndex == 1) { // Edit menu
      switch (itemIndex) {
        case 0: // Undo
          if (doc.buffer.canUndo()) {
            doc.buffer.undo();
            doc.isDirty = true;
          }
          break;
        case 1: // Redo
          if (doc.buffer.canRedo()) {
            doc.buffer.redo();
            doc.isDirty = true;
          }
          break;
        case 3: // Cut
          if (doc.buffer.hasSelection()) {
            std::string selected = doc.buffer.getSelectedText();
            if (!selected.empty()) {
              raylib::SetClipboardText(selected.c_str());
              doc.buffer.deleteSelection();
              doc.isDirty = true;
            }
          }
          break;
        case 4: // Copy
          if (doc.buffer.hasSelection()) {
            std::string selected = doc.buffer.getSelectedText();
            if (!selected.empty()) {
              raylib::SetClipboardText(selected.c_str());
            }
          }
          break;
        case 5: // Paste
          {
            const char* clipText = raylib::GetClipboardText();
            if (clipText && clipText[0] != '\0') {
              doc.buffer.insertText(clipText);
              doc.isDirty = true;
            }
          }
          break;
        case 7: // Select All
          doc.buffer.selectAll();
          break;
        default:
          break;
      }
    } else if (menuIndex == 2) { // View menu
      switch (itemIndex) {
        case 0: // Pageless Mode
          layout.pageMode = PageMode::Pageless;
          layout.updateLayout(layout.screenWidth, layout.screenHeight);
          status.set("Switched to Pageless mode");
          status.expiresAt = raylib::GetTime() + 2.0;
          break;
        case 1: // Paged Mode
          layout.pageMode = PageMode::Paged;
          layout.updateLayout(layout.screenWidth, layout.screenHeight);
          status.set("Switched to Paged mode");
          status.expiresAt = raylib::GetTime() + 2.0;
          break;
        case 3: // Line Width: Normal (no limit)
          layout.setLineWidthLimit(0.0f);
          status.set("Line width: Normal");
          status.expiresAt = raylib::GetTime() + 2.0;
          break;
        case 4: // Line Width: Narrow (60 chars)
          layout.setLineWidthLimit(60.0f);
          status.set("Line width: Narrow (60 chars)");
          status.expiresAt = raylib::GetTime() + 2.0;
          break;
        case 5: // Line Width: Wide (100 chars)
          layout.setLineWidthLimit(100.0f);
          status.set("Line width: Wide (100 chars)");
          status.expiresAt = raylib::GetTime() + 2.0;
          break;
        default:
          break;
      }
    } else if (menuIndex == 3) { // Format menu
      TextStyle style = doc.buffer.textStyle();
      switch (itemIndex) {
        case 0: // Bold
          style.bold = !style.bold;
          doc.buffer.setTextStyle(style);
          break;
        case 1: // Italic
          style.italic = !style.italic;
          doc.buffer.setTextStyle(style);
          break;
        case 3: // Font: Gaegu
          style.font = "Gaegu-Bold";
          doc.buffer.setTextStyle(style);
          break;
        case 4: // Font: Garamond
          style.font = "EBGaramond-Regular";
          doc.buffer.setTextStyle(style);
          break;
        case 6: // Increase Size
          style.fontSize = std::min(72, style.fontSize + 2);
          doc.buffer.setTextStyle(style);
          break;
        case 7: // Decrease Size
          style.fontSize = std::max(8, style.fontSize - 2);
          doc.buffer.setTextStyle(style);
          break;
        case 8: // Reset Size
          style.fontSize = 16;
          doc.buffer.setTextStyle(style);
          break;
        default:
          break;
      }
    } else if (menuIndex == 4) { // Help menu
      if (itemIndex == 0) { // About
        menu.showAboutDialog = true;
      }
    }
  }
};

} // namespace ecs
