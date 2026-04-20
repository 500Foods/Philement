# OverlayScrollbars Integration (Lithium OSB)

This document describes the scrollbar implementation in Lithium:

- **Tabulator tables**: Native CSS scrollbars with consistent cross-browser styling
- **CodeMirror/SunEditor**: OverlayScrollbars for enhanced control
- **Popups/Dropdowns**: OverlayScrollbars with scroll containment

**Important:** OverlayScrollbars is intentionally NOT used for Tabulator tables because it conflicts with Tabulator's virtual scrolling mechanism.

---

## Overview

Lithium uses [OverlayScrollbars](https://kingsora.github.io/OverlayScrollbars/) to provide consistent scrollbar styling across all major browsers. Unlike native CSS scrollbars that have limited Firefox support, OverlayScrollbars provides:

- **Rounded scrollbar track and thumb** with consistent appearance
- **Inset thumb effect** via padding/border trick
- **Visible arrow buttons** at track ends (top/bottom for vertical, left/right for horizontal)
- **Cross-browser consistency** including Firefox
- **Preserved native scroll behavior** for performance
- **Theme-aware styling** via CSS variables

---

## Why OverlayScrollbars?

### The Problem with Pure CSS

Pure CSS scrollbars using `::-webkit-scrollbar` work well in Chrome/Safari/Edge but Firefox only supports `scrollbar-width` and `scrollbar-color` with minimal styling control:

- No rounded corners on the track in Firefox
- No inset/padding effect on the thumb
- No visible arrow buttons
- Different behavior between browsers

### Why Not SimpleBar?

SimpleBar was considered but OverlayScrollbars was chosen because:

- **Better styling control** via CSS custom properties and class-based overrides
- **Active maintenance** and modern API
- **Better virtualization compatibility** with Tabulator-style virtual scrolling
- **Smaller footprint** for the features we need
- **Explicit Firefox support** for all visual features

---

## Architecture

### Files

| File | Purpose |
|------|---------|
| `src/core/scrollbar-manager.js` | Centralized OverlayScrollbars initialization and management |
| `src/styles/scrollbars.css` | Lithium theme styles for OverlayScrollbars |
| `src/styles/vendor-fixes.css` | Compatibility overrides for Tabulator, CodeMirror, SunEditor |
| `src/styles/base.css` | CSS variables for scrollbar theming |

### Integration Points

```
┌─────────────────────────────────────────────────────────────┐
│                    Scrollbar Manager                         │
│              (src/core/scrollbar-manager.js)                 │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │  Tabulator   │  │  CodeMirror  │  │  SunEditor   │      │
│  │   Tables     │  │   Editors    │  │   Editor     │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
```

---

## Usage

### For Tabulator Tables (LithiumTable)

Tabulator tables use **native CSS scrollbars** (not OverlayScrollbars) due to virtual scrolling incompatibility:

```css
/* Native scrollbar styling in vendor-fixes.css */
.lithium-table-container .tabulator-tableholder {
  scrollbar-width: thin;
  scrollbar-color: var(--accent-alt-primary) var(--bg-secondary);
}

.lithium-table-container .tabulator-tableholder::-webkit-scrollbar {
  width: 8px;
  height: 8px;
}

.lithium-table-container .tabulator-tableholder::-webkit-scrollbar-thumb {
  background: var(--accent-alt-primary);
  border-radius: 16px;
}
```

**Why native CSS?** OverlayScrollbars modifies the DOM structure (wrapping content, hiding native scrollbars) which breaks Tabulator's virtual scrolling that relies on specific scroll events and DOM structure.

### For CodeMirror Editors

Use the helper function after creating the editor:

```javascript
import { initCodeMirrorScrollbars } from '../../core/codemirror-setup.js';

const view = new EditorView({
  state: EditorState.create({ doc: '', extensions }),
  parent: container,
});

// Initialize OverlayScrollbars
const osInstance = initCodeMirrorScrollbars(view);

// Cleanup when destroying
scrollbarManager.destroy(osInstance);
```

### For SunEditor

OverlayScrollbars is automatically initialized in the Lookups Manager when SunEditor is created:

```javascript
// In lookups.js, automatically applied:
const sunEditorElement = this.elements.summaryEditor?.closest('.sun-editor');
if (sunEditorElement) {
  this._sunEditorScrollbar = scrollbarManager.initSunEditor(sunEditorElement);
}
```

### For Generic Elements

Use the generic initialization method:

```javascript
import { scrollbarManager } from '../../core/scrollbar-manager.js';

// Initialize on any scrollable element
const osInstance = scrollbarManager.initGeneric(element, {
  minimal: false, // Use minimal style
});

// Cleanup
scrollbarManager.destroy(osInstance);
```

---

## CSS Theme Customization

### CSS Variables

The scrollbar appearance is controlled via CSS variables in `base.css`:

```css
:root {
  /* Base scrollbar colors */
  --scrollbar-track: var(--bg-secondary);
  --scrollbar-thumb: var(--bg-tertiary);
  --scrollbar-thumb-hover: var(--border-color-hover);

  /* OverlayScrollbars-specific - Lithium Style */
  --os-size: 8px;           /* Track width */
  --os-thumb-size: 6px;     /* Thumb width (creates 1px "border") */
  --os-padding: 1px;        /* Creates 1px space = border effect */
  --os-track-bg: var(--bg-secondary);
  --os-track-bg-hover: var(--bg-secondary);
  --os-thumb-bg: var(--accent-alt-primary);     /* Default: accent-alt-primary */
  --os-thumb-bg-hover: var(--accent-primary);   /* Hover: accent-primary */
  --os-thumb-bg-active: var(--accent-primary);  /* Active: accent-primary */
  --os-thumb-border-radius: 16px;
  --os-track-border-radius: 16px;
}
```

### Visual Design

- **Track**: 8px wide, rounded 16px, background matches `--bg-secondary`
- **Thumb**: 6px wide, centered in track (1px padding creates border effect)
- **Colors**: 
  - Default: `--accent-alt-primary` (muted accent)
  - Hover/Active: `--accent-primary` (bright accent)
- **Rounding**: 16px border radius on both track and thumb
- **Buttons**: Disabled for minimal, modern look

### Theme Classes

| Class | Description |
|-------|-------------|
| `.os-theme-lithium` | Default Lithium dark theme |
| `.os-theme-lithium-minimal` | Minimal/auto-hide style |
| `.os-theme-lithium-tabulator` | Optimized for Tabulator tables |
| `.os-theme-lithium-codemirror` | Optimized for CodeMirror editors |
| `.os-theme-lithium-suneditor` | Optimized for SunEditor |

### Customizing the Theme

To override styles, use the `@layer vendor-fixes` layer:

```css
@layer vendor-fixes {
  .os-theme-lithium {
    --os-size: 16px;
    --os-thumb-bg: #555;
    --os-thumb-bg-hover: #777;
  }
}
```

---

## Configuration

### Default Configuration

```javascript
const BASE_CONFIG = {
  className: 'os-theme-lithium',
  resize: 'both',
  paddingAbsolute: true,  // Classic mode - reserves space
  scrollbars: {
    theme: 'lithium',
    visibility: 'auto',   // Hide when content fits
    autoHide: 'never',    // Always visible when overflow exists
    autoHideDelay: 800,
    dragScroll: true,
    clickScroll: true,
    pointers: ['mouse', 'touch', 'pen'],
  },
};
```

### Component-Specific Configurations

**Tabulator:**
- Classic mode (`paddingAbsolute: true`) - reserves 8px space
- Thumb: 6px, centered with 1px padding
- Virtual scrolling compatibility

**CodeMirror:**
- Click scroll disabled (CodeMirror handles its own)
- Matches editor gutter styling

**SunEditor:**
- Targets `.se-wrapper-inner` or `.se-wrapper`
- Minimal button styling

**Popups:**
- `overscroll-behavior: contain` prevents scroll propagation
- Isolated scroll context from parent table

**Popups:**
- Prevents scroll propagation to parent containers
- Uses `overscroll-behavior: contain`
- Isolated scroll context

---

## Popup/Dropdown Scrollbars

For lookup dropdowns and popups that need independent scrolling:

### Usage

```javascript
import { scrollbarManager } from '../../core/scrollbar-manager.js';

// Initialize on a popup/dropdown element
const dropdown = document.querySelector('.tabulator-edit-list');
const osInstance = scrollbarManager.initPopup(dropdown);

// Cleanup when popup closes
scrollbarManager.destroy(osInstance);
```

### Features

- **Scroll containment**: Scrolling in the popup doesn't scroll the parent table
- **Independent context**: Popup has its own scrollbar state
- **Auto-hide**: Only shows when content overflows

### CSS Classes

| Class | Purpose |
|-------|---------|
| `.lithium-popup-scrollable` | Applied to popup elements |
| `[data-overlayscrollbars~="popup"]` | Attribute for CSS targeting |

---

## API Reference

### ScrollbarManager Methods

| Method | Description |
|--------|-------------|
| `init(element, config, callbacks)` | Initialize on any element |
| `initCodeMirror(scroller, callbacks)` | Initialize for CodeMirror |
| `initSunEditor(editorElement, callbacks)` | Initialize for SunEditor |
| `initPopup(element, callbacks)` | Initialize for popup/dropdown (scroll containment) |
| `initGeneric(element, options, callbacks)` | Initialize with options |
| `update(instance)` | Update after content changes |
| `destroy(instance)` | Destroy an instance |
| `destroyAll()` | Destroy all instances |
| `getInstance(element)` | Get instance for element |
| `hasInstance(element)` | Check if element has instance |
| `scrollTo(instance, position, options)` | Programmatic scroll |
| `getScrollPosition(instance)` | Get current scroll position |
| `isScrollable(element)` | Check if element is scrollable |

### Callbacks

All `init*` methods accept a callbacks object:

```javascript
{
  initialized: (instance) => { },
  destroyed: (instance, destroyedBy) => { },
  scroll: (instance, event) => { },
}
```

---

## Troubleshooting

### Scrollbars Not Appearing

1. Check that the element has `overflow: auto` or `overflow: scroll`
2. Verify content is larger than container (scrollable content required)
3. Check browser console for OverlayScrollbars errors
4. Ensure CSS file is imported: `import '../../styles/scrollbars.css'`

### Visual Glitches

1. Verify `vendor-fixes.css` is loaded (provides compatibility)
2. Check for conflicting CSS with `!important` rules
3. Ensure proper z-index stacking context

### Performance Issues

1. Use `update()` sparingly — don't call on every scroll
2. For Tabulator, let virtual scrolling handle most updates
3. Consider `minimal` mode for scroll-heavy UIs

### Firefox-Specific Issues

If scrollbars appear different in Firefox:
1. This is expected for native CSS scrollbars
2. OverlayScrollbars should normalize the appearance
3. Check that `scrollbar-width: auto` is set on the element

---

## Migration from Native CSS

### Before (Native CSS)

```css
/* WebKit only */
::-webkit-scrollbar {
  width: 10px;
}
::-webkit-scrollbar-thumb {
  background: #555;
  border-radius: 5px;
}

/* Firefox - limited styling */
* {
  scrollbar-width: auto;
  scrollbar-color: #555 #222;
}
```

### After (OverlayScrollbars)

```javascript
// Initialize via JavaScript
import { scrollbarManager } from '../../core/scrollbar-manager.js';

const osInstance = scrollbarManager.initGeneric(element);
```

```css
/* CSS variables control appearance */
.os-theme-lithium {
  --os-size: 14px;
  --os-thumb-bg: #555;
  --os-track-bg: #222;
}
```

---

## Related Documentation

- [LITHIUM-TAB.md](LITHIUM-TAB.md) — LithiumTable component
- [LITHIUM-CSS.md](LITHIUM-CSS.md) — CSS architecture and theming
- [LITHIUM-LIB.md](LITHIUM-LIB.md) — Third-party libraries
- [OverlayScrollbars Documentation](https://kingsora.github.io/OverlayScrollbars/)

---

## Implementation History

**April 2026:** Initial OverlayScrollbars integration
- Added `overlayscrollbars` npm package
- Created `scrollbar-manager.js` for centralized management
- Created `scrollbars.css` with Lithium theme
- Attempted integration with Tabulator, CodeMirror, and SunEditor

**April 2026 (Update):** Hybrid approach adopted
- **Tabulator tables**: Reverted to native CSS scrollbars due to virtual scrolling incompatibility
  - 8px track, 6px thumb (1px padding creates border effect)
  - 16px border radius
  - Thumb colors: `--accent-alt-primary` (default), `--accent-primary` (hover)
- **CodeMirror/SunEditor**: Retained OverlayScrollbars
- **Popups**: Added `initPopup()` with scroll containment via `overscroll-behavior: contain`

---

**Last Updated:** April 19, 2026
