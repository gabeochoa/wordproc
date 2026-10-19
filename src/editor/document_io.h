#ifndef WORDPROC_EDITOR_DOCUMENT_IO_H
#define WORDPROC_EDITOR_DOCUMENT_IO_H

#include <string>
#include <utility>
#include <vector>

#include "document_settings.h"
#include "table.h"
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

// Type alias for table storage (line number -> table)
using TableList = std::vector<std::pair<std::size_t, Table>>;

// Full document save/load with tables. The optional header /
// footer buffers (Batch 9) round-trip through
// "headerText"/"footerText"; a null pointer means "no header /
// footer buffer on this path" (old callers keep compiling and
// keep their old file shape). On load a non-null buffer is
// replaced (cleared when the file carries no header/footer
// text), mirroring the tables behaviour.
DocumentResult saveDocumentWithTables(const TextBuffer &buffer,
                                      const DocumentSettings &settings,
                                      const TableList &tables,
                                      const std::string &path,
                                      const TextBuffer *headerBuffer = nullptr,
                                      const TextBuffer *footerBuffer = nullptr);
DocumentResult loadDocumentWithTables(TextBuffer &buffer,
                                      DocumentSettings &settings,
                                      TableList &tables,
                                      const std::string &path,
                                      TextBuffer *headerBuffer = nullptr,
                                      TextBuffer *footerBuffer = nullptr);

#endif  // WORDPROC_EDITOR_DOCUMENT_IO_H
