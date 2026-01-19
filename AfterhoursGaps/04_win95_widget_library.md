# Win95-Style Widget Library

## Status: ADDRESSED ✅

Afterhours now provides bevel border support in `vendor/afterhours/src/plugins/ui/`:
- `components.h` - `BevelStyle` enum (None, Raised, Sunken) and `BevelBorder` struct
- `components.h` - `HasBevelBorder` component
- `rendering.h` - `render_bevel()` function for automatic bevel rendering

### Migration TODO
Wordproc's `src/ui/win95_widgets.cpp` can be simplified to use:
```cpp
entity.add<HasBevelBorder>(BevelBorder{
  .light_color = {255, 255, 255, 255},
  .dark_color = {128, 128, 128, 255},
  .thickness = 2.0f,
  .style = BevelStyle::Raised  // or Sunken
});
```

## Original Problem (now solved)
Afterhours previously did not support 3D beveled borders needed for classic Win95/retro UI styling.

## Current Workaround
Custom `src/ui/win95_widgets.cpp` implements `DrawRaisedBorder()` and `DrawSunkenBorder()` 
which draw multi-colored edges to create the 3D effect:
- **Raised** (buttons, panels): light color on top/left, dark on bottom/right
- **Sunken** (text fields, list boxes): dark on top/left, light on bottom/right

## What Afterhours Currently Provides
The existing `Border` struct only supports single-color borders:

```cpp
struct Border {
  Color color = Color{0, 0, 0, 0};
  float thickness = 2.0f;
  bool has_border() const { return thickness > 0.0f && color.a > 0; }
};
```

## Required Feature: Beveled/3D Borders

### Proposed API Addition to `components.h`

```cpp
enum class BevelStyle {
  None,    // No bevel (flat border)
  Raised,  // Light on top-left, dark on bottom-right (buttons, panels)
  Sunken,  // Dark on top-left, light on bottom-right (inputs, wells)
};

struct BevelBorder {
  Color light_color = Color{255, 255, 255, 255};  // Top-left edges
  Color dark_color = Color{128, 128, 128, 255};   // Bottom-right edges
  float thickness = 1.0f;
  BevelStyle style = BevelStyle::Raised;

  bool has_bevel() const { return style != BevelStyle::None && thickness > 0.0f; }
};

struct HasBevelBorder : BaseComponent {
  BevelBorder bevel;
  HasBevelBorder() = default;
  explicit HasBevelBorder(const BevelBorder &b) : bevel(b) {}
};
```

### Proposed API Addition to `component_config.h`

```cpp
ComponentConfig &with_bevel(BevelStyle style, 
                            Color light = {255, 255, 255, 255},
                            Color dark = {128, 128, 128, 255},
                            float thickness = 1.0f) {
  bevel_config = BevelBorder{light, dark, thickness, style};
  return *this;
}

ComponentConfig &with_raised_bevel(Color light = {255, 255, 255, 255},
                                   Color dark = {128, 128, 128, 255}) {
  return with_bevel(BevelStyle::Raised, light, dark);
}

ComponentConfig &with_sunken_bevel(Color light = {255, 255, 255, 255},
                                   Color dark = {128, 128, 128, 255}) {
  return with_bevel(BevelStyle::Sunken, light, dark);
}
```

### Proposed Rendering in `rendering.h`

Add bevel rendering after regular border rendering:

```cpp
if (entity.has<HasBevelBorder>()) {
  const BevelBorder &bevel = entity.template get<HasBevelBorder>().bevel;
  if (bevel.has_bevel()) {
    Color top_left, bottom_right;
    if (bevel.style == BevelStyle::Raised) {
      top_left = bevel.light_color;
      bottom_right = bevel.dark_color;
    } else {
      top_left = bevel.dark_color;
      bottom_right = bevel.light_color;
    }
    draw_bevel_border(draw_rect, bevel.thickness, top_left, bottom_right);
  }
}
```

### Usage Example

```cpp
// Win95-style button
auto btn = button(ctx, id, "OK")
    .with_color(Theme::Usage::Custom, {192, 192, 192, 255})  // Button face gray
    .with_raised_bevel({255, 255, 255, 255}, {128, 128, 128, 255})
    .with_roundness(0.0f);  // Sharp corners

// Win95-style text input
auto input = div(ctx, id)
    .with_color(Theme::Usage::Custom, {255, 255, 255, 255})  // White background
    .with_sunken_bevel({255, 255, 255, 255}, {128, 128, 128, 255})
    .with_roundness(0.0f);
```

## What We Can Already Do With Afterhours
- Theme colors for Win95 palette
- `roundness = 0.0f` for sharp corners
- Button hover/pressed/disabled states
- Font configuration

## Blocked Until Feature Added
- Proper 3D button appearance
- Sunken text field borders
- Raised panel borders
- Menu bar styling
