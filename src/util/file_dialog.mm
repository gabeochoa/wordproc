#include "file_dialog.h"

// ── Test mode state ──
namespace file_dialog {
namespace detail {
    static bool test_mode_active = false;
    static std::string queued_path;
}

void set_test_path(const std::string& path) {
    detail::queued_path = path;
}

void enable_test_mode() {
    detail::test_mode_active = true;
}

}  // namespace file_dialog

#ifdef __APPLE__

#import <Cocoa/Cocoa.h>

namespace file_dialog {

std::string open_file(const std::vector<std::string>& extensions) {
    if (detail::test_mode_active) {
        std::string path = detail::queued_path;
        detail::queued_path.clear();
        return path;
    }
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        [panel setCanChooseFiles:YES];
        [panel setCanChooseDirectories:NO];
        [panel setAllowsMultipleSelection:NO];
        [panel setTitle:@"Open Document"];

        // Build allowed file types (strip leading dots)
        if (!extensions.empty()) {
            NSMutableArray<NSString*>* types = [NSMutableArray array];
            for (const auto& ext : extensions) {
                std::string e = ext;
                if (!e.empty() && e[0] == '.') e = e.substr(1);
                [types addObject:[NSString stringWithUTF8String:e.c_str()]];
            }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
            [panel setAllowedFileTypes:types];
#pragma clang diagnostic pop
        }

        if ([panel runModal] == NSModalResponseOK) {
            NSURL* url = [[panel URLs] firstObject];
            if (url) {
                return std::string([[url path] UTF8String]);
            }
        }
        return {};
    }
}

std::string save_file(const std::string& default_name,
                      const std::vector<std::string>& extensions) {
    if (detail::test_mode_active) {
        std::string path = detail::queued_path;
        detail::queued_path.clear();
        return path;
    }
    @autoreleasepool {
        NSSavePanel* panel = [NSSavePanel savePanel];
        [panel setTitle:@"Save Document"];
        [panel setCanCreateDirectories:YES];

        if (!default_name.empty()) {
            [panel setNameFieldStringValue:
                [NSString stringWithUTF8String:default_name.c_str()]];
        }

        // Build allowed file types (strip leading dots)
        if (!extensions.empty()) {
            NSMutableArray<NSString*>* types = [NSMutableArray array];
            for (const auto& ext : extensions) {
                std::string e = ext;
                if (!e.empty() && e[0] == '.') e = e.substr(1);
                [types addObject:[NSString stringWithUTF8String:e.c_str()]];
            }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
            [panel setAllowedFileTypes:types];
#pragma clang diagnostic pop
        }

        if ([panel runModal] == NSModalResponseOK) {
            NSURL* url = [panel URL];
            if (url) {
                return std::string([[url path] UTF8String]);
            }
        }
        return {};
    }
}

}  // namespace file_dialog

#elif defined(__EMSCRIPTEN__)

// Emscripten stub: native dialogs not available, callers fall back to
// text-input modals or other web-specific approaches.
namespace file_dialog {

std::string open_file(const std::vector<std::string>&) {
    if (detail::test_mode_active) {
        std::string path = detail::queued_path;
        detail::queued_path.clear();
        return path;
    }
    return {};
}

std::string save_file(const std::string&,
                      const std::vector<std::string>&) {
    if (detail::test_mode_active) {
        std::string path = detail::queued_path;
        detail::queued_path.clear();
        return path;
    }
    return {};
}

}  // namespace file_dialog

#else

// Fallback stub for unsupported platforms
namespace file_dialog {

std::string open_file(const std::vector<std::string>&) {
    if (detail::test_mode_active) {
        std::string path = detail::queued_path;
        detail::queued_path.clear();
        return path;
    }
    return {};
}

std::string save_file(const std::string&,
                      const std::vector<std::string>&) {
    if (detail::test_mode_active) {
        std::string path = detail::queued_path;
        detail::queued_path.clear();
        return path;
    }
    return {};
}

}  // namespace file_dialog

#endif
