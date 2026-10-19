#include "file_dialog.h"
#include <afterhours/src/plugins/e2e_testing/native_dialog_responses.h>

namespace file_dialog {
namespace {
bool test_mode = false;
afterhours::testing::NativeDialogResponses responses;
afterhours::native_dialogs::Queue queue{[](const auto& request) {
    return test_mode ? responses(request) : afterhours::native_dialogs::show_native(request);
}};

afterhours::native_dialogs::Result show(afterhours::native_dialogs::Request request) {
    const auto id = queue.submit(std::move(request));
    queue.process_pending();
    return std::move(*queue.take_result(id));
}
}

void enable_test_mode() { test_mode = true; }
void reset_test_responses() { responses = {}; }

void set_test_path(const std::string& path) {
    if (path.empty()) responses.push(afterhours::native_dialogs::Cancelled{});
    else responses.push(afterhours::native_dialogs::Selected{path});
}

afterhours::native_dialogs::Result open_file(const std::vector<std::string>& extensions) {
    return show({.kind = afterhours::native_dialogs::Kind::OpenFile,
                 .title = "Open Document", .extensions = extensions});
}

afterhours::native_dialogs::Result save_file(const std::string& default_name,
                                          const std::vector<std::string>& extensions) {
    return show({.kind = afterhours::native_dialogs::Kind::SaveFile,
                 .title = "Save Document", .default_name = default_name,
                 .extensions = extensions});
}
}
