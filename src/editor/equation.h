#pragma once

#include <cstddef>
#include <string>
#include <vector>

// Equation display style
enum class EquationStyle {
    Inline,   // Displayed within text flow
    Display   // Centered on its own line, larger size
};

// Get display name for equation style
inline const char* equationStyleName(EquationStyle style) {
    switch (style) {
        case EquationStyle::Inline: return "Inline";
        case EquationStyle::Display: return "Display";
        default: return "Inline";
    }
}

// Document equation structure
struct DocumentEquation {
    // LaTeX-like equation source text
    std::string source;
    
    // Position in document
    std::size_t anchorLine = 0;
    std::size_t anchorColumn = 0;
    
    // Display properties
    EquationStyle style = EquationStyle::Inline;
    float fontSize = 16.0f;
    
    // Unique identifier
    std::size_t id = 0;
    
    // Whether equation is valid/renderable
    bool isValid = true;
    std::string errorMessage;
    
    // Helper to check if this is an inline equation
    bool isInline() const { return style == EquationStyle::Inline; }
};

// Equation collection for a document
class EquationCollection {
public:
    EquationCollection() = default;
    
    // Add an equation
    std::size_t addEquation(const DocumentEquation& eq);
    
    // Get equation by ID
    DocumentEquation* getEquation(std::size_t id);
    const DocumentEquation* getEquation(std::size_t id) const;
    
    // Remove equation by ID
    bool removeEquation(std::size_t id);
    
    // Get all equations
    const std::vector<DocumentEquation>& equations() const { return equations_; }
    std::vector<DocumentEquation>& equations() { return equations_; }
    
    // Get equations at a line
    std::vector<DocumentEquation*> equationsAtLine(std::size_t line);
    std::vector<const DocumentEquation*> equationsAtLine(std::size_t line) const;
    
    // Update anchor positions after text edits
    void shiftAnchorsFrom(std::size_t line, std::ptrdiff_t linesDelta);
    
    // Clear all equations
    void clear();
    
    // Count
    std::size_t count() const { return equations_.size(); }
    bool isEmpty() const { return equations_.empty(); }
    
private:
    std::vector<DocumentEquation> equations_;
    std::size_t nextId_ = 1;
};

// ============================================================================
// Special Characters
// ============================================================================

// Category of special characters
enum class CharacterCategory {
    Greek,           // Greek letters (α, β, γ, etc.)
    Mathematical,    // Math operators and symbols (±, ×, ÷, √, etc.)
    Arrows,          // Arrow symbols (→, ←, ↑, ↓, etc.)
    Subscript,       // Subscript digits and letters (₀, ₁, ₐ, etc.)
    Superscript,     // Superscript digits and letters (⁰, ¹, ², etc.)
    Currency,        // Currency symbols (€, £, ¥, etc.)
    Punctuation,     // Special punctuation (—, –, …, etc.)
    Symbols,         // Misc symbols (©, ®, ™, °, etc.)
    Fractions,       // Fraction characters (½, ⅓, ¼, etc.)
    Other
};

// Get display name for character category
inline const char* characterCategoryName(CharacterCategory cat) {
    switch (cat) {
        case CharacterCategory::Greek: return "Greek Letters";
        case CharacterCategory::Mathematical: return "Mathematical";
        case CharacterCategory::Arrows: return "Arrows";
        case CharacterCategory::Subscript: return "Subscript";
        case CharacterCategory::Superscript: return "Superscript";
        case CharacterCategory::Currency: return "Currency";
        case CharacterCategory::Punctuation: return "Punctuation";
        case CharacterCategory::Symbols: return "Symbols";
        case CharacterCategory::Fractions: return "Fractions";
        case CharacterCategory::Other: return "Other";
        default: return "Other";
    }
}

// Special character entry
struct SpecialCharacter {
    const char* character;     // UTF-8 character
    const char* name;          // Display name
    const char* shortcut;      // Keyboard shortcut (null if none)
    CharacterCategory category;
};

// Get all special characters for a category
const std::vector<SpecialCharacter>& getSpecialCharacters(CharacterCategory category);

// Get all Greek letters
const std::vector<SpecialCharacter>& getGreekLetters();

// Get all mathematical symbols
const std::vector<SpecialCharacter>& getMathSymbols();

// Get all arrow symbols
const std::vector<SpecialCharacter>& getArrowSymbols();

// Get all subscript characters
const std::vector<SpecialCharacter>& getSubscriptChars();

// Get all superscript characters
const std::vector<SpecialCharacter>& getSuperscriptChars();

// Get all currency symbols
const std::vector<SpecialCharacter>& getCurrencySymbols();

// Get all fraction characters
const std::vector<SpecialCharacter>& getFractionChars();

// Get all special punctuation
const std::vector<SpecialCharacter>& getPunctuationChars();

// Get all misc symbols
const std::vector<SpecialCharacter>& getMiscSymbols();

// Find a special character by name (case-insensitive)
const SpecialCharacter* findSpecialCharacter(const std::string& name);

// Convert text with LaTeX-like commands to UTF-8 with special chars
// e.g., "\\alpha" -> "α", "\\pm" -> "±"
std::string convertLatexToUnicode(const std::string& latex);

// Convert superscript/subscript text
// e.g., "x^2" -> "x²", "H_2O" -> "H₂O"
std::string applySubSuperscript(const std::string& text);
