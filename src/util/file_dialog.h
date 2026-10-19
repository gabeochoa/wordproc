#pragma once

#include <afterhours/src/plugins/native_dialogs.h>

namespace file_dialog {
void set_test_path(const std::string& path);
void enable_test_mode();
void reset_test_responses();
afterhours::native_dialogs::Result open_file(const std::vector<std::string>& extensions);
afterhours::native_dialogs::Result save_file(const std::string& default_name,
                                          const std::vector<std::string>& extensions);
}
