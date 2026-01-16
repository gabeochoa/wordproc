#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

// Represents a spelling error in the document
struct SpellingError {
    std::size_t offset = 0;      // Character offset in document
    std::size_t length = 0;      // Length of the misspelled word
    std::string word;            // The misspelled word
    std::vector<std::string> suggestions;  // Suggested corrections (max 5)
};

// Per-word action types for spell checking UI
enum class SpellAction {
    Ignore,           // Ignore this instance
    IgnoreAll,        // Ignore all instances of this word in document
    AddToDictionary,  // Add word to user dictionary
    Replace,          // Replace with a specific correction
    ReplaceAll        // Replace all instances with a correction
};

// Result of applying a spell action
struct SpellActionResult {
    bool success = false;
    std::size_t replacementCount = 0;  // Number of replacements made (for ReplaceAll)
};

// Spell checker with dictionary and suggestions
class SpellChecker {
   public:
    SpellChecker();
    ~SpellChecker() = default;

    // Check if a single word is spelled correctly
    bool isCorrect(const std::string& word) const;

    // Get spelling suggestions for a word (up to maxSuggestions)
    std::vector<std::string> getSuggestions(const std::string& word,
                                            std::size_t maxSuggestions = 5) const;

    // Check entire text and return all spelling errors
    std::vector<SpellingError> checkText(const std::string& text) const;

    // Check a single word at a specific offset and return error if misspelled
    bool checkWord(const std::string& text, std::size_t offset, std::size_t length,
                   SpellingError& outError) const;

    // Dictionary management
    void addToUserDictionary(const std::string& word);
    void removeFromUserDictionary(const std::string& word);
    bool isInUserDictionary(const std::string& word) const;
    void clearUserDictionary();
    const std::unordered_set<std::string>& userDictionary() const {
        return userDictionary_;
    }

    // Session-based ignore list (not persisted)
    void ignoreWord(const std::string& word);
    void clearIgnoreList();
    bool isIgnored(const std::string& word) const;

    // Load/save user dictionary
    bool loadUserDictionary(const std::string& path);
    bool saveUserDictionary(const std::string& path) const;

    // Get default dictionary word count
    std::size_t dictionarySize() const { return dictionary_.size(); }

    // Word extraction utilities
    static std::vector<std::pair<std::size_t, std::string>> extractWords(
        const std::string& text);
    static bool isWordChar(char ch);
    static std::string normalizeWord(const std::string& word);

   private:
    // Calculate Levenshtein edit distance between two words
    static std::size_t editDistance(const std::string& a, const std::string& b);

    // Initialize the built-in dictionary
    void initDefaultDictionary();

    std::unordered_set<std::string> dictionary_;      // Built-in dictionary
    std::unordered_set<std::string> userDictionary_;  // User-added words
    std::unordered_set<std::string> ignoreList_;      // Session ignore list
};

// Grammar checker (basic rules-based approach)
struct GrammarError {
    std::size_t offset = 0;
    std::size_t length = 0;
    std::string text;              // The problematic text
    std::string message;           // Description of the issue
    std::string suggestion;        // Suggested correction
    std::string ruleId;            // Rule identifier (e.g., "DOUBLE_SPACE")
};

enum class GrammarAction {
    Ignore,
    IgnoreRule,  // Ignore all instances of this rule
    Accept       // Accept the suggestion
};

class GrammarChecker {
   public:
    GrammarChecker() = default;
    ~GrammarChecker() = default;

    // Check text for grammar issues
    std::vector<GrammarError> checkText(const std::string& text) const;

    // Enable/disable specific rules
    void enableRule(const std::string& ruleId);
    void disableRule(const std::string& ruleId);
    bool isRuleEnabled(const std::string& ruleId) const;

    // Get all available rule IDs
    std::vector<std::string> availableRules() const;

   private:
    std::unordered_set<std::string> disabledRules_;

    // Individual grammar rules
    void checkDoubleSpaces(const std::string& text,
                           std::vector<GrammarError>& errors) const;
    void checkCapitalization(const std::string& text,
                             std::vector<GrammarError>& errors) const;
    void checkRepeatedWords(const std::string& text,
                            std::vector<GrammarError>& errors) const;
    void checkCommonErrors(const std::string& text,
                           std::vector<GrammarError>& errors) const;
};
