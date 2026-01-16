#include "font_loader.h"

namespace fonts {

FontLoader& FontLoader::get() {
    static FontLoader instance;
    return instance;
}

FontLoader::FontLoader() {
    registerBuiltinFonts();
}

void FontLoader::registerBuiltinFonts() {
    // P0: Required startup fonts
    builtinFonts_.push_back({
        "Gaegu Bold", "Gaegu-Bold", "Gaegu-Bold.ttf",
        true, false, ""
    });
    builtinFonts_.push_back({
        "EqPro Rounded", "EqProRounded", "eqprorounded-regular.ttf",
        false, false, ""
    });
    builtinFonts_.push_back({
        "EB Garamond", "Garamond", "EBGaramond-Regular.ttf",
        false, false, ""
    });
    builtinFonts_.push_back({
        "Nerd Symbols", "NerdSymbols", "SymbolsNerdFont-Regular.ttf",
        false, false, ""
    });
    builtinFonts_.push_back({
        "Fredoka", "Fredoka", "Fredoka-VariableFont_wdth,wght.ttf",
        false, false, ""
    });
    builtinFonts_.push_back({
        "Black Ops One", "BlackOpsOne", "BlackOpsOne-Regular.ttf",
        false, false, ""
    });
    builtinFonts_.push_back({
        "Atkinson Hyperlegible", "Atkinson", "AtkinsonHyperlegible-Regular.ttf",
        false, false, ""
    });
    // CJK fonts
    builtinFonts_.push_back({
        "Noto Sans Korean", "NotoSansKR", "NotoSansMonoCJKkr-Bold.otf",
        false, true, "Korean"
    });
    builtinFonts_.push_back({
        "Sazanami Japanese", "Sazanami", "Sazanami-Hanazono-Mincho.ttf",
        false, true, "Japanese"
    });
}

std::vector<FontInfo> FontLoader::getAvailableFonts() const {
    std::vector<FontInfo> fonts = builtinFonts_;
    std::sort(fonts.begin(), fonts.end(), [](const FontInfo& a, const FontInfo& b) {
        if (a.isDefault != b.isDefault) return a.isDefault > b.isDefault;
        return a.name < b.name;
    });
    return fonts;
}

std::string FontLoader::getDefaultFontId() const {
    for (const auto& font : builtinFonts_) {
        if (font.isDefault) return font.internalId;
    }
    return "Gaegu-Bold";
}

bool FontLoader::isFontLoaded(const std::string& fontId) const {
    return std::find(loadedFonts_.begin(), loadedFonts_.end(), fontId) 
           != loadedFonts_.end();
}

std::optional<FontInfo> FontLoader::getFontInfo(const std::string& fontId) const {
    for (const auto& font : builtinFonts_) {
        if (font.internalId == fontId) return font;
    }
    return std::nullopt;
}

} // namespace fonts
