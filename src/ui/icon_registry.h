#pragma once

#include <string>
#include <unordered_map>
#include <optional>

// Icon registry maps actions to approved icons
// Design rule: One action = one icon. Icons add meaning that text cannot.
//
// Usage:
//   auto icon = IconRegistry::getIcon(Action::Undo);
//   if (icon) drawIcon(*icon, x, y);
//
// Icon sources (Win95/Mac 3.1 style):
// - Icons should be 16x16 or 32x32 pixels
// - Pixel-aligned, minimal detail, clear silhouettes
// - Paired actions (undo/redo) use mirrored metaphors

namespace IconRegistry {

// Icon identifiers - these map to actual icon resources
// For now we use text labels; can be replaced with actual icon paths/sprites
enum class IconId {
    // File operations
    New,
    Open,
    Save,
    SaveAs,
    Print,
    
    // Edit operations
    Undo,
    Redo,
    Cut,
    Copy,
    Paste,
    Delete,
    SelectAll,
    Find,
    Replace,
    
    // Format operations
    Bold,
    Italic,
    Underline,
    Strikethrough,
    AlignLeft,
    AlignCenter,
    AlignRight,
    AlignJustify,
    BulletList,
    NumberList,
    IndentIncrease,
    IndentDecrease,
    
    // Insert operations
    InsertImage,
    InsertTable,
    InsertLink,
    InsertPageBreak,
    InsertShape,
    InsertEquation,
    
    // View operations
    ZoomIn,
    ZoomOut,
    ZoomReset,
    ShowGrid,
    ShowRuler,
    
    // Navigation
    GoToTop,
    GoToBottom,
    PageUp,
    PageDown,
    NextBookmark,
    PrevBookmark,
    
    // Help
    Help,
    About,
    
    // No icon (placeholder)
    None
};

// Icon metadata
struct IconInfo {
    IconId id;
    std::string name;           // Display name for debugging
    std::string resourcePath;   // Path to icon resource (empty = no visual icon)
    char textSymbol;            // Fallback text symbol (for Win95 style)
    bool useMirror;            // True if this is the mirrored version of another icon
    IconId mirrorOf;           // If mirrored, the base icon
};

// Get icon info for an action (returns nullopt if no icon assigned)
inline std::optional<IconInfo> getIcon(IconId id) {
    static const std::unordered_map<IconId, IconInfo> registry = {
        // File operations - simple document metaphors
        {IconId::New,      {IconId::New, "New", "", '+', false, IconId::None}},
        {IconId::Open,     {IconId::Open, "Open", "", 'O', false, IconId::None}},
        {IconId::Save,     {IconId::Save, "Save", "", 'S', false, IconId::None}},
        {IconId::Print,    {IconId::Print, "Print", "", 'P', false, IconId::None}},
        
        // Edit operations - paired undo/redo use mirrored arrows
        {IconId::Undo,     {IconId::Undo, "Undo", "", '<', false, IconId::None}},
        {IconId::Redo,     {IconId::Redo, "Redo", "", '>', true, IconId::Undo}},
        {IconId::Cut,      {IconId::Cut, "Cut", "", 'X', false, IconId::None}},
        {IconId::Copy,     {IconId::Copy, "Copy", "", 'C', false, IconId::None}},
        {IconId::Paste,    {IconId::Paste, "Paste", "", 'V', false, IconId::None}},
        
        // Format operations
        {IconId::Bold,         {IconId::Bold, "Bold", "", 'B', false, IconId::None}},
        {IconId::Italic,       {IconId::Italic, "Italic", "", 'I', false, IconId::None}},
        {IconId::Underline,    {IconId::Underline, "Underline", "", 'U', false, IconId::None}},
        {IconId::AlignLeft,    {IconId::AlignLeft, "Align Left", "", '[', false, IconId::None}},
        {IconId::AlignCenter,  {IconId::AlignCenter, "Align Center", "", '|', false, IconId::None}},
        {IconId::AlignRight,   {IconId::AlignRight, "Align Right", "", ']', true, IconId::AlignLeft}},
        
        // Indent uses mirrored arrows
        {IconId::IndentIncrease, {IconId::IndentIncrease, "Indent", "", '>', false, IconId::None}},
        {IconId::IndentDecrease, {IconId::IndentDecrease, "Outdent", "", '<', true, IconId::IndentIncrease}},
        
        // Navigation
        {IconId::ZoomIn,   {IconId::ZoomIn, "Zoom In", "", '+', false, IconId::None}},
        {IconId::ZoomOut,  {IconId::ZoomOut, "Zoom Out", "", '-', false, IconId::None}},
        
        // Help
        {IconId::Help,     {IconId::Help, "Help", "", '?', false, IconId::None}},
    };
    
    auto it = registry.find(id);
    if (it != registry.end()) {
        return it->second;
    }
    return std::nullopt;
}

// Check if an icon is approved for use
inline bool hasApprovedIcon(IconId id) {
    return getIcon(id).has_value();
}

// Get the text symbol fallback for an icon
inline char getIconSymbol(IconId id) {
    auto info = getIcon(id);
    return info ? info->textSymbol : ' ';
}

// Check if icons should be shown (based on UI density preference)
// For Win95 style, icons are optional and text labels are primary
inline bool shouldShowIcons() {
    // Default: icons add meaning but are not required
    // Return true to show icons in toolbar/menus where available
    return true;
}

// Design rules for icon usage:
// 1. Icons are opt-in only - add meaning that text cannot
// 2. Each action has at most one icon (consistency)
// 3. Icons must be legible at 16x16 (minimal detail)
// 4. Paired actions use mirrored metaphors (undo/redo, indent/outdent)
// 5. Icons never replace text labels - labels are always shown
// 6. Use standard Win95 metaphors where applicable

}  // namespace IconRegistry
