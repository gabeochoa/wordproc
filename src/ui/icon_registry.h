#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "../input/action_map.h"

// Icon identifier type
using IconId = std::string;

// Icon metadata
struct IconInfo {
    IconId id;                    // Unique identifier (e.g., "save", "undo")
    const char* name;             // Display name for accessibility
    const char* description;      // Tooltip/description
    bool isPaired = false;        // Part of a paired action (undo/redo, etc.)
    IconId pairedWith;            // ID of paired icon (if isPaired)
};

// Icon registry - maps actions to approved icons
// Ensures one action = one icon, consistent metaphors across the app
class IconRegistry {
public:
    static IconRegistry& instance() {
        static IconRegistry registry;
        return registry;
    }
    
    // Get icon for an action
    const IconInfo* iconForAction(Action action) const {
        auto it = actionToIcon_.find(action);
        if (it != actionToIcon_.end()) {
            return &it->second;
        }
        return nullptr;
    }
    
    // Check if an action has an approved icon
    bool hasIcon(Action action) const {
        return actionToIcon_.count(action) > 0;
    }
    
    // Get all icons (for auditing/display)
    const std::unordered_map<Action, IconInfo>& allIcons() const {
        return actionToIcon_;
    }
    
    // Get paired actions (for consistent metaphor verification)
    const std::vector<std::pair<Action, Action>>& pairedActions() const {
        return pairedActions_;
    }
    
private:
    IconRegistry() {
        // Register approved icons for actions
        // Icons are opt-in only - actions without icons here won't display icons
        
        // File operations
        registerIcon(Action::NewDocument, {"new", "New", "Create new document"});
        registerIcon(Action::Open, {"open", "Open", "Open document"});
        registerIcon(Action::Save, {"save", "Save", "Save document"});
        registerIcon(Action::Print, {"print", "Print", "Print document"});
        
        // Edit operations (paired)
        registerPairedIcons(
            Action::Undo, {"undo", "Undo", "Undo last action"},
            Action::Redo, {"redo", "Redo", "Redo last action"}
        );
        
        registerIcon(Action::Cut, {"cut", "Cut", "Cut selection"});
        registerIcon(Action::Copy, {"copy", "Copy", "Copy selection"});
        registerIcon(Action::Paste, {"paste", "Paste", "Paste from clipboard"});
        
        // Formatting
        registerIcon(Action::Bold, {"bold", "Bold", "Toggle bold"});
        registerIcon(Action::Italic, {"italic", "Italic", "Toggle italic"});
        registerIcon(Action::Underline, {"underline", "Underline", "Toggle underline"});
        
        // Alignment (related group - use similar visual style)
        registerIcon(Action::AlignLeft, {"align-left", "Align Left", "Align text left"});
        registerIcon(Action::AlignCenter, {"align-center", "Align Center", "Center text"});
        registerIcon(Action::AlignRight, {"align-right", "Align Right", "Align text right"});
        registerIcon(Action::AlignJustify, {"align-justify", "Justify", "Justify text"});
        
        // Lists (paired concept)
        registerPairedIcons(
            Action::BulletedList, {"list-bullet", "Bullets", "Toggle bulleted list"},
            Action::NumberedList, {"list-numbered", "Numbering", "Toggle numbered list"}
        );
        
        // Indentation (paired)
        registerPairedIcons(
            Action::IndentIncrease, {"indent-increase", "Increase Indent", "Increase indentation"},
            Action::IndentDecrease, {"indent-decrease", "Decrease Indent", "Decrease indentation"}
        );
        
        // Find
        registerIcon(Action::Find, {"find", "Find", "Find text"});
        registerIcon(Action::Replace, {"replace", "Replace", "Find and replace"});
        
        // Zoom (paired)
        registerPairedIcons(
            Action::ZoomIn, {"zoom-in", "Zoom In", "Increase zoom"},
            Action::ZoomOut, {"zoom-out", "Zoom Out", "Decrease zoom"}
        );
    }
    
    void registerIcon(Action action, IconInfo info) {
        actionToIcon_[action] = info;
    }
    
    void registerPairedIcons(Action action1, IconInfo info1, 
                             Action action2, IconInfo info2) {
        info1.isPaired = true;
        info1.pairedWith = info2.id;
        info2.isPaired = true;
        info2.pairedWith = info1.id;
        
        actionToIcon_[action1] = info1;
        actionToIcon_[action2] = info2;
        pairedActions_.emplace_back(action1, action2);
    }
    
    std::unordered_map<Action, IconInfo> actionToIcon_;
    std::vector<std::pair<Action, Action>> pairedActions_;
};

// Helper function to check if an action should show an icon
inline bool shouldShowIcon(Action action) {
    return IconRegistry::instance().hasIcon(action);
}

// Helper function to get icon ID for an action
inline const char* getIconId(Action action) {
    const IconInfo* info = IconRegistry::instance().iconForAction(action);
    return info ? info->id.c_str() : nullptr;
}

// Helper function to get icon name for accessibility
inline const char* getIconName(Action action) {
    const IconInfo* info = IconRegistry::instance().iconForAction(action);
    return info ? info->name : nullptr;
}
