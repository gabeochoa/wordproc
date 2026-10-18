#pragma once

// AutoCorrect: replace-as-you-type (Word 95 Tools > AutoCorrect).
//
// correct() runs when a word is completed with a space or punctuation
// mark. It applies, in order: TWo INitials fix, the replacement table
// (case-preserving), then first-letter-of-sentence capitalization.
// Returns nullopt when nothing applies, so the typed word is left alone.

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "text_buffer.h"

struct AutoCorrectSettings {
    bool replaceTextAsYouType = true;
    bool capitalizeFirstLetterOfSentence = true;
    bool correctTwoInitialCapitals = true;
};

class AutoCorrect {
   public:
    AutoCorrect() = default;

    // Toggles, bound to the Tools > AutoCorrect... dialog checkboxes.
    AutoCorrectSettings settings;

    // Correct a completed word. sentenceStart: the word begins a sentence.
    // Returns the corrected word, or nullopt when nothing applies.
    std::optional<std::string> correct(const std::string& word,
                                       bool sentenceStart) const;

    // Replace-as-you-type table lookup (case-insensitive key).
    // A user entry (added via the dialog) shadows a built-in entry
    // with the same Replace key.
    std::optional<std::string> lookup(const std::string& word) const;

    // User entries added via the Tools > AutoCorrect... dialog.
    // Session-only: they live in this AutoCorrect instance, are checked
    // alongside the built-in table, and vanish with the document.
    // Trims whitespace; ignores empty Replace/With. Adding a Replace
    // key that already has a user entry updates its With value.
    void addUserEntry(const std::string& replace, const std::string& with);
    // Removes the user entry with this Replace key (case-insensitive).
    // Built-in entries are never removed: returns false for them.
    bool removeUserEntry(const std::string& replace);
    bool isUserEntry(const std::string& replace) const;
    const std::vector<std::pair<std::string, std::string>>& userEntries()
        const;

    // Sentence-start test on the text before a word: true at doc start,
    // after a paragraph break, or after . ! ? (skipping whitespace and a
    // closing quote/bracket). False mid-sentence.
    static bool isSentenceStart(const std::string& textBeforeWord);

    // Same test, reading backwards from wordStart in the buffer (no
    // allocation: only the last kMaxLookback characters are examined).
    static bool isSentenceStart(const TextBuffer& buffer,
                                std::size_t wordStart);

    static const std::unordered_map<std::string, std::string>& table();

    // Batch pass over the whole document (Tools > AutoFormat...):
    // applies correct() to every word, as a single undo step via
    // TextBuffer::replaceRanges. Uses the same settings as
    // as-you-type (table lookup honors replaceTextAsYouType, etc.).
    // Returns the number of words corrected (0 = no change, no
    // undo step recorded).
    std::size_t autoFormatPass(TextBuffer& buffer) const;

    // The replacement table as (Replace, With) pairs sorted by the
    // Replace key, for the AutoCorrect dialog's list: built-ins plus
    // user entries, a user entry shadowing a built-in with the same key.
    std::vector<std::pair<std::string, std::string>> entries() const;

   private:
    static constexpr std::size_t kMaxLookback = 4096;

    // Session-only user entries, in insertion order.
    std::vector<std::pair<std::string, std::string>> userEntries_;

    // Copy the original word's case onto the correction: ALL-CAPS ->
    // ALL-CAPS, Capitalized -> Capitalized, otherwise as-is.
    static std::string matchCase(const std::string& original,
                                 const std::string& correction);
    static bool isTwoInitialCapitals(const std::string& word);
    static std::string fixTwoInitialCapitals(const std::string& word);
    static std::string capitalizeFirst(const std::string& word);
};
