# Performance with Large Documents

The editor should remain fast and responsive even with very large documents (10MB+, hundreds of pages). This means efficient text storage, lazy rendering, virtualized scrolling, and background processing for expensive operations.

## Status

Current performance is strong for typical documents (< 1ms typing latency, < 80ms cold start on M1). Gap buffer is implemented. Needs validation and optimization for 10MB+ documents, lazy text layout, and virtualized page rendering.
