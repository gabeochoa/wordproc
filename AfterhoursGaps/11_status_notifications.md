# Status Notifications

## Working Implementation
See these files for a complete working example:
- `src/extracted/status_notifications.h` - Clean, standalone implementation ready for PR
- `src/ecs/components.h` - StatusComponent (simpler version)
- `src/ecs/component_helpers.h` - status::set(), status::hasMessage()

## Problem
Afterhours does not provide a notification/toast system for temporary status messages.

## Use Cases
- **Game feedback**: "Checkpoint saved!", "Achievement unlocked!"
- **App status**: "File saved", "Error: Could not connect"
- **Tutorials**: Timed hints and tips
- **Debugging**: On-screen debug messages with auto-dismiss

## Current Workaround
Custom `StatusComponent` in `src/ecs/components.h`:

```cpp
struct StatusComponent : public afterhours::BaseComponent {
    std::string text;
    double expiresAt = 0.0;
    bool isError = false;
};
```

## Proposed API

```cpp
namespace afterhours {

enum class NotificationLevel {
  Info,     // Normal message
  Success,  // Positive feedback (green)
  Warning,  // Warning (yellow/orange)
  Error,    // Error (red)
};

struct Notification {
  std::string message;
  NotificationLevel level;
  double duration;
  
  static Notification info(const std::string& msg, double duration = 3.0);
  static Notification success(const std::string& msg, double duration = 3.0);
  static Notification warning(const std::string& msg, double duration = 5.0);
  static Notification error(const std::string& msg, double duration = 7.0);
};

struct ProvidesNotifications : BaseComponent {
  void push(Notification notif, double current_time);
  void info(const std::string& msg, double current_time);
  void success(const std::string& msg, double current_time);
  void warning(const std::string& msg, double current_time);
  void error(const std::string& msg, double current_time);
  
  void cleanup(double current_time);  // Remove expired
  std::vector<const Notification*> get_visible(double current_time) const;
  bool has_visible(double current_time) const;
};

// Static API for easy access
namespace notifications {
  void info(const std::string& msg);
  void success(const std::string& msg);
  void warning(const std::string& msg);
  void error(const std::string& msg);
}

} // namespace afterhours
```

## Usage Example

```cpp
// Setup
add_notifications_component(singleton_entity);

// Show notifications from anywhere
notifications::success("Game saved!");
notifications::error("Failed to load level");

// In render system
if (auto* provider = notifications::get_provider()) {
  double time = GetTime();
  int y = 10;
  for (const auto* n : provider->get_visible(time)) {
    Color c = (n->level == NotificationLevel::Error) ? RED : WHITE;
    DrawText(n->message.c_str(), 10, y, 20, c);
    y += 25;
  }
}
```

## Notes
- Auto-dismissing messages reduce UI clutter
- Different durations for different severity levels
- Queue supports multiple simultaneous notifications
- Could be extended with animations, positioning, stacking

