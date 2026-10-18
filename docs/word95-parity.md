# Word for Windows 95 (Word 7.0) — Parity Checklist

Status key: **Have** = works end-to-end (menu/toolbar + behavior + test) ·
**Partial** = present but incomplete vs Word 95 · **Missing** = absent.
Priority: P0 = daily-driver value, P1 = common, P2 = niche/stub-OK.
Inventory by code read, 2026-10-18 (`src/ui/menu_setup.h`, `src/ecs/menu_actions.h`,
`src/editor/*`, `tests/`). Counts at bottom are computed from these tables.

## File
| Feature | Status | Priority | Notes |
|---|---|---|---|
| New / Open / Save / Save As | Have | P0 | native dialogs, recent files |
| New from Template... | Partial | P1 | template dialog exists; no shipped .dot-style templates |
| Export RTF / HTML / PDF | Have | P1 | File menu exports; native .doc binary I/O Missing below |
| Save/open .doc (Word binary) | Missing | P2 | RTF is the interchange path |
| Save/open .rtf / .txt | Have | P0 | document_io + tests |
| Page Setup (margins/paper) | Partial | P0 | dialog + margins state; verify paged render honors all fields |
| Print | Missing | P1 | toolbar Print icon exists; no File>Print, no print path |
| Print Preview | Missing | P1 | — |
| Exit / MRU list | Have | P2 | — |

## Edit
| Feature | Status | Priority | Notes |
|---|---|---|---|
| Undo / Redo | Have | P0 | CommandHistory, multi-level |
| Cut / Copy / Paste | Have | P0 | incl. toolbar + context menu |
| Select All | Have | P0 | |
| Find / Replace / Find Next-Prev | Have | P0 | dialog + F3/Shift+F3 |
| Go To / Bookmarks | Partial | P1 | bookmark dialog + Go To Bookmark |
| Track Changes (revise mode) | Partial | P1 | toggle + accept/reject all; no per-revision UI |
| AutoCorrect | Missing | P0 | no engine hook on typing — Batch 1 |
| Repeat (F4) | Missing | P2 | |

## View
| Feature | Status | Priority | Notes |
|---|---|---|---|
| Normal view | Missing | P1 | only pageless/paged |
| Page Layout view | Partial | P0 | Paged Mode exists; verify margins/header rendering |
| Outline view | Partial | P1 | Show Outline pane exists; not a Word outline view |
| Zoom (in/out/reset, %) | Have | P0 | Ctrl+Alt +/-/0 |
| Ruler (indents/tabs draggable) | Partial | P0 | ruler rendered; drag indents/tabs to verify |
| Toolbars show/hide | Missing | P2 | Standard+Formatting always on |
| Status bar | Partial | P0 | Word 6-style; page/sec/Ln/Col present, verify accuracy |
| Header/Footer view | Partial | P1 | Insert Header/Footer entries exist |

## Insert
| Feature | Status | Priority | Notes |
|---|---|---|---|
| Page/Section Break | Have | P0 | |
| Table... | Have | P0 | grid insert + Table menu |
| Picture... | Have | P1 | image insert + layout/wrap |
| Symbol / Special Character... | Partial | P1 | dialog exists; date/time insert Missing |
| Date & Time... | Missing | P1 | |
| Header / Footer / Page Number | Have | P0 | |
| Footnote | Have | P2 | |
| Hyperlink / Bookmark / Comment | Have | P1 | comment is also a Docs-era feature, kept |
| Table of Contents | Partial | P2 | entry exists; field update semantics TBD |
| Equation / Shapes | Have | P2 | beyond Word 95 basic drawing |

## Format
| Feature | Status | Priority | Notes |
|---|---|---|---|
| Font... dialog (family/size/effects) | Partial | P0 | Formatting toolbar dropdowns; only 2 families wired |
| Bold/Italic/Underline/Strike/Super/Sub | Have | P0 | |
| Alignment L/C/R/Justify | Have | P0 | |
| Indents & Tabs (ruler + dialog) | Partial | P0 | indent keys + Tab Width dialog |
| Bullets & Numbering | Have | P0 | multi-level list indent |
| Paragraph... (spacing/line spacing) | Partial | P1 | menu increments, no full dialog |
| Borders and Shading... | Missing | P1 | table borders render; no dialog |
| Columns... | Missing | P1 | |
| Styles (Normal/Title/Headings 1-6) | Have | P0 | style dropdown + Ctrl+Alt+n |
| Drop Cap | Partial | P2 | menu entry exists |

## Tools
| Feature | Status | Priority | Notes |
|---|---|---|---|
| Word Count... | Have | P0 | dialog: words/chars/lines/paragraphs/sentences (verified in menu_ui_system.h) |
| Spelling (F7 dialog) | Missing | P0 | SpellChecker engine exists, no dialog — Batch 1 |
| Red squiggle spell check as you type | Missing | P0 | engine exists, no render — Batch 1 |
| Grammar check | Partial | P2 | GrammarChecker engine, no UI |
| AutoCorrect engine | Missing | P0 | Batch 1 (two initial caps, replace-as-type table) |
| Thesaurus | Missing | P2 | |
| Mail Merge... | Missing | P2 | stub OK per plan |
| Options/Preferences | Partial | P2 | Preferences disabled in menu |

## Table
| Feature | Status | Priority | Notes |
|---|---|---|---|
| Insert/Delete rows & columns | Have | P0 | |
| Merge / Split cells | Have | P1 | |
| Table borders rendering | Have | P1 | test_border_rendering |
| Sort / Formula / AutoFormat | Missing | P2 | |

## Window
| Feature | Status | Priority | Notes |
|---|---|---|---|
| Window menu (multi-document) | Missing | P1 | single-document app; menu absent entirely |
| Split pane | Partial | P1 | View>Split View exists |

## Help
| Feature | Status | Priority | Notes |
|---|---|---|---|
| About / Keyboard Shortcuts (F1) | Have | P2 | |
| Help Topics / Answer Wizard / Office Assistant | Missing | P2 | never (Clippy is post-95) |

## Chrome (toolbars / ruler / status bar)
| Feature | Status | Priority | Notes |
|---|---|---|---|
| Standard toolbar (icons) | Partial | P0 | pixel icons in render pass; verify vs Word 95 set (Print Preview, Spelling, etc.) |
| Formatting toolbar (style/font/size/BIU/align) | Partial | P0 | present; font list = 2 families |
| Title bar / menu bar / bevels | Have | P0 | Win95 theme, drop shadows, focus ring |
| Scrollbars (Win95 style) | Partial | P1 | verify |

## Counts (from tables above, 2026-10-18)
**Have 26 · Partial 21 · Missing 18** (65 grouped rows; several rows bundle
e.g. Cut/Copy/Paste). Supersedes the front-door first-reply estimate (31/11/13).
Ranking: Batch 1 = Tools spell trio (squiggles + F7 dialog + AutoCorrect) —
highest daily value, engine already exists. Then: Print Preview/Print,
Window menu + Normal/Outline views, Columns, Borders/Shading dialog,
full Font dialog + more families, Date/Time insert, toolbar icon fidelity.
