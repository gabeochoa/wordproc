# Search Results Panel

A panel showing search results grouped by file with collapsible file headers, match counts, and configurable context lines. Click a result to navigate to it in the editor.

## Status

Not yet implemented. Find & replace exists within a single document but no multi-file search results view.

## Decisions

- **Location**: App decides placement (sidebar panel by default)
- **Grouping**: Results grouped by file with collapsible file headers showing match count
- **Context**: Each result shows the matching line with the match highlighted, with configurable surrounding context lines
- **Click navigation**: Click a result to open the file and scroll to the match
- **Replace**: TODO -- decide later whether the search panel includes replace functionality
