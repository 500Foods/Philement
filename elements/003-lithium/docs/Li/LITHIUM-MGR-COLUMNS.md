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

| Level | Background | CSS Class |
|-------|------------|-----------|
| 1 | Dark blue | `.col-manager-popup` |
| 2 | Dark red | `.col-manager-popup.depth-2` |

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
  // Close all existing column managers first
  this.closeAllColumnManagers();
  
  // Then open this one
  await this.openColumnManager(e, column);
}

closeAllColumnManagers() {
  // Close all column managers in the DOM
  document.querySelectorAll('.col-manager-popup').forEach(popup => {
    const managerId = popup.getAttribute('data-manager-id');
    // Find and cleanup the manager instance
    if (managerInstance) {
      managerInstance.cleanup();
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
2. `cleanup()` is called on the column manager
3. Popup is removed from DOM
4. Parent table is notified of changes

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
| `show()` | Show the popup |
| `hide()` | Hide the popup |
| `cleanup()` | Clean up and destroy |
| `refreshColumnData()` | Refresh data from parent table |
| `setTableWidth(mode)` | Adjust the column manager popup width |

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