// Wrapper that uses the extracted e2e_testing.h implementation
// This file maintains backward compatibility with existing code.

#pragma once

#include "../extracted/e2e_testing.h"

namespace test_input {

// Singleton wrapper for backward compatibility
class VisibleTextRegistry {
public:
    static VisibleTextRegistry& instance() {
        static VisibleTextRegistry inst;
        return inst;
    }
    
    void clear() {
        afterhours::testing::visible_text::clear();
    }
    
    void registerText(const std::string& text) {
        afterhours::testing::visible_text::register_text(text);
    }
    
    bool containsText(const std::string& needle) const {
        return afterhours::testing::visible_text::contains(needle);
    }
    
    bool hasExactText(const std::string& needle) const {
        return afterhours::testing::visible_text::has_exact(needle);
    }
    
    std::string getAllText() const {
        return afterhours::testing::visible_text::get_all();
    }
    
    std::vector<std::string> getTexts() const {
        return afterhours::testing::visible_text::Registry::instance().get_texts();
    }

private:
    VisibleTextRegistry() = default;
};

// Convenience functions
inline void registerVisibleText(const std::string& text) {
    afterhours::testing::visible_text::register_text(text);
}

inline void clearVisibleTextRegistry() {
    afterhours::testing::visible_text::clear();
}

}  // namespace test_input
