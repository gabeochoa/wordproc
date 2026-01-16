#include "e2e_runner.h"

#include <filesystem>
#include <fstream>

#include "../editor/document_settings.h"
#include "../rl.h"
#include "../util/logging.h"

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
    runner.setPropertyGetter([&docComp](const std::string& prop) -> std::string {
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
        
        return "<unknown>";
    });
    
    // Set up screenshot taker
    runner.setScreenshotTaker([screenshotDir](const std::string& name) {
        takeScreenshot(screenshotDir, name);
    });
    
    // Set up document dumper
    runner.setDocumentDumper([&docComp](const std::string& path) {
        std::ofstream file(path);
        if (file.is_open()) {
            file << docComp.buffer.getText();
        }
    });
    
    // Set up document clearer (for batch mode)
    runner.setDocumentClearer([&docComp]() {
        // Clear text content
        docComp.buffer.setText("");
        docComp.buffer.clearSelection();
        docComp.buffer.clearBookmarks();
        docComp.buffer.clearFootnotes();
        docComp.buffer.clearSections();
        docComp.buffer.clearHistory();
        // Reset text style to defaults
        docComp.buffer.setTextStyle(TextStyle{});
        docComp.isDirty = false;
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
    runner.loadScript(scriptPath);
    
    if (!runner.hasCommands()) {
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
    runner.loadScriptsFromDirectory(scriptDir);
    
    if (!runner.hasCommands()) {
        LOG_WARNING("No scripts found in directory: %s", scriptDir.c_str());
        return;
    }
    
    setupCallbacks(runner, docComp, screenshotDir);
}

}  // namespace e2e
