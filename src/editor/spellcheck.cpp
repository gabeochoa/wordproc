#include "spellcheck.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

// ============================================================================
// SpellChecker Implementation
// ============================================================================

SpellChecker::SpellChecker() { initDefaultDictionary(); }

void SpellChecker::initDefaultDictionary() {
    // Common English words - this is a minimal dictionary for v0.1
    // A full implementation would load from a file (e.g., /usr/share/dict/words)
    static const char* commonWords[] = {
        // Greetings and common words
        "hello", "world", "hi", "bye", "goodbye", "yes", "no", "ok", "okay",
        "please", "thank", "thanks", "sorry", "welcome", "here", "there",
        
        // Articles and pronouns
        "a", "an", "the", "i", "you", "he", "she", "it", "we", "they", "me",
        "him", "her", "us", "them", "my", "your", "his", "its", "our", "their",
        "mine", "yours", "hers", "ours", "theirs", "this", "that", "these",
        "those", "who", "whom", "whose", "which", "what",

        // Common verbs
        "is", "am", "are", "was", "were", "be", "been", "being", "have", "has",
        "had", "having", "do", "does", "did", "doing", "will", "would", "shall",
        "should", "may", "might", "must", "can", "could", "go", "goes", "went",
        "gone", "going", "come", "comes", "came", "coming", "get", "gets",
        "got", "getting", "make", "makes", "made", "making", "take", "takes",
        "took", "taken", "taking", "see", "sees", "saw", "seen", "seeing",
        "know", "knows", "knew", "known", "knowing", "think", "thinks",
        "thought", "thinking", "say", "says", "said", "saying", "give", "gives",
        "gave", "given", "giving", "find", "finds", "found", "finding", "tell",
        "tells", "told", "telling", "ask", "asks", "asked", "asking", "use",
        "uses", "used", "using", "seem", "seems", "seemed", "seeming", "leave",
        "leaves", "left", "leaving", "call", "calls", "called", "calling",
        "keep", "keeps", "kept", "keeping", "let", "lets", "letting", "begin",
        "begins", "began", "begun", "beginning", "help", "helps", "helped",
        "helping", "show", "shows", "showed", "shown", "showing", "hear",
        "hears", "heard", "hearing", "play", "plays", "played", "playing",
        "run", "runs", "ran", "running", "move", "moves", "moved", "moving",
        "live", "lives", "lived", "living", "believe", "believes", "believed",
        "believing", "hold", "holds", "held", "holding", "bring", "brings",
        "brought", "bringing", "write", "writes", "wrote", "written", "writing",
        "read", "reads", "reading", "learn", "learns", "learned", "learning",
        "change", "changes", "changed", "changing", "follow", "follows",
        "followed", "following", "stop", "stops", "stopped", "stopping",
        "create", "creates", "created", "creating", "open", "opens", "opened",
        "opening", "close", "closes", "closed", "closing", "work", "works",
        "worked", "working", "need", "needs", "needed", "needing", "feel",
        "feels", "felt", "feeling", "become", "becomes", "became", "becoming",
        "start", "starts", "started", "starting", "try", "tries", "tried",
        "trying", "want", "wants", "wanted", "wanting", "like", "likes",
        "liked", "liking", "look", "looks", "looked", "looking", "put", "puts",
        "putting", "mean", "means", "meant", "meaning", "set", "sets",
        "setting", "turn", "turns", "turned", "turning",

        // Common nouns
        "time", "year", "people", "way", "day", "man", "woman", "child",
        "children", "world", "life", "hand", "part", "place", "case", "week",
        "company", "system", "program", "question", "work", "government",
        "number", "night", "point", "home", "water", "room", "mother", "area",
        "money", "story", "fact", "month", "lot", "right", "study", "book",
        "eye", "job", "word", "business", "issue", "side", "kind", "head",
        "house", "service", "friend", "father", "power", "hour", "game",
        "line", "end", "member", "law", "car", "city", "community", "name",
        "president", "team", "minute", "idea", "kid", "body", "information",
        "back", "parent", "face", "others", "level", "office", "door",
        "health", "person", "art", "war", "history", "party", "result",
        "change", "morning", "reason", "research", "girl", "guy", "moment",
        "air", "teacher", "force", "education",

        // Animals
        "dog", "cat", "fox", "bird", "fish", "horse", "cow", "pig", "sheep",
        "lion", "tiger", "bear", "wolf", "rabbit", "mouse", "rat", "deer",
        "elephant", "monkey", "snake", "frog", "duck", "chicken", "turkey",

        // Common adjectives
        "good", "new", "first", "last", "long", "great", "little", "own",
        "other", "old", "right", "big", "high", "different", "small", "large",
        "next", "early", "young", "important", "few", "public", "bad", "same",
        "able", "best", "better", "sure", "free", "true", "whole", "special",
        "easy", "clear", "recent", "certain", "personal", "open", "red",
        "blue", "green", "black", "white", "brown", "yellow", "purple", "orange",
        "short", "full", "wrong", "real",
        "local", "hard", "major", "strong", "happy", "serious", "ready",
        "simple", "possible", "nice", "beautiful", "quick", "fast", "slow",

        // Common adverbs
        "not", "just", "also", "very", "often", "however", "too", "usually",
        "really", "early", "never", "always", "sometimes", "together", "likely",
        "simply", "generally", "instead", "actually", "already", "ever",
        "probably", "maybe", "perhaps", "finally", "quickly", "slowly",
        "directly", "recently", "suddenly", "certainly", "clearly",

        // Prepositions and conjunctions
        "to", "of", "in", "for", "on", "with", "at", "by", "from", "up",
        "about", "into", "over", "after", "beneath", "under", "above",
        "between", "out", "through", "during", "before", "without", "again",
        "and", "or", "but", "if", "because", "as", "until", "while", "although",
        "though", "since", "unless", "so", "than", "when", "where", "why",
        "how", "whether", "both", "either", "neither", "each", "every", "all",
        "any", "some", "no", "none", "most", "many", "much", "more", "less",
        "such", "even", "only", "just", "now", "then", "here", "there",

        // Numbers
        "one", "two", "three", "four", "five", "six", "seven", "eight", "nine",
        "ten", "hundred", "thousand", "million", "billion", "first", "second",
        "third", "fourth", "fifth",

        // Technology and document-related words
        "file", "document", "text", "word", "page", "paragraph", "font",
        "style", "format", "save", "open", "close", "edit", "copy", "paste",
        "cut", "undo", "redo", "print", "search", "find", "replace", "insert",
        "delete", "select", "menu", "button", "window", "dialog", "option",
        "setting", "preference", "computer", "software", "application", "app",
        "program", "data", "information", "email", "internet", "website",
        "online", "offline", "user", "password", "login", "logout", "account",
        "profile", "image", "picture", "photo", "video", "audio", "music",
        "screen", "display", "keyboard", "mouse", "click", "type", "enter",
        "escape", "tab", "space", "shift", "control", "alt", "command",

        // Common contractions (without apostrophe for simplicity)
        "dont", "wont", "cant", "isnt", "arent", "wasnt", "werent", "hasnt",
        "havent", "hadnt", "doesnt", "didnt", "wouldnt", "couldnt", "shouldnt",
        "mightnt", "mustnt", "neednt", "thats", "whats", "heres", "theres",
        "wheres", "hows", "whys", "whos", "im", "youre", "hes", "shes", "its",
        "were", "theyre", "ive", "youve", "weve", "theyve", "id", "youd",
        "hed", "shed", "wed", "theyd", "ill", "youll", "hell", "shell", "well",
        "theyll",

        // Days and months
        "monday", "tuesday", "wednesday", "thursday", "friday", "saturday",
        "sunday", "january", "february", "march", "april", "may", "june",
        "july", "august", "september", "october", "november", "december",

        nullptr  // Sentinel
    };

    for (const char** p = commonWords; *p; ++p) {
        dictionary_.insert(*p);
    }
}

bool SpellChecker::isWordChar(char ch) {
    return std::isalpha(static_cast<unsigned char>(ch)) || ch == '\'';
}

std::string SpellChecker::normalizeWord(const std::string& word) {
    std::string result;
    result.reserve(word.size());
    for (char ch : word) {
        if (ch != '\'') {  // Remove apostrophes for lookup
            result += static_cast<char>(
                std::tolower(static_cast<unsigned char>(ch)));
        }
    }
    return result;
}

std::vector<std::pair<std::size_t, std::string>> SpellChecker::extractWords(
    const std::string& text) {
    std::vector<std::pair<std::size_t, std::string>> words;
    std::size_t i = 0;
    while (i < text.size()) {
        // Skip non-word characters
        while (i < text.size() && !isWordChar(text[i])) {
            ++i;
        }
        if (i >= text.size()) break;

        // Extract word
        std::size_t start = i;
        while (i < text.size() && isWordChar(text[i])) {
            ++i;
        }
        if (i > start) {
            words.emplace_back(start, text.substr(start, i - start));
        }
    }
    return words;
}

bool SpellChecker::isCorrect(const std::string& word) const {
    if (word.empty()) return true;

    // Skip words that are all uppercase (likely acronyms)
    bool allUpper = true;
    for (char ch : word) {
        if (ch != '\'' && !std::isupper(static_cast<unsigned char>(ch))) {
            allUpper = false;
            break;
        }
    }
    if (allUpper && word.size() >= 2) return true;

    // Skip words with numbers
    for (char ch : word) {
        if (std::isdigit(static_cast<unsigned char>(ch))) return true;
    }

    std::string normalized = normalizeWord(word);
    if (normalized.empty()) return true;

    // Single letters are always correct
    if (normalized.size() == 1) return true;

    // Check ignore list first
    if (ignoreList_.count(normalized)) return true;

    // Check user dictionary
    if (userDictionary_.count(normalized)) return true;

    // Check main dictionary
    return dictionary_.count(normalized) > 0;
}

std::size_t SpellChecker::editDistance(const std::string& a,
                                       const std::string& b) {
    const std::size_t m = a.size();
    const std::size_t n = b.size();

    // Use two rows instead of full matrix for space efficiency
    std::vector<std::size_t> prev(n + 1);
    std::vector<std::size_t> curr(n + 1);

    // Initialize first row
    for (std::size_t j = 0; j <= n; ++j) {
        prev[j] = j;
    }

    for (std::size_t i = 1; i <= m; ++i) {
        curr[0] = i;
        for (std::size_t j = 1; j <= n; ++j) {
            if (std::tolower(static_cast<unsigned char>(a[i - 1])) ==
                std::tolower(static_cast<unsigned char>(b[j - 1]))) {
                curr[j] = prev[j - 1];
            } else {
                curr[j] = 1 + std::min({prev[j - 1],  // substitution
                                        prev[j],      // deletion
                                        curr[j - 1]   // insertion
                                       });
            }
        }
        std::swap(prev, curr);
    }

    return prev[n];
}

std::vector<std::string> SpellChecker::getSuggestions(
    const std::string& word, std::size_t maxSuggestions) const {
    std::string normalized = normalizeWord(word);
    if (normalized.empty()) return {};

    // If word is already correct, no suggestions needed
    if (isCorrect(word)) return {};

    // Collect candidates with their edit distances
    std::vector<std::pair<std::size_t, std::string>> candidates;

    // Check dictionary words with reasonable edit distance
    std::size_t maxDistance = std::min<std::size_t>(3, normalized.size() / 2 + 1);

    auto checkDictionary = [&](const std::unordered_set<std::string>& dict) {
        for (const auto& dictWord : dict) {
            // Quick length filter
            if (dictWord.size() > normalized.size() + maxDistance ||
                dictWord.size() + maxDistance < normalized.size()) {
                continue;
            }

            std::size_t dist = editDistance(normalized, dictWord);
            if (dist > 0 && dist <= maxDistance) {
                candidates.emplace_back(dist, dictWord);
            }
        }
    };

    checkDictionary(dictionary_);
    checkDictionary(userDictionary_);

    // Sort by edit distance, then alphabetically
    std::sort(candidates.begin(), candidates.end(),
              [](const auto& a, const auto& b) {
                  if (a.first != b.first) return a.first < b.first;
                  return a.second < b.second;
              });

    // Return top suggestions
    std::vector<std::string> result;
    result.reserve(std::min(maxSuggestions, candidates.size()));
    for (std::size_t i = 0; i < candidates.size() && i < maxSuggestions; ++i) {
        result.push_back(candidates[i].second);
    }

    return result;
}

std::vector<SpellingError> SpellChecker::checkText(
    const std::string& text) const {
    std::vector<SpellingError> errors;
    auto words = extractWords(text);

    for (const auto& [offset, word] : words) {
        if (!isCorrect(word)) {
            SpellingError error;
            error.offset = offset;
            error.length = word.size();
            error.word = word;
            error.suggestions = getSuggestions(word);
            errors.push_back(std::move(error));
        }
    }

    return errors;
}

bool SpellChecker::checkWord(const std::string& text, std::size_t offset,
                             std::size_t length, SpellingError& outError) const {
    if (offset + length > text.size()) return false;

    std::string word = text.substr(offset, length);
    if (isCorrect(word)) return false;

    outError.offset = offset;
    outError.length = length;
    outError.word = word;
    outError.suggestions = getSuggestions(word);
    return true;
}

void SpellChecker::addToUserDictionary(const std::string& word) {
    std::string normalized = normalizeWord(word);
    if (!normalized.empty()) {
        userDictionary_.insert(normalized);
        ignoreList_.erase(normalized);  // Remove from ignore if added to dict
    }
}

void SpellChecker::removeFromUserDictionary(const std::string& word) {
    userDictionary_.erase(normalizeWord(word));
}

bool SpellChecker::isInUserDictionary(const std::string& word) const {
    return userDictionary_.count(normalizeWord(word)) > 0;
}

void SpellChecker::clearUserDictionary() { userDictionary_.clear(); }

void SpellChecker::ignoreWord(const std::string& word) {
    std::string normalized = normalizeWord(word);
    if (!normalized.empty()) {
        ignoreList_.insert(normalized);
    }
}

void SpellChecker::clearIgnoreList() { ignoreList_.clear(); }

bool SpellChecker::isIgnored(const std::string& word) const {
    return ignoreList_.count(normalizeWord(word)) > 0;
}

bool SpellChecker::loadUserDictionary(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::string word;
    while (std::getline(file, word)) {
        if (!word.empty()) {
            userDictionary_.insert(normalizeWord(word));
        }
    }
    return true;
}

bool SpellChecker::saveUserDictionary(const std::string& path) const {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    for (const auto& word : userDictionary_) {
        file << word << '\n';
    }
    return true;
}

// ============================================================================
// GrammarChecker Implementation
// ============================================================================

std::vector<std::string> GrammarChecker::availableRules() const {
    return {"DOUBLE_SPACE", "SENTENCE_CAPITALIZATION", "REPEATED_WORD",
            "COMMON_ERRORS"};
}

bool GrammarChecker::isRuleEnabled(const std::string& ruleId) const {
    return disabledRules_.count(ruleId) == 0;
}

void GrammarChecker::enableRule(const std::string& ruleId) {
    disabledRules_.erase(ruleId);
}

void GrammarChecker::disableRule(const std::string& ruleId) {
    disabledRules_.insert(ruleId);
}

std::vector<GrammarError> GrammarChecker::checkText(
    const std::string& text) const {
    std::vector<GrammarError> errors;

    if (isRuleEnabled("DOUBLE_SPACE")) {
        checkDoubleSpaces(text, errors);
    }
    if (isRuleEnabled("SENTENCE_CAPITALIZATION")) {
        checkCapitalization(text, errors);
    }
    if (isRuleEnabled("REPEATED_WORD")) {
        checkRepeatedWords(text, errors);
    }
    if (isRuleEnabled("COMMON_ERRORS")) {
        checkCommonErrors(text, errors);
    }

    // Sort by offset
    std::sort(errors.begin(), errors.end(),
              [](const auto& a, const auto& b) { return a.offset < b.offset; });

    return errors;
}

void GrammarChecker::checkDoubleSpaces(const std::string& text,
                                       std::vector<GrammarError>& errors) const {
    for (std::size_t i = 0; i + 1 < text.size(); ++i) {
        if (text[i] == ' ' && text[i + 1] == ' ') {
            // Count consecutive spaces
            std::size_t end = i + 2;
            while (end < text.size() && text[end] == ' ') {
                ++end;
            }

            GrammarError error;
            error.offset = i;
            error.length = end - i;
            error.text = text.substr(i, end - i);
            error.message = "Multiple consecutive spaces";
            error.suggestion = " ";
            error.ruleId = "DOUBLE_SPACE";
            errors.push_back(std::move(error));

            i = end - 1;  // Skip past the spaces
        }
    }
}

void GrammarChecker::checkCapitalization(
    const std::string& text, std::vector<GrammarError>& errors) const {
    bool sentenceStart = true;

    for (std::size_t i = 0; i < text.size(); ++i) {
        char ch = text[i];

        if (ch == '.' || ch == '!' || ch == '?') {
            sentenceStart = true;
        } else if (std::isalpha(static_cast<unsigned char>(ch))) {
            if (sentenceStart) {
                if (std::islower(static_cast<unsigned char>(ch))) {
                    // Find word boundary
                    std::size_t end = i + 1;
                    while (end < text.size() &&
                           std::isalpha(static_cast<unsigned char>(text[end]))) {
                        ++end;
                    }

                    GrammarError error;
                    error.offset = i;
                    error.length = end - i;
                    error.text = text.substr(i, end - i);
                    error.message = "Sentence should start with capital letter";
                    std::string suggestion = error.text;
                    suggestion[0] = static_cast<char>(
                        std::toupper(static_cast<unsigned char>(suggestion[0])));
                    error.suggestion = suggestion;
                    error.ruleId = "SENTENCE_CAPITALIZATION";
                    errors.push_back(std::move(error));
                }
                sentenceStart = false;
            }
        } else if (ch != ' ' && ch != '\n' && ch != '\t') {
            sentenceStart = false;
        }
    }
}

void GrammarChecker::checkRepeatedWords(
    const std::string& text, std::vector<GrammarError>& errors) const {
    auto words = SpellChecker::extractWords(text);

    for (std::size_t i = 1; i < words.size(); ++i) {
        const auto& prev = words[i - 1];
        const auto& curr = words[i];

        // Check if same word repeated
        std::string prevNorm = SpellChecker::normalizeWord(prev.second);
        std::string currNorm = SpellChecker::normalizeWord(curr.second);

        if (prevNorm == currNorm && !prevNorm.empty()) {
            GrammarError error;
            error.offset = prev.first;
            error.length = curr.first + curr.second.size() - prev.first;
            error.text = text.substr(error.offset, error.length);
            error.message = "Repeated word";
            error.suggestion = prev.second;
            error.ruleId = "REPEATED_WORD";
            errors.push_back(std::move(error));
        }
    }
}

void GrammarChecker::checkCommonErrors(
    const std::string& text, std::vector<GrammarError>& errors) const {
    // Common word pair errors
    static const struct {
        const char* wrong;
        const char* correct;
        const char* message;
    } commonErrors[] = {
        {"your welcome", "you're welcome", "Use \"you're\" (you are)"},
        {"could of", "could have", "Use \"could have\" instead of \"could of\""},
        {"should of", "should have", "Use \"should have\" instead of \"should of\""},
        {"would of", "would have", "Use \"would have\" instead of \"would of\""},
        {"alot", "a lot", "\"A lot\" should be two words"},
        {"definately", "definitely", "Correct spelling is \"definitely\""},
        {"seperate", "separate", "Correct spelling is \"separate\""},
        {"occured", "occurred", "Correct spelling is \"occurred\""},
        {"recieve", "receive", "Correct spelling is \"receive\""},
        {"untill", "until", "Correct spelling is \"until\""},
        {"wierd", "weird", "Correct spelling is \"weird\""},
        {"thier", "their", "Correct spelling is \"their\""},
        {"truely", "truly", "Correct spelling is \"truly\""},
        {"accomodate", "accommodate", "Correct spelling is \"accommodate\""},
        {"occurence", "occurrence", "Correct spelling is \"occurrence\""},
        {nullptr, nullptr, nullptr}  // Sentinel
    };

    std::string textLower;
    textLower.reserve(text.size());
    for (char ch : text) {
        textLower += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }

    for (const auto* p = commonErrors; p->wrong; ++p) {
        std::size_t pos = 0;
        std::string wrongLower = p->wrong;
        while ((pos = textLower.find(wrongLower, pos)) != std::string::npos) {
            // Check word boundaries
            bool startOk = (pos == 0 ||
                            !SpellChecker::isWordChar(text[pos - 1]));
            bool endOk = (pos + wrongLower.size() >= text.size() ||
                          !SpellChecker::isWordChar(text[pos + wrongLower.size()]));

            if (startOk && endOk) {
                GrammarError error;
                error.offset = pos;
                error.length = wrongLower.size();
                error.text = text.substr(pos, wrongLower.size());
                error.message = p->message;
                error.suggestion = p->correct;
                error.ruleId = "COMMON_ERRORS";
                errors.push_back(std::move(error));
            }
            pos += wrongLower.size();
        }
    }
}
