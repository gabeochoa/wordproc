#pragma once

#include <string>

#include "text_buffer.h"

bool saveTextFile(const TextBuffer &buffer, const std::string &path);
bool loadTextFile(TextBuffer &buffer, const std::string &path);
