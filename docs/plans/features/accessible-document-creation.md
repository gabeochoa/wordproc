# Accessible Document Creation

Tools to help authors create accessible documents: alt text for images, semantic heading structure validation, color contrast checking, and an accessibility checker that flags issues.

## Status

Not yet implemented.

## Decisions

- **Alt text on images**: Optional on insert, but flagged by the accessibility checker if missing
- **Checker mode**: On-demand by default; can be turned on as always-on in settings
- **Heading validation**: Report skipped heading levels in the checker (no inline warnings)
- **Accessibility standard**: User-configurable (WCAG 2.1 AA or AAA)
- **Reading order**: Skip for now
- **Color contrast**: Check text/background contrast ratios against the chosen WCAG level
- **Tagged export**: Best-effort accessible/tagged output for PDF and DOCX exports
- **Table headers**: Prompt to mark header rows/columns when creating tables; flag in checker if missing
- **Link text**: Flag non-descriptive hyperlink text (e.g., "click here", bare URLs)
- **Issue presentation**: Both inline markers and a sidebar panel, controlled by a setting
- **Document language**: Auto-detect and tag silently
