# Wordproc Style Guide

A design guide for Wordproc, styled as a Windows 95 application with Mac OS 3.1 accents.

## Design Philosophy

Wordproc combines the utilitarian clarity of Windows 95 with subtle refinements from early Macintosh design. The goal is a nostalgic yet functional word processor that feels like it could have shipped in 1995.

## Color Palette

### Windows 95 Core Colors
| Name | RGB | Hex | Usage |
|------|-----|-----|-------|
| Window Gray | 192, 192, 192 | #C0C0C0 | Main window background, status bar |
| Title Bar Blue | 0, 0, 128 | #000080 | Active window title bar |
| White | 255, 255, 255 | #FFFFFF | Text area background, title text |
| Black | 0, 0, 0 | #000000 | Primary text, caret |
| Border Light | 255, 255, 255 | #FFFFFF | 3D border highlight (top/left) |
| Border Dark | 128, 128, 128 | #808080 | 3D border shadow (bottom/right) |

### Mac OS 3.1 Accent Colors (subtle use)
| Name | RGB | Hex | Usage |
|------|-----|-----|-------|
| Selection Blue | 0, 0, 128 | #000080 | Text selection highlight |
| Menu Highlight | 0, 0, 0 | #000000 | Menu hover state (inverted text) |

## Typography

### Primary Fonts
- **UI Font**: System default (rendered via raylib)
- **Document Font**: Gaegu-Bold (default), EBGaramond-Regular (alternate)
- **Monospace**: NotoSansMonoCJK (for code blocks, if implemented)

### Font Sizes
- Title Bar: 16px
- Menu Bar: 14px
- Document Text: 16px (default), adjustable 8-72px
- Status Bar: 14px

### Line Height
- Document lines: 20px (1.25x font size)
- Menu items: 20px

## 3D Border Effects

Windows 95's signature is the beveled 3D appearance. Implement consistently:

### Raised (Buttons, Menu Bar)
```
+-------------------+
|  Light            |
|  +-------------+  |
|  | Content     | D|
|  +-------------+ a|
|        Dark     r |
+-------------------+k
```
- Top/Left edge: Border Light (#FFFFFF)
- Bottom/Right edge: Border Dark (#808080)

### Sunken (Text Areas, Input Fields)
```
+-------------------+
|  Dark             |
|  +-------------+  |
|  | Content     | L|
|  +-------------+ i|
|        Light    g |
+-------------------+ht
```
- Top/Left edge: Border Dark (#808080)
- Bottom/Right edge: Border Light (#FFFFFF)

## Window Layout

```
+------------------------------------------+
| [icon] Wordproc - filename.txt *         | <- Title Bar (24px)
+------------------------------------------+
| File  Edit  Format  Help                 | <- Menu Bar (20px)
+------------------------------------------+
|                                          |
|  Text editing area                       |
|  (sunken 3D border)                      |
|                                          |
|                                          |
+------------------------------------------+
| Ln 1, Col 1 | B I | Gaegu-Bold           | <- Status Bar (20px)
+------------------------------------------+
```

## UI Components

### Title Bar
- Height: 24px
- Background: Title Bar Blue (#000080)
- Text: White, left-aligned with 4px padding
- Format: "Wordproc - [filename]" or "Wordproc - Untitled"
- Dirty indicator: Append " *" when unsaved changes exist

### Menu Bar
- Height: 20px
- Background: Window Gray (#C0C0C0)
- Text: Black, 14px
- Items: File, Edit, Format, Help
- Hover: Inverted (black background, white text)
- Border: Raised 3D effect

### Text Area
- Background: White (#FFFFFF)
- Border: Sunken 3D effect (2px)
- Padding: 8px internal
- Scrollbars: Win95 style (when implemented)

### Status Bar
- Height: 20px
- Background: Window Gray (#C0C0C0)
- Border: Raised 3D effect
- Contents (left to right):
  - Line/Column: "Ln X, Col Y"
  - Style indicators: "B" for bold, "I" for italic
  - Current font name

### Buttons (for dialogs)
- Min width: 75px
- Height: 23px
- Padding: 2px 8px
- Border: Raised 3D effect
- States:
  - Normal: Gray background, raised border
  - Hover: Slightly lighter
  - Pressed: Sunken border, shifted content 1px down-right
  - Disabled: Gray text

### Dialogs
- Modal, centered on window
- Title bar same style as main window
- Content area: Window Gray background
- Button row at bottom, right-aligned

## Caret & Selection

### Caret
- Width: 2px
- Color: Black (#000000)
- Blink rate: 500ms on, 500ms off
- Shape: Vertical bar at insertion point

### Selection
- Background: Title Bar Blue (#000080)
- Text: White (#FFFFFF) - inverted from normal
- Multi-line: Each line highlighted separately

## Animations & Timing

### Caret Blink
- Period: 1000ms total (500ms visible, 500ms hidden)
- Reset to visible on any input/navigation

### Menu Animation
- Open: Instant (no animation, authentic to era)
- Close: Instant

### Window Resize
- Real-time redraw (no animation)

### Scroll
- Discrete steps (line-by-line or page), no smooth scrolling

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| Ctrl+N | New document |
| Ctrl+O | Open document |
| Ctrl+S | Save document |
| Ctrl+B | Toggle bold |
| Ctrl+I | Toggle italic |
| Ctrl+1 | Select Gaegu-Bold font |
| Ctrl+2 | Select EBGaramond font |
| Ctrl+Z | Undo |
| Ctrl+Y | Redo |
| Ctrl+X | Cut |
| Ctrl+C | Copy |
| Ctrl+V | Paste |
| Ctrl+A | Select all |

## Interaction States

### Focus
- Active window: Blue title bar
- Inactive window: Gray title bar (future)
- Text area always accepts input when window is focused

### Hover
- Buttons: Subtle highlight
- Menu items: Inverted colors
- Text area: I-beam cursor

### Press/Click
- Buttons: Sunken appearance
- Menu items: Stay inverted, trigger on release

## Accessibility

### Font Size
- Minimum: 8px (warn below)
- Default: 16px
- Maximum: 72px

### Color Contrast
- All text meets WCAG AA minimum contrast ratio
- Black text on white: 21:1 (maximum)
- White text on blue: 8.6:1 (excellent)

### Keyboard Navigation
- Full keyboard access to all features
- Tab order: Menu bar -> Text area -> Status bar
- Arrow keys for caret movement
- Shift+Arrow for selection extension

## File Format

### Native Format (.wpdoc)
```json
{
  "version": 1,
  "text": "Document content here...",
  "style": {
    "bold": false,
    "italic": false,
    "font": "Gaegu-Bold"
  }
}
```

### Import Formats
- `.txt`: Plain text, UTF-8
- `.md`: Markdown (basic rendering)
- `.doc`: Not implemented (placeholder)

## Future Considerations

### Mac OS 3.1 Accents to Add
- Chicago-style font for menus (if licensing permits)
- Rounded corners on dialogs (subtle, 2px radius)
- Striped title bar pattern (optional skin)

### Win95 Features to Implement
- Minimize/Maximize/Close buttons in title bar
- System menu (click on icon)
- Keyboard mnemonics (Alt+F for File menu)
- Context menus (right-click)
