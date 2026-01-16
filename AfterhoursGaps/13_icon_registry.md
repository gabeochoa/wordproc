# Icon Registry

## Working Implementation
See `src/ui/icon_registry.h` for a complete working example.

Extracted clean version: `src/extracted/icon_registry.h`

## Problem
Afterhours does not provide a centralized icon management system. Games and apps
often need to:
- Map actions/items to icons consistently
- Provide fallback symbols when icons aren't loaded
- Handle mirrored icon pairs (undo/redo, left/right arrows)
- Control which icons are approved for use

## Use Cases

### Games
- **Inventory**: Map item IDs to icons (sword, potion, armor)
- **Skills/Abilities**: Fireball icon, heal icon, buff icons
- **Status Effects**: Poison, burn, shield icons
- **HUD**: Health, mana, stamina icons
- **Minimap**: Player, enemy, objective markers

### Level Editors
- **Tools**: Select, move, rotate, scale icons
- **Object palette**: Spawn points, triggers, decorations
- **Layers**: Show/hide, lock icons

### Apps
- **Toolbar**: File operations, formatting, navigation
- **Menus**: Action icons next to text labels

## Key Features

### Central Registry
One source of truth for all icons:
```cpp
auto icon = IconRegistry::get(IconId::Save);
if (icon) {
    draw_icon(icon->resource_path, x, y);
}
```

### Fallback Text Symbols
When icons aren't loaded or for minimal UIs:
```cpp
char symbol = IconRegistry::get_symbol(IconId::Undo);  // Returns '<'
```

### Mirrored Icon Pairs
Paired actions share the same base icon, flipped:
```cpp
// Undo uses left arrow, Redo uses same arrow mirrored
{IconId::Undo, {..., mirror: false}},
{IconId::Redo, {..., mirror: true, mirror_of: IconId::Undo}},
```

### Approval System
Only approved icons are used (prevents icon sprawl):
```cpp
if (IconRegistry::has_approved_icon(action)) {
    show_icon();
} else {
    show_text_only();
}
```

## Proposed API for Afterhours

```cpp
namespace afterhours::ui {

// Icon identifier (user defines their own enum)
// template<typename IconId>
// Or use string-based IDs for flexibility

struct IconInfo {
    std::string name;           // Display name
    std::string resource_path;  // Path to texture/sprite
    char fallback_symbol;       // Text fallback
    bool is_mirrored;          // Flip horizontally when drawing
};

class IconRegistry {
public:
    // Register an icon
    void register_icon(const std::string& id, IconInfo info);
    
    // Get icon info (returns nullopt if not registered)
    std::optional<IconInfo> get(const std::string& id) const;
    
    // Get fallback symbol
    char get_symbol(const std::string& id) const;
    
    // Check if icon is registered
    bool has_icon(const std::string& id) const;
    
    // Set mirroring relationship
    void set_mirror(const std::string& id, const std::string& mirror_of);
    
    // Load all icons from a directory (convention: filename = id)
    void load_from_directory(const std::string& path);
    
private:
    std::unordered_map<std::string, IconInfo> icons_;
};

// Global instance (or could be per-context)
IconRegistry& icons();

} // namespace afterhours::ui
```

## Integration with UI Components

```cpp
// In button/menu rendering
void draw_button(const std::string& action_id, const std::string& label) {
    auto icon = icons().get(action_id);
    
    if (icon && icon->resource_path.size() > 0) {
        // Draw icon
        auto texture = load_texture(icon->resource_path);
        draw_texture(texture, x, y, icon->is_mirrored ? FLIP_H : 0);
        x += icon_width + padding;
    } else if (icon) {
        // Draw fallback symbol
        draw_text(std::string(1, icon->fallback_symbol), x, y);
        x += char_width + padding;
    }
    
    // Always draw label (icons supplement, don't replace)
    draw_text(label, x, y);
}
```

## Design Principles

1. **Icons are opt-in** - Add meaning that text cannot convey
2. **One action = one icon** - Consistency across the app
3. **Icons supplement text** - Labels are always shown
4. **Minimal detail** - Icons must be legible at 16x16
5. **Paired actions mirror** - Undo/redo, indent/outdent, prev/next
6. **Central registry** - Single source of truth prevents duplicates

