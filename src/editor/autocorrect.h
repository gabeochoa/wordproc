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
    std::optional<std::string> lookup(const std::string& word) const;

    // Sentence-start test on the text before a word: true at doc start,
    // after a paragraph break, or after . ! ? (skipping whitespace and a
    // closing quote/bracket). False mid-sentence.
    static bool isSentenceStart(const std::string& textBeforeWord);

    // Same test, reading backwards from wordStart in the buffer (no
    // allocation: only the last kMaxLookback characters are examined).
    static bool isSentenceStart(const TextBuffer& buffer,
                                std::size_t wordStart);

    static const std::unordered_map<std::string, std::string>& table();

   private:
    static constexpr std::size_t kMaxLookback = 4096;

    // Copy the original word's case onto the correction: ALL-CAPS ->
    // ALL-CAPS, Capitalized -> Capitalized, otherwise as-is.
    static std::string matchCase(const std::string& original,
                                 const std::string& correction);
    static bool isTwoInitialCapitals(const std::string& word);
    static std::string fixTwoInitialCapitals(const std::string& word);
    static std::string capitalizeFirst(const std::string& word);
};
