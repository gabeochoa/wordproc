#pragma once

#include <filesystem>
#include <format>

#include "../util/clipboard.h"
#include "../editor/document_io.h"
#include "../editor/export/export_html.h"
#include "../editor/export/export_pdf.h"
#include "../editor/export/export_rtf.h"
#include "../editor/drawing.h"
#include "../editor/equation.h"
#include "../editor/image.h"
#include "../editor/table.h"
#include "../settings.h"
#include "../ui/theme.h"
#include "../ui/ui_context.h"
#include "../ui/menu_setup.h"
#include "components.h"
#include "document_commands.h"

namespace ecs {

inline void handleMenuActionImpl(int menuResult, DocumentComponent& doc,
                          MenuComponent& menu, DialogState& dialogs,
                          LayoutComponent& layout) {
        int menuIndex = menuResult / 100;
        int itemIndex = menuResult % 100;

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
                    return;
                }
            }
            switch (itemIndex) {
                case 0:  // New
                    cmd::newDocument(doc);
                    break;
                case 1:  // New from Template...
                    dialogs.showTemplateDialog = true;
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
                case 6:  // Export PDF
                {
                    std::filesystem::path basePath =
                        doc.filePath.empty() ? doc.defaultPath : doc.filePath;
                    basePath.replace_extension(".pdf");
                    auto result = exportDocumentPdf(doc.buffer, doc.docSettings,
                                                    basePath.string());
                    if (result.success) {
                        toast_notify::success(
                            "Exported PDF: " + basePath.filename().string());
                    } else {
                        toast_notify::error("Export PDF failed: " + result.error);
                    }
                } break;
                case 7:  // Export HTML
                {
                    std::filesystem::path basePath =
                        doc.filePath.empty() ? doc.defaultPath : doc.filePath;
                    basePath.replace_extension(".html");
                    auto result = exportDocumentHtml(doc.buffer, doc.docSettings,
                                                     basePath.string());
                    if (result.success) {
                        toast_notify::success(
                            "Exported HTML: " + basePath.filename().string());
                    } else {
                        toast_notify::error("Export HTML failed: " + result.error);
                    }
                } break;
                case 8:  // Export RTF
                {
                    std::filesystem::path basePath =
                        doc.filePath.empty() ? doc.defaultPath : doc.filePath;
                    basePath.replace_extension(".rtf");
                    auto result = exportDocumentRtf(doc.buffer, doc.docSettings,
                                                    basePath.string());
                    if (result.success) {
                        toast_notify::success(
                            "Exported RTF: " + basePath.filename().string());
                    } else {
                        toast_notify::error("Export RTF failed: " + result.error);
                    }
                } break;
                case 10:  // Page Setup
                {
                    dialogs.showPageSetup = !dialogs.showPageSetup;
                } break;
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
                    if (doc.trackChangesEnabled) {
                        doc.trackChangesBaseline = doc.buffer.getText();
                        menu.menus[1].items[3].mark = win95::MenuMark::Checkmark;
                        toast_notify::info("Track Changes: On");
                    } else {
                        menu.menus[1].items[3].mark = win95::MenuMark::None;
                        toast_notify::info("Track Changes: Off");
                    }
                    break;
                case 4:  // Accept All Changes
                    doc.revisions.clear();
                    doc.trackChangesBaseline = doc.buffer.getText();
                    toast_notify::success("All changes accepted");
                    break;
                case 5:  // Reject All Changes
                    if (!doc.trackChangesBaseline.empty()) {
                        doc.buffer.setText(doc.trackChangesBaseline);
                        doc.isDirty = true;
                    }
                    doc.revisions.clear();
                    toast_notify::success("All changes rejected");
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
                    toast_notify::info("Find: Ctrl+G next, Ctrl+Shift+G prev");
                    break;
                case 14:  // Find Next
                    if (!dialogs.lastSearchTerm.empty()) {
                        FindResult result = doc.buffer.findNext(dialogs.lastSearchTerm, dialogs.findOptions);
                        if (result.found) {
                            doc.buffer.setCaret(result.start);
                            doc.buffer.setSelectionAnchor(result.start);
                            doc.buffer.setCaret(result.end);
                            doc.buffer.updateSelectionToCaret();
                            toast_notify::info("Found", 2.0f);
                        } else {
                            toast_notify::warning("Not found");
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
                            toast_notify::info("Found", 2.0f);
                        } else {
                            toast_notify::warning("Not found");
                        }
                    }
                    break;
                case 16:  // Replace...
                    dialogs.showFindDialog = true;
                    dialogs.findReplaceMode = true;
                    toast_notify::info("Replace mode");
                    break;
                case 18:  // Go To Bookmark...
                    dialogs.showBookmarkListDialog = true;
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
                    toast_notify::info("Switched to Pageless mode");
                    break;
                case 1:  // Paged Mode
                    layout.pageMode = PageMode::Paged;
                    layout::updateLayout(layout, layout.screenWidth,
                                         layout.screenHeight);
                    toast_notify::info("Switched to Paged mode");
                    break;
                case 3:  // Zoom In
                    layout.zoomLevel = std::min(4.0f, layout.zoomLevel + 0.1f);
                    toast_notify::info("Zoom in");
                    break;
                case 4:  // Zoom Out
                    layout.zoomLevel = std::max(0.5f, layout.zoomLevel - 0.1f);
                    toast_notify::info("Zoom out");
                    break;
                case 5:  // Zoom Reset
                    layout.zoomLevel = 1.0f;
                    toast_notify::info("Zoom reset");
                    break;
                case 7:  // Focus Mode
                    layout.focusMode = !layout.focusMode;
                    layout::updateLayout(layout, layout.screenWidth,
                                         layout.screenHeight);
                    toast_notify::info(layout.focusMode ? "Focus mode: On" : "Focus mode: Off");
                    break;
                case 8:  // Split View
                    layout.splitViewEnabled = !layout.splitViewEnabled;
                    toast_notify::info(layout.splitViewEnabled ? "Split view: On" : "Split view: Off");
                    break;
                case 9:  // Dark Mode
                    theme::applyDarkMode(!theme::DARK_MODE_ENABLED);
                    toast_notify::info(theme::DARK_MODE_ENABLED ? "Dark mode: On" : "Dark mode: Off");
                    break;
                case 11:  // Line Width: Normal (no limit)
                    layout::setLineWidthLimit(layout, 0.0f);
                    toast_notify::info("Line width: Normal");
                    break;
                case 12:  // Line Width: Narrow (60 chars)
                    layout::setLineWidthLimit(layout, 60.0f);
                    toast_notify::info("Line width: Narrow (60 chars)");
                    break;
                case 13:  // Line Width: Wide (100 chars)
                    layout::setLineWidthLimit(layout, 100.0f);
                    toast_notify::info("Line width: Wide (100 chars)");
                    break;
                case 15:  // Show Line Numbers
                    layout.showLineNumbers = !layout.showLineNumbers;
                    toast_notify::info(layout.showLineNumbers ? "Line numbers: On" : "Line numbers: Off");
                    break;
                case 16:  // Show Outline
                    layout.showOutline = !layout.showOutline;
                    toast_notify::info(layout.showOutline ? "Outline: On" : "Outline: Off");
                    break;
                default:
                    break;
            }
        } else if (menuIndex == 4) {  // Format menu
            TextStyle style = doc.buffer.textStyle();
            switch (itemIndex) {
                case 0:  // Normal
                    doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Normal);
                    toast_notify::info("Style: Normal");
                    break;
                case 1:  // Title
                    doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Title);
                    toast_notify::info("Style: Title");
                    break;
                case 2:  // Subtitle
                    doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Subtitle);
                    toast_notify::info("Style: Subtitle");
                    break;
                case 3:  // Heading 1
                    doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Heading1);
                    toast_notify::info("Style: Heading 1");
                    break;
                case 4:  // Heading 2
                    doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Heading2);
                    toast_notify::info("Style: Heading 2");
                    break;
                case 5:  // Heading 3
                    doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Heading3);
                    toast_notify::info("Style: Heading 3");
                    break;
                case 6:  // Heading 4
                    doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Heading4);
                    toast_notify::info("Style: Heading 4");
                    break;
                case 7:  // Heading 5
                    doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Heading5);
                    toast_notify::info("Style: Heading 5");
                    break;
                case 8:  // Heading 6
                    doc.buffer.setCurrentParagraphStyle(ParagraphStyle::Heading6);
                    toast_notify::info("Style: Heading 6");
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
                    doc.buffer.setCurrentAlignment(TextAlignment::Left);
                    toast_notify::info("Align: Left");
                    break;
                case 21:  // Align Center
                    doc.buffer.setCurrentAlignment(TextAlignment::Center);
                    toast_notify::info("Align: Center");
                    break;
                case 22:  // Align Right
                    doc.buffer.setCurrentAlignment(TextAlignment::Right);
                    toast_notify::info("Align: Right");
                    break;
                case 23:  // Justify
                    doc.buffer.setCurrentAlignment(TextAlignment::Justify);
                    toast_notify::info("Align: Justify");
                    break;
                case 25:  // Text: Black
                    style.textColor = TextColors::Black;
                    doc.buffer.setTextStyle(style);
                    break;
                case 26:  // Text: Red
                    style.textColor = TextColors::Red;
                    doc.buffer.setTextStyle(style);
                    break;
                case 27:  // Text: Orange
                    style.textColor = TextColors::Orange;
                    doc.buffer.setTextStyle(style);
                    break;
                case 28:  // Text: Green
                    style.textColor = TextColors::Green;
                    doc.buffer.setTextStyle(style);
                    break;
                case 29:  // Text: Blue
                    style.textColor = TextColors::Blue;
                    doc.buffer.setTextStyle(style);
                    break;
                case 30:  // Text: Purple
                    style.textColor = TextColors::Purple;
                    doc.buffer.setTextStyle(style);
                    break;
                case 31:  // Text: Gray
                    style.textColor = TextColors::Gray;
                    doc.buffer.setTextStyle(style);
                    break;
                case 33:  // Highlight: None
                    style.highlightColor = HighlightColors::None;
                    doc.buffer.setTextStyle(style);
                    break;
                case 34:  // Highlight: Yellow
                    style.highlightColor = HighlightColors::Yellow;
                    doc.buffer.setTextStyle(style);
                    break;
                case 35:  // Highlight: Green
                    style.highlightColor = HighlightColors::Green;
                    doc.buffer.setTextStyle(style);
                    break;
                case 36:  // Highlight: Cyan
                    style.highlightColor = HighlightColors::Cyan;
                    doc.buffer.setTextStyle(style);
                    break;
                case 37:  // Highlight: Pink
                    style.highlightColor = HighlightColors::Pink;
                    doc.buffer.setTextStyle(style);
                    break;
                case 38:  // Highlight: Orange
                    style.highlightColor = HighlightColors::Orange;
                    doc.buffer.setTextStyle(style);
                    break;
                case 40:  // Font: Gaegu
                    style.font = "Gaegu-Bold";
                    doc.buffer.setTextStyle(style);
                    break;
                case 41:  // Font: Garamond
                    style.font = "EBGaramond-Regular";
                    doc.buffer.setTextStyle(style);
                    break;
                case 43:  // Increase Size
                    style.fontSize = std::min(72, style.fontSize + 2);
                    doc.buffer.setTextStyle(style);
                    break;
                case 44:  // Decrease Size
                    style.fontSize = std::max(8, style.fontSize - 2);
                    doc.buffer.setTextStyle(style);
                    break;
                case 45:  // Reset Size
                    style.fontSize = 16;
                    doc.buffer.setTextStyle(style);
                    break;
                case 47:  // Align Left
                    doc.buffer.setCurrentAlignment(TextAlignment::Left);
                    toast_notify::info("Align: Left");
                    break;
                case 48:  // Align Center
                    doc.buffer.setCurrentAlignment(TextAlignment::Center);
                    toast_notify::info("Align: Center");
                    break;
                case 49:  // Align Right
                    doc.buffer.setCurrentAlignment(TextAlignment::Right);
                    toast_notify::info("Align: Right");
                    break;
                case 50:  // Justify
                    doc.buffer.setCurrentAlignment(TextAlignment::Justify);
                    toast_notify::info("Align: Justify");
                    break;
                case 52:  // Increase Indent
                    doc.buffer.increaseIndent();
                    toast_notify::info("Indent increased");
                    break;
                case 53:  // Decrease Indent
                    doc.buffer.decreaseIndent();
                    toast_notify::info("Indent decreased");
                    break;
                case 55:  // Single Spacing
                    doc.buffer.setLineSpacingSingle();
                    toast_notify::info("Line spacing: Single");
                    break;
                case 56:  // 1.5 Line Spacing
                    doc.buffer.setLineSpacing1_5();
                    toast_notify::info("Line spacing: 1.5");
                    break;
                case 57:  // Double Spacing
                    doc.buffer.setLineSpacingDouble();
                    toast_notify::info("Line spacing: Double");
                    break;
                case 59:  // Bulleted List
                    doc.buffer.toggleBulletedList();
                    toast_notify::info(doc.buffer.currentListType() == ListType::Bulleted ? "Bullets on" : "Bullets off");
                    break;
                case 60:  // Numbered List
                    doc.buffer.toggleNumberedList();
                    toast_notify::info(doc.buffer.currentListType() == ListType::Numbered ? "Numbering on" : "Numbering off");
                    break;
                case 61:  // Increase List Level
                    doc.buffer.increaseListLevel();
                    toast_notify::info("List level increased");
                    break;
                case 62:  // Decrease List Level
                    doc.buffer.decreaseListLevel();
                    toast_notify::info("List level decreased");
                    break;
                case 64:  // Increase Space Before
                    doc.buffer.setCurrentSpaceBefore(doc.buffer.currentSpaceBefore() + 6);
                    toast_notify::info("Space before increased");
                    break;
                case 65:  // Decrease Space Before
                    doc.buffer.setCurrentSpaceBefore(doc.buffer.currentSpaceBefore() - 6);
                    toast_notify::info("Space before decreased");
                    break;
                case 66:  // Increase Space After
                    doc.buffer.setCurrentSpaceAfter(doc.buffer.currentSpaceAfter() + 6);
                    toast_notify::info("Space after increased");
                    break;
                case 67:  // Decrease Space After
                    doc.buffer.setCurrentSpaceAfter(doc.buffer.currentSpaceAfter() - 6);
                    toast_notify::info("Space after decreased");
                    break;
                case 69:  // Drop Cap
                    doc.buffer.toggleCurrentLineDropCap();
                    toast_notify::info(doc.buffer.currentLineHasDropCap() ? "Drop cap: On" : "Drop cap: Off");
                    break;
                case 70:  // Tab Width...
                    dialogs.showTabWidthDialog = true;
                    break;
                default:
                    break;
            }
        } else if (menuIndex == 3) {  // Insert menu
            switch (itemIndex) {
                case 0:  // Page Break
                    doc.buffer.insertPageBreak();
                    doc.isDirty = true;
                    toast_notify::info("Page break inserted");
                    break;
                case 1:  // Section Break
                    doc.buffer.insertSectionBreak();
                    doc.isDirty = true;
                    toast_notify::info("Section break inserted");
                    break;
                case 3:  // Hyperlink...
                    if (doc.buffer.hasSelection()) {
                        if (doc.buffer.addHyperlink("https://example.com")) {
                            doc.isDirty = true;
                            toast_notify::info("Hyperlink added (edit URL)");
                        }
                    } else {
                        toast_notify::error("Select text first");
                    }
                    break;
                case 4:  // Remove Hyperlink
                    if (doc.buffer.hyperlinkAtCaret()) {
                        std::size_t caretOffset = doc.buffer.caretOffset();
                        if (doc.buffer.removeHyperlink(caretOffset)) {
                            doc.isDirty = true;
                            toast_notify::success("Hyperlink removed");
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
                        toast_notify::success("Bookmark added");
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
                    toast_notify::info("Inserted 3x3 table");
                    break;
                }
                case 9:  // Image...
                {
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
                    toast_notify::info("Image placeholder inserted");
                    break;
                }
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
                    toast_notify::info(std::format("{} inserted", shapeTypeName(drawing.shapeType)));
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
                    toast_notify::info("Equation inserted");
                    break;
                }
                case 22:  // Footnote
                {
                    if (doc.buffer.addFootnote("")) {
                        doc.isDirty = true;
                        toast_notify::info("Footnote inserted");
                    } else {
                        toast_notify::error("Failed to insert footnote");
                    }
                    break;
                }
                case 25:  // Header
                {
                    doc.buffer.insertText("[HEADER]");
                    doc.isDirty = true;
                    toast_notify::info("Header placeholder inserted");
                    break;
                }
                case 26:  // Footer
                {
                    doc.buffer.insertText("[FOOTER]");
                    doc.isDirty = true;
                    toast_notify::info("Footer placeholder inserted");
                    break;
                }
                case 27:  // Page Number
                {
                    doc.buffer.insertText("[PAGE #]");
                    doc.isDirty = true;
                    toast_notify::info("Page number placeholder inserted");
                    break;
                }
                case 30:  // Table of Contents
                {
                    doc.buffer.insertTableOfContents();
                    doc.isDirty = true;
                    toast_notify::info("Table of Contents inserted");
                    break;
                }
                default:
                    break;
            }
        } else if (menuIndex == 6) {  // Table menu
            switch (itemIndex) {
                case 0: {  // Insert Table...
                    std::size_t currentLine = doc.buffer.caret().row;
                    doc.insertTable(currentLine, 3, 3);
                    doc.isDirty = true;
                    toast_notify::info("Inserted 3x3 table");
                    break;
                }
                case 2: {  // Insert Row Above
                    std::size_t currentLine = doc.buffer.caret().row;
                    Table* table = doc.tableAtLine(currentLine);
                    if (table) {
                        table->insertRowAbove(table->currentCell().row);
                        doc.isDirty = true;
                        toast_notify::info("Row inserted above");
                    } else {
                        toast_notify::error("No table at cursor");
                    }
                    break;
                }
                case 3: {  // Insert Row Below
                    std::size_t currentLine = doc.buffer.caret().row;
                    Table* table = doc.tableAtLine(currentLine);
                    if (table) {
                        table->insertRowBelow(table->currentCell().row);
                        doc.isDirty = true;
                        toast_notify::info("Row inserted below");
                    } else {
                        toast_notify::error("No table at cursor");
                    }
                    break;
                }
                case 4: {  // Insert Column Left
                    std::size_t currentLine = doc.buffer.caret().row;
                    Table* table = doc.tableAtLine(currentLine);
                    if (table) {
                        table->insertColumnLeft(table->currentCell().col);
                        doc.isDirty = true;
                        toast_notify::info("Column inserted left");
                    } else {
                        toast_notify::error("No table at cursor");
                    }
                    break;
                }
                case 5: {  // Insert Column Right
                    std::size_t currentLine = doc.buffer.caret().row;
                    Table* table = doc.tableAtLine(currentLine);
                    if (table) {
                        table->insertColumnRight(table->currentCell().col);
                        doc.isDirty = true;
                        toast_notify::info("Column inserted right");
                    } else {
                        toast_notify::error("No table at cursor");
                    }
                    break;
                }
                case 7: {  // Delete Row
                    std::size_t currentLine = doc.buffer.caret().row;
                    Table* table = doc.tableAtLine(currentLine);
                    if (table) {
                        table->deleteRow(table->currentCell().row);
                        doc.isDirty = true;
                        toast_notify::info("Row deleted");
                    } else {
                        toast_notify::error("No table at cursor");
                    }
                    break;
                }
                case 8: {  // Delete Column
                    std::size_t currentLine = doc.buffer.caret().row;
                    Table* table = doc.tableAtLine(currentLine);
                    if (table) {
                        table->deleteColumn(table->currentCell().col);
                        doc.isDirty = true;
                        toast_notify::info("Column deleted");
                    } else {
                        toast_notify::error("No table at cursor");
                    }
                    break;
                }
                case 10: {  // Merge Cells
                    std::size_t currentLine = doc.buffer.caret().row;
                    Table* table = doc.tableAtLine(currentLine);
                    if (table && table->hasSelection()) {
                        CellPosition start = table->selectionStart();
                        CellPosition end = table->selectionEnd();
                        CellPosition topLeft = {std::min(start.row, end.row), std::min(start.col, end.col)};
                        CellPosition bottomRight = {std::max(start.row, end.row), std::max(start.col, end.col)};
                        if (table->mergeCells(topLeft, bottomRight)) {
                            doc.isDirty = true;
                            toast_notify::success("Cells merged");
                        } else {
                            toast_notify::error("Cannot merge cells");
                        }
                    } else {
                        toast_notify::error("Select cells to merge");
                    }
                    break;
                }
                case 11: {  // Split Cell
                    std::size_t currentLine = doc.buffer.caret().row;
                    Table* table = doc.tableAtLine(currentLine);
                    if (table) {
                        if (table->splitCell(table->currentCell())) {
                            doc.isDirty = true;
                            toast_notify::success("Cell split");
                        } else {
                            toast_notify::error("Cell is not merged");
                        }
                    } else {
                        toast_notify::error("No table at cursor");
                    }
                    break;
                }
                default:
                    break;
            }
        } else if (menuIndex == 7) {  // Help menu
            if (itemIndex == 0) {     // Keyboard Shortcuts
                dialogs.showHelpWindow = true;
            } else if (itemIndex == 2) {  // About (after separator)
                dialogs.showAboutDialog = true;
            }
        } else if (menuIndex == 5) {  // Tools menu (includes former Settings items)
            switch (itemIndex) {
                case 0:  // Word Count...
                    dialogs.showWordCountDialog = true;
                    break;
                case 2:  // UI Scale... (was Settings item 0)
                    dialogs.showSettingsDialog = true;
                    {
                        int currentPercentage = static_cast<int>(Settings::get().get_ui_scale() * 100.0f);
                        dialogs.uiScaleInputStr = std::to_string(currentPercentage);
                    }
                    break;
                default:
                    break;
            }
        }
    }

}  // namespace ecs
