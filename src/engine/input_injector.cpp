#include "input_injector.h"

#include <array>

#undef IsMouseButtonPressed
#undef IsMouseButtonDown
#undef IsMouseButtonReleased
#undef IsMouseButtonUp
#undef GetMousePosition
#undef IsKeyPressed
#undef GetCharPressed

namespace input_injector {

namespace {

struct PendingClick {
  bool has_pending = false;
  vec2 pos;
};
static PendingClick pending_click;

struct PendingKeyHold {
  bool is_holding = false;
  int keycode = 0;
  float remaining_time = 0.0f;
};
static PendingKeyHold pending_key_hold;

static std::array<bool, 512> synthetic_keys{};
static std::array<int, 512> synthetic_press_count{};
static std::array<int, 512> synthetic_press_delay{};

struct MouseState {
  vec2 position{0, 0};
  bool simulation_active = false;
  bool left_button_held = false;
  bool left_button_pressed_this_frame = false;
  bool left_button_released_this_frame = false;
};
static MouseState mouse_state;

} // namespace

void release_scheduled_click() {
  if (pending_click.has_pending && mouse_state.left_button_held) {
    mouse_state.left_button_held = false;
    mouse_state.left_button_released_this_frame = true;
    pending_click.has_pending = false;
  }
}

void schedule_mouse_click_at(const raylib::Rectangle &rect) {
  vec2 center = {rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f};
  pending_click.has_pending = true;
  pending_click.pos = center;
}

void inject_scheduled_click() {
  if (!pending_click.has_pending) {
    return;
  }

  mouse_state.position = pending_click.pos;
  mouse_state.simulation_active = true;
  mouse_state.left_button_held = true;
  mouse_state.left_button_pressed_this_frame = true;
  raylib::SetMousePosition(static_cast<int>(pending_click.pos.x),
                           static_cast<int>(pending_click.pos.y));
}

void hold_key_for_duration(int keycode, float duration) {
  pending_key_hold.is_holding = true;
  pending_key_hold.keycode = keycode;
  pending_key_hold.remaining_time = duration;
}

void set_key_down(int keycode) {
  if (keycode >= 0 && keycode < static_cast<int>(synthetic_keys.size())) {
    synthetic_keys[static_cast<size_t>(keycode)] = true;
    synthetic_press_count[static_cast<size_t>(keycode)]++;
    synthetic_press_delay[static_cast<size_t>(keycode)] = 1;
  }
}

void set_key_up(int keycode) {
  if (keycode >= 0 && keycode < static_cast<int>(synthetic_keys.size())) {
    synthetic_keys[static_cast<size_t>(keycode)] = false;
  }
}

void inject_key_press(int keycode) { set_key_down(keycode); }

bool consume_synthetic_press(int keycode) {
  if (keycode < 0 ||
      keycode >= static_cast<int>(synthetic_press_count.size())) {
    return false;
  }
  size_t idx = static_cast<size_t>(keycode);
  if (synthetic_press_count[idx] > 0) {
    if (synthetic_press_delay[idx] > 0) {
      synthetic_press_delay[idx]--;
      return false;
    }
    synthetic_press_count[idx]--;
    return true;
  }
  return false;
}

void update_key_hold(float dt) {
  if (!pending_key_hold.is_holding) {
    return;
  }

  pending_key_hold.remaining_time -= dt;
  if (pending_key_hold.remaining_time <= 0.0f) {
    set_key_up(pending_key_hold.keycode);
    pending_key_hold.is_holding = false;
  }
}

bool is_key_synthetically_down(int keycode) {
  if (keycode < 0 || keycode >= static_cast<int>(synthetic_keys.size())) {
    return false;
  }
  return synthetic_keys[static_cast<size_t>(keycode)];
}

void set_mouse_position(int x, int y) {
  mouse_state.position = {static_cast<float>(x), static_cast<float>(y)};
  mouse_state.simulation_active = true;
  raylib::SetMousePosition(x, y);
}

vec2 get_mouse_position() {
  if (mouse_state.simulation_active) {
    return mouse_state.position;
  }
  return raylib::GetMousePosition_Real();
}

bool is_mouse_button_down(int button) {
  if (mouse_state.simulation_active && button == raylib::MOUSE_BUTTON_LEFT) {
    return mouse_state.left_button_held;
  }
  return raylib::IsMouseButtonDown_Real(button);
}

bool is_mouse_button_pressed(int button) {
  if (mouse_state.simulation_active && button == raylib::MOUSE_BUTTON_LEFT) {
    return mouse_state.left_button_pressed_this_frame;
  }
  return raylib::IsMouseButtonPressed_Real(button);
}

bool is_mouse_button_released(int button) {
  if (mouse_state.simulation_active && button == raylib::MOUSE_BUTTON_LEFT) {
    return mouse_state.left_button_released_this_frame;
  }
  return raylib::IsMouseButtonReleased_Real(button);
}

void reset_frame() {
  mouse_state.left_button_pressed_this_frame = false;
  mouse_state.left_button_released_this_frame = false;
}

} // namespace input_injector



