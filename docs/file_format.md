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
  }
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

## Version History

| Version | Description |
|---------|-------------|
| 1 (v0.1) | Initial format with global style |

## Future Considerations

- Per-range styles (bold/italic specific text ranges)
- Images and embedded content
- Page layout settings
- Print margins
