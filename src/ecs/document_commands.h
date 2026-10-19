#pragma once

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <filesystem>

#include "../external.h"
#include "../util/clipboard.h"
#include "../editor/date_time.h"
#include "../editor/goto.h"
#include "../editor/document_io.h"
#include "../editor/revisions.h"
#include "../editor/version_history.h"
#include "../editor/table_formula.h"
#include "../editor/outline_view.h"
#include "../editor/templates.h"
#include "../editor/export/export_pdf.h"
#include "../editor/spell_session.h"
#include "../settings.h"
#include "../ui/ui_context.h"
#include "../ui/menu_setup.h"
#include "component_helpers.h"
#include "components.h"

namespace ecs {
namespace cmd {

// Begin the quit flow (title-bar X, File > Exit): prompt when the
// document is dirty, quit immediately when clean. The main loop turns
// DialogState::quitRequested into request_quit.
inline void requestQuit(DocumentComponent& doc, DialogState& dialogs) {
    if (doc.isDirty) {
        dialogs.showQuitPrompt = true;
    } else {
        dialogs.quitRequested = true;
    }
}

// --- Status-bar confirmations (Word 95 channel) ---
// Confirmations ("Comment added", "Track Changes: On", "Saved")
// go to the transient status-bar flash, not a floating toast
// (the rounded green toasts are modern, not Word 95). The
// StatusComponent lives on the editor entity; call sites that
// already hold it (menu actions, dialogs) flash directly, while
// the command layer — which has no status handle — uses this
// lookup. In contexts without one (unit tests) it is a no-op.
// Errors keep their toast_notify::error calls.
// Mirrors the e2e_runner status_text lookup.
inline void flashStatusConfirmation(const std::string& msg,
                                    double duration = 3.0) {
    auto entities = afterhours::EntityQuery({.force_merge = true})
                        .whereHasComponent<StatusComponent>()
                        .gen();
    if (entities.empty()) return;
    auto& status = entities[0].get().get<StatusComponent>();
    status::flash(status, msg, afterhours::graphics::get_time(), duration);
}

// Clear a live transient flash: the status bar reverts to the
// caret/stats line. Same lookup as flashStatusConfirmation —
// the command layer has no status handle — and the same no-op
// in contexts without one (unit tests). Called from the typed
// insert commands only, so typing (not Backspace/Delete/paste)
// ends a flash.
inline void clearStatusFlash() {
    auto entities = afterhours::EntityQuery({.force_merge = true})
                        .whereHasComponent<StatusComponent>()
                        .gen();
    if (entities.empty()) return;
    auto& status = entities[0].get().get<StatusComponent>();
    status::clearFlash(status);
}

// --- Revision tracking helpers (moved here as single source of truth) ---

inline void recordInsertRevision(DocumentComponent& doc, std::size_t offset,
                                 const std::string& text) {
    if (!doc.trackChangesEnabled || text.empty()) return;
    Revision rev;
    rev.type = RevisionType::Insert;
    rev.startOffset = offset;
    rev.text = text;
    rev.author = "Gabe";
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
    rev.author = "Gabe";
    rev.timestamp = std::time(nullptr);
    doc.revisions.push_back(rev);
}

// --- Suggesting mode: per-revision accept/reject (revisions.h) ---
// Accept makes the change permanent, Reject makes it as if the
// change never happened; each is one undo step via replaceRange.
// Accept All / Reject All resolve every record; after either, the
// baseline (the pre-tracking snapshot) equals the current buffer.

inline bool acceptRevision(DocumentComponent& doc, std::size_t index) {
    if (!revisions::acceptRevision(doc.buffer, doc.revisions, index)) {
        return false;
    }
    // Resolving a revision is a document change even when the text
    // itself does not move (the common accept case).
    doc.isDirty = true;
    return true;
}

inline bool rejectRevision(DocumentComponent& doc, std::size_t index) {
    if (!revisions::rejectRevision(doc.buffer, doc.revisions, index)) {
        return false;
    }
    doc.isDirty = true;
    return true;
}

// Group dialog rows: Accept/Reject applies to every member of one
// groupRevisions group, as one undo step (see revisions.h).
inline bool acceptRevisionGroup(DocumentComponent& doc,
                                std::size_t groupIndex) {
    if (!revisions::acceptRevisionGroup(doc.buffer, doc.revisions,
                                        groupIndex)) {
        return false;
    }
    doc.isDirty = true;
    return true;
}

inline bool rejectRevisionGroup(DocumentComponent& doc,
                                std::size_t groupIndex) {
    if (!revisions::rejectRevisionGroup(doc.buffer, doc.revisions,
                                        groupIndex)) {
        return false;
    }
    doc.isDirty = true;
    return true;
}

inline void acceptAllRevisions(DocumentComponent& doc) {
    revisions::acceptAllRevisions(doc.buffer, doc.revisions);
    doc.trackChangesBaseline = doc.buffer.getText();
    flashStatusConfirmation("All changes accepted");
}

inline void rejectAllRevisions(DocumentComponent& doc) {
    // Reverse-apply (not the old baseline restore, which replaced the
    // whole buffer in one history-less setText and also discarded
    // edits made while tracking was off). Each reject is one undo
    // step, so Reject All is undoable revision by revision.
    revisions::rejectAllRevisions(doc.buffer, doc.revisions);
    doc.trackChangesBaseline = doc.buffer.getText();
    doc.isDirty = true;
    flashStatusConfirmation("All changes rejected");
}

// --- @ smart chips ---

// Filter = buffer text between the "@" and the caret.
inline std::string chipPopupFilter(const DocumentComponent& doc,
                                   const DialogState& dialogs) {
    std::size_t at = dialogs.chipPopupAtOffset;
    std::size_t caret = doc.buffer.caretOffset();
    if (caret <= at) return "";
    std::string filter;
    for (std::size_t i = at + 1; i < caret; ++i) {
        filter.push_back(doc.buffer.charAtOffset(i));
    }
    return filter;
}

// Is the popup still anchored: "@" still at its offset, caret
// after it, short filter with no whitespace/newline (typing the
// filter keeps it valid; moving the caret away invalidates it).
inline bool chipPopupValid(const DocumentComponent& doc,
                           const DialogState& dialogs) {
    if (!dialogs.showChipPopup) return false;
    std::size_t at = dialogs.chipPopupAtOffset;
    if (at >= doc.buffer.textSize() ||
        doc.buffer.charAtOffset(at) != '@') {
        return false;
    }
    std::size_t caret = doc.buffer.caretOffset();
    if (caret <= at || caret - at > 49) return false;
    for (std::size_t i = at + 1; i < caret; ++i) {
        char ch = doc.buffer.charAtOffset(i);
        if (std::isspace(static_cast<unsigned char>(ch))) return false;
    }
    return true;
}

// Apply the selected popup item: replace "@" + filter with the
// chip label, as one undo step. Shared by Enter (TextInputSystem)
// and popup clicks (MenuUISystem).
inline bool applyChipPopup(DocumentComponent& doc, DialogState& dialogs) {
    if (!chipPopupValid(doc, dialogs)) {
        dialogs.showChipPopup = false;
        return false;
    }
    std::size_t at = dialogs.chipPopupAtOffset;
    std::size_t caret = doc.buffer.caretOffset();
    auto items = chips::popupItems(chipPopupFilter(doc, dialogs),
                                   std::time(nullptr));
    if (items.empty()) {
        dialogs.showChipPopup = false;
        return false;
    }
    int sel = std::clamp(dialogs.chipPopupSelected, 0,
                         static_cast<int>(items.size()) - 1);
    const ChipItem& item = items[static_cast<std::size_t>(sel)];
    std::string oldText;
    for (std::size_t i = at; i < caret; ++i) {
        oldText.push_back(doc.buffer.charAtOffset(i));
    }
    if (!oldText.empty()) recordDeleteRevision(doc, at, oldText);
    recordInsertRevision(doc, at, item.label);
    bool ok = chips::insertChipAt(doc.buffer, doc.chips, at, caret,
                                  item.kind, item.label);
    dialogs.showChipPopup = false;
    if (ok) {
        doc.isDirty = true;
        // A chip insert is not a repeatable typed run.
        repeat::clear(doc.repeatRecord);
    }
    return ok;
}

// Backspace at a chip's end deletes the whole chip (one undo step).
inline bool eraseChipAtCaret(DocumentComponent& doc) {
    int idx = chips::chipEndingAt(doc.buffer, doc.chips,
                                  doc.buffer.caretOffset());
    if (idx < 0) return false;
    const Chip& chip = doc.chips[static_cast<std::size_t>(idx)];
    recordDeleteRevision(doc, chip.startOffset, chip.label);
    if (chips::eraseChipAt(doc.buffer, doc.chips,
                           static_cast<std::size_t>(idx))) {
        doc.isDirty = true;
        return true;
    }
    return false;
}

// --- Version History (Docs-era) ---
// Snapshots live under settings_dir()/wordproc_versions, except in
// test mode, where they go to output/wordproc_versions (the same
// CWD-relative output convention as the test-mode print PDF, so
// tests never write snapshots into the checkout).

inline std::filesystem::path versionHistoryBaseDir() {
    if (test_input::test_mode) {
        return std::filesystem::path("output") / "wordproc_versions";
    }
    return Settings::get().settings_dir() / "wordproc_versions";
}

inline std::string versionDocName(const DocumentComponent& doc) {
    if (doc.filePath.empty()) return "Untitled";
    return std::filesystem::path(doc.filePath).stem().string();
}

inline std::vector<version_history::VersionSnapshot> versionSnapshots(
    const DocumentComponent& doc) {
    return version_history::listSnapshots(
        versionHistoryBaseDir() /
        version_history::sanitizeDocName(versionDocName(doc)));
}

// Write one snapshot of the current buffer (called on every
// successful Save / Save As).
inline void snapshotVersionHistory(const DocumentComponent& doc) {
    version_history::writeSnapshot(versionHistoryBaseDir(),
                                   versionDocName(doc),
                                   doc.buffer.getText(),
                                   std::time(nullptr));
}

// Restore a snapshot through the command layer: one undo step,
// marks dirty.
inline void restoreVersion(DocumentComponent& doc,
                           const std::string& snapshotText) {
    doc.buffer.replaceRange(0, doc.buffer.textSize(), snapshotText);
    doc.isDirty = true;
}

// --- Layout <-> DocumentSettings sync ---
// DocumentSettings.pageSettings is the source of truth; the layout
// is its display mirror. settings->layout copies the WHOLE page
// block — size, orientation, dims, per-side margins, line-width
// limit — so no field can silently stay stale (Page Setup used
// to write only mode/dims/one margin, and the renderer never saw
// orientation or per-side margins). layout->settings copies back
// the layout-owned fields (mode, dims, margins); size/orientation
// are mirrors, never origins (see below).
// pageMargin (legacy uniform) is NOT synced: per-side margins are
// authoritative, and a single value cannot represent them. The
// Page Setup dialog keeps it mirroring the left margin on OK;
// document_io fills per-side margins from it for legacy files.

inline void syncLayoutToSettings(DocumentComponent& doc, LayoutComponent& layout) {
    doc.docSettings.textStyle = doc.buffer.textStyle();
    doc.docSettings.pageSettings.mode = layout.pageMode;
    // size / orientation are NOT copied back: the layout mirrors
    // them but never originates them, so copying back could
    // clobber settings loaded through a path that does not sync
    // the layout (e.g. the E2E open_file handler).
    doc.docSettings.pageSettings.pageWidth = layout.pageWidth;
    doc.docSettings.pageSettings.pageHeight = layout.pageHeight;
    doc.docSettings.pageSettings.marginTop = layout.marginTop;
    doc.docSettings.pageSettings.marginBottom = layout.marginBottom;
    doc.docSettings.pageSettings.marginLeft = layout.marginLeft;
    doc.docSettings.pageSettings.marginRight = layout.marginRight;
    doc.docSettings.pageSettings.lineWidthLimit = layout.lineWidthLimit;
}

inline void syncSettingsToLayout(DocumentComponent& doc, LayoutComponent& layout) {
    layout.pageMode = doc.docSettings.pageSettings.mode;
    layout.pageSize = doc.docSettings.pageSettings.size;
    layout.pageOrientation = doc.docSettings.pageSettings.orientation;
    layout.pageWidth = doc.docSettings.pageSettings.pageWidth;
    layout.pageHeight = doc.docSettings.pageSettings.pageHeight;
    layout.marginTop = doc.docSettings.pageSettings.marginTop;
    layout.marginBottom = doc.docSettings.pageSettings.marginBottom;
    layout.marginLeft = doc.docSettings.pageSettings.marginLeft;
    layout.marginRight = doc.docSettings.pageSettings.marginRight;
    layout.lineWidthLimit = doc.docSettings.pageSettings.lineWidthLimit;
}

// Apply the document's page settings to the display layout and
// recompute the derived geometry (page fit, display size, text
// area) in one call. The Page Setup dialog OK, and any other
// settings->layout push, goes through here so the paged renderer,
// ruler, hit-test, and Print Preview pagination all consume the
// same values from the next frame.
inline void applyPageSettings(DocumentComponent& doc, LayoutComponent& layout) {
    syncSettingsToLayout(doc, layout);
    layout::updateLayout(layout, layout.screenWidth, layout.screenHeight);
}

// Refresh the menu bar after a file operation (updates recent files list
// and restores the Track Changes checkmark if needed). addRecent=false
// rebuilds the bar (e.g. the Window menu's "1 <file>" label) without
// recording the path as a recent file — used when a template opens as
// a new Untitled document.
inline void refreshMenuBar(MenuComponent& menu, DocumentComponent& doc,
                           const std::string& filePath,
                           bool addRecent = true) {
    if (addRecent) Settings::get().add_recent_file(filePath);
    menu.menus = menu_setup::createMenuBar(
        Settings::get().get_recent_files(), doc.filePath);
    menu.windowDocLabel = menu_setup::windowDocLabel(doc.filePath);
    menu.recentFilesCount = static_cast<int>(
        Settings::get().get_recent_files().size());
    if (doc.trackChangesEnabled &&
        menu.menus.size() > 1 &&
        menu.menus[1].items.size() > 3) {
        menu.menus[1].items[3].mark = win95::MenuMark::Checkmark;
    }
}

inline void newDocument(DocumentComponent& doc);
inline void exitOutlineMode(LayoutComponent& layout,
                            ScrollComponent* scroll);

// File > New from Template...: open a shipped .wpdoc template as a
// NEW Untitled document — filePath cleared, dirty, template content
// + per-line styles adopted, undo history cleared (no undoing into
// the previous document). The template path itself is not recorded
// as a recent file (refreshMenuBar with addRecent=false).
inline bool openTemplateAsNew(DocumentComponent& doc, MenuComponent& menu,
                              const std::string& name) {
    // Blank Document has no file: an empty document, File > New
    // semantics (clean, not dirty).
    if (name == templates::kBlankTemplateName) {
        newDocument(doc);
        refreshMenuBar(menu, doc, "", false);
        return true;
    }
    std::filesystem::path path =
        templates::templateDir() / (name + ".wpdoc");
    TextBuffer tmp;
    DocumentSettings tmpSettings;
    if (!templates::loadTemplate(path, tmp, tmpSettings)) {
        toast_notify::error("Template not found: " + name);
        return false;
    }
    doc.comments.clear();
    doc.revisions.clear();
    doc.chips.clear();
    doc.repeatRecord = ActionRecord{};
    doc.trackChangesBaseline.clear();
    doc.docSettings = tmpSettings;
    templates::adoptTemplate(doc.buffer, tmp, tmpSettings);
    doc.filePath.clear();
    doc.isDirty = true;
    refreshMenuBar(menu, doc, "", false);
    flashStatusConfirmation("Template loaded: " + name);
    return true;
}

// --- File commands ---

inline void newDocument(DocumentComponent& doc) {
    doc.buffer.setText("");
    doc.headerBuffer.setText("");
    doc.footerBuffer.setText("");
    doc.headerBuffer.clearHistory();
    doc.footerBuffer.clearHistory();
    doc.filePath.clear();
    doc.isDirty = false;
    doc.comments.clear();
    doc.revisions.clear();
    doc.chips.clear();
    doc.repeatRecord = ActionRecord{};
    doc.trackChangesBaseline.clear();
    doc.tables.clear();
    doc.cellEdit = CellEditState{};
    doc.tableUndo.clear();
}

// Forward: defined with the TOC commands below; Save / Save As
// regenerate a marked TOC before writing the file.
inline void refreshTocForSave(DocumentComponent& doc,
                              const LayoutComponent& layout);

inline bool saveDocument(DocumentComponent& doc, LayoutComponent& layout,
                         MenuComponent& menu) {
    std::string savePath =
        doc.filePath.empty() ? doc.defaultPath : doc.filePath;
    refreshTocForSave(doc, layout);
    syncLayoutToSettings(doc, layout);
    // WithTables: saveDocumentEx drops tables (its serializeTable
    // helpers were dead code) — a saved table must round-trip.
    // Header/footer text rides the same path (Batch 9).
    auto result = saveDocumentWithTables(doc.buffer, doc.docSettings,
                                         doc.tables, savePath,
                                         &doc.headerBuffer,
                                         &doc.footerBuffer);
    if (result.success) {
        doc.isDirty = false;
        doc.filePath = savePath;
        if (!doc.autoSavePath.empty()) {
            std::filesystem::remove(doc.autoSavePath);
        }
        refreshMenuBar(menu, doc, savePath);
        snapshotVersionHistory(doc);
        flashStatusConfirmation(
            "Saved: " +
            std::filesystem::path(savePath).filename().string());
        return true;
    }
    toast_notify::error("Save failed: " + result.error);
    return false;
}

inline bool saveDocumentAs(DocumentComponent& doc, LayoutComponent& layout,
                           MenuComponent& menu, const std::string& path) {
    refreshTocForSave(doc, layout);
    syncLayoutToSettings(doc, layout);
    auto result = saveDocumentWithTables(doc.buffer, doc.docSettings,
                                         doc.tables, path,
                                         &doc.headerBuffer,
                                         &doc.footerBuffer);
    if (result.success) {
        doc.isDirty = false;
        doc.filePath = path;
        if (!doc.autoSavePath.empty()) {
            std::filesystem::remove(doc.autoSavePath);
        }
        refreshMenuBar(menu, doc, path);
        snapshotVersionHistory(doc);
        flashStatusConfirmation(
            "Saved as: " +
            std::filesystem::path(path).filename().string());
        return true;
    }
    toast_notify::error("Save failed: " + result.error);
    return false;
}

inline bool openDocument(DocumentComponent& doc, LayoutComponent& layout,
                         MenuComponent& menu, const std::string& path) {
    // WithTables: loadDocumentEx drops tables (and leaves the old
    // ones in place); loadDocumentWithTables replaces them, or
    // clears them when the file has none. Header/footer text
    // rides the same path (Batch 9); opening another document
    // ends any header/footer edit on the old one.
    layout::exitHeaderFooterEdit(layout);
    auto result = loadDocumentWithTables(doc.buffer, doc.docSettings,
                                         doc.tables, path,
                                         &doc.headerBuffer,
                                         &doc.footerBuffer);
    if (result.success) {
        doc.filePath = path;
        doc.isDirty = false;
        doc.comments.clear();
        doc.revisions.clear();
        doc.chips.clear();
        doc.repeatRecord = ActionRecord{};
        doc.cellEdit = CellEditState{};
        doc.tableUndo.clear();
        syncSettingsToLayout(doc, layout);
        refreshMenuBar(menu, doc, path);
        flashStatusConfirmation(
            "Opened: " +
            std::filesystem::path(path).filename().string());
        return true;
    }
    toast_notify::error("Open failed: " + result.error);
    return false;
}

// --- Edit commands ---

// Table contents equality for the table-undo guard: dimensions,
// contents, and merge spans (the fields Sort/Formula change or an
// intervening table edit would change).
inline bool tablesEqualForUndo(const Table& a, const Table& b) {
    if (a.rowCount() != b.rowCount() || a.colCount() != b.colCount()) {
        return false;
    }
    for (std::size_t r = 0; r < a.rowCount(); ++r) {
        for (std::size_t c = 0; c < a.colCount(); ++c) {
            const TableCell& ca = a.cell(r, c);
            const TableCell& cb = b.cell(r, c);
            if (ca.content != cb.content || ca.isMerged != cb.isMerged ||
                ca.span.rowSpan != cb.span.rowSpan ||
                ca.span.colSpan != cb.span.colSpan) {
                return false;
            }
        }
    }
    return true;
}

inline bool undo(DocumentComponent& doc) {
    // Table undo stack: pop the top entry and restore `before`
    // only when it is still the latest action for its table
    // (buffer untouched since that op, table still exactly as
    // the op left it). A stale top entry is dropped and the
    // next entry tried — a buffer edit after it makes older
    // entries stale too, so the loop ends at the buffer undo.
    // Any pop ends a coalescing typing run: a later keystroke
    // starts a new one.
    while (!doc.tableUndo.empty()) {
        DocumentComponent::TableUndoEntry entry = doc.tableUndo.back();
        doc.tableUndo.pop_back();
        doc.cellEdit.undoPending = false;
        Table* table = nullptr;
        if (entry.tableIndex < doc.tables.size() &&
            doc.tables[entry.tableIndex].first == entry.line) {
            table = &doc.tables[entry.tableIndex].second;
        } else {
            table = doc.tableAtLine(entry.line);
        }
        if (table && doc.buffer.version() == entry.bufferVersion &&
            tablesEqualForUndo(*table, entry.after)) {
            *table = entry.before;
            // The restored table may be shorter than the live
            // cell caret (e.g. undoing a typing run), and its
            // currentCell may differ (undoing a Tab): sync the
            // cell caret to the restored table, then clamp it.
            if (doc.cellEdit.active &&
                doc.cellEdit.tableIndex < doc.tables.size() &&
                doc.tables[doc.cellEdit.tableIndex].first ==
                    entry.line) {
                doc.cellEdit.cell = table->currentCell();
                cell_edit::clampState(*table, doc.cellEdit);
            }
            doc.isDirty = true;
            return true;
        }
    }
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
    // Cut is not repeatable: it clears the Repeat record.
    repeat::clear(doc.repeatRecord);
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
        doc.repeatRecord = repeat::setText(RepeatKind::Paste, clipText);
    }
}

// --- Typed text (TextInputSystem): one typed unit (a char, a smart
// quote, a Tab's spaces, a newline) inserts through the buffer and
// coalesces into the Repeat InsertText run (repeat::noteTyped).
// Revision records mirror the plain insertChar path below.
inline void insertTypedText(DocumentComponent& doc, const std::string& text) {
    if (text.empty()) return;
    std::size_t offset = doc.buffer.caretOffset();
    bool hadSelection = doc.buffer.hasSelection();
    repeat::noteTyped(doc.repeatRecord, text, offset, hadSelection);
    recordInsertRevision(doc, offset, text);
    doc.buffer.insertText(text);
    doc.isDirty = true;
    // A typed character ends any transient status-bar flash: the
    // bar shows the caret/stats line while editing, not a stale
    // confirmation waiting out its 3s timer.
    clearStatusFlash();
}

inline void insertTypedChar(DocumentComponent& doc, char ch) {
    insertTypedText(doc, std::string(1, ch));
}

inline void selectAll(DocumentComponent& doc) {
    doc.buffer.selectAll();
}

// --- Cell editing commands (Batch 8) ---
// The table at the caret: the cell-edit table while a cell edit
// is active, else the table on the body caret's line. Sort /
// Formula / the Table menu all use this, so they target the
// table being edited even when the body caret sits elsewhere.
inline Table* tableAtCaret(DocumentComponent& doc) {
    if (doc.cellEdit.active &&
        doc.cellEdit.tableIndex < doc.tables.size()) {
        return &doc.tables[doc.cellEdit.tableIndex].second;
    }
    return doc.tableAtLine(doc.buffer.caret().row);
}

inline std::size_t tableLineAtCaret(DocumentComponent& doc) {
    if (doc.cellEdit.active &&
        doc.cellEdit.tableIndex < doc.tables.size()) {
        return doc.tables[doc.cellEdit.tableIndex].first;
    }
    return doc.buffer.caret().row;
}

// The real font measure for cell wrapping / row heights: the
// same measure_text the render pass uses, so a row grows to
// exactly the height its wrapped lines draw at.
inline CellMeasureFn cellMeasure() {
    return [](const std::string& text, int fontSize) {
        return static_cast<float>(
            afterhours::graphics::measure_text(text.c_str(), fontSize));
    };
}

// Enter cell editing: place the cell caret at caretOffset in
// cell (merged-covered cells resolve to their parent). Ends
// any in-progress typing run in the previous cell. Does not
// mark dirty (entering is not a modification); clears the body
// selection (a click into a table deselects body text).
inline bool beginCellEdit(DocumentComponent& doc,
                          std::size_t tableIndex, CellPosition cell,
                          std::size_t caretOffset) {
    if (tableIndex >= doc.tables.size()) return false;
    doc.cellEdit.undoPending = false;
    doc.cellEdit.active = true;
    doc.cellEdit.tableIndex = tableIndex;
    doc.cellEdit.cell = cell;
    doc.cellEdit.caretOffset = caretOffset;
    doc.cellEdit.anchorOffset = caretOffset;
    doc.cellEdit.hasSelection = false;
    Table& table = doc.tables[tableIndex].second;
    cell_edit::clampState(table, doc.cellEdit);
    doc.buffer.clearSelection();
    return true;
}

// Exit cell editing back to the body caret (click outside any
// table, or Esc). Ends the typing run; the snapshot entry stays
// live until cmd::undo consumes or drops it.
inline void endCellEdit(DocumentComponent& doc) {
    doc.cellEdit.active = false;
    doc.cellEdit.undoPending = false;
    doc.cellEdit.hasSelection = false;
}

// End the current typing run without leaving the cell
// (navigation, Tab, clipboard ops): the next text edit starts
// a fresh snapshot entry.
inline void commitCellEditRun(DocumentComponent& doc) {
    doc.cellEdit.undoPending = false;
}

// Record a cell mutation as one undo step. When a typing run
// is already coalescing into the stack's top entry (same
// table, cellEditRun), update its `after` in place; otherwise
// push before/after as a new entry. Callers capture `before`
// BEFORE mutating, and pass the mutated table. No-op entries
// (mutation changed nothing) are never created by callers.
inline void recordCellEdit(DocumentComponent& doc,
                           std::size_t tableIndex, std::size_t line,
                           const Table& before, const Table& after,
                           bool coalesce) {
    if (coalesce && doc.cellEdit.undoPending &&
        !doc.tableUndo.empty() && doc.tableUndo.back().cellEditRun &&
        doc.tableUndo.back().line == line &&
        doc.tableUndo.back().tableIndex == tableIndex) {
        doc.tableUndo.back().after = after;
        return;
    }
    doc.tableUndo.push_back(DocumentComponent::TableUndoEntry{
        line, tableIndex, before, after, doc.buffer.version(),
        coalesce});
    doc.cellEdit.undoPending = coalesce;
    doc.isDirty = true;
}

// Index of a table pointer in doc.tables (for undo entries).
inline std::size_t tableIndexOf(DocumentComponent& doc,
                                const Table* table) {
    for (std::size_t i = 0; i < doc.tables.size(); ++i) {
        if (&doc.tables[i].second == table) return i;
    }
    return 0;
}

// Insert text into the active cell (typing, newline, paste):
// one undo step per typing run, coalesced while the run
// continues. Replaces a selection. Returns false when no cell
// edit is active or nothing changed.
inline bool insertCellText(DocumentComponent& doc,
                           const std::string& text) {
    if (!doc.cellEdit.active ||
        doc.cellEdit.tableIndex >= doc.tables.size()) {
        return false;
    }
    Table& table = doc.tables[doc.cellEdit.tableIndex].second;
    Table before = table;
    if (!cell_edit::insertText(table, doc.cellEdit, text)) return false;
    // Recompute heights for the edited row (and any row a
    // pasted newline / selection delete could have affected —
    // recompute is cheap for table-sized grids).
    cell_edit::recomputeRowHeights(table, cellMeasure());
    recordCellEdit(doc, doc.cellEdit.tableIndex,
                   doc.tables[doc.cellEdit.tableIndex].first,
                   before, table, true);
    // A typed character ends any transient status-bar flash (same
    // as the body path): the bar shows the stats line while editing.
    clearStatusFlash();
    return true;
}

inline bool backspaceCell(DocumentComponent& doc) {
    if (!doc.cellEdit.active ||
        doc.cellEdit.tableIndex >= doc.tables.size()) {
        return false;
    }
    Table& table = doc.tables[doc.cellEdit.tableIndex].second;
    Table before = table;
    if (!cell_edit::backspace(table, doc.cellEdit)) return false;
    cell_edit::recomputeRowHeights(table, cellMeasure());
    recordCellEdit(doc, doc.cellEdit.tableIndex,
                   doc.tables[doc.cellEdit.tableIndex].first,
                   before, table, true);
    return true;
}

inline bool deleteCell(DocumentComponent& doc) {
    if (!doc.cellEdit.active ||
        doc.cellEdit.tableIndex >= doc.tables.size()) {
        return false;
    }
    Table& table = doc.tables[doc.cellEdit.tableIndex].second;
    Table before = table;
    if (!cell_edit::del(table, doc.cellEdit)) return false;
    cell_edit::recomputeRowHeights(table, cellMeasure());
    recordCellEdit(doc, doc.cellEdit.tableIndex,
                   doc.tables[doc.cellEdit.tableIndex].first,
                   before, table, true);
    return true;
}

// Tab / Shift+Tab in a cell: ends the typing run first.
// Navigation itself is NOT an undo step (like arrow keys):
// only Tab in the last cell appending a row records one,
// because that mutates the document.
inline bool tabCell(DocumentComponent& doc, bool shift) {
    if (!doc.cellEdit.active ||
        doc.cellEdit.tableIndex >= doc.tables.size()) {
        return false;
    }
    commitCellEditRun(doc);
    Table& table = doc.tables[doc.cellEdit.tableIndex].second;
    std::size_t rowsBefore = table.rowCount();
    Table before = table;
    bool moved = shift ? cell_edit::tabBackward(table, doc.cellEdit)
                       : cell_edit::tabForward(table, doc.cellEdit);
    if (!moved) return false;
    if (table.rowCount() != rowsBefore) {
        cell_edit::recomputeRowHeights(table, cellMeasure());
        recordCellEdit(doc, doc.cellEdit.tableIndex,
                       doc.tables[doc.cellEdit.tableIndex].first,
                       before, table, false);
    }
    return true;
}

// Clipboard in a cell (KeyboardShortcutSystem routes Copy /
// Cut / Paste / Select All here while a cell edit is active,
// so they never touch the body buffer). Cut and Paste are each
// one undo step, ending the typing run.
inline bool copyCell(DocumentComponent& doc) {
    if (!doc.cellEdit.active ||
        doc.cellEdit.tableIndex >= doc.tables.size()) {
        return false;
    }
    const Table& table = doc.tables[doc.cellEdit.tableIndex].second;
    std::string selected = doc.cellEdit.selectedText(
        table.cell(doc.cellEdit.cell).content);
    if (selected.empty()) return false;
    app::clipboard::set_text(selected);
    return true;
}

inline bool cutCell(DocumentComponent& doc) {
    if (!doc.cellEdit.active ||
        doc.cellEdit.tableIndex >= doc.tables.size()) {
        return false;
    }
    Table& table = doc.tables[doc.cellEdit.tableIndex].second;
    if (!doc.cellEdit.hasSelection) return false;
    std::string selected = doc.cellEdit.selectedText(
        table.cell(doc.cellEdit.cell).content);
    if (selected.empty()) return false;
    commitCellEditRun(doc);
    repeat::clear(doc.repeatRecord);
    Table before = table;
    app::clipboard::set_text(selected);
    if (!cell_edit::insertText(table, doc.cellEdit, "")) return false;
    cell_edit::recomputeRowHeights(table, cellMeasure());
    recordCellEdit(doc, doc.cellEdit.tableIndex,
                   doc.tables[doc.cellEdit.tableIndex].first,
                   before, table, false);
    return true;
}

inline bool pasteCell(DocumentComponent& doc) {
    if (!doc.cellEdit.active ||
        doc.cellEdit.tableIndex >= doc.tables.size()) {
        return false;
    }
    if (!app::clipboard::has_text()) return false;
    std::string clipText = app::clipboard::get_text();
    if (clipText.empty()) return false;
    commitCellEditRun(doc);
    Table& table = doc.tables[doc.cellEdit.tableIndex].second;
    Table before = table;
    if (!cell_edit::insertText(table, doc.cellEdit, clipText)) {
        return false;
    }
    cell_edit::recomputeRowHeights(table, cellMeasure());
    recordCellEdit(doc, doc.cellEdit.tableIndex,
                   doc.tables[doc.cellEdit.tableIndex].first,
                   before, table, false);
    doc.repeatRecord = repeat::setText(RepeatKind::Paste, clipText);
    return true;
}

inline void selectAllCell(DocumentComponent& doc) {
    if (!doc.cellEdit.active ||
        doc.cellEdit.tableIndex >= doc.tables.size()) {
        return;
    }
    Table& table = doc.tables[doc.cellEdit.tableIndex].second;
    cell_edit::selectAllInCell(table, doc.cellEdit);
}

// --- Table commands (Table menu) ---
// Sort: sorts the table at the caret's line by the caret cell's
// column, all rows (no header concept in the model), as one undo
// step (before/after snapshot in doc.tableUndo; see cmd::undo).
// Formula...: opens the dialog (prefilled =SUM(ABOVE)) when a
// table is at the caret; OK evaluates against the caret cell and
// writes the result into it, also as one undo step. A failed
// evaluation leaves the dialog open with the error message.

inline bool sortTable(DocumentComponent& doc, bool ascending) {
    std::size_t line = tableLineAtCaret(doc);
    Table* table = tableAtCaret(doc);
    if (table == nullptr) {
        toast_notify::error("No table at cursor");
        return false;
    }
    Table before = *table;
    if (!table->sortRows(table->currentCell().col, ascending)) {
        toast_notify::error("Cannot sort table with merged cells");
        return false;
    }
    doc.tableUndo.push_back(DocumentComponent::TableUndoEntry{
        line, tableIndexOf(doc, table), before, *table,
        doc.buffer.version(), false});
    // A Sort supersedes any in-progress typing run (it pushes
    // above the run's entry) and the caret cell follows its
    // row (sortRows moved Table::currentCell): sync the cell
    // caret to it.
    doc.cellEdit.undoPending = false;
    if (doc.cellEdit.active) {
        doc.cellEdit.cell = table->currentCell();
        cell_edit::clampState(*table, doc.cellEdit);
    }
    doc.isDirty = true;
    flashStatusConfirmation(ascending ? "Table sorted ascending"
                                 : "Table sorted descending");
    return true;
}

inline bool openTableFormulaDialog(DocumentComponent& doc,
                                   DialogState& dialogs) {
    Table* table = tableAtCaret(doc);
    if (table == nullptr) {
        toast_notify::error("No table at cursor");
        return false;
    }
    dialogs.showTableFormulaDialog = true;
    dialogs.tableFormulaStr = "=SUM(ABOVE)";
    dialogs.tableFormulaError.clear();
    return true;
}

inline bool applyTableFormula(DocumentComponent& doc, DialogState& dialogs) {
    std::size_t line = tableLineAtCaret(doc);
    Table* table = tableAtCaret(doc);
    if (table == nullptr) {
        dialogs.tableFormulaError = "No table at cursor";
        return false;
    }
    CellPosition cell = table->currentCell();
    table_formula::FormulaResult result = table_formula::evaluate(
        *table, cell.row, cell.col, dialogs.tableFormulaStr);
    if (!result.ok) {
        dialogs.tableFormulaError = result.error;
        return false;
    }
    Table before = *table;
    table->setCellContent(cell.row, cell.col, result.value);
    doc.tableUndo.push_back(DocumentComponent::TableUndoEntry{
        line, tableIndexOf(doc, table), before, *table,
        doc.buffer.version(), false});
    // A Formula supersedes any in-progress typing run; clamp
    // the cell caret to the rewritten cell.
    doc.cellEdit.undoPending = false;
    if (doc.cellEdit.active) {
        cell_edit::clampState(*table, doc.cellEdit);
    }
    doc.isDirty = true;
    dialogs.tableFormulaError.clear();
    dialogs.showTableFormulaDialog = false;
    return true;
}

// Insert > Date and Time...: inserts the chosen formatted string at
// the caret (replacing a selection, like typing) as ONE undo step via
// replaceRange (insertText would record one undo step per character).
// "Update automatically" has no field engine behind it: the dialog
// inserts the same static text either way (noted in the dialog code).
inline void insertDateTime(DocumentComponent& doc, const std::string& text) {
    if (text.empty()) return;
    CaretPosition start = doc.buffer.hasSelection()
                              ? doc.buffer.selectionStart()
                              : doc.buffer.caret();
    CaretPosition end = doc.buffer.hasSelection()
                            ? doc.buffer.selectionEnd()
                            : start;
    std::size_t offset = doc.buffer.offsetForPosition(start);
    std::size_t length = doc.buffer.offsetForPosition(end) - offset;
    std::string oldText;
    oldText.reserve(length);
    for (std::size_t i = 0; i < length; ++i) {
        oldText.push_back(doc.buffer.charAtOffset(offset + i));
    }
    if (!oldText.empty()) recordDeleteRevision(doc, offset, oldText);
    recordInsertRevision(doc, offset, text);
    doc.buffer.clearSelection();
    doc.buffer.replaceRange(offset, length, text);
    doc.isDirty = true;
}

// --- Table of Contents (Batch 11) ---
// linesPerPage is the Batch 6 estimate the status bar, Print
// Preview, and the Normal-view break lines share: page text
// height / scaled line height. The TOC's page numbers come from
// the same math, so a heading's TOC page matches its status-bar
// page in the same layout.
inline int tocLinesPerPage(const DocumentComponent& doc,
                           const LayoutComponent& layout) {
    float ds = layout::displayScale(layout);
    int lineHeight =
        ruler_math::scaledLineHeight(doc.buffer.textStyle().fontSize, ds);
    return static_cast<int>(columns_math::linesPerColumnForPage(
        layout.pageHeight, layout.marginTop, layout.marginBottom, ds,
        lineHeight));
}

// Measured TOC layout: the dot leaders are counted in pixels
// against the tab stop at the text area's right edge, so the page
// number lands flush right instead of floating mid-line after a
// fixed character column. The target is the effective text area's
// text span (effective width minus the 8px text padding on each
// side and the line-number gutter when shown — the same span the
// ruler geometry uses); the measure is theme::MeasureUIText at
// the renderer's body font size (the global font size scaled by
// displayScale, the baseFontSize the render pass gives Normal
// rows). Recomputed on every insert / update / save-refresh, so
// a resize, zoom, or margin change re-flows the leaders.
inline toc::Layout tocLayout(const DocumentComponent& doc,
                             const LayoutComponent& layout) {
    const auto effective = layout::effectiveTextArea(layout);
    const float gutter = layout.showLineNumbers && !layout.previewMode
                             ? layout.lineNumberGutterWidth
                             : 0.0f;
    const int target =
        static_cast<int>(effective.width) -
        2 * static_cast<int>(ruler_math::TEXT_PADDING) -
        static_cast<int>(gutter);
    const int bodyPx = ruler_math::scaledFontSize(
        doc.buffer.textStyle().fontSize, layout::displayScale(layout));
    toc::Layout result;
    result.targetWidthPx = std::max(0, target);
    result.measure = [bodyPx](const std::string& text) {
        return theme::MeasureUIText(text.c_str(), bodyPx);
    };
    return result;
}

// Insert > Table of Contents: inserts at the caret, or replaces
// the existing marked block when one exists. One undo step.
inline bool insertToc(DocumentComponent& doc,
                      const LayoutComponent& layout) {
    if (!doc.buffer.insertTableOfContents(tocLinesPerPage(doc, layout),
                                          tocLayout(doc, layout))) {
        flashStatusConfirmation("No headings for a table of contents");
        return false;
    }
    doc.isDirty = true;
    flashStatusConfirmation("Table of Contents inserted");
    return true;
}

// Insert > Update Table of Contents (and the right-click context
// menu): regenerates the marked block in place. No-op + flash
// when there is no marked block.
inline bool updateToc(DocumentComponent& doc,
                      const LayoutComponent& layout) {
    if (!doc.buffer.hasToc()) {
        flashStatusConfirmation("No table of contents to update");
        return false;
    }
    doc.buffer.updateTableOfContents(tocLinesPerPage(doc, layout),
                                     tocLayout(doc, layout));
    doc.isDirty = true;
    flashStatusConfirmation(!doc.buffer.hasToc()
                                ? "Table of Contents removed (no headings)"
                                : "Table of Contents updated");
    return true;
}

// Explicit Save / Save As regenerate a marked TOC first, so the
// saved file's entries (page numbers especially) match the
// document as saved. Already-fresh blocks are a no-op in the
// buffer (no undo step, no dirty churn).
inline void refreshTocForSave(DocumentComponent& doc,
                              const LayoutComponent& layout) {
    if (doc.buffer.hasToc()) {
        doc.buffer.updateTableOfContents(tocLinesPerPage(doc, layout),
                                         tocLayout(doc, layout));
    }
}

// Insert > Special Character...: inserts one symbol character at
// the caret (replacing a selection, like typing) as ONE undo step
// via replaceRange — insertText would record one undo step per
// UTF-8 byte. Same shape as insertDateTime above.
inline void insertSymbol(DocumentComponent& doc, const std::string& text) {
    if (text.empty()) return;
    CaretPosition start = doc.buffer.hasSelection()
                              ? doc.buffer.selectionStart()
                              : doc.buffer.caret();
    CaretPosition end = doc.buffer.hasSelection()
                            ? doc.buffer.selectionEnd()
                            : start;
    std::size_t offset = doc.buffer.offsetForPosition(start);
    std::size_t length = doc.buffer.offsetForPosition(end) - offset;
    std::string oldText;
    oldText.reserve(length);
    for (std::size_t i = 0; i < length; ++i) {
        oldText.push_back(doc.buffer.charAtOffset(offset + i));
    }
    if (!oldText.empty()) recordDeleteRevision(doc, offset, oldText);
    recordInsertRevision(doc, offset, text);
    doc.buffer.clearSelection();
    doc.buffer.replaceRange(offset, length, text);
    doc.isDirty = true;
}

// --- Insert commands (Insert menu + Standard toolbar share these) ---

// Insert > Hyperlink... (Ctrl+K): applies the default URL to the
// selection; with no selection it reports instead of inserting.
inline void insertHyperlink(DocumentComponent& doc) {
    if (doc.buffer.hasSelection()) {
        if (doc.buffer.addHyperlink("https://example.com")) {
            doc.isDirty = true;
            flashStatusConfirmation("Hyperlink added (edit URL)");
        }
    } else {
        toast_notify::error("Select text first");
    }
}

// Insert > Image...: placeholder image at the caret (no file
// picker exists yet — same honest placeholder the menu inserts).
inline void insertImagePlaceholder(DocumentComponent& doc) {
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
    flashStatusConfirmation("Image placeholder inserted");
}

// --- Header / footer editing (Batch 9) ---
// The buffer for the active header/footer target (or the body
// buffer when not editing one — callers that need "header only"
// check layout.headerEditTarget first).
inline TextBuffer& headerFooterBuffer(DocumentComponent& doc,
                                      LayoutComponent& layout) {
    if (layout.headerEditTarget == HeaderEditTarget::Footer) {
        return doc.footerBuffer;
    }
    return doc.headerBuffer;
}

inline const TextBuffer& headerFooterBuffer(
    const DocumentComponent& doc, const LayoutComponent& layout) {
    if (layout.headerEditTarget == HeaderEditTarget::Footer) {
        return doc.footerBuffer;
    }
    return doc.headerBuffer;
}

// Enter header/footer editing on `target`: a cell edit ends
// first (one editing mode at a time), Outline ends (it forces
// Pageless, header editing forces Paged), and entering while
// already editing just switches the target (layout::enter*
// keeps headerPrevPageMode). Preview ends too: the preview
// toolbar replaces the Formatting bar the strip needs.
inline void enterHeaderFooterEdit(DocumentComponent& doc,
                                  LayoutComponent& layout,
                                  ScrollComponent* scroll,
                                  HeaderEditTarget target) {
    endCellEdit(doc);
    if (layout.previewMode) layout::exitPrintPreview(layout);
    if (layout.showOutline) exitOutlineMode(layout, scroll);
    layout::enterHeaderFooterEdit(layout, target);
    flashStatusConfirmation(target == HeaderEditTarget::Footer
                                ? "Footer"
                                : "Header");
}

// Exit back to the body caret (toolbar Close, Esc, Insert >
// Document Body, View > Header and Footer toggle).
inline void exitHeaderFooterEdit(LayoutComponent& layout) {
    layout::exitHeaderFooterEdit(layout);
    flashStatusConfirmation("Document body");
}

// The Header/Footer strip's "Page Number": toggles the
// page-number field in the ACTIVE target's center section
// (HeaderFooter::center.showPageNumber). The render pass draws
// "Page N" centered from that flag; the config persists with
// the document. Enabling the field also marks the section
// enabled so a header with only a page number still renders.
inline void toggleHeaderFooterPageNumber(DocumentComponent& doc,
                                         LayoutComponent& layout) {
    if (layout.headerEditTarget == HeaderEditTarget::None) return;
    HeaderFooter& config =
        layout.headerEditTarget == HeaderEditTarget::Footer
            ? doc.docSettings.footer
            : doc.docSettings.header;
    config.center.showPageNumber = !config.center.showPageNumber;
    if (config.center.showPageNumber) config.enabled = true;
    doc.isDirty = true;
    flashStatusConfirmation(config.center.showPageNumber
                                ? "Page number: On"
                                : "Page number: Off");
}

// Clipboard / Select All on an arbitrary TextBuffer (the
// header/footer buffers): the body commands above hard-code
// doc.buffer, these take the buffer so the header-mode
// keyboard shortcuts route to the active header/footer buffer.
inline void copyBuffer(TextBuffer& buffer) {
    if (buffer.hasSelection()) {
        std::string selected = buffer.getSelectedText();
        if (!selected.empty()) app::clipboard::set_text(selected);
    }
}

inline void cutBuffer(TextBuffer& buffer, DocumentComponent& doc) {
    repeat::clear(doc.repeatRecord);
    if (buffer.hasSelection()) {
        std::string selected = buffer.getSelectedText();
        if (!selected.empty()) {
            app::clipboard::set_text(selected);
            buffer.deleteSelection();
            doc.isDirty = true;
        }
    }
}

inline void pasteBuffer(TextBuffer& buffer, DocumentComponent& doc) {
    if (app::clipboard::has_text()) {
        std::string clipText = app::clipboard::get_text();
        buffer.insertText(clipText);
        doc.isDirty = true;
    }
}

// --- Go To (Batch 9, Edit > Go To... / Ctrl+G) ---
// Word 95's ONE Go To dialog: Page / Line / Bookmark
// targets, a number/name field, Go To + Close. Page uses
// the Batch 6 page math (lines-per-page estimate); Line is
// 1-based and clamped; Bookmark jumps through
// TextBuffer::goToBookmark and flashes when the name does
// not exist. The dialog stays open after a jump (Word does)
// so the user can jump again; Close / Esc dismisses it.
// Opening exits header/footer editing (the jump targets
// the body) and Print Preview.
//
// Dialog-local helpers for the number field live here so
// the dialog (MenuUISystem) and the unit tests share the
// one jump implementation.
inline void openGoToDialog(LayoutComponent& layout,
                           DialogState& dialogs) {
    layout::exitHeaderFooterEdit(layout);
    if (layout.previewMode) layout::exitPrintPreview(layout);
    dialogs.goToTarget = 0;  // Page
    dialogs.goToInputStr = "1";
    dialogs.showGoToDialog = true;
    flashStatusConfirmation("Go To");
}

inline bool performGoTo(DocumentComponent& doc, LayoutComponent& layout,
                        ScrollComponent* scroll, DialogState& dialogs) {
    TextBuffer& buffer = doc.buffer;
    auto snapScroll = [&](int caretLine) {
        if (scroll == nullptr) return;
        if (caretLine < scroll->offset) {
            scroll->offset = caretLine;
        } else if (caretLine >= scroll->offset + scroll->visibleLines) {
            scroll->offset = caretLine - scroll->visibleLines + 1;
        }
        if (scroll->offset < 0) scroll->offset = 0;
    };
    if (dialogs.goToTarget == 2) {  // Bookmark
        std::string name = dialogs.goToInputStr;
        std::size_t begin = name.find_first_not_of(" \t\r\n");
        std::size_t end = name.find_last_not_of(" \t\r\n");
        if (begin == std::string::npos) {
            flashStatusConfirmation("Enter a bookmark name");
            return false;
        }
        name = name.substr(begin, end - begin + 1);
        if (buffer.goToBookmark(name)) {
            snapScroll(static_cast<int>(buffer.caret().row));
            flashStatusConfirmation("Go to bookmark: " + name);
            return true;
        }
        flashStatusConfirmation("Bookmark not found: " + name);
        return false;
    }
    int number = goto_math::parseGoToNumber(dialogs.goToInputStr);
    if (dialogs.goToTarget == 0) {  // Page
        if (number < 0) {
            flashStatusConfirmation("Enter a valid page number");
            return false;
        }
        float ds = layout::displayScale(layout);
        int lineHeight = buffer.textStyle().fontSize + 4;
        std::size_t linesPerPage = columns_math::linesPerColumnForPage(
            layout.pageHeight, layout.marginTop, layout.marginBottom, ds,
            lineHeight);
        std::size_t firstLine = goto_math::firstLineOfPage(
            buffer.lineCount(), linesPerPage, number);
        buffer.setCaret(CaretPosition{firstLine, 0});
        if (scroll != nullptr) {
            scroll->offset = static_cast<int>(firstLine);
        }
        // The page actually landed on (after clamping), for
        // the confirmation flash.
        int page = static_cast<int>(firstLine / linesPerPage) + 1;
        flashStatusConfirmation("Go to page " + std::to_string(page));
        return true;
    }
    // Line (1-based, clamped).
    if (number < 0) {
        flashStatusConfirmation("Enter a valid line number");
        return false;
    }
    std::size_t line = goto_math::clampedLine(buffer.lineCount(), number);
    buffer.setCaret(CaretPosition{line, 0});
    snapScroll(static_cast<int>(line));
    flashStatusConfirmation("Go to line " + std::to_string(line + 1));
    return true;
}

// --- Print Preview / Print (File menu, Ctrl+P) ---

inline void openPrintPreview(DocumentComponent& doc, LayoutComponent& layout) {
    // Preview is read-only: a cell edit ends on entry (its
    // typing run commits; the snapshot entry stays undoable).
    // A header/footer edit ends too (its toolbar strip is
    // replaced by the preview toolbar).
    endCellEdit(doc);
    layout::exitHeaderFooterEdit(layout);
    layout::enterPrintPreview(layout, doc.buffer.lineCount(),
                              doc.buffer.textStyle().fontSize);
}

inline void closePrintPreview(LayoutComponent& layout) {
    layout::exitPrintPreview(layout);
}

// File > Print... / Ctrl+P / preview-toolbar Print: render the document
// to PDF via the existing export_pdf path, then hand the file to the OS
// printer (`lp`). In test mode the PDF is still written (so tests can
// assert it exists) but `lp` is NEVER invoked — printing from a test
// would spool a real job. HONEST PARTIAL: export_pdf writes a single
// page and lets long documents overflow past its bottom margin — Print
// prints what's supported, not a true multi-page job.
inline void requestPrint(DocumentComponent& doc, DialogState& dialogs) {
    dialogs.printRequested = true;
    dialogs.printPdfExists = false;
    std::filesystem::path pdfPath;
    if (test_input::test_mode) {
        // Deterministic relative path so E2E can assert on it (same
        // convention as the Export PDF path: relative to the CWD the
        // test runner launched from).
        std::filesystem::create_directories("output");
        pdfPath = std::filesystem::path("output") / "print_test.pdf";
    } else {
        pdfPath = std::filesystem::temp_directory_path() / "wordproc_print.pdf";
    }
    dialogs.printPdfPath = pdfPath.string();
    auto result = exportDocumentPdf(doc.buffer, doc.docSettings, pdfPath.string());
    dialogs.printPdfExists =
        result.success && std::filesystem::exists(pdfPath);
    if (!result.success) {
        toast_notify::error("Print failed: PDF export failed: " + result.error);
        return;
    }
    if (test_input::test_mode) return;  // Never spool from a test.
    std::string cmd = "lp \"" + pdfPath.string() + "\"";
    std::system(cmd.c_str());
}

// --- Grammar dialog commands (Tools > Grammar...) ---
// Interactive, mirroring the Spelling dialog: one issue at a time
// at grammarIndex. Ignore Once advances; Ignore All session-ignores
// this rule + this phrase (other instances of the same rule, or the
// same phrase under another rule, are unaffected); Change applies
// the suggestion through the buffer as one undo step, when a
// concrete suggestion exists. EVERY action re-scans the document
// and lands on the next still-existing issue, and recomputes
// grammarNoErrors from the re-scanned list — a stale index or a
// stale noErrors flag never leaves a fixed issue (or a complete
// screen for a document that still has issues) on display. When no
// issue remains, the dialog shows its clean state ("No grammar
// issues found." in the fields, Change/Ignore disabled).
// The dialog never renders grammar underlines: GrammarChecker has
// no render path (grammar issues are NOT spell squiggles), and the
// one squiggle the old screenshot showed under "sat." was a spell
// squiggle — "sat" is not in the dictionary. Dialog only.

// Human wording for the current issue: the checker's message when
// present (already human); the rule display name as fallback.
// The enum name (e.g. REPEATED_WORD) is never shown.
inline std::string grammarIssueLabel(const GrammarError& err) {
    return err.message.empty() ? grammarRuleDisplayName(err.ruleId)
                               : err.message;
}

inline const GrammarError* grammarCurrentError(const DialogState& dialogs) {
    if (dialogs.grammarNoErrors) return nullptr;
    if (dialogs.grammarIndex >= dialogs.grammarErrors.size()) {
        return nullptr;
    }
    return &dialogs.grammarErrors[dialogs.grammarIndex];
}

// Re-check the document and rebuild the issue list, dropping
// session-ignored (rule + phrase) issues. Callers own grammarIndex
// / grammarNoErrors.
inline void refreshGrammarDialog(DocumentComponent& doc,
                                 DialogState& dialogs) {
    GrammarChecker checker;
    auto all = checker.checkText(doc.buffer.getText());
    dialogs.grammarErrors.clear();
    for (auto& err : all) {
        if (dialogs.grammarIgnored.count(grammarIgnoreKey(err)) == 0) {
            dialogs.grammarErrors.push_back(std::move(err));
        }
    }
}

inline void openGrammarDialog(DocumentComponent& doc, DialogState& dialogs) {
    dialogs.showGrammarDialog = true;
    dialogs.grammarIndex = 0;
    dialogs.grammarNoErrors = false;
    refreshGrammarDialog(doc, dialogs);
    if (dialogs.grammarErrors.empty()) dialogs.grammarNoErrors = true;
}

inline void grammarIgnoreOnce(DocumentComponent& doc,
                              DialogState& dialogs) {
    const GrammarError* current = grammarCurrentError(dialogs);
    if (!current) return;
    // Copy the ignored issue: refresh rebuilds grammarErrors and
    // would dangle the pointer.
    GrammarError ignored = *current;
    refreshGrammarDialog(doc, dialogs);
    // The ignored issue still exists in the re-scanned list (the
    // document did not change); the next issue is the one after it
    // in the new list. If the re-scan no longer contains it, fall
    // back to the first issue at/after the end of its span. Either
    // way grammarNoErrors is recomputed from the new list, never
    // left at a stale value.
    std::size_t next = dialogs.grammarErrors.size();
    for (std::size_t i = 0; i < dialogs.grammarErrors.size(); ++i) {
        const GrammarError& err = dialogs.grammarErrors[i];
        if (err.offset == ignored.offset &&
            err.length == ignored.length &&
            err.ruleId == ignored.ruleId) {
            next = i + 1;
            break;
        }
    }
    if (next == dialogs.grammarErrors.size()) {
        for (std::size_t i = 0; i < dialogs.grammarErrors.size(); ++i) {
            if (dialogs.grammarErrors[i].offset >=
                ignored.offset + ignored.length) {
                next = i;
                break;
            }
        }
    }
    dialogs.grammarIndex = next;
    dialogs.grammarNoErrors = next >= dialogs.grammarErrors.size();
}

inline void grammarIgnoreAll(DocumentComponent& doc,
                             DialogState& dialogs) {
    const GrammarError* current = grammarCurrentError(dialogs);
    if (!current) return;
    std::size_t offset = current->offset;
    dialogs.grammarIgnored.insert(grammarIgnoreKey(*current));
    refreshGrammarDialog(doc, dialogs);
    // The ignored issues are gone from the re-scanned list, so the
    // next still-existing issue is the first at/after the ignored
    // span's start; if none does, the check is complete.
    // grammarNoErrors is recomputed, not left stale.
    std::size_t next = dialogs.grammarErrors.size();
    for (std::size_t i = 0; i < dialogs.grammarErrors.size(); ++i) {
        if (dialogs.grammarErrors[i].offset >= offset) {
            next = i;
            break;
        }
    }
    dialogs.grammarIndex = next;
    dialogs.grammarNoErrors = next >= dialogs.grammarErrors.size();
}

inline void grammarChange(DocumentComponent& doc, DialogState& dialogs) {
    const GrammarError* current = grammarCurrentError(dialogs);
    if (!current) return;
    // No concrete suggestion: Change is disabled in the dialog and
    // a no-op here.
    if (current->suggestion.empty()) return;
    std::size_t offset = current->offset;
    std::size_t length = current->length;
    std::string oldText = current->text;
    std::string suggestion = current->suggestion;
    recordDeleteRevision(doc, offset, oldText);
    recordInsertRevision(doc, offset, suggestion);
    doc.buffer.replaceRange(offset, length, suggestion);
    doc.isDirty = true;
    refreshGrammarDialog(doc, dialogs);
    // Resume at the first remaining error at/after the change.
    std::size_t next = dialogs.grammarErrors.size();
    for (std::size_t i = 0; i < dialogs.grammarErrors.size(); ++i) {
        if (dialogs.grammarErrors[i].offset >= offset) {
            next = i;
            break;
        }
    }
    dialogs.grammarIndex = next;
    // Recomputed in both branches: a stale true must clear when
    // issues remain, and a stale false must set when none do.
    dialogs.grammarNoErrors = next >= dialogs.grammarErrors.size();
}

// --- Find/Replace dialog commands (Edit > Find/Replace..., Ctrl+H) ---

// Replace All: every match of the dialog's Find term is swapped for the
// Replace term as ONE undo step (replaceRanges, the spellChangeAll
// machinery), so a single Ctrl+Z restores all of them. Returns the
// match count; 0 when the Find term is empty or nothing matches.
// Respects the dialog's Find options (case sensitive / whole word)
// exactly as Find Next does. Matches are non-overlapping left-to-right
// (the standard Replace All reading): findAll itself can return
// overlapping matches for a repeated-character needle ("aa" in "aaa"),
// and overlapping ranges would corrupt both the text and the undo.
inline std::size_t findReplaceAll(DocumentComponent& doc, DialogState& dialogs) {
    if (dialogs.findInputStr.empty()) return 0;
    dialogs.lastSearchTerm = dialogs.findInputStr;
    const std::string replacement = dialogs.replaceInputStr;
    std::vector<TextReplacement> replacements;
    std::size_t nextSearchOffset = 0;
    for (const FindResult& match : doc.buffer.findAll(
             dialogs.lastSearchTerm, dialogs.findOptions)) {
        const std::size_t startOffset =
            doc.buffer.offsetForPosition(match.start);
        const std::size_t endOffset = doc.buffer.offsetForPosition(match.end);
        if (startOffset < nextSearchOffset) continue;
        nextSearchOffset = endOffset;
        std::string oldText;
        oldText.reserve(endOffset - startOffset);
        for (std::size_t i = startOffset; i < endOffset; ++i) {
            oldText.push_back(doc.buffer.charAtOffset(i));
        }
        recordDeleteRevision(doc, startOffset, oldText);
        recordInsertRevision(doc, startOffset, replacement);
        replacements.push_back({match.start, std::move(oldText), replacement});
    }
    if (replacements.empty()) return 0;
    const std::size_t count = replacements.size();
    doc.buffer.replaceRanges(std::move(replacements));
    doc.isDirty = true;
    return count;
}

// --- Spelling dialog commands (Tools > Spelling..., F7) ---
// The dialog shows one error at a time; each action advances
// spellSearchOffset and refreshes the current error from the buffer.

inline void refreshSpellingDialog(DocumentComponent& doc, DialogState& dialogs) {
    auto error = findNextSpellingError(doc.spellChecker, doc.buffer.getText(),
                                       dialogs.spellSearchOffset);
    if (!error) {
        dialogs.spellNoErrors = true;
        dialogs.spellCurrentWord.clear();
        dialogs.spellCurrentLength = 0;
        dialogs.spellSuggestions.clear();
        dialogs.spellReplacementStr.clear();
        return;
    }
    dialogs.spellNoErrors = false;
    dialogs.spellCurrentOffset = error->offset;
    dialogs.spellCurrentLength = error->length;
    dialogs.spellCurrentWord = error->word;
    dialogs.spellSuggestions = error->suggestions;
    dialogs.spellReplacementStr =
        error->suggestions.empty() ? "" : error->suggestions[0];
}

inline void openSpellingDialog(DocumentComponent& doc, DialogState& dialogs) {
    dialogs.showSpellingDialog = true;
    // Start at the caret, like Word; if nothing remains after it, wrap
    // to the start (errors before the caret still get checked).
    dialogs.spellSearchOffset = doc.buffer.caretOffset();
    refreshSpellingDialog(doc, dialogs);
    if (dialogs.spellNoErrors && dialogs.spellSearchOffset != 0) {
        dialogs.spellSearchOffset = 0;
        refreshSpellingDialog(doc, dialogs);
    }
}

inline void spellIgnore(DocumentComponent& doc, DialogState& dialogs) {
    dialogs.spellSearchOffset =
        dialogs.spellCurrentOffset + dialogs.spellCurrentLength;
    refreshSpellingDialog(doc, dialogs);
}

inline void spellIgnoreAll(DocumentComponent& doc, DialogState& dialogs) {
    doc.spellChecker.ignoreWord(dialogs.spellCurrentWord);
    // Re-scan from the same offset: the word is now ignored, so the
    // scan passes it (and the squiggle cache drops it on next edit).
    dialogs.spellSearchOffset = dialogs.spellCurrentOffset;
    refreshSpellingDialog(doc, dialogs);
}

inline void spellAddToDictionary(DocumentComponent& doc, DialogState& dialogs) {
    doc.spellChecker.addToUserDictionary(dialogs.spellCurrentWord);
    dialogs.spellSearchOffset = dialogs.spellCurrentOffset;
    refreshSpellingDialog(doc, dialogs);
}

inline void spellChange(DocumentComponent& doc, DialogState& dialogs) {
    std::string replacement = dialogs.spellReplacementStr;
    if (replacement.empty()) return;
    std::size_t offset = dialogs.spellCurrentOffset;
    std::size_t length = dialogs.spellCurrentLength;
    recordDeleteRevision(doc, offset, dialogs.spellCurrentWord);
    recordInsertRevision(doc, offset, replacement);
    doc.buffer.replaceRange(offset, length, replacement);
    doc.isDirty = true;
    dialogs.spellSearchOffset = offset + replacement.size();
    refreshSpellingDialog(doc, dialogs);
}

inline void spellChangeAll(DocumentComponent& doc, DialogState& dialogs) {
    std::string replacement = dialogs.spellReplacementStr;
    if (replacement.empty()) return;
    std::string word = dialogs.spellCurrentWord;
    FindOptions options;
    options.caseSensitive = false;
    options.wholeWord = true;
    std::vector<TextReplacement> replacements;
    for (const FindResult& match : doc.buffer.findAll(word, options)) {
        std::size_t startOffset = doc.buffer.offsetForPosition(match.start);
        std::size_t endOffset = doc.buffer.offsetForPosition(match.end);
        std::string oldText;
        oldText.reserve(endOffset - startOffset);
        for (std::size_t i = startOffset; i < endOffset; ++i) {
            oldText.push_back(doc.buffer.charAtOffset(i));
        }
        recordDeleteRevision(doc, startOffset, oldText);
        recordInsertRevision(doc, startOffset, replacement);
        replacements.push_back(
            {match.start, std::move(oldText), replacement});
    }
    if (replacements.empty()) return;
    // One undo step for the whole Change All.
    doc.buffer.replaceRanges(std::move(replacements));
    doc.isDirty = true;
    // Continue from the top: the replaced word is gone everywhere, so
    // the next refresh shows the first remaining error in doc order.
    dialogs.spellSearchOffset = 0;
    refreshSpellingDialog(doc, dialogs);
}

// --- Thesaurus dialog commands (Tools > Thesaurus..., Shift+F7) ---
// Open seeds from the word at the caret (a caret at a word's end
// still belongs to that word) or the selection — Current word and
// Look Up both start as that word, with its synonyms listed and
// the first synonym preselected so Replace is enabled at open.
// Look Up re-reads
// the Look Up field against the entry map loaded on open; a miss
// leaves an empty synonym list and the dialog shows
// "(no thesaurus entry)". Replace swaps the stored word range for
// the selected synonym as one undo step (replaceRange), then
// re-seeds the dialog on the replacement (its own synonyms, which
// may be empty). The dialog is modal, so the stored range cannot
// go stale while it is open; Replace still verifies the text at
// the range before writing.

inline bool thesaurusWordRange(DocumentComponent& doc, std::size_t& offset,
                               std::size_t& length, std::string& word) {
    if (doc.buffer.hasSelection()) {
        CaretPosition start = doc.buffer.selectionStart();
        CaretPosition end = doc.buffer.selectionEnd();
        offset = doc.buffer.offsetForPosition(start);
        std::size_t endOffset = doc.buffer.offsetForPosition(end);
        if (endOffset < offset) return false;
        length = endOffset - offset;
        if (length == 0) return false;
        word = doc.buffer.getSelectedText();
        return true;
    }
    std::size_t caret = doc.buffer.caretOffset();
    std::string text = doc.buffer.getText();
    for (const auto& [start, w] : SpellChecker::extractWords(text)) {
        if (caret >= start && caret <= start + w.size()) {
            offset = start;
            length = w.size();
            word = w;
            return true;
        }
    }
    return false;
}

inline void openThesaurusDialog(DocumentComponent& doc, DialogState& dialogs) {
    dialogs.showThesaurusDialog = true;
    dialogs.thesaurusEntries = thesaurus::loadDefault();
    std::size_t offset = 0;
    std::size_t length = 0;
    std::string word;
    dialogs.thesaurusHasWord = thesaurusWordRange(doc, offset, length, word);
    if (dialogs.thesaurusHasWord) {
        dialogs.thesaurusWordOffset = offset;
        dialogs.thesaurusWordLength = length;
        dialogs.thesaurusCurrentWord = word;
        dialogs.thesaurusLookupStr = word;
        dialogs.thesaurusSynonyms =
            thesaurus::lookup(dialogs.thesaurusEntries, word);
    } else {
        dialogs.thesaurusWordOffset = 0;
        dialogs.thesaurusWordLength = 0;
        dialogs.thesaurusCurrentWord.clear();
        dialogs.thesaurusLookupStr.clear();
        dialogs.thesaurusSynonyms.clear();
    }
    // The first synonym is preselected, so Replace is enabled in
    // the opening state (no click on the list needed); a word with
    // no entry leaves nothing selected and Replace disabled.
    dialogs.thesaurusSelected =
        dialogs.thesaurusSynonyms.empty() ? -1 : 0;
}

inline void thesaurusLookup(DocumentComponent& /*doc*/, DialogState& dialogs) {
    dialogs.thesaurusSynonyms =
        thesaurus::lookup(dialogs.thesaurusEntries, dialogs.thesaurusLookupStr);
    dialogs.thesaurusSelected =
        dialogs.thesaurusSynonyms.empty() ? -1 : 0;
}

inline bool thesaurusReplace(DocumentComponent& doc, DialogState& dialogs) {
    if (!dialogs.thesaurusHasWord || dialogs.thesaurusSelected < 0 ||
        static_cast<std::size_t>(dialogs.thesaurusSelected) >=
            dialogs.thesaurusSynonyms.size()) {
        return false;
    }
    std::string synonym =
        dialogs.thesaurusSynonyms[static_cast<std::size_t>(
            dialogs.thesaurusSelected)];
    std::size_t offset = dialogs.thesaurusWordOffset;
    std::size_t length = dialogs.thesaurusWordLength;
    if (offset + length > doc.buffer.textSize()) return false;
    std::string current;
    for (std::size_t i = 0; i < length; ++i) {
        current.push_back(doc.buffer.charAtOffset(offset + i));
    }
    if (current != dialogs.thesaurusCurrentWord) return false;
    recordDeleteRevision(doc, offset, current);
    recordInsertRevision(doc, offset, synonym);
    doc.buffer.replaceRange(offset, length, synonym);
    doc.isDirty = true;
    dialogs.thesaurusCurrentWord = synonym;
    dialogs.thesaurusLookupStr = synonym;
    dialogs.thesaurusWordLength = synonym.size();
    dialogs.thesaurusSynonyms =
        thesaurus::lookup(dialogs.thesaurusEntries, synonym);
    dialogs.thesaurusSelected =
        dialogs.thesaurusSynonyms.empty() ? -1 : 0;
    return true;
}

// --- AutoFormat (Tools > AutoFormat...) ---
// Batch pass over the whole document: the AutoCorrect replacements
// as one undo step. Returns the number of words corrected.
inline std::size_t autoFormatDocument(DocumentComponent& doc) {
    std::size_t corrected = doc.autoCorrect.autoFormatPass(doc.buffer);
    if (corrected > 0) doc.isDirty = true;
    return corrected;
}

// --- AutoCorrect (replace-as-you-type) ---

// Word-completion characters that trigger AutoCorrect: space and
// punctuation. Deliberately excludes newline (its own action) and
// '-'/'/' (word-internal). '"' is included: when smart quotes are on,
// the smart-quote branch handles quotes before this is reached, so this
// only fires with smart quotes off (e.g. teh" -> the").
inline bool isAutoCorrectDelimiter(char ch) {
    switch (ch) {
        case ' ':
        case '.':
        case ',':
        case ';':
        case ':':
        case '!':
        case '?':
        case ')':
        case ']':
        case '}':
        case '"':
            return true;
        default:
            return false;
    }
}

// If the word just before the caret has an AutoCorrect fix, replace the
// word AND the delimiter being typed with the corrected word + delimiter,
// as a single undo step. Returns true when a correction was applied
// (the caller must not insert the delimiter separately then).
inline bool tryAutoCorrect(DocumentComponent& doc, char delimiter) {
    TextBuffer& buffer = doc.buffer;
    if (buffer.hasSelection()) return false;
    std::size_t caret = buffer.caretOffset();
    // Caret inside a word (inserting mid-word) is not a completion.
    if (caret < buffer.textSize() &&
        SpellChecker::isWordChar(buffer.charAtOffset(caret))) {
        return false;
    }
    std::size_t start = caret;
    while (start > 0 &&
           SpellChecker::isWordChar(buffer.charAtOffset(start - 1))) {
        --start;
    }
    if (start == caret) return false;  // No word before the caret
    std::string word;
    word.reserve(caret - start);
    for (std::size_t i = start; i < caret; ++i) {
        word.push_back(buffer.charAtOffset(i));
    }
    bool sentenceStart =
        doc.autoCorrect.settings.capitalizeFirstLetterOfSentence &&
        AutoCorrect::isSentenceStart(buffer, start);
    auto corrected = doc.autoCorrect.correct(word, sentenceStart);
    if (!corrected) return false;
    std::string replacement = *corrected + delimiter;
    recordDeleteRevision(doc, start, word);
    recordInsertRevision(doc, start, replacement);
    buffer.replaceRange(start, word.size(), replacement);
    doc.isDirty = true;
    // An AutoCorrect fix is not a repeatable typed run: the "teh"
    // run must not survive to be re-inserted by F4.
    repeat::clear(doc.repeatRecord);
    return true;
}

// --- Format Painter (Standard toolbar) ---
// Click the button to copy the current style and arm; clicking it
// again disarms. The next click in the text applies the captured
// style (through here) and disarms. See format_painter.h: the
// buffer's style is document-global, so apply sets the global
// style like the Bold/Italic buttons do.
inline void toggleFormatPainter(DocumentComponent& doc,
                                ToolbarComponent& toolbar) {
    if (toolbar.formatPainter.armed) {
        toolbar.formatPainter.disarm();
    } else {
        toolbar.formatPainter.arm(doc.buffer.textStyle());
    }
}

inline bool applyFormatPainter(DocumentComponent& doc,
                               ToolbarComponent& toolbar) {
    if (!toolbar.formatPainter.armed) return false;
    if (!toolbar.formatPainter.apply(doc.buffer)) return false;
    doc.isDirty = true;
    return true;
}

// --- Outline view (View > Outline) ---
// Entering saves the page mode + scroll offset, forces Pageless, and
// parks the caret on a heading (the first heading at-or-after the
// caret row, else the first heading): in Outline the caret can only
// sit on a rendered row. Exiting restores the saved page mode +
// scroll. When the document has no headings, entering is still a
// mode switch (the view is simply empty) and the caret stays put.
inline void toggleOutlineMode(DocumentComponent& doc,
                              LayoutComponent& layout,
                              ScrollComponent* scroll) {
    if (!layout.showOutline) {
        layout.outlinePrevPageMode = layout.pageMode;
        layout.outlinePrevScroll = scroll ? scroll->offset : 0;
        layout.showOutline = true;
        layout.pageMode = PageMode::Pageless;
        if (scroll) scroll->offset = 0;
        if (!outline_view::isHeadingStyle(
                doc.buffer.lineParagraphStyle(doc.buffer.caret().row))) {
            std::size_t rows = doc.buffer.lineCount();
            std::size_t target = rows;
            for (std::size_t i = doc.buffer.caret().row; i < rows; ++i) {
                if (outline_view::isHeadingStyle(
                        doc.buffer.lineParagraphStyle(i))) {
                    target = i;
                    break;
                }
            }
            if (target == rows) {
                for (std::size_t i = 0; i < rows; ++i) {
                    if (outline_view::isHeadingStyle(
                            doc.buffer.lineParagraphStyle(i))) {
                        target = i;
                        break;
                    }
                }
            }
            if (target != rows) {
                doc.buffer.setCaret({target, 0});
                doc.buffer.clearSelection();
            }
        }
        // No floating toast: the caller (menu action) flashes the
        // transient status-bar text, like the Normal/Page Layout
        // switches, so no rounded blue "Outline: On" pill covers
        // the document.
    } else {
        layout.showOutline = false;
        layout.pageMode = layout.outlinePrevPageMode;
        if (scroll) {
            scroll->offset =
                std::max(0, layout.outlinePrevScroll);
        }
    }
}

// --- Outlining toolbar (View > Outline) ---
// Promote / Demote change the caret paragraph's heading level
// via the existing style application (setLineParagraphStyle),
// recorded as ONE undo step (commitParagraphStyle). A no-op
// style (Promote on Heading 1, Demote on Heading 6, caret on a
// Normal paragraph) records nothing.
inline bool promoteOutlineHeading(DocumentComponent& doc) {
    std::size_t row = doc.buffer.caret().row;
    ParagraphStyle oldStyle = doc.buffer.lineParagraphStyle(row);
    ParagraphStyle newStyle = outline_view::promoteParagraphStyle(oldStyle);
    if (newStyle == oldStyle) return false;
    doc.buffer.setLineParagraphStyle(row, newStyle);
    doc.buffer.commitParagraphStyle(row, oldStyle, newStyle);
    doc.buffer.clearSelection();
    doc.buffer.setCaret({row, 0});
    doc.isDirty = true;
    return true;
}

inline bool demoteOutlineHeading(DocumentComponent& doc) {
    std::size_t row = doc.buffer.caret().row;
    ParagraphStyle oldStyle = doc.buffer.lineParagraphStyle(row);
    ParagraphStyle newStyle = outline_view::demoteParagraphStyle(oldStyle);
    if (newStyle == oldStyle) return false;
    doc.buffer.setLineParagraphStyle(row, newStyle);
    doc.buffer.commitParagraphStyle(row, oldStyle, newStyle);
    doc.buffer.clearSelection();
    doc.buffer.setCaret({row, 0});
    doc.isDirty = true;
    return true;
}

// Move the caret heading's block (heading + subordinates) up /
// down past its previous / next sibling block, as ONE undo step
// (swapAdjacentLineRanges + commitSwapAdjacentLineRanges).
// The caret follows its heading. Returns false when there is no
// sibling block in that direction. A collapsed heading carries
// its collapsed entry with it: the entry moves from the old
// heading row to the new one (the swap does not move the
// LayoutComponent's row-keyed set, so the command layer does).
inline bool moveOutlineBlock(DocumentComponent& doc, LayoutComponent& layout,
                             bool up) {
    std::size_t row = doc.buffer.caret().row;
    std::vector<ParagraphStyle> styles;
    for (std::size_t i = 0; i < doc.buffer.lineCount(); ++i) {
        styles.push_back(doc.buffer.lineParagraphStyle(i));
    }
    if (up) {
        auto prev = outline_view::outlinePrevSiblingBlock(styles, row);
        if (!prev) return false;
        std::size_t prevStart = prev->first;
        std::size_t prevEnd = prev->second;  // == row
        std::size_t blockEnd = outline_view::outlineBlockEnd(styles, row);
        std::size_t newRow = row - (prevEnd - prevStart);
        // Collapsed entries inside the combined range move with
        // their headings (entries outside it are untouched): an
        // entry in the first block moves by +(caret block size),
        // one in the caret block by -(first block size).
        std::unordered_set<std::size_t> moved;
        for (std::size_t r : layout.outlineCollapsedRows) {
            if (r >= prevStart && r < prevEnd) {
                moved.insert(r + (blockEnd - row));
            } else if (r >= row && r < blockEnd) {
                moved.insert(r - (prevEnd - prevStart));
            } else {
                moved.insert(r);
            }
        }
        doc.buffer.swapAdjacentLineRanges(prevStart, row, blockEnd);
        doc.buffer.commitSwapAdjacentLineRanges(prevStart, row, blockEnd,
                                                row, newRow);
        doc.buffer.clearSelection();
        doc.buffer.setCaret({newRow, 0});
        layout.outlineCollapsedRows = std::move(moved);
        doc.isDirty = true;
        return true;
    }
    auto next = outline_view::outlineNextSiblingBlock(styles, row);
    if (!next) return false;
    std::size_t blockEnd = outline_view::outlineBlockEnd(styles, row);
    std::size_t nextEnd = next->second;
    // newRow = row + size of the next sibling block: the heading
    // lands at the start of where its block ends up after the
    // swap (the block that was after it now sits before it).
    std::size_t newRow = row + (nextEnd - blockEnd);
    std::unordered_set<std::size_t> moved;
    for (std::size_t r : layout.outlineCollapsedRows) {
        if (r >= row && r < blockEnd) {
            moved.insert(r + (nextEnd - blockEnd));
        } else if (r >= blockEnd && r < nextEnd) {
            moved.insert(r - (blockEnd - row));
        } else {
            moved.insert(r);
        }
    }
    doc.buffer.swapAdjacentLineRanges(row, blockEnd, nextEnd);
    doc.buffer.commitSwapAdjacentLineRanges(row, blockEnd, nextEnd, row,
                                            newRow);
    doc.buffer.clearSelection();
    doc.buffer.setCaret({newRow, 0});
    layout.outlineCollapsedRows = std::move(moved);
    doc.isDirty = true;
    return true;
}

// Expand / Collapse the heading at the caret: toggles its row in
// layout.outlineCollapsedRows. A heading with no subordinate
// headings has no collapse box and this is a no-op (nothing
// would hide). Not an undo step: collapse is view state, like
// Show Level, not document state.
inline bool toggleOutlineCollapse(DocumentComponent& doc,
                                  LayoutComponent& layout) {
    std::size_t row = doc.buffer.caret().row;
    std::vector<ParagraphStyle> styles;
    for (std::size_t i = 0; i < doc.buffer.lineCount(); ++i) {
        styles.push_back(doc.buffer.lineParagraphStyle(i));
    }
    if (!outline_view::outlineHasSubordinateHeadings(styles, row)) {
        return false;
    }
    if (layout.outlineCollapsedRows.count(row) > 0) {
        layout.outlineCollapsedRows.erase(row);
    } else {
        layout.outlineCollapsedRows.insert(row);
    }
    return true;
}

inline bool isOutlineCollapsedAtCaret(DocumentComponent& doc,
                                      LayoutComponent& layout) {
    return layout.outlineCollapsedRows.count(doc.buffer.caret().row) > 0;
}

// Leave Outline for an explicit view choice (View > Normal /
// Page Layout): restore the saved scroll, then the caller sets the
// chosen page mode.
inline void exitOutlineMode(LayoutComponent& layout,
                            ScrollComponent* scroll) {
    if (!layout.showOutline) return;
    layout.showOutline = false;
    if (scroll) scroll->offset = std::max(0, layout.outlinePrevScroll);
}

// --- Format toggles ---

inline void toggleBold(DocumentComponent& doc) {
    // Cells have no formatting model yet: formatting
    // commands never act on the body buffer from a cell.
    if (doc.cellEdit.active) return;
    TextStyle style = doc.buffer.textStyle();
    style.bold = !style.bold;
    doc.buffer.setTextStyle(style);
    doc.repeatRecord = repeat::setBool(RepeatKind::SetBold, style.bold);
}

inline void toggleItalic(DocumentComponent& doc) {
    // Cells have no formatting model yet: formatting
    // commands never act on the body buffer from a cell.
    if (doc.cellEdit.active) return;
    TextStyle style = doc.buffer.textStyle();
    style.italic = !style.italic;
    doc.buffer.setTextStyle(style);
    doc.repeatRecord = repeat::setBool(RepeatKind::SetItalic, style.italic);
}

inline void toggleUnderline(DocumentComponent& doc) {
    // Cells have no formatting model yet: formatting
    // commands never act on the body buffer from a cell.
    if (doc.cellEdit.active) return;
    TextStyle style = doc.buffer.textStyle();
    style.underline = !style.underline;
    doc.buffer.setTextStyle(style);
    doc.repeatRecord =
        repeat::setBool(RepeatKind::SetUnderline, style.underline);
}

inline void toggleStrikethrough(DocumentComponent& doc) {
    // Cells have no formatting model yet: formatting
    // commands never act on the body buffer from a cell.
    if (doc.cellEdit.active) return;
    TextStyle style = doc.buffer.textStyle();
    style.strikethrough = !style.strikethrough;
    doc.buffer.setTextStyle(style);
    doc.repeatRecord =
        repeat::setBool(RepeatKind::SetStrikethrough, style.strikethrough);
}

inline void toggleSuperscript(DocumentComponent& doc) {
    // Cells have no formatting model yet: formatting
    // commands never act on the body buffer from a cell.
    if (doc.cellEdit.active) return;
    TextStyle style = doc.buffer.textStyle();
    style.superscript = !style.superscript;
    if (style.superscript) style.subscript = false;
    doc.buffer.setTextStyle(style);
    doc.repeatRecord =
        repeat::setBool(RepeatKind::SetSuperscript, style.superscript);
}

inline void toggleSubscript(DocumentComponent& doc) {
    // Cells have no formatting model yet: formatting
    // commands never act on the body buffer from a cell.
    if (doc.cellEdit.active) return;
    TextStyle style = doc.buffer.textStyle();
    style.subscript = !style.subscript;
    if (style.subscript) style.superscript = false;
    doc.buffer.setTextStyle(style);
    doc.repeatRecord =
        repeat::setBool(RepeatKind::SetSubscript, style.subscript);
}

// --- Format applications (menu / toolbar / shortcuts share these) ---
// Each applies the same buffer write the old direct call made, then
// records the RESULTING value for Edit > Repeat (F4).

inline void setParagraphStyle(DocumentComponent& doc, ParagraphStyle style) {
    // Cells have no formatting model yet: formatting
    // commands never act on the body buffer from a cell.
    if (doc.cellEdit.active) return;
    doc.buffer.setCurrentParagraphStyle(style);
    doc.repeatRecord = repeat::setInt(RepeatKind::SetParagraphStyle,
                                      static_cast<int>(style));
}

inline void setAlignment(DocumentComponent& doc, TextAlignment align) {
    // Cells have no formatting model yet: formatting
    // commands never act on the body buffer from a cell.
    if (doc.cellEdit.active) return;
    doc.buffer.setCurrentAlignment(align);
    doc.repeatRecord = repeat::setInt(RepeatKind::SetAlignment,
                                      static_cast<int>(align));
    doc.isDirty = true;
}

inline void setFontSize(DocumentComponent& doc, int size) {
    // Cells have no formatting model yet: formatting
    // commands never act on the body buffer from a cell.
    if (doc.cellEdit.active) return;
    TextStyle style = doc.buffer.textStyle();
    style.fontSize = size;
    doc.buffer.setTextStyle(style);
    doc.repeatRecord = repeat::setInt(RepeatKind::SetFontSize, size);
}

inline void setFont(DocumentComponent& doc, const std::string& font) {
    // Cells have no formatting model yet: formatting
    // commands never act on the body buffer from a cell.
    if (doc.cellEdit.active) return;
    TextStyle style = doc.buffer.textStyle();
    style.font = font;
    doc.buffer.setTextStyle(style);
    doc.repeatRecord = repeat::setText(RepeatKind::SetFont, font);
}

inline void setTextColor(DocumentComponent& doc, TextColor color) {
    // Cells have no formatting model yet: formatting
    // commands never act on the body buffer from a cell.
    if (doc.cellEdit.active) return;
    TextStyle style = doc.buffer.textStyle();
    style.textColor = color;
    doc.buffer.setTextStyle(style);
    doc.repeatRecord = repeat::setColor(RepeatKind::SetTextColor, color);
}

inline void setHighlightColor(DocumentComponent& doc, TextColor color) {
    // Cells have no formatting model yet: formatting
    // commands never act on the body buffer from a cell.
    if (doc.cellEdit.active) return;
    TextStyle style = doc.buffer.textStyle();
    style.highlightColor = color;
    doc.buffer.setTextStyle(style);
    doc.repeatRecord =
        repeat::setColor(RepeatKind::SetHighlightColor, color);
}

inline void increaseIndent(DocumentComponent& doc) {
    // Cells have no formatting model yet: formatting
    // commands never act on the body buffer from a cell.
    if (doc.cellEdit.active) return;
    doc.buffer.increaseIndent();
    doc.repeatRecord = repeat::setInt(RepeatKind::SetLeftIndent,
                                      doc.buffer.currentLeftIndent());
}

inline void decreaseIndent(DocumentComponent& doc) {
    // Cells have no formatting model yet: formatting
    // commands never act on the body buffer from a cell.
    if (doc.cellEdit.active) return;
    doc.buffer.decreaseIndent();
    doc.repeatRecord = repeat::setInt(RepeatKind::SetLeftIndent,
                                      doc.buffer.currentLeftIndent());
}

inline void setLineSpacing(DocumentComponent& doc, int spacingTimes100) {
    // Cells have no formatting model yet: formatting
    // commands never act on the body buffer from a cell.
    if (doc.cellEdit.active) return;
    if (spacingTimes100 == 100) {
        doc.buffer.setLineSpacingSingle();
    } else if (spacingTimes100 == 150) {
        doc.buffer.setLineSpacing1_5();
    } else if (spacingTimes100 == 200) {
        doc.buffer.setLineSpacingDouble();
    } else {
        return;
    }
    doc.repeatRecord =
        repeat::setInt(RepeatKind::SetLineSpacing, spacingTimes100);
}

inline void toggleBulletedList(DocumentComponent& doc) {
    // Cells have no formatting model yet: formatting
    // commands never act on the body buffer from a cell.
    if (doc.cellEdit.active) return;
    doc.buffer.toggleBulletedList();
    doc.repeatRecord = repeat::setInt(
        RepeatKind::SetListType,
        static_cast<int>(doc.buffer.currentListType()));
}

inline void toggleNumberedList(DocumentComponent& doc) {
    // Cells have no formatting model yet: formatting
    // commands never act on the body buffer from a cell.
    if (doc.cellEdit.active) return;
    doc.buffer.toggleNumberedList();
    doc.repeatRecord = repeat::setInt(
        RepeatKind::SetListType,
        static_cast<int>(doc.buffer.currentListType()));
}

inline void setSpaceBefore(DocumentComponent& doc, int pixels) {
    // Cells have no formatting model yet: formatting
    // commands never act on the body buffer from a cell.
    if (doc.cellEdit.active) return;
    doc.buffer.setCurrentSpaceBefore(pixels);
    doc.repeatRecord = repeat::setInt(RepeatKind::SetSpaceBefore, pixels);
}

inline void setSpaceAfter(DocumentComponent& doc, int pixels) {
    // Cells have no formatting model yet: formatting
    // commands never act on the body buffer from a cell.
    if (doc.cellEdit.active) return;
    doc.buffer.setCurrentSpaceAfter(pixels);
    doc.repeatRecord = repeat::setInt(RepeatKind::SetSpaceAfter, pixels);
}

// --- Edit > Repeat (F4) ---
// Re-applies doc.repeatRecord at the current caret/selection.
// Text kinds (InsertText / Paste) go through replaceRange with
// revision records (one undo step); formatting kinds go through
// repeat::apply. Repeat never records itself. Empty history is a
// no-op with a status-bar flash ("Nothing to repeat").
inline bool repeatLastAction(DocumentComponent& doc) {
    // Repeat in a cell would re-apply a body action at the
    // body caret; skip it while a cell is active.
    if (doc.cellEdit.active) return false;
    if (!repeat::hasAction(doc.repeatRecord)) {
        flashStatusConfirmation("Nothing to repeat");
        return false;
    }
    ActionRecord record = doc.repeatRecord;
    if (record.kind == RepeatKind::InsertText ||
        record.kind == RepeatKind::Paste) {
        std::size_t offset = doc.buffer.caretOffset();
        std::size_t length = 0;
        if (doc.buffer.hasSelection()) {
            offset = doc.buffer.offsetForPosition(
                doc.buffer.selectionStart());
            length = doc.buffer.offsetForPosition(
                         doc.buffer.selectionEnd()) -
                     offset;
        }
        std::string oldText;
        oldText.reserve(length);
        for (std::size_t i = 0; i < length; ++i) {
            oldText.push_back(doc.buffer.charAtOffset(offset + i));
        }
        if (!oldText.empty()) recordDeleteRevision(doc, offset, oldText);
        recordInsertRevision(doc, offset, record.text);
        doc.buffer.clearSelection();
        doc.buffer.replaceRange(offset, length, record.text);
    } else {
        repeat::apply(doc.buffer, record);
    }
    doc.isDirty = true;
    return true;
}


}  // namespace cmd
}  // namespace ecs
