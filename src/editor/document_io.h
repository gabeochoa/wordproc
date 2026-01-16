#pragma once

#include <string>

#include "text_buffer.h"

// Load/save result with error information
struct DocumentResult {
    bool success = false;
    bool usedFallback =
        false;          // True if file was loaded as plain text fallback
    std::string error;  // Error message if not successful
};

bool saveTextFile(const TextBuffer &buffer, const std::string &path);
bool loadTextFile(TextBuffer &buffer, const std::string &path);

// Extended versions with error reporting
DocumentResult saveTextFileEx(const TextBuffer &buffer,
                              const std::string &path);
DocumentResult loadTextFileEx(TextBuffer &buffer, const std::string &path);
