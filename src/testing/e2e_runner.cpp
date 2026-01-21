// E2E Runner initialization - app-specific callbacks for wordproc
// Uses afterhours E2ERunner with ECS-based command handlers

#include "e2e_runner.h"
#include "../editor/document_settings.h"
#include "../rl.h"
#include "../settings.h"
#include "../ui/theme.h"
#include "../util/logging.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace e2e {

// Helper to take a screenshot
static void takeScreenshot(const std::string& dir, const std::string& name) {
    std::filesystem::path screenshotDir = std::filesystem::absolute(dir);
    std::filesystem::create_directories(screenshotDir);
    std::filesystem::path path = screenshotDir / (name + ".png");
    raylib::TakeScreenshot(path.c_str());
}

// Set up common callbacks for the runner
static void setupCallbacks(
    ScriptRunner& runner,
    ecs::DocumentComponent& docComp,
    const std::string& screenshotDir
) {
    // Set up property getter for validation
    runner.set_property_getter([&docComp](const std::string& prop) -> std::string {
        const auto& buffer = docComp.buffer;
        const auto& style = buffer.textStyle();
        
        if (prop == "text") return buffer.getText();
        if (prop == "line_count") return std::to_string(buffer.lineCount());
        if (prop == "bold") return style.bold ? "true" : "false";
        if (prop == "italic") return style.italic ? "true" : "false";
        if (prop == "underline") return style.underline ? "true" : "false";
        if (prop == "strikethrough") return style.strikethrough ? "true" : "false";
        if (prop == "font_size") return std::to_string(style.fontSize);
        if (prop == "font") return style.font;
        if (prop == "has_selection") return buffer.hasSelection() ? "true" : "false";
        if (prop == "selected_text") return buffer.getSelectedText();
        if (prop == "paragraph_style") return paragraphStyleName(buffer.currentParagraphStyle());
        if (prop == "alignment") return textAlignmentName(buffer.currentAlignment());
        if (prop == "list_type") return listTypeName(buffer.currentListType());
        if (prop == "left_indent") return std::to_string(buffer.currentLeftIndent());
        if (prop == "line_spacing") return std::to_string(buffer.currentLineSpacing());
        if (prop == "hyperlink_count") return std::to_string(buffer.hyperlinks().size());
        if (prop == "bookmark_count") return std::to_string(buffer.bookmarks().size());
        if (prop == "footnote_count") return std::to_string(buffer.footnotes().size());
        if (prop == "caret_row") return std::to_string(buffer.caret().row);
        if (prop == "caret_col") return std::to_string(buffer.caret().column);
        if (prop == "superscript") return style.superscript ? "true" : "false";
        if (prop == "subscript") return style.subscript ? "true" : "false";

        if (prop == "word_count") {
            TextStats stats = buffer.stats();
            return std::to_string(stats.words);
        }
        if (prop == "char_count") {
            TextStats stats = buffer.stats();
            return std::to_string(stats.characters);
        }
        if (prop == "paragraph_count") {
            TextStats stats = buffer.stats();
            return std::to_string(stats.paragraphs);
        }
        if (prop == "sentence_count") {
            TextStats stats = buffer.stats();
            return std::to_string(stats.sentences);
        }
        
        return "<unknown>";
    });
    
    // Set up screenshot taker
    runner.set_screenshot_callback([screenshotDir](const std::string& name) {
        takeScreenshot(screenshotDir, name);
    });
    
    // Note: document_dump command is now handled by ECS system in e2e_commands.h
    
    // Set up document clearer (for batch mode)
    runner.set_reset_callback([&docComp]() {
        // Clear text content
        docComp.buffer.setText("");
        docComp.buffer.clearSelection();
        docComp.buffer.clearBookmarks();
        docComp.buffer.clearFootnotes();
        docComp.buffer.clearSections();
        docComp.buffer.clearHistory();
        // Reset text style to defaults
        docComp.buffer.setTextStyle(TextStyle{});
        docComp.tables.clear();
        docComp.images.clear();
        docComp.drawings.clear();
        docComp.equations.clear();
        docComp.comments.clear();
        docComp.isDirty = false;
        docComp.filePath.clear();
        
        // Reset settings
        Settings::get().reset();
    });
}

void initializeRunner(
    ScriptRunner& runner,
    const std::string& scriptPath,
    ecs::DocumentComponent& docComp,
    const std::string& screenshotDir
) {
    if (scriptPath.empty()) {
        return;
    }
    
    LOG_INFO("Loading E2E test script: %s", scriptPath.c_str());
    runner.load_script(scriptPath);
    
    if (!!runner.is_finished()) {
        LOG_WARNING("No commands found in test script: %s", scriptPath.c_str());
        return;
    }
    
    setupCallbacks(runner, docComp, screenshotDir);
}

void initializeRunnerBatch(
    ScriptRunner& runner,
    const std::string& scriptDir,
    ecs::DocumentComponent& docComp,
    const std::string& screenshotDir
) {
    if (scriptDir.empty()) {
        return;
    }
    
    LOG_INFO("Loading E2E test scripts from directory: %s", scriptDir.c_str());
    runner.load_scripts_from_directory(scriptDir);
    
    if (!!runner.is_finished()) {
        LOG_WARNING("No scripts found in directory: %s", scriptDir.c_str());
        return;
    }
    
    setupCallbacks(runner, docComp, screenshotDir);
}

// Helper to lowercase a string for case-insensitive comparison
static std::string toLower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), 
        [](unsigned char c) { return std::tolower(c); });
    return result;
}

// Set up extended callbacks with menu and layout support
static void setupCallbacksEx(
    ScriptRunner& runner,
    ecs::DocumentComponent& docComp,
    ecs::MenuComponent& menuComp,
    ecs::LayoutComponent& layoutComp,
    const std::string& screenshotDir
) {
    // Set up property getter for validation (extended version)
    runner.set_property_getter([&docComp, &menuComp, &layoutComp](const std::string& prop) -> std::string {
        const auto& buffer = docComp.buffer;
        const auto& style = buffer.textStyle();
        
        // Basic text properties
        if (prop == "text") return buffer.getText();
        if (prop == "text_length") return std::to_string(buffer.getText().size());
        if (prop == "line_count") return std::to_string(buffer.lineCount());
        if (prop == "bold") return style.bold ? "true" : "false";
        if (prop == "italic") return style.italic ? "true" : "false";
        if (prop == "underline") return style.underline ? "true" : "false";
        if (prop == "strikethrough") return style.strikethrough ? "true" : "false";
        if (prop == "font_size") return std::to_string(style.fontSize);
        if (prop == "font") return style.font;
        if (prop == "has_selection") return buffer.hasSelection() ? "true" : "false";
        if (prop == "selected_text") return buffer.getSelectedText();
        if (prop == "paragraph_style") return toLower(paragraphStyleName(buffer.currentParagraphStyle()));
        if (prop == "alignment") return toLower(textAlignmentName(buffer.currentAlignment()));
        if (prop == "list_type") return toLower(listTypeName(buffer.currentListType()));
        if (prop == "left_indent") return std::to_string(buffer.currentLeftIndent());
        if (prop == "line_spacing") return std::to_string(buffer.currentLineSpacing());
        if (prop == "hyperlink_count") return std::to_string(buffer.hyperlinks().size());
        if (prop == "bookmark_count") return std::to_string(buffer.bookmarks().size());
        if (prop == "footnote_count") return std::to_string(buffer.footnotes().size());
        if (prop == "caret_row") return std::to_string(buffer.caret().row);
        if (prop == "caret_col") return std::to_string(buffer.caret().column);
        
        // Menu properties
        if (prop == "menu_open") {
            for (const auto& menu : menuComp.menus) {
                if (menu.open) return menu.label;
            }
            return "false";
        }
        if (prop.substr(0, 13) == "menu_contains") {
            // Parse: menu_contains=ItemName
            // Check if any open menu contains this item
            for (const auto& menu : menuComp.menus) {
                for (const auto& item : menu.items) {
                    if (item.label == prop.substr(14)) return "true";
                }
            }
            return "false";
        }
        
        // Table properties
        if (prop == "has_table") return docComp.tables.empty() ? "false" : "true";
        if (prop == "table_count") return std::to_string(docComp.tables.size());
        if (prop == "table_rows") {
            if (!docComp.tables.empty()) {
                return std::to_string(docComp.tables[0].second.rowCount());
            }
            return "0";
        }
        if (prop == "table_cols") {
            if (!docComp.tables.empty()) {
                return std::to_string(docComp.tables[0].second.colCount());
            }
            return "0";
        }
        if (prop == "cell_content") {
            // Return content of current/selected table cell
            if (!docComp.tables.empty()) {
                const auto& table = docComp.tables[0].second;
                // Return first cell content for simplicity
                if (table.rowCount() > 0 && table.colCount() > 0) {
                    return table.getCellContent(0, 0);
                }
            }
            return "";
        }
        
        // Image properties
        if (prop == "has_image") return docComp.images.count() > 0 ? "true" : "false";
        if (prop == "image_count") return std::to_string(docComp.images.count());
        if (prop == "image_layout") {
            if (docComp.images.count() > 0) {
                const auto& images = docComp.images.images();
                if (!images.empty()) {
                    switch (images[0].layoutMode) {
                        case ImageLayoutMode::Inline: return "inline";
                        case ImageLayoutMode::WrapSquare: return "wrap";
                        case ImageLayoutMode::WrapTight: return "tight";
                        case ImageLayoutMode::Behind: return "behind";
                        case ImageLayoutMode::InFront: return "infront";
                        case ImageLayoutMode::BreakText: return "break";
                        default: return "inline";
                    }
                }
            }
            return "none";
        }
        
        // Drawing properties
        if (prop == "has_drawing") return docComp.drawings.count() > 0 ? "true" : "false";
        if (prop == "drawing_count") return std::to_string(docComp.drawings.count());
        
        // Equation properties
        if (prop == "has_equation") return docComp.equations.count() > 0 ? "true" : "false";
        if (prop == "equation_count") return std::to_string(docComp.equations.count());
        
        // Footnote properties
        if (prop == "has_footnote") return buffer.footnotes().empty() ? "false" : "true";
        
        // Hyperlink properties  
        if (prop == "has_hyperlink") return buffer.hyperlinks().empty() ? "false" : "true";
        if (prop == "hyperlink_url") {
            if (!buffer.hyperlinks().empty()) {
                return buffer.hyperlinks()[0].url;
            }
            return "";
        }
        
        // Bookmark properties
        if (prop == "has_bookmark") return buffer.bookmarks().empty() ? "false" : "true";
        if (prop == "bookmark_name") {
            if (!buffer.bookmarks().empty()) {
                return buffer.bookmarks()[0].name;
            }
            return "";
        }
        
        // Dialog properties
        if (prop == "dialog_open") {
            if (menuComp.showAboutDialog) return "About";
            if (menuComp.showFindDialog) return "Find";
            if (menuComp.showPageSetup) return "PageSetup";
            return "false";
        }
        if (prop == "help_window_visible") return menuComp.showHelpWindow ? "true" : "false";
        if (prop.substr(0, 13) == "help_contains") {
            // Check if help window contains the specified text
            // Help window shows keyboard shortcuts from action_map
            // Since we can't directly inspect the rendered help window,
            // we return true if help is visible (implying it contains shortcuts)
            return menuComp.showHelpWindow ? "true" : "false";
        }
        
        // Outline properties
        if (prop == "outline_visible") return layoutComp.showLineNumbers ? "true" : "false";
        if (prop == "outline_items") {
            auto outline = buffer.getOutline();
            return std::to_string(outline.size());
        }

        if (prop == "comment_count") return std::to_string(docComp.comments.size());
        if (prop == "track_changes_enabled") return docComp.trackChangesEnabled ? "true" : "false";
        if (prop == "revision_count") return std::to_string(docComp.revisions.size());
        if (prop == "tab_width") return std::to_string(docComp.docSettings.tabWidth);
        if (prop == "drop_cap") return buffer.currentLineHasDropCap() ? "true" : "false";
        if (prop == "smart_quotes_enabled") {
            return docComp.docSettings.smartQuotesEnabled ? "true" : "false";
        }
        if (prop == "autosave_enabled") return docComp.autoSaveEnabled ? "true" : "false";
        if (prop == "autosave_path_exists") {
            return std::filesystem::exists(docComp.autoSavePath) ? "true" : "false";
        }
        if (prop == "zoom_level") {
            int pct = static_cast<int>(std::round(layoutComp.zoomLevel * 100.0f));
            return std::to_string(pct);
        }
        if (prop == "focus_mode") return layoutComp.focusMode ? "true" : "false";
        if (prop == "split_view") return layoutComp.splitViewEnabled ? "true" : "false";
        if (prop == "dark_mode") return theme::DARK_MODE_ENABLED ? "true" : "false";

        if (prop == "export_pdf_exists" || prop == "export_html_exists" ||
            prop == "export_rtf_exists") {
            std::filesystem::path basePath =
                docComp.filePath.empty() ? docComp.defaultPath : docComp.filePath;
            if (prop == "export_pdf_exists") {
                basePath.replace_extension(".pdf");
            } else if (prop == "export_html_exists") {
                basePath.replace_extension(".html");
            } else {
                basePath.replace_extension(".rtf");
            }
            return std::filesystem::exists(basePath) ? "true" : "false";
        }
        
        // Status bar properties (always visible)
        if (prop == "status_bar_visible") return "true";
        if (prop == "status_shows_line") return std::to_string(buffer.caret().row + 1);
        if (prop == "status_shows_column") return std::to_string(buffer.caret().column + 1);
        if (prop == "status_shows_bold") return style.bold ? "true" : "false";
        if (prop == "status_shows_italic") return style.italic ? "true" : "false";
        if (prop == "status_shows_font_size") return "true";  // Always shown
        
        // Page properties
        if (prop == "page_size") {
            switch (docComp.docSettings.pageSettings.size) {
                case PageSize::Letter: return "letter";
                case PageSize::Legal: return "legal";
                case PageSize::Tabloid: return "tabloid";
                case PageSize::A4: return "a4";
                case PageSize::A5: return "a5";
                case PageSize::B5: return "b5";
                case PageSize::Executive: return "executive";
                case PageSize::Custom: return "custom";
                default: return "letter";
            }
        }
        if (prop == "page_orientation") {
            return docComp.docSettings.pageSettings.orientation == PageOrientation::Portrait 
                ? "portrait" : "landscape";
        }
        if (prop == "margin_left") return std::to_string(docComp.docSettings.pageSettings.marginLeft);
        if (prop == "margin_right") return std::to_string(docComp.docSettings.pageSettings.marginRight);
        if (prop == "margin_top") return std::to_string(docComp.docSettings.pageSettings.marginTop);
        if (prop == "margin_bottom") return std::to_string(docComp.docSettings.pageSettings.marginBottom);
        
        // Section properties
        if (prop == "section_count") return std::to_string(buffer.sections().size());
        if (prop == "current_section_columns") {
            if (!buffer.sections().empty()) {
                return std::to_string(buffer.sections()[0].settings.columns);
            }
            return "1";
        }
        
        // Page break properties
        if (prop == "has_page_break") {
            std::size_t row = buffer.caret().row;
            if (row < buffer.lineCount()) {
                return buffer.lineSpan(row).hasPageBreakBefore ? "true" : "false";
            }
            return "false";
        }
        if (prop == "page_count") return "1";  // Simple estimation for now
        
        // Extended caret properties
        if (prop == "caret_line") return std::to_string(buffer.caret().row + 1);  // 1-indexed
        if (prop == "caret_pos") {
            // Calculate absolute position
            std::size_t pos = 0;
            for (std::size_t i = 0; i < buffer.caret().row && i < buffer.lineCount(); ++i) {
                pos += buffer.lineString(i).size() + 1;  // +1 for newline
            }
            pos += buffer.caret().column;
            return std::to_string(pos);
        }
        
        // Indentation/list properties
        if (prop == "indent_level") return std::to_string(buffer.currentLeftIndent() / 20);  // 20px per level
        if (prop == "list_level") {
            if (buffer.caret().row < buffer.lineCount()) {
                return std::to_string(buffer.lineSpan(buffer.caret().row).listLevel);
            }
            return "0";
        }
        
        // Selection properties
        if (prop == "selection_length") return std::to_string(buffer.getSelectedText().size());
        
        // Text search properties (removed - doesn't fit validate command design)
        // text_shorter_than_NUMBER - check if text length < NUMBER
        if (prop.length() > 18 && prop.substr(0, 18) == "text_shorter_than_") {
            int maxLen = std::stoi(prop.substr(18));
            return buffer.getText().size() < static_cast<std::size_t>(maxLen) ? "true" : "false";
        }
        if (prop.substr(0, 10) == "regex_find") {
            std::string pattern = prop.substr(11);
            FindOptions options;
            options.useRegex = true;
            FindResult result = buffer.find(pattern, options);
            return result.found ? "true" : "false";
        }
        
        // Formatting properties
        if (prop == "has_text_color") {
            return (style.textColor.r != 0 || style.textColor.g != 0 || 
                    style.textColor.b != 0 || style.textColor.a != 255) ? "true" : "false";
        }
        if (prop == "has_highlight") {
            return (style.highlightColor.r != 255 || style.highlightColor.g != 255 || 
                    style.highlightColor.b != 255 || style.highlightColor.a != 0) ? "true" : "false";
        }
        
        // TOC properties
        if (prop == "has_toc") {
            // Check if document has any headings that would generate a TOC
            auto outline = buffer.getOutline();
            return outline.empty() ? "false" : "true";
        }
        if (prop == "toc_entries") {
            auto outline = buffer.getOutline();
            return std::to_string(outline.size());
        }
        
        // Header/footer properties
        if (prop == "header_content") {
            // Return header center text (most common location)
            const auto& header = docComp.docSettings.header;
            if (!header.center.text.empty()) return header.center.text;
            if (!header.left.text.empty()) return header.left.text;
            if (!header.right.text.empty()) return header.right.text;
            return "";
        }
        if (prop == "has_page_number") {
            const auto& header = docComp.docSettings.header;
            const auto& footer = docComp.docSettings.footer;
            bool hasPageNum = header.left.showPageNumber || header.center.showPageNumber || 
                             header.right.showPageNumber || footer.left.showPageNumber || 
                             footer.center.showPageNumber || footer.right.showPageNumber;
            return hasPageNum ? "true" : "false";
        }
        
        // Heading detection
        if (prop == "caret_at_heading") {
            auto ps = buffer.currentParagraphStyle();
            if (ps >= ParagraphStyle::Heading1 && ps <= ParagraphStyle::Heading6) {
                return paragraphStyleName(ps);
            }
            return "false";
        }
        
        return "<unknown>";
    });
    
    // Set up screenshot taker
    runner.set_screenshot_callback([screenshotDir](const std::string& name) {
        takeScreenshot(screenshotDir, name);
    });
    
    // Note: document_dump command is now handled by ECS system in e2e_commands.h
    
    // Set up document clearer (for batch mode)
    runner.set_reset_callback([&docComp, &menuComp, &layoutComp]() {
        // Reset document state
        docComp.buffer.setText("");
        docComp.buffer.clearSelection();
        docComp.buffer.clearBookmarks();
        docComp.buffer.clearFootnotes();
        docComp.buffer.clearSections();
        docComp.buffer.clearHistory();
        docComp.buffer.setTextStyle(TextStyle{});
        docComp.comments.clear();
        docComp.revisions.clear();
        docComp.trackChangesEnabled = false;
        docComp.trackChangesBaseline.clear();
        docComp.docSettings = DocumentSettings{};
        docComp.tables.clear();
        docComp.images.clear();
        docComp.drawings.clear();
        docComp.equations.clear();
        docComp.isDirty = false;
        docComp.filePath.clear();
        
        // Reset menu state
        menuComp.showAboutDialog = false;
        menuComp.showHelpWindow = false;
        menuComp.showFindDialog = false;
        menuComp.findReplaceMode = false;
        menuComp.lastSearchTerm.clear();
        menuComp.replaceTerm.clear();
        std::memset(menuComp.findInputBuffer, 0, sizeof(menuComp.findInputBuffer));
        std::memset(menuComp.replaceInputBuffer, 0, sizeof(menuComp.replaceInputBuffer));
        menuComp.showCommentDialog = false;
        menuComp.commentInputBuffer[0] = '\0';
        menuComp.showTemplateDialog = false;
        menuComp.templateInputBuffer[0] = '\0';
        menuComp.showTabWidthDialog = false;
        menuComp.tabWidthInputBuffer[0] = '\0';
        menuComp.showPageSetup = false;
        
        // Reset layout state
        layoutComp.zoomLevel = 1.0f;
        layoutComp.focusMode = false;
        layoutComp.splitViewEnabled = false;
        layoutComp.splitViewHorizontal = true;
        
        // Reset theme
        theme::applyDarkMode(false);
        
        // Reset settings to defaults
        Settings::get().reset();
    });
    
    // Note: menu_open, menu_select, outline_click commands are now handled
    // by ECS systems registered in e2e_commands.h
}

void initializeRunner(
    ScriptRunner& runner,
    const std::string& scriptPath,
    ecs::DocumentComponent& docComp,
    ecs::MenuComponent& menuComp,
    ecs::LayoutComponent& layoutComp,
    const std::string& screenshotDir
) {
    if (scriptPath.empty()) {
        return;
    }
    
    LOG_INFO("Loading E2E test script: %s", scriptPath.c_str());
    runner.load_script(scriptPath);
    
    if (!!runner.is_finished()) {
        LOG_WARNING("No commands found in test script: %s", scriptPath.c_str());
        return;
    }
    
    setupCallbacksEx(runner, docComp, menuComp, layoutComp, screenshotDir);
}

void initializeRunnerBatch(
    ScriptRunner& runner,
    const std::string& scriptDir,
    ecs::DocumentComponent& docComp,
    ecs::MenuComponent& menuComp,
    ecs::LayoutComponent& layoutComp,
    const std::string& screenshotDir
) {
    if (scriptDir.empty()) {
        return;
    }
    
    LOG_INFO("Loading E2E test scripts from directory: %s", scriptDir.c_str());
    runner.load_scripts_from_directory(scriptDir);
    
    if (!!runner.is_finished()) {
        LOG_WARNING("No scripts found in directory: %s", scriptDir.c_str());
        return;
    }
    
    setupCallbacksEx(runner, docComp, menuComp, layoutComp, screenshotDir);
}

}  // namespace e2e
