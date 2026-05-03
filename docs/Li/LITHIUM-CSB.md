# Lithium CodeMirror Scrollbars (CSB)

This document describes Lithium's custom CodeMirror scrollbar implementation — a vanilla JavaScript plugin that replaces native browser scrollbars with themed, consistent cross-browser scrollbars.

---

## Overview

Lithium's CodeMirror scrollbar plugin provides:

- **Custom themed scrollbars** that match Lithium's design system
- **Fixed positioning** so scrollbars stay anchored to viewport edges
- **Corner management** when both scrollbars are visible
- **Content protection** with automatic padding to prevent overlap
- **Drag constraints** that respect visual track boundaries
- **Native scrollbar suppression** to prevent conflicts

**Location:** `src/core/cm6-custom-scrollbars.js`  
**Integration:** `src/core/codemirror-setup.js`  
**Status:** Active in all CodeMirror instances across Lithium

---

## Architecture

### Plugin Structure

The scrollbar plugin is implemented as a CodeMirror 6 ViewPlugin that:

1. **Hides native scrollbars** using comprehensive CSS rules
2. **Creates custom DOM elements** for tracks, thumbs, and corners
3. **Positions elements** using fixed positioning relative to viewport
4. **Handles user interactions** with proper drag constraints
5. **Manages content padding** to prevent scrollbar overlap

### DOM Structure

When active, the plugin creates this structure within the CodeMirror editor:

```html
<div class="cm-editor">
  <div class="cm-scroller" data-scrollbars="vertical">
    <!-- Existing CodeMirror content -->
    <div class="cm-content" style="padding-right: 14px;">
      <!-- Text content -->
    </div>

    <!-- Custom scrollbar elements -->
    <div class="cm-scroller-track cm-scroller-track-v"></div>
    <div class="cm-scroller-thumb cm-scroller-thumb-v"></div>
    <div class="cm-scroller-corner"></div>
  </div>
</div>
```

### Key Features

| Feature | Implementation |
|---------|----------------|
| **Fixed Positioning** | Scrollbars stay at viewport edges during scrolling |
| **Theming** | Uses Lithium CSS variables (`--accent-primary`, `--bg-secondary`) |
| **Corner Management** | Fills bottom-right gap when both scrollbars visible |
| **Content Protection** | Adds padding to prevent text overlap |
| **Drag Constraints** | Thumbs respect shortened track lengths |
| **Responsive** | Updates automatically on resize/content changes |

---

## Integration

### Automatic Application

The scrollbar plugin is automatically applied to all CodeMirror instances via `buildEditorExtensions()`:

```javascript
// In codemirror-setup.js
export function buildEditorExtensions(options = {}) {
  const extensions = [
    // ... other extensions
    customScrollbars(), // Always included
  ];
  return extensions;
}
```

### No Manual Setup Required

Managers using CodeMirror get custom scrollbars automatically:

```javascript
// Style Manager - gets custom scrollbars
this.cssEditor = new EditorView({
  state: EditorState.create({ doc: css, extensions }),
  parent: container
});

// Queries Manager - gets custom scrollbars
this.sqlEditor = new EditorView({
  state: EditorState.create({ doc: sql, extensions }),
  parent: container
});
```

---

## Styling

### CSS Variables

The plugin uses Lithium's design system variables:

```css
/* Track appearance */
--bg-secondary        /* Track background */
/* Thumb appearance */
--accent-alt-primary  /* Thumb background */
/* Hover states */
--accent-primary      /* Thumb hover background */
/* Spacing */
--border-radius-full  /* Rounded corners */
```

### Customization

Override default styles by targeting scrollbar classes:

```css
/* Custom track styling */
.cm-scroller-track {
  background: var(--custom-track-bg);
}

/* Custom thumb styling */
.cm-scroller-thumb {
  background: var(--custom-thumb-bg);
  border-radius: 4px; /* Less rounded */
}

/* Custom corner styling */
.cm-scroller-corner {
  background: var(--custom-corner-bg);
}
```

### Theme Integration

The plugin respects Lithium's theming system:

- **Dark theme**: Uses secondary backgrounds and accent colors
- **Light theme**: Automatically adapts via CSS variables
- **Custom themes**: Override variables for complete customization

---

## Behavior

### Scrollbar Visibility

- **Auto-show/hide**: Scrollbars appear only when content overflows
- **Always visible when active**: Once shown, scrollbars stay visible
- **Independent axes**: Horizontal and vertical scrollbars operate separately

### Positioning

- **Fixed to viewport**: Scrollbars don't move when content scrolls
- **Edge alignment**: Vertical scrollbar at right edge, horizontal at bottom
- **Gutter awareness**: Horizontal scrollbar starts after line number gutter

### Interaction

- **Drag to scroll**: Click and drag thumbs to scroll
- **Smooth dragging**: Proper acceleration and constraints
- **Mouse wheel**: Normal scrolling works alongside custom scrollbars
- **Touch support**: Works on touch devices

### Corner Management

When both scrollbars are visible:

1. **Track shortening**: Both tracks are shortened by 12px to create corner space
2. **Corner filling**: Bottom-right corner gets a background fill
3. **Thumb constraints**: Thumbs cannot be dragged into corner area
4. **Content padding**: Text content gets padding to avoid scrollbar overlap

---

## Content Protection

### Automatic Padding

The plugin adds padding to content based on visible scrollbars:

```css
/* Single scrollbar padding */
.cm-scroller[data-scrollbars="vertical"] .cm-content {
  padding-right: 14px;  /* 12px scrollbar + 2px spacing */
}

.cm-scroller[data-scrollbars="horizontal"] .cm-content {
  padding-bottom: 14px; /* 12px scrollbar + 2px spacing */
}

/* Both scrollbars padding */
.cm-scroller[data-scrollbars="both"] .cm-content {
  padding-right: 14px;
  padding-bottom: 14px;
}
```

### Why This Matters

Without padding, scrollbars would overlap the last line of text:

```
Before: [Text line N] [scrollbar covers this]
After:  [Text line N]
                     [scrollbar here]
```

### Dynamic Updates

Padding is applied/removed dynamically as scrollbars appear/disappear during content changes.

---

## Technical Details

### Performance

- **RequestAnimationFrame**: Updates use RAF for smooth performance
- **Minimal DOM**: Only creates elements when scrollbars are needed
- **Efficient calculations**: Optimized positioning math
- **Memory management**: Proper cleanup on editor destruction

### Browser Support

- **Chrome/Edge**: Uses `-webkit-scrollbar: none`
- **Firefox**: Uses `scrollbar-width: none`
- **Safari**: Uses `-webkit-scrollbar: none`
- **IE11+**: Uses `-ms-overflow-style: none`

### Edge Cases

- **Zero-height content**: No scrollbars shown
- **Window resize**: Scrollbars reposition automatically
- **Content changes**: Scrollbars update after DOM changes
- **Multiple editors**: Each editor gets independent scrollbars

---

## Migration from Native Scrollbars

### Before (Native Scrollbars)

```css
/* Native scrollbars - inconsistent across browsers */
.cm-scroller {
  scrollbar-width: auto;  /* Firefox */
  scrollbar-color: ...;   /* Firefox */
}

.cm-scroller::-webkit-scrollbar { /* Chrome/Safari/Edge */
  width: 12px;
  background: ...;
}
```

### After (Custom Scrollbars)

```javascript
// Automatic - no manual setup needed
const extensions = buildEditorExtensions({
  language: 'css',
  // ... other options
});
// Scrollbars applied automatically
```

### Benefits

- **Consistent appearance** across all browsers
- **Theme integration** with Lithium design system
- **Better UX** with proper corner management
- **Content protection** prevents text overlap
- **Future-proof** - can be extended with features

---

## Troubleshooting

### Scrollbars Not Appearing

**Check:** Is content actually overflowing?
```javascript
const { scrollHeight, clientHeight } = scroller;
console.log('Overflow:', scrollHeight > clientHeight);
```

**Check:** Are native scrollbars suppressed?
```css
.cm-scroller {
  scrollbar-width: none !important;
}
```

### Scrollbars Overlapping Content

**Check:** Is content padding applied?
```css
.cm-scroller[data-scrollbars] .cm-content {
  /* Should have padding-right/bottom */
}
```

### Thumbs Dragging Into Corner

**Check:** Are drag constraints accounting for shortened tracks?
```javascript
// Should subtract 12px when both scrollbars visible
thumbScrollable -= needsHorizontal ? 12 : 0;
```

---

## Related Documentation

- [LITHIUM-OSB.md](LITHIUM-OSB.md) — OverlayScrollbars (for Tabulator tables)
- [LITHIUM-FTR.md](LITHIUM-FTR.md) — CodeMirror Editor Footer
- [LITHIUM-CSS.md](LITHIUM-CSS.md) — CSS variables and theming
- [LITHIUM-LIB.md](LITHIUM-LIB.md) — CodeMirror setup and configuration

---

## Implementation History

**April 2026:** Initial implementation
- Created custom scrollbar plugin to replace OverlayScrollbars issues
- Implemented fixed positioning and corner management
- Added content padding to prevent overlap
- Integrated with existing CodeMirror setup

**April 2026:** Refinements
- Added thumb padding (2px from edges)
- Fixed drag constraints for corner areas
- Improved content padding logic
- Enhanced documentation and cross-references

---

**Last Updated:** April 2026  
**Status:** Production-ready, actively maintained