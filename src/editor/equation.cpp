#include "equation.h"
#include <algorithm>

// EquationCollection Implementation
std::size_t EquationCollection::addEquation(const DocumentEquation& eq) {
    DocumentEquation newEq = eq;
    if (newEq.id == 0) {
        newEq.id = nextId_++;
    }
    equations_.push_back(newEq);
    return newEq.id;
}

bool EquationCollection::removeEquation(std::size_t id) {
    auto it = std::find_if(equations_.begin(), equations_.end(),
                          [id](const DocumentEquation& eq) { return eq.id == id; });
    if (it != equations_.end()) {
        equations_.erase(it);
        return true;
    }
    return false;
}

DocumentEquation* EquationCollection::getEquation(std::size_t id) {
    for (auto& eq : equations_) {
        if (eq.id == id) return &eq;
    }
    return nullptr;
}

const DocumentEquation* EquationCollection::getEquation(std::size_t id) const {
    for (const auto& eq : equations_) {
        if (eq.id == id) return &eq;
    }
    return nullptr;
}

std::vector<DocumentEquation*> EquationCollection::equationsAtLine(std::size_t line) {
    std::vector<DocumentEquation*> result;
    for (auto& eq : equations_) {
        if (eq.anchorLine == line) result.push_back(&eq);
    }
    return result;
}

std::vector<const DocumentEquation*> EquationCollection::equationsAtLine(std::size_t line) const {
    std::vector<const DocumentEquation*> result;
    for (const auto& eq : equations_) {
        if (eq.anchorLine == line) result.push_back(&eq);
    }
    return result;
}

void EquationCollection::shiftAnchorsFrom(std::size_t line, std::ptrdiff_t linesDelta) {
    for (auto& eq : equations_) {
        if (eq.anchorLine >= line) {
            auto newLine = static_cast<std::ptrdiff_t>(eq.anchorLine) + linesDelta;
            eq.anchorLine = static_cast<std::size_t>(std::max(std::ptrdiff_t(0), newLine));
        }
    }
}

void EquationCollection::clear() {
    equations_.clear();
}

// Special Characters
static const std::vector<SpecialCharacter> greekLetters = {
    {"\xCE\xB1", "Alpha", nullptr, CharacterCategory::Greek},
    {"\xCE\xB2", "Beta", nullptr, CharacterCategory::Greek},
    {"\xCF\x80", "Pi", nullptr, CharacterCategory::Greek},
};

static const std::vector<SpecialCharacter> mathSymbols = {
    {"\xC2\xB1", "Plus-minus", nullptr, CharacterCategory::Mathematical},
    {"\xE2\x88\x9E", "Infinity", nullptr, CharacterCategory::Mathematical},
};

static const std::vector<SpecialCharacter> emptyChars;

const std::vector<SpecialCharacter>& getGreekLetters() { return greekLetters; }
const std::vector<SpecialCharacter>& getMathSymbols() { return mathSymbols; }
const std::vector<SpecialCharacter>& getArrowSymbols() { return emptyChars; }
const std::vector<SpecialCharacter>& getSubscriptChars() { return emptyChars; }
const std::vector<SpecialCharacter>& getSuperscriptChars() { return emptyChars; }
const std::vector<SpecialCharacter>& getCurrencySymbols() { return emptyChars; }
const std::vector<SpecialCharacter>& getFractionChars() { return emptyChars; }
const std::vector<SpecialCharacter>& getPunctuationChars() { return emptyChars; }
const std::vector<SpecialCharacter>& getMiscSymbols() { return emptyChars; }

const std::vector<SpecialCharacter>& getSpecialCharacters(CharacterCategory category) {
    switch (category) {
        case CharacterCategory::Greek: return greekLetters;
        case CharacterCategory::Mathematical: return mathSymbols;
        case CharacterCategory::Arrows:
        case CharacterCategory::Subscript:
        case CharacterCategory::Superscript:
        case CharacterCategory::Currency:
        case CharacterCategory::Punctuation:
        case CharacterCategory::Symbols:
        case CharacterCategory::Fractions:
        case CharacterCategory::Other:
        default:
            return emptyChars;
    }
}

const SpecialCharacter* findSpecialCharacter(const std::string& name) {
    for (const auto& ch : greekLetters) {
        if (std::string(ch.name) == name) return &ch;
    }
    for (const auto& ch : mathSymbols) {
        if (std::string(ch.name) == name) return &ch;
    }
    return nullptr;
}

std::string convertLatexToUnicode(const std::string& latex) {
    std::string result = latex;
    // Simple substitutions
    size_t pos;
    while ((pos = result.find("\\alpha")) != std::string::npos) result.replace(pos, 6, "\xCE\xB1");
    while ((pos = result.find("\\pi")) != std::string::npos) result.replace(pos, 3, "\xCF\x80");
    while ((pos = result.find("\\pm")) != std::string::npos) result.replace(pos, 3, "\xC2\xB1");
    while ((pos = result.find("\\infty")) != std::string::npos) result.replace(pos, 6, "\xE2\x88\x9E");
    return result;
}

std::string applySubSuperscript(const std::string& text) {
    std::string result = text;
    size_t pos;
    while ((pos = result.find("^2")) != std::string::npos) result.replace(pos, 2, "\xC2\xB2");
    while ((pos = result.find("_2")) != std::string::npos) result.replace(pos, 2, "\xE2\x82\x82");
    return result;
}
