# Lithium Column Manager

This document describes the Column Manager component — a recursive table-based interface for managing column configurations in LithiumTable instances.

---

## Overview

The Column Manager is a specialized LithiumTable that displays a list of columns from the parent table, allowing users to configure:

- **Visibility** — Show/hide columns
- **Order** — Reorder columns via drag-and-drop
- **Alignment** — Left, center, right alignment
- **Summary values** — Sum, count, average, min, max
- **Column names** — Custom display names
- **Format** — Column formatting options

### Key Design Decisions

1. **Recursive Architecture** — The Column Manager itself is a LithiumTable, providing consistent UX
2. **Depth Limiting** — Column managers are limited to 2 levels deep to prevent infinite recursion
3. **Shared Template** — Column manager layout is shared across all instances
4. **Singleton Behavior** — Only one column manager can be open at a time

---

## Architecture

### File Structure

```
src/core/
├── lithium-column-manager.js      # Column Manager class
├── lithium-column-manager.css     # Column Manager styling
├── lithium-table-ui.js            # UI mixin with column manager integration
└── lithium-table-base.js          # Base class with useColumnManager option
```

### Class Structure

```javascript
export class LithiumColumnManager {
  constructor(options) {
    this.parentTable = options.parentTable;  // The table being managed
    this.managerId = options.managerId;      // Unique ID for this instance
    this.app = options.app;                  // App instance for API access
    // ...
  }

  async init() {
    // Create popup and initialize LithiumTable
    this.columnTable = new LithiumTable({
      container: tableContainer,
      navigatorContainer: navigatorContainer,
      app: this.app,
      tableDef: columnDef,
      coltypes: await this.getColtypesForManager(),
      cssPrefix: 'lithium',
      storageKey: `col_manager_template`,  // Shared template key
      readonly: false,
      useColumnManager: false,  // Disable recursive column managers
    });
    
    await this.columnTable.init();
    await this.columnTable.replaceData(this.columnData);
  }
}
```

---

## Depth Limiting

To prevent infinite recursion, column managers are limited to 2 levels:

| Level | Description | Column Manager Available | Theme |
|-------|-------------|--------------------------|-------|
| 0 | Main table | Yes | Default (black) |
| 1 | Column manager for main table | Yes | Dark blue |
| 2 | Column manager for column manager | No | Dark red |
| 3+ | Not allowed | Not reachable | N/A |

### Implementation

The depth is controlled via the `useColumnManager` option:

```javascript
// Main table: column manager enabled (default)
const mainTable = new LithiumTable({
  container: mainContainer,
  useColumnManager: true,  // or omit (defaults to true)
});

// Column manager's table: column manager disabled
this.columnTable = new LithiumTable({
  container: tableContainer,
  useColumnManager: false,  // Prevents further recursion
});
```

### Visual Indicators

Different background colors indicate the column manager depth:

| Level | Background | CSS Class | Column Manager Available |
|-------|------------|-----------|--------------------------|
| 0 | Default (black) | N/A | Yes |
| 1 | Dark blue | `.col-manager-popup` | Yes |
| 2 | Dark red | `.col-manager-popup.depth-2` | No |

### Depth Detection

The column manager automatically detects its depth by checking if its parent table's container is inside a `.col-manager-popup` element. This detection is used to:

1. Apply the appropriate theme (blue vs red)
2. Enable/disable the column chooser in the nested table
3. Control recursive behavior

---

## Singleton Behavior

Only one column manager can be open at a time. When opening a column manager:

1. Close all other column managers **except the parent column manager**
2. Close any nested column managers (column managers of column managers)
3. Open the requested column manager

This ensures that when opening a depth-2 column manager (red), the depth-1 column manager (blue) stays open.

### Implementation

```javascript
async toggleColumnManager(e, column) {
  e.stopPropagation();

  if (this.columnManager) {
    this.closeColumnManager();
    return;
  }

  // Close all other column managers first (singleton behavior)
  // But keep the parent column manager open if this is a depth-2 column manager
  const parentColumnManager = this.container.closest('.col-manager-popup');
  this.closeAllColumnManagers(parentColumnManager);

  // Create and open the column manager
  // ...
}

closeAllColumnManagers(parentColumnManager = null) {
  // Close all column managers in the DOM except the parent
  document.querySelectorAll('.col-manager-popup').forEach(popup => {
    // Don't close the parent column manager
    if (popup === parentColumnManager) {
      return;
    }
    
    const managerId = popup.getAttribute('data-manager-id');
    // Find and cleanup the manager instance
    if (popup._columnManagerInstance) {
      popup._columnManagerInstance.cleanup();
    } else {
      popup.remove();
    }
  });
}
```

---

## Shared Template

The column manager's own column configuration (visibility, order, widths) is shared across all instances via a common storage key.

### Storage

```javascript
// Shared template key
storageKey: 'col_manager_template'

// Stored in localStorage
{
  columns: [...],      // Column definitions
  sorters: [...],      // Sort configuration
  filters: [...],      // Filter configuration
  layoutMode: 'fitColumns',
  panelWidth: 500,
  panelHeight: 400
}
```

### Benefits

- Consistent column manager appearance across all tables
- User preferences persist across sessions
- Single template to manage instead of per-table templates

---

## CSS Architecture

### Popup Visibility

The column manager popup uses proper visibility management to prevent mouse event interception when hidden:

```css
.col-manager-popup {
  /* Hidden state */
  opacity: 0;
  visibility: hidden;
  transform: scale(0.95);
  pointer-events: none;
  transition: opacity 0.2s ease, visibility 0.2s ease, transform 0.2s ease;
}

.col-manager-popup.visible {
  /* Visible state */
  opacity: 1;
  visibility: visible;
  transform: scale(1);
  pointer-events: auto;
}
```

This ensures:
- Hidden popups don't intercept mouse events
- Smooth transitions between visible/hidden states
- No "ghost" popups blocking interaction with underlying elements

### Popup Styling

The column manager uses a dark blue theme to distinguish it from the main table:

```css
.col-manager-popup {
  background: linear-gradient(180deg, #0a1628 0%, #0d1e36 100%);
  border: 1px solid #1e3a5f;
  border-radius: 8px;
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.5);
}

/* Depth-2 column manager (dark red) */
.col-manager-popup.depth-2 {
  background: linear-gradient(180deg, #280a0a 0%, #360d0d 100%);
  border: 1px solid #5f1e1e;
}
```

### Table Inside Column Manager

The LithiumTable inside the column manager uses standard `lithium` prefix classes, with overrides in the column manager CSS:

```css
.col-manager-table-container .lithium-table-container {
  background: #0a1628 !important;
  border: none !important;
  margin: 0 !important;
  border-radius: 0 !important;
}
```

---

## Lifecycle

### Opening a Column Manager

1. User clicks the column chooser button (three dots in selector column header)
2. `toggleColumnManager()` is called
3. All existing column managers are closed
4. New column manager is created (lazy initialization)
5. Popup is shown with animation

### Closing a Column Manager

1. User clicks:
   - Close button (X)
   - Escape key
   - Outside the popup
2. `hide()` is called on the column manager
3. Popup is hidden with animation (not removed from DOM)
4. Parent table is notified of changes

### Cleanup vs Hide

- **`hide()`** — Hides the popup but keeps it in DOM for reuse
- **`cleanup()`** — Completely removes the popup from DOM and destroys resources

The column manager uses `hide()` for normal closing to allow quick reopening, and `cleanup()` when destroying the instance completely.

### Important: Closing Behavior

When closing a nested column manager (depth-2):

- **Close the nested manager** — Remove the depth-2 popup
- **Keep the parent manager open** — Don't close the depth-1 popup
- **Apply changes** — Update the parent column manager's table

This prevents the issue where closing a nested manager inadvertently closes the parent manager.

### Navigator Popup Behavior

The column manager's table has its own navigator with popups (width, layout, template). When clicking on these popup items:

- **Don't close the column manager** — The popup items are part of the column manager's interface
- **Allow width/layout changes** — Users should be able to adjust the column manager's table appearance
- **Handle correctly** — Click detection excludes navigator popup elements
- **Resize the popup** — Width popup adjusts the column manager popup width

This ensures that clicking on width popup items doesn't inadvertently close the column manager and instead resizes the popup.

### Click Outside Detection

The column manager's click-outside detection is sophisticated to handle nested popups:

```javascript
handleClickOutside(e) {
  if (this.popup && !this.popup.contains(e.target) && this.popup.classList.contains('visible')) {
    // Check if click was on the anchor element (column chooser button)
    if (this.anchorElement && this.anchorElement.contains(e.target)) {
      return; // Don't close if clicking the anchor
    }
    
    // Check if click is inside any other column manager popup
    const otherPopups = document.querySelectorAll('.col-manager-popup');
    for (const popup of otherPopups) {
      if (popup !== this.popup && popup.contains(e.target)) {
        return; // Don't close if clicking inside another column manager
      }
    }
    
    // Check if click is on a navigator popup item (e.g., width popup)
    if (e.target.closest('.lithium-nav-popup') || 
        e.target.closest('.lithium-nav-popup-item') ||
        e.target.closest('[class*="nav-popup"]')) {
      return; // Don't close if clicking on navigator popup
    }
    
    this.close();
  }
}
```

This ensures that:
1. Clicking on the anchor button doesn't close the popup
2. Clicking inside nested column managers doesn't close the parent
3. Clicking on navigator popup items doesn't close the column manager

---

## Column Data Format

The column manager displays columns in a specific format:

| Field | Type | Description |
|-------|------|-------------|
| `order` | integer | Display order (drag to reorder) |
| `visible` | boolean | Show/hide toggle |
| `field_name` | string | Column field name |
| `column_name` | string | Display name (editable) |
| `format` | select | Column format options |
| `summary` | select | Summary calculation |
| `alignment` | select | Text alignment |

---

## API Reference

### Constructor Options

| Option | Type | Required | Description |
|--------|------|----------|-------------|
| `parentContainer` | HTMLElement | ✅ | Parent table container |
| `anchorElement` | HTMLElement | ✅ | Element to anchor popup to |
| `parentTable` | LithiumTable | ✅ | Table being managed |
| `app` | Object | ✅ | App instance for API access |
| `managerId` | string | ✅ | Unique manager ID |
| `cssPrefix` | string | ❌ | CSS class prefix (default: `'col-manager'`) |
| `onColumnChange` | Function | ❌ | Called when column changes |
| `onClose` | Function | ❌ | Called when manager closes |

### Methods

| Method | Description |
|--------|-------------|
| `init()` | Initialize the column manager |
| `show()` | Show the popup with animation |
| `hide()` | Hide the popup (doesn't remove from DOM) |
| `cleanup()` | Clean up and destroy (removes from DOM) |
| `refreshColumnData()` | Refresh data from parent table |
| `setTableWidth(mode)` | Adjust the column manager popup width |
| `isDepth2ColumnManager()` | Check if this is a depth-2 column manager |
| `positionPopup()` | Position popup relative to anchor element |

---

## Width Presets

Table and popup widths are controlled via CSS variables for consistent theming:

### CSS Variable Definition

Width presets are defined in `base.css`:

```css
:root {
  /* Table width presets (for regular tables) */
  --table-width-narrow: 160px;
  --table-width-compact: 314px;
  --table-width-normal: 468px;
  --table-width-wide: 622px;

  /* Popup width presets (4px wider for border) */
  --table-popup-width-narrow: 164px;
  --table-popup-width-compact: 318px;
  --table-popup-width-normal: 472px;
  --table-popup-width-wide: 626px;
}
```

### Usage

The column manager reads these CSS variables dynamically:

```javascript
setTableWidth(mode) {
  // Get width from CSS variables
  const widthVar = `--table-popup-width-${mode}`;
  const computedStyle = getComputedStyle(document.documentElement);
  const width = computedStyle.getPropertyValue(widthVar).trim();
  
  // Apply width to popup
  this.popup.style.width = width;
}
```

### Benefits

- **Themeable** — Widths can be overridden in theme files
- **Consistent** — Same widths across all panels and managers
- **Maintainable** — Single source of truth for width values
- **Popup-specific** — Popup widths are 4px wider to account for borders

These CSS variables can be overridden in theme files to adjust widths for different themes or padding requirements.

---

## Related Documentation

- [LITHIUM-TAB.md](LITHIUM-TAB.md) — LithiumTable component
- [LITHIUM-MGR-QUERY.md](LITHIUM-MGR-QUERY.md) — Query Manager (uses column manager)
- [LITHIUM-MGR-LOOKUPS.md](LITHIUM-MGR-LOOKUPS.md) — Lookups Manager (uses column manager)

---

## Implementation History

The Column Manager was created to provide a more powerful alternative to the simple column chooser. Key features:

1. **Recursive Design** — Column manager is itself a LithiumTable
2. **Depth Limiting** — Prevents infinite recursion with 2-level limit
3. **Shared Template** — Consistent appearance across all instances
4. **Singleton Behavior** — Only one manager open at a time (except parent)
5. **Theme Differentiation** — Visual indicators for depth level (blue/red)
6. **Immediate Changes** — Cell edits are applied to parent table immediately
7. **Popup Width Control** — Navigator width popup resizes the column manager popup
8. **CSS Variables** — Width presets controlled via CSS variables for theming
9. **Proper Visibility** — Hidden popups use `visibility: hidden` and `pointer-events: none`
10. **Parent Preservation** — Opening depth-2 column manager keeps depth-1 open