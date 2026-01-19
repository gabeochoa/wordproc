# Status Notifications (Toast System)

## ✅ RESOLVED - Afterhours Now Has toast.h

**Status**: This gap has been resolved. Afterhours added `src/plugins/toast.h` which provides a complete toast notification system.

### Migration Complete
Wordproc has been migrated to use the afterhours toast plugin:
- All `status::set()` calls replaced with `toast_notify::info/success/warning/error()`
- Toast systems registered via `ui_imm::registerToastSystems()`
- Status bar now shows persistent doc info; notifications appear as floating toasts

### Afterhours toast.h Features
- **5 notification levels**: Info, Success, Warning, Error, Custom
- **Slide-in animation** with exponential easing
- **Progress bar countdown** indicator
- **Auto-contrast text** based on background color
- **Resolution-aware scaling** (designed for 720p, scales proportionally)
- **Static helpers**: `toast::send_info()`, `send_success()`, `send_warning()`, `send_error()`, `send_custom()`

### Minor Issues Found (Upstream Fix Candidates)

1. **Unused variable warning**:
```cpp
// toast.h:261
int toast_count = 0;  // Set but not used
```

2. **Missing switch cases**:
```cpp
// toast.h:33 - switch on size.dim
// Missing explicit cases for: Dim::None, Dim::Text, Dim::Children
// Has default: case but -Wswitch-enum still warns
```

These are cosmetic/warning issues, not functional problems.

---

## Historical Context (Before Resolution)

### Previous Workaround
Custom `StatusComponent` in `src/ecs/components.h`:

```cpp
struct StatusComponent : public afterhours::BaseComponent {
    std::string text;
    double expiresAt = 0.0;
    bool isError = false;
};
```

### What We Had Proposed
The proposal below influenced the design that was implemented in toast.h:

```cpp
namespace afterhours {

enum class NotificationLevel {
  Info,     // Normal message
  Success,  // Positive feedback (green)
  Warning,  // Warning (yellow/orange)
  Error,    // Error (red),
};

struct Notification {
  std::string message;
  NotificationLevel level;
  double duration;
  double created_at;
  double expires_at;
  
  float progress(double current_time) const;
  
  static Notification info(const std::string& msg, double duration = 3.0);
  static Notification success(const std::string& msg, double duration = 3.0);
  static Notification warning(const std::string& msg, double duration = 5.0);
  static Notification error(const std::string& msg, double duration = 7.0);
};
}
```

The actual implementation in toast.h follows a similar pattern with `Toast` component and static `send_*` helpers.
