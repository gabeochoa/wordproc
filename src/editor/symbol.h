#pragma once

// Insert > Special Character...: the grid of insertable symbols.
//
// Every entry is verified present in the UI/document font
// (Roboto-Regular, the face the dialog and the document render
// with): the full required set — © ® ™ § ¶ • – — ' ' " " « »
// € £ ¥ ° ± ½ ÷ × … plus lowercase Greek alpha-omega — is in
// Roboto's cmap (checked with a cmap parse of
// resources/fonts/Roboto-Regular.ttf; EB Garamond covers them
// too), so nothing is dropped from the grid. A glyph missing
// from Roboto would render blank in both the dialog and the
// document, so such a glyph must not be added here without a
// font that actually carries it.

#include <string>
#include <vector>

namespace symbol {

struct Entry {
    const char* character;  // UTF-8 character
    const char* name;       // Display name ("Copyright")
    const char* slug;       // Debug-name slug ("copyright")
};

// The grid, in display order: punctuation/symbols first, then
// Greek alpha-omega.
inline const std::vector<Entry>& entries() {
    static const std::vector<Entry> kEntries = {
        {"\xC2\xA9", "Copyright", "copyright"},
        {"\xC2\xAE", "Registered", "registered"},
        {"\xE2\x84\xA2", "Trademark", "trademark"},
        {"\xC2\xA7", "Section", "section"},
        {"\xC2\xB6", "Pilcrow", "pilcrow"},
        {"\xE2\x80\xA2", "Bullet", "bullet"},
        {"\xE2\x80\x93", "En dash", "en_dash"},
        {"\xE2\x80\x94", "Em dash", "em_dash"},
        {"\xE2\x80\x98", "Left single quote", "left_single_quote"},
        {"\xE2\x80\x99", "Right single quote", "right_single_quote"},
        {"\xE2\x80\x9C", "Left double quote", "left_double_quote"},
        {"\xE2\x80\x9D", "Right double quote", "right_double_quote"},
        {"\xC2\xAB", "Left guillemet", "left_guillemet"},
        {"\xC2\xBB", "Right guillemet", "right_guillemet"},
        {"\xE2\x82\xAC", "Euro", "euro"},
        {"\xC2\xA3", "Pound", "pound"},
        {"\xC2\xA5", "Yen", "yen"},
        {"\xC2\xB0", "Degree", "degree"},
        {"\xC2\xB1", "Plus-minus", "plus_minus"},
        {"\xC2\xBD", "One half", "one_half"},
        {"\xC3\xB7", "Divide", "divide"},
        {"\xC3\x97", "Multiply", "multiply"},
        {"\xE2\x80\xA6", "Ellipsis", "ellipsis"},
        {"\xCE\xB1", "Alpha", "alpha"},
        {"\xCE\xB2", "Beta", "beta"},
        {"\xCE\xB3", "Gamma", "gamma"},
        {"\xCE\xB4", "Delta", "delta"},
        {"\xCE\xB5", "Epsilon", "epsilon"},
        {"\xCE\xB6", "Zeta", "zeta"},
        {"\xCE\xB7", "Eta", "eta"},
        {"\xCE\xB8", "Theta", "theta"},
        {"\xCE\xB9", "Iota", "iota"},
        {"\xCE\xBA", "Kappa", "kappa"},
        {"\xCE\xBB", "Lambda", "lambda"},
        {"\xCE\xBC", "Mu", "mu"},
        {"\xCE\xBD", "Nu", "nu"},
        {"\xCE\xBE", "Xi", "xi"},
        {"\xCE\xBF", "Omicron", "omicron"},
        {"\xCF\x80", "Pi", "pi"},
        {"\xCF\x81", "Rho", "rho"},
        {"\xCF\x83", "Sigma", "sigma"},
        {"\xCF\x84", "Tau", "tau"},
        {"\xCF\x85", "Upsilon", "upsilon"},
        {"\xCF\x86", "Phi", "phi"},
        {"\xCF\x87", "Chi", "chi"},
        {"\xCF\x88", "Psi", "psi"},
        {"\xCF\x89", "Omega", "omega"},
    };
    return kEntries;
}

// Grid width: the dialog lays entries out in rows of this many.
inline constexpr std::size_t kColumns = 8;

}  // namespace symbol
