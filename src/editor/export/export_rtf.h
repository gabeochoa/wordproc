#pragma once

#include <string>

#include "../document_io.h"
#include "../document_settings.h"
#include "../text_buffer.h"

DocumentResult exportDocumentRtf(const TextBuffer& buffer,
                                 const DocumentSettings& settings,
                                 const std::string& path);

