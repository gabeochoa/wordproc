// E2E Testing Framework for Afterhours
// Provides script-based UI testing with input injection and text assertions.
//
// Features:
// - Simple DSL for test scripts (.e2e files)
// - Input injection (keyboard, mouse)
// - Visible text assertions (expect_text)
// - Screenshot capture
// - Batch mode for multiple scripts
// - Timeout handling
//
// To integrate into Afterhours:
// 1. Add this as src/plugins/testing.h
// 2. Games include it when building with tests enabled

#pragma once

#include <algorithm>
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

//=============================================================================
// INPUT INJECTION
//=============================================================================

namespace test_input {

/// Represents a key press or character input
struct KeyPress {
  int key = 0;
  bool is_char = false;
  char char_value = 0;
};

/// Mouse simulation state
struct MouseState {
  std::optional<float> x, y;
  bool left_down = false;
  bool left_pressed = false;
  bool left_released = false;
  bool active = false;
};

/// Global test input state
inline std::queue<KeyPress> g_key_queue;
inline MouseState g_mouse;
inline bool g_test_mode = false;
inline bool g_key_consumed = false;
inline bool g_char_consumed = false;

/// Enable/disable test mode
inline void set_test_mode(bool enabled) { g_test_mode = enabled; }
inline bool is_test_mode() { return g_test_mode; }

/// Queue a key press
inline void push_key(int key) {
  KeyPress press;
  press.key = key;
  press.is_char = false;
  g_key_queue.push(press);
}

/// Queue a character
inline void push_char(char c) {
  KeyPress press;
  press.is_char = true;
  press.char_value = c;
  g_key_queue.push(press);
}

/// Set mouse position
inline void set_mouse_position(float x, float y) {
  g_mouse.x = x;
  g_mouse.y = y;
  g_mouse.active = true;
}

/// Simulate mouse click
inline void simulate_click(float x, float y) {
  set_mouse_position(x, y);
  g_mouse.left_down = true;
  g_mouse.left_pressed = true;
}

/// Release mouse
inline void release_mouse() {
  g_mouse.left_down = false;
  g_mouse.left_released = true;
}

/// Call at start of each frame
inline void reset_frame() {
  g_key_consumed = false;
  g_char_consumed = false;
  g_mouse.left_pressed = false;
  g_mouse.left_released = false;
}

/// Clear all queued input
inline void clear() {
  while (!g_key_queue.empty()) g_key_queue.pop();
  g_mouse = MouseState{};
}

//-----------------------------------------------------------------------------
// Input query wrappers - use these instead of raw raylib calls in test mode
//-----------------------------------------------------------------------------

/// Check if key was pressed (use instead of IsKeyPressed)
template<typename RealFn>
inline bool is_key_pressed(int key, RealFn real_fn) {
  if (!g_test_mode || g_key_queue.empty() || g_key_consumed) {
    return real_fn(key);
  }
  if (!g_key_queue.front().is_char && g_key_queue.front().key == key) {
    g_key_queue.pop();
    g_key_consumed = true;
    return true;
  }
  return real_fn(key);
}

/// Get pressed character (use instead of GetCharPressed)
template<typename RealFn>
inline int get_char_pressed(RealFn real_fn) {
  if (!g_test_mode || g_key_queue.empty() || g_char_consumed) {
    return real_fn();
  }
  if (g_key_queue.front().is_char) {
    char c = g_key_queue.front().char_value;
    g_key_queue.pop();
    g_char_consumed = true;
    return static_cast<int>(c);
  }
  return real_fn();
}

/// Get mouse position (use instead of GetMousePosition)
template<typename Vec2, typename RealFn>
inline Vec2 get_mouse_position(RealFn real_fn) {
  if (g_test_mode && g_mouse.active && g_mouse.x && g_mouse.y) {
    return Vec2{*g_mouse.x, *g_mouse.y};
  }
  return real_fn();
}

/// Check mouse button pressed (use instead of IsMouseButtonPressed)
template<typename RealFn>
inline bool is_mouse_button_pressed(int button, RealFn real_fn) {
  if (g_test_mode && g_mouse.active && button == 0) {
    return g_mouse.left_pressed;
  }
  return real_fn(button);
}

/// Check mouse button down (use instead of IsMouseButtonDown)
template<typename RealFn>
inline bool is_mouse_button_down(int button, RealFn real_fn) {
  if (g_test_mode && g_mouse.active && button == 0) {
    return g_mouse.left_down;
  }
  return real_fn(button);
}

} // namespace test_input

//=============================================================================
// VISIBLE TEXT REGISTRY
//=============================================================================

namespace visible_text {

/// Singleton registry for tracking rendered text
class Registry {
public:
  static Registry& instance() {
    static Registry inst;
    return inst;
  }
  
  void clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    texts_.clear();
  }
  
  void register_text(const std::string& text) {
    if (text.empty()) return;
    std::lock_guard<std::mutex> lock(mutex_);
    texts_.push_back(text);
  }
  
  bool contains(const std::string& needle) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& t : texts_) {
      if (t.find(needle) != std::string::npos) return true;
    }
    return false;
  }
  
  std::string get_all() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string result;
    for (const auto& t : texts_) {
      if (!result.empty()) result += " | ";
      result += t;
    }
    return result;
  }

private:
  Registry() = default;
  mutable std::mutex mutex_;
  std::vector<std::string> texts_;
};

inline void clear() { Registry::instance().clear(); }
inline void register_text(const std::string& t) { Registry::instance().register_text(t); }
inline bool contains(const std::string& t) { return Registry::instance().contains(t); }

} // namespace visible_text

//=============================================================================
// E2E SCRIPT RUNNER
//=============================================================================

/// Command types
enum class CommandType {
  Type, Key, Click, DoubleClick, Drag, Wait,
  Validate, ExpectText, Screenshot, Clear, Comment, Unknown
};

/// A single test command
struct TestCommand {
  CommandType type = CommandType::Unknown;
  std::string arg1, arg2;
  int x = 0, y = 0, x2 = 0, y2 = 0;
  int line_number = 0;
};

/// Validation result
struct ValidationResult {
  bool success = true;
  std::string property, expected, actual, message;
  int line_number = 0;
};

/// Key combo (modifiers + key)
struct KeyCombo {
  bool ctrl = false, shift = false, alt = false;
  int key = 0;
};

/// Parse key string like "CTRL+S"
inline KeyCombo parse_key_combo(const std::string& str) {
  KeyCombo combo;
  std::string s = str;
  
  auto has_prefix = [&](const std::string& p) {
    if (s.find(p) == 0) { s = s.substr(p.length()); return true; }
    return false;
  };
  
  while (true) {
    if (has_prefix("CTRL+") || has_prefix("CMD+")) combo.ctrl = true;
    else if (has_prefix("SHIFT+")) combo.shift = true;
    else if (has_prefix("ALT+")) combo.alt = true;
    else break;
  }
  
  // Map key names to codes (raylib KEY_* values)
  if (s.length() == 1 && s[0] >= 'A' && s[0] <= 'Z') {
    combo.key = s[0]; // ASCII = raylib key for A-Z
  } else if (s == "ENTER") combo.key = 257;
  else if (s == "ESCAPE" || s == "ESC") combo.key = 256;
  else if (s == "TAB") combo.key = 258;
  else if (s == "BACKSPACE") combo.key = 259;
  else if (s == "DELETE") combo.key = 261;
  else if (s == "LEFT") combo.key = 263;
  else if (s == "RIGHT") combo.key = 262;
  else if (s == "UP") combo.key = 265;
  else if (s == "DOWN") combo.key = 264;
  // Add more as needed
  
  return combo;
}

/// Parse script file into commands
inline std::vector<TestCommand> parse_script(const std::string& path) {
  std::vector<TestCommand> cmds;
  std::ifstream file(path);
  if (!file.is_open()) return cmds;
  
  std::string line;
  int line_num = 0;
  
  while (std::getline(file, line)) {
    line_num++;
    
    // Trim
    size_t start = line.find_first_not_of(" \t");
    if (start == std::string::npos) continue;
    line = line.substr(start);
    if (line.empty() || line[0] == '#') continue;
    
    TestCommand cmd;
    cmd.line_number = line_num;
    
    std::istringstream iss(line);
    std::string verb;
    iss >> verb;
    
    if (verb == "type") {
      cmd.type = CommandType::Type;
      std::getline(iss >> std::ws, cmd.arg1);
      // Remove quotes
      if (cmd.arg1.front() == '"') cmd.arg1 = cmd.arg1.substr(1);
      if (cmd.arg1.back() == '"') cmd.arg1.pop_back();
    } else if (verb == "key") {
      cmd.type = CommandType::Key;
      iss >> cmd.arg1;
    } else if (verb == "click") {
      cmd.type = CommandType::Click;
      iss >> cmd.x >> cmd.y;
    } else if (verb == "double_click") {
      cmd.type = CommandType::DoubleClick;
      iss >> cmd.x >> cmd.y;
    } else if (verb == "drag") {
      cmd.type = CommandType::Drag;
      iss >> cmd.x >> cmd.y >> cmd.x2 >> cmd.y2;
    } else if (verb == "wait") {
      cmd.type = CommandType::Wait;
      iss >> cmd.x; // Reuse x for frame count
      if (cmd.x <= 0) cmd.x = 1;
    } else if (verb == "validate") {
      cmd.type = CommandType::Validate;
      std::string rest;
      std::getline(iss >> std::ws, rest);
      size_t eq = rest.find('=');
      if (eq != std::string::npos) {
        cmd.arg1 = rest.substr(0, eq);
        cmd.arg2 = rest.substr(eq + 1);
      }
    } else if (verb == "expect_text") {
      cmd.type = CommandType::ExpectText;
      std::getline(iss >> std::ws, cmd.arg1);
      if (cmd.arg1.front() == '"') cmd.arg1 = cmd.arg1.substr(1);
      if (cmd.arg1.back() == '"') cmd.arg1.pop_back();
    } else if (verb == "screenshot") {
      cmd.type = CommandType::Screenshot;
      iss >> cmd.arg1;
    } else if (verb == "clear") {
      cmd.type = CommandType::Clear;
    } else {
      cmd.type = CommandType::Unknown;
      cmd.arg1 = verb;
    }
    
    cmds.push_back(cmd);
  }
  
  return cmds;
}

/// E2E Script Runner
class E2ERunner {
public:
  static constexpr int DEFAULT_TIMEOUT = 600; // ~10s at 60fps
  
  void load_script(const std::string& path) {
    commands_ = parse_script(path);
    reset();
  }
  
  void load_scripts_from_directory(const std::string& dir) {
    commands_.clear();
    reset();
    
    std::vector<std::string> scripts;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
      if (entry.path().extension() == ".e2e") {
        scripts.push_back(entry.path().string());
      }
    }
    std::sort(scripts.begin(), scripts.end());
    
    for (size_t i = 0; i < scripts.size(); ++i) {
      auto script_cmds = parse_script(scripts[i]);
      for (auto& cmd : script_cmds) commands_.push_back(cmd);
      
      // Add clear between scripts
      if (i < scripts.size() - 1) {
        TestCommand clear_cmd;
        clear_cmd.type = CommandType::Clear;
        commands_.push_back(clear_cmd);
      }
    }
  }
  
  void reset() {
    index_ = 0;
    wait_frames_ = 0;
    frame_count_ = 0;
    results_.clear();
    finished_ = false;
    failed_ = false;
    timed_out_ = false;
  }
  
  void set_timeout_frames(int frames) { timeout_ = frames; }
  
  void set_property_getter(std::function<std::string(const std::string&)> fn) {
    property_getter_ = std::move(fn);
  }
  
  void set_screenshot_callback(std::function<void(const std::string&)> fn) {
    screenshot_fn_ = std::move(fn);
  }
  
  void set_clear_callback(std::function<void()> fn) {
    clear_fn_ = std::move(fn);
  }
  
  void tick() {
    if (finished_ || commands_.empty()) return;
    
    frame_count_++;
    
    // Check timeout
    if (timeout_ > 0 && frame_count_ > timeout_) {
      timed_out_ = true;
      failed_ = true;
      finished_ = true;
      return;
    }
    
    // Handle wait
    if (wait_frames_ > 0) {
      wait_frames_--;
      
      // Release mouse if pending
      if (wait_frames_ == 0 && pending_release_) {
        test_input::release_mouse();
        pending_release_ = false;
        wait_frames_ = 2;
      }
      return;
    }
    
    if (index_ >= commands_.size()) {
      finished_ = true;
      return;
    }
    
    const auto& cmd = commands_[index_];
    execute_command(cmd);
    index_++;
    
    if (index_ >= commands_.size()) {
      finished_ = true;
    }
  }
  
  bool is_finished() const { return finished_; }
  bool has_failed() const { return failed_; }
  bool has_timed_out() const { return timed_out_; }
  int frame_count() const { return frame_count_; }
  const std::vector<ValidationResult>& results() const { return results_; }
  
  void print_results() const {
    int passed = 0, failed = 0;
    for (const auto& r : results_) {
      if (r.success) passed++;
      else {
        failed++;
        printf("[FAIL] Line %d: %s: expected '%s', got '%s'\n",
               r.line_number, r.property.c_str(), 
               r.expected.c_str(), r.actual.c_str());
      }
    }
    if (timed_out_) {
      printf("[TIMEOUT] after %d frames\n", frame_count_);
    }
    printf("E2E Results: %d passed, %d failed (%d frames)\n",
           passed, failed, frame_count_);
  }

private:
  void execute_command(const TestCommand& cmd) {
    switch (cmd.type) {
      case CommandType::Type:
        for (char c : cmd.arg1) test_input::push_char(c);
        wait_frames_ = static_cast<int>(cmd.arg1.size()) + 2;
        break;
        
      case CommandType::Key: {
        auto combo = parse_key_combo(cmd.arg1);
        // Set modifier keys down, push main key
        if (combo.ctrl) test_input::push_key(341); // KEY_LEFT_CONTROL
        if (combo.shift) test_input::push_key(340); // KEY_LEFT_SHIFT
        if (combo.alt) test_input::push_key(342); // KEY_LEFT_ALT
        test_input::push_key(combo.key);
        wait_frames_ = 2;
        break;
      }
      
      case CommandType::Click:
        test_input::simulate_click(static_cast<float>(cmd.x), 
                                   static_cast<float>(cmd.y));
        wait_frames_ = 1;
        pending_release_ = true;
        break;
        
      case CommandType::Wait:
        wait_frames_ = cmd.x;
        break;
        
      case CommandType::Validate:
        if (property_getter_) {
          ValidationResult r;
          r.property = cmd.arg1;
          r.expected = cmd.arg2;
          r.line_number = cmd.line_number;
          r.actual = property_getter_(cmd.arg1);
          r.success = (r.actual == r.expected);
          if (!r.success) failed_ = true;
          results_.push_back(r);
        }
        break;
        
      case CommandType::ExpectText: {
        ValidationResult r;
        r.property = "visible_text";
        r.expected = cmd.arg1;
        r.line_number = cmd.line_number;
        r.success = visible_text::contains(cmd.arg1);
        r.actual = r.success ? cmd.arg1 : visible_text::Registry::instance().get_all();
        if (!r.success) failed_ = true;
        results_.push_back(r);
        break;
      }
      
      case CommandType::Screenshot:
        if (screenshot_fn_) screenshot_fn_(cmd.arg1);
        break;
        
      case CommandType::Clear:
        if (clear_fn_) clear_fn_();
        wait_frames_ = 2;
        break;
        
      default:
        break;
    }
  }
  
  std::vector<TestCommand> commands_;
  std::size_t index_ = 0;
  int wait_frames_ = 0;
  int frame_count_ = 0;
  int timeout_ = DEFAULT_TIMEOUT;
  bool pending_release_ = false;
  bool finished_ = false;
  bool failed_ = false;
  bool timed_out_ = false;
  
  std::vector<ValidationResult> results_;
  std::function<std::string(const std::string&)> property_getter_;
  std::function<void(const std::string&)> screenshot_fn_;
  std::function<void()> clear_fn_;
};

} // namespace testing
} // namespace afterhours

//=============================================================================
// USAGE EXAMPLE
//=============================================================================
/*
#include "e2e_testing.h"

int main(int argc, char** argv) {
  using namespace afterhours::testing;
  
  // Parse args for --e2e flag
  std::string e2e_script;
  for (int i = 1; i < argc; i++) {
    if (std::string(argv[i]) == "--e2e" && i + 1 < argc) {
      e2e_script = argv[i + 1];
    }
  }
  
  InitWindow(800, 600, "My App");
  
  E2ERunner runner;
  if (!e2e_script.empty()) {
    runner.load_script(e2e_script);
    runner.set_screenshot_callback([](const std::string& name) {
      TakeScreenshot((name + ".png").c_str());
    });
    test_input::set_test_mode(true);
  }
  
  while (!WindowShouldClose()) {
    // E2E testing
    test_input::reset_frame();
    visible_text::clear();
    
    if (!e2e_script.empty()) {
      runner.tick();
      if (runner.is_finished()) {
        runner.print_results();
        break;
      }
    }
    
    // Normal update/render
    BeginDrawing();
    ClearBackground(RAYWHITE);
    
    // Register text for E2E assertions
    DrawText("Hello World", 10, 10, 20, BLACK);
    visible_text::register_text("Hello World");
    
    EndDrawing();
  }
  
  CloseWindow();
  return runner.has_failed() ? 1 : 0;
}

// Example .e2e script (tests/example.e2e):
// # Test basic text visibility
// wait 5
// expect_text "Hello World"
// screenshot hello_test
*/

