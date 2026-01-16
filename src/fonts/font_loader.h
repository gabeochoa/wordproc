#pragma once

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

namespace fonts {

// Font metadata for UI display and selection
struct FontInfo {
    std::string name;  // Display name (e.g., "Gaegu Bold")
    std::string
        internalId;        // Internal ID used for loading (e.g., "Gaegu-Bold")
    std::string filename;  // Filename (e.g., "Gaegu-Bold.ttf")
    bool isDefault = false;  // Whether this is the default font
    bool supportsUnicode =
        false;                 // Whether this font supports extended Unicode
    std::string languageHint;  // Hint for what language this font supports
};

// Priority levels for font loading
enum class FontPriority {
    P0_Startup,   // Required for initial UI (must load before window shows)
    P1_Document,  // Loaded when documents specify them
    P2_Optional   // Available for selection but loaded on-demand
};

// Font loading result
struct FontLoadResult {
    bool success = false;
    std::string error;
};

// FontLoader manages all font resources for the word processor
class FontLoader {
   public:
    // Singleton access
    static FontLoader& get();

    // P0: Load fonts required for startup UI
    template<typename FontManager>
    bool loadStartupFonts(FontManager& fontManager);

    // P1: Load a font from a file path (for document-specified fonts)
    template<typename FontManager>
    FontLoadResult loadFontFromFile(FontManager& fontManager,
                                    const std::string& fontId,
                                    const std::string& filePath);

    // P2: Get list of available fonts for the font picker UI
    std::vector<FontInfo> getAvailableFonts() const;

    // Get the default font ID
    std::string getDefaultFontId() const;

    // Check if a font is loaded
    bool isFontLoaded(const std::string& fontId) const;

    // Get font info by ID
    std::optional<FontInfo> getFontInfo(const std::string& fontId) const;

   private:
    FontLoader();
    void registerBuiltinFonts();

    std::vector<FontInfo> builtinFonts_;
    std::vector<std::string> loadedFonts_;
};

// Template implementations
template<typename FontManager>
bool FontLoader::loadStartupFonts(FontManager& /*fontManager*/) {
    loadedFonts_.clear();
    for (const auto& font : builtinFonts_) {
        if (font.supportsUnicode && !font.languageHint.empty()) {
            continue;  // CJK fonts loaded separately
        }
        loadedFonts_.push_back(font.internalId);
    }
    return true;
}

template<typename FontManager>
FontLoadResult FontLoader::loadFontFromFile(FontManager& fontManager,
                                            const std::string& fontId,
                                            const std::string& filePath) {
    FontLoadResult result;
    if (isFontLoaded(fontId)) {
        result.success = true;
        return result;
    }
    try {
        fontManager.load_font(fontId.c_str(), filePath.c_str());
        loadedFonts_.push_back(fontId);
        result.success = true;
    } catch (...) {
        result.success = false;
        result.error = "Failed to load font from: " + filePath;
    }
    return result;
}

}  // namespace fonts
