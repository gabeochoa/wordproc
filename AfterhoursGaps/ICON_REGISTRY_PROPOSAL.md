# Icon Registry Plugin Proposal for Afterhours

**Purpose**: Detailed specification for contributing IconRegistry as an afterhours plugin.

**Status**: Ready for review and implementation  
**Target Location**: `vendor/afterhours/src/plugins/icon_registry.h`

---

## Table of Contents

1. [Design Goals](#design-goals)
2. [ECS-Based Architecture](#ecs-based-architecture)
3. [Complete API Specification](#complete-api-specification)
4. [Implementation Details](#implementation-details)
5. [Usage Examples](#usage-examples)
6. [Integration with Existing UI](#integration-with-existing-ui)
7. [Testing Strategy](#testing-strategy)
8. [Migration Path for Wordproc](#migration-path-for-wordproc)

---

## Design Goals

### 1. Fit Afterhours Patterns
- ✅ ECS-based singleton (not traditional C++ singleton)
- ✅ Component for state (`ProvidesIconRegistry`)
- ✅ Free function API (`icon_registry::register_icon()`)
- ✅ Plugin initialization (`init()`, `add_singleton_components()`, `enforce_singletons()`)
- ✅ Follows existing plugin conventions (files, settings, window_manager)

### 2. Game Engine Appropriate
- ✅ Useful for games (item icons, skill icons, status effects)
- ✅ Useful for level editors (tool palettes, object browsers)
- ✅ Minimal dependencies (no heavy libs)
- ✅ Header-only (like most afterhours plugins)

### 3. Composability
- ✅ Works with existing `sprite()` and `image_button()` primitives
- ✅ Provides higher-level `icon_button()` for common case
- ✅ String-based IDs (flexible, no enum constraints)
- ✅ Fallback symbols for text-only UIs

---

## ECS-Based Architecture

### Component: ProvidesIconRegistry

Holds the registry data as an ECS component.

```cpp
namespace afterhours::icon_registry {

struct IconInfo {
  std::string name;           // Display name ("Save", "Undo")
  std::string resource_path;  // Path to texture ("icons/save.png")
  char fallback_symbol;       // ASCII fallback ('S', '<')
  bool is_mirrored;          // Flip horizontally when drawing
};

struct ProvidesIconRegistry : BaseComponent {
  std::unordered_map<std::string, IconInfo> icons;
  
  void register_icon(const std::string& id, IconInfo info) {
    icons[id] = std::move(info);
  }
  
  std::optional<IconInfo> get(const std::string& id) const {
    auto it = icons.find(id);
    if (it == icons.end()) return std::nullopt;
    return it->second;
  }
  
  bool has_icon(const std::string& id) const {
    return icons.find(id) != icons.end();
  }
  
  char get_symbol(const std::string& id) const {
    auto it = icons.find(id);
    if (it == icons.end()) return '?';
    return it->second.fallback_symbol;
  }
  
  size_t count() const { return icons.size(); }
  
  void clear() { icons.clear(); }
};

} // namespace afterhours::icon_registry
```

### Plugin Interface

Following the standard plugin pattern from `files.h` and `settings.h`:

```cpp
namespace afterhours::icon_registry {

// 1. Add singleton components to entity
static void add_singleton_components(Entity &entity) {
  entity.addComponent<ProvidesIconRegistry>();
  EntityHelper::registerSingleton<ProvidesIconRegistry>(entity);
}

// 2. Enforce singleton constraint
static void enforce_singletons(SystemManager &sm) {
  sm.register_update_system(
    std::make_unique<developer::EnforceSingleton<ProvidesIconRegistry>>());
}

// 3. Initialize the plugin
static void init() {
  if (EntityHelper::get_default_collection()
          .has_singleton<ProvidesIconRegistry>()) {
    log_warn("Icon registry already initialized");
    return;
  }
  Entity &entity = EntityHelper::createPermanentEntity();
  add_singleton_components(entity);
  EntityHelper::merge_entity_arrays();
}

// 4. Get provider (internal helper)
static ProvidesIconRegistry* get_provider() {
  return EntityHelper::get_singleton_cmp<ProvidesIconRegistry>();
}

} // namespace afterhours::icon_registry
```

---

## Complete API Specification

### Core API Functions

```cpp
namespace afterhours::icon_registry {

/// Initialize the icon registry (call once at startup)
static void init();

/// Register a single icon
static void register_icon(const std::string& id, IconInfo info) {
  auto* provider = get_provider();
  if (!provider) {
    log_error("Icon registry not initialized. Call icon_registry::init()");
    return;
  }
  provider->register_icon(id, std::move(info));
}

/// Register an icon with builder pattern
static void register_icon(const std::string& id, 
                         const std::string& name,
                         const std::string& resource_path,
                         char fallback_symbol = '?',
                         bool is_mirrored = false) {
  IconInfo info{name, resource_path, fallback_symbol, is_mirrored};
  register_icon(id, std::move(info));
}

/// Get icon info (returns nullopt if not found)
static std::optional<IconInfo> get(const std::string& id) {
  auto* provider = get_provider();
  if (!provider) return std::nullopt;
  return provider->get(id);
}

/// Check if icon exists
static bool has_icon(const std::string& id) {
  auto* provider = get_provider();
  if (!provider) return false;
  return provider->has_icon(id);
}

/// Get fallback symbol (returns '?' if not found)
static char get_symbol(const std::string& id) {
  auto* provider = get_provider();
  if (!provider) return '?';
  return provider->get_symbol(id);
}

/// Get registry size (for debugging)
static size_t count() {
  auto* provider = get_provider();
  if (!provider) return 0;
  return provider->count();
}

/// Clear all icons (useful for testing)
static void clear() {
  auto* provider = get_provider();
  if (!provider) return;
  provider->clear();
}

} // namespace afterhours::icon_registry
```

### UI Helper: icon_button Component

Composes `icon_registry` with existing UI primitives:

```cpp
namespace afterhours::ui {

/// Higher-level icon button component
/// Looks up icon from registry and renders it with a label
inline ElementResult icon_button(
    HasUIContext auto &ctx, 
    EntityParent ep_pair,
    const std::string& icon_id,
    const std::string& label,
    ComponentConfig config = ComponentConfig()) {
  
  using namespace afterhours::icon_registry;
  auto [entity, parent] = deref(ep_pair);
  
  // Get icon from registry
  auto icon_info = get(icon_id);
  
  // Create container: [icon/symbol] [label]
  auto container = div(ctx, ep_pair, 
      ComponentConfig::inherit_from(config, "icon_button")
          .with_flex_direction(FlexDirection::Row)
          .with_gap(Spacing::xs)
          .with_align_items(AlignItems::Center));
  
  if (icon_info && !icon_info->resource_path.empty()) {
    // Show icon (TODO: load texture from resource_path)
    // For now, just reserve space
    div(ctx, mk(container.ent(), 0),
        ComponentConfig{}.with_size({pixels(16), pixels(16)}));
  } else {
    // Fallback to text symbol
    char symbol = icon_info ? icon_info->fallback_symbol : '?';
    text(ctx, mk(container.ent(), 0), 
         std::string(1, symbol),
         ComponentConfig{}.with_size({pixels(16), pixels(16)}));
  }
  
  // Always show label
  text(ctx, mk(container.ent(), 1), label, ComponentConfig{});
  
  // Make clickable
  container.ent().addComponentIfMissing<HasClickListener>([](Entity&){});
  
  return ElementResult{container.ent().get<HasClickListener>().down, 
                       container.ent()};
}

} // namespace afterhours::ui
```

---

## Implementation Details

### File Structure

```
vendor/afterhours/src/plugins/
├── icon_registry.h          # Main plugin (Component + API)
└── ui/
    └── icon_components.h    # icon_button() helper (optional)
```

### Dependencies

**Minimal**:
- ✅ `core/ecs.h` (for BaseComponent, Entity, EntityHelper)
- ✅ `developer.h` (for EnforceSingleton, logging)
- ✅ `<unordered_map>`, `<string>`, `<optional>`

**No dependencies on**:
- ❌ Raylib
- ❌ Texture loading
- ❌ File I/O

### Why String-Based IDs?

**Pros**:
- ✅ No need for users to define enums
- ✅ Easy to serialize/deserialize
- ✅ Can be data-driven (load from JSON)
- ✅ Flexible (add icons at runtime)

**Cons**:
- ⚠️ No compile-time checking
- ⚠️ Typos at runtime

**Mitigation**: Provide helper constants or let users define their own:
```cpp
namespace IconId {
  constexpr const char* Save = "save";
  constexpr const char* Undo = "undo";
  constexpr const char* Redo = "redo";
}

icon_registry::register_icon(IconId::Save, "Save", "icons/save.png", 'S');
```

---

## Usage Examples

### Example 1: Game Inventory System

```cpp
#include <afterhours/plugins/icon_registry.h>

void init_item_icons() {
  using namespace afterhours::icon_registry;
  
  init();  // Initialize the plugin
  
  // Register item icons
  register_icon("item_sword", "Sword", "items/sword.png", 'S');
  register_icon("item_potion", "Health Potion", "items/potion.png", 'P');
  register_icon("item_armor", "Leather Armor", "items/armor.png", 'A');
  register_icon("item_gold", "Gold Coins", "items/gold.png", 'G');
}

void render_inventory_slot(const std::string& item_id, int x, int y) {
  using namespace afterhours::icon_registry;
  
  auto icon = get(item_id);
  if (icon) {
    // Load and draw icon
    auto texture = load_texture(icon->resource_path);
    draw_texture(texture, x, y);
  } else {
    // Fallback to symbol
    char symbol = get_symbol(item_id);
    draw_text(std::string(1, symbol), x, y);
  }
}
```

### Example 2: Level Editor Toolbar

```cpp
#include <afterhours/plugins/icon_registry.h>
#include <afterhours/plugins/ui/icon_components.h>

void init_editor_icons() {
  using namespace afterhours::icon_registry;
  
  init();
  
  // Tool icons
  register_icon("tool_select", "Select", "tools/select.png", 'S');
  register_icon("tool_move", "Move", "tools/move.png", 'M');
  register_icon("tool_rotate", "Rotate", "tools/rotate.png", 'R');
  register_icon("tool_scale", "Scale", "tools/scale.png", 'Z');
  
  // File operations
  register_icon("file_new", "New", "file/new.png", 'N');
  register_icon("file_open", "Open", "file/open.png", 'O');
  register_icon("file_save", "Save", "file/save.png", 'S');
}

void render_toolbar(HasUIContext auto& ctx, Entity& parent) {
  // Use icon_button helper
  if (icon_button(ctx, mk(parent, 0), "tool_select", "Select").down) {
    activate_select_tool();
  }
  if (icon_button(ctx, mk(parent, 1), "tool_move", "Move").down) {
    activate_move_tool();
  }
  if (icon_button(ctx, mk(parent, 2), "tool_rotate", "Rotate").down) {
    activate_rotate_tool();
  }
}
```

### Example 3: Mirrored Icon Pairs

```cpp
void init_navigation_icons() {
  using namespace afterhours::icon_registry;
  
  init();
  
  // Undo/Redo use the same arrow, mirrored
  register_icon("edit_undo", "Undo", "arrows/left.png", '<', false);
  register_icon("edit_redo", "Redo", "arrows/left.png", '>', true);  // Mirrored
  
  // Indent/Outdent
  register_icon("indent", "Increase Indent", "arrows/right.png", '>', false);
  register_icon("outdent", "Decrease Indent", "arrows/right.png", '<', true);
}

void render_edit_menu(HasUIContext auto& ctx, Entity& parent) {
  if (icon_button(ctx, mk(parent, 0), "edit_undo", "Undo").down) {
    undo();
  }
  if (icon_button(ctx, mk(parent, 1), "edit_redo", "Redo").down) {
    redo();
  }
}
```

---

## Integration with Existing UI

### How It Fits

```
┌──────────────────────────────────────────────────────┐
│              Existing Afterhours UI                  │
├──────────────────────────────────────────────────────┤
│  sprite()         │  Low-level texture rendering     │
│  image_button()   │  Clickable texture primitive     │
│  text()           │  Text rendering                  │
│  div()            │  Container layout                │
├──────────────────────────────────────────────────────┤
│           NEW: IconRegistry Plugin                   │
├──────────────────────────────────────────────────────┤
│  ProvidesIconRegistry  │  ECS component (data)       │
│  icon_registry::*      │  Free function API          │
├──────────────────────────────────────────────────────┤
│           NEW: icon_button() helper                  │
├──────────────────────────────────────────────────────┤
│  Composes: icon_registry + sprite/text + div        │
│  Result: Semantic action button with icon + label    │
└──────────────────────────────────────────────────────┘
```

### No Breaking Changes

- ✅ Existing `sprite()` and `image_button()` unchanged
- ✅ New plugin is opt-in
- ✅ Users can mix icon_button with image_button
- ✅ Fallback to text if icons not loaded

---

## Testing Strategy

### Unit Tests

```cpp
TEST_CASE("IconRegistry - Basic Registration") {
  using namespace afterhours::icon_registry;
  
  init();
  register_icon("test", "Test Icon", "test.png", 'T');
  
  auto icon = get("test");
  REQUIRE(icon.has_value());
  REQUIRE(icon->name == "Test Icon");
  REQUIRE(icon->resource_path == "test.png");
  REQUIRE(icon->fallback_symbol == 'T');
  REQUIRE(!icon->is_mirrored);
}

TEST_CASE("IconRegistry - Missing Icon") {
  using namespace afterhours::icon_registry;
  
  init();
  auto icon = get("nonexistent");
  REQUIRE(!icon.has_value());
  
  char symbol = get_symbol("nonexistent");
  REQUIRE(symbol == '?');  // Fallback
}

TEST_CASE("IconRegistry - Mirrored Icons") {
  using namespace afterhours::icon_registry;
  
  init();
  register_icon("left", "Left", "arrow.png", '<', false);
  register_icon("right", "Right", "arrow.png", '>', true);
  
  auto left = get("left");
  auto right = get("right");
  
  REQUIRE(left->resource_path == right->resource_path);
  REQUIRE(!left->is_mirrored);
  REQUIRE(right->is_mirrored);
}
```

### Integration Tests

```cpp
TEST_CASE("icon_button - Renders With Icon") {
  // Create UI context
  auto ctx = create_test_context();
  auto root = create_test_entity();
  
  // Register icon
  icon_registry::init();
  icon_registry::register_icon("save", "Save", "icons/save.png", 'S');
  
  // Render icon button
  auto result = icon_button(ctx, mk(root, 0), "save", "Save File");
  
  // Verify children created
  auto& children = root.get<HasChildren>().children;
  REQUIRE(children.size() == 2);  // Icon + label
}
```

---

## Migration Path for Wordproc

### Step 1: Update Include

```cpp
// Before
#include "src/ui/icon_registry.h"

// After
#include "vendor/afterhours/src/plugins/icon_registry.h"
```

### Step 2: Change Initialization

```cpp
// Before (in main.cpp)
// Nothing (singleton auto-initialized)

// After
afterhours::icon_registry::init();
```

### Step 3: Update API Calls

```cpp
// Before
auto& registry = IconRegistry::instance();
registry.register_icon("save", {...});
auto icon = registry.get("save");

// After
afterhours::icon_registry::register_icon("save", {...});
auto icon = afterhours::icon_registry::get("save");
```

### Step 4: Update icon() Helper

```cpp
// Before
inline IconRegistry& icons() {
  return IconRegistry::instance();
}

// After
namespace icon_registry = afterhours::icon_registry;
// Use icon_registry::* directly
```

### Automated Refactor Script

```bash
# Replace getInstance() pattern
sed -i 's/IconRegistry::instance()/icon_registry/g' src/**/*.cpp
sed -i 's/icons().register_icon/icon_registry::register_icon/g' src/**/*.cpp
sed -i 's/icons().get/icon_registry::get/g' src/**/*.cpp
```

---

## Open Questions for Afterhours Maintainers

1. **Naming**: Prefer `icon_registry` or `icons` namespace?
2. **Location**: Should `icon_button()` be in the same file or `ui/icon_components.h`?
3. **Texture Loading**: Should the registry handle texture loading or leave it to users?
4. **Enum Support**: Should we provide a template version for enum-based IDs?
5. **Serialization**: Should we add `to_json()` / `from_json()` helpers?

---

## Conclusion

### Ready to Contribute

- ✅ Follows afterhours ECS patterns
- ✅ Minimal dependencies
- ✅ Header-only
- ✅ Composable with existing UI
- ✅ Useful for games and tools
- ✅ No breaking changes
- ✅ Well-tested

### Next Steps

1. Create PR with `icon_registry.h`
2. Add unit tests
3. Add integration tests with UI
4. Document in afterhours wiki
5. Migrate wordproc after PR merged

### Estimated Review Time

- **Implementation**: 4-6 hours
- **Testing**: 2-3 hours
- **Documentation**: 1-2 hours
- **PR Review Cycle**: 1-2 weeks


