#pragma once

#include <ctime>
#include <filesystem>

#include "../util/clipboard.h"
#include "../editor/document_io.h"
#include "../settings.h"
#include "../ui/ui_context.h"
#include "../ui/menu_setup.h"
#include "components.h"

namespace ecs {
namespace cmd {

// --- Revision tracking helpers (moved here as single source of truth) ---

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

// --- Layout <-> DocumentSettings sync ---

inline void syncLayoutToSettings(DocumentComponent& doc, LayoutComponent& layout) {
    doc.docSettings.textStyle = doc.buffer.textStyle();
    doc.docSettings.pageSettings.mode = layout.pageMode;
    doc.docSettings.pageSettings.pageWidth = layout.pageWidth;
    doc.docSettings.pageSettings.pageHeight = layout.pageHeight;
    doc.docSettings.pageSettings.pageMargin = layout.pageMargin;
    doc.docSettings.pageSettings.lineWidthLimit = layout.lineWidthLimit;
}

inline void syncSettingsToLayout(DocumentComponent& doc, LayoutComponent& layout) {
    layout.pageMode = doc.docSettings.pageSettings.mode;
    layout.pageWidth = doc.docSettings.pageSettings.pageWidth;
    layout.pageHeight = doc.docSettings.pageSettings.pageHeight;
    layout.pageMargin = doc.docSettings.pageSettings.pageMargin;
    layout.lineWidthLimit = doc.docSettings.pageSettings.lineWidthLimit;
}

// Refresh the menu bar after a file operation (updates recent files list
// and restores the Track Changes checkmark if needed).
inline void refreshMenuBar(MenuComponent& menu, DocumentComponent& doc,
                           const std::string& filePath) {
    Settings::get().add_recent_file(filePath);
    menu.menus = menu_setup::createMenuBar(
        Settings::get().get_recent_files());
    menu.recentFilesCount = static_cast<int>(
        Settings::get().get_recent_files().size());
    if (doc.trackChangesEnabled &&
        menu.menus.size() > 1 &&
        menu.menus[1].items.size() > 3) {
        menu.menus[1].items[3].mark = win95::MenuMark::Checkmark;
    }
}

// --- File commands ---

inline void newDocument(DocumentComponent& doc) {
    doc.buffer.setText("");
    doc.filePath.clear();
    doc.isDirty = false;
    doc.comments.clear();
    doc.revisions.clear();
    doc.trackChangesBaseline.clear();
}

inline bool saveDocument(DocumentComponent& doc, LayoutComponent& layout,
                         MenuComponent& menu) {
    std::string savePath =
        doc.filePath.empty() ? doc.defaultPath : doc.filePath;
    syncLayoutToSettings(doc, layout);
    auto result = saveDocumentEx(doc.buffer, doc.docSettings, savePath);
    if (result.success) {
        doc.isDirty = false;
        doc.filePath = savePath;
        if (!doc.autoSavePath.empty()) {
            std::filesystem::remove(doc.autoSavePath);
        }
        refreshMenuBar(menu, doc, savePath);
        toast_notify::success(
            "Saved: " +
            std::filesystem::path(savePath).filename().string());
        return true;
    }
    toast_notify::error("Save failed: " + result.error);
    return false;
}

inline bool saveDocumentAs(DocumentComponent& doc, LayoutComponent& layout,
                           MenuComponent& menu, const std::string& path) {
    syncLayoutToSettings(doc, layout);
    auto result = saveDocumentEx(doc.buffer, doc.docSettings, path);
    if (result.success) {
        doc.isDirty = false;
        doc.filePath = path;
        if (!doc.autoSavePath.empty()) {
            std::filesystem::remove(doc.autoSavePath);
        }
        refreshMenuBar(menu, doc, path);
        toast_notify::success(
            "Saved as: " +
            std::filesystem::path(path).filename().string());
        return true;
    }
    toast_notify::error("Save failed: " + result.error);
    return false;
}

inline bool openDocument(DocumentComponent& doc, LayoutComponent& layout,
                         MenuComponent& menu, const std::string& path) {
    auto result = loadDocumentEx(doc.buffer, doc.docSettings, path);
    if (result.success) {
        doc.filePath = path;
        doc.isDirty = false;
        doc.comments.clear();
        doc.revisions.clear();
        syncSettingsToLayout(doc, layout);
        refreshMenuBar(menu, doc, path);
        toast_notify::success(
            "Opened: " +
            std::filesystem::path(path).filename().string());
        return true;
    }
    toast_notify::error("Open failed: " + result.error);
    return false;
}

// --- Edit commands ---

inline bool undo(DocumentComponent& doc) {
    if (doc.buffer.canUndo()) {
        doc.buffer.undo();
        doc.isDirty = true;
        return true;
    }
    return false;
}

inline bool redo(DocumentComponent& doc) {
    if (doc.buffer.canRedo()) {
        doc.buffer.redo();
        doc.isDirty = true;
        return true;
    }
    return false;
}

inline void cut(DocumentComponent& doc) {
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

inline void copy(DocumentComponent& doc) {
    if (doc.buffer.hasSelection()) {
        std::string selected = doc.buffer.getSelectedText();
        if (!selected.empty()) {
            app::clipboard::set_text(selected);
        }
    }
}

inline void paste(DocumentComponent& doc) {
    if (app::clipboard::has_text()) {
        std::string clipText = app::clipboard::get_text();
        std::size_t offset = doc.buffer.caretOffset();
        recordInsertRevision(doc, offset, clipText);
        doc.buffer.insertText(clipText);
        doc.isDirty = true;
    }
}

inline void selectAll(DocumentComponent& doc) {
    doc.buffer.selectAll();
}

// --- Format toggles ---

inline void toggleBold(DocumentComponent& doc) {
    TextStyle style = doc.buffer.textStyle();
    style.bold = !style.bold;
    doc.buffer.setTextStyle(style);
}

inline void toggleItalic(DocumentComponent& doc) {
    TextStyle style = doc.buffer.textStyle();
    style.italic = !style.italic;
    doc.buffer.setTextStyle(style);
}

inline void toggleUnderline(DocumentComponent& doc) {
    TextStyle style = doc.buffer.textStyle();
    style.underline = !style.underline;
    doc.buffer.setTextStyle(style);
}

inline void toggleStrikethrough(DocumentComponent& doc) {
    TextStyle style = doc.buffer.textStyle();
    style.strikethrough = !style.strikethrough;
    doc.buffer.setTextStyle(style);
}

inline void toggleSuperscript(DocumentComponent& doc) {
    TextStyle style = doc.buffer.textStyle();
    style.superscript = !style.superscript;
    if (style.superscript) style.subscript = false;
    doc.buffer.setTextStyle(style);
}

inline void toggleSubscript(DocumentComponent& doc) {
    TextStyle style = doc.buffer.textStyle();
    style.subscript = !style.subscript;
    if (style.subscript) style.superscript = false;
    doc.buffer.setTextStyle(style);
}

}  // namespace cmd
}  // namespace ecs
