#pragma once

// Spelling dialog session helpers (no UI): find the next misspelled word
// at/after a document offset, with suggestions for that one word only.
// Scanning the whole document for errors is cheap (hash lookups); the
// expensive part is getSuggestions(), which scans the dictionary, so it
// runs only for the single current word the dialog shows.

#include <optional>
#include <string>

#include "spellcheck.h"

// First misspelled word whose end is after fromOffset, so a caret sitting
// inside a word still checks that word. Returns nullopt when the rest of
// the text (from fromOffset on) is clean.
inline std::optional<SpellingError> findNextSpellingError(
    const SpellChecker& checker, const std::string& text,
    std::size_t fromOffset) {
    for (const auto& [offset, word] : SpellChecker::extractWords(text)) {
        if (offset + word.size() <= fromOffset) continue;
        if (!checker.isCorrect(word)) {
            SpellingError error;
            error.offset = offset;
            error.length = word.size();
            error.word = word;
            error.suggestions = checker.getSuggestions(word);
            return error;
        }
    }
    return std::nullopt;
}
