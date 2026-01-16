// Wrapper that uses the extracted e2e_testing.h implementation
// This file maintains backward compatibility with existing code.

#pragma once

#include "../extracted/e2e_testing.h"
#include "../rl.h"
#include "input_injector.h"
#include "test_input.h"
#include "visible_text_registry.h"

// Key constants for backward compatibility
namespace e2e_keys {
    constexpr int CTRL = 341;   // KEY_LEFT_CONTROL
    constexpr int SHIFT = 340;  // KEY_LEFT_SHIFT
    constexpr int ALT = 342;    // KEY_LEFT_ALT
}

namespace e2e {

// Re-export types from extracted
using CommandType = afterhours::testing::CommandType;
using TestCommand = afterhours::testing::TestCommand;
using ValidationResult = afterhours::testing::ValidationResult;
using ScriptError = afterhours::testing::ScriptError;
using KeyCombo = afterhours::testing::KeyCombo;

// Re-export functions
inline KeyCombo parseKeyCombo(const std::string& keyStr) {
    return afterhours::testing::parse_key_combo(keyStr);
}

inline std::vector<TestCommand> parseScript(const std::string& path) {
    return afterhours::testing::parse_script(path);
}

// ScriptRunner is an alias to E2ERunner with some compatibility methods
class ScriptRunner : public afterhours::testing::E2ERunner {
public:
    using PropertyGetter = std::function<std::string(const std::string&)>;
    using ScreenshotTaker = std::function<void(const std::string&)>;
    using DocumentDumper = std::function<void(const std::string&)>;
    using DocumentClearer = std::function<void()>;
    using MenuOpener = std::function<bool(const std::string&)>;
    using MenuItemSelector = std::function<bool(const std::string&)>;
    using OutlineClicker = std::function<bool(const std::string&)>;
    
    void loadScript(const std::string& path) { load_script(path); }
    void loadScriptsFromDirectory(const std::string& dir) { load_scripts_from_directory(dir); }
    
    void setTimeoutFrames(int frames) { set_timeout_frames(frames); }
    void setPropertyGetter(PropertyGetter fn) { set_property_getter(fn); }
    void setScreenshotTaker(ScreenshotTaker fn) { set_screenshot_callback(fn); }
    void setDocumentDumper(DocumentDumper fn) { document_dumper_ = fn; }
    void setDocumentClearer(DocumentClearer fn) { set_clear_callback(fn); }
    void setMenuOpener(MenuOpener fn) { set_menu_opener(fn); }
    void setMenuItemSelector(MenuItemSelector fn) { set_menu_selector(fn); }
    void setOutlineClicker(OutlineClicker fn) { outline_clicker_ = fn; }
    
    bool hasCommands() const { return !is_finished(); }
    bool isFinished() const { return is_finished(); }
    bool hasFailed() const { return has_failed(); }
    bool hasTimedOut() const { return has_timed_out(); }
    int frameCount() const { return frame_count(); }
    
    std::string getCurrentCommandDescription() const { return current_command_desc(); }
    int getRemainingTimeoutFrames() const { return -1; /* Not tracked in extracted */ }
    float getRemainingTimeoutSeconds() const { return -1.0f; }
    
    bool showDebugOverlay() const { return debug_overlay_; }
    void setDebugOverlay(bool show) { debug_overlay_ = show; }
    
    void printResults() const { E2ERunner::print_results(); }
    
private:
    bool debug_overlay_ = false;
    DocumentDumper document_dumper_;
    OutlineClicker outline_clicker_;
};

// Forward declare DocumentAccessor for compatibility
class DocumentAccessor;

}  // namespace e2e
