#pragma once

#include <cstdlib>
#include <ctime>
#include <filesystem>

#include "../external.h"
#include "../util/clipboard.h"
#include "../editor/document_io.h"
#include "../editor/export/export_pdf.h"
#include "../editor/spell_session.h"
#include "../settings.h"
#include "../ui/ui_context.h"
#include "../ui/menu_setup.h"
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

// --- Print Preview / Print (File menu, Ctrl+P) ---

inline void openPrintPreview(DocumentComponent& doc, LayoutComponent& layout) {
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
// concrete suggestion exists. Re-scanning after Change resumes at
// the first error at/after the change, so later errors (whose
// offsets shifted) stay in order and earlier ones are not re-shown.
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
    if (!grammarCurrentError(dialogs)) return;
    ++dialogs.grammarIndex;
    refreshGrammarDialog(doc, dialogs);
    if (dialogs.grammarIndex >= dialogs.grammarErrors.size()) {
        dialogs.grammarNoErrors = true;
    }
}

inline void grammarIgnoreAll(DocumentComponent& doc,
                             DialogState& dialogs) {
    const GrammarError* current = grammarCurrentError(dialogs);
    if (!current) return;
    dialogs.grammarIgnored.insert(grammarIgnoreKey(*current));
    refreshGrammarDialog(doc, dialogs);
    // The ignored issue is gone, so the next issue (if any) now
    // sits at the same index; if none does, the check is complete.
    if (dialogs.grammarIndex >= dialogs.grammarErrors.size()) {
        dialogs.grammarNoErrors = true;
    }
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
    if (next >= dialogs.grammarErrors.size()) {
        dialogs.grammarNoErrors = true;
    }
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
    return true;
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
