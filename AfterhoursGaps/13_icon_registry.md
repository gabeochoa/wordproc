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

## Relationship to `image_button`

Afterhours already has an `image_button` primitive in `imm_components.h`:

```cpp
inline ElementResult
image_button(HasUIContext auto &ctx, EntityParent ep_pair,
             afterhours::texture_manager::Texture texture,
             afterhours::texture_manager::Rectangle source_rect,
             ComponentConfig config = ComponentConfig()) {
  // Adds HasImage + HasClickListener components
  auto &img = entity.addComponentIfMissing<ui::HasImage>(texture, source_rect, alignment);
  entity.addComponentIfMissing<HasClickListener>([](Entity &) {});
  return ElementResult{entity.get<HasClickListener>().down, entity};
}
```

### Are They the Same Thing?

**No.** They serve different purposes at different abstraction layers:

| Aspect | `image_button` | `IconRegistry` |
|--------|---------------|----------------|
| **Layer** | UI primitive (rendering) | Data management (lookup) |
| **Purpose** | "Display this texture as clickable" | "What visual represents this action?" |
| **Input** | Raw `Texture` + `Rectangle` | Icon ID → IconInfo |
| **Text support** | None | Fallback symbols, labels |
| **Mirroring** | Not handled | Paired icon relationships |
| **Semantics** | None - just pixels | Full metadata (name, resource, fallback) |

### Why Both Are Needed

- `image_button` = **low-level primitive** for "how to render a clickable image"
- `IconRegistry` = **data layer** for "what visual represents this action"

They should **compose together** via a higher-level helper.

## Proposed Composition: `icon_button`

A new component that bridges both systems:

```cpp
// Higher-level component that uses IconRegistry + image_button
inline ElementResult icon_button(
    HasUIContext auto &ctx, 
    EntityParent ep_pair,
    const std::string& icon_id,  // Lookup from registry
    const std::string& label,     // Always shown (design rule #3)
    ComponentConfig config = ComponentConfig()) {
  
  auto [entity, parent] = deref(ep_pair);
  auto icon = icons().get(icon_id);
  
  // Create a row container: [icon/symbol] [label]
  auto container = div(ctx, ep_pair, 
      ComponentConfig::inherit_from(config, "icon_button")
          .with_flex_direction(FlexDirection::Row)
          .with_gap(Spacing::xs));
  
  if (icon && !icon->resource_path.empty()) {
    // Use sprite primitive for the icon portion
    auto tex = load_texture(icon->resource_path);
    Rectangle src = {0, 0, 
        icon->is_mirrored ? -(float)tex.width : (float)tex.width, 
        (float)tex.height};
    sprite(ctx, mk(container.ent(), 0), tex, src,
           ComponentConfig{}.with_size({pixels(16), pixels(16)}));
  } else if (icon) {
    // Fallback to text symbol
    text(ctx, mk(container.ent(), 0), 
         std::string(1, icon->fallback_symbol),
         ComponentConfig{}.with_size({pixels(16), pixels(16)}));
  }
  
  // Always show label
  text(ctx, mk(container.ent(), 1), label, ComponentConfig{});
  
  // Make the whole container clickable
  container.ent().addComponentIfMissing<HasClickListener>([](Entity&){});
  
  return ElementResult{container.ent().get<HasClickListener>().down, 
                       container.ent()};
}
```

### Composition Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     icon_button()                           │  ← High-level: action-aware
│  Uses IconRegistry to resolve what to show                  │
├─────────────────────────────────────────────────────────────┤
│         ┌──────────────┐    ┌───────────────────┐          │
│         │    sprite    │ OR │ text (fallback)   │          │  ← Low-level primitives
│         │  (texture)   │    │  (text symbol)    │          │
│         └──────────────┘    └───────────────────┘          │
├─────────────────────────────────────────────────────────────┤
│                      + text label                           │  ← Always present
├─────────────────────────────────────────────────────────────┤
│                   HasClickListener                          │  ← Interaction
└─────────────────────────────────────────────────────────────┘
```

### When to Use Each

| Component | Use Case |
|-----------|----------|
| `image_button` | Custom spritesheets, game-specific buttons, raw texture clicks |
| `icon_button` | Toolbar buttons, menu items, any action with semantic meaning |
| `IconRegistry` | Centralized icon data, fallback logic, mirrored pairs |

## What Should Go Into Afterhours

Two additions to the library:

1. **`IconRegistry`** (data layer) - see `src/extracted/icon_registry.h`
2. **`icon_button`** component - composes registry lookup with existing primitives

This keeps `image_button` as the low-level primitive for raw texture clicks, while
`icon_button` provides the higher-level semantic pattern for toolbars and menus.

## Design Principles

1. **Icons are opt-in** - Add meaning that text cannot convey
2. **One action = one icon** - Consistency across the app
3. **Icons supplement text** - Labels are always shown
4. **Minimal detail** - Icons must be legible at 16x16
5. **Paired actions mirror** - Undo/redo, indent/outdent, prev/next
6. **Central registry** - Single source of truth prevents duplicates
7. **Layered composition** - Registry (data) + primitives (rendering) = icon_button (semantic)

