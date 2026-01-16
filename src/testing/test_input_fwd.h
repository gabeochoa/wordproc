// Forward declarations for test_input functions
// These are defined in test_input.h which must be included in at least one translation unit

#pragma once

struct test_input_vec2 {
    float x;
    float y;
};

namespace test_input {

// Forward declarations - defined in test_input.h
bool is_mouse_button_pressed(int button);
bool is_mouse_button_down(int button);
bool is_mouse_button_released(int button);
bool is_mouse_button_up(int button);
int get_char_pressed();
bool is_key_pressed(int key);
test_input_vec2 get_mouse_position_fwd();
float get_mouse_wheel_move();

}  // namespace test_input
