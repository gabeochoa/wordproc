#pragma once

#include "../rl.h"
#include <optional>
#include <queue>

namespace test_input {
struct KeyPress {
  int key;
  bool is_char = false;
  char char_value = 0;
};

struct MouseState {
  std::optional<vec2> position;
  bool left_button_held = false;
  bool left_button_pressed_this_frame = false;
  bool left_button_released_this_frame = false;
  bool simulation_active = false;
};

extern std::queue<KeyPress> input_queue;
extern bool test_mode;
extern bool slow_test_mode;
extern MouseState mouse_state;

void push_key(int key);
void push_char(char c);
void clear_queue();
void reset_frame();

bool is_key_pressed(int key);
int get_char_pressed();

void set_mouse_position(vec2 pos);
void simulate_mouse_button_press(int button);
void simulate_mouse_button_release(int button);
void clear_mouse_simulation();

vec2 get_mouse_position();
bool is_mouse_button_pressed(int button);
bool is_mouse_button_down(int button);
bool is_mouse_button_released(int button);
bool is_mouse_button_up(int button);

void simulate_tab();
void simulate_shift_tab();
void simulate_arrow_key(int arrow_key);
void simulate_enter();
void simulate_escape();

extern bool key_consumed_this_frame;
extern bool char_consumed_this_frame;
} // namespace test_input
