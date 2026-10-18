#include "../src/editor/spellcheck.h"
#include "../src/editor/text_buffer.h"

#include "catch2/catch.hpp"

// GrammarChecker (Tools > Grammar... dialog lists these, read-only).
// The E2E pins the repeated-word case: "The the cat sat." yields
// exactly one REPEATED_WORD issue.

TEST_CASE("GrammarChecker - repeated word", "[grammar]") {
    GrammarChecker checker;

    SECTION("detects a repeated word (pinned: Grammar E2E relies on it)") {
        auto errors = checker.checkText("The the cat sat.");
        REQUIRE(errors.size() == 1);
        REQUIRE(errors[0].ruleId == "REPEATED_WORD");
        REQUIRE(errors[0].text == "The the");
        REQUIRE(errors[0].message == "Repeated word");
        REQUIRE(errors[0].suggestion == "The");
    }
    SECTION("clean text has no repeated-word error") {
        auto errors = checker.checkText("The cat sat.");
        REQUIRE(errors.empty());
    }
    SECTION("disabling the rule silences it") {
        checker.disableRule("REPEATED_WORD");
        REQUIRE(checker.isRuleEnabled("REPEATED_WORD") == false);
        auto errors = checker.checkText("The the cat sat.");
        REQUIRE(errors.empty());
        checker.enableRule("REPEATED_WORD");
        REQUIRE(checker.isRuleEnabled("REPEATED_WORD") == true);
    }
}

TEST_CASE("Grammar dialog - human wording never shows the enum name",
          "[grammar]") {
    SECTION("every rule maps to a human sentence") {
        REQUIRE(grammarRuleDisplayName("REPEATED_WORD") == "Repeated word");
        REQUIRE(grammarRuleDisplayName("DOUBLE_SPACE") == "Double space");
        REQUIRE(grammarRuleDisplayName("SENTENCE_CAPITALIZATION") ==
                "Sentence capitalization");
        REQUIRE(grammarRuleDisplayName("COMMON_ERRORS") == "Common error");
    }
    SECTION("unknown rules fall back, never leak the id") {
        REQUIRE(grammarRuleDisplayName("SOMETHING_NEW") == "Grammar issue");
        REQUIRE(grammarRuleDisplayName("") == "Grammar issue");
        for (const auto& id :
             {"REPEATED_WORD", "DOUBLE_SPACE", "SENTENCE_CAPITALIZATION",
              "COMMON_ERRORS", "SOMETHING_NEW"}) {
            std::string display = grammarRuleDisplayName(id);
            REQUIRE(display.find('_') == std::string::npos);
            REQUIRE(display != id);
        }
    }
    SECTION("ignore key couples rule + phrase") {
        GrammarError a{0, 7, "The the", "Repeated word", "The",
                       "REPEATED_WORD"};
        GrammarError b{0, 7, "The the", "Repeated word", "The",
                       "REPEATED_WORD"};
        GrammarError c{0, 3, "The", "Repeated word", "The", "REPEATED_WORD"};
        REQUIRE(grammarIgnoreKey(a) == grammarIgnoreKey(b));
        REQUIRE(grammarIgnoreKey(a) != grammarIgnoreKey(c));
    }
}

TEST_CASE("Grammar dialog - sentence context for In context:",
          "[grammar]") {
    GrammarChecker checker;

    SECTION("pinned: repeated word shows the whole sentence") {
        auto errors = checker.checkText("The the cat sat.");
        REQUIRE(errors.size() == 1);
        REQUIRE(grammarSentenceContext("The the cat sat.", errors[0]) ==
                "The the cat sat.");
    }
    SECTION("middle sentence of three") {
        std::string text = "First sentence here. The the cat sat. Last one.";
        auto errors = checker.checkText(text);
        REQUIRE(errors.size() == 1);
        REQUIRE(grammarSentenceContext(text, errors[0]) ==
                "The the cat sat.");
    }
    SECTION("double space shows its sentence") {
        std::string text = "A  B";
        auto errors = checker.checkText(text);
        REQUIRE(errors.size() == 1);
        REQUIRE(grammarSentenceContext(text, errors[0]) == "A  B");
    }
    SECTION("empty text has no context") {
        GrammarError err;
        REQUIRE(grammarSentenceContext("", err).empty());
    }
}

TEST_CASE("Grammar dialog - In-context highlight span is sentence-relative",
          "[grammar]") {
    GrammarChecker checker;

    SECTION("repeated word: 'The the' is 0:7 in its sentence") {
        auto errors = checker.checkText("The the cat sat.");
        REQUIRE(errors.size() == 1);
        auto ctx = grammarSentenceContextWithHighlight(
            "The the cat sat.", errors[0]);
        REQUIRE(ctx.sentence == "The the cat sat.");
        REQUIRE(ctx.highlightStart == 0);
        REQUIRE(ctx.highlightLength == 7);
        REQUIRE(ctx.sentence.substr(ctx.highlightStart,
                                    ctx.highlightLength) == "The the");
    }
    SECTION("middle sentence: span is relative to the sentence, not the doc") {
        std::string text = "First sentence here. The the cat sat. Last one.";
        auto errors = checker.checkText(text);
        REQUIRE(errors.size() == 1);
        auto ctx = grammarSentenceContextWithHighlight(text, errors[0]);
        REQUIRE(ctx.sentence == "The the cat sat.");
        REQUIRE(ctx.highlightStart == 0);
        REQUIRE(ctx.highlightLength == 7);
    }
    SECTION("double space inside a sentence") {
        std::string text = "The the cat sat. A  B";
        auto errors = checker.checkText(text);
        REQUIRE(errors.size() == 2);
        auto ctx = grammarSentenceContextWithHighlight(text, errors[1]);
        REQUIRE(ctx.sentence == "A  B");
        REQUIRE(ctx.highlightStart == 1);
        REQUIRE(ctx.highlightLength == 2);
        REQUIRE(ctx.sentence.substr(ctx.highlightStart,
                                    ctx.highlightLength) == "  ");
    }
    SECTION("empty text has an empty span") {
        GrammarError err;
        auto ctx = grammarSentenceContextWithHighlight("", err);
        REQUIRE(ctx.sentence.empty());
        REQUIRE(ctx.highlightStart == 0);
        REQUIRE(ctx.highlightLength == 0);
    }
}

TEST_CASE("Grammar dialog - Change applies the suggestion, one undo step",
          "[grammar]") {
    // Pinned flow, at buffer level (the command layer does exactly
    // this replaceRange, then re-checks): "The the cat sat." Change
    // -> "The cat sat."
    GrammarChecker checker;
    TextBuffer buffer;
    buffer.setText("The the cat sat.");
    auto errors = checker.checkText(buffer.getText());
    REQUIRE(errors.size() == 1);
    REQUIRE(errors[0].suggestion == "The");

    SECTION("Change collapses the repeated word") {
        buffer.replaceRange(errors[0].offset, errors[0].length,
                            errors[0].suggestion);
        REQUIRE(buffer.getText() == "The cat sat.");
        REQUIRE(checker.checkText(buffer.getText()).empty());
    }
    SECTION("Change is a single undo step") {
        buffer.replaceRange(errors[0].offset, errors[0].length,
                            errors[0].suggestion);
        REQUIRE(buffer.getText() == "The cat sat.");
        buffer.undo();
        REQUIRE(buffer.getText() == "The the cat sat.");
    }
}

TEST_CASE("GrammarChecker - capitalization, double spaces, common errors",
          "[grammar]") {
    GrammarChecker checker;

    SECTION("sentence must start with a capital") {
        auto errors = checker.checkText("hello. world");
        REQUIRE(errors.size() == 2);
        REQUIRE(errors[0].ruleId == "SENTENCE_CAPITALIZATION");
        REQUIRE(errors[0].text == "hello");
        REQUIRE(errors[0].suggestion == "Hello");
        REQUIRE(errors[1].text == "world");
    }
    SECTION("double space") {
        // Capitalized words: lowercase "a" would also trip
        // SENTENCE_CAPITALIZATION, so the fixture uses capitals.
        auto errors = checker.checkText("A  B");
        REQUIRE(errors.size() == 1);
        REQUIRE(errors[0].ruleId == "DOUBLE_SPACE");
        REQUIRE(errors[0].length == 2);
        REQUIRE(errors[0].suggestion == " ");
    }
    SECTION("common error: could of -> could have") {
        auto errors = checker.checkText("I could of gone.");
        REQUIRE(errors.size() == 1);
        REQUIRE(errors[0].ruleId == "COMMON_ERRORS");
        REQUIRE(errors[0].text == "could of");
        REQUIRE(errors[0].suggestion == "could have");
    }
    SECTION("clean sentence is clean") {
        auto errors = checker.checkText("The cat sat.");
        REQUIRE(errors.empty());
    }
    SECTION("available rules list the four engines") {
        auto rules = checker.availableRules();
        REQUIRE(rules.size() == 4);
    }
}
