#pragma once

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

#include "../rl.h"
#include "input_injector.h"
#include "test_input.h"
#include "visible_text_registry.h"

// Key constants (avoiding issues with raylib namespace in headers)
namespace e2e_keys {
    constexpr int CTRL = 341;   // KEY_LEFT_CONTROL
    constexpr int SHIFT = 340;  // KEY_LEFT_SHIFT
    constexpr int ALT = 342;    // KEY_LEFT_ALT
}

namespace e2e {

// Forward declare the document accessor
class DocumentAccessor;

// Test command types
enum class CommandType {
    Type,          // type "text" - types text into document
    Key,           // key CTRL+B - presses a key combination
    SelectAll,     // select_all - selects all text
    Click,         // click x y - clicks at coordinates
    DoubleClick,   // double_click x y - double-clicks at coordinates
    Drag,          // drag x1 y1 x2 y2 - drags from (x1,y1) to (x2,y2)
    MouseMove,     // mouse_move x y - moves mouse to coordinates
    Wait,          // wait 5 - waits N frames
    Validate,      // validate property=value - validates document state
    ExpectText,    // expect_text "text" - validates text is visible on screen
    DumpDocument,  // dump_document path.txt - dumps document to file
    Screenshot,    // screenshot name - takes a screenshot
    Clear,         // clear - clears document and resets state
    Comment,       // # comment - ignored line
    MenuOpen,      // menu_open "File" - opens a menu by name
    MenuSelect,    // menu_select "Save" - selects item from open menu
    ClickOutline,  // click_outline "Heading 1" - clicks an outline entry
    Unknown
};

// A single test command
struct TestCommand {
    CommandType type = CommandType::Unknown;
    std::string arg1;
    std::string arg2;
    int intArg = 0;
    int intArg2 = 0;   // For second coordinate (y or x2)
    int intArg3 = 0;   // For third coordinate (y2 in drag)
    int intArg4 = 0;   // For fourth coordinate (unused currently)
    int lineNumber = 0;  // For error reporting
};

// Validation result
struct ValidationResult {
    bool success = true;
    std::string property;
    std::string expected;
    std::string actual;
    std::string message;
    int lineNumber = 0;  // Line in script file for error reporting
};

// Error result for script errors
struct ScriptError {
    int lineNumber = 0;
    std::string command;
    std::string message;
};

// Parse a key string like "CTRL+B" into modifiers and key
struct KeyCombo {
    bool ctrl = false;
    bool shift = false;
    bool alt = false;
    int key = 0;
};

inline KeyCombo parseKeyCombo(const std::string& keyStr) {
    KeyCombo combo;
    std::string remaining = keyStr;
    
    // Check for modifiers
    auto hasPrefix = [&](const std::string& prefix) {
        if (remaining.find(prefix) == 0) {
            remaining = remaining.substr(prefix.length());
            return true;
        }
        return false;
    };
    
    while (true) {
        if (hasPrefix("CTRL+") || hasPrefix("CMD+")) {
            combo.ctrl = true;
        } else if (hasPrefix("SHIFT+")) {
            combo.shift = true;
        } else if (hasPrefix("ALT+")) {
            combo.alt = true;
        } else {
            break;
        }
    }
    
    // Parse the key itself
    if (remaining == "A") combo.key = raylib::KEY_A;
    else if (remaining == "B") combo.key = raylib::KEY_B;
    else if (remaining == "C") combo.key = raylib::KEY_C;
    else if (remaining == "D") combo.key = raylib::KEY_D;
    else if (remaining == "E") combo.key = raylib::KEY_E;
    else if (remaining == "F") combo.key = raylib::KEY_F;
    else if (remaining == "G") combo.key = raylib::KEY_G;
    else if (remaining == "H") combo.key = raylib::KEY_H;
    else if (remaining == "I") combo.key = raylib::KEY_I;
    else if (remaining == "J") combo.key = raylib::KEY_J;
    else if (remaining == "K") combo.key = raylib::KEY_K;
    else if (remaining == "L") combo.key = raylib::KEY_L;
    else if (remaining == "M") combo.key = raylib::KEY_M;
    else if (remaining == "N") combo.key = raylib::KEY_N;
    else if (remaining == "O") combo.key = raylib::KEY_O;
    else if (remaining == "P") combo.key = raylib::KEY_P;
    else if (remaining == "Q") combo.key = raylib::KEY_Q;
    else if (remaining == "R") combo.key = raylib::KEY_R;
    else if (remaining == "S") combo.key = raylib::KEY_S;
    else if (remaining == "T") combo.key = raylib::KEY_T;
    else if (remaining == "U") combo.key = raylib::KEY_U;
    else if (remaining == "V") combo.key = raylib::KEY_V;
    else if (remaining == "W") combo.key = raylib::KEY_W;
    else if (remaining == "X") combo.key = raylib::KEY_X;
    else if (remaining == "Y") combo.key = raylib::KEY_Y;
    else if (remaining == "Z") combo.key = raylib::KEY_Z;
    else if (remaining == "0") combo.key = raylib::KEY_ZERO;
    else if (remaining == "1") combo.key = raylib::KEY_ONE;
    else if (remaining == "2") combo.key = raylib::KEY_TWO;
    else if (remaining == "3") combo.key = raylib::KEY_THREE;
    else if (remaining == "4") combo.key = raylib::KEY_FOUR;
    else if (remaining == "5") combo.key = raylib::KEY_FIVE;
    else if (remaining == "6") combo.key = raylib::KEY_SIX;
    else if (remaining == "7") combo.key = raylib::KEY_SEVEN;
    else if (remaining == "8") combo.key = raylib::KEY_EIGHT;
    else if (remaining == "9") combo.key = raylib::KEY_NINE;
    else if (remaining == "ENTER") combo.key = raylib::KEY_ENTER;
    else if (remaining == "ESCAPE" || remaining == "ESC") combo.key = raylib::KEY_ESCAPE;
    else if (remaining == "TAB") combo.key = raylib::KEY_TAB;
    else if (remaining == "BACKSPACE") combo.key = raylib::KEY_BACKSPACE;
    else if (remaining == "DELETE") combo.key = raylib::KEY_DELETE;
    else if (remaining == "LEFT") combo.key = raylib::KEY_LEFT;
    else if (remaining == "RIGHT") combo.key = raylib::KEY_RIGHT;
    else if (remaining == "UP") combo.key = raylib::KEY_UP;
    else if (remaining == "DOWN") combo.key = raylib::KEY_DOWN;
    else if (remaining == "HOME") combo.key = raylib::KEY_HOME;
    else if (remaining == "END") combo.key = raylib::KEY_END;
    else if (remaining == "PAGEUP") combo.key = raylib::KEY_PAGE_UP;
    else if (remaining == "PAGEDOWN") combo.key = raylib::KEY_PAGE_DOWN;
    else if (remaining == "PLUS" || remaining == "=") combo.key = raylib::KEY_EQUAL;
    else if (remaining == "MINUS" || remaining == "-") combo.key = raylib::KEY_MINUS;
    else if (remaining == "LEFTBRACKET" || remaining == "[") combo.key = raylib::KEY_LEFT_BRACKET;
    else if (remaining == "RIGHTBRACKET" || remaining == "]") combo.key = raylib::KEY_RIGHT_BRACKET;
    else if (remaining == "F1") combo.key = raylib::KEY_F1;
    else if (remaining == "F2") combo.key = raylib::KEY_F2;
    else if (remaining == "F3") combo.key = raylib::KEY_F3;
    else if (remaining == "F4") combo.key = raylib::KEY_F4;
    else if (remaining == "F5") combo.key = raylib::KEY_F5;
    else if (remaining == "F6") combo.key = raylib::KEY_F6;
    else if (remaining == "F7") combo.key = raylib::KEY_F7;
    else if (remaining == "F8") combo.key = raylib::KEY_F8;
    else if (remaining == "F9") combo.key = raylib::KEY_F9;
    else if (remaining == "F10") combo.key = raylib::KEY_F10;
    else if (remaining == "F11") combo.key = raylib::KEY_F11;
    else if (remaining == "F12") combo.key = raylib::KEY_F12;
    
    return combo;
}

// Parse a test script file
inline std::vector<TestCommand> parseScript(const std::string& path) {
    std::vector<TestCommand> commands;
    std::ifstream file(path);
    
    if (!file.is_open()) {
        return commands;
    }
    
    std::string line;
    int lineNumber = 0;
    
    while (std::getline(file, line)) {
        lineNumber++;
        
        // Trim whitespace
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        line = line.substr(start);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;
        
        TestCommand cmd;
        cmd.lineNumber = lineNumber;
        
        // Parse command
        std::istringstream iss(line);
        std::string verb;
        iss >> verb;
        
        if (verb == "type") {
            cmd.type = CommandType::Type;
            // Get the rest of the line (quoted string)
            std::getline(iss >> std::ws, cmd.arg1);
            // Remove quotes if present
            if (!cmd.arg1.empty() && cmd.arg1.front() == '"') {
                cmd.arg1 = cmd.arg1.substr(1);
                if (!cmd.arg1.empty() && cmd.arg1.back() == '"') {
                    cmd.arg1.pop_back();
                }
            }
        } else if (verb == "key") {
            cmd.type = CommandType::Key;
            iss >> cmd.arg1;
        } else if (verb == "select_all") {
            cmd.type = CommandType::SelectAll;
        } else if (verb == "click") {
            cmd.type = CommandType::Click;
            iss >> cmd.intArg >> cmd.intArg2;  // x y
        } else if (verb == "double_click") {
            cmd.type = CommandType::DoubleClick;
            iss >> cmd.intArg >> cmd.intArg2;  // x y
        } else if (verb == "drag") {
            cmd.type = CommandType::Drag;
            iss >> cmd.intArg >> cmd.intArg2 >> cmd.intArg3 >> cmd.intArg4;  // x1 y1 x2 y2
        } else if (verb == "mouse_move") {
            cmd.type = CommandType::MouseMove;
            iss >> cmd.intArg >> cmd.intArg2;  // x y
        } else if (verb == "wait") {
            cmd.type = CommandType::Wait;
            iss >> cmd.intArg;
            if (cmd.intArg <= 0) cmd.intArg = 1;
        } else if (verb == "validate") {
            cmd.type = CommandType::Validate;
            std::string rest;
            std::getline(iss >> std::ws, rest);
            // Parse property=value (value may contain spaces)
            size_t eq = rest.find('=');
            if (eq != std::string::npos) {
                cmd.arg1 = rest.substr(0, eq);
                cmd.arg2 = rest.substr(eq + 1);
            }
        } else if (verb == "dump_document") {
            cmd.type = CommandType::DumpDocument;
            iss >> cmd.arg1;
        } else if (verb == "screenshot") {
            cmd.type = CommandType::Screenshot;
            iss >> cmd.arg1;
        } else if (verb == "clear") {
            cmd.type = CommandType::Clear;
        } else if (verb == "menu_open") {
            cmd.type = CommandType::MenuOpen;
            std::getline(iss >> std::ws, cmd.arg1);
            // Remove quotes if present
            if (!cmd.arg1.empty() && cmd.arg1.front() == '"') {
                cmd.arg1 = cmd.arg1.substr(1);
                if (!cmd.arg1.empty() && cmd.arg1.back() == '"') {
                    cmd.arg1.pop_back();
                }
            }
        } else if (verb == "menu_select") {
            cmd.type = CommandType::MenuSelect;
            std::getline(iss >> std::ws, cmd.arg1);
            // Remove quotes if present
            if (!cmd.arg1.empty() && cmd.arg1.front() == '"') {
                cmd.arg1 = cmd.arg1.substr(1);
                if (!cmd.arg1.empty() && cmd.arg1.back() == '"') {
                    cmd.arg1.pop_back();
                }
            }
        } else if (verb == "click_outline") {
            cmd.type = CommandType::ClickOutline;
            std::getline(iss >> std::ws, cmd.arg1);
            // Remove quotes if present
            if (!cmd.arg1.empty() && cmd.arg1.front() == '"') {
                cmd.arg1 = cmd.arg1.substr(1);
                if (!cmd.arg1.empty() && cmd.arg1.back() == '"') {
                    cmd.arg1.pop_back();
                }
            }
        } else if (verb == "expect_text") {
            cmd.type = CommandType::ExpectText;
            std::getline(iss >> std::ws, cmd.arg1);
            // Remove quotes if present
            if (!cmd.arg1.empty() && cmd.arg1.front() == '"') {
                cmd.arg1 = cmd.arg1.substr(1);
                if (!cmd.arg1.empty() && cmd.arg1.back() == '"') {
                    cmd.arg1.pop_back();
                }
            }
        } else {
            cmd.type = CommandType::Unknown;
            cmd.arg1 = verb;
        }
        
        commands.push_back(cmd);
    }
    
    return commands;
}

// Script runner state
class ScriptRunner {
public:
    // Default timeout: 600 frames (~10 seconds at 60fps)
    static constexpr int DEFAULT_TIMEOUT_FRAMES = 600;
    
    ScriptRunner() = default;
    
    void loadScript(const std::string& path) {
        commands_ = parseScript(path);
        currentIndex_ = 0;
        waitFrames_ = 0;
        frameCount_ = 0;
        scriptStartFrame_ = 0;
        results_.clear();
        errors_.clear();
        scriptResults_.clear();
        finished_ = false;
        failed_ = false;
        timedOut_ = false;
        currentScriptName_ = path;
    }
    
    // Set timeout in frames (0 = no timeout)
    void setTimeoutFrames(int frames) { timeoutFrames_ = frames; }
    
    // Load multiple scripts from a directory (batch mode)
    // Inserts 'clear' command between each script automatically
    void loadScriptsFromDirectory(const std::string& dirPath) {
        commands_.clear();
        scriptResults_.clear();
        results_.clear();
        errors_.clear();
        currentIndex_ = 0;
        waitFrames_ = 0;
        frameCount_ = 0;
        scriptStartFrame_ = 0;
        finished_ = false;
        failed_ = false;
        timedOut_ = false;
        
        // Find all .e2e files in directory
        std::vector<std::string> scripts;
        for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
            if (entry.path().extension() == ".e2e") {
                scripts.push_back(entry.path().string());
            }
        }
        
        // Sort alphabetically for consistent ordering
        std::sort(scripts.begin(), scripts.end());
        
        // Load each script with clear commands between them
        for (size_t i = 0; i < scripts.size(); ++i) {
            const auto& scriptPath = scripts[i];
            std::string scriptName = std::filesystem::path(scriptPath).stem().string();
            
            // Add script start marker
            TestCommand startMarker;
            startMarker.type = CommandType::Comment;
            startMarker.arg1 = "=== START: " + scriptName + " ===";
            startMarker.lineNumber = 0;
            commands_.push_back(startMarker);
            
            // Track which script each command belongs to
            auto scriptCommands = parseScript(scriptPath);
            for (auto& cmd : scriptCommands) {
                commands_.push_back(cmd);
            }
            
            // Add script end marker and record point
            TestCommand endMarker;
            endMarker.type = CommandType::Comment;
            endMarker.arg1 = "=== END: " + scriptName + " ===";
            endMarker.lineNumber = 0;
            commands_.push_back(endMarker);
            
            // Add clear between scripts (except after last)
            if (i < scripts.size() - 1) {
                TestCommand clearCmd;
                clearCmd.type = CommandType::Clear;
                clearCmd.lineNumber = 0;
                commands_.push_back(clearCmd);
            }
            
            // Track script boundaries
            scriptNames_.push_back(scriptName);
        }
        
        printf("[BATCH] Loaded %zu scripts with %zu total commands\n", 
               scripts.size(), commands_.size());
    }
    
    bool hasCommands() const { return !commands_.empty(); }
    bool isFinished() const { return finished_; }
    bool hasFailed() const { return failed_; }
    bool hasTimedOut() const { return timedOut_; }
    int frameCount() const { return frameCount_; }
    const std::vector<ValidationResult>& results() const { return results_; }
    const std::vector<ScriptError>& errors() const { return errors_; }
    
    // Debug overlay info - get current command description and timeout status
    std::string getCurrentCommandDescription() const {
        if (finished_ || currentIndex_ >= commands_.size()) {
            return "(finished)";
        }
        const TestCommand& cmd = commands_[currentIndex_];
        std::string desc;
        switch (cmd.type) {
            case CommandType::Type: desc = "type \"" + cmd.arg1 + "\""; break;
            case CommandType::Key: desc = "key " + cmd.arg1; break;
            case CommandType::SelectAll: desc = "select_all"; break;
            case CommandType::Click: desc = "click " + std::to_string(cmd.intArg) + " " + std::to_string(cmd.intArg2); break;
            case CommandType::DoubleClick: desc = "double_click " + std::to_string(cmd.intArg) + " " + std::to_string(cmd.intArg2); break;
            case CommandType::Drag: desc = "drag"; break;
            case CommandType::MouseMove: desc = "mouse_move " + std::to_string(cmd.intArg) + " " + std::to_string(cmd.intArg2); break;
            case CommandType::Wait: desc = "wait " + std::to_string(cmd.intArg); break;
            case CommandType::Validate: desc = "validate " + cmd.arg1 + "=" + cmd.arg2; break;
            case CommandType::ExpectText: desc = "expect_text \"" + cmd.arg1 + "\""; break;
            case CommandType::DumpDocument: desc = "dump_document"; break;
            case CommandType::Screenshot: desc = "screenshot " + cmd.arg1; break;
            case CommandType::Clear: desc = "clear"; break;
            case CommandType::MenuOpen: desc = "menu_open \"" + cmd.arg1 + "\""; break;
            case CommandType::MenuSelect: desc = "menu_select \"" + cmd.arg1 + "\""; break;
            case CommandType::ClickOutline: desc = "click_outline \"" + cmd.arg1 + "\""; break;
            case CommandType::Comment: desc = "# comment"; break;
            case CommandType::Unknown: desc = "unknown: " + cmd.arg1; break;
            default: desc = "(unknown)"; break;
        }
        return desc;
    }
    
    // Get remaining frames before timeout (returns -1 if no timeout)
    int getRemainingTimeoutFrames() const {
        if (timeoutFrames_ <= 0) return -1;
        int elapsed = frameCount_ - scriptStartFrame_;
        int remaining = timeoutFrames_ - elapsed;
        return remaining > 0 ? remaining : 0;
    }
    
    // Get remaining seconds before timeout (at 60fps)
    float getRemainingTimeoutSeconds() const {
        int frames = getRemainingTimeoutFrames();
        if (frames < 0) return -1.0f;
        return static_cast<float>(frames) / 60.0f;
    }
    
    // Check if debug overlay should be shown
    bool showDebugOverlay() const { return debugOverlay_; }
    void setDebugOverlay(bool show) { debugOverlay_ = show; }
    
    // Set callback for getting document properties (set from main.cpp)
    using PropertyGetter = std::function<std::string(const std::string&)>;
    void setPropertyGetter(PropertyGetter getter) { propertyGetter_ = getter; }
    
    // Set callback for taking screenshots
    using ScreenshotTaker = std::function<void(const std::string&)>;
    void setScreenshotTaker(ScreenshotTaker taker) { screenshotTaker_ = taker; }
    
    // Set callback for dumping document
    using DocumentDumper = std::function<void(const std::string&)>;
    void setDocumentDumper(DocumentDumper dumper) { documentDumper_ = dumper; }
    
    // Set callback for clearing document (for batch mode)
    using DocumentClearer = std::function<void()>;
    void setDocumentClearer(DocumentClearer clearer) { documentClearer_ = clearer; }
    
    // Set callback for opening a menu by name
    using MenuOpener = std::function<bool(const std::string&)>;
    void setMenuOpener(MenuOpener opener) { menuOpener_ = opener; }
    
    // Set callback for selecting a menu item by name
    using MenuItemSelector = std::function<bool(const std::string&)>;
    void setMenuItemSelector(MenuItemSelector selector) { menuItemSelector_ = selector; }
    
    // Set callback for clicking an outline entry by heading text
    using OutlineClicker = std::function<bool(const std::string&)>;
    void setOutlineClicker(OutlineClicker clicker) { outlineClicker_ = clicker; }
    
    // Execute one frame of the script
    void tick() {
        if (finished_ || commands_.empty()) return;
        
        // Increment frame counter
        frameCount_++;
        
        // Check for timeout
        if (timeoutFrames_ > 0) {
            int framesInCurrentScript = frameCount_ - scriptStartFrame_;
            if (framesInCurrentScript > timeoutFrames_) {
                printf("[TIMEOUT] Script exceeded %d frames at command %zu/%zu\n", 
                       timeoutFrames_, currentIndex_, commands_.size());
                timedOut_ = true;
                failed_ = true;
                finished_ = true;
                
                // Record timeout error
                ScriptError error;
                error.lineNumber = currentIndex_ < commands_.size() ? 
                    commands_[currentIndex_].lineNumber : 0;
                error.command = "timeout";
                error.message = "Script timed out after " + std::to_string(timeoutFrames_) + " frames";
                errors_.push_back(error);
                return;
            }
        }
        
        // Handle wait
        if (waitFrames_ > 0) {
            waitFrames_--;
            
            // Release mouse button if pending (for click simulation)
            if (waitFrames_ == 0 && pendingMouseRelease_) {
                test_input::simulate_mouse_button_release(raylib::MOUSE_BUTTON_LEFT);
                pendingMouseRelease_ = false;
                waitFrames_ = 2;  // Wait for click to be processed
            }
            
            // Check for pending double-click on the last wait frame
            if (waitFrames_ == 0 && pendingDoubleClick_) {
                raylib::Rectangle clickRect = {
                    doubleClickPos_.x,
                    doubleClickPos_.y,
                    1.0f, 1.0f
                };
                input_injector::schedule_mouse_click_at(clickRect);
                input_injector::inject_scheduled_click();
                pendingDoubleClick_ = false;
                waitFrames_ = 2;  // Wait for second click to process
            }
            return;
        }
        
        // Process current command
        if (currentIndex_ >= commands_.size()) {
            finished_ = true;
            return;
        }
        
        const auto& cmd = commands_[currentIndex_];
        
        switch (cmd.type) {
            case CommandType::Type:
                executeType(cmd);
                break;
            case CommandType::Key:
                executeKey(cmd);
                break;
            case CommandType::SelectAll:
                executeSelectAll(cmd);
                break;
            case CommandType::Click:
                executeClick(cmd);
                break;
            case CommandType::DoubleClick:
                executeDoubleClick(cmd);
                break;
            case CommandType::Drag:
                executeDrag(cmd);
                break;
            case CommandType::MouseMove:
                executeMouseMove(cmd);
                break;
            case CommandType::Wait:
                waitFrames_ = cmd.intArg;
                break;
            case CommandType::Validate:
                executeValidate(cmd);
                break;
            case CommandType::DumpDocument:
                executeDumpDocument(cmd);
                break;
            case CommandType::Screenshot:
                executeScreenshot(cmd);
                break;
            case CommandType::Clear:
                executeClear(cmd);
                break;
            case CommandType::MenuOpen:
                executeMenuOpen(cmd);
                break;
            case CommandType::MenuSelect:
                executeMenuSelect(cmd);
                break;
            case CommandType::ClickOutline:
                executeClickOutline(cmd);
                break;
            case CommandType::ExpectText:
                executeExpectText(cmd);
                break;
            case CommandType::Comment:
                // Comments are ignored
                break;
            case CommandType::Unknown:
                // Report unknown command as error
                reportError(cmd, "Unknown command: '" + cmd.arg1 + "'");
                break;
            default:
                break;
        }
        
        currentIndex_++;
        
        if (currentIndex_ >= commands_.size()) {
            finished_ = true;
        }
    }
    
    // Print test results
    void printResults() const {
        int passed = 0;
        int failed = 0;
        
        for (const auto& result : results_) {
            if (result.success) {
                passed++;
            } else {
                failed++;
                if (result.lineNumber > 0) {
                    printf("[FAIL] Line %d: %s: expected '%s', got '%s'\n",
                           result.lineNumber,
                           result.property.c_str(),
                           result.expected.c_str(),
                           result.actual.c_str());
                } else {
                    printf("[FAIL] %s: expected '%s', got '%s'\n",
                           result.property.c_str(),
                           result.expected.c_str(),
                           result.actual.c_str());
                }
            }
        }
        
        // Print script errors summary
        if (!errors_.empty()) {
            printf("\nScript Errors: %zu\n", errors_.size());
            for (const auto& error : errors_) {
                printf("  Line %d: %s\n", error.lineNumber, error.message.c_str());
            }
        }
        
        if (timedOut_) {
            printf("\n[TIMEOUT] Test execution timed out after %d frames\n", frameCount_);
        }
        
        printf("E2E Test Results: %d passed, %d failed (total frames: %d)\n", 
               passed, failed, frameCount_);
    }
    
private:
    void executeType(const TestCommand& cmd) {
        // Queue each character for typing
        for (char c : cmd.arg1) {
            test_input::push_char(c);
        }
        // Wait enough frames for all characters to be processed (1 char per frame + buffer)
        waitFrames_ = static_cast<int>(cmd.arg1.size()) + 2;
    }
    
    void executeKey(const TestCommand& cmd) {
        KeyCombo combo = parseKeyCombo(cmd.arg1);
        
        // For keyboard shortcuts, we need to simulate modifier + key
        // The input system checks IsKeyDown for modifiers
        if (combo.ctrl) {
            input_injector::set_key_down(e2e_keys::CTRL);
        }
        if (combo.shift) {
            input_injector::set_key_down(e2e_keys::SHIFT);
        }
        if (combo.alt) {
            input_injector::set_key_down(e2e_keys::ALT);
        }
        
        // Push the actual key
        test_input::push_key(combo.key);
        
        // Wait a frame, then release modifiers
        waitFrames_ = 2;
    }
    
    void executeSelectAll(const TestCommand& /*cmd*/) {
        // Ctrl+A
        input_injector::set_key_down(e2e_keys::CTRL);
        test_input::push_key(65);  // KEY_A
        waitFrames_ = 2;
    }
    
    void executeClick(const TestCommand& cmd) {
        // Set mouse position and simulate click using test_input
        vec2 clickPos = {static_cast<float>(cmd.intArg), 
                         static_cast<float>(cmd.intArg2)};
        test_input::set_mouse_position(clickPos);
        test_input::simulate_mouse_button_press(raylib::MOUSE_BUTTON_LEFT);
        // Set wait frames - the release will happen after 1 frame
        waitFrames_ = 1;
        pendingMouseRelease_ = true;
    }
    
    void executeDoubleClick(const TestCommand& cmd) {
        // Double-click: two clicks in quick succession
        raylib::Rectangle clickRect = {
            static_cast<float>(cmd.intArg),
            static_cast<float>(cmd.intArg2),
            1.0f, 1.0f
        };
        // First click
        input_injector::schedule_mouse_click_at(clickRect);
        input_injector::inject_scheduled_click();
        // Note: actual double-click detection happens via timing in the app
        // For testing, we simulate by setting a flag or using rapid clicks
        // For now, queue a second click by scheduling it
        pendingDoubleClick_ = true;
        doubleClickPos_ = {static_cast<float>(cmd.intArg), static_cast<float>(cmd.intArg2)};
        waitFrames_ = 2;  // First click, then second click will happen
    }
    
    void executeDrag(const TestCommand& cmd) {
        // Drag from (intArg, intArg2) to (intArg3, intArg4)
        // First move to start position and click down
        raylib::Rectangle startRect = {
            static_cast<float>(cmd.intArg),
            static_cast<float>(cmd.intArg2),
            1.0f, 1.0f
        };
        input_injector::schedule_mouse_click_at(startRect);
        input_injector::inject_scheduled_click();
        
        // Store end position for drag - we'll need multiple frames
        // For now, set end position immediately (simplified drag)
        input_injector::set_mouse_position(cmd.intArg3, cmd.intArg4);
        
        waitFrames_ = 5;  // Wait for drag to complete
    }
    
    void executeMouseMove(const TestCommand& cmd) {
        input_injector::set_mouse_position(cmd.intArg, cmd.intArg2);
        waitFrames_ = 1;
    }
    
    void executeValidate(const TestCommand& cmd) {
        if (!propertyGetter_) {
            reportError(cmd, "No property getter configured");
            return;
        }
        
        ValidationResult result;
        result.property = cmd.arg1;
        result.expected = cmd.arg2;
        result.lineNumber = cmd.lineNumber;
        result.actual = propertyGetter_(cmd.arg1);
        
        // Check for unknown property
        if (result.actual == "<unknown>") {
            reportError(cmd, "Unknown property: '" + cmd.arg1 + "'");
            result.success = false;
            result.message = "Unknown property";
        } else {
            result.success = (result.actual == result.expected);
            if (!result.success) {
                result.message = "Value mismatch";
            }
        }
        
        if (!result.success) {
            failed_ = true;
        }
        
        results_.push_back(result);
    }
    
    void executeDumpDocument(const TestCommand& cmd) {
        if (documentDumper_) {
            documentDumper_(cmd.arg1);
        }
    }
    
    void executeScreenshot(const TestCommand& cmd) {
        if (screenshotTaker_) {
            screenshotTaker_(cmd.arg1);
        }
    }
    
    void executeClear(const TestCommand& /*cmd*/) {
        if (documentClearer_) {
            documentClearer_();
        }
        // Reset script start frame for timeout tracking in batch mode
        scriptStartFrame_ = frameCount_;
        waitFrames_ = 2;  // Wait for UI to update
    }
    
    void executeMenuOpen(const TestCommand& cmd) {
        if (!menuOpener_) {
            reportError(cmd, "No menu opener configured");
            return;
        }
        if (!menuOpener_(cmd.arg1)) {
            reportError(cmd, "Failed to open menu: '" + cmd.arg1 + "'");
        }
        waitFrames_ = 2;  // Wait for menu to render
    }
    
    void executeMenuSelect(const TestCommand& cmd) {
        if (!menuItemSelector_) {
            reportError(cmd, "No menu item selector configured");
            return;
        }
        if (!menuItemSelector_(cmd.arg1)) {
            reportError(cmd, "Failed to select menu item: '" + cmd.arg1 + "'");
        }
        waitFrames_ = 2;  // Wait for action to process
    }
    
    void executeClickOutline(const TestCommand& cmd) {
        if (!outlineClicker_) {
            reportError(cmd, "No outline clicker configured");
            return;
        }
        if (!outlineClicker_(cmd.arg1)) {
            reportError(cmd, "Failed to click outline entry: '" + cmd.arg1 + "'");
        }
        waitFrames_ = 2;  // Wait for navigation to complete
    }
    
    void executeExpectText(const TestCommand& cmd) {
        const std::string& expectedText = cmd.arg1;
        
        // Check if the text is visible on screen
        auto& registry = test_input::VisibleTextRegistry::instance();
        bool found = registry.containsText(expectedText);
        
        ValidationResult result;
        result.property = "visible_text";
        result.expected = expectedText;
        result.lineNumber = cmd.lineNumber;
        result.success = found;
        
        if (found) {
            result.actual = expectedText;
            result.message = "Text found on screen";
        } else {
            // Show what was visible for debugging
            std::string allVisible = registry.getAllText();
            if (allVisible.empty()) {
                result.actual = "(no visible text)";
            } else if (allVisible.length() > 200) {
                result.actual = allVisible.substr(0, 200) + "...";
            } else {
                result.actual = allVisible;
            }
            result.message = "Text not found on screen";
            failed_ = true;
        }
        
        results_.push_back(result);
    }
    
    void reportError(const TestCommand& cmd, const std::string& message) {
        ScriptError error;
        error.lineNumber = cmd.lineNumber;
        error.command = cmd.arg1;
        error.message = message;
        errors_.push_back(error);
        failed_ = true;
        
        // Print error immediately
        printf("[ERROR] Line %d: %s\n", cmd.lineNumber, message.c_str());
    }
    
    std::vector<TestCommand> commands_;
    std::size_t currentIndex_ = 0;
    int waitFrames_ = 0;
    int frameCount_ = 0;           // Total frames elapsed
    int scriptStartFrame_ = 0;     // Frame when current script started (for batch timeout)
    int timeoutFrames_ = DEFAULT_TIMEOUT_FRAMES;  // Max frames per script (0 = no timeout)
    std::vector<ValidationResult> results_;
    std::vector<ScriptError> errors_;
    bool finished_ = false;
    bool failed_ = false;
    bool timedOut_ = false;
    bool debugOverlay_ = false;  // Show debug overlay with current command
    
    // For double-click simulation
    bool pendingDoubleClick_ = false;
    vec2 doubleClickPos_ = {0, 0};
    
    // For click release (mouse needs to be released after a frame)
    bool pendingMouseRelease_ = false;
    
    PropertyGetter propertyGetter_;
    ScreenshotTaker screenshotTaker_;
    DocumentDumper documentDumper_;
    DocumentClearer documentClearer_;
    MenuOpener menuOpener_;
    MenuItemSelector menuItemSelector_;
    OutlineClicker outlineClicker_;
    
    // Batch mode tracking
    std::string currentScriptName_;
    std::vector<std::string> scriptNames_;
    
    // Per-script results for batch mode
    struct ScriptResult {
        std::string name;
        bool passed = true;
        int validationsPassed = 0;
        int validationsFailed = 0;
        int errorCount = 0;
    };
    std::vector<ScriptResult> scriptResults_;
};

}  // namespace e2e

