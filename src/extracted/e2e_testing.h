// E2E Testing Framework for Afterhours
// Complete testing framework with input injection, script DSL, and UI assertions.
//
// Architecture (5 layers):
// 1. input_injector - Low-level synthetic key/mouse state
// 2. test_input - High-level input queue with frame awareness
// 3. TestInputProvider - Afterhours UIContext integration (ECS component)
// 4. visible_text - Track rendered text for assertions
// 5. E2ERunner - Script DSL parser and runner
//
// See src/testing/ for the full multi-file implementation.
// This is a standalone single-header version for quick integration.
//
// To integrate into Afterhours: add as src/plugins/testing.h

#pragma once

#include <algorithm>
#include <array>
#include <bitset>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

namespace afterhours {
namespace testing {

// #region agent log
inline void debug_log(const char* location, const char* message,
                      const char* hypothesisId, const char* runId,
                      const std::string& dataJson) {
  const char* logPath = "/Users/gabeochoa/p/wordproc/.cursor/debug.log";
  auto now = std::chrono::system_clock::now();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch())
                .count();
  std::ofstream out(logPath, std::ios::app);
  if (!out.is_open()) return;
  out << "{\"sessionId\":\"debug-session\",\"runId\":\"" << runId
      << "\",\"hypothesisId\":\"" << hypothesisId << "\",\"location\":\""
      << location << "\",\"message\":\"" << message << "\",\"data\":"
      << dataJson << ",\"timestamp\":" << ms << "}\n";
}
// #endregion agent log

//=============================================================================
// LAYER 1: LOW-LEVEL INPUT INJECTOR (from afterhours)
//=============================================================================
// Using afterhours input_injector directly - no local implementation needed
}  // namespace testing
}  // namespace afterhours

// Include afterhours E2E testing core
#include "../../vendor/afterhours/src/plugins/e2e_testing/test_input.h"
#include "../../vendor/afterhours/src/plugins/e2e_testing/visible_text.h"

// Re-open namespace for compatibility shims and remaining layers
namespace afterhours {
namespace testing {
namespace test_input {

// Compatibility: set_test_mode/is_test_mode functions
inline void set_test_mode(bool enabled) { detail::test_mode = enabled; }
inline bool is_test_mode() { return detail::test_mode; }

// Compatibility: MouseState struct (wraps input_injector mouse)
struct MouseState {
  std::optional<float> x, y;
  bool left_held = false;
  bool left_pressed = false;
  bool left_released = false;
  int press_frames = 0;
  bool active = false;
};

} // namespace test_input

//=============================================================================
// LAYER 3: AFTERHOURS UICONTEXT INTEGRATION (optional)
//=============================================================================

// This is a template that integrates with Afterhours UIContext.
// Users should uncomment and adapt when integrating into Afterhours.

/*
#include <afterhours/src/ecs.h>
#include <afterhours/src/plugins/ui/context.h>

namespace test_input_provider {

constexpr size_t MAX_ACTIONS = 32;

struct TestInputProvider : afterhours::BaseComponent {
  std::optional<float> mouse_x, mouse_y;
  bool mouse_left_down = false;
  bool mouse_left_pressed = false;
  bool mouse_left_released = false;
  std::optional<int> pending_action;
  std::bitset<MAX_ACTIONS> held_actions;
  bool active = false;
  
  void setMousePosition(float x, float y) { mouse_x = x; mouse_y = y; active = true; }
  void pressMouseLeft() { mouse_left_down = true; mouse_left_pressed = true; active = true; }
  void releaseMouseLeft() { mouse_left_down = false; mouse_left_released = true; active = true; }
  void queueAction(int action) { pending_action = action; active = true; }
  void holdAction(int action) { if (action >= 0 && action < MAX_ACTIONS) held_actions[action] = true; active = true; }
  void releaseAction(int action) { if (action >= 0 && action < MAX_ACTIONS) held_actions[action] = false; }
  void resetFrame() { mouse_left_pressed = false; mouse_left_released = false; pending_action = std::nullopt; }
  void reset() { *this = TestInputProvider{}; }
};

template<typename InputAction>
struct TestInputSystem : afterhours::System<afterhours::ui::UIContext<InputAction>> {
  void for_each_with(afterhours::Entity&, afterhours::ui::UIContext<InputAction>& ctx, float) override {
    auto* provider = afterhours::EntityHelper::get_singleton_cmp<TestInputProvider>();
    if (!provider || !provider->active) return;
    
    if (provider->mouse_x && provider->mouse_y) {
      ctx.mouse.pos = {*provider->mouse_x, *provider->mouse_y};
    }
    ctx.mouse.left_down = provider->mouse_left_down;
    
    if (provider->pending_action) {
      ctx.last_action = static_cast<InputAction>(*provider->pending_action);
      provider->pending_action = std::nullopt;
    }
    
    for (size_t i = 0; i < provider->held_actions.size() && i < ctx.all_actions.size(); ++i) {
      if (provider->held_actions[i]) ctx.all_actions[i] = true;
    }
  }
};

} // namespace test_input_provider
*/

//=============================================================================
// LAYER 4: VISIBLE TEXT REGISTRY (using afterhours)
//=============================================================================

// Compatibility wrapper: visible_text namespace using afterhours::testing::VisibleTextRegistry
namespace visible_text {

// Registry alias for backward compatibility
using Registry = VisibleTextRegistry;

inline void clear() { VisibleTextRegistry::instance().clear(); }
inline void register_text(const std::string& t) { VisibleTextRegistry::instance().register_text(t); }
inline bool contains(const std::string& t) { return VisibleTextRegistry::instance().contains(t); }
inline bool has_exact(const std::string& t) { return VisibleTextRegistry::instance().has_exact(t); }
inline std::string get_all() { return VisibleTextRegistry::instance().get_all(); }

} // namespace visible_text

//=============================================================================
// LAYER 5: E2E SCRIPT RUNNER
//=============================================================================

enum class CommandType {
  Type, Key, Click, DoubleClick, Drag, MouseMove, Wait,
  Validate, ExpectText, Screenshot, Clear, 
  MenuOpen, MenuSelect, Comment, Unknown
};

struct TestCommand {
  CommandType type = CommandType::Unknown;
  std::string arg1, arg2;
  int x = 0, y = 0, x2 = 0, y2 = 0;
  int line_number = 0;
};

struct ValidationResult {
  bool success = true;
  std::string property, expected, actual, message;
  int line_number = 0;
};

struct ScriptError {
  int line_number = 0;
  std::string command, message;
};

struct ScriptResult {
  std::string name;
  std::string path;
  bool expected_to_pass = true;  // Based on pass_* or fail_* prefix
  bool passed = true;
  int error_count = 0;
  int validation_failures = 0;
};

// KeyCombo and parse_key_combo are now from afterhours/core/key_codes.h
using afterhours::KeyCombo;
using afterhours::parse_key_combo;

inline std::vector<TestCommand> parse_script(const std::string& path) {
  std::vector<TestCommand> cmds;
  std::ifstream file(path);
  if (!file.is_open()) return cmds;
  
  std::string line;
  int line_num = 0;
  
  while (std::getline(file, line)) {
    line_num++;
    size_t start = line.find_first_not_of(" \t");
    if (start == std::string::npos) continue;
    line = line.substr(start);
    if (line.empty() || line[0] == '#') continue;
    
    TestCommand cmd;
    cmd.line_number = line_num;
    std::istringstream iss(line);
    std::string verb;
    iss >> verb;
    
    auto parse_quoted = [&]() {
      std::string result;
      std::getline(iss >> std::ws, result);
      if (!result.empty() && result.front() == '"') result = result.substr(1);
      if (!result.empty() && result.back() == '"') result.pop_back();
      return result;
    };
    
    if (verb == "type") { cmd.type = CommandType::Type; cmd.arg1 = parse_quoted(); }
    else if (verb == "key") { cmd.type = CommandType::Key; iss >> cmd.arg1; }
    else if (verb == "click") { cmd.type = CommandType::Click; iss >> cmd.x >> cmd.y; }
    else if (verb == "double_click") { cmd.type = CommandType::DoubleClick; iss >> cmd.x >> cmd.y; }
    else if (verb == "drag") { cmd.type = CommandType::Drag; iss >> cmd.x >> cmd.y >> cmd.x2 >> cmd.y2; }
    else if (verb == "mouse_move") { cmd.type = CommandType::MouseMove; iss >> cmd.x >> cmd.y; }
    else if (verb == "wait") { cmd.type = CommandType::Wait; iss >> cmd.x; if (cmd.x <= 0) cmd.x = 1; }
    else if (verb == "validate") {
      cmd.type = CommandType::Validate;
      std::string rest; std::getline(iss >> std::ws, rest);
      size_t eq = rest.find('=');
      if (eq != std::string::npos) { cmd.arg1 = rest.substr(0, eq); cmd.arg2 = rest.substr(eq + 1); }
    }
    else if (verb == "expect_text") { cmd.type = CommandType::ExpectText; cmd.arg1 = parse_quoted(); }
    else if (verb == "screenshot") { cmd.type = CommandType::Screenshot; iss >> cmd.arg1; }
    else if (verb == "clear") { cmd.type = CommandType::Clear; }
    else if (verb == "menu_open") { cmd.type = CommandType::MenuOpen; cmd.arg1 = parse_quoted(); }
    else if (verb == "menu_select") { cmd.type = CommandType::MenuSelect; cmd.arg1 = parse_quoted(); }
    else if (verb == "select_all") { cmd.type = CommandType::Key; cmd.arg1 = "CTRL+A"; }  // Convenience alias
    else { cmd.type = CommandType::Unknown; cmd.arg1 = verb; }
    
    cmds.push_back(cmd);
  }
  return cmds;
}

class E2ERunner {
public:
  static constexpr int DEFAULT_TIMEOUT = 600; // ~10s at 60fps
  
  void load_script(const std::string& path) {
    commands_ = parse_script(path);
    script_path_ = path;
    reset();
  }
  
  void load_scripts_from_directory(const std::string& dir) {
    commands_.clear();
    script_results_.clear();
    script_boundaries_.clear();
    reset();
    
    std::vector<std::string> scripts;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
      if (entry.path().extension() == ".e2e") {
        scripts.push_back(entry.path().string());
      }
    }
    std::sort(scripts.begin(), scripts.end());
    
    for (size_t i = 0; i < scripts.size(); ++i) {
      // Record script boundary (command index where this script starts)
      script_boundaries_.push_back(commands_.size());
      
      // Parse script name and expected outcome
      std::string script_name = std::filesystem::path(scripts[i]).stem().string();
      ScriptResult result;
      result.path = scripts[i];
      result.name = script_name;
      
      // Determine expected outcome based on prefix
      if (script_name.substr(0, 5) == "pass_") {
        result.expected_to_pass = true;
      } else if (script_name.substr(0, 5) == "fail_") {
        result.expected_to_pass = false;
      } else {
        result.expected_to_pass = true;  // Default to expecting pass
      }
      script_results_.push_back(result);
      
      auto script_cmds = parse_script(scripts[i]);
      for (auto& cmd : script_cmds) commands_.push_back(cmd);
      
      // Add clear command between scripts (always, to reset state)
      TestCommand clear_cmd;
      clear_cmd.type = CommandType::Clear;
      commands_.push_back(clear_cmd);
    }
    // Add sentinel for last script boundary
    script_boundaries_.push_back(commands_.size());
    
    printf("[BATCH] Loaded %zu scripts with %zu commands\n", scripts.size(), commands_.size());
  }
  
  void reset() {
    index_ = 0; wait_frames_ = 0; frame_count_ = 0; script_start_ = 0;
    results_.clear(); errors_.clear();
    finished_ = failed_ = timed_out_ = pending_release_ = false;
    pending_key_release_ = false;
    pending_key_ = 0;
    pending_ctrl_ = pending_shift_ = pending_alt_ = false;
    current_script_idx_ = 0;
    current_script_errors_ = 0;
    current_script_validation_failures_ = 0;
  }
  
  void set_timeout_frames(int frames) { timeout_ = frames; }
  void set_property_getter(std::function<std::string(const std::string&)> fn) { property_getter_ = fn; }
  void set_screenshot_callback(std::function<void(const std::string&)> fn) { screenshot_fn_ = fn; }
  void set_clear_callback(std::function<void()> fn) { clear_fn_ = fn; }
  void set_menu_opener(std::function<bool(const std::string&)> fn) { menu_opener_ = fn; }
  void set_menu_selector(std::function<bool(const std::string&)> fn) { menu_selector_ = fn; }
  void set_document_dumper(std::function<void(const std::string&)> fn) { document_dumper_ = fn; }
  void set_outline_clicker(std::function<bool(const std::string&)> fn) { outline_clicker_ = fn; }
  
  void tick() {
    if (finished_ || commands_.empty()) return;
    frame_count_++;

    // #region agent log
    if (frame_count_ == 1 || (frame_count_ % 120 == 0)) {
      std::ostringstream data;
      data << "{\"frameCount\":" << frame_count_
           << ",\"index\":" << index_
           << ",\"waitFrames\":" << wait_frames_
           << ",\"pendingRelease\":"
           << (pending_release_ ? "true" : "false")
           << ",\"commands\":" << commands_.size()
           << ",\"timeout\":" << timeout_
           << ",\"finished\":" << (finished_ ? "true" : "false") << "}";
      debug_log("e2e_testing.h:686", "Tick state", "H3", "e2e-hang-pre",
                data.str());
    }
    // #endregion agent log
    
    // Timeout check
    if (timeout_ > 0 && (frame_count_ - script_start_) > timeout_) {
      current_script_errors_++;  // Count timeout as an error
      finalize_batch();
      timed_out_ = failed_ = finished_ = true;
      ScriptError err; err.command = "timeout";
      err.message = "Timed out after " + std::to_string(timeout_) + " frames";
      errors_.push_back(err);
      // #region agent log
      {
        std::ostringstream data;
        data << "{\"frameCount\":" << frame_count_
             << ",\"scriptStart\":" << script_start_
             << ",\"timeout\":" << timeout_ << "}";
        debug_log("e2e_testing.h:699", "Runner timeout", "H4", "e2e-hang-pre",
                  data.str());
      }
      // #endregion agent log
      return;
    }
    
    // Handle wait
    if (wait_frames_ > 0) {
      wait_frames_--;
      if (wait_frames_ == 0) {
        // Release mouse if pending
        if (pending_release_) {
          test_input::simulate_mouse_release();
          pending_release_ = false;
          wait_frames_ = 2;
          return;
        }
        // Release key modifiers if pending
        if (pending_key_release_) {
          if (pending_ctrl_) input_injector::set_key_up(341);  // KEY_LEFT_CONTROL
          if (pending_shift_) input_injector::set_key_up(340); // KEY_LEFT_SHIFT
          if (pending_alt_) input_injector::set_key_up(342);   // KEY_LEFT_ALT
          input_injector::set_key_up(pending_key_);
          pending_key_release_ = false;
          pending_ctrl_ = pending_shift_ = pending_alt_ = false;
        }
      }
      return;
    }
    
    if (index_ >= commands_.size()) { finalize_batch(); finished_ = true; return; }
    
    execute(commands_[index_]);
    index_++;
    if (index_ >= commands_.size()) { finalize_batch(); finished_ = true; }
  }
  
  // Finalize the current script in batch mode (called when all commands done or on timeout)
  void finalize_batch() {
    if (script_results_.empty() || current_script_idx_ >= script_results_.size()) return;
    
    auto& result = script_results_[current_script_idx_];
    result.error_count = current_script_errors_;
    result.validation_failures = current_script_validation_failures_;
    
    bool actually_passed = (current_script_errors_ == 0 && current_script_validation_failures_ == 0);
    
    if (result.expected_to_pass) {
      result.passed = actually_passed;
    } else {
      result.passed = !actually_passed;
    }
    
    if (result.passed) {
      printf("[PASS] %s\n", result.name.c_str());
    } else {
      if (result.expected_to_pass) {
        printf("[FAIL] %s (expected pass, got %d errors, %d validation failures)\n", 
               result.name.c_str(), current_script_errors_, current_script_validation_failures_);
      } else {
        printf("[FAIL] %s (expected fail, but passed)\n", result.name.c_str());
      }
    }
  }
  
  bool is_finished() const { return finished_; }
  bool has_failed() const {
    // In batch mode, check if any script failed its expectations
    if (!script_results_.empty()) {
      for (const auto& sr : script_results_) {
        if (!sr.passed) return true;
      }
      return false;
    }
    return failed_;
  }
  bool has_timed_out() const { return timed_out_; }
  int frame_count() const { return frame_count_; }
  const std::vector<ValidationResult>& results() const { return results_; }
  const std::vector<ScriptError>& errors() const { return errors_; }
  const std::vector<ScriptResult>& script_results() const { return script_results_; }
  
  // camelCase aliases for backward compatibility
  bool isFinished() const { return is_finished(); }
  bool hasFailed() const { return has_failed(); }
  bool hasTimedOut() const { return has_timed_out(); }
  int frameCount() const { return frame_count(); }
  void printResults() const { print_results(); }
  bool hasCommands() const { return !commands_.empty(); }
  std::string getCurrentCommandDescription() const { return current_command_desc(); }
  float getRemainingTimeoutSeconds() const {
    if (timeout_ <= 0) return -1.0f;
    int elapsed = frame_count_ - script_start_;
    int remaining = timeout_ - elapsed;
    return remaining > 0 ? static_cast<float>(remaining) / 60.0f : 0.0f;
  }
  int getRemainingTimeoutFrames() const {
    if (timeout_ <= 0) return -1;
    int elapsed = frame_count_ - script_start_;
    int remaining = timeout_ - elapsed;
    return remaining > 0 ? remaining : 0;
  }
  bool showDebugOverlay() const { return debug_overlay_; }
  void setDebugOverlay(bool show) { debug_overlay_ = show; }
  
  std::string current_command_desc() const {
    if (finished_ || index_ >= commands_.size()) return "(finished)";
    const auto& cmd = commands_[index_];
    switch (cmd.type) {
      case CommandType::Type: return "type \"" + cmd.arg1 + "\"";
      case CommandType::Key: return "key " + cmd.arg1;
      case CommandType::Click: return "click " + std::to_string(cmd.x) + " " + std::to_string(cmd.y);
      case CommandType::DoubleClick: return "double_click " + std::to_string(cmd.x) + " " + std::to_string(cmd.y);
      case CommandType::Drag: return "drag " + std::to_string(cmd.x) + " " + std::to_string(cmd.y) + " -> " + std::to_string(cmd.x2) + " " + std::to_string(cmd.y2);
      case CommandType::MouseMove: return "mouse_move " + std::to_string(cmd.x) + " " + std::to_string(cmd.y);
      case CommandType::Wait: return "wait " + std::to_string(cmd.x);
      case CommandType::Validate: return "validate " + cmd.arg1 + "=" + cmd.arg2;
      case CommandType::ExpectText: return "expect_text \"" + cmd.arg1 + "\"";
      case CommandType::Screenshot: return "screenshot " + cmd.arg1;
      case CommandType::Clear: return "clear";
      case CommandType::MenuOpen: return "menu_open \"" + cmd.arg1 + "\"";
      case CommandType::MenuSelect: return "menu_select \"" + cmd.arg1 + "\"";
      case CommandType::Comment: return "# comment";
      case CommandType::Unknown: return "(unknown: " + cmd.arg1 + ")";
      default: return "(unhandled)";
    }
  }
  
  void print_results() const {
    // If in batch mode, print per-script summary
    if (!script_results_.empty()) {
      printf("\n============================================\n");
      printf("          E2E Batch Test Summary            \n");
      printf("============================================\n\n");
      
      int scripts_passed = 0, scripts_failed = 0;
      std::vector<std::string> failed_names;
      
      for (const auto& sr : script_results_) {
        if (sr.passed) {
          scripts_passed++;
        } else {
          scripts_failed++;
          failed_names.push_back(sr.name);
        }
      }
      
      printf("Scripts run:    %zu\n", script_results_.size());
      printf("Scripts passed: %d\n", scripts_passed);
      printf("Scripts failed: %d\n", scripts_failed);
      
      if (!failed_names.empty()) {
        printf("\nFailed tests:\n");
        for (const auto& name : failed_names) {
          printf("  - %s\n", name.c_str());
        }
      }
      
      printf("\n");
      return;
    }
    
    // Single script mode - show validation details
    int passed = 0, failed_count = 0;
    for (const auto& r : results_) {
      if (r.success) passed++;
      else {
        failed_count++;
        printf("[FAIL] Line %d: %s: expected '%s', got '%s'\n",
               r.line_number, r.property.c_str(), r.expected.c_str(), r.actual.c_str());
      }
    }
    if (!errors_.empty()) {
      printf("\nErrors: %zu\n", errors_.size());
      for (const auto& e : errors_) printf("  Line %d: %s\n", e.line_number, e.message.c_str());
    }
    if (timed_out_) printf("[TIMEOUT] after %d frames\n", frame_count_);
    printf("E2E Results: %d passed, %d failed (%d frames)\n", passed, failed_count, frame_count_);
  }

private:
  void execute(const TestCommand& cmd) {
    switch (cmd.type) {
      case CommandType::Type:
        for (char c : cmd.arg1) test_input::push_char(c);
        wait_frames_ = static_cast<int>(cmd.arg1.size()) + 2;
        break;
        
      case CommandType::Key: {
        auto combo = parse_key_combo(cmd.arg1);
        if (combo.ctrl) input_injector::set_key_down(341);  // KEY_LEFT_CONTROL
        if (combo.shift) input_injector::set_key_down(340); // KEY_LEFT_SHIFT
        if (combo.alt) input_injector::set_key_down(342);   // KEY_LEFT_ALT
        // Use input_injector for main key too (for ActionMap to detect it)
        input_injector::set_key_down(combo.key);
        // Also push to queue for legacy handlers that use test_input::is_key_pressed
        test_input::push_key(combo.key);
        // Store key info for release after wait
        pending_key_release_ = true;
        pending_key_ = combo.key;
        pending_ctrl_ = combo.ctrl;
        pending_shift_ = combo.shift;
        pending_alt_ = combo.alt;
        wait_frames_ = 2;
        break;
      }
      
      case CommandType::Click:
        test_input::simulate_click(static_cast<float>(cmd.x), static_cast<float>(cmd.y));
        wait_frames_ = 1;
        pending_release_ = true;
        break;
        
      case CommandType::MouseMove:
        test_input::set_mouse_position(static_cast<float>(cmd.x), static_cast<float>(cmd.y));
        wait_frames_ = 1;
        break;
        
      case CommandType::Wait:
        wait_frames_ = cmd.x;
        break;
        
      case CommandType::Validate:
        if (property_getter_) {
          ValidationResult r;
          r.property = cmd.arg1; r.expected = cmd.arg2; r.line_number = cmd.line_number;
          r.actual = property_getter_(cmd.arg1);
          r.success = (r.actual == r.expected);
          if (!r.success) {
            failed_ = true;
            current_script_validation_failures_++;
          }
          results_.push_back(r);
        }
        break;
        
      case CommandType::ExpectText: {
        ValidationResult r;
        r.property = "visible_text"; r.expected = cmd.arg1; r.line_number = cmd.line_number;
        r.success = visible_text::contains(cmd.arg1);
        r.actual = r.success ? cmd.arg1 : visible_text::get_all().substr(0, 200);
        if (!r.success) {
          failed_ = true;
          current_script_validation_failures_++;
        }
        results_.push_back(r);
        break;
      }
      
      case CommandType::Screenshot:
        if (screenshot_fn_) screenshot_fn_(cmd.arg1);
        break;
        
      case CommandType::Clear:
        // Finalize current script results before clearing
        if (!script_results_.empty() && current_script_idx_ < script_results_.size()) {
          auto& result = script_results_[current_script_idx_];
          result.error_count = current_script_errors_;
          result.validation_failures = current_script_validation_failures_;
          
          // A script "passed" if no errors/failures occurred
          bool actually_passed = (current_script_errors_ == 0 && current_script_validation_failures_ == 0);
          
          // Check against expectation
          if (result.expected_to_pass) {
            result.passed = actually_passed;
          } else {
            // Expected to fail: we "pass" the test if it actually failed
            result.passed = !actually_passed;
          }
          
          // Print per-script result
          if (result.passed) {
            printf("[PASS] %s\n", result.name.c_str());
          } else {
            if (result.expected_to_pass) {
              printf("[FAIL] %s (expected pass, got %d errors, %d validation failures)\n", 
                     result.name.c_str(), current_script_errors_, current_script_validation_failures_);
            } else {
              printf("[FAIL] %s (expected fail, but passed)\n", result.name.c_str());
            }
          }
          
          // Move to next script
          current_script_idx_++;
          current_script_errors_ = 0;
          current_script_validation_failures_ = 0;
        }
        
        if (clear_fn_) clear_fn_();
        script_start_ = frame_count_;
        wait_frames_ = 2;
        break;
        
      case CommandType::MenuOpen:
        if (menu_opener_ && !menu_opener_(cmd.arg1)) report_error(cmd, "Failed to open menu: " + cmd.arg1);
        wait_frames_ = 2;
        break;
        
      case CommandType::MenuSelect:
        if (menu_selector_ && !menu_selector_(cmd.arg1)) report_error(cmd, "Failed to select: " + cmd.arg1);
        wait_frames_ = 2;
        break;
        
      case CommandType::Unknown:
        report_error(cmd, "Unknown command: " + cmd.arg1);
        break;
        
      case CommandType::DoubleClick:
        // Double-click: two clicks in quick succession
        test_input::simulate_click(static_cast<float>(cmd.x), static_cast<float>(cmd.y));
        // TODO: Queue second click after release
        wait_frames_ = 3;
        pending_release_ = true;
        break;
        
      case CommandType::Drag:
        // Start drag at (x,y), end at (x2,y2)
        test_input::simulate_click(static_cast<float>(cmd.x), static_cast<float>(cmd.y));
        // Move to end position (simplified - full impl would animate)
        input_injector::set_mouse_position(static_cast<float>(cmd.x2), static_cast<float>(cmd.y2));
        wait_frames_ = 5;
        pending_release_ = true;
        break;
        
      case CommandType::Comment:
        // Comments are ignored
        break;
        
      default:
        // Unhandled command type
        break;
    }
  }
  
  void report_error(const TestCommand& cmd, const std::string& msg) {
    ScriptError err;
    err.line_number = cmd.line_number;
    err.command = cmd.arg1;
    err.message = msg;
    errors_.push_back(err);
    failed_ = true;
    current_script_errors_++;
    printf("[ERROR] Line %d: %s\n", cmd.line_number, msg.c_str());
  }
  
  std::vector<TestCommand> commands_;
  std::string script_path_;
  std::size_t index_ = 0;
  int wait_frames_ = 0, frame_count_ = 0, script_start_ = 0;
  int timeout_ = DEFAULT_TIMEOUT;
  bool pending_release_ = false;
  bool pending_key_release_ = false;
  int pending_key_ = 0;
  bool pending_ctrl_ = false, pending_shift_ = false, pending_alt_ = false;
  bool finished_ = false, failed_ = false, timed_out_ = false;
  bool debug_overlay_ = false;
  
  std::vector<ValidationResult> results_;
  std::vector<ScriptError> errors_;
  std::function<std::string(const std::string&)> property_getter_;
  std::function<void(const std::string&)> screenshot_fn_;
  std::function<void()> clear_fn_;
  std::function<bool(const std::string&)> menu_opener_;
  std::function<bool(const std::string&)> menu_selector_;
  std::function<void(const std::string&)> document_dumper_;
  std::function<bool(const std::string&)> outline_clicker_;
  
  // Per-script tracking for batch mode
  std::vector<ScriptResult> script_results_;
  std::vector<std::size_t> script_boundaries_;  // Command index where each script starts
  std::size_t current_script_idx_ = 0;
  int current_script_errors_ = 0;
  int current_script_validation_failures_ = 0;
};

} // namespace testing
} // namespace afterhours
