// E2E Runner initialization - app-specific callbacks for wordproc
// Uses afterhours E2ERunner with ECS-based command handlers

#include "e2e_runner.h"
#include "../editor/document_settings.h"
#include "../input_mapping.h"
#include "../rl.h"
#include "../../vendor/afterhours/src/plugins/modal.h"
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
    afterhours::graphics::take_screenshot(path.c_str());
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
        if (prop == "file_path") return docComp.filePath;
        if (prop == "is_dirty") return docComp.isDirty ? "true" : "false";
        
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

// Set up extended callbacks with menu, layout, and toolbar support
static void setupCallbacksEx(
    ScriptRunner& runner,
    ecs::DocumentComponent& docComp,
    ecs::MenuComponent& menuComp,
    ecs::DialogState& dialogState,
    ecs::LayoutComponent& layoutComp,
    ecs::ToolbarComponent& toolbarComp,
    const std::string& screenshotDir
) {
    // Set up property getter for validation (extended version)
    runner.set_property_getter([&docComp, &menuComp, &dialogState, &layoutComp, &toolbarComp](const std::string& prop) -> std::string {
        (void)toolbarComp; // Available if needed for future property queries
        const auto& buffer = docComp.buffer;
        const auto& style = buffer.textStyle();
        
        // Basic text properties
        if (prop == "text") return buffer.getText();
        if (prop == "text_length") return std::to_string(buffer.textSize());
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
        if (prop == "superscript") return style.superscript ? "true" : "false";
        if (prop == "subscript") return style.subscript ? "true" : "false";
        
        // Text statistics
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
        
        // Text content search: validate text_contains_NEEDLE=true
        if (prop.length() > 14 && prop.substr(0, 14) == "text_contains_") {
            std::string needle = prop.substr(14);
            return buffer.getText().find(needle) != std::string::npos ? "true" : "false";
        }
        
        // Menu properties
        if (prop == "menu_open") {
            for (const auto& menu : menuComp.menus) {
                if (menu.open) return menu.label;
            }
            return "false";
        }
        // menu_item_ITEMNAME=true/false — check if any menu contains an item
        if (prop.length() > 10 && prop.substr(0, 10) == "menu_item_") {
            std::string itemName = prop.substr(10);
            for (const auto& menu : menuComp.menus) {
                for (const auto& item : menu.items) {
                    if (item.label == itemName) return "true";
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
            if (dialogState.showAboutDialog) return "About";
            if (dialogState.showFindDialog) return "Find";
            if (dialogState.showPageSetup) return "PageSetup";
            if (dialogState.showSettingsDialog) return "Settings";
            if (dialogState.showWordCountDialog) return "WordCount";
            return "false";
        }
        if (prop == "help_window_visible") return dialogState.showHelpWindow ? "true" : "false";
        // help_has_TEXT=true/false — check if help window is visible
        // (content check not possible, so we just verify visibility)
        if (prop.length() > 9 && prop.substr(0, 9) == "help_has_") {
            return dialogState.showHelpWindow ? "true" : "false";
        }
        
        // Outline properties
        if (prop == "outline_visible") return layoutComp.showOutline ? "true" : "false";
        if (prop == "line_numbers_visible") return layoutComp.showLineNumbers ? "true" : "false";
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
        if (prop == "file_path") return docComp.filePath;
        if (prop == "is_dirty") return docComp.isDirty ? "true" : "false";
        if (prop == "autosave_enabled") return docComp.autoSaveEnabled ? "true" : "false";
        if (prop == "autosave_path_exists") {
            return std::filesystem::exists(docComp.autoSavePath) ? "true" : "false";
        }
        // file_exists_PATH — check if a file exists at the given path
        if (prop.length() > 12 && prop.substr(0, 12) == "file_exists_") {
            std::string path = prop.substr(12);
            return std::filesystem::exists(path) ? "true" : "false";
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
        // caret_pos_greater_than_N: returns "true" if absolute caret pos > N
        if (prop.length() > 23 && prop.substr(0, 23) == "caret_pos_greater_than_") {
            int threshold = std::stoi(prop.substr(23));
            std::size_t pos = 0;
            for (std::size_t i = 0; i < buffer.caret().row && i < buffer.lineCount(); ++i) {
                pos += buffer.lineSpan(i).length + 1;
            }
            pos += buffer.caret().column;
            return static_cast<int>(pos) > threshold ? "true" : "false";
        }
        if (prop == "caret_pos") {
            // Calculate absolute position
            std::size_t pos = 0;
            for (std::size_t i = 0; i < buffer.caret().row && i < buffer.lineCount(); ++i) {
                pos += buffer.lineSpan(i).length + 1;  // +1 for newline
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
            return buffer.textSize() < static_cast<std::size_t>(maxLen) ? "true" : "false";
        }
        // regex_match_PATTERN=true/false — check if regex pattern matches buffer
        if (prop.length() > 12 && prop.substr(0, 12) == "regex_match_") {
            std::string pattern = prop.substr(12);
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
        
        // Window / viewport dimensions
        if (prop == "window_width") {
            auto* pcr = afterhours::EntityHelper::get_singleton_cmp<
                afterhours::window_manager::ProvidesCurrentResolution>();
            return pcr ? std::to_string(pcr->current_resolution.width) : "0";
        }
        if (prop == "window_height") {
            auto* pcr = afterhours::EntityHelper::get_singleton_cmp<
                afterhours::window_manager::ProvidesCurrentResolution>();
            return pcr ? std::to_string(pcr->current_resolution.height) : "0";
        }
        
        // Menu bar accessibility: check all menu headers fit within window width
        if (prop == "menu_bar_fits") {
            auto* pcr = afterhours::EntityHelper::get_singleton_cmp<
                afterhours::window_manager::ProvidesCurrentResolution>();
            if (!pcr) return "false";
            float totalWidth = 0;
            int menuFontSize = 16;
            for (const auto& m : menuComp.menus) {
                totalWidth += static_cast<float>(
                    theme::MeasureUIText(m.label.c_str(), menuFontSize) + theme::layout::scaleInt(16));
            }
            return totalWidth <= static_cast<float>(pcr->current_resolution.width) ? "true" : "false";
        }
        
        // Menu bar total width in pixels
        if (prop == "menu_bar_width") {
            float totalWidth = 0;
            int menuFontSize = 16;
            for (const auto& m : menuComp.menus) {
                totalWidth += static_cast<float>(
                    theme::MeasureUIText(m.label.c_str(), menuFontSize) + theme::layout::scaleInt(16));
            }
            return std::to_string(static_cast<int>(totalWidth));
        }
        
        // menu_header_onscreen_MENUNAME — true if that header's right edge fits in viewport
        if (prop.length() > 21 && prop.substr(0, 21) == "menu_header_onscreen_") {
            std::string menuName = prop.substr(21);
            auto* pcr = afterhours::EntityHelper::get_singleton_cmp<
                afterhours::window_manager::ProvidesCurrentResolution>();
            if (!pcr) return "false";
            int winW = pcr->current_resolution.width;
            float x = 0;
            int menuFontSize = 16;
            for (const auto& m : menuComp.menus) {
                float bw = static_cast<float>(
                    theme::MeasureUIText(m.label.c_str(), menuFontSize) + theme::layout::scaleInt(16));
                x += bw;
                if (m.label == menuName) {
                    // Right edge of this menu header must be within viewport
                    return x <= static_cast<float>(winW) ? "true" : "false";
                }
            }
            return "false"; // Menu not found
        }
        
        // toolbar_fits — check if all toolbar buttons fit within window width
        if (prop == "toolbar_fits") {
            auto* pcr = afterhours::EntityHelper::get_singleton_cmp<
                afterhours::window_manager::ProvidesCurrentResolution>();
            if (!pcr) return "false";
            int winW = pcr->current_resolution.width;
            // Calculate toolbar width: buttons + separators + formatting bar dropdowns
            int buttonCount = 9;   // New, Open, Save, Print, Cut, Copy, Paste, Undo, Redo
            int separatorCount = 3; // Between groups
            float toolbarWidth = static_cast<float>(
                buttonCount * theme::layout::scaleInt(theme::layout::TOOLBAR_BUTTON_SIZE + 
                    theme::layout::TOOLBAR_BUTTON_PADDING * 2) +
                separatorCount * theme::layout::scaleInt(theme::layout::TOOLBAR_SEPARATOR_WIDTH + 8));
            return toolbarWidth <= static_cast<float>(winW) ? "true" : "false";
        }
        
        // formatting_bar_fits — check if style/font/size dropdowns fit in window width
        if (prop == "formatting_bar_fits") {
            auto* pcr = afterhours::EntityHelper::get_singleton_cmp<
                afterhours::window_manager::ProvidesCurrentResolution>();
            if (!pcr) return "false";
            int winW = pcr->current_resolution.width;
            // Approximate: style dropdown (110px) + font dropdown (180px) + size dropdown (50px) + buttons (~200px)
            float fmtWidth = theme::layout::scale(110.0f + 180.0f + 50.0f + 200.0f);
            return fmtWidth <= static_cast<float>(winW) ? "true" : "false";
        }
        
        // Editing area height: window height minus chrome (title bar + menu bar + toolbar + formatting bar + status bar)
        if (prop == "editing_area_height") {
            auto* pcr = afterhours::EntityHelper::get_singleton_cmp<
                afterhours::window_manager::ProvidesCurrentResolution>();
            if (!pcr) return "0";
            float chrome = theme::layout::scale(
                static_cast<float>(theme::layout::TITLE_BAR_HEIGHT +
                                   theme::layout::MENU_BAR_HEIGHT +
                                   theme::layout::TOOLBAR_HEIGHT +
                                   theme::layout::FORMATTING_BAR_HEIGHT +
                                   theme::layout::STATUS_BAR_HEIGHT));
            int editHeight = pcr->current_resolution.height - static_cast<int>(chrome);
            return std::to_string(editHeight < 0 ? 0 : editHeight);
        }
        // editing_area_gt_N: returns "true" if editing area height > N pixels
        if (prop.length() > 16 && prop.substr(0, 16) == "editing_area_gt_") {
            int threshold = std::stoi(prop.substr(16));
            auto* pcr = afterhours::EntityHelper::get_singleton_cmp<
                afterhours::window_manager::ProvidesCurrentResolution>();
            if (!pcr) return "false";
            float chrome = theme::layout::scale(
                static_cast<float>(theme::layout::TITLE_BAR_HEIGHT +
                                   theme::layout::MENU_BAR_HEIGHT +
                                   theme::layout::TOOLBAR_HEIGHT +
                                   theme::layout::FORMATTING_BAR_HEIGHT +
                                   theme::layout::STATUS_BAR_HEIGHT));
            int editHeight = pcr->current_resolution.height - static_cast<int>(chrome);
            return editHeight > threshold ? "true" : "false";
        }
        
        // Check if dropdown for a given menu would be fully on-screen vertically
        // menu_dropdown_fits_MENUNAME=true/false
        if (prop.length() > 20 && prop.substr(0, 20) == "menu_dropdown_fits_") {
            std::string menuName = prop.substr(20);
            auto* pcr = afterhours::EntityHelper::get_singleton_cmp<
                afterhours::window_manager::ProvidesCurrentResolution>();
            if (!pcr) return "false";
            int winH = pcr->current_resolution.height;
            float menuBarBottom = theme::layout::scale(
                static_cast<float>(theme::layout::TITLE_BAR_HEIGHT + theme::layout::MENU_BAR_HEIGHT));
            for (const auto& m : menuComp.menus) {
                if (m.label == menuName) {
                    float dropdownHeight = 0;
                    for (const auto& item : m.items) {
                        dropdownHeight += item.separator ? theme::layout::scale(8.0f) : theme::layout::scale(20.0f);
                    }
                    return (menuBarBottom + dropdownHeight) <= static_cast<float>(winH) ? "true" : "false";
                }
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
    runner.set_reset_callback([&docComp, &menuComp, &dialogState, &layoutComp, &toolbarComp]() {
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
        dialogState.showAboutDialog = false;
        dialogState.showHelpWindow = false;
        dialogState.showFindDialog = false;
        dialogState.findReplaceMode = false;
        dialogState.lastSearchTerm.clear();
        dialogState.replaceTerm.clear();
        std::memset(dialogState.findInputBuffer, 0, sizeof(dialogState.findInputBuffer));
        std::memset(dialogState.replaceInputBuffer, 0, sizeof(dialogState.replaceInputBuffer));
        dialogState.findInputStr.clear();
        dialogState.replaceInputStr.clear();
        dialogState.showCommentDialog = false;
        dialogState.commentInputBuffer[0] = '\0';
        dialogState.commentInputStr.clear();
        dialogState.showTemplateDialog = false;
        dialogState.templateInputBuffer[0] = '\0';
        dialogState.templateInputStr.clear();
        dialogState.showTabWidthDialog = false;
        dialogState.tabWidthInputBuffer[0] = '\0';
        dialogState.tabWidthInputStr.clear();
        dialogState.showPageSetup = false;
        dialogState.showSaveAsDialog = false;
        std::memset(dialogState.saveAsInputBuffer, 0, sizeof(dialogState.saveAsInputBuffer));
        dialogState.saveAsInputStr.clear();
        dialogState.showBookmarkListDialog = false;
        dialogState.showSettingsDialog = false;
        dialogState.uiScaleInputStr.clear();
        dialogState.showWordCountDialog = false;
        
        // Close any open dropdown menus
        menuComp.activeMenuIndex = -1;
        menuComp.lastClickedResult = -1;
        for (auto& menu : menuComp.menus) {
            menu.open = false;
        }
        
        // Reset layout state
        layoutComp.zoomLevel = 1.0f;
        layoutComp.focusMode = false;
        layoutComp.splitViewEnabled = false;
        layoutComp.splitViewHorizontal = true;
        
        // Reset toolbar dropdown state
        toolbarComp.styleDropdownOpen = false;
        toolbarComp.fontDropdownOpen = false;
        toolbarComp.fontSizeDropdownOpen = false;
        
        // Reset theme
        theme::applyDarkMode(false);
        
        // Reset afterhours UI context (focus, active element, etc.)
        auto* uiCtx = afterhours::EntityHelper::get_singleton_cmp<
            afterhours::ui::UIContext<InputAction>>();
        if (uiCtx) {
            uiCtx->reset();
            uiCtx->last_action = InputAction::None;
            uiCtx->last_action_modifiers = 0;
            uiCtx->all_actions.reset();
        }
        
        // Clear modal stack so stale modals don't block input in subsequent tests
        auto* modalRoot = afterhours::EntityHelper::get_singleton_cmp<
            afterhours::modal::ModalRoot>();
        if (modalRoot) {
            modalRoot->modal_stack.clear();
        }
        
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
    ecs::DialogState& dialogState,
    ecs::LayoutComponent& layoutComp,
    ecs::ToolbarComponent& toolbarComp,
    const std::string& screenshotDir
) {
    if (scriptPath.empty()) {
        return;
    }
    
    LOG_INFO("Loading E2E test script: %s", scriptPath.c_str());
    runner.load_script(scriptPath);
    
    fprintf(stderr, "[E2E DEBUG] Script loaded. is_finished=%d, hasCommands=%d\n", 
            runner.is_finished(), runner.hasCommands());
    fflush(stderr);
    
    if (!!runner.is_finished()) {
        LOG_WARNING("No commands found in test script: %s", scriptPath.c_str());
        fprintf(stderr, "[E2E DEBUG] Runner is finished immediately after loading\n");
        fflush(stderr);
        return;
    }
    
    fprintf(stderr, "[E2E DEBUG] Setting up callbacks for script\n");
    fflush(stderr);
    setupCallbacksEx(runner, docComp, menuComp, dialogState, layoutComp, toolbarComp, screenshotDir);
    fprintf(stderr, "[E2E DEBUG] Callbacks setup complete\n");
    fflush(stderr);
}

void initializeRunnerBatch(
    ScriptRunner& runner,
    const std::string& scriptDir,
    ecs::DocumentComponent& docComp,
    ecs::MenuComponent& menuComp,
    ecs::DialogState& dialogState,
    ecs::LayoutComponent& layoutComp,
    ecs::ToolbarComponent& toolbarComp,
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
    
    setupCallbacksEx(runner, docComp, menuComp, dialogState, layoutComp, toolbarComp, screenshotDir);
}

}  // namespace e2e
