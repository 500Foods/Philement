# Lithium Toolbar Implementation Summary

## Overview
This implementation adds a standardized toolbar component to the Lithium project, addressing the need for consistent toolbar styling and behavior across all managers.

## Files Created

### 1. `src/core/lithium-toolbar.css`
New CSS file providing standardized toolbar styles that match the Navigator control bar styling:
- **Base toolbar container** (`.lithium-toolbar`) - 24px height, matching Navigator
- **Left/right sections** (`.lithium-toolbar-left`, `.lithium-toolbar-right`) with nav-block styling:
  - 3px solid outline with -5px offset
  - 1px gap between buttons
  - Rounded corners on first/last buttons
- **Placeholder** (`.lithium-toolbar-placeholder`) - flexible gap between sections
- **Button styles**:
  - `.lithium-toolbar-btn` - standard buttons (accent-primary background, white text)
  - `.lithium-toolbar-tab` - tab-style buttons
  - `.lithium-toolbar-state-btn` - CSS state toggle buttons
- **Button groups** (`.lithium-toolbar-group`) - grouped buttons
- **Automatic text collapsing** via container query (`@container toolbar`)
- **Compact variant** (`.compact`) - even smaller 20px height
- **Popup menus** (`.lithium-toolbar-popup`) - dropdown menus

## Files Modified

### 2. `src/core/manager-ui.js`
Added `LithiumToolbar` JavaScript class with:
- Dynamic button creation (left/right sections)
- Tab button support
- State button support (for Style Manager pattern)
- Collapse button support
- ResizeObserver for placeholder width monitoring
- Automatic text collapse when placeholder < 50px
- Cleanup methods

**Key Methods:**
- `addLeftButton(config)` - Add button to left section
- `addRightButton(config)` - Add button to right section
- `addTab(config)` - Add tab-style button
- `addStateButton(config)` - Add CSS state toggle button
- `setTitle(title)` - Set toolbar title
- `destroy()` - Cleanup resources

**Factory Functions:**
- `createToolbar(container, options)` - Create new toolbar
- `setupToolbar(container, options)` - Setup from existing HTML

### 3. `src/core/manager-panels.css`
Added import for new toolbar CSS:
```css
@import url('./lithium-toolbar.css');
```

### 4. `docs/Li/LITHIUM-MGR.md`
Added documentation section for LithiumToolbar component with:
- Architecture overview
- Feature list
- Usage examples
- HTML structure reference
- Text collapsing behavior explanation

### 5. `src/managers/style-manager/style-manager.html`
Updated to demonstrate new toolbar system:
- **Main toolbar** (Section/JSON toggle + action buttons)
- **Secondary toolbar** (Target name + state buttons)
- Uses new `.lithium-toolbar` classes
- Shows multiple toolbars in one manager

### 6. `src/managers/style-manager/style-manager.css`
Cleaned up old toolbar styles, kept backwards compatibility classes.

### 7. `src/managers/queries/queries.html`
Updated to use new toolbar classes:
- Tabs on the left (SQL, Test, Summary, Preview, Collection)
- Action buttons on the right (Undo, Redo, Fold, Unfold, Font, Prettify)
- Uses `data-collapsible-text` for automatic text hiding

### 8. `src/managers/queries/queries.js`
Fixed tab selector to use `[data-tab]` instead of old class name.

### 9. `src/managers/queries/queries.css`
Updated to remove old container query approach in favor of new toolbar system.

### 10. `src/managers/lookups/lookups.html`
Updated to use new toolbar classes with matching nav-block styling.

### 11. `src/managers/lookups/lookups.js`
Fixed tab selector to use `[data-tab]` instead of old class name.

### 12. `src/managers/version-history/version-history.html`
Updated to use new toolbar classes with title display.

## Key Features

### 1. Nav-Block Styling (Matching Navigator)
Toolbars now use the same visual styling as the Navigator control bar:
```css
outline: 3px solid var(--accent-primary);
outline-offset: -5px;
gap: 1px;
```

### 2. Smaller Size (24px height)
Matches the LithiumTable navigator size, providing visual consistency.

### 3. Standardized Left/Right Button Placement
```html
<div class="lithium-toolbar">
  <div class="lithium-toolbar-left"><!-- tabs/title --></div>
  <div class="lithium-toolbar-placeholder"></div>
  <div class="lithium-toolbar-right"><!-- actions --></div>
</div>
```

### 4. Automatic Text Collapsing
Buttons with `data-collapsible-text` automatically hide their text labels when the placeholder width drops below 50px. Two approaches:
- **CSS Container Query**: `@container toolbar (max-width: 400px)`
- **JavaScript**: `LithiumToolbar` class monitors placeholder and adds `collapsed-text` class

### 5. Multiple Toolbars Per Manager
The Style Manager demonstrates this with:
- Top toolbar for Section/JSON mode toggle
- Bottom toolbar for CSS state selection

## Usage Examples

### JavaScript API
```javascript
import { createToolbar } from '../../core/manager-ui.js';

const toolbar = createToolbar(document.getElementById('toolbar'));

// Add tabs
toolbar.addTab({
  icon: 'fa-database',
  label: 'SQL',
  active: true,
  onClick: () => this.switchTab('sql')
});

// Add action buttons with collapsible text
toolbar.addRightButton({
  icon: 'fa-rotate-left',
  label: 'Undo',
  collapsibleText: true,
  onClick: () => this.handleUndo()
});
```

### Static HTML
```html
<div class="lithium-toolbar">
  <div class="lithium-toolbar-left">
    <button class="lithium-toolbar-tab active" data-tab="preview">
      <fa fa-eye></fa>
      <span>Preview</span>
    </button>
  </div>
  <div class="lithium-toolbar-placeholder"></div>
  <div class="lithium-toolbar-right">
    <button class="lithium-toolbar-btn" data-collapsible-text>
      <fa fa-undo></fa>
      <span>Undo</span>
    </button>
  </div>
</div>
```

## Migration Guide

### For Existing Managers

1. **Replace toolbar container class:**
   - Old: `<div class="custom-tabs-header">`
   - New: `<div class="lithium-toolbar">`

2. **Add left/right sections:**
   - Wrap left buttons in `<div class="lithium-toolbar-left">`
   - Add `<div class="lithium-toolbar-placeholder">` for gap
   - Wrap right buttons in `<div class="lithium-toolbar-right">`

3. **Update button classes:**
   - Old: `<button class="custom-tab-btn">`
   - New: `<button class="lithium-toolbar-tab" data-tab="id">`

4. **Update JavaScript selectors:**
   - Old: `querySelectorAll('.custom-tab-btn')`
   - New: `querySelectorAll('[data-tab]')`

5. **Add collapsible text attribute:**
   - Add `data-collapsible-text` to buttons that should collapse

6. **Remove old CSS:**
   - Remove container queries for text hiding (now handled automatically)
   - Remove custom toolbar styles (now in lithium-toolbar.css)

## Backwards Compatibility

The existing managers (queries, lookups, etc.) will continue to work with their current styles. The new system is opt-in - managers can be migrated individually.

## Testing

Build verified: `npm run build` passes successfully.

## Managers Updated

- ✅ Query Manager - Full toolbar implementation
- ✅ Lookups Manager - Full toolbar implementation
- ✅ Style Manager - Dual toolbar implementation (main + state toolbar)
- ✅ Version Manager - Simple toolbar with title
