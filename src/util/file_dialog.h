#pragma once

#include <string>
#include <vector>

namespace file_dialog {

// Show a native OS file-open dialog.
// |extensions| filters visible files (e.g. {".wpdoc", ".txt", ".md"}).
// Empty list means show all files.
// Returns the selected file path, or empty string if the user cancelled.
// In test mode, returns the queued test path instead of showing a dialog.
std::string open_file(const std::vector<std::string>& extensions = {});

// Show a native OS file-save dialog.
// |default_name| pre-fills the filename field.
// |extensions| filters visible file types.
// Returns the chosen save path, or empty string if the user cancelled.
// In test mode, returns the queued test path instead of showing a dialog.
std::string save_file(const std::string& default_name = "",
                      const std::vector<std::string>& extensions = {});

// Test mode support: queue a path to be returned by the next open/save call
// instead of showing a native dialog. The path is consumed on use.
void set_test_path(const std::string& path);

// Enable/disable test mode (skips native dialogs entirely)
void enable_test_mode();

}  // namespace file_dialog
