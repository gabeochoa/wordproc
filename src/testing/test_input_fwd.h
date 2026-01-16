#pragma once

// Forward declare vec2 for get_mouse_position
// vec2 is typedef'd to raylib::Vector2 in rl.h, but we're before that include
// so we use a struct that's compatible
struct test_input_vec2 {
  float x;
  float y;
};

namespace test_input {
bool is_mouse_button_pressed(int button);
bool is_mouse_button_down(int button);
bool is_mouse_button_released(int button);
bool is_mouse_button_up(int button);
int get_char_pressed();
bool is_key_pressed(int key);
test_input_vec2 get_mouse_position_fwd();
} // namespace test_input
