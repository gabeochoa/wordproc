#pragma once

#include <algorithm>
#include <cctype>
#include <ctime>
#include <filesystem>

#include "../../vendor/afterhours/src/core/system.h"
#include "../input/action_map.h"
#include "../rl.h"
// test_input:: available via rl.h -> external.h
#include "../editor/scroll_math.h"
#include "../ui/input.h"
#include "../ui/theme.h"
#include "../ui/ui_context.h"
#include "component_helpers.h"
#include "document_commands.h"

namespace ecs {

using cmd::recordInsertRevision;
using cmd::recordDeleteRevision;

// System for handling text input (typing characters) using ActionMap
struct TextInputSystem
    : public afterhours::System<DocumentComponent, CaretComponent, DialogState, InputComponent,
                              LayoutComponent, MenuComponent> {

    void for_each_with(afterhours::Entity& /*entity*/, DocumentComponent& doc,
                       CaretComponent& caret, DialogState& dialogs,
                       InputComponent& inputComp, LayoutComponent& layout,
                       MenuComponent& menu, const float) override {
        using input::Action;
        auto& actionMap_ = inputComp.actionMap;

        // Menu typing gate: while any dropdown menu is open, menus
        // consume keys -- typed characters must not reach the document
        // (arrows/Esc are already handled by MenuUISystem). Queued
        // chars are drained, not just ignored: an unconsumed char
        // would otherwise sit in the queue and insert itself the
        // frame the menu closes.
        bool anyMenuOpen = menu.activeMenuIndex >= 0;
        if (!anyMenuOpen) {
            for (const auto& m : menu.menus) {
                if (m.open) {
                    anyMenuOpen = true;
                    break;
                }
            }
        }
        if (anyMenuOpen) {
            while (test_input::get_char_pressed() > 0) {
            }
            return;
        }

        // Print Preview is read-only: no typing, newline, backspace,
        // delete, or tab insertion while previewMode is on. Scroll and
        // zoom stay live (NavigationSystem pages, KeyboardShortcutSystem
        // allows zoom/print in preview). The queued chars are drained
        // (not just ignored): an unconsumed char would otherwise sit
        // in the queue and insert itself the frame preview exits.
        if (layout.previewMode) {
            while (test_input::get_char_pressed() > 0) {
            }
            return;
        }

        // @ chip popup owns Enter (select) and Escape (dismiss)
        // while open. Typing / Backspace fall through below: they
        // edit the filter text in the buffer. Up/Down are owned by
        // NavigationSystem (it runs after this one).
        if (dialogs.showChipPopup) {
            if (!cmd::chipPopupValid(doc, dialogs)) {
                dialogs.showChipPopup = false;
            } else if (input::isKeyPressed(afterhours::keys::ESCAPE)) {
                dialogs.showChipPopup = false;
                return;
            } else if (actionMap_.isActionPressed(Action::InsertNewline)) {
                cmd::applyChipPopup(doc, dialogs);
                caret::resetBlink(caret);
                return;
            }
        }

        // Find/Replace keyboard shortcuts
        if (actionMap_.isActionPressed(Action::Find)) {
            dialogs.showFindDialog = true;
            dialogs.findReplaceMode = false;
        }
        if (actionMap_.isActionPressed(Action::Replace)) {
            dialogs.showFindDialog = true;
            dialogs.findReplaceMode = true;
        }

        int codepoint = test_input::get_char_pressed();
        while (codepoint > 0) {
            if (codepoint >= 32) {
                char ch = static_cast<char>(codepoint);
                if (doc.docSettings.smartQuotesEnabled &&
                    (ch == '"' || ch == '\'')) {
                    auto insertSmartQuote = [&](char /*ascii*/, const char* openQuote,
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
                    // AutoCorrect: on word completion (space/punctuation),
                    // replace the word + delimiter as a single undo step.
                    // tryAutoCorrect returns true when it handled the key.
                    bool handled = cmd::isAutoCorrectDelimiter(ch) &&
                                   cmd::tryAutoCorrect(doc, ch);
                    if (!handled) {
                        std::size_t offset = doc.buffer.caretOffset();
                        // "@" opens the smart-chip popup when the char
                        // before it is the start or whitespace (and
                        // there is no selection: "@" replacing a
                        // selection would mis-anchor the popup).
                        bool openChipPopup = false;
                        if (ch == '@' && !doc.buffer.hasSelection()) {
                            if (offset == 0) {
                                openChipPopup = true;
                            } else {
                                char prev =
                                    doc.buffer.charAtOffset(offset - 1);
                                if (std::isspace(
                                        static_cast<unsigned char>(prev))) {
                                    openChipPopup = true;
                                }
                            }
                        }
                        recordInsertRevision(doc, offset, std::string(1, ch));
                        doc.buffer.insertChar(ch);
                        doc.isDirty = true;
                        if (openChipPopup) {
                            dialogs.showChipPopup = true;
                            dialogs.chipPopupAtOffset = offset;
                            dialogs.chipPopupSelected = 0;
                        }
                    }
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
        // Use isActionPressedRepeat for backspace/delete so they repeat when held
        if (actionMap_.isActionPressedRepeat(Action::Backspace)) {
            // Backspace at a chip's end deletes the whole chip (one
            // undo step), not one character. Not while the popup is
            // open: there Backspace edits the filter text.
            bool chipErased = !doc.buffer.hasSelection() &&
                              !dialogs.showChipPopup &&
                              cmd::eraseChipAtCaret(doc);
            if (!chipErased) {
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
            }
            doc.isDirty = true;
        }
        if (actionMap_.isActionPressedRepeat(Action::Delete)) {
            if (doc.buffer.hasSelection()) {
                CaretPosition start = doc.buffer.selectionStart();
                std::string selected = doc.buffer.getSelectedText();
                recordDeleteRevision(doc, doc.buffer.offsetForPosition(start), selected);
            } else {
                std::size_t offset = doc.buffer.caretOffset();
                if (offset < doc.buffer.textSize()) {
                    char deleted = doc.buffer.charAtOffset(offset);
                    recordDeleteRevision(doc, offset, std::string(1, deleted));
                }
            }
            doc.buffer.del();
            doc.isDirty = true;
        }

        // Tab inserts spaces (tab stops)
        if (input::isKeyPressed(afterhours::keys::TAB)) {
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
                                LayoutComponent, MenuComponent, DialogState, InputComponent> {

    void for_each_with(afterhours::Entity& /*entity*/, DocumentComponent& doc,
                       CaretComponent& caret,
                       LayoutComponent& layout, MenuComponent& menu,
                       DialogState& dialogs, InputComponent& inputComp, const float) override {
        using input::Action;
        auto& actionMap_ = inputComp.actionMap;

        // Print Preview is read-only: only zoom and Print stay live.
        // Everything else below (New/Save, formatting, clipboard,
        // undo/redo, Spelling...) is blocked while previewMode is on.
        if (layout.previewMode) {
            // Steps are in EFFECTIVE zoom (what the status bar and
            // dropdown show): + / - 10 effective points, reset to
            // effective 100% (page at 72ppi), not user zoom 1.0.
            if (actionMap_.isActionPressed(Action::ZoomIn)) {
                layout::setEffectiveZoom(
                    layout, layout::effectiveZoom(layout) + 0.1f);
            }
            if (actionMap_.isActionPressed(Action::ZoomOut)) {
                layout::setEffectiveZoom(
                    layout, std::max(0.1f,
                                     layout::effectiveZoom(layout) - 0.1f));
            }
            if (actionMap_.isActionPressed(Action::ZoomReset)) {
                layout::setEffectiveZoom(layout, 1.0f);
            }
            if (actionMap_.isActionPressed(Action::Print)) {
                cmd::requestPrint(doc, dialogs);
            }
            return;
        }

        // Print (Ctrl+P)
        if (actionMap_.isActionPressed(Action::Print)) {
            cmd::requestPrint(doc, dialogs);
        }

        // New document
        if (actionMap_.isActionPressed(Action::New)) {
            cmd::newDocument(doc);
            toast_notify::info("New document");
        }

        // Save
        if (actionMap_.isActionPressed(Action::Save)) {
            cmd::saveDocument(doc, layout, menu);
        }

        // Save As - defer to top of next frame to avoid blocking mid-ECS
        if (actionMap_.isActionPressed(Action::SaveAs)) {
            menu.pendingDialog = MenuComponent::PendingDialog::SaveAs;
        }

        // Open - defer to top of next frame to avoid blocking mid-ECS
        if (actionMap_.isActionPressed(Action::Open)) {
            menu.pendingDialog = MenuComponent::PendingDialog::Open;
        }

        // Word Count
        if (actionMap_.isActionPressed(Action::ShowWordCount)) {
            dialogs.showWordCountDialog = true;
        }

        // Spelling (F7)
        if (actionMap_.isActionPressed(Action::ShowSpelling)) {
            cmd::openSpellingDialog(doc, dialogs);
        }

        // Text formatting
        if (actionMap_.isActionPressed(Action::ToggleBold)) cmd::toggleBold(doc);
        if (actionMap_.isActionPressed(Action::ToggleItalic)) cmd::toggleItalic(doc);
        if (actionMap_.isActionPressed(Action::ToggleUnderline)) cmd::toggleUnderline(doc);
        if (actionMap_.isActionPressed(Action::ToggleStrikethrough)) cmd::toggleStrikethrough(doc);
        if (actionMap_.isActionPressed(Action::ToggleSuperscript)) cmd::toggleSuperscript(doc);
        if (actionMap_.isActionPressed(Action::ToggleSubscript)) cmd::toggleSubscript(doc);

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

        // Clipboard
        if (actionMap_.isActionPressed(Action::Copy)) cmd::copy(doc);
        if (actionMap_.isActionPressed(Action::Cut)) cmd::cut(doc);
        if (actionMap_.isActionPressed(Action::Paste)) cmd::paste(doc);
        if (actionMap_.isActionPressed(Action::SelectAll)) cmd::selectAll(doc);

        // Undo/Redo
        if (actionMap_.isActionPressed(Action::Undo)) {
            if (cmd::undo(doc)) caret::resetBlink(caret);
        }
        if (actionMap_.isActionPressed(Action::Redo)) {
            if (cmd::redo(doc)) caret::resetBlink(caret);
        }
    }
};

// System for handling navigation keys using remappable ActionMap
struct NavigationSystem
    : public afterhours::System<DocumentComponent, CaretComponent,
                                ScrollComponent, LayoutComponent, InputComponent,
                                DialogState> {
    // Last caret row we snapped to. The caret snap only fires when the
    // caret actually moved: otherwise every wheel/scrollbar scroll would
    // be undone on the next frame by snapping back to the caret.
    int lastCaretRow_ = -1;

    void for_each_with(afterhours::Entity& /*entity*/, DocumentComponent& doc,
                       CaretComponent& caret, ScrollComponent& scroll,
                       LayoutComponent& layout, InputComponent& inputComp,
                       DialogState& dialogs,
                       const float) override {
        using input::Action;
        auto& actionMap_ = inputComp.actionMap;

        auto isKeyDownOrSynthetic = [](int key) {
            // Use test_input which handles both real and synthetic keys
            return test_input::is_key_down(key);
        };
        bool shift_down = isKeyDownOrSynthetic(afterhours::keys::LEFT_SHIFT) ||
                          isKeyDownOrSynthetic(afterhours::keys::RIGHT_SHIFT);

        // Print Preview: wheel / PageUp / PageDown page through the
        // preview; caret navigation is skipped (the render stride is
        // previewPage * previewLinesPerPage, not the caret row).
        if (layout.previewMode) {
            float wheelMove = input::getMouseWheelMove();
            if (wheelMove != 0.0f) {
                int wheelLines = scroll_math::accumulateWheel(
                    scroll.wheelRemainder, wheelMove);
                // accumulateWheel returns whole lines in scroll
                // direction; a full page per notch would jump, so each
                // line-step moves one preview page when it crosses zero.
                if (wheelLines < 0) layout::previewPrevPage(layout);
                if (wheelLines > 0) layout::previewNextPage(layout);
            }
            if (actionMap_.isActionPressed(Action::PageUp)) {
                layout::previewPrevPage(layout);
            }
            if (actionMap_.isActionPressed(Action::PageDown)) {
                layout::previewNextPage(layout);
            }
            return;
        }

        // @ chip popup owns Up/Down (it moves the popup selection,
        // not the caret). Any other navigation key pressed while the
        // popup is open dismisses it and then moves the caret
        // normally; a frame with no navigation key leaves the popup
        // alone.
        if (dialogs.showChipPopup) {
            bool up = actionMap_.isActionPressedRepeat(Action::MoveUp);
            bool down = actionMap_.isActionPressedRepeat(Action::MoveDown);
            if (up || down) {
                const auto items = chips::popupItems(
                    cmd::chipPopupFilter(doc, dialogs), std::time(nullptr));
                const int count = static_cast<int>(items.size());
                if (count > 0) {
                    if (up) {
                        dialogs.chipPopupSelected =
                            std::max(0, dialogs.chipPopupSelected - 1);
                    }
                    if (down) {
                        dialogs.chipPopupSelected = std::min(
                            count - 1, dialogs.chipPopupSelected + 1);
                    }
                }
                return;
            }
            bool otherNav =
                actionMap_.isActionPressedRepeat(Action::MoveLeft) ||
                actionMap_.isActionPressedRepeat(Action::MoveRight) ||
                actionMap_.isActionPressedRepeat(Action::MoveWordLeft) ||
                actionMap_.isActionPressedRepeat(Action::MoveWordRight) ||
                actionMap_.isActionPressedRepeat(Action::MoveLineStart) ||
                actionMap_.isActionPressedRepeat(Action::MoveLineEnd) ||
                actionMap_.isActionPressedRepeat(Action::MoveDocumentStart) ||
                actionMap_.isActionPressedRepeat(Action::MoveDocumentEnd) ||
                actionMap_.isActionPressedRepeat(Action::PageUp) ||
                actionMap_.isActionPressedRepeat(Action::PageDown);
            if (otherNav) dialogs.showChipPopup = false;
        }

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
        // Use isActionPressedRepeat for held arrow keys to keep moving
        if (actionMap_.isActionPressedRepeat(Action::MoveWordLeft)) {
            navigateWithSelection([&]() { doc.buffer.moveWordLeft(); });
        } else if (actionMap_.isActionPressedRepeat(Action::MoveLeft)) {
            navigateWithSelection([&]() { doc.buffer.moveLeft(); });
        }
        if (actionMap_.isActionPressedRepeat(Action::MoveWordRight)) {
            navigateWithSelection([&]() { doc.buffer.moveWordRight(); });
        } else if (actionMap_.isActionPressedRepeat(Action::MoveRight)) {
            navigateWithSelection([&]() { doc.buffer.moveRight(); });
        }
        if (actionMap_.isActionPressedRepeat(Action::MoveUp)) {
            navigateWithSelection([&]() { doc.buffer.moveUp(); });
        }
        if (actionMap_.isActionPressedRepeat(Action::MoveDown)) {
            navigateWithSelection([&]() { doc.buffer.moveDown(); });
        }

        // Home/End with Ctrl for document start/end
        if (actionMap_.isActionPressedRepeat(Action::MoveDocumentStart)) {
            navigateWithSelection([&]() { doc.buffer.moveToDocumentStart(); });
        } else if (actionMap_.isActionPressedRepeat(Action::MoveLineStart)) {
            navigateWithSelection([&]() { doc.buffer.moveToLineStart(); });
        }
        if (actionMap_.isActionPressedRepeat(Action::MoveDocumentEnd)) {
            navigateWithSelection([&]() { doc.buffer.moveToDocumentEnd(); });
        } else if (actionMap_.isActionPressedRepeat(Action::MoveLineEnd)) {
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

        // Mouse wheel scrolling: fractional wheel deltas accumulate in
        // scroll.wheelRemainder so small deltas sum to whole lines instead
        // of being truncated away each frame; large flicks apply at once.
        float wheelMove = input::getMouseWheelMove();
        int wheelLines = 0;
        if (wheelMove != 0.0f) {
            wheelLines = scroll_math::accumulateWheel(scroll.wheelRemainder, wheelMove);
            if (layout.splitViewEnabled && shift_down) {
                scroll.secondaryOffset += wheelLines;
                clampSecondary();
            } else {
                scroll.offset += wheelLines;
            }
        }

        // Auto-scroll to keep the caret visible, but only when the caret
        // actually moved (typing, arrows, clicks). Wheel/scrollbar scrolls
        // leave the caret alone, so snapping every frame would undo them.
        {
            int caretRow = static_cast<int>(doc.buffer.caret().row);
            if (caretRow != lastCaretRow_) {
                scroll::scrollToRow(scroll, caretRow);
                lastCaretRow_ = caretRow;
            }
        }
        scroll::clamp(scroll, static_cast<int>(doc.buffer.lineCount()));
        // Pinned at a bound in the direction of travel: drop the remainder
        // so reversing doesn't have to overcome a stale fraction.
        if (wheelLines < 0 && scroll.offset == 0) scroll.wheelRemainder = 0.0f;
        if (wheelLines > 0 && scroll.offset == scroll.maxScroll) scroll.wheelRemainder = 0.0f;
    }
};

// System for auto-saving documents periodically
struct AutoSaveSystem
    : public afterhours::System<DocumentComponent, LayoutComponent> {
    void for_each_with(afterhours::Entity& /*entity*/, DocumentComponent& doc,
                       LayoutComponent& layout,
                       const float) override {
        if (!doc.autoSaveEnabled || !doc.isDirty) {
            return;
        }

        double now = afterhours::graphics::get_time();
        if ((now - doc.lastAutoSaveTime) < doc.autoSaveIntervalSeconds) {
            return;
        }

        if (doc.autoSavePath.empty()) {
            doc.autoSavePath = "output/autosave.wpdoc";
        }

        std::filesystem::create_directories(
            std::filesystem::path(doc.autoSavePath).parent_path());

        cmd::syncLayoutToSettings(doc, layout);
        auto result = saveDocumentEx(doc.buffer, doc.docSettings, doc.autoSavePath);
        if (result.success) {
            doc.lastAutoSaveTime = now;
            toast_notify::info("[saved] Auto-saved", 2.0f);
        } else {
            toast_notify::error("[error] Auto-save failed: " + result.error);
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
        int w = afterhours::graphics::get_screen_width();
        int h = afterhours::graphics::get_screen_height();
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
