# Lithium Toolbar System (LithiumBar)

Standardized toolbar component for manager interfaces, providing consistent styling, automatic text collapsing, and support for multiple toolbars per manager.

---

## Overview

The Lithium Toolbar system provides a standardized way to create toolbars in manager interfaces. It matches the Navigator control bar styling (24px height, outline styling, 1px gap between buttons) and supports automatic text collapsing when the toolbar shrinks.

**Key Features:**
- Consistent 24px height matching Navigator control bars
- Nav-block styling with 3px outline and 1px gap between buttons
- Automatic text collapsing when toolbar width is constrained
- Support for multiple toolbars per manager
- Left/right section layout with flexible placeholder

---

## CSS Architecture

**Primary File:** `src/core/lithium-toolbar.css`

The toolbar CSS is automatically imported via `manager-panels.css`, so managers using the shared panel system get toolbar styles automatically.

### CSS Classes

| Class | Purpose |
|-------|---------|
| `.lithium-toolbar` | Main container with nav-block styling |
| `.lithium-toolbar-left` | Left section (tabs, title, collapse buttons) |
| `.lithium-toolbar-right` | Right section (action buttons) |
| `.lithium-toolbar-placeholder` | Flexible gap between left and right |
| `.lithium-toolbar-btn` | Standard action button |
| `.lithium-toolbar-tab` | Tab-style button (for view switching) |
| `.lithium-toolbar-state-btn` | CSS state toggle (for Style Manager pattern) |
| `.lithium-toolbar-title` | Static title text |

---

## HTML Structure

The toolbar uses a **flat structure**: buttons, placeholder, and more buttons are all direct children of the toolbar container. This enables proper outline styling with automatic first/last child rounding.

### Basic Toolbar

```html
<div class="lithium-toolbar">
  <!-- Left buttons (before placeholder) -->
  <button class="lithium-toolbar-tab active" data-tab="view1" data-collapsible-text>
    <fa fa-icon></fa>
    <span>Tab Label</span>
  </button>

  <!-- Placeholder fills available space -->
  <div class="lithium-toolbar-placeholder"></div>

  <!-- Right buttons (after placeholder) -->
  <button class="lithium-toolbar-btn" data-collapsible-text>
    <fa fa-undo></fa>
    <span>Undo</span>
  </button>
</div>
```

### With Collapse Buttons

```html
<div class="lithium-toolbar">
  <!-- Collapse button (icon only, no text to collapse) -->
  <button class="lithium-toolbar-btn lithium-collapse-btn"
          data-collapse-target="left" title="Toggle Left Panel">
    <fa fa-angles-left class="lithium-collapse-icon"></fa>
  </button>
  <!-- Tab buttons (with collapsible text) -->
  <button class="lithium-toolbar-tab active" data-tab="json" data-collapsible-text>
    <fa fa-brackets-curly></fa>
    <span>JSON</span>
  </button>
  <button class="lithium-toolbar-tab" data-tab="summary" data-collapsible-text>
    <fa fa-file-lines></fa>
    <span>Summary</span>
  </button>

  <!-- Placeholder fills available space -->
  <div class="lithium-toolbar-placeholder"></div>

  <!-- Action buttons (with collapsible text) -->
  <button class="lithium-toolbar-btn" data-collapsible-text>
    <fa fa-undo></fa>
    <span>Undo</span>
  </button>
</div>
```

### Title-Only Toolbar (Version Manager Pattern)

```html
<div class="lithium-toolbar">
  <button class="lithium-toolbar-btn lithium-collapse-btn" 
          data-collapse-target="left">
    <fa fa-angles-left class="lithium-collapse-icon"></fa>
  </button>
  <span class="lithium-toolbar-title">Panel Title</span>
  
  <!-- Placeholder fills available space -->
  <div class="lithium-toolbar-placeholder"></div>
  
  <!-- optional action buttons -->
</div>
```

### Multiple Toolbars (Style Manager Pattern)

```html
<!-- Main toolbar -->
<div class="lithium-toolbar" id="main-toolbar">
  <!-- Left: tabs -->
  <button class="lithium-toolbar-tab active">...</button>
  <button class="lithium-toolbar-tab">...</button>
  
  <!-- Placeholder -->
  <div class="lithium-toolbar-placeholder"></div>
  
  <!-- Right: actions -->
  <button class="lithium-toolbar-btn" data-collapsible-text>...</button>
</div>

<!-- Secondary toolbar -->
<div class="lithium-toolbar" id="state-toolbar">
  <span class="lithium-toolbar-title">Target Name</span>
  
  <!-- Placeholder -->
  <div class="lithium-toolbar-placeholder"></div>
  
  <!-- State buttons -->
  <button class="lithium-toolbar-state-btn active" data-state="default">Default</button>
  <button class="lithium-toolbar-state-btn" data-state=":hover">:HOVER</button>
</div>
```

---

## JavaScript API

**Module:** `src/core/manager-ui.js`

### LithiumToolbar Class

```javascript
import { createToolbar } from '../../core/manager-ui.js';

const toolbar = createToolbar(document.getElementById('toolbar-container'), {
  onCollapseChange: (isCollapsed) => {
    // Called when text collapsing state changes
  }
});
```

### Methods

| Method | Description |
|--------|-------------|
| `addLeftButton(config)` | Add button to left section |
| `addRightButton(config)` | Add button to right section |
| `addTab(config)` | Add tab-style button |
| `addStateButton(config)` | Add CSS state toggle button |
| `addCollapseButton(config)` | Add panel collapse button |
| `setTitle(title)` | Set toolbar title text |
| `clear()` | Remove all buttons |
| `recalculateBreakpoint()` | Recalculate collapse breakpoint after label changes |
| `destroy()` | Cleanup resources |

### Button Configuration

```javascript
toolbar.addRightButton({
  icon: 'fa-rotate-left',      // Font Awesome icon class
  label: 'Undo',               // Button text (optional)
  title: 'Undo (Ctrl+Z)',      // Tooltip text
  collapsibleText: true,       // Hide text when toolbar shrinks
  disabled: false,             // Initial disabled state
  active: false,               // Initial active state
  onClick: (btn) => {          // Click handler
    this.handleUndo();
  }
});
```

### Tab Configuration

```javascript
toolbar.addTab({
  icon: 'fa-database',
  label: 'SQL',
  active: true,
  onClick: (btn) => {
    this.switchTab('sql');
  }
});
```

### State Button Configuration

```javascript
toolbar.addStateButton({
  label: ':HOVER',
  state: 'hover',
  active: false,
  onClick: (btn, state) => {
    this.handleStateToggle(state);
  }
});
```

---

## Text Collapsing Behavior

Buttons with text labels can automatically collapse to show only icons when the toolbar becomes narrow.

### How It Works

1. **Calculate breakpoint once**: Sum of all button widths + gap widths + 50px buffer
2. **Monitor toolbar width**: Uses ResizeObserver on the toolbar container
3. **Collapse when needed**: When toolbar width < breakpoint, text collapses
4. **Cached for performance**: Breakpoint is calculated once and reused

### Usage

Add `data-collapsible-text` to **any button or tab** with text that should collapse:

```html
<!-- Tab buttons (left side) -->
<button class="lithium-toolbar-tab" data-tab="sql" data-collapsible-text>
  <fa fa-database></fa>
  <span>SQL</span>
</button>

<!-- Action buttons (right side) -->
<button class="lithium-toolbar-btn" data-collapsible-text>
  <fa fa-undo></fa>
  <span>Undo</span>
</button>
```

Or via JavaScript:

```javascript
// Tab buttons
toolbar.addTab({
  icon: 'fa-database',
  label: 'SQL',
  collapsibleText: true,  // Sets data-collapsible-text attribute
  onClick: () => this.switchTab('sql')
});

// Action buttons
toolbar.addRightButton({
  icon: 'fa-undo',
  label: 'Undo',
  collapsibleText: true,  // Sets data-collapsible-text attribute
  onClick: () => this.handleUndo()
});
```

### Recalculating After Content Changes

If button labels change dynamically (e.g., after a language switch), recalculate the breakpoint:

```javascript
// Update button labels...
undoButton.querySelector('span').textContent = 'Annuler';  // French

// Recalculate the collapse breakpoint
toolbar.recalculateBreakpoint();
```

### Static HTML Toolbars

For toolbars defined in HTML templates, use `initToolbars()`:
initDynamicToolbars();

// Or with custom selector
initDynamicToolbars('.my-custom-toolbar');
```

2. **JavaScript ResizeObserver**:
   - The `LithiumToolbar` class monitors the placeholder width
   - When placeholder < 50px (configurable), adds `collapsed-text` class
   - This hides text on buttons with `data-collapsible-text` attribute

### Usage

```javascript
import { createToolbar } from '../../core/manager-ui.js';

// Create toolbar (breakpoint auto-calculated from button widths)
const toolbar = createToolbar(document.getElementById('toolbar'), {
  onCollapseChange: (isCollapsed) => {
    log(Subsystems.MANAGER, Status.INFO, `Toolbar collapsed: ${isCollapsed}`);
  }
});

// Add tab buttons (left side)
toolbar.addTab({
  icon: 'fa-database',
  label: 'SQL',
  active: true,
  onClick: () => this.switchTab('sql')
});

toolbar.addTab({
  icon: 'fa-file-lines',
  label: 'Summary',
  onClick: () => this.switchTab('summary')
});

// Add action buttons (right side, with collapsible text)
toolbar.addRightButton({
  icon: 'fa-rotate-left',
  label: 'Undo',
  collapsibleText: true,  // Text will hide when toolbar shrinks
  onClick: () => this.handleUndo()
});

toolbar.addRightButton({
  icon: 'fa-rotate-right',
  label: 'Redo',
  collapsibleText: true,
  onClick: () => this.handleRedo()
});

// Add state buttons (for Style Manager pattern)
toolbar.addStateButton({
  label: ':HOVER',
  state: 'hover',
  onClick: (btn, state) => this.handleStateToggle(state)
});

// If button labels change (e.g., language switch), recalculate breakpoint
toolbar.recalculateBreakpoint();

// Cleanup
toolbar.destroy();
```

Or via JavaScript:

```javascript
toolbar.addRightButton({
  icon: 'fa-undo',
  label: 'Undo',
  collapsibleText: true  // Sets data-collapsible-text attribute
});
```

---

## Styling Reference

### Nav-Block Styling

Toolbars match the Navigator control bar styling with outline on the container:

```css
.lithium-toolbar {
  display: flex;
  align-items: center;
  gap: 1px;
  height: 24px;
  line-height: 1;
  /* Outline creates the nav-block appearance */
  outline: 3px solid var(--accent-primary);
  outline-offset: -5px;
  border-radius: var(--border-radius-sm);
  overflow: hidden;
  background-color: var(--accent-primary);
}

/* First button gets left rounding */
.lithium-toolbar > *:first-child {
  border-top-left-radius: var(--border-radius-sm);
  border-bottom-left-radius: var(--border-radius-sm);
}

/* Last button gets right rounding */
.lithium-toolbar > *:last-child {
  border-top-right-radius: var(--border-radius-sm);
  border-bottom-right-radius: var(--border-radius-sm);
}
```

### Button Styling

All toolbar buttons use:
- `background-color: var(--accent-primary)`
- `color: white`
- `height: 24px`
- `font-size: 11px`
- `border-radius: 0` (container handles rounding)
- `flex-shrink: 0` (prevents buttons from shrinking)

### Active State

Active buttons use success color:
```css
.lithium-toolbar-btn.active,
.lithium-toolbar-tab.active {
  background-color: var(--accent-success) !important;
}
```

---

## Migration from Old Toolbars

### Step 1: Update HTML Structure

**Before (old structure with left/right sections):**
```html
<div class="lithium-toolbar">
  <div class="lithium-toolbar-left">
    <button class="lithium-toolbar-tab active" data-tab="sql">SQL</button>
  </div>
  <div class="lithium-toolbar-placeholder"></div>
  <div class="lithium-toolbar-right">
    <button class="lithium-toolbar-btn" data-collapsible-text>Undo</button>
  </div>
</div>
```

**After (flat structure):**
```html
<div class="lithium-toolbar">
  <!-- Left buttons first -->
  <button class="lithium-toolbar-tab active" data-tab="sql">SQL</button>
  
  <!-- Placeholder in the middle -->
  <div class="lithium-toolbar-placeholder"></div>
  
  <!-- Right buttons after placeholder -->
  <button class="lithium-toolbar-btn" data-collapsible-text>Undo</button>
</div>
```

### Step 2: Update JavaScript Selectors

**Before:**
```javascript
this.tabBtns = this.container.querySelectorAll('.custom-tab-btn');
```

**After:**
```javascript
this.tabBtns = this.container.querySelectorAll('[data-tab]');
```

### Step 3: Remove Old CSS

Remove custom toolbar styles from manager CSS files. The toolbar styles are now provided by `lithium-toolbar.css` (imported via `manager-panels.css`).

---

## Examples by Manager

### Query Manager

- **Left:** Collapse button + 5 tabs (SQL, Test, Summary, Preview, Collection)
- **Right:** Undo, Redo, Fold, Unfold, Font, Prettify (with collapsible text)

### Lookups Manager

- **Left:** 2 collapse buttons + 3 tabs (Collection, Summary, Preview)
- **Right:** Undo, Redo, Fold, Unfold, Font (with collapsible text)

### Style Manager

- **Main Toolbar:**
  - Left: 2 collapse buttons + 2 tabs (Section, JSON)
  - Right: Undo, Redo, Fold, Unfold, Font, Prettify
- **State Toolbar:**
  - Left: Title (target name)
  - Right: State buttons (Default, :HOVER, :FOCUS, :DISABLED, CSS)

### Version Manager

- **Left:** Collapse button + Title ("Version Summary")
- **Right:** (empty, reserved for future actions)

---

## Related Documentation

- [LITHIUM-MGR.md](LITHIUM-MGR.md) - Manager system overview
- [LITHIUM-MGR-NEW.md](LITHIUM-MGR-NEW.md) - Creating a new manager (includes toolbar setup)
- [LITHIUM-TAB.md](LITHIUM-TAB.md) - LithiumTable component (Navigator styling reference)
- [LITHIUM-CSS.md](LITHIUM-CSS.md) - CSS architecture and variables

---

Last updated: April 2026
