# E2E Testing Framework

## Working Implementation
See these files for a complete working example:
- `src/testing/e2e_script.h` - Script parser and runner (main file)
- `src/testing/e2e_runner.h/.cpp` - Runner initialization helpers
- `src/testing/test_input.h/.cpp` - Input injection layer
- `src/testing/input_injector.h/.cpp` - Low-level input simulation
- `src/testing/test_input_provider.h` - Afterhours UIContext integration
- `src/testing/visible_text_registry.h` - Track rendered text for assertions
- `tests/e2e_scripts/*.e2e` - 67 example test scripts

Extracted clean version:
- `src/extracted/e2e_testing.h` - Standalone E2E framework ready for PR

## Problem
Afterhours does not provide an E2E testing framework for UI applications.
Testing UI-heavy games and apps requires manual testing or complex custom solutions.

## Use Cases
- **Games**: Test menu navigation, settings screens, tutorials
- **Level editors**: Test tool interactions, save/load
- **Productivity apps**: Test document editing, dialogs
- **CI/CD**: Automated regression testing with screenshots

## Features of This Framework

### Simple Script DSL
Test scripts are plain text `.e2e` files:

```
# Test: Basic menu interaction
wait 5
click 28 34
expect_text "New"
screenshot menu_open
```

### Commands
| Command | Description | Example |
|---------|-------------|---------|
| `type "text"` | Type text | `type "Hello World"` |
| `key COMBO` | Press key combo | `key CTRL+S` |
| `click x y` | Click at position | `click 100 200` |
| `double_click x y` | Double-click | `double_click 100 200` |
| `drag x1 y1 x2 y2` | Drag mouse | `drag 10 10 100 100` |
| `wait N` | Wait N frames | `wait 5` |
| `validate prop=val` | Check property | `validate text=Hello` |
| `expect_text "text"` | Check visible text | `expect_text "Save"` |
| `screenshot name` | Take screenshot | `screenshot result` |
| `menu_open "name"` | Open menu | `menu_open "File"` |
| `menu_select "item"` | Select menu item | `menu_select "Save"` |
| `clear` | Reset document | `clear` |
| `# comment` | Comment line | `# This is a comment` |

### Input Injection
Works at two levels:
1. **Raw raylib**: Intercepts `IsKeyPressed`, `GetMousePosition`, etc.
2. **Afterhours UIContext**: Injects into `context.mouse`, `context.last_action`

### Visible Text Registry
Render systems register text they draw:
```cpp
void drawText(const char* text, int x, int y) {
    raylib::DrawText(text, x, y, 16, WHITE);
    test_input::registerVisibleText(text);  // Register for E2E
}
```

Tests can then assert:
```
expect_text "File saved"
```

### Batch Mode
Run all `.e2e` files in a directory with automatic cleanup between tests:
```cpp
runner.loadScriptsFromDirectory("tests/e2e_scripts/");
```

### Timeout Handling
Scripts timeout after configurable frame count (default 600 = ~10s at 60fps).

## Proposed API for Afterhours

```cpp
namespace afterhours::testing {

// Script runner
class E2ERunner {
public:
  void load_script(const std::string& path);
  void load_scripts_from_directory(const std::string& dir);
  
  void set_timeout_frames(int frames);
  void set_screenshot_callback(std::function<void(const std::string&)>);
  void set_property_getter(std::function<std::string(const std::string&)>);
  
  void tick();  // Call each frame
  
  bool is_finished() const;
  bool has_failed() const;
  void print_results() const;
};

// Input injection
namespace test_input {
  void set_test_mode(bool enabled);
  void push_key(int key);
  void push_char(char c);
  void set_mouse_position(float x, float y);
  void simulate_click(float x, float y);
  void reset_frame();  // Call at start of each frame
}

// Visible text tracking
namespace visible_text {
  void register_text(const std::string& text);
  void clear();
  bool contains(const std::string& text);
}

// UIContext integration (system that injects test input)
template<typename InputAction>
struct TestInputSystem : System<UIContext<InputAction>> {
  void for_each_with(...) override;
};

} // namespace afterhours::testing
```

## Usage Example

```cpp
// In main.cpp
#include <afterhours/src/plugins/testing.h>

int main() {
  // ... normal setup ...
  
  if (args.contains("--e2e")) {
    testing::E2ERunner runner;
    runner.load_script(args.get("--e2e"));
    runner.set_screenshot_callback([](const std::string& name) {
      TakeScreenshot((name + ".png").c_str());
    });
    
    testing::test_input::set_test_mode(true);
    
    while (!WindowShouldClose() && !runner.is_finished()) {
      testing::test_input::reset_frame();
      runner.tick();
      // ... normal update/render ...
    }
    
    runner.print_results();
    return runner.has_failed() ? 1 : 0;
  }
  
  // ... normal game loop ...
}
```

## Benefits
- **Simple DSL**: Non-programmers can write tests
- **Screenshot comparison**: Visual regression testing
- **CI-friendly**: Exit codes, timeout handling
- **Incremental**: Add tests as features are built
- **Cross-platform**: Works anywhere raylib works

