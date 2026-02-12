# Hover Cards

A floating card that appears near the cursor when hovering over an element. The app provides content; the widget handles positioning, delay, and dismissal. Interactive -- the user can scroll and click links within the card.

## Status

Not yet implemented.

## Decisions

- **Trigger**: Mouse hover after a short delay (e.g., 300-500ms)
- **Content**: App provides the hover card content -- the widget is just a positioned container
- **Interaction**: Interactive -- mouse can enter the card to scroll, click links, select text
- **Pinning**: No, always ephemeral (dismiss when mouse moves away from both the target and the card)
- **Positioning**: Auto-position near the hover target, avoid going off-screen
