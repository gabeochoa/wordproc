#include "../src/editor/spellcheck.h"

#include "catch.hpp"

// ============================================================================
// SpellChecker Tests
// ============================================================================

TEST_CASE("SpellChecker - Basic word checking", "[spellcheck]") {
    SpellChecker checker;

    SECTION("Common words are correct") {
        REQUIRE(checker.isCorrect("the"));
        REQUIRE(checker.isCorrect("hello"));
        REQUIRE(checker.isCorrect("world"));
        REQUIRE(checker.isCorrect("document"));
        REQUIRE(checker.isCorrect("file"));
    }

    SECTION("Case insensitive checking") {
        REQUIRE(checker.isCorrect("The"));
        REQUIRE(checker.isCorrect("THE"));
        REQUIRE(checker.isCorrect("Hello"));
        REQUIRE(checker.isCorrect("WORLD"));
    }

    SECTION("Misspelled words detected") {
        REQUIRE_FALSE(checker.isCorrect("teh"));
        REQUIRE_FALSE(checker.isCorrect("wrold"));
        REQUIRE_FALSE(checker.isCorrect("documnet"));
        REQUIRE_FALSE(checker.isCorrect("helo"));
    }

    SECTION("Empty and single characters always correct") {
        REQUIRE(checker.isCorrect(""));
        REQUIRE(checker.isCorrect("a"));
        REQUIRE(checker.isCorrect("I"));
        REQUIRE(checker.isCorrect("X"));
    }

    SECTION("All-caps words (acronyms) skipped") {
        REQUIRE(checker.isCorrect("NASA"));
        REQUIRE(checker.isCorrect("FBI"));
        REQUIRE(checker.isCorrect("API"));
        REQUIRE(checker.isCorrect("PDF"));
    }

    SECTION("Words with numbers skipped") {
        REQUIRE(checker.isCorrect("test123"));
        REQUIRE(checker.isCorrect("2nd"));
        REQUIRE(checker.isCorrect("21st"));
    }
}

TEST_CASE("SpellChecker - Suggestions", "[spellcheck]") {
    SpellChecker checker;

    SECTION("Suggestions for misspelled words") {
        auto suggestions = checker.getSuggestions("teh");
        REQUIRE_FALSE(suggestions.empty());
        // "the" should be a top suggestion
        bool foundThe = false;
        for (const auto& s : suggestions) {
            if (s == "the") foundThe = true;
        }
        REQUIRE(foundThe);
    }

    SECTION("Suggestions for transposition errors") {
        auto suggestions = checker.getSuggestions("wrold");
        bool foundWorld = false;
        for (const auto& s : suggestions) {
            if (s == "world") foundWorld = true;
        }
        REQUIRE(foundWorld);
    }

    SECTION("Suggestions for missing letter") {
        auto suggestions = checker.getSuggestions("helo");
        bool foundHelp = false;
        for (const auto& s : suggestions) {
            if (s == "help" || s == "hello") foundHelp = true;
        }
        REQUIRE(foundHelp);
    }

    SECTION("Suggestions limited to max count") {
        auto suggestions = checker.getSuggestions("documnet", 3);
        REQUIRE(suggestions.size() <= 3);
    }

    SECTION("No suggestions for correct word") {
        auto suggestions = checker.getSuggestions("the");
        REQUIRE(suggestions.empty());
    }
}

TEST_CASE("SpellChecker - Text checking", "[spellcheck]") {
    SpellChecker checker;

    SECTION("Check text with no errors") {
        auto errors = checker.checkText("The quick brown fox");
        REQUIRE(errors.empty());
    }

    SECTION("Check text with spelling errors") {
        auto errors = checker.checkText("Teh qiuck browm fox");
        REQUIRE(errors.size() >= 2);  // At least "teh" and "qiuck"
    }

    SECTION("Error offsets are correct") {
        auto errors = checker.checkText("hello wrold");
        REQUIRE(errors.size() == 1);
        REQUIRE(errors[0].offset == 6);  // "wrold" starts at offset 6
        REQUIRE(errors[0].length == 5);
        REQUIRE(errors[0].word == "wrold");
    }

    SECTION("Multiple errors in text") {
        auto errors = checker.checkText("teh browm qiuck fox");
        REQUIRE(errors.size() >= 3);
    }
}

TEST_CASE("SpellChecker - Word extraction", "[spellcheck]") {
    SECTION("Extract simple words") {
        auto words = SpellChecker::extractWords("hello world");
        REQUIRE(words.size() == 2);
        REQUIRE(words[0].second == "hello");
        REQUIRE(words[0].first == 0);
        REQUIRE(words[1].second == "world");
        REQUIRE(words[1].first == 6);
    }

    SECTION("Handle punctuation") {
        auto words = SpellChecker::extractWords("Hello, world!");
        REQUIRE(words.size() == 2);
        REQUIRE(words[0].second == "Hello");
        REQUIRE(words[1].second == "world");
    }

    SECTION("Handle apostrophes") {
        auto words = SpellChecker::extractWords("don't won't");
        REQUIRE(words.size() == 2);
        REQUIRE(words[0].second == "don't");
        REQUIRE(words[1].second == "won't");
    }

    SECTION("Handle multiple spaces") {
        auto words = SpellChecker::extractWords("hello    world");
        REQUIRE(words.size() == 2);
    }

    SECTION("Empty text") {
        auto words = SpellChecker::extractWords("");
        REQUIRE(words.empty());
    }
}

TEST_CASE("SpellChecker - User dictionary", "[spellcheck]") {
    SpellChecker checker;

    SECTION("Add word to user dictionary") {
        REQUIRE_FALSE(checker.isCorrect("asdfgh"));
        checker.addToUserDictionary("asdfgh");
        REQUIRE(checker.isCorrect("asdfgh"));
        REQUIRE(checker.isInUserDictionary("asdfgh"));
    }

    SECTION("Remove word from user dictionary") {
        checker.addToUserDictionary("qwerty");
        REQUIRE(checker.isCorrect("qwerty"));
        checker.removeFromUserDictionary("qwerty");
        REQUIRE_FALSE(checker.isCorrect("qwerty"));
    }

    SECTION("Clear user dictionary") {
        checker.addToUserDictionary("abc123word");
        checker.addToUserDictionary("xyz789word");
        checker.clearUserDictionary();
        REQUIRE_FALSE(checker.isCorrect("abc123word"));
        REQUIRE_FALSE(checker.isCorrect("xyz789word"));
    }

    SECTION("Case insensitive user dictionary") {
        checker.addToUserDictionary("MyWord");
        REQUIRE(checker.isCorrect("myword"));
        REQUIRE(checker.isCorrect("MYWORD"));
        REQUIRE(checker.isCorrect("MyWord"));
    }
}

TEST_CASE("SpellChecker - Ignore list", "[spellcheck]") {
    SpellChecker checker;

    SECTION("Ignore word for session") {
        REQUIRE_FALSE(checker.isCorrect("xyzzy"));
        checker.ignoreWord("xyzzy");
        REQUIRE(checker.isCorrect("xyzzy"));
        REQUIRE(checker.isIgnored("xyzzy"));
    }

    SECTION("Clear ignore list") {
        checker.ignoreWord("plugh");
        REQUIRE(checker.isCorrect("plugh"));
        checker.clearIgnoreList();
        REQUIRE_FALSE(checker.isCorrect("plugh"));
    }

    SECTION("Adding to dictionary removes from ignore list") {
        checker.ignoreWord("specialword");
        REQUIRE(checker.isIgnored("specialword"));
        checker.addToUserDictionary("specialword");
        REQUIRE_FALSE(checker.isIgnored("specialword"));
        REQUIRE(checker.isInUserDictionary("specialword"));
    }
}

TEST_CASE("SpellChecker - Word normalization", "[spellcheck]") {
    SECTION("Lowercase conversion") {
        REQUIRE(SpellChecker::normalizeWord("HELLO") == "hello");
        REQUIRE(SpellChecker::normalizeWord("Hello") == "hello");
        REQUIRE(SpellChecker::normalizeWord("hElLo") == "hello");
    }

    SECTION("Apostrophe removal") {
        REQUIRE(SpellChecker::normalizeWord("don't") == "dont");
        REQUIRE(SpellChecker::normalizeWord("won't") == "wont");
    }

    SECTION("Empty string") {
        REQUIRE(SpellChecker::normalizeWord("") == "");
    }
}

TEST_CASE("SpellChecker - checkWord function", "[spellcheck]") {
    SpellChecker checker;

    SECTION("Check correct word returns false") {
        SpellingError error;
        bool result = checker.checkWord("hello world", 0, 5, error);
        REQUIRE_FALSE(result);  // "hello" is correct
    }

    SECTION("Check incorrect word returns true") {
        SpellingError error;
        bool result = checker.checkWord("helo world", 0, 4, error);
        REQUIRE(result);  // "helo" is misspelled
        REQUIRE(error.word == "helo");
        REQUIRE(error.offset == 0);
        REQUIRE(error.length == 4);
        REQUIRE_FALSE(error.suggestions.empty());
    }
}

// ============================================================================
// GrammarChecker Tests
// ============================================================================

TEST_CASE("GrammarChecker - Double spaces", "[grammar]") {
    GrammarChecker checker;

    SECTION("Detect double spaces") {
        auto errors = checker.checkText("Hello  world");
        REQUIRE(errors.size() == 1);
        REQUIRE(errors[0].ruleId == "DOUBLE_SPACE");
        REQUIRE(errors[0].suggestion == " ");
    }

    SECTION("Detect multiple consecutive spaces") {
        auto errors = checker.checkText("Hello    world");
        REQUIRE(errors.size() == 1);
        REQUIRE(errors[0].length == 4);
    }

    SECTION("No error for single spaces") {
        auto errors = checker.checkText("Hello world");
        bool hasDoubleSpace = false;
        for (const auto& e : errors) {
            if (e.ruleId == "DOUBLE_SPACE") hasDoubleSpace = true;
        }
        REQUIRE_FALSE(hasDoubleSpace);
    }
}

TEST_CASE("GrammarChecker - Sentence capitalization", "[grammar]") {
    GrammarChecker checker;

    SECTION("Detect uncapitalized sentence start") {
        auto errors = checker.checkText("hello world.");
        bool found = false;
        for (const auto& e : errors) {
            if (e.ruleId == "SENTENCE_CAPITALIZATION") {
                found = true;
                REQUIRE(e.suggestion == "Hello");
            }
        }
        REQUIRE(found);
    }

    SECTION("Detect after period") {
        auto errors = checker.checkText("Hello. world");
        bool found = false;
        for (const auto& e : errors) {
            if (e.ruleId == "SENTENCE_CAPITALIZATION") {
                found = true;
            }
        }
        REQUIRE(found);
    }

    SECTION("No error for properly capitalized") {
        auto errors = checker.checkText("Hello world. This is fine.");
        bool found = false;
        for (const auto& e : errors) {
            if (e.ruleId == "SENTENCE_CAPITALIZATION") found = true;
        }
        REQUIRE_FALSE(found);
    }
}

TEST_CASE("GrammarChecker - Repeated words", "[grammar]") {
    GrammarChecker checker;

    SECTION("Detect repeated word") {
        auto errors = checker.checkText("the the quick fox");
        bool found = false;
        for (const auto& e : errors) {
            if (e.ruleId == "REPEATED_WORD") {
                found = true;
                REQUIRE(e.message == "Repeated word");
            }
        }
        REQUIRE(found);
    }

    SECTION("Case insensitive detection") {
        auto errors = checker.checkText("The the quick fox");
        bool found = false;
        for (const auto& e : errors) {
            if (e.ruleId == "REPEATED_WORD") found = true;
        }
        REQUIRE(found);
    }

    SECTION("No error for different words") {
        auto errors = checker.checkText("the quick brown fox");
        bool found = false;
        for (const auto& e : errors) {
            if (e.ruleId == "REPEATED_WORD") found = true;
        }
        REQUIRE_FALSE(found);
    }
}

TEST_CASE("GrammarChecker - Common errors", "[grammar]") {
    GrammarChecker checker;

    SECTION("Detect 'alot' error") {
        auto errors = checker.checkText("I have alot of work");
        bool found = false;
        for (const auto& e : errors) {
            if (e.ruleId == "COMMON_ERRORS" && e.text == "alot") {
                found = true;
                REQUIRE(e.suggestion == "a lot");
            }
        }
        REQUIRE(found);
    }

    SECTION("Detect 'could of' error") {
        auto errors = checker.checkText("I could of done it");
        bool found = false;
        for (const auto& e : errors) {
            if (e.ruleId == "COMMON_ERRORS" && e.text == "could of") {
                found = true;
                REQUIRE(e.suggestion == "could have");
            }
        }
        REQUIRE(found);
    }
}

TEST_CASE("GrammarChecker - Rule enabling/disabling", "[grammar]") {
    GrammarChecker checker;

    SECTION("Rules enabled by default") {
        REQUIRE(checker.isRuleEnabled("DOUBLE_SPACE"));
        REQUIRE(checker.isRuleEnabled("SENTENCE_CAPITALIZATION"));
        REQUIRE(checker.isRuleEnabled("REPEATED_WORD"));
        REQUIRE(checker.isRuleEnabled("COMMON_ERRORS"));
    }

    SECTION("Disable rule") {
        checker.disableRule("DOUBLE_SPACE");
        REQUIRE_FALSE(checker.isRuleEnabled("DOUBLE_SPACE"));

        auto errors = checker.checkText("Hello  world");
        bool found = false;
        for (const auto& e : errors) {
            if (e.ruleId == "DOUBLE_SPACE") found = true;
        }
        REQUIRE_FALSE(found);
    }

    SECTION("Re-enable rule") {
        checker.disableRule("DOUBLE_SPACE");
        checker.enableRule("DOUBLE_SPACE");
        REQUIRE(checker.isRuleEnabled("DOUBLE_SPACE"));
    }

    SECTION("Available rules list") {
        auto rules = checker.availableRules();
        REQUIRE(rules.size() == 4);
    }
}

TEST_CASE("GrammarChecker - Combined checks", "[grammar]") {
    GrammarChecker checker;

    SECTION("Multiple errors in one text") {
        auto errors = checker.checkText("hello  the the world.");
        // Should have: capitalization, double space, repeated word
        REQUIRE(errors.size() >= 3);
    }

    SECTION("Errors sorted by offset") {
        auto errors = checker.checkText("hello  the the world.");
        for (std::size_t i = 1; i < errors.size(); ++i) {
            REQUIRE(errors[i].offset >= errors[i - 1].offset);
        }
    }
}

// ============================================================================
// SpellingError struct tests
// ============================================================================

TEST_CASE("SpellingError - Structure", "[spellcheck]") {
    SpellingError error;
    error.offset = 10;
    error.length = 5;
    error.word = "wrold";
    error.suggestions = {"world", "would"};

    REQUIRE(error.offset == 10);
    REQUIRE(error.length == 5);
    REQUIRE(error.word == "wrold");
    REQUIRE(error.suggestions.size() == 2);
}

// ============================================================================
// GrammarError struct tests
// ============================================================================

TEST_CASE("GrammarError - Structure", "[grammar]") {
    GrammarError error;
    error.offset = 5;
    error.length = 2;
    error.text = "  ";
    error.message = "Multiple consecutive spaces";
    error.suggestion = " ";
    error.ruleId = "DOUBLE_SPACE";

    REQUIRE(error.offset == 5);
    REQUIRE(error.length == 2);
    REQUIRE(error.ruleId == "DOUBLE_SPACE");
}
