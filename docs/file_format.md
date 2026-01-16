# Wordproc Document Format (.wpdoc)

Version: 0.1

## Overview

Wordproc documents use JSON for structured storage with backward compatibility to plain text.

## File Extension

- **Primary**: `.wpdoc` - Native Wordproc format
- **Import**: `.txt`, `.md` - Loaded as plain text

## JSON Schema (v0.1)

```json
{
  "version": 1,
  "text": "<document content as string>",
  "style": {
    "bold": false,
    "italic": false,
    "font": "Gaegu-Bold",
    "fontSize": 16
  },
  "fontRequirements": [
    {
      "fontId": "NotoSansKR",
      "scripts": ["korean"]
    }
  ]
}
```

## Field Definitions

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| version | integer | Yes | Document format version (currently `1`) |
| text | string | Yes | Full document text content |
| style | object | No | Global document style (may become per-range in future) |
| style.bold | boolean | No | Default: false |
| style.italic | boolean | No | Default: false |
| style.font | string | No | Font name. Default: "Gaegu-Bold" |
| style.fontSize | integer | No | Font size in pixels. Default: 16, Range: 8-72 |
| fontRequirements | array | No | Fonts and scripts needed by document (for lazy loading) |
| fontRequirements[].fontId | string | Yes | Font identifier (e.g., "NotoSansKR") |
| fontRequirements[].scripts | array | Yes | Script identifiers this font provides |

### Script Identifiers

Scripts are identified by lowercase strings:

| Script | Description | Codepoint Range |
|--------|-------------|-----------------|
| `latin` | ASCII + Latin Extended | 0x0020-0x024F |
| `korean` | Hangul Syllables + Jamo | 0xAC00-0xD7AF, 0x1100-0x11FF |
| `japanese` | Hiragana + Katakana + Kanji | 0x3040-0x30FF, 0x4E00-0x9FFF |
| `chinese` | CJK Unified Ideographs | 0x4E00-0x9FFF |
| `cyrillic` | Russian, Ukrainian, etc. | 0x0400-0x04FF |
| `greek` | Greek alphabet | 0x0370-0x03FF |
| `arabic` | Arabic script | 0x0600-0x06FF |
| `hebrew` | Hebrew script | 0x0590-0x05FF |
| `thai` | Thai script | 0x0E00-0x0E7F |

### Lazy Font Loading

CJK fonts are not loaded at startup to keep cold start fast (< 100ms target).
When a document specifies `fontRequirements`, the app loads those fonts on demand:

1. Document opens, `fontRequirements` is parsed
2. App checks which fonts are already loaded
3. Missing fonts are loaded with appropriate codepoint ranges
4. Text is rendered once fonts are ready

## Fallback Behavior

When loading a file:
1. If valid JSON with `text` field: Extract text and style
2. If valid JSON without `text` field: Treat as plain text
3. If invalid JSON: Treat entire content as plain text

This allows `.wpdoc` files to be opened even if partially corrupted.

## Plain Text Import

When loading `.txt` or `.md` files:
- Content is loaded as-is into the text buffer
- Default style is applied
- No metadata is preserved

## Validator Rules

When validating a .wpdoc file:

| Rule | Condition | Action on Failure |
|------|-----------|-------------------|
| Valid JSON | Must parse as JSON | Treat as plain text |
| Has version | `version` field exists | Treat as plain text |
| Version supported | `version` == 1 | Warn but attempt load |
| Has text | `text` field exists | Treat raw content as text |
| Text is string | `text` is string type | Treat as plain text |
| Font size range | 8 <= fontSize <= 72 | Clamp to range |
| File size | <= 10MB recommended | Warn if larger |

## Version History

| Version | Description |
|---------|-------------|
| 1 (v0.1) | Initial format with global style |

## Future Considerations

- Per-range styles (bold/italic specific text ranges)
- Images and embedded content
- Page layout settings
- Print margins

## Format Evaluation: JSON vs wpdoc Zip Container

### Current Decision (v0.1): Keep JSON

JSON is the correct choice for v0.1 because:

| Aspect | JSON | Zip Container |
|--------|------|---------------|
| Simplicity | Simple to implement | Requires zip library |
| Debuggability | Human-readable, easy to inspect | Binary, requires extraction |
| Dependencies | None (nlohmann/json already in use) | Would need minizip or similar |
| Text-only docs | Optimal | Overhead for small docs |
| File size | Minimal for text-only | Compression benefits at scale |

### Recommendation for v0.2+

Switch to zip container when adding:
1. **Images/media**: Base64 in JSON is inefficient (33% size overhead)
2. **Per-range styles**: Complex style maps benefit from separate style.json
3. **Multiple revisions**: Can include revision history as separate files

### Proposed v0.2 Zip Structure

```
document.wpdoc/
├── content.txt          # Raw text content
├── styles.json          # Style ranges and metadata
├── media/               # Embedded images
│   ├── image1.png
│   └── image2.jpg
├── settings.json        # Page layout, margins
└── manifest.json        # Version, file list
```

### Migration Path

1. v0.1 JSON format continues to work
2. v0.2 loader detects format by file header (ZIP magic bytes vs JSON `{`)
3. Save-as option to convert between formats
