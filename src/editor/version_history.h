#pragma once

// Version History (Docs-era, Win95-styled): snapshots written on
// every successful Save / Save As, listed newest-first in File >
// Version History.... Pure filesystem helpers so unit tests can
// pin naming/indexing/listing against a temp dir.
//
// Layout: <base>/<sanitized doc name>/<YYYYmmdd-HHMMSS>.txt plus an
// index.txt in the doc dir with one line per snapshot (time, words,
// chars). The dialog lists by scanning the snapshot files (the
// index is a log, not the source of truth — a snapshot whose index
// line is missing still lists).

#include <algorithm>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace version_history {

struct VersionSnapshot {
    std::string filename;   // e.g. "20261019-120000.txt"
    std::string label;      // filename stem: "20261019-120000"
    std::string path;       // full path to the snapshot file
    std::size_t words = 0;
    std::size_t chars = 0;
};

// Keep [A-Za-z0-9._-], replace anything else with '_'. Empty (or
// all-replaced) names become "Untitled". The stem never contains a
// path separator, so the snapshot dir cannot escape its parent.
inline std::string sanitizeDocName(const std::string& name) {
    std::string out;
    out.reserve(name.size());
    for (char ch : name) {
        unsigned char c = static_cast<unsigned char>(ch);
        if (std::isalnum(c) || ch == '.' || ch == '_' || ch == '-') {
            out.push_back(ch);
        } else {
            out.push_back('_');
        }
    }
    if (out.empty() || out.find_first_not_of("._") == std::string::npos) {
        return "Untitled";
    }
    return out;
}

// "YYYYmmdd-HHMMSS" in local time.
inline std::string timestampLabel(std::time_t when) {
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &when);
#else
    localtime_r(&when, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y%m%d-%H%M%S", &tm);
    return buf;
}

// Word count: runs of alphanumeric chars (same rule as
// TextBuffer::stats words).
inline std::size_t countWords(const std::string& text) {
    std::size_t words = 0;
    bool inWord = false;
    for (char ch : text) {
        if (std::isalnum(static_cast<unsigned char>(ch))) {
            if (!inWord) {
                ++words;
                inWord = true;
            }
        } else {
            inWord = false;
        }
    }
    return words;
}

inline std::string indexLine(const std::string& label, std::size_t words,
                             std::size_t chars, const std::string& filename) {
    return label + " words=" + std::to_string(words) +
           " chars=" + std::to_string(chars) + " file=" + filename;
}

// Newest-first snapshots in `dir`: *.txt except index.txt, sorted
// by filename descending (the timestamp prefix sorts
// chronologically). Each file is read for its word/char counts;
// unreadable files still list with 0/0.
inline std::vector<VersionSnapshot> listSnapshots(
    const std::filesystem::path& dir) {
    std::vector<VersionSnapshot> out;
    std::error_code ec;
    if (!std::filesystem::exists(dir, ec)) return out;
    for (const auto& entry :
         std::filesystem::directory_iterator(dir, ec)) {
        if (!entry.is_regular_file(ec)) continue;
        std::filesystem::path path = entry.path();
        if (path.extension() != ".txt") continue;
        if (path.filename() == "index.txt") continue;
        std::ifstream in(path);
        std::string text((std::istreambuf_iterator<char>(in)),
                         std::istreambuf_iterator<char>());
        VersionSnapshot snap;
        snap.filename = path.filename().string();
        snap.label = path.stem().string();
        snap.path = path.string();
        snap.words = countWords(text);
        snap.chars = text.size();
        out.push_back(std::move(snap));
    }
    std::sort(out.begin(), out.end(),
              [](const VersionSnapshot& a, const VersionSnapshot& b) {
                  return a.filename > b.filename;
              });
    return out;
}

// Write a snapshot of `text` for document `docName` under `base`
// and append its index line. Same-second saves get a -1/-2 suffix
// so no snapshot is ever overwritten. Returns the snapshot path,
// or empty on failure.
inline std::string writeSnapshot(const std::filesystem::path& base,
                                 const std::string& docName,
                                 const std::string& text, std::time_t when) {
    std::error_code ec;
    std::filesystem::path dir = base / sanitizeDocName(docName);
    std::filesystem::create_directories(dir, ec);
    if (ec) return "";
    const std::string label = timestampLabel(when);
    std::filesystem::path path;
    for (int suffix = 0;; ++suffix) {
        std::string filename = label + (suffix == 0 ? "" : "-" + std::to_string(suffix)) + ".txt";
        std::filesystem::path candidate = dir / filename;
        if (!std::filesystem::exists(candidate, ec)) {
            path = candidate;
            break;
        }
    }
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out) return "";
        out << text;
        if (!out) return "";
    }
    {
        std::ofstream index(dir / "index.txt", std::ios::app);
        if (index) {
            index << indexLine(label, countWords(text), text.size(),
                               path.filename().string())
                  << '\n';
        }
    }
    return path.string();
}

// Read a snapshot file's full text; empty when unreadable. (An
// empty snapshot is indistinguishable from a missing one here;
// callers that need the distinction check existence first.)
inline std::string readSnapshot(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return "";
    return std::string((std::istreambuf_iterator<char>(in)),
                       std::istreambuf_iterator<char>());
}

}  // namespace version_history
