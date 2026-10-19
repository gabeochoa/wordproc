#pragma once

#include <cstdlib>
#include <filesystem>
#include <format>

#include "../util/clipboard.h"
#include "../editor/borders_shading.h"
#include "../editor/columns_math.h"
#include "../editor/paragraph_dialog.h"
#include "../editor/document_io.h"
#include "../editor/export/export_html.h"
#include "../editor/export/export_pdf.h"
#include "../editor/export/export_rtf.h"
#include "../editor/drawing.h"
#include "../editor/equation.h"
#include "../editor/image.h"
#include "../editor/table.h"
#include "../settings.h"
#include "../ui/new_window.h"
#include "../ui/theme.h"
#include "../ui/ui_context.h"
#include "../ui/menu_setup.h"
#include "component_helpers.h"
#include "components.h"
#include "document_commands.h"

namespace ecs {

inline void handleMenuActionImpl(int menuResult, DocumentComponent& doc,
                          MenuComponent& menu, DialogState& dialogs,
                          LayoutComponent& layout,
                          StatusComponent* statusComp = nullptr,
                          ScrollComponent* scroll = nullptr) {
        int menuIndex = menuResult / 100;
        int itemIndex = menuResult % 100;

        // Confirmations to the transient status-bar flash, not a
        // floating toast (rounded modern toasts are not Word 95).
        // Falls back to the EntityQuery lookup when the caller did
        // not pass a status handle. Errors keep toast_notify::error.
        auto flash = [&](const std::string& msg, double duration = 3.0) {
            if (statusComp) {
                status::flash(*statusComp, msg,
                              afterhours::graphics::get_time(), duration);
            } else {
                cmd::flashStatusConfirmation(msg, duration);
            }
        };

        // Header/footer editing (Batch 9): the body is not
        // editable while the mode is active, so menu actions
        // that would mutate the body are refused (Close the
        // strip first). Allowed through: File > Save / Save As /
        // Exit, View > Header and Footer (toggle), Insert >
        // Header / Footer (switch target) / Page Number
        // (toggle the field) / Document Body (exit). Everything
        // else — formatting, tables, clipboard, view switches —
        // would act on the body buffer behind the mode.
        if (layout.headerEditTarget != HeaderEditTarget::None) {
            bool allowed = false;
            if (menuIndex == 0 && itemIndex >= 0 &&
                itemIndex < static_cast<int>(menu.menus[0].items.size())) {
                const std::string& label =
                    menu.menus[0].items[itemIndex].label;
                if (itemIndex == 3 || itemIndex == 4 || label == "Exit") {
                    allowed = true;
                }
            }
            if (menuIndex == 1 && itemIndex == 18) allowed = true;
            if (menuIndex == 2 && itemIndex == 4) allowed = true;
            if (menuIndex == 3 &&
                (itemIndex == 26 || itemIndex == 27 ||
                 itemIndex == 28 || itemIndex == 29)) {
                allowed = true;
            }
            if (!allowed) {
                flash("Close the header/footer first");
                return;
            }
        }

        if (menuIndex == 0) {  // File menu
            if (itemIndex >= 0 &&
                itemIndex < static_cast<int>(menu.menus[0].items.size())) {
                const std::string& label = menu.menus[0].items[itemIndex].label;
                if (label.rfind("Recent: ", 0) == 0) {
                    std::string path = label.substr(std::string("Recent: ").size());
                    cmd::openDocument(doc, layout, menu, path);
                    return;
                }
                if (label == "Exit") {
                    cmd::requestQuit(doc, dialogs);
                    return;
                }
            }
            switch (itemIndex) {
                case 0:  // New
                    cmd::newDocument(doc);
                    break;
                case 1:  // New from Template...
                    dialogs.showTemplateDialog = true;
                    // Blank Document first (no file on disk), then the
                    // shipped .wpdoc stems.
                    dialogs.templateNames = {templates::kBlankTemplateName};
                    for (const auto& name : templates::listTemplateNames(
                             templates::templateDir())) {
                        dialogs.templateNames.push_back(name);
                    }
                    dialogs.templateSelected = 0;
                    break;
                case 2:  // Open
                    menu.pendingDialog = MenuComponent::PendingDialog::Open;
                    break;
                case 3:  // Save
                    cmd::saveDocument(doc, layout, menu);
                    break;
                case 4:  // Save As...
                    menu.pendingDialog = MenuComponent::PendingDialog::SaveAs;
                    break;
                case 5:  // Version History... (Docs-era)
                    dialogs.showVersionHistoryDialog = true;
                    dialogs.versionHistorySnapshots =
                        cmd::versionSnapshots(doc);
                    dialogs.versionHistorySelected =
                        dialogs.versionHistorySnapshots.empty() ? -1 : 0;
                    break;
                case 7:  // Export PDF
                {
                    std::filesystem::path basePath =
                        doc.filePath.empty() ? doc.defaultPath : doc.filePath;
                    basePath.replace_extension(".pdf");
                    auto result = exportDocumentPdf(doc.buffer, doc.docSettings,
                                                    basePath.string());
                    if (result.success) {
                        flash(
                            "Exported PDF: " + basePath.filename().string());
                    } else {
                        toast_notify::error("Export PDF failed: " + result.error);
                    }
                } break;
                case 8:  // Export HTML
                {
                    std::filesystem::path basePath =
                        doc.filePath.empty() ? doc.defaultPath : doc.filePath;
                    basePath.replace_extension(".html");
                    auto result = exportDocumentHtml(doc.buffer, doc.docSettings,
                                                     basePath.string());
                    if (result.success) {
                        flash(
                            "Exported HTML: " + basePath.filename().string());
                    } else {
                        toast_notify::error("Export HTML failed: " + result.error);
                    }
                } break;
                case 9:  // Export RTF
                {
                    std::filesystem::path basePath =
                        doc.filePath.empty() ? doc.defaultPath : doc.filePath;
                    basePath.replace_extension(".rtf");
                    auto result = exportDocumentRtf(doc.buffer, doc.docSettings,
                                                    basePath.string());
                    if (result.success) {
                        flash(
                            "Exported RTF: " + basePath.filename().string());
                    } else {
                        toast_notify::error("Export RTF failed: " + result.error);
                    }
                } break;
                case 11:  // Page Setup
                {
                    // Open (not toggle) and seed the working selection
                    // from the document, so the dialog shows the current
                    // paper/orientation/margins and Cancel leaves them
                    // untouched. OK writes back + syncs the layout.
                    dialogs.showPageSetup = true;
                    dialogs.selectedPageSize =
                        doc.docSettings.pageSettings.size;
                    dialogs.selectedOrientation =
                        doc.docSettings.pageSettings.orientation;
                    dialogs.pageSetupTopStr =
                        columns_math::pointsToInchesString(
                            doc.docSettings.pageSettings.marginTop);
                    dialogs.pageSetupBottomStr =
                        columns_math::pointsToInchesString(
                            doc.docSettings.pageSettings.marginBottom);
                    dialogs.pageSetupLeftStr =
                        columns_math::pointsToInchesString(
                            doc.docSettings.pageSettings.marginLeft);
                    dialogs.pageSetupRightStr =
                        columns_math::pointsToInchesString(
                            doc.docSettings.pageSettings.marginRight);
                } break;
                case 12:  // Print Preview
                    cmd::openPrintPreview(doc, layout);
                    break;
                case 13:  // Print...
                    cmd::requestPrint(doc, dialogs);
                    break;
                default:
                    break;
            }
        } else if (menuIndex == 1) {  // Edit menu
            switch (itemIndex) {
                case 0:  // Undo
                    cmd::undo(doc);
                    break;
                case 1:  // Redo
                    cmd::redo(doc);
                    break;
                case 3:  // Track Changes
                    doc.trackChangesEnabled = !doc.trackChangesEnabled;
                    // Confirmation to the status-bar flash (same
                    // pattern as Normal/Page Layout above), not a
                    // floating toast.
                    if (doc.trackChangesEnabled) {
                        doc.trackChangesBaseline = doc.buffer.getText();
                        menu.menus[1].items[3].mark = win95::MenuMark::Checkmark;
                        if (statusComp)
                            status::flash(*statusComp, "Track Changes: On",
                                          afterhours::graphics::get_time());
                    } else {
                        menu.menus[1].items[3].mark = win95::MenuMark::None;
                        if (statusComp)
                            status::flash(*statusComp, "Track Changes: Off",
                                          afterhours::graphics::get_time());
                    }
                    break;
                case 4:  // Accept All Changes
                    cmd::acceptAllRevisions(doc);
                    break;
                case 5:  // Reject All Changes
                    cmd::rejectAllRevisions(doc);
                    break;
                case 7:  // Cut
                    cmd::cut(doc);
                    break;
                case 8:  // Copy
                    cmd::copy(doc);
                    break;
                case 9:  // Paste
                    cmd::paste(doc);
                    break;
                case 11:  // Select All
                    cmd::selectAll(doc);
                    break;
                case 13:  // Find...
                    dialogs.showFindDialog = true;
                    dialogs.findReplaceMode = false;
                    flash("Find: F3 next, Shift+F3 prev");
                    break;
                case 14:  // Find Next
                    if (!dialogs.lastSearchTerm.empty()) {
                        FindResult result = doc.buffer.findNext(dialogs.lastSearchTerm, dialogs.findOptions);
                        if (result.found) {
                            doc.buffer.setCaret(result.start);
                            doc.buffer.setSelectionAnchor(result.start);
                            doc.buffer.setCaret(result.end);
                            doc.buffer.updateSelectionToCaret();
                            flash("Found", 2.0f);
                        } else {
                            flash("Not found");
                        }
                    }
                    break;
                case 15:  // Find Previous
                    if (!dialogs.lastSearchTerm.empty()) {
                        FindResult result = doc.buffer.findPrevious(dialogs.lastSearchTerm, dialogs.findOptions);
                        if (result.found) {
                            doc.buffer.setCaret(result.start);
                            doc.buffer.setSelectionAnchor(result.start);
                            doc.buffer.setCaret(result.end);
                            doc.buffer.updateSelectionToCaret();
                            flash("Found", 2.0f);
                        } else {
                            flash("Not found");
                        }
                    }
                    break;
                case 16:  // Replace...
                    dialogs.showFindDialog = true;
                    dialogs.findReplaceMode = true;
                    flash("Replace mode");
                    break;
                case 18:  // Go To... (Ctrl+G, Batch 9:
                          // Word's ONE Go To — Page / Line /
                          // Bookmark — replaces Go To Bookmark...)
                    cmd::openGoToDialog(layout, dialogs);
                    break;
                case 20:  // Repeat (F4)
                    cmd::repeatLastAction(doc);
                    break;
                default:
                    break;
            }
        } else if (menuIndex == 2) {  // View menu
            switch (itemIndex) {
                case 0:  // Normal (preview owns pageMode: ignored)
                    if (layout.previewMode) break;
                    cmd::exitOutlineMode(layout, scroll);
                    // The View switch is a real mode change: write it
                    // to the document settings too, or the next
                    // settings->layout sync (Page Setup OK, Open)
                    // would resurrect the old mode from settings.
                    // Preview / Outline switches stay layout-only:
                    // they are transient and must not rewrite the doc.
                    // Normal IS Pageless (PageMode::Pageless): the
                    // rename is a label only, no file-format change;
                    // the renderer adds the dashed computed
                    // page-break lines for this mode.
                    doc.docSettings.pageSettings.mode = PageMode::Pageless;
                    layout.pageMode = PageMode::Pageless;
                    layout::updateLayout(layout, layout.screenWidth,
                                         layout.screenHeight);
                    // No floating toast for a mode switch (modern,
                    // not Word 95): transient status-bar text instead.
                    if (statusComp)
                        status::flash(*statusComp, "Normal view",
                                      afterhours::graphics::get_time());
                    break;
                case 1:  // Page Layout (preview owns pageMode: ignored)
                    if (layout.previewMode) break;
                    cmd::exitOutlineMode(layout, scroll);
                    // See Normal above: the View switch persists.
                    doc.docSettings.pageSettings.mode = PageMode::Paged;
                    layout.pageMode = PageMode::Paged;
                    layout::updateLayout(layout, layout.screenWidth,
                                         layout.screenHeight);
                    // No floating toast for a mode switch (modern,
                    // not Word 95): transient status-bar text instead.
                    if (statusComp)
                        status::flash(*statusComp, "Page Layout",
                                      afterhours::graphics::get_time());
                    break;
                case 3:  // Ruler
                    ruler::toggleRuler(layout);
                    menu.menus[2].items[3].mark =
                        layout.showRuler ? win95::MenuMark::Checkmark
                                         : win95::MenuMark::None;
                    flash(layout.showRuler ? "Ruler: On" : "Ruler: Off");
                    break;
                case 4:  // Header and Footer (toggle: enter on
                         // Header, or exit when already editing)
                    if (layout.headerEditTarget !=
                        HeaderEditTarget::None) {
                        cmd::exitHeaderFooterEdit(layout);
                    } else {
                        cmd::enterHeaderFooterEdit(
                            doc, layout, scroll, HeaderEditTarget::Header);
                    }
                    break;
                case 6:  // Zoom In
                    layout.zoomLevel = std::min(4.0f, layout.zoomLevel + 0.1f);
                    flash("Zoom in");
                    break;
                case 7:  // Zoom Out
                    layout.zoomLevel = std::max(0.5f, layout.zoomLevel - 0.1f);
                    flash("Zoom out");
                    break;
                case 8:  // Zoom Reset
                    layout.zoomLevel = 1.0f;
                    flash("Zoom reset");
                    break;
                case 10:  // Focus Mode
                    layout.focusMode = !layout.focusMode;
                    layout::updateLayout(layout, layout.screenWidth,
                                         layout.screenHeight);
                    flash(layout.focusMode ? "Focus mode: On" : "Focus mode: Off");
                    break;
                case 11:  // Split View
                    layout.splitViewEnabled = !layout.splitViewEnabled;
                    flash(layout.splitViewEnabled ? "Split view: On" : "Split view: Off");
                    break;
                case 12:  // Dark Mode
                    theme::applyDarkMode(!theme::DARK_MODE_ENABLED);
                    flash(theme::DARK_MODE_ENABLED ? "Dark mode: On" : "Dark mode: Off");
                    break;
                case 14:  // Line Width: Normal (no limit)
                    layout::setLineWidthLimit(layout, 0.0f);
                    flash("Line width: Normal");
                    break;
                case 15:  // Line Width: Narrow (60 chars)
                    layout::setLineWidthLimit(layout, 60.0f);
                    flash("Line width: Narrow (60 chars)");
                    break;
                case 16:  // Line Width: Wide (100 chars)
                    layout::setLineWidthLimit(layout, 100.0f);
                    flash("Line width: Wide (100 chars)");
                    break;
                case 18:  // Show Line Numbers
                    layout.showLineNumbers = !layout.showLineNumbers;
                    flash(layout.showLineNumbers ? "Line numbers: On" : "Line numbers: Off");
                    break;
                case 19:  // Outline
                    cmd::toggleOutlineMode(doc, layout, scroll);
                    // Transient status-bar flash, not a floating
                    // toast (same pattern as Normal/Page Layout above).
                    if (statusComp)
                        status::flash(*statusComp,
                                      layout.showOutline ? "Outline mode"
                                                         : "Outline off",
                                      afterhours::graphics::get_time());
                    break;
                case 21:  // Standard Toolbar
                    layout.showStandardToolbar = !layout.showStandardToolbar;
                    layout::updateLayout(layout, layout.screenWidth,
                                         layout.screenHeight);
                    menu.menus[2].items[21].mark =
                        layout.showStandardToolbar ? win95::MenuMark::Checkmark
                                                   : win95::MenuMark::None;
                    flash(layout.showStandardToolbar ? "Standard Toolbar: On" : "Standard Toolbar: Off");
                    break;
                case 22:  // Formatting Toolbar
                    layout.showFormattingToolbar = !layout.showFormattingToolbar;
                    layout::updateLayout(layout, layout.screenWidth,
                                         layout.screenHeight);
                    menu.menus[2].items[22].mark =
                        layout.showFormattingToolbar ? win95::MenuMark::Checkmark
                                                     : win95::MenuMark::None;
                    flash(layout.showFormattingToolbar ? "Formatting Toolbar: On" : "Formatting Toolbar: Off");
                    break;
                case 24:  // UI Scale... (moved from Tools)
                    dialogs.showSettingsDialog = true;
                    {
                        int currentPercentage = static_cast<int>(Settings::get().get_ui_scale() * 100.0f);
                        dialogs.uiScaleInputStr = std::to_string(currentPercentage);
                    }
                    break;
                default:
                    break;
            }
        } else if (menuIndex == 4) {  // Format menu
            // "Font..." is item 0, so every item below it dispatches
            // on itemIndex - 1 and the old case numbers are unchanged.
            if (itemIndex == 0) {  // Font...
                dialogs.showFontDialog = true;
                dialogs.fontDialogInitialized = false;
                dialogs.fontDialogUnderlineDropdownOpen = false;
                dialogs.fontDialogColorDropdownOpen = false;
            } else switch (itemIndex - 1) {
                case 0:  // Normal
                    cmd::setParagraphStyle(doc, ParagraphStyle::Normal);
                    flash("Style: Normal");
                    break;
                case 1:  // Title
                    cmd::setParagraphStyle(doc, ParagraphStyle::Title);
                    flash("Style: Title");
                    break;
                case 2:  // Subtitle
                    cmd::setParagraphStyle(doc, ParagraphStyle::Subtitle);
                    flash("Style: Subtitle");
                    break;
                case 3:  // Heading 1
                    cmd::setParagraphStyle(doc, ParagraphStyle::Heading1);
                    flash("Style: Heading 1");
                    break;
                case 4:  // Heading 2
                    cmd::setParagraphStyle(doc, ParagraphStyle::Heading2);
                    flash("Style: Heading 2");
                    break;
                case 5:  // Heading 3
                    cmd::setParagraphStyle(doc, ParagraphStyle::Heading3);
                    flash("Style: Heading 3");
                    break;
                case 6:  // Heading 4
                    cmd::setParagraphStyle(doc, ParagraphStyle::Heading4);
                    flash("Style: Heading 4");
                    break;
                case 7:  // Heading 5
                    cmd::setParagraphStyle(doc, ParagraphStyle::Heading5);
                    flash("Style: Heading 5");
                    break;
                case 8:  // Heading 6
                    cmd::setParagraphStyle(doc, ParagraphStyle::Heading6);
                    flash("Style: Heading 6");
                    break;
                case 10:  // Bold
                    cmd::toggleBold(doc);
                    break;
                case 11:  // Italic
                    cmd::toggleItalic(doc);
                    break;
                case 12:  // Underline
                    cmd::toggleUnderline(doc);
                    break;
                case 13:  // Strikethrough
                    cmd::toggleStrikethrough(doc);
                    break;
                case 14:  // Superscript
                    cmd::toggleSuperscript(doc);
                    break;
                case 15:  // Subscript
                    cmd::toggleSubscript(doc);
                    break;
                case 20:  // Align Left
                    cmd::setAlignment(doc, TextAlignment::Left);
                    flash("Align: Left");
                    break;
                case 21:  // Align Center
                    cmd::setAlignment(doc, TextAlignment::Center);
                    flash("Align: Center");
                    break;
                case 22:  // Align Right
                    cmd::setAlignment(doc, TextAlignment::Right);
                    flash("Align: Right");
                    break;
                case 23:  // Justify
                    cmd::setAlignment(doc, TextAlignment::Justify);
                    flash("Align: Justify");
                    break;
                case 25:  // Text: Black
                    cmd::setTextColor(doc, TextColors::Black);
                    break;
                case 26:  // Text: Red
                    cmd::setTextColor(doc, TextColors::Red);
                    break;
                case 27:  // Text: Orange
                    cmd::setTextColor(doc, TextColors::Orange);
                    break;
                case 28:  // Text: Green
                    cmd::setTextColor(doc, TextColors::Green);
                    break;
                case 29:  // Text: Blue
                    cmd::setTextColor(doc, TextColors::Blue);
                    break;
                case 30:  // Text: Purple
                    cmd::setTextColor(doc, TextColors::Purple);
                    break;
                case 31:  // Text: Gray
                    cmd::setTextColor(doc, TextColors::Gray);
                    break;
                case 33:  // Highlight: None
                    cmd::setHighlightColor(doc, HighlightColors::None);
                    break;
                case 34:  // Highlight: Yellow
                    cmd::setHighlightColor(doc, HighlightColors::Yellow);
                    break;
                case 35:  // Highlight: Green
                    cmd::setHighlightColor(doc, HighlightColors::Green);
                    break;
                case 36:  // Highlight: Cyan
                    cmd::setHighlightColor(doc, HighlightColors::Cyan);
                    break;
                case 37:  // Highlight: Pink
                    cmd::setHighlightColor(doc, HighlightColors::Pink);
                    break;
                case 38:  // Highlight: Orange
                    cmd::setHighlightColor(doc, HighlightColors::Orange);
                    break;
                case 40:  // Font: Gaegu
                    cmd::setFont(doc, "Gaegu-Bold");
                    break;
                case 41:  // Font: Garamond
                    cmd::setFont(doc, "EBGaramond-Regular");
                    break;
                case 43:  // Increase Size
                    cmd::setFontSize(doc, std::min(72, doc.buffer.effectiveStyleAtCaretOrSelection().fontSize + 2));
                    break;
                case 44:  // Decrease Size
                    cmd::setFontSize(doc, std::max(8, doc.buffer.effectiveStyleAtCaretOrSelection().fontSize - 2));
                    break;
                case 45:  // Reset Size
                    cmd::setFontSize(doc, 16);
                    break;
                case 47:  // Paragraph...
                    // Seed the dialog from the caret paragraph; OK
                    // re-reads the caret/selection then, so a
                    // multi-paragraph selection gets the same format.
                    dialogs.showParagraphDialog = true;
                    dialogs.paragraphTab = 0;
                    dialogs.paragraphLineSpacingDropdownOpen = false;
                    dialogs.paragraphAlignmentDropdownOpen = false;
                    dialogs.paragraphSpecialDropdownOpen = false;
                    {
                        ParagraphFormat fmt =
                            doc.buffer.lineParagraphFormat(
                                doc.buffer.caret().row);
                        dialogs.paragraphLeftStr =
                            paragraph_dialog::storedToInchesString(
                                fmt.indents.left);
                        dialogs.paragraphSpecialIndex =
                            paragraph_dialog::specialIndexFor(
                                fmt.indents.firstLine);
                        dialogs.paragraphByStr =
                            paragraph_dialog::storedToInchesString(
                                std::abs(fmt.indents.firstLine) > 0
                                    ? std::abs(fmt.indents.firstLine)
                                    : static_cast<int>(std::lround(
                                          paragraph_dialog::
                                              kDefaultByInches *
                                          paragraph_dialog::
                                              POINTS_PER_INCH)));
                        dialogs.paragraphRightStr =
                            paragraph_dialog::storedToInchesString(
                                fmt.indents.right);
                        dialogs.paragraphBeforeStr =
                            std::to_string(fmt.spaceBefore);
                        dialogs.paragraphAfterStr =
                            std::to_string(fmt.spaceAfter);
                        dialogs.paragraphLineSpacingIndex =
                            paragraph_dialog::lineSpacingIndexFor(
                                fmt.lineSpacing);
                        dialogs.paragraphAlignmentIndex =
                            paragraph_dialog::alignmentIndexFor(
                                fmt.alignment);
                        dialogs.paragraphPageBreakBefore =
                            fmt.pageBreakBefore;
                        dialogs.paragraphKeepWithNext = fmt.keepWithNext;
                    }
                    break;
                case 48:  // Align Left
                    cmd::setAlignment(doc, TextAlignment::Left);
                    flash("Align: Left");
                    break;
                case 49:  // Align Center
                    cmd::setAlignment(doc, TextAlignment::Center);
                    flash("Align: Center");
                    break;
                case 50:  // Align Right
                    cmd::setAlignment(doc, TextAlignment::Right);
                    flash("Align: Right");
                    break;
                case 51:  // Justify
                    cmd::setAlignment(doc, TextAlignment::Justify);
                    flash("Align: Justify");
                    break;
                case 53:  // Increase Indent
                    cmd::increaseIndent(doc);
                    flash("Indent increased");
                    break;
                case 54:  // Decrease Indent
                    cmd::decreaseIndent(doc);
                    flash("Indent decreased");
                    break;
                case 56:  // Single Spacing
                    cmd::setLineSpacing(doc, 100);
                    flash("Line spacing: Single");
                    break;
                case 57:  // 1.5 Line Spacing
                    cmd::setLineSpacing(doc, 150);
                    flash("Line spacing: 1.5");
                    break;
                case 58:  // Double Spacing
                    cmd::setLineSpacing(doc, 200);
                    flash("Line spacing: Double");
                    break;
                case 60:  // Bulleted List
                    cmd::toggleBulletedList(doc);
                    flash(doc.buffer.currentListType() == ListType::Bulleted ? "Bullets on" : "Bullets off");
                    break;
                case 61:  // Numbered List
                    cmd::toggleNumberedList(doc);
                    flash(doc.buffer.currentListType() == ListType::Numbered ? "Numbering on" : "Numbering off");
                    break;
                case 62:  // Increase List Level
                    doc.buffer.increaseListLevel();
                    flash("List level increased");
                    break;
                case 63:  // Decrease List Level
                    doc.buffer.decreaseListLevel();
                    flash("List level decreased");
                    break;
                case 65:  // Increase Space Before
                    cmd::setSpaceBefore(doc, doc.buffer.currentSpaceBefore() + 6);
                    flash("Space before increased");
                    break;
                case 66:  // Decrease Space Before
                    cmd::setSpaceBefore(doc, doc.buffer.currentSpaceBefore() - 6);
                    flash("Space before decreased");
                    break;
                case 67:  // Increase Space After
                    cmd::setSpaceAfter(doc, doc.buffer.currentSpaceAfter() + 6);
                    flash("Space after increased");
                    break;
                case 68:  // Decrease Space After
                    cmd::setSpaceAfter(doc, doc.buffer.currentSpaceAfter() - 6);
                    flash("Space after decreased");
                    break;
                case 70:  // Drop Cap... (dialog)
                    dialogs.showDropCapDialog = true;
                    dialogs.dropCapPosition =
                        doc.buffer.currentLineHasDropCap() ? 1 : 0;
                    dialogs.dropCapLines =
                        std::max(2, doc.buffer.currentDropCapLines());
                    break;
                case 71:  // Tab Width...
                    dialogs.showTabWidthDialog = true;
                    break;
                case 72:  // Borders and Shading...
                    // Seed the dialog from the caret paragraph; OK
                    // re-reads the caret/selection then, so a
                    // multi-paragraph selection gets the same border
                    // + shading (like Paragraph...).
                    dialogs.showBordersShadingDialog = true;
                    {
                        ParagraphFormat fmt =
                            doc.buffer.lineParagraphFormat(
                                doc.buffer.caret().row);
                        dialogs.bordersEdges = fmt.borderEdges;
                        dialogs.bordersStyle = fmt.borderStyle;
                        dialogs.bordersShadingIndex = fmt.shadingColorIndex;
                    }
                    break;
                case 73:  // Columns...
                    dialogs.showColumnsDialog = true;
                    dialogs.columnsSelected =
                        std::clamp(doc.docSettings.pageSettings.columnCount,
                                   1, 4);
                    dialogs.columnsSpacingStr =
                        columns_math::pointsToInchesString(
                            doc.docSettings.pageSettings.columnSpacing);
                    break;
                default:
                    break;
            }
        } else if (menuIndex == 3) {  // Insert menu
            switch (itemIndex) {
                case 0:  // Page Break
                    doc.buffer.insertPageBreak();
                    doc.isDirty = true;
                    flash("Page break inserted");
                    break;
                case 1:  // Section Break
                    doc.buffer.insertSectionBreak();
                    doc.isDirty = true;
                    flash("Section break inserted");
                    break;
                case 3:  // Hyperlink...
                    cmd::insertHyperlink(doc);
                    break;
                case 4:  // Remove Hyperlink
                    if (doc.buffer.hyperlinkAtCaret()) {
                        std::size_t caretOffset = doc.buffer.caretOffset();
                        if (doc.buffer.removeHyperlink(caretOffset)) {
                            doc.isDirty = true;
                            flash("Hyperlink removed");
                        }
                    } else {
                        toast_notify::error("No hyperlink at cursor");
                    }
                    break;
                case 5:  // Bookmark...
                {
                    std::string name =
                        std::format("bookmark_{}", doc.buffer.caret().row + 1);
                    if (doc.buffer.addBookmark(name)) {
                        flash("Bookmark added");
                    } else {
                        toast_notify::error("Bookmark not added");
                    }
                    break;
                }
                case 6:  // Comment...
                    if (doc.buffer.hasSelection()) {
                        CaretPosition start = doc.buffer.selectionStart();
                        CaretPosition end = doc.buffer.selectionEnd();
                        dialogs.pendingCommentStart =
                            doc.buffer.offsetForPosition(start);
                        dialogs.pendingCommentEnd = doc.buffer.offsetForPosition(end);
                        dialogs.showCommentDialog = true;
                    } else {
                        toast_notify::error("Select text to comment");
                    }
                    break;
                case 8:  // Table...
                {
                    std::size_t currentLine = doc.buffer.caret().row;
                    doc.insertTable(currentLine, 3, 3);
                    doc.isDirty = true;
                    flash("Inserted 3x3 table");
                    break;
                }
                case 9:  // Image...
                    cmd::insertImagePlaceholder(doc);
                    break;
                case 13:  // Shape: Line
                case 14:  // Shape: Rectangle
                case 15:  // Shape: Circle
                case 16:  // Shape: Ellipse
                case 17:  // Shape: Arrow
                case 18:  // Shape: Rounded Rect
                case 19:  // Shape: Triangle
                {
                    DocumentDrawing drawing;
                    drawing.anchorLine = doc.buffer.caret().row;
                    drawing.anchorColumn = doc.buffer.caret().column;
                    drawing.x = 10.0f;
                    drawing.y = 10.0f;
                    drawing.width = 100.0f;
                    drawing.height = 50.0f;
                    drawing.strokeColor = DrawingColors::Black;
                    drawing.fillColor = DrawingColors::LightGray;
                    drawing.strokeWidth = 2.0f;
                    drawing.layoutMode = DrawingLayoutMode::Inline;
                    
                    switch (itemIndex) {
                        case 13: drawing.shapeType = ShapeType::Line; break;
                        case 14: drawing.shapeType = ShapeType::Rectangle; break;
                        case 15: drawing.shapeType = ShapeType::Ellipse; break;
                        case 16: drawing.shapeType = ShapeType::Ellipse; break;
                        case 17: drawing.shapeType = ShapeType::Arrow; break;
                        case 18: drawing.shapeType = ShapeType::RoundedRect; break;
                        case 19: drawing.shapeType = ShapeType::Triangle; break;
                        default: break;
                    }
                    
                    doc.drawings.addDrawing(drawing);
                    doc.isDirty = true;
                    flash(std::format("{} inserted", shapeTypeName(drawing.shapeType)));
                    break;
                }
                case 21:  // Equation...
                {
                    DocumentEquation eq;
                    eq.anchorLine = doc.buffer.caret().row;
                    eq.anchorColumn = doc.buffer.caret().column;
                    eq.source = "f(x) = x^2";
                    eq.style = EquationStyle::Inline;
                    doc.equations.addEquation(eq);
                    doc.isDirty = true;
                    flash("Equation inserted");
                    break;
                }
                case 22:  // Footnote
                {
                    if (doc.buffer.addFootnote("")) {
                        doc.isDirty = true;
                        flash("Footnote inserted");
                    } else {
                        toast_notify::error("Failed to insert footnote");
                    }
                    break;
                }
                case 23:  // Date and Time...
                    dialogs.showDateTimeDialog = true;
                    dialogs.dateTimeFormats =
                        date_time::dateTimeFormats(date_time::localTimeNow());
                    dialogs.dateTimeSelected = 0;
                    dialogs.dateTimeUpdateAutomatically = false;
                    break;
                case 24:  // Special Character...
                    dialogs.showSymbolDialog = true;
                    dialogs.symbolSelected = 0;
                    dialogs.symbolTab = 0;
                    dialogs.symbolFontIndex = 0;
                    dialogs.symbolFontDropdownOpen = false;
                    break;
                case 26:  // Header — enter header editing
                    // (the "[HEADER]" body-text placeholder is gone:
                    // headers are a real buffer now, Batch 9).
                    cmd::enterHeaderFooterEdit(doc, layout, scroll,
                                               HeaderEditTarget::Header);
                    break;
                case 27:  // Footer — enter footer editing
                    cmd::enterHeaderFooterEdit(doc, layout, scroll,
                                               HeaderEditTarget::Footer);
                    break;
                case 28:  // Page Number — in header/footer editing
                          // toggles the page-number field of the
                          // active target; in the body it still
                          // inserts the "[PAGE #]" placeholder.
                    if (layout.headerEditTarget !=
                        HeaderEditTarget::None) {
                        cmd::toggleHeaderFooterPageNumber(doc, layout);
                    } else {
                        doc.buffer.insertText("[PAGE #]");
                        doc.isDirty = true;
                        flash("Page number placeholder inserted");
                    }
                    break;
                case 29:  // Document Body — exit header/footer
                          // editing back to the body caret.
                    if (layout.headerEditTarget !=
                        HeaderEditTarget::None) {
                        cmd::exitHeaderFooterEdit(layout);
                    }
                    break;
                case 31:  // Table of Contents
                    if (cmd::insertToc(doc, layout)) {
                        doc.isDirty = true;
                    }
                    break;
                case 32:  // Update Table of Contents
                    cmd::updateToc(doc, layout);
                    break;
                default:
                    break;
            }
        } else if (menuIndex == 6) {  // Table menu
            switch (itemIndex) {
                case 0: {  // Insert Table...
                    std::size_t currentLine = doc.buffer.caret().row;
                    doc.insertTable(currentLine, 3, 3);
                    doc.isDirty = true;
                    flash("Inserted 3x3 table");
                    break;
                }
                case 2: {  // Insert Row Above
                    Table* table = cmd::tableAtCaret(doc);
                    if (table) {
                        table->insertRowAbove(table->currentCell().row);
                        doc.isDirty = true;
                        flash("Row inserted above");
                    } else {
                        toast_notify::error("No table at cursor");
                    }
                    break;
                }
                case 3: {  // Insert Row Below
                    Table* table = cmd::tableAtCaret(doc);
                    if (table) {
                        table->insertRowBelow(table->currentCell().row);
                        doc.isDirty = true;
                        flash("Row inserted below");
                    } else {
                        toast_notify::error("No table at cursor");
                    }
                    break;
                }
                case 4: {  // Insert Column Left
                    Table* table = cmd::tableAtCaret(doc);
                    if (table) {
                        table->insertColumnLeft(table->currentCell().col);
                        doc.isDirty = true;
                        flash("Column inserted left");
                    } else {
                        toast_notify::error("No table at cursor");
                    }
                    break;
                }
                case 5: {  // Insert Column Right
                    Table* table = cmd::tableAtCaret(doc);
                    if (table) {
                        table->insertColumnRight(table->currentCell().col);
                        doc.isDirty = true;
                        flash("Column inserted right");
                    } else {
                        toast_notify::error("No table at cursor");
                    }
                    break;
                }
                case 7: {  // Delete Row
                    Table* table = cmd::tableAtCaret(doc);
                    if (table) {
                        table->deleteRow(table->currentCell().row);
                        doc.isDirty = true;
                        flash("Row deleted");
                    } else {
                        toast_notify::error("No table at cursor");
                    }
                    break;
                }
                case 8: {  // Delete Column
                    Table* table = cmd::tableAtCaret(doc);
                    if (table) {
                        table->deleteColumn(table->currentCell().col);
                        doc.isDirty = true;
                        flash("Column deleted");
                    } else {
                        toast_notify::error("No table at cursor");
                    }
                    break;
                }
                case 10: {  // Merge Cells
                    Table* table = cmd::tableAtCaret(doc);
                    if (table && table->hasSelection()) {
                        CellPosition start = table->selectionStart();
                        CellPosition end = table->selectionEnd();
                        CellPosition topLeft = {std::min(start.row, end.row), std::min(start.col, end.col)};
                        CellPosition bottomRight = {std::max(start.row, end.row), std::max(start.col, end.col)};
                        if (table->mergeCells(topLeft, bottomRight)) {
                            doc.isDirty = true;
                            flash("Cells merged");
                        } else {
                            toast_notify::error("Cannot merge cells");
                        }
                    } else {
                        toast_notify::error("Select cells to merge");
                    }
                    break;
                }
                case 11: {  // Split Cell
                    Table* table = cmd::tableAtCaret(doc);
                    if (table) {
                        if (table->splitCell(table->currentCell())) {
                            doc.isDirty = true;
                            flash("Cell split");
                        } else {
                            toast_notify::error("Cell is not merged");
                        }
                    } else {
                        toast_notify::error("No table at cursor");
                    }
                    break;
                }
                case 13:  // Sort Ascending (by the caret's column)
                    cmd::sortTable(doc, true);
                    break;
                case 14:  // Sort Descending (by the caret's column)
                    cmd::sortTable(doc, false);
                    break;
                case 15:  // Formula...
                    cmd::openTableFormulaDialog(doc, dialogs);
                    break;
                default:
                    break;
            }
        } else if (menuIndex == 7) {  // Window menu
            switch (itemIndex) {
                case 0: {  // New Window: second instance of our own
                           // executable onto the same file (no file:
                           // plain new instance). std::system with the
                           // exe path from argv[0] (MenuComponent::
                           // exePath); the command detaches, so this
                           // instance is not blocked.
                    std::string cmd = window_menu::newWindowCommand(
                        menu.exePath, doc.filePath);
                    if (!cmd.empty()) std::system(cmd.c_str());
                    break;
                }
                case 2:  // Split (same as View > Split View)
                    layout.splitViewEnabled = !layout.splitViewEnabled;
                    flash(layout.splitViewEnabled ? "Split view: On" : "Split view: Off");
                    break;
                case 1:  // Arrange All — disabled: never dispatched
                case 4:  // "1 <current document>" — checked, enabled:
                         // clicking it is a no-op (stays on this doc)
                default:
                    break;
            }
        } else if (menuIndex == 8) {  // Help menu
            if (itemIndex == 0) {     // Keyboard Shortcuts
                dialogs.showHelpWindow = true;
            } else if (itemIndex == 2) {  // About (after separator)
                dialogs.showAboutDialog = true;
            }
        } else if (menuIndex == 5) {  // Tools menu (Word 95 order)
            switch (itemIndex) {
                case 0:  // Spelling...
                    cmd::openSpellingDialog(doc, dialogs);
                    break;
                case 1:  // Word Count...
                    dialogs.showWordCountDialog = true;
                    break;
                case 2:  // AutoCorrect...
                    dialogs.showAutoCorrectDialog = true;
                    // Fresh selection/fields on every open.
                    dialogs.autoCorrectSelected = -1;
                    dialogs.autoCorrectReplaceStr.clear();
                    dialogs.autoCorrectWithStr.clear();
                    break;
                case 4:  // Grammar...
                    cmd::openGrammarDialog(doc, dialogs);
                    break;
                case 5: {  // AutoFormat...: AutoCorrect batch pass,
                           // one undo step.
                    std::size_t corrected = cmd::autoFormatDocument(doc);
                    if (corrected == 0) {
                        flash("AutoFormat: no corrections");
                    } else {
                        flash(
                            "AutoFormat: " + std::to_string(corrected) +
                            (corrected == 1 ? " correction" : " corrections"));
                    }
                    break;
                }
                case 6:  // Revisions... (suggesting mode)
                    dialogs.showRevisionsDialog = true;
                    dialogs.revisionsSelected =
                        doc.revisions.empty() ? -1 : 0;
                    break;
                case 8:  // Mail Merge... — DISABLED: never dispatched
                         // (menu item is enabled=false; the dialog code
                         // stays unreachable while disabled).
                    break;
                case 10:  // Options...
                    dialogs.showOptionsDialog = true;
                    break;
                case 12:  // Thesaurus...
                    cmd::openThesaurusDialog(doc, dialogs);
                    break;
                default:
                    break;
            }
        }
    }

}  // namespace ecs
