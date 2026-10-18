#include "autocorrect.h"

#include <cctype>

namespace {

bool isUpper(char ch) { return std::isupper(static_cast<unsigned char>(ch)) != 0; }
bool isLower(char ch) { return std::islower(static_cast<unsigned char>(ch)) != 0; }
bool isAlpha(char ch) { return std::isalpha(static_cast<unsigned char>(ch)) != 0; }

std::string toLowerStr(const std::string& word) {
    std::string result = word;
    for (char& ch : result) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return result;
}

std::string toUpperStr(const std::string& word) {
    std::string result = word;
    for (char& ch : result) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    return result;
}

}  // namespace

const std::unordered_map<std::string, std::string>& AutoCorrect::table() {
    static const std::unordered_map<std::string, std::string> kTable = {
        {"teh", "the"},       {"hte", "the"},       {"taht", "that"},
        {"adn", "and"},       {"wtih", "with"},     {"waht", "what"},
        {"yuo", "you"},       {"liek", "like"},     {"wich", "which"},
        {"adress", "address"}, {"recieve", "receive"},
        {"seperate", "separate"}, {"definately", "definitely"},
        {"occured", "occurred"}, {"untill", "until"},
        {"wierd", "weird"},    {"thier", "their"},  {"truely", "truly"},
        {"accomodate", "accommodate"}, {"occurence", "occurrence"},
        {"enviroment", "environment"}, {"goverment", "government"},
        {"neccessary", "necessary"}, {"sucess", "success"},
        {"tommorow", "tomorrow"}, {"alot", "a lot"},
        {"dont", "don't"},    {"wont", "won't"},    {"cant", "can't"},
        {"isnt", "isn't"},    {"i", "I"},
    };
    return kTable;
}

std::optional<std::string> AutoCorrect::lookup(const std::string& word) const {
    auto it = table().find(toLowerStr(word));
    if (it == table().end()) return std::nullopt;
    return it->second;
}

std::string AutoCorrect::matchCase(const std::string& original,
                                   const std::string& correction) {
    if (original.size() > 1) {
        bool allUpper = true;
        for (char ch : original) {
            if (isAlpha(ch) && !isUpper(ch)) {
                allUpper = false;
                break;
            }
        }
        if (allUpper) return toUpperStr(correction);
    }
    if (!original.empty() && isUpper(original[0])) {
        return capitalizeFirst(correction);
    }
    return correction;
}

bool AutoCorrect::isTwoInitialCapitals(const std::string& word) {
    return word.size() >= 3 && isUpper(word[0]) && isUpper(word[1]) &&
           isLower(word[2]);
}

std::string AutoCorrect::fixTwoInitialCapitals(const std::string& word) {
    std::string result = word;
    result[1] = static_cast<char>(
        std::tolower(static_cast<unsigned char>(result[1])));
    return result;
}

std::string AutoCorrect::capitalizeFirst(const std::string& word) {
    if (word.empty() || !isAlpha(word[0])) return word;
    std::string result = word;
    result[0] =
        static_cast<char>(std::toupper(static_cast<unsigned char>(result[0])));
    return result;
}

std::optional<std::string> AutoCorrect::correct(const std::string& word,
                                               bool sentenceStart) const {
    if (word.empty()) return std::nullopt;

    std::string result = word;
    bool changed = false;

    if (settings.correctTwoInitialCapitals && isTwoInitialCapitals(result)) {
        result = fixTwoInitialCapitals(result);
        changed = true;
    }
    if (settings.replaceTextAsYouType) {
        if (auto hit = lookup(result)) {
            result = matchCase(word, *hit);
            changed = true;
        }
    }
    if (changed) {
        // A table / two-initials fix at a sentence start still gets
        // capitalized ("teh" -> "the" -> "The").
        if (sentenceStart && settings.capitalizeFirstLetterOfSentence) {
            result = capitalizeFirst(result);
        }
        return result == word ? std::nullopt
                              : std::optional<std::string>(result);
    }

    if (sentenceStart && settings.capitalizeFirstLetterOfSentence &&
        isLower(result[0])) {
        return capitalizeFirst(result);
    }
    return std::nullopt;
}

bool AutoCorrect::isSentenceStart(const std::string& textBeforeWord) {
    std::size_t i = textBeforeWord.size();
    bool sawNewline = false;
    while (i > 0 &&
           std::isspace(static_cast<unsigned char>(textBeforeWord[i - 1]))) {
        if (textBeforeWord[i - 1] == '\n') sawNewline = true;
        --i;
    }
    if (i == 0) return true;        // Start of document
    if (sawNewline) return true;    // Start of a new paragraph
    char c = textBeforeWord[i - 1];
    if (c == '.' || c == '!' || c == '?') return true;
    if (c == '"' || c == '\'' || c == '(' || c == '[' || c == '{') {
        // Opening quote/bracket: what precedes it decides.
        --i;
        while (i > 0 &&
               std::isspace(static_cast<unsigned char>(textBeforeWord[i - 1]))) {
            --i;
        }
        if (i == 0) return true;
        char p = textBeforeWord[i - 1];
        return p == '.' || p == '!' || p == '?';
    }
    return false;
}

bool AutoCorrect::isSentenceStart(const TextBuffer& buffer,
                                  std::size_t wordStart) {
    // Bounded prefix: sentence punctuation is normally within a few
    // hundred characters; cap the scan so a delimiter keystroke in a
    // giant single-sentence paragraph stays cheap.
    std::size_t begin =
        wordStart > kMaxLookback ? wordStart - kMaxLookback : 0;
    std::string prefix;
    prefix.reserve(wordStart - begin);
    for (std::size_t i = begin; i < wordStart; ++i) {
        prefix.push_back(buffer.charAtOffset(i));
    }
    // A truncated window can miss a distant terminator (missed
    // capitalization), but the prefix ending in ". " / a paragraph break
    // still proves sentence-start, so this stays exact in the common case.
    return isSentenceStart(prefix);
}
