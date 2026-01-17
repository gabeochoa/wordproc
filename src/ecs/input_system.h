#pragma once

#include <algorithm>
#include <cctype>
#include <ctime>
#include <filesystem>

#include "../../vendor/afterhours/src/core/system.h"
#include "../util/clipboard.h"
#include "../editor/document_io.h"
#include "../input/action_map.h"
#include "../rl.h"
#include "../settings.h"
#include "../testing/test_input.h"
#include "../ui/theme.h"
#include "component_helpers.h"
#include "components.h"

namespace ecs {

inline void recordInsertRevision(DocumentComponent& doc, std::size_t offset,
                                 const std::string& text) {
    if (!doc.trackChangesEnabled || text.empty()) return;
    Revision rev;
    rev.type = RevisionType::Insert;
    rev.startOffset = offset;
    rev.text = text;
    rev.timestamp = std::time(nullptr);
    doc.revisions.push_back(rev);
}

inline void recordDeleteRevision(DocumentComponent& doc, std::size_t offset,
                                 const std::string& text) {
    if (!doc.trackChangesEnabled || text.empty()) return;
    Revision rev;
    rev.type = RevisionType::Delete;
    rev.startOffset = offset;
    rev.text = text;
    rev.timestamp = std::time(nullptr);
    doc.revisions.push_back(rev);
}

// System for handling text input (typing characters) using ActionMap
struct TextInputSystem
    : public afterhours::System<DocumentComponent, CaretComponent, MenuComponent> {
    input::ActionMap actionMap_ = input::createDefaultActionMap();

    void for_each_with(afterhours::Entity& /*entity*/, DocumentComponent& doc,
                       CaretComponent& caret, MenuComponent& menu, const float) override {
        using input::Action;
        
        // Find/Replace keyboard shortcuts
        if (actionMap_.isActionPressed(Action::Find)) {
            menu.showFindDialog = true;
            menu.findReplaceMode = false;
        }
        if (actionMap_.isActionPressed(Action::Replace)) {
            menu.showFindDialog = true;
            menu.findReplaceMode = true;
        }

        int codepoint = test_input::get_char_pressed();
        while (codepoint > 0) {
            if (codepoint >= 32) {
                char ch = static_cast<char>(codepoint);
                if (doc.docSettings.smartQuotesEnabled &&
                    (ch == '"' || ch == '\'')) {
                    auto insertSmartQuote = [&](char ascii, const char* openQuote,
                                                const char* closeQuote) {
                        std::size_t offset = doc.buffer.caretOffset();
                        bool opening = true;
                        if (offset > 0) {
                            char prev = doc.buffer.charAtOffset(offset - 1);
                            if (!(std::isspace(static_cast<unsigned char>(prev)) ||
                                  prev == '(' || prev == '[' || prev == '{' ||
                                  prev == '<' || prev == '\n' || prev == '\t')) {
                                opening = false;
                            }
                        }
                        const char* quote = opening ? openQuote : closeQuote;
                        recordInsertRevision(doc, offset, quote);
                        doc.buffer.insertText(quote);
                        doc.isDirty = true;
                        caret::resetBlink(caret);
                    };
                    if (ch == '"') {
                        insertSmartQuote(ch, "\xE2\x80\x9C", "\xE2\x80\x9D");
                    } else {
                        insertSmartQuote(ch, "\xE2\x80\x98", "\xE2\x80\x99");
                    }
                } else {
                    std::size_t offset = doc.buffer.caretOffset();
                    recordInsertRevision(doc, offset, std::string(1, ch));
                    doc.buffer.insertChar(ch);
                    doc.isDirty = true;
                    caret::resetBlink(caret);
                }
            }
            codepoint = test_input::get_char_pressed();
        }

        if (actionMap_.isActionPressed(Action::InsertNewline)) {
            std::size_t offset = doc.buffer.caretOffset();
            recordInsertRevision(doc, offset, "\n");
            doc.buffer.insertChar('\n');
            doc.isDirty = true;
        }
        if (actionMap_.isActionPressed(Action::Backspace)) {
            if (doc.buffer.hasSelection()) {
                CaretPosition start = doc.buffer.selectionStart();
                std::string selected = doc.buffer.getSelectedText();
                recordDeleteRevision(doc, doc.buffer.offsetForPosition(start), selected);
            } else {
                std::size_t offset = doc.buffer.caretOffset();
                if (offset > 0) {
                    char deleted = doc.buffer.charAtOffset(offset - 1);
                    recordDeleteRevision(doc, offset - 1, std::string(1, deleted));
                }
            }
            doc.buffer.backspace();
            doc.isDirty = true;
        }
        if (actionMap_.isActionPressed(Action::Delete)) {
            if (doc.buffer.hasSelection()) {
                CaretPosition start = doc.buffer.selectionStart();
                std::string selected = doc.buffer.getSelectedText();
                recordDeleteRevision(doc, doc.buffer.offsetForPosition(start), selected);
            } else {
                std::size_t offset = doc.buffer.caretOffset();
                if (offset < doc.buffer.getText().size()) {
                    char deleted = doc.buffer.charAtOffset(offset);
                    recordDeleteRevision(doc, offset, std::string(1, deleted));
                }
            }
            doc.buffer.del();
            doc.isDirty = true;
        }

        // Tab inserts spaces (tab stops)
        if (IsKeyPressed(raylib::KEY_TAB)) {
            int width = std::max(1, doc.docSettings.tabWidth);
            std::string spaces(static_cast<std::size_t>(width), ' ');
            std::size_t offset = doc.buffer.caretOffset();
            recordInsertRevision(doc, offset, spaces);
            doc.buffer.insertText(spaces);
            doc.isDirty = true;
        }
    }
};

// System for handling keyboard shortcuts using remappable ActionMap
struct KeyboardShortcutSystem
    : public afterhours::System<DocumentComponent, CaretComponent,
                                StatusComponent, LayoutComponent> {
    input::ActionMap actionMap_ = input::createDefaultActionMap();

    void for_each_with(afterhours::Entity& /*entity*/, DocumentComponent& doc,
                       CaretComponent& caret, StatusComponent& status,
                       LayoutComponent& layout, const float) override {
        using input::Action;

        // New document
        if (actionMap_.isActionPressed(Action::New)) {
            doc.buffer.setText("");
            doc.filePath.clear();
            doc.isDirty = false;
            doc.comments.clear();
            doc.revisions.clear();
            doc.trackChangesBaseline.clear();
            status::set(status, "New document");
            status.expiresAt = raylib::GetTime() + 2.0;
        }

        // Save
        if (actionMap_.isActionPressed(Action::Save)) {
            std::string savePath =
                doc.filePath.empty() ? doc.defaultPath : doc.filePath;
            // Sync layout settings to document settings before save
            doc.docSettings.textStyle = doc.buffer.textStyle();
            doc.docSettings.pageSettings.mode = layout.pageMode;
            doc.docSettings.pageSettings.pageWidth = layout.pageWidth;
            doc.docSettings.pageSettings.pageHeight = layout.pageHeight;
            doc.docSettings.pageSettings.pageMargin = layout.pageMargin;
            doc.docSettings.pageSettings.lineWidthLimit = layout.lineWidthLimit;
            // Save document with all settings
            auto result = saveDocumentEx(doc.buffer, doc.docSettings, savePath);
            if (result.success) {
                doc.isDirty = false;
                doc.filePath = savePath;
                if (!doc.autoSavePath.empty()) {
                    std::filesystem::remove(doc.autoSavePath);
                }
                Settings::get().add_recent_file(savePath);
                status::set(
                    status,
                    "Saved: " +
                        std::filesystem::path(savePath).filename().string());
                status.expiresAt = raylib::GetTime() + 3.0;
            } else {
                status::set(status, "Save failed: " + result.error, true);
                status.expiresAt = raylib::GetTime() + 3.0;
            }
        }

        // Open
        if (actionMap_.isActionPressed(Action::Open)) {
            // Load document with settings
            auto result =
                loadDocumentEx(doc.buffer, doc.docSettings, doc.defaultPath);
            if (result.success) {
                doc.filePath = doc.defaultPath;
                doc.isDirty = false;
                doc.comments.clear();
                doc.revisions.clear();
                // Sync loaded document settings to layout component
                layout.pageMode = doc.docSettings.pageSettings.mode;
                layout.pageWidth = doc.docSettings.pageSettings.pageWidth;
                layout.pageHeight = doc.docSettings.pageSettings.pageHeight;
                layout.pageMargin = doc.docSettings.pageSettings.pageMargin;
                layout.lineWidthLimit =
                    doc.docSettings.pageSettings.lineWidthLimit;
                Settings::get().add_recent_file(doc.defaultPath);
                status::set(status,
                            "Opened: " + std::filesystem::path(doc.defaultPath)
                                             .filename()
                                             .string());
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

        // Underline
        if (actionMap_.isActionPressed(Action::ToggleUnderline)) {
            TextStyle style = doc.buffer.textStyle();
            style.underline = !style.underline;
            doc.buffer.setTextStyle(style);
        }

        // Strikethrough
        if (actionMap_.isActionPressed(Action::ToggleStrikethrough)) {
            TextStyle style = doc.buffer.textStyle();
            style.strikethrough = !style.strikethrough;
            doc.buffer.setTextStyle(style);
        }

        // Superscript
        if (actionMap_.isActionPressed(Action::ToggleSuperscript)) {
            TextStyle style = doc.buffer.textStyle();
            style.superscript = !style.superscript;
            if (style.superscript) {
                style.subscript = false;
            }
            doc.buffer.setTextStyle(style);
        }

        // Subscript
        if (actionMap_.isActionPressed(Action::ToggleSubscript)) {
            TextStyle style = doc.buffer.textStyle();
            style.subscript = !style.subscript;
            if (style.subscript) {
                style.superscript = false;
            }
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

        // Paragraph styles
        if (actionMap_.isActionPressed(Action::StyleNormal)) {
            doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Normal);
        }
        if (actionMap_.isActionPressed(Action::StyleTitle)) {
            doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Title);
        }
        if (actionMap_.isActionPressed(Action::StyleSubtitle)) {
            doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Subtitle);
        }
        if (actionMap_.isActionPressed(Action::StyleHeading1)) {
            doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Heading1);
        }
        if (actionMap_.isActionPressed(Action::StyleHeading2)) {
            doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Heading2);
        }
        if (actionMap_.isActionPressed(Action::StyleHeading3)) {
            doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Heading3);
        }
        if (actionMap_.isActionPressed(Action::StyleHeading4)) {
            doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Heading4);
        }
        if (actionMap_.isActionPressed(Action::StyleHeading5)) {
            doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Heading5);
        }
        if (actionMap_.isActionPressed(Action::StyleHeading6)) {
            doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Heading6);
        }
        
        // Text alignment
        if (actionMap_.isActionPressed(Action::AlignLeft)) {
            doc.buffer.setCurrentAlignment(TextAlignment::Left);
        }
        if (actionMap_.isActionPressed(Action::AlignCenter)) {
            doc.buffer.setCurrentAlignment(TextAlignment::Center);
        }
        if (actionMap_.isActionPressed(Action::AlignRight)) {
            doc.buffer.setCurrentAlignment(TextAlignment::Right);
        }
        if (actionMap_.isActionPressed(Action::AlignJustify)) {
            doc.buffer.setCurrentAlignment(TextAlignment::Justify);
        }
        
        // Indentation
        if (actionMap_.isActionPressed(Action::IndentIncrease)) {
            doc.buffer.increaseIndent();
        }
        if (actionMap_.isActionPressed(Action::IndentDecrease)) {
            doc.buffer.decreaseIndent();
        }
        
        // Line spacing
        if (actionMap_.isActionPressed(Action::LineSpacingSingle)) {
            doc.buffer.setLineSpacingSingle();
        }
        if (actionMap_.isActionPressed(Action::LineSpacing1_5)) {
            doc.buffer.setLineSpacing1_5();
        }
        if (actionMap_.isActionPressed(Action::LineSpacingDouble)) {
            doc.buffer.setLineSpacingDouble();
        }
        
        // Lists
        if (actionMap_.isActionPressed(Action::ToggleBulletedList)) {
            doc.buffer.toggleBulletedList();
        }
        if (actionMap_.isActionPressed(Action::ToggleNumberedList)) {
            doc.buffer.toggleNumberedList();
        }
        
        // Paragraph spacing (increase/decrease by 6px increments)
        if (actionMap_.isActionPressed(Action::IncreaseSpaceBefore)) {
            doc.buffer.setCurrentSpaceBefore(doc.buffer.currentSpaceBefore() + 6);
        }
        if (actionMap_.isActionPressed(Action::DecreaseSpaceBefore)) {
            doc.buffer.setCurrentSpaceBefore(doc.buffer.currentSpaceBefore() - 6);
        }
        if (actionMap_.isActionPressed(Action::IncreaseSpaceAfter)) {
            doc.buffer.setCurrentSpaceAfter(doc.buffer.currentSpaceAfter() + 6);
        }
        if (actionMap_.isActionPressed(Action::DecreaseSpaceAfter)) {
            doc.buffer.setCurrentSpaceAfter(doc.buffer.currentSpaceAfter() - 6);
        }

        // View controls
        if (actionMap_.isActionPressed(Action::ZoomIn)) {
            layout.zoomLevel = std::min(4.0f, layout.zoomLevel + 0.1f);
        }
        if (actionMap_.isActionPressed(Action::ZoomOut)) {
            layout.zoomLevel = std::max(0.5f, layout.zoomLevel - 0.1f);
        }
        if (actionMap_.isActionPressed(Action::ZoomReset)) {
            layout.zoomLevel = 1.0f;
        }
        if (actionMap_.isActionPressed(Action::ToggleFocusMode)) {
            layout.focusMode = !layout.focusMode;
            layout::updateLayout(layout, layout.screenWidth, layout.screenHeight);
        }
        if (actionMap_.isActionPressed(Action::ToggleSplitView)) {
            layout.splitViewEnabled = !layout.splitViewEnabled;
        }
        if (actionMap_.isActionPressed(Action::ToggleDarkMode)) {
            theme::applyDarkMode(!theme::DARK_MODE_ENABLED);
        }
        
        // Page breaks
        if (actionMap_.isActionPressed(Action::InsertPageBreak)) {
            doc.buffer.insertPageBreak();
            doc.isDirty = true;
        }
        if (actionMap_.isActionPressed(Action::TogglePageBreak)) {
            doc.buffer.togglePageBreak();
            doc.isDirty = true;
        }

        // Copy
        if (actionMap_.isActionPressed(Action::Copy)) {
            if (doc.buffer.hasSelection()) {
                std::string selected = doc.buffer.getSelectedText();
                if (!selected.empty()) {
                    app::clipboard::set_text(selected);
                }
            }
        }
        // Cut
        if (actionMap_.isActionPressed(Action::Cut)) {
            if (doc.buffer.hasSelection()) {
                std::string selected = doc.buffer.getSelectedText();
                if (!selected.empty()) {
                    CaretPosition start = doc.buffer.selectionStart();
                    recordDeleteRevision(doc, doc.buffer.offsetForPosition(start), selected);
                    app::clipboard::set_text(selected);
                    doc.buffer.deleteSelection();
                    doc.isDirty = true;
                }
            }
        }
        // Paste
        if (actionMap_.isActionPressed(Action::Paste)) {
            if (app::clipboard::has_text()) {
                std::string clipText = app::clipboard::get_text();
                std::size_t offset = doc.buffer.caretOffset();
                recordInsertRevision(doc, offset, clipText);
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

// System for handling navigation keys using remappable ActionMap
struct NavigationSystem
    : public afterhours::System<DocumentComponent, CaretComponent,
                                ScrollComponent, LayoutComponent> {
    input::ActionMap actionMap_ = input::createDefaultActionMap();

    void for_each_with(afterhours::Entity& /*entity*/, DocumentComponent& doc,
                       CaretComponent& caret, ScrollComponent& scroll,
                       LayoutComponent& layout, const float) override {
        using input::Action;

        auto isKeyDownOrSynthetic = [](int key) {
            return raylib::IsKeyDown(key) || input_injector::is_key_synthetically_down(key);
        };
        bool shift_down = isKeyDownOrSynthetic(raylib::KEY_LEFT_SHIFT) ||
                          isKeyDownOrSynthetic(raylib::KEY_RIGHT_SHIFT);

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

        // Left/Right with Ctrl for word movement
        if (actionMap_.isActionPressed(Action::MoveWordLeft)) {
            navigateWithSelection([&]() { doc.buffer.moveWordLeft(); });
        } else if (actionMap_.isActionPressed(Action::MoveLeft)) {
            navigateWithSelection([&]() { doc.buffer.moveLeft(); });
        }
        if (actionMap_.isActionPressed(Action::MoveWordRight)) {
            navigateWithSelection([&]() { doc.buffer.moveWordRight(); });
        } else if (actionMap_.isActionPressed(Action::MoveRight)) {
            navigateWithSelection([&]() { doc.buffer.moveRight(); });
        }
        if (actionMap_.isActionPressed(Action::MoveUp)) {
            navigateWithSelection([&]() { doc.buffer.moveUp(); });
        }
        if (actionMap_.isActionPressed(Action::MoveDown)) {
            navigateWithSelection([&]() { doc.buffer.moveDown(); });
        }

        // Home/End with Ctrl for document start/end
        if (actionMap_.isActionPressed(Action::MoveDocumentStart)) {
            navigateWithSelection([&]() { doc.buffer.moveToDocumentStart(); });
        } else if (actionMap_.isActionPressed(Action::MoveLineStart)) {
            navigateWithSelection([&]() { doc.buffer.moveToLineStart(); });
        }
        if (actionMap_.isActionPressed(Action::MoveDocumentEnd)) {
            navigateWithSelection([&]() { doc.buffer.moveToDocumentEnd(); });
        } else if (actionMap_.isActionPressed(Action::MoveLineEnd)) {
            navigateWithSelection([&]() { doc.buffer.moveToLineEnd(); });
        }

        auto clampSecondary = [&]() {
            int lineCount = static_cast<int>(doc.buffer.lineCount());
            int maxScroll = lineCount - scroll.visibleLines;
            if (maxScroll < 0) maxScroll = 0;
            scroll.secondaryOffset = std::clamp(scroll.secondaryOffset, 0, maxScroll);
        };

        // Page Up/Down
        constexpr std::size_t LINES_PER_PAGE = 20;
        if (actionMap_.isActionPressed(Action::PageUp)) {
            if (layout.splitViewEnabled && shift_down) {
                scroll.secondaryOffset -= static_cast<int>(LINES_PER_PAGE);
                clampSecondary();
            } else {
                navigateWithSelection(
                    [&]() { doc.buffer.movePageUp(LINES_PER_PAGE); });
            }
        }
        if (actionMap_.isActionPressed(Action::PageDown)) {
            if (layout.splitViewEnabled && shift_down) {
                scroll.secondaryOffset += static_cast<int>(LINES_PER_PAGE);
                clampSecondary();
            } else {
                navigateWithSelection(
                    [&]() { doc.buffer.movePageDown(LINES_PER_PAGE); });
            }
        }

        // Mouse wheel scrolling
        float wheelMove = GetMouseWheelMove();
        if (wheelMove != 0.0f) {
            int scrollLines = static_cast<int>(-wheelMove * 3);
            if (layout.splitViewEnabled && shift_down) {
                scroll.secondaryOffset += scrollLines;
                clampSecondary();
            } else {
                scroll.offset += scrollLines;
            }
        }

        // Auto-scroll to keep caret visible
        CaretPosition caretPos = doc.buffer.caret();
        scroll::scrollToRow(scroll, static_cast<int>(caretPos.row));
        scroll::clamp(scroll, static_cast<int>(doc.buffer.lineCount()));
    }
};

// System for auto-saving documents periodically
struct AutoSaveSystem
    : public afterhours::System<DocumentComponent, StatusComponent, LayoutComponent> {
    void for_each_with(afterhours::Entity& /*entity*/, DocumentComponent& doc,
                       StatusComponent& status, LayoutComponent& layout,
                       const float) override {
        if (!doc.autoSaveEnabled || !doc.isDirty) {
            return;
        }

        double now = raylib::GetTime();
        if ((now - doc.lastAutoSaveTime) < doc.autoSaveIntervalSeconds) {
            return;
        }

        if (doc.autoSavePath.empty()) {
            doc.autoSavePath = "output/autosave.wpdoc";
        }

        std::filesystem::create_directories(
            std::filesystem::path(doc.autoSavePath).parent_path());

        // Sync layout settings to document settings before save
        doc.docSettings.textStyle = doc.buffer.textStyle();
        doc.docSettings.pageSettings.mode = layout.pageMode;
        doc.docSettings.pageSettings.pageWidth = layout.pageWidth;
        doc.docSettings.pageSettings.pageHeight = layout.pageHeight;
        doc.docSettings.pageSettings.pageMargin = layout.pageMargin;
        doc.docSettings.pageSettings.lineWidthLimit = layout.lineWidthLimit;

        auto result = saveDocumentEx(doc.buffer, doc.docSettings, doc.autoSavePath);
        if (result.success) {
            doc.lastAutoSaveTime = now;
            status::set(status, "Auto-saved");
            status.expiresAt = now + 2.0;
        } else {
            status::set(status, "Auto-save failed: " + result.error, true);
            status.expiresAt = now + 2.0;
        }
    }
};

// System for updating caret blink
struct CaretBlinkSystem : public afterhours::System<CaretComponent> {
    void for_each_with(afterhours::Entity& /*entity*/, CaretComponent& caret,
                       const float dt) override {
        caret.blinkTimer += static_cast<double>(dt);
        if (caret.blinkTimer >= CaretComponent::BLINK_INTERVAL) {
            caret.blinkTimer = 0.0;
            caret.visible = !caret.visible;
        }
    }
};

// System for updating layout calculations
struct LayoutUpdateSystem
    : public afterhours::System<LayoutComponent, DocumentComponent,
                                ScrollComponent> {
    void for_each_with(afterhours::Entity& /*entity*/, LayoutComponent& layout,
                       DocumentComponent& doc, ScrollComponent& scroll,
                       const float) override {
        int w = raylib::GetScreenWidth();
        int h = raylib::GetScreenHeight();
        layout::updateLayout(layout, w, h);

        // Calculate visible lines
        TextStyle style = doc.buffer.textStyle();
        int lineHeight = style.fontSize + 4;
        int visibleLines = static_cast<int>(
            (layout.textArea.height - 2 * layout.textPadding) / lineHeight);
        if (visibleLines < 1) visibleLines = 1;
        scroll.visibleLines = visibleLines;
    }
};

}  // namespace ecs
