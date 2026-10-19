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
//
// The dialog has two tabs: "Symbols" shows the whole grid;
// "Special Characters" shows the punctuation/currency subset
// (isSpecial) with Word-style shortcut hints (shortcut; empty
// when Word assigns none — the hints are decorative text, not
// key bindings). The bottom readout shows the selection's code
// + formal Unicode name ("U+00A9 Copyright Sign"). The Font
// dropdown lists the loaded families (fontChoices); the grid
// re-renders in that face — a glyph the chosen face lacks draws
// blank, like Word — so the choices are the faces the FontManager
// actually registered (Roboto is the "__default" face), not the
// toolbar names (Times New Roman / Arial / Courier New are
// names, not loaded faces).

#include <cstdint>
#include <string>
#include <vector>

namespace symbol {

struct Entry {
    const char* character;  // UTF-8 character
    const char* name;       // Display name ("Copyright")
    const char* slug;       // Debug-name slug ("copyright")
    // Formal Unicode data for the bottom readout.
    std::uint32_t codepoint = 0;  // e.g. 0x00A9
    const char* unicodeName = "";  // e.g. "Copyright Sign"
    // Word-style shortcut hint for the Special Characters tab;
    // "" when Word assigns none. Decorative, not a binding.
    const char* shortcut = "";
    // In the Special Characters tab (punctuation/currency
    // subset): all non-Greek entries.
    bool isSpecial = false;
};

// "U+00A9": uppercase hex, at least four digits.
inline std::string codeLabel(const Entry& entry) {
    static constexpr char kHex[] = "0123456789ABCDEF";
    std::string hex;
    std::uint32_t cp = entry.codepoint;
    do {
        hex.insert(hex.begin(), kHex[cp & 0xF]);
        cp >>= 4;
    } while (cp != 0);
    if (hex.size() < 4) hex.insert(hex.begin(), 4 - hex.size(), '0');
    return "U+" + hex;
}

// The bottom readout: "U+00A9 Copyright Sign".
inline std::string readout(const Entry& entry) {
    return codeLabel(entry) + " " + entry.unicodeName;
}

// The grid, in display order: punctuation/symbols first, then
// Greek alpha-omega.
inline const std::vector<Entry>& entries() {
    static const std::vector<Entry> kEntries = {
        // character, name, slug, codepoint, unicodeName, shortcut, isSpecial
        {"\xC2\xA9", "Copyright", "copyright", 0x00A9, "Copyright Sign",
         "Alt+Ctrl+C", true},
        {"\xC2\xAE", "Registered", "registered", 0x00AE, "Registered Sign",
         "Alt+Ctrl+R", true},
        {"\xE2\x84\xA2", "Trademark", "trademark", 0x2122,
         "Trade Mark Sign", "Alt+Ctrl+T", true},
        {"\xC2\xA7", "Section", "section", 0x00A7, "Section Sign", "",
         true},
        {"\xC2\xB6", "Pilcrow", "pilcrow", 0x00B6, "Pilcrow Sign", "",
         true},
        {"\xE2\x80\xA2", "Bullet", "bullet", 0x2022, "Bullet", "", true},
        {"\xE2\x80\x93", "En dash", "en_dash", 0x2013, "En Dash",
         "Ctrl+Num -", true},
        {"\xE2\x80\x94", "Em dash", "em_dash", 0x2014, "Em Dash",
         "Alt+Ctrl+Num -", true},
        {"\xE2\x80\x98", "Left single quote", "left_single_quote", 0x2018,
         "Left Single Quotation Mark", "", true},
        {"\xE2\x80\x99", "Right single quote", "right_single_quote", 0x2019,
         "Right Single Quotation Mark", "", true},
        {"\xE2\x80\x9C", "Left double quote", "left_double_quote", 0x201C,
         "Left Double Quotation Mark", "", true},
        {"\xE2\x80\x9D", "Right double quote", "right_double_quote", 0x201D,
         "Right Double Quotation Mark", "", true},
        {"\xC2\xAB", "Left guillemet", "left_guillemet", 0x00AB,
         "Left-Pointing Double Angle Quotation Mark", "", true},
        {"\xC2\xBB", "Right guillemet", "right_guillemet", 0x00BB,
         "Right-Pointing Double Angle Quotation Mark", "", true},
        {"\xE2\x82\xAC", "Euro", "euro", 0x20AC, "Euro Sign", "", true},
        {"\xC2\xA3", "Pound", "pound", 0x00A3, "Pound Sign", "", true},
        {"\xC2\xA5", "Yen", "yen", 0x00A5, "Yen Sign", "", true},
        {"\xC2\xB0", "Degree", "degree", 0x00B0, "Degree Sign", "", true},
        {"\xC2\xB1", "Plus-minus", "plus_minus", 0x00B1, "Plus-Minus Sign",
         "", true},
        {"\xC2\xBD", "One half", "one_half", 0x00BD,
         "Vulgar Fraction One Half", "", true},
        {"\xC3\xB7", "Divide", "divide", 0x00F7, "Division Sign", "",
         true},
        {"\xC3\x97", "Multiply", "multiply", 0x00D7, "Multiplication Sign",
         "", true},
        {"\xE2\x80\xA6", "Ellipsis", "ellipsis", 0x2026,
         "Horizontal Ellipsis", "Alt+Ctrl+.", true},
        {"\xCE\xB1", "Alpha", "alpha", 0x03B1,
         "Greek Small Letter Alpha", "", false},
        {"\xCE\xB2", "Beta", "beta", 0x03B2, "Greek Small Letter Beta",
         "", false},
        {"\xCE\xB3", "Gamma", "gamma", 0x03B3, "Greek Small Letter Gamma",
         "", false},
        {"\xCE\xB4", "Delta", "delta", 0x03B4, "Greek Small Letter Delta",
         "", false},
        {"\xCE\xB5", "Epsilon", "epsilon", 0x03B5,
         "Greek Small Letter Epsilon", "", false},
        {"\xCE\xB6", "Zeta", "zeta", 0x03B6, "Greek Small Letter Zeta",
         "", false},
        {"\xCE\xB7", "Eta", "eta", 0x03B7, "Greek Small Letter Eta", "",
         false},
        {"\xCE\xB8", "Theta", "theta", 0x03B8, "Greek Small Letter Theta",
         "", false},
        {"\xCE\xB9", "Iota", "iota", 0x03B9, "Greek Small Letter Iota",
         "", false},
        {"\xCE\xBA", "Kappa", "kappa", 0x03BA, "Greek Small Letter Kappa",
         "", false},
        {"\xCE\xBB", "Lambda", "lambda", 0x03BB,
         "Greek Small Letter Lambda", "", false},
        {"\xCE\xBC", "Mu", "mu", 0x03BC, "Greek Small Letter Mu", "",
         false},
        {"\xCE\xBD", "Nu", "nu", 0x03BD, "Greek Small Letter Nu", "",
         false},
        {"\xCE\xBE", "Xi", "xi", 0x03BE, "Greek Small Letter Xi", "",
         false},
        {"\xCE\xBF", "Omicron", "omicron", 0x03BF,
         "Greek Small Letter Omicron", "", false},
        {"\xCF\x80", "Pi", "pi", 0x03C0, "Greek Small Letter Pi", "",
         false},
        {"\xCF\x81", "Rho", "rho", 0x03C1, "Greek Small Letter Rho", "",
         false},
        {"\xCF\x83", "Sigma", "sigma", 0x03C3, "Greek Small Letter Sigma",
         "", false},
        {"\xCF\x84", "Tau", "tau", 0x03C4, "Greek Small Letter Tau", "",
         false},
        {"\xCF\x85", "Upsilon", "upsilon", 0x03C5,
         "Greek Small Letter Upsilon", "", false},
        {"\xCF\x86", "Phi", "phi", 0x03C6, "Greek Small Letter Phi", "",
         false},
        {"\xCF\x87", "Chi", "chi", 0x03C7, "Greek Small Letter Chi", "",
         false},
        {"\xCF\x88", "Psi", "psi", 0x03C8, "Greek Small Letter Psi", "",
         false},
        {"\xCF\x89", "Omega", "omega", 0x03C9, "Greek Small Letter Omega",
         "", false},
    };
    return kEntries;
}

// The Special Characters tab: the isSpecial entries, in grid
// order, as indices into entries().
inline std::vector<std::size_t> specialIndices() {
    std::vector<std::size_t> out;
    const auto& all = entries();
    for (std::size_t i = 0; i < all.size(); ++i) {
        if (all[i].isSpecial) out.push_back(i);
    }
    return out;
}

// Grid width: the dialog lays entries out in rows of this many.
inline constexpr std::size_t kColumns = 8;

// A loaded family for the Font dropdown: the display name plus
// the FontManager key the grid cells render with
// (ComponentConfig::with_font). Roboto is the "__default" face;
// the others are registered under their own names in preload.
struct FontChoice {
    const char* display;    // Dropdown label ("Roboto")
    const char* managerKey;  // FontManager name ("__default")
    const char* slug;       // Debug-name slug ("roboto")
};

inline const std::vector<FontChoice>& fontChoices() {
    static const std::vector<FontChoice> kFonts = {
        {"Roboto", "__default", "roboto"},
        {"Garamond", "Garamond", "garamond"},
        {"Gaegu", "Gaegu", "gaegu"},
        {"Gaegu-Bold", "Gaegu-Bold", "gaegu_bold"},
    };
    return kFonts;
}

}  // namespace symbol
