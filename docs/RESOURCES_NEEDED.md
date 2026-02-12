# UI Resources Needed

Additional resources to support a Windows 95 + Mac OS 3.1 inspired word processor UI.

## Priority: High

- **Window chrome assets**: title bar gradients, close/minimize/maximize buttons, border edges, resize grip.
- **Menu + toolbar icons**: new/open/save, print, cut/copy/paste, undo/redo, find, zoom.
- **Formatting icons**: bold, italic, underline, align left/center/right/justify, bullets/numbering.
- **Form controls**: checkbox, radio, dropdown arrow, scrollbar thumb/track, splitter handle.
- **Cursors**: arrow, I-beam text, resize (horizontal/vertical/diag), hand, wait.

## Priority: Medium

- **Document/file icons**: blank page, multiple pages, recent file thumbnail.
- **Status bar icons**: page count, line/column indicator, word count badge.
- **Dialog icons**: info, warning, error, question (Win95-style).
- **Mac OS 3.1 accents**: platinum-style window buttons and titlebar variants for theme toggle.

## Priority: Low

- **Optional stickers**: classic app icons (diskette, clipboard) for splash/about.
- **Toolbar separators**: etched-line dividers and grip textures.

## Notes

- Prefer 1x and 2x raster sizes (16px/32px) for Win95 authenticity.
- Provide SVG originals if possible for future scaling.
- For icons, use a symbol-capable font (Google Fonts or Font Awesome) instead of per-menu-item bitmaps.
- Avoid adding icons to every menu item; follow Apple HIG guidance on menu clarity and readability.
- Ralph should attempt to generate these assets locally as needed, but do not commit the generation scripts.
- Read/consider: Apple Macintosh HIG 1992, Tahoe icons critique, and menu icons discussion:
  - https://dn721903.ca.archive.org/0/items/apple-hig/Macintosh_HIG_1992.pdf
  - https://tonsky.me/blog/tahoe-icons/
  - https://blog.jim-nielsen.com/2025/icons-in-menus/
