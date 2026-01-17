# Status Notifications (Toast System)

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

## Reference: Pharmasea Toast System

Pharmasea has a working toast implementation in `src/engine/toastmanager.h` and `src/layers/toastlayer.h`.

### What We Like

**1. Animation support (`pctOpen` + easing):**
```cpp
struct ToastMsg {
    std::string msg;
    AnnouncementType type;
    float timeToShow = 1.f;
    float timeHasShown = 0.f;
    float pctOpen = 1.f;  // Visual progress indicator

    void update(float dt) {
        timeHasShown = fmaxf(timeHasShown + (0.90f * dt), 0.f);
        pctOpen = 1.f - (timeHasShown / timeToShow);
    }
};
```
The `pctOpen` field lets renderers easily animate fades, shrinking progress bars, etc.

**2. Easing for smooth fade-out:**
```cpp
float ease = 1 - reasings::EaseExpoIn(toast.timeHasShown, 0.f, 1.f, toast.timeToShow);
unsigned char alpha = (unsigned char)(255 * ease);
```

**3. Simplicity - global inline vector:**
```cpp
inline std::vector<ToastMsg> TOASTS;
```
Zero ceremony to use from anywhere.

**4. Dedicated rendering layer:**
Clean separation - `ToastLayer` handles its own update and draw.

**5. Delta-time based, not wall-clock:**
Uses `float dt` - more robust for pausing, slow-mo, etc.

### What We Don't Like

1. **No static convenience API** - have to do `TOASTS.push_back({...})` everywhere
2. **Global state** - works but not ECS-idiomatic for Afterhours
3. **No max-visible cap** - could flood UI if spammed

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

Should combine best of both: ECS integration + static API + animation support.

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
  double created_at;
  double expires_at;
  
  // Animation support (from pharmasea)
  float progress(double current_time) const {
    if (current_time >= expires_at) return 0.0f;
    return 1.0f - static_cast<float>((current_time - created_at) / duration);
  }
  
  static Notification info(const std::string& msg, double duration = 3.0);
  static Notification success(const std::string& msg, double duration = 3.0);
  static Notification warning(const std::string& msg, double duration = 5.0);
  static Notification error(const std::string& msg, double duration = 7.0);
};

struct ProvidesNotifications : BaseComponent {
  std::size_t max_visible = 5;  // Cap to prevent UI flood
  
  void push(Notification notif, double current_time);
  void info(const std::string& msg, double current_time);
  void success(const std::string& msg, double current_time);
  void warning(const std::string& msg, double current_time);
  void error(const std::string& msg, double current_time);
  
  void cleanup(double current_time);  // Remove expired
  std::vector<const Notification*> get_visible(double current_time) const;
  bool has_visible(double current_time) const;
};

// Static API for easy access (like we want, unlike pharmasea)
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

// In render system - with animation support
if (auto* provider = notifications::get_provider()) {
  double time = GetTime();
  int y = 10;
  for (const auto* n : provider->get_visible(time)) {
    float progress = n->progress(time);  // 1.0 -> 0.0 as it expires
    unsigned char alpha = (unsigned char)(255 * progress);
    Color c = (n->level == NotificationLevel::Error) ? RED : WHITE;
    c.a = alpha;
    DrawText(n->message.c_str(), 10, y, 20, c);
    y += 25;
  }
}
```

## Notes
- Auto-dismissing messages reduce UI clutter
- Different durations for different severity levels
- Queue supports multiple simultaneous notifications with max cap
- `progress()` method enables easy animation (fade, shrink, etc.)
- Static API for ergonomic usage from anywhere
- ECS-integrated for Afterhours style
