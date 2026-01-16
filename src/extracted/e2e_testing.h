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
// LAYER 1: LOW-LEVEL INPUT INJECTOR
//=============================================================================

namespace input_injector {

namespace detail {
  inline std::array<bool, 512> synthetic_keys{};
  inline std::array<int, 512> synthetic_press_count{};
  inline std::array<int, 512> synthetic_press_delay{};
  
  struct MouseState {
    float x = 0, y = 0;
    bool active = false;
    bool left_held = false;
    bool left_pressed = false;
    bool left_released = false;
  };
  inline MouseState mouse;
  
  struct PendingClick {
    bool pending = false;
    float x = 0, y = 0;
  };
  inline PendingClick pending_click;
  
  struct KeyHold {
    bool active = false;
    int keycode = 0;
    float remaining = 0.0f;
  };
  inline KeyHold key_hold;
}

/// Set a key as synthetically held down
inline void set_key_down(int key) {
  if (key >= 0 && key < 512) {
    detail::synthetic_keys[key] = true;
    detail::synthetic_press_count[key]++;
    detail::synthetic_press_delay[key] = 1; // Delay 1 frame before consumable
  }
}

/// Release a synthetically held key
inline void set_key_up(int key) {
  if (key >= 0 && key < 512) {
    detail::synthetic_keys[key] = false;
  }
}

/// Check if key is synthetically held
inline bool is_key_down(int key) {
  return key >= 0 && key < 512 && detail::synthetic_keys[key];
}

/// Consume a synthetic key press (returns true once per press)
inline bool consume_press(int key) {
  if (key < 0 || key >= 512) return false;
  if (detail::synthetic_press_count[key] > 0) {
    if (detail::synthetic_press_delay[key] > 0) {
      detail::synthetic_press_delay[key]--;
      return false;
    }
    detail::synthetic_press_count[key]--;
    return true;
  }
  return false;
}

/// Hold a key for specified duration (seconds)
inline void hold_key_for_duration(int key, float duration) {
  set_key_down(key);
  detail::key_hold = {true, key, duration};
}

/// Update timed key holds (call each frame with delta time)
inline void update_key_hold(float dt) {
  if (detail::key_hold.active) {
    detail::key_hold.remaining -= dt;
    if (detail::key_hold.remaining <= 0) {
      set_key_up(detail::key_hold.keycode);
      detail::key_hold.active = false;
    }
  }
}

/// Set mouse position
inline void set_mouse_position(float x, float y) {
  detail::mouse.x = x;
  detail::mouse.y = y;
  detail::mouse.active = true;
}

/// Get mouse position
inline void get_mouse_position(float& x, float& y) {
  x = detail::mouse.x;
  y = detail::mouse.y;
}

/// Schedule a click at center of rectangle (x, y, w, h)
inline void schedule_click_at(float x, float y, float w, float h) {
  detail::pending_click = {true, x + w/2, y + h/2};
}

/// Execute scheduled click (sets mouse position and pressed state)
inline void inject_scheduled_click() {
  if (detail::pending_click.pending) {
    detail::mouse.x = detail::pending_click.x;
    detail::mouse.y = detail::pending_click.y;
    detail::mouse.active = true;
    detail::mouse.left_held = true;
    detail::mouse.left_pressed = true;
  }
}

/// Release scheduled click
inline void release_scheduled_click() {
  if (detail::pending_click.pending && detail::mouse.left_held) {
    detail::mouse.left_held = false;
    detail::mouse.left_released = true;
    detail::pending_click.pending = false;
  }
}

/// Check mouse button state
inline bool is_mouse_button_pressed() { return detail::mouse.active && detail::mouse.left_pressed; }
inline bool is_mouse_button_down() { return detail::mouse.active && detail::mouse.left_held; }
inline bool is_mouse_button_released() { return detail::mouse.active && detail::mouse.left_released; }

/// Reset per-frame state (call at start of frame)
inline void reset_frame() {
  detail::mouse.left_pressed = false;
  detail::mouse.left_released = false;
}

/// Clear all synthetic input state
inline void reset_all() {
  detail::synthetic_keys.fill(false);
  detail::synthetic_press_count.fill(0);
  detail::synthetic_press_delay.fill(0);
  detail::mouse = {};
  detail::pending_click = {};
  detail::key_hold = {};
}

} // namespace input_injector

//=============================================================================
// LAYER 2: HIGH-LEVEL INPUT QUEUE
//=============================================================================

namespace test_input {

struct KeyPress {
  int key = 0;
  bool is_char = false;
  char char_value = 0;
};

struct MouseState {
  std::optional<float> x, y;
  bool left_held = false;
  bool left_pressed = false;
  bool left_released = false;
  int press_frames = 0;  // Keep press active for N frames
  bool active = false;
};

namespace detail {
  inline std::queue<KeyPress> key_queue;
  inline MouseState mouse;
  inline bool test_mode = false;
  inline bool key_consumed = false;
  inline bool char_consumed = false;
}

/// Enable/disable test mode
inline void set_test_mode(bool enabled) { detail::test_mode = enabled; }
inline bool is_test_mode() { return detail::test_mode; }

/// Queue a key press
inline void push_key(int key) {
  KeyPress kp;
  kp.key = key;
  detail::key_queue.push(kp);
}

/// Queue a character
inline void push_char(char c) {
  KeyPress kp;
  kp.is_char = true;
  kp.char_value = c;
  detail::key_queue.push(kp);
}

/// Clear input queue
inline void clear_queue() {
  while (!detail::key_queue.empty()) detail::key_queue.pop();
}

/// Set mouse position
inline void set_mouse_position(float x, float y) {
  detail::mouse.x = x;
  detail::mouse.y = y;
  detail::mouse.active = true;
  input_injector::set_mouse_position(x, y);
}

/// Simulate mouse press
inline void simulate_mouse_press() {
  detail::mouse.left_held = true;
  detail::mouse.left_pressed = true;
  detail::mouse.press_frames = 1;
  detail::mouse.active = true;
}

/// Simulate mouse release
inline void simulate_mouse_release() {
  detail::mouse.left_held = false;
  detail::mouse.left_released = true;
  detail::mouse.active = true;
}

/// Click at position (press + release on next frame)
inline void simulate_click(float x, float y) {
  set_mouse_position(x, y);
  simulate_mouse_press();
}

/// Reset per-frame state
inline void reset_frame() {
  detail::key_consumed = false;
  detail::char_consumed = false;
  
  if (detail::mouse.press_frames > 0) {
    detail::mouse.press_frames--;
  } else {
    detail::mouse.left_pressed = false;
  }
  detail::mouse.left_released = false;
  
  input_injector::reset_frame();
}

/// Clear all test input state
inline void reset_all() {
  clear_queue();
  detail::mouse = MouseState{};
  input_injector::reset_all();
}

// Convenience helpers
inline void simulate_tab() { push_key(258); }  // KEY_TAB
inline void simulate_enter() { push_key(257); } // KEY_ENTER
inline void simulate_escape() { push_key(256); } // KEY_ESCAPE
inline void simulate_backspace() { push_key(259); } // KEY_BACKSPACE
inline void simulate_arrow_left() { push_key(263); }
inline void simulate_arrow_right() { push_key(262); }
inline void simulate_arrow_up() { push_key(265); }
inline void simulate_arrow_down() { push_key(264); }

//-----------------------------------------------------------------------------
// Input query wrappers (use instead of raw raylib/backend calls)
//-----------------------------------------------------------------------------

/// Check if key pressed (wraps backend call)
template<typename BackendFn>
inline bool is_key_pressed(int key, BackendFn backend_fn) {
  // Check synthetic press first
  if (input_injector::consume_press(key)) return true;
  
  if (!detail::test_mode || detail::key_queue.empty() || detail::key_consumed) {
    return backend_fn(key);
  }
  
  if (!detail::key_queue.front().is_char && detail::key_queue.front().key == key) {
    detail::key_queue.pop();
    detail::key_consumed = true;
    return true;
  }
  return backend_fn(key);
}

/// Get next character (wraps backend call)
template<typename BackendFn>
inline int get_char_pressed(BackendFn backend_fn) {
  if (!detail::test_mode || detail::key_queue.empty() || detail::char_consumed) {
    return backend_fn();
  }
  
  if (detail::key_queue.front().is_char) {
    char c = detail::key_queue.front().char_value;
    detail::key_queue.pop();
    detail::char_consumed = true;
    return static_cast<int>(c);
  }
  return backend_fn();
}

/// Get mouse position (wraps backend call)
template<typename Vec2, typename BackendFn>
inline Vec2 get_mouse_position(BackendFn backend_fn) {
  if (detail::test_mode && detail::mouse.active && detail::mouse.x && detail::mouse.y) {
    return Vec2{*detail::mouse.x, *detail::mouse.y};
  }
  return backend_fn();
}

/// Check mouse button pressed (wraps backend call)
template<typename BackendFn>
inline bool is_mouse_button_pressed(int button, BackendFn backend_fn) {
  if (detail::test_mode && detail::mouse.active && button == 0) {
    return detail::mouse.left_pressed;
  }
  return backend_fn(button);
}

/// Check mouse button down (wraps backend call)
template<typename BackendFn>
inline bool is_mouse_button_down(int button, BackendFn backend_fn) {
  if (detail::test_mode && detail::mouse.active && button == 0) {
    return detail::mouse.left_held;
  }
  return backend_fn(button);
}

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
// LAYER 4: VISIBLE TEXT REGISTRY
//=============================================================================

namespace visible_text {

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
  
  bool has_exact(const std::string& needle) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& t : texts_) {
      if (t == needle) return true;
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
  
  std::vector<std::string> get_texts() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return texts_;
  }

private:
  Registry() = default;
  mutable std::mutex mutex_;
  std::vector<std::string> texts_;
};

inline void clear() { Registry::instance().clear(); }
inline void register_text(const std::string& t) { Registry::instance().register_text(t); }
inline bool contains(const std::string& t) { return Registry::instance().contains(t); }
inline bool has_exact(const std::string& t) { return Registry::instance().has_exact(t); }
inline std::string get_all() { return Registry::instance().get_all(); }

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

struct KeyCombo {
  bool ctrl = false, shift = false, alt = false;
  int key = 0;
};

inline KeyCombo parse_key_combo(const std::string& str) {
  KeyCombo combo;
  std::string s = str;
  
  auto consume = [&](const std::string& p) {
    if (s.find(p) == 0) { s = s.substr(p.length()); return true; }
    return false;
  };
  
  while (true) {
    if (consume("CTRL+") || consume("CMD+")) combo.ctrl = true;
    else if (consume("SHIFT+")) combo.shift = true;
    else if (consume("ALT+")) combo.alt = true;
    else break;
  }
  
  // Map key names to raylib key codes
  if (s.length() == 1 && s[0] >= 'A' && s[0] <= 'Z') combo.key = s[0];
  else if (s == "ENTER") combo.key = 257;
  else if (s == "ESC" || s == "ESCAPE") combo.key = 256;
  else if (s == "TAB") combo.key = 258;
  else if (s == "BACKSPACE") combo.key = 259;
  else if (s == "DELETE") combo.key = 261;
  else if (s == "LEFT") combo.key = 263;
  else if (s == "RIGHT") combo.key = 262;
  else if (s == "UP") combo.key = 265;
  else if (s == "DOWN") combo.key = 264;
  else if (s == "HOME") combo.key = 268;
  else if (s == "END") combo.key = 269;
  else if (s == "PAGEUP") combo.key = 266;
  else if (s == "PAGEDOWN") combo.key = 267;
  
  return combo;
}

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
      if (i < scripts.size() - 1) {
        TestCommand clear_cmd;
        clear_cmd.type = CommandType::Clear;
        commands_.push_back(clear_cmd);
      }
    }
    printf("[BATCH] Loaded %zu scripts with %zu commands\n", scripts.size(), commands_.size());
  }
  
  void reset() {
    index_ = 0; wait_frames_ = 0; frame_count_ = 0; script_start_ = 0;
    results_.clear(); errors_.clear();
    finished_ = failed_ = timed_out_ = pending_release_ = false;
  }
  
  void set_timeout_frames(int frames) { timeout_ = frames; }
  void set_property_getter(std::function<std::string(const std::string&)> fn) { property_getter_ = fn; }
  void set_screenshot_callback(std::function<void(const std::string&)> fn) { screenshot_fn_ = fn; }
  void set_clear_callback(std::function<void()> fn) { clear_fn_ = fn; }
  void set_menu_opener(std::function<bool(const std::string&)> fn) { menu_opener_ = fn; }
  void set_menu_selector(std::function<bool(const std::string&)> fn) { menu_selector_ = fn; }
  
  void tick() {
    if (finished_ || commands_.empty()) return;
    frame_count_++;
    
    // Timeout check
    if (timeout_ > 0 && (frame_count_ - script_start_) > timeout_) {
      timed_out_ = failed_ = finished_ = true;
      ScriptError err; err.command = "timeout";
      err.message = "Timed out after " + std::to_string(timeout_) + " frames";
      errors_.push_back(err);
      return;
    }
    
    // Handle wait
    if (wait_frames_ > 0) {
      wait_frames_--;
      if (wait_frames_ == 0 && pending_release_) {
        test_input::simulate_mouse_release();
        pending_release_ = false;
        wait_frames_ = 2;
      }
      return;
    }
    
    if (index_ >= commands_.size()) { finished_ = true; return; }
    
    execute(commands_[index_]);
    index_++;
    if (index_ >= commands_.size()) finished_ = true;
  }
  
  bool is_finished() const { return finished_; }
  bool has_failed() const { return failed_; }
  bool has_timed_out() const { return timed_out_; }
  int frame_count() const { return frame_count_; }
  const std::vector<ValidationResult>& results() const { return results_; }
  const std::vector<ScriptError>& errors() const { return errors_; }
  
  std::string current_command_desc() const {
    if (finished_ || index_ >= commands_.size()) return "(finished)";
    const auto& cmd = commands_[index_];
    switch (cmd.type) {
      case CommandType::Type: return "type \"" + cmd.arg1 + "\"";
      case CommandType::Key: return "key " + cmd.arg1;
      case CommandType::Click: return "click " + std::to_string(cmd.x) + " " + std::to_string(cmd.y);
      case CommandType::Wait: return "wait " + std::to_string(cmd.x);
      case CommandType::Validate: return "validate " + cmd.arg1 + "=" + cmd.arg2;
      case CommandType::ExpectText: return "expect_text \"" + cmd.arg1 + "\"";
      default: return "(cmd)";
    }
  }
  
  void print_results() const {
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
        test_input::push_key(combo.key);
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
          if (!r.success) failed_ = true;
          results_.push_back(r);
        }
        break;
        
      case CommandType::ExpectText: {
        ValidationResult r;
        r.property = "visible_text"; r.expected = cmd.arg1; r.line_number = cmd.line_number;
        r.success = visible_text::contains(cmd.arg1);
        r.actual = r.success ? cmd.arg1 : visible_text::get_all().substr(0, 200);
        if (!r.success) failed_ = true;
        results_.push_back(r);
        break;
      }
      
      case CommandType::Screenshot:
        if (screenshot_fn_) screenshot_fn_(cmd.arg1);
        break;
        
      case CommandType::Clear:
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
        
      default: break;
    }
  }
  
  void report_error(const TestCommand& cmd, const std::string& msg) {
    ScriptError err;
    err.line_number = cmd.line_number;
    err.command = cmd.arg1;
    err.message = msg;
    errors_.push_back(err);
    failed_ = true;
    printf("[ERROR] Line %d: %s\n", cmd.line_number, msg.c_str());
  }
  
  std::vector<TestCommand> commands_;
  std::string script_path_;
  std::size_t index_ = 0;
  int wait_frames_ = 0, frame_count_ = 0, script_start_ = 0;
  int timeout_ = DEFAULT_TIMEOUT;
  bool pending_release_ = false;
  bool finished_ = false, failed_ = false, timed_out_ = false;
  
  std::vector<ValidationResult> results_;
  std::vector<ScriptError> errors_;
  std::function<std::string(const std::string&)> property_getter_;
  std::function<void(const std::string&)> screenshot_fn_;
  std::function<void()> clear_fn_;
  std::function<bool(const std::string&)> menu_opener_;
  std::function<bool(const std::string&)> menu_selector_;
};

} // namespace testing
} // namespace afterhours
