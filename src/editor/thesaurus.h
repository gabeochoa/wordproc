#pragma once

// Tools > Thesaurus... (Shift+F7) data layer (no UI): parse + lookup
// for resources/thesaurus.txt, format `word: syn1, syn2, ...`
// (lowercase). Parsing is pure (parse(content)) so unit tests never
// touch the filesystem; loadDefault() resolves the shipped file with
// the same exe/cwd fallbacks as templates::templateDir().

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

namespace thesaurus {

inline std::string trim(const std::string& value) {
    std::size_t begin = 0;
    while (begin < value.size() &&
           std::isspace(static_cast<unsigned char>(value[begin]))) {
        ++begin;
    }
    std::size_t end = value.size();
    while (end > begin &&
           std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(begin, end - begin);
}

inline std::string lower(const std::string& value) {
    std::string result = value;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char ch) {
                       return static_cast<char>(std::tolower(ch));
                   });
    return result;
}

// Parse thesaurus text. Lines starting with '#' and blank lines are
// skipped; a line without ':' is skipped (never a partial entry);
// headword and synonyms are trimmed and lowercased; an entry with
// no synonyms after the colon is skipped. Later entries for the
// same word replace earlier ones.
inline std::map<std::string, std::vector<std::string>> parse(
    const std::string& content) {
    std::map<std::string, std::vector<std::string>> entries;
    std::size_t pos = 0;
    while (pos <= content.size()) {
        std::size_t eol = content.find('\n', pos);
        std::string line = trim(content.substr(
            pos, eol == std::string::npos ? std::string::npos : eol - pos));
        pos = eol == std::string::npos ? content.size() + 1 : eol + 1;
        if (line.empty() || line[0] == '#') continue;
        std::size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string word = lower(trim(line.substr(0, colon)));
        if (word.empty()) continue;
        std::vector<std::string> synonyms;
        std::string rest = line.substr(colon + 1);
        std::size_t start = 0;
        while (start <= rest.size()) {
            std::size_t comma = rest.find(',', start);
            std::string syn = lower(trim(rest.substr(
                start, comma == std::string::npos ? std::string::npos
                                                  : comma - start)));
            if (!syn.empty()) synonyms.push_back(syn);
            if (comma == std::string::npos) break;
            start = comma + 1;
        }
        if (!synonyms.empty()) entries[word] = std::move(synonyms);
    }
    return entries;
}

inline std::vector<std::string> lookup(
    const std::map<std::string, std::vector<std::string>>& entries,
    const std::string& word) {
    auto it = entries.find(lower(trim(word)));
    if (it == entries.end()) return {};
    return it->second;
}

// Path of the shipped thesaurus file, resolved like
// templates::templateDir(): exe-adjacent resources/ first, then
// <cwd>/resources, then the repo-root resources/ one level up from
// either (the dev layout where the binary lives in output/ and the
// batch runner's cwd IS output/). The unit-test binary does not link
// files.cpp, so this uses plain filesystem + executable-path APIs.
inline std::filesystem::path thesaurusPath() {
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
        exeDir / "resources" / "thesaurus.txt",
        cwd / "resources" / "thesaurus.txt",
        exeDir / ".." / "resources" / "thesaurus.txt",
        cwd / ".." / "resources" / "thesaurus.txt",
    };
    for (const auto& candidate : candidates) {
        if (!candidate.empty() && fs::is_regular_file(candidate, ec)) {
            return candidate;
        }
    }
    return cwd / "resources" / "thesaurus.txt";
}

inline std::map<std::string, std::vector<std::string>> loadFromPath(
    const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) return {};
    std::string content((std::istreambuf_iterator<char>(in)),
                        std::istreambuf_iterator<char>());
    return parse(content);
}

inline std::map<std::string, std::vector<std::string>> loadDefault() {
    return loadFromPath(thesaurusPath());
}

}  // namespace thesaurus
