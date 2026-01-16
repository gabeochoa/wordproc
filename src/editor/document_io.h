#ifndef WORDPROC_EDITOR_DOCUMENT_IO_H
#define WORDPROC_EDITOR_DOCUMENT_IO_H

#include <string>

#include "document_settings.h"
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

// Full document save/load with all settings
DocumentResult saveDocumentEx(const TextBuffer &buffer,
                              const DocumentSettings &settings,
                              const std::string &path);
DocumentResult loadDocumentEx(TextBuffer &buffer, DocumentSettings &settings,
                              const std::string &path);

#endif  // WORDPROC_EDITOR_DOCUMENT_IO_H
