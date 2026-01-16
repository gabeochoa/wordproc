#pragma once

// Registry for tracking all text rendered on screen
// Used by E2E tests to validate visible UI text

#include <string>
#include <vector>
#include <mutex>

namespace test_input {

// Singleton registry for visible text
class VisibleTextRegistry {
public:
    static VisibleTextRegistry& instance() {
        static VisibleTextRegistry instance;
        return instance;
    }
    
    // Clear all registered text (call at start of each frame)
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        texts_.clear();
    }
    
    // Register text that was drawn on screen
    void registerText(const std::string& text) {
        if (text.empty()) return;
        std::lock_guard<std::mutex> lock(mutex_);
        texts_.push_back(text);
    }
    
    // Check if a specific text is visible (substring match)
    bool containsText(const std::string& needle) const {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& text : texts_) {
            if (text.find(needle) != std::string::npos) {
                return true;
            }
        }
        return false;
    }
    
    // Check if exact text is visible
    bool hasExactText(const std::string& needle) const {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& text : texts_) {
            if (text == needle) {
                return true;
            }
        }
        return false;
    }
    
    // Get all visible text as a single string (for debugging)
    std::string getAllText() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string result;
        for (const auto& text : texts_) {
            if (!result.empty()) result += " | ";
            result += text;
        }
        return result;
    }
    
    // Get all visible texts as a vector
    std::vector<std::string> getTexts() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return texts_;
    }
    
private:
    VisibleTextRegistry() = default;
    
    mutable std::mutex mutex_;
    std::vector<std::string> texts_;
};

// Convenience function to register text
inline void registerVisibleText(const std::string& text) {
    VisibleTextRegistry::instance().registerText(text);
}

// Convenience function to clear registry
inline void clearVisibleTextRegistry() {
    VisibleTextRegistry::instance().clear();
}

}  // namespace test_input

