#pragma once

// File > New from Template... helpers (Word 95 parity).
// Templates are real .wpdoc documents in resources/templates/,
// opened as NEW Untitled documents (filePath cleared, dirty).
// Listing resolves through the same resource-root order as the
// afterhours files provider (exe-adjacent resources/, then
// <cwd>/resources — the cwd-relative layout Settings uses for
// settings.json), plus the output/-subdir fallbacks in
// templateDir(), so test mode (run from the repo root or from
// output/) finds the same files.

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

#include "document_io.h"
#include "text_buffer.h"

namespace templates {

// First entry of the New from Template list: no file on disk, OK
// opens an empty document (File > New semantics).
inline constexpr const char* kBlankTemplateName = "Blank Document";

// One-line description per template entry, shown in the dialog as
// "Name — description". Blank has no file; the others are the
// shipped .wpdoc stems.
inline std::string templateDescriptionFor(const std::string& name) {
    if (name == kBlankTemplateName) return "Empty document";
    if (name == "Letter") return "Business letter with return address";
    if (name == "Memo") return "MEMORANDUM header block";
    if (name == "Report") return "Title, headings, body sections";
    return "";
}

// Directory holding the shipped .wpdoc templates. Resolution
// mirrors the afterhours files provider's resource root (an
// exe-adjacent resources/ dir, else <cwd>/resources — the same
// cwd-relative layout Settings uses for settings.json), with two
// extra fallbacks for the dev layout where the binary lives in
// output/ under the repo root and the batch runner's cwd IS
// output/ (in that layout the provider's own root,
// <cwd>/resources, does not exist; the repo-root resources/ one
// level up does). Deliberately implemented with plain filesystem
// + executable-path APIs instead of calling files::get_provider(),
// whose definition lives in files.cpp — the unit-test binary does
// not link that TU, and templates.h must stay linkable there.
inline std::filesystem::path templateDir() {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path exeDir;
#ifdef __APPLE__
    {
        std::uint32_t size = 0;
        _NSGetExecutablePath(nullptr, &size);
        std::string buf(size, '\0');
        if (_NSGetExecutablePath(buf.data(), &size) == 0) {
            std::error_code ec2;
            exeDir = fs::canonical(fs::path(buf), ec2).parent_path();
        }
    }
#elif defined(__linux__)
    {
        std::error_code ec2;
        fs::path exe = fs::read_symlink("/proc/self/exe", ec2);
        if (!ec2 && !exe.empty()) exeDir = exe.parent_path();
    }
#elif defined(_WIN32)
    {
        char buf[MAX_PATH] = {0};
        if (GetModuleFileNameA(nullptr, buf, MAX_PATH) > 0) {
            exeDir = fs::path(buf).parent_path();
        }
    }
#endif
    fs::path cwd = fs::current_path(ec);
    const fs::path candidates[] = {
        exeDir / "resources" / "templates",
        cwd / "resources" / "templates",
        exeDir / ".." / "resources" / "templates",
        cwd / ".." / "resources" / "templates",
    };
    for (const auto& candidate : candidates) {
        if (!candidate.empty() && fs::is_directory(candidate, ec)) {
            return candidate;
        }
    }
    return cwd / "resources" / "templates";
}

// Template names (file stems, sorted) for .wpdoc files in dir.
// Empty when the directory does not exist.
inline std::vector<std::string> listTemplateNames(
    const std::filesystem::path& dir) {
    namespace fs = std::filesystem;
    std::vector<std::string> names;
    std::error_code ec;
    if (!fs::is_directory(dir, ec)) return names;
    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (ec) break;
        if (!entry.is_regular_file(ec)) continue;
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char c) {
                           return static_cast<char>(std::tolower(c));
                       });
        if (ext == ".wpdoc") {
            names.push_back(entry.path().stem().string());
        }
    }
    std::sort(names.begin(), names.end());
    return names;
}

// Adopt a loaded template buffer into a target buffer as a NEW
// document: replace the text, carry the per-line paragraph styles
// (setText alone would reset them to Normal), the template's global
// style, and park the caret at the start with no selection. The
// caller owns filePath/dirty/history semantics.
inline void adoptTemplate(TextBuffer& target, const TextBuffer& source,
                          const DocumentSettings& sourceSettings) {
    target.setText(source.getText());
    target.clearHistory();
    for (std::size_t i = 0; i < source.lineCount(); ++i) {
        target.setLineParagraphStyle(i, source.lineParagraphStyle(i));
    }
    target.setTextStyle(sourceSettings.textStyle);
    target.setCaret({0, 0});
    target.clearSelection();
}

// Load a template file into a scratch buffer + settings (validates
// it parses as a .wpdoc). Returns false on failure.
inline bool loadTemplate(const std::filesystem::path& path,
                         TextBuffer& buffer, DocumentSettings& settings) {
    auto result = loadDocumentEx(buffer, settings, path.string());
    return result.success;
}

}  // namespace templates
