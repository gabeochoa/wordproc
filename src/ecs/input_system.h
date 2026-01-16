#pragma once

#include "components.h"
#include "component_helpers.h"
#include "../editor/document_io.h"
#include "../input/action_map.h"
#include "../rl.h"
#include "../../vendor/afterhours/src/core/system.h"

#include <filesystem>

namespace ecs {

// System for handling text input (typing characters)
struct TextInputSystem : public afterhours::System<DocumentComponent, CaretComponent> {
  void for_each_with(afterhours::Entity& entity, 
                     DocumentComponent& doc, 
                     CaretComponent& caret, 
                     const float) override {
    int codepoint = raylib::GetCharPressed();
    while (codepoint > 0) {
      if (codepoint >= 32) {
        doc.buffer.insertChar(static_cast<char>(codepoint));
        doc.isDirty = true;
        caret::resetBlink(caret);
      }
      codepoint = raylib::GetCharPressed();
    }

    if (raylib::IsKeyPressed(raylib::KEY_ENTER) ||
        raylib::IsKeyPressed(raylib::KEY_KP_ENTER)) {
      doc.buffer.insertChar('\n');
      doc.isDirty = true;
    }
    if (raylib::IsKeyPressed(raylib::KEY_BACKSPACE)) {
      doc.buffer.backspace();
      doc.isDirty = true;
    }
    if (raylib::IsKeyPressed(raylib::KEY_DELETE)) {
      doc.buffer.del();
      doc.isDirty = true;
    }
  }
};

// System for handling keyboard shortcuts using remappable ActionMap
struct KeyboardShortcutSystem : public afterhours::System<DocumentComponent, CaretComponent, StatusComponent> {
  input::ActionMap actionMap_ = input::createDefaultActionMap();
  
  void for_each_with(afterhours::Entity& entity,
                     DocumentComponent& doc,
                     CaretComponent& caret,
                     StatusComponent& status,
                     const float) override {
    using input::Action;
    
    // Save
    if (actionMap_.isActionPressed(Action::Save)) {
      std::string savePath = doc.filePath.empty() ? doc.defaultPath : doc.filePath;
      auto result = saveTextFileEx(doc.buffer, savePath);
      if (result.success) {
        doc.isDirty = false;
        doc.filePath = savePath;
        status::set(status, "Saved: " + std::filesystem::path(savePath).filename().string());
        status.expiresAt = raylib::GetTime() + 3.0;
      } else {
        status::set(status, "Save failed: " + result.error, true);
        status.expiresAt = raylib::GetTime() + 3.0;
      }
    }

    // Open
    if (actionMap_.isActionPressed(Action::Open)) {
      auto result = loadTextFileEx(doc.buffer, doc.defaultPath);
      if (result.success) {
        doc.filePath = doc.defaultPath;
        doc.isDirty = false;
        status::set(status, "Opened: " + std::filesystem::path(doc.defaultPath).filename().string());
        status.expiresAt = raylib::GetTime() + 3.0;
      } else {
        status::set(status, "Open failed: " + result.error, true);
        status.expiresAt = raylib::GetTime() + 3.0;
      }
    }

    // Bold
    if (actionMap_.isActionPressed(Action::ToggleBold)) {
      TextStyle style = doc.buffer.textStyle();
      style.bold = !style.bold;
      doc.buffer.setTextStyle(style);
    }

    // Italic
    if (actionMap_.isActionPressed(Action::ToggleItalic)) {
      TextStyle style = doc.buffer.textStyle();
      style.italic = !style.italic;
      doc.buffer.setTextStyle(style);
    }

    // Font selection
    if (actionMap_.isActionPressed(Action::FontGaegu)) {
      TextStyle style = doc.buffer.textStyle();
      style.font = "Gaegu-Bold";
      doc.buffer.setTextStyle(style);
    }
    if (actionMap_.isActionPressed(Action::FontGaramond)) {
      TextStyle style = doc.buffer.textStyle();
      style.font = "EBGaramond-Regular";
      doc.buffer.setTextStyle(style);
    }

    // Font size
    if (actionMap_.isActionPressed(Action::IncreaseFontSize)) {
      TextStyle style = doc.buffer.textStyle();
      style.fontSize = std::min(72, style.fontSize + 2);
      doc.buffer.setTextStyle(style);
    }
    if (actionMap_.isActionPressed(Action::DecreaseFontSize)) {
      TextStyle style = doc.buffer.textStyle();
      style.fontSize = std::max(8, style.fontSize - 2);
      doc.buffer.setTextStyle(style);
    }
    if (actionMap_.isActionPressed(Action::ResetFontSize)) {
      TextStyle style = doc.buffer.textStyle();
      style.fontSize = 16;
      doc.buffer.setTextStyle(style);
    }

    // Copy
    if (actionMap_.isActionPressed(Action::Copy)) {
      if (doc.buffer.hasSelection()) {
        std::string selected = doc.buffer.getSelectedText();
        if (!selected.empty()) {
          raylib::SetClipboardText(selected.c_str());
        }
      }
    }
    // Cut
    if (actionMap_.isActionPressed(Action::Cut)) {
      if (doc.buffer.hasSelection()) {
        std::string selected = doc.buffer.getSelectedText();
        if (!selected.empty()) {
          raylib::SetClipboardText(selected.c_str());
          doc.buffer.deleteSelection();
          doc.isDirty = true;
        }
      }
    }
    // Paste
    if (actionMap_.isActionPressed(Action::Paste)) {
      const char* clipText = raylib::GetClipboardText();
      if (clipText && clipText[0] != '\0') {
        doc.buffer.insertText(clipText);
        doc.isDirty = true;
      }
    }
    // Select All
    if (actionMap_.isActionPressed(Action::SelectAll)) {
      doc.buffer.selectAll();
    }

    // Undo
    if (actionMap_.isActionPressed(Action::Undo)) {
      if (doc.buffer.canUndo()) {
        doc.buffer.undo();
        doc.isDirty = true;
        caret::resetBlink(caret);
      }
    }
    // Redo
    if (actionMap_.isActionPressed(Action::Redo)) {
      if (doc.buffer.canRedo()) {
        doc.buffer.redo();
        doc.isDirty = true;
        caret::resetBlink(caret);
      }
    }
  }
};

// System for handling navigation keys
struct NavigationSystem : public afterhours::System<DocumentComponent, CaretComponent, ScrollComponent> {
  void for_each_with(afterhours::Entity& entity,
                     DocumentComponent& doc,
                     CaretComponent& caret,
                     ScrollComponent& scroll,
                     const float) override {
    bool ctrl_down = raylib::IsKeyDown(raylib::KEY_LEFT_CONTROL) ||
                     raylib::IsKeyDown(raylib::KEY_RIGHT_CONTROL);
    bool shift_down = raylib::IsKeyDown(raylib::KEY_LEFT_SHIFT) ||
                      raylib::IsKeyDown(raylib::KEY_RIGHT_SHIFT);

    auto navigateWithSelection = [&](auto moveFunc) {
      CaretPosition before = doc.buffer.caret();
      if (shift_down && !doc.buffer.hasSelection()) {
        doc.buffer.setSelectionAnchor(before);
      }
      if (!shift_down) {
        doc.buffer.clearSelection();
      }
      moveFunc();
      if (shift_down) {
        doc.buffer.updateSelectionToCaret();
      }
      caret::resetBlink(caret);
    };

    if (raylib::IsKeyPressed(raylib::KEY_LEFT)) {
      if (ctrl_down) {
        navigateWithSelection([&]() { doc.buffer.moveWordLeft(); });
      } else {
        navigateWithSelection([&]() { doc.buffer.moveLeft(); });
      }
    }
    if (raylib::IsKeyPressed(raylib::KEY_RIGHT)) {
      if (ctrl_down) {
        navigateWithSelection([&]() { doc.buffer.moveWordRight(); });
      } else {
        navigateWithSelection([&]() { doc.buffer.moveRight(); });
      }
    }
    if (raylib::IsKeyPressed(raylib::KEY_UP)) {
      navigateWithSelection([&]() { doc.buffer.moveUp(); });
    }
    if (raylib::IsKeyPressed(raylib::KEY_DOWN)) {
      navigateWithSelection([&]() { doc.buffer.moveDown(); });
    }

    // Home/End
    if (raylib::IsKeyPressed(raylib::KEY_HOME)) {
      if (ctrl_down) {
        navigateWithSelection([&]() { doc.buffer.moveToDocumentStart(); });
      } else {
        navigateWithSelection([&]() { doc.buffer.moveToLineStart(); });
      }
    }
    if (raylib::IsKeyPressed(raylib::KEY_END)) {
      if (ctrl_down) {
        navigateWithSelection([&]() { doc.buffer.moveToDocumentEnd(); });
      } else {
        navigateWithSelection([&]() { doc.buffer.moveToLineEnd(); });
      }
    }

    // Page Up/Down
    constexpr std::size_t LINES_PER_PAGE = 20;
    if (raylib::IsKeyPressed(raylib::KEY_PAGE_UP)) {
      navigateWithSelection([&]() { doc.buffer.movePageUp(LINES_PER_PAGE); });
    }
    if (raylib::IsKeyPressed(raylib::KEY_PAGE_DOWN)) {
      navigateWithSelection([&]() { doc.buffer.movePageDown(LINES_PER_PAGE); });
    }

    // Mouse wheel scrolling
    float wheelMove = raylib::GetMouseWheelMove();
    if (wheelMove != 0.0f) {
      int scrollLines = static_cast<int>(-wheelMove * 3);
      scroll.offset += scrollLines;
    }

    // Auto-scroll to keep caret visible
    CaretPosition caretPos = doc.buffer.caret();
    scroll::scrollToRow(scroll, static_cast<int>(caretPos.row));
    scroll::clamp(scroll, static_cast<int>(doc.buffer.lineCount()));
  }
};

// System for updating caret blink
struct CaretBlinkSystem : public afterhours::System<CaretComponent> {
  void for_each_with(afterhours::Entity& entity,
                     CaretComponent& caret,
                     const float dt) override {
    caret.blinkTimer += static_cast<double>(dt);
    if (caret.blinkTimer >= CaretComponent::BLINK_INTERVAL) {
      caret.blinkTimer = 0.0;
      caret.visible = !caret.visible;
    }
  }
};

// System for updating layout calculations
struct LayoutUpdateSystem : public afterhours::System<LayoutComponent, DocumentComponent, ScrollComponent> {
  void for_each_with(afterhours::Entity& entity,
                     LayoutComponent& layout,
                     DocumentComponent& doc,
                     ScrollComponent& scroll,
                     const float) override {
    int w = raylib::GetScreenWidth();
    int h = raylib::GetScreenHeight();
    layout::updateLayout(layout, w, h);
    
    // Calculate visible lines
    TextStyle style = doc.buffer.textStyle();
    int lineHeight = style.fontSize + 4;
    int visibleLines = static_cast<int>((layout.textArea.height - 2 * layout.textPadding) / lineHeight);
    if (visibleLines < 1) visibleLines = 1;
    scroll.visibleLines = visibleLines;
  }
};

} // namespace ecs
