#pragma once

#include <string>

#include "../document_io.h"
#include "../document_settings.h"
#include "../text_buffer.h"

DocumentResult exportDocumentHtml(const TextBuffer& buffer,
                                  const DocumentSettings& settings,
                                  const std::string& path);

