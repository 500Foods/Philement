# LithiumTable Component

This document describes the `LithiumTable` reusable component — a standardized Tabulator data grid with integrated Navigator control bar.

---

## Overview

The `LithiumTable` component combines:

- **Tabulator** — Feature-rich data grid library
- **Navigator** — Custom control bar with Control, Move, Manage, Search blocks
- **JSON-driven configuration** — Column definitions from `config/tabulator/`
- **Edit mode** — Inline editing with dirty state tracking
- **Templates** — Save/load column configurations

The component is modular, reusable across all managers, and provides consistent table behavior throughout Lithium.

---

## Architecture

### File Structure

```structure
src/core/
├── lithium-table-base.js      # Core functionality (init, events, navigation)
├── lithium-table-ops.js       # CRUD operations mixin
├── lithium-table-ui.js        # Navigator UI and popups mixin
├── lithium-table-main.js      # Combined LithiumTable class export
└── lithium-table.css          # Component styles
```

### Class Hierarchy

```javascript
// lithium-table-main.js
export class LithiumTable extends LithiumTableBase {
  constructor(options) { super(options); }
}

// Apply mixins
Object.assign(LithiumTable.prototype, LithiumTableOpsMixin, LithiumTableUIMixin);
```

**Mixins provide:**

- **LithiumTableBase** — Initialization, config loading, Tabulator management, events, navigation
- **LithiumTableOpsMixin** — Add, duplicate, edit, save, cancel, delete operations
- **LithiumTableUIMixin** — Navigator UI, popups, column chooser, filter editors

---

## Usage

### Basic Usage

```javascript
import { LithiumTable } from '../../core/lithium-table-main.js';

const table = new LithiumTable({
  // Required
  container: document.getElementById('table-container'),
  navigatorContainer: document.getElementById('nav-container'),
  app: this.app,  // App instance for API calls

  // Data source (one of)
  queryRef: 25,                    // QueryRef for data loading
  tablePath: 'queries/query-manager',  // JSON config path

  // Optional
  cssPrefix: 'mytable',            // CSS class prefix (default: 'lithium')
  storageKey: 'mytable_config',    // localStorage key for templates
  readonly: false,                 // Enable editing (default: false)

  // Callbacks
  onRowSelected: (rowData) => { /* ... */ },
  onRowDeselected: () => { /* ... */ },
  onDataLoaded: (rows) => { /* ... */ },
  onEditModeChange: (isEditing) => { /* ... */ },
});

await table.init();
await table.loadData();
```

### Full Options Reference

| Option | Type | Required | Description |
|--------|------|----------|-------------|
| `container` | HTMLElement | ✅ | Table container element |
| `navigatorContainer` | HTMLElement | ✅ | Navigator bar container |
| `app` | Object | ✅ | App instance (for API access) |
| `queryRef` | number | ⚪ | QueryRef for data loading |
| `tablePath` | string | ⚪ | Path to JSON table definition |
| `tableDef` | Object | ⚪ | Pre-loaded table definition |
| `coltypes` | Object | ⚪ | Pre-loaded coltypes |
| `cssPrefix` | string | ❌ | CSS class prefix (default: `'lithium'`) |
| `storageKey` | string | ❌ | localStorage key (default: `'lithium_table'`) |
| `readonly` | boolean | ❌ | Disable editing (default: `false`) |
| `searchQueryRef` | number | ⚪ | QueryRef for search |
| `detailQueryRef` | number | ⚪ | QueryRef for detail loading |
| `updateQueryRef` | number | ⚪ | QueryRef for updates |
| `insertQueryRef` | number | ⚪ | QueryRef for inserts |
| `deleteQueryRef` | number | ⚪ | QueryRef for deletes |
| `onRowSelected` | Function | ❌ | Called when row selected |
| `onRowDeselected` | Function | ❌ | Called when row deselected |
| `onDataLoaded` | Function | ❌ | Called when data loaded |
| `onEditModeChange` | Function | ❌ | Called when edit mode changes |

---

## The Navigator Block

The Navigator provides standard table controls in four groups:

### 1. Control Block

| Button | Action |
|--------|--------|
| 🔄 Refresh | Reload data from source |
| 🔽 Filter | Toggle column header filters |
| ☰ Menu | Table options popup (expand/collapse all) |
| ↔ Width | Table width presets (Narrow/Compact/Normal/Wide/Auto) |
| ⊞ Layout | Layout mode (fitColumns/fitData/fitDataFill/etc) |
| 🛠 Template | Save/load column configurations |

### 2. Move Block

| Button | Action | Keyboard |
|--------|--------|----------|
| ⏮ First | Go to first record | `Home` |
| ⏪ Previous Page | Page up | `Page Up` |
| ◀ Previous | Previous record | `↑` |
| ▶ Next | Next record | `↓` |
| ⏩ Next Page | Page down | `Page Down` |
| ⏭ Last | Go to last record | `End` |

### 3. Manage Block

| Button | Action |
|--------|--------|
| ➕ Add | Create new row |
| 📋 Duplicate | Copy selected row |
| ✏️ Edit | Enter edit mode |
| 💾 Save | Commit changes |
| 🚫 Cancel | Revert changes |
| 🗑 Delete | Remove selected row |

**Edit Mode Behavior:**

- Save/Cancel only enabled when in edit mode AND changes exist
- Auto-save when navigating away from edited row (configurable)
- Visual indicator (I-cursor) in selector column during edit

### 4. Search Block

- 🔍 Magnifying glass icon
- Text input field
- ✕ Clear button
- Enter key triggers search

---

## Column Configuration

### JSON-Driven Columns

Columns are defined in `config/tabulator/<table-path>.json`:

```json
{
  "title": "Query Manager",
  "queryRef": 25,
  "readonly": false,
  "layout": "fitColumns",
  "columns": {
    "query_ref": {
      "field": "query_ref",
      "display": "Ref",
      "coltype": "integer",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "overrides": {
        "width": 80,
        "bottomCalc": "count"
      }
    },
    "name": {
      "field": "name",
      "display": "Name",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": true
    }
  }
}
```

### Column Properties

| Property | Type | Description |
|----------|------|-------------|
| `field` | string | JSON field name from API |
| `display` | string | Column header title |
| `coltype` | string | Reference to coltypes.json |
| `visible` | boolean | Show by default |
| `sort` | boolean | Allow sorting |
| `filter` | boolean | Show header filter |
| `editable` | boolean | Allow editing |
| `primaryKey` | boolean | Is primary key field |
| `lookupRef` | string | Lookup table reference (e.g., `"a27"`) |
| `overrides` | Object | Override any coltype property |

---

## Edit Mode

### Entering Edit Mode

- Click **Edit** button
- Double-click a row
- Press `Enter` on selected row

### Edit Mode Features

- **Inline editors** appear in cells
- **I-cursor indicator** shows in selector column
- **Save/Cancel** buttons become available
- **Dirty tracking** — changes detected automatically
- **Auto-save** on row navigation (optional)

### Exiting Edit Mode

- **Save** — Commit changes to API
- **Cancel** — Revert to original values
- **Navigate away** — Auto-save if enabled

---

## Templates

Templates save column configurations to localStorage:

### Saved Data

- Column visibility
- Column order
- Column widths
- Sort order
- Filter values
- Layout mode
- Panel width

### Template Menu

Access via **Template** button in Control block:

- **Saved templates** — List with checkmark for default
- **Save template...** — Save current configuration
- **Make default...** — Set default template
- **Delete** — Remove saved template

---

## Loading States

### Custom Loading Overlay

The component shows a loading spinner during data fetch:

```css
/* HTML structure */
<div class="table-container">
  <div class="table-loader">
    <div class="spinner-fancy spinner-fancy-md"></div>
  </div>
  <!-- Tabulator mounts here -->
</div>
```

**Styling:**

- Semi-transparent backdrop (`rgba(--bg-primary-rgb, 0.8)`)
- Backdrop blur effect
- Centered spinner
- Z-index above table content

---

## CSS Architecture

### Prefix System

The Navigator uses a **two-tier CSS class system**:

1. **Visual styling classes** always use the base `lithium-` prefix (from `lithium-table.css`)
2. **Element IDs** use the configured `cssPrefix` for unique selection within each table instance
3. **Sort icon classes** use `cssPrefix` (auto-injected for custom prefixes)

```css
/* Visual styling — always lithium- prefix (shared across all tables) */
.lithium-nav-wrapper { }
.lithium-nav-block { }
.lithium-nav-btn { }
.lithium-nav-popup { }
.lithium-nav-search-input { }
.lithium-selector-col { }

/* Sort icons — use cssPrefix, auto-injected by injectSortIconStyles() */
.lithium-sort-icons { }       /* default prefix */
.lithium-sort-asc { }
.lithium-sort-desc { }
.queries-sort-icons { }      /* custom prefix — auto-injected */
.queries-sort-asc { }
.queries-sort-desc { }

/* Element IDs — use cssPrefix for uniqueness */
#lookups-parent-nav-first { }
#lookups-parent-nav-next { }
```

### Key Classes

| Class | Element | Prefix Source |
|-------|---------|---------------|
| `.lithium-nav-wrapper` | Navigator container | Always base `lithium-` |
| `.lithium-nav-block` | Navigator button group | Always base `lithium-` |
| `.lithium-nav-btn` | Navigator buttons | Always base `lithium-` |
| `.lithium-nav-popup` | Popup menus | Always base `lithium-` |
| `.lithium-nav-popup-item` | Popup menu items | Always base `lithium-` |
| `.lithium-nav-search-input` | Search input field | Always base `lithium-` |
| `.lithium-selector-col` | Row selector column | Always base `lithium-` |
| `.lithium-col-chooser-btn` | Column chooser trigger | Always base `lithium-` |
| `.{prefix}-sort-icons` | Sort indicator container | Auto-injected for custom prefixes |
| `.{prefix}-sort-asc` | Sort ascending arrow | Auto-injected for custom prefixes |
| `.{prefix}-sort-desc` | Sort descending arrow | Auto-injected for custom prefixes |
| `.{prefix}-edit-mode` | Applied to container during edit | Always base `lithium-` |

### Why Base Classes for Navigator?

All Navigator instances share the same visual styles via `.lithium-*` classes in `lithium-table.css`. This enables:

- **Themeability** — CSS variables on `.lithium-nav-btn` cascade to all tables
- **No duplication** — One set of CSS rules covers all Navigator instances
- **Layer-based overrides** — Themes can override via `@layer custom-theme` (highest priority)

### Tri-State Sort

All LithiumTable instances support **tri-state sorting** (asc → desc → unsorted). This is enabled globally at the Tabulator constructor level:

```javascript
this.table = new Tabulator(this.container, {
  headerSortTristate: true,  // Always enabled
  headerSortElement: (column, dir) => {
    // dir is "asc", "desc", or "none"
    return `<span class="${prefix}-sort-icons" data-sort-dir="${dir}">
      <span class="${prefix}-sort-asc">&#9650;</span>
      <span class="${prefix}-sort-desc">&#9660;</span>
    </span>`;
  },
  // ...
});
```

CSS for tri-state sort is included in both `lithium-table.css` and the auto-injected styles for custom prefixes:

```css
/* Ascending — up arrow highlighted */
.lithium-table-container .tabulator-col[aria-sort="ascending"] .lithium-sort-asc {
  color: white;
  opacity: 1;
}

/* Descending — down arrow highlighted */
.lithium-table-container .tabulator-col[aria-sort="descending"] .lithium-sort-desc {
  color: white;
  opacity: 1;
}

/* None/unsorted — both arrows dimmed */
.lithium-table-container .tabulator-col[aria-sort="none"] .lithium-sort-asc,
.lithium-table-container .tabulator-col[aria-sort="none"] .lithium-sort-desc {
  color: var(--text-muted);
  opacity: 0.35;
}
```

### Sort Icons CSS (Automatic)

LithiumTable automatically injects sort icon styles for the specified `cssPrefix`. You **do not** need to add sort icon CSS to your manager's CSS file when using LithiumTable.

When you create a LithiumTable instance with a custom `cssPrefix` (e.g., `cssPrefix: 'mymanager'`), LithiumTable will automatically inject a `<style>` element with the required sort icon styles:

```javascript
// LithiumTable automatically handles sort icons for 'mymanager' prefix
const table = new LithiumTable({
  container: document.getElementById('my-table-container'),
  navigatorContainer: document.getElementById('my-navigator'),
  cssPrefix: 'mymanager',  // Sort icons automatically injected
  // ... other options
});
```

**What gets injected automatically (sort icons only):**

```css
.mymanager-sort-icons { /* sort icon container */ }
.mymanager-sort-asc { /* ascending arrow */ }
.mymanager-sort-desc { /* descending arrow */ }

/* Tri-state: unsorted columns */
.mymanager-sort-asc, .mymanager-sort-desc {
  color: var(--text-muted);
  opacity: 0.35;
}

/* Highlighted states */
[aria-sort="ascending"] .mymanager-sort-asc { color: white; opacity: 1; }
[aria-sort="descending"] .mymanager-sort-desc { color: white; opacity: 1; }
```

**Navigator styles are NOT injected** — they come from `lithium-table.css` using `lithium-*` base classes.

**Note:** If you use the default `cssPrefix` (`lithium`), the base `lithium-table.css` already provides these styles. The automatic injection only happens for custom prefixes.

**Exception:** Managers that use raw Tabulator (not LithiumTable), such as the Queries Manager and Login Manager, must still define their own sort icon styles in their CSS files.

### Container HTML Requirements

**IMPORTANT:** When adding a LithiumTable to a manager, the container elements in the HTML template **must include both** the manager-specific class AND the LithiumTable base classes:

```html
<!-- CORRECT — Both classes present -->
<div class="mymanager-table-container lithium-table-container" id="my-table-container">
  <!-- Tabulator table injected here -->
</div>
<div class="mymanager-navigator lithium-navigator-container" id="my-navigator">
  <!-- Navigator injected here -->
</div>

<!-- INCORRECT — Missing base classes -->
<div class="mymanager-table-container" id="my-table-container">
  <!-- Styles will NOT apply correctly -->
</div>
```

**Required base classes:**

| Container | Required Class |
|-----------|----------------|
| Table container | `lithium-table-container` |
| Navigator container | `lithium-navigator-container` |

**CSS Overrides for custom layouts:**

If the default lithium-table.css margins/padding don't fit your layout, override them in your manager CSS:

```css
/* Override default margins to fit custom layout */
.mymanager-table-container.lithium-table-container {
  margin: 0;
  border: none;
  border-radius: 0;
  flex: 1;
  min-height: 0;
}

.mymanager-navigator.lithium-navigator-container {
  padding: var(--space-1) var(--space-2);
}

/* Ensure table fills container */
.mymanager-table-container .tabulator {
  border: none;
  height: 100% !important;
}
```

---

## Methods Reference

### Data Loading

| Method | Description |
|--------|-------------|
| `loadData(searchTerm?, extraParams?)` | Load data from QueryRef |
| `loadStaticData(rows, options?)` | Load hardcoded/pre-fetched data |
| `refresh()` | Reload current data |
| `setData(rows)` | Set data directly (low-level) |

### Navigation

| Method | Description |
|--------|-------------|
| `navigateFirst()` | Go to first record |
| `navigateLast()` | Go to last record |
| `navigatePrevRec()` | Previous record |
| `navigateNextRec()` | Next record |
| `navigatePrevPage()` | Previous page |
| `navigateNextPage()` | Next page |

### CRUD Operations

| Method | Description |
|--------|-------------|
| `addRow()` | Add new row |
| `duplicateRow()` | Duplicate selected row |
| `enterEditMode(row?)` | Enter edit mode |
| `exitEditMode(reason)` | Exit edit mode |
| `saveRow()` | Save changes |
| `cancelEdit()` | Cancel changes |
| `deleteRow()` | Delete selected row |

### Column Management

| Method | Description |
|--------|-------------|
| `toggleColumnChooser(e, column)` | Show/hide column chooser |
| `toggleColumnFilters()` | Show/hide header filters |
| `setTableWidth(mode)` | Set width preset |
| `setTableLayout(mode)` | Set layout mode |
| `discoverColumns(rows)` | Auto-add hidden columns |

### Lifecycle

| Method | Description |
|--------|-------------|
| `init()` | Initialize component |
| `destroy()` | Clean up and destroy |
| `cleanup()` | Close popups, etc. |

---

## Events

### Table Events (Tabulator)

Standard Tabulator events are wired automatically:

- `rowClick` — Row selection
- `rowDblClick` — Enter edit mode
- `cellEdited` — Cell value changed
- `rowSelected` / `rowDeselected` — Selection changes

### Callbacks

Component callbacks (passed in options):

- `onRowSelected(rowData)` — Row was selected
- `onRowDeselected()` — Row was deselected
- `onDataLoaded(rows)` — Data finished loading
- `onEditModeChange(isEditing)` — Edit mode changed

---

## Integration Examples

### Simple Read-Only Table

```javascript
const table = new LithiumTable({
  container: document.getElementById('table'),
  navigatorContainer: document.getElementById('nav'),
  queryRef: 25,
  app: this.app,
  readonly: true,
  cssPrefix: 'readonly-table',
});

await table.init();
await table.loadData();
```

### Editable Table with Custom Callbacks

```javascript
const table = new LithiumTable({
  container: document.getElementById('table'),
  navigatorContainer: document.getElementById('nav'),
  tablePath: 'myapp/my-table',
  app: this.app,
  readonly: false,

  onRowSelected: (rowData) => {
    console.log('Selected:', rowData);
    this.loadDetails(rowData.id);
  },

  onDataLoaded: (rows) => {
    console.log(`Loaded ${rows.length} rows`);
  },
});

await table.init();
```

### Dual Table Setup (Like Lookups Manager)

```javascript
// Parent table
this.parentTable = new LithiumTable({
  container: this.elements.parentTableContainer,
  navigatorContainer: this.elements.parentNavigator,
  queryRef: 30,
  cssPrefix: 'lookups-parent',
  app: this.app,
  onRowSelected: (row) => this.loadChildData(row.LOOKUPID),
});

// Child table
this.childTable = new LithiumTable({
  container: this.elements.childTableContainer,
  navigatorContainer: this.elements.childNavigator,
  queryRef: 34,
  cssPrefix: 'lookups-child',
  app: this.app,
});

await Promise.all([
  this.parentTable.init(),
  this.childTable.init(),
]);

await this.parentTable.loadData();
```

---

## Collapsible Panels with Animated Icons

Managers using LithiumTable often need collapsible panels (left panel, middle panel) with collapse/expand buttons in the toolbar. The collapse buttons should have a rotation animation on their icons.

### Implementation Pattern

When adding collapsible panels to a manager, follow this pattern:

#### 1. HTML Structure

```html
<div class="mymanager-tabs-header">
  <div class="subpanel-header-group">
    <!-- Collapse button for left panel -->
    <button type="button" class="subpanel-header-btn subpanel-header-close collapse-btn" 
            id="mymanager-collapse-left-btn" title="Toggle Left Panel">
      <fa fa-angles-left id="mymanager-collapse-left-icon"></fa>
    </button>
    <!-- Other toolbar buttons... -->
  </div>
</div>
```

#### 2. CSS for Collapse Button Rotation

Add this to your manager's CSS file:

```css
/* Collapse button styling */
.collapse-btn {
  flex: 0 0 auto;
  padding: var(--space-1);
}

/* Collapse icon rotation - base state (pointing left) */
.collapse-btn > i,
.collapse-btn > svg {
  transition: transform var(--transition-delay) ease;
  transform: rotate(0deg);
}

/* Collapse icon rotation when panel is collapsed (pointing right) */
.collapse-btn.collapsed > i,
.collapse-btn.collapsed > svg {
  transform: rotate(180deg);
}
```

#### 3. JavaScript Toggle Method

In your manager's toggle method, toggle the `.collapsed` class on the collapse button:

```javascript
toggleLeftPanel() {
  this.isLeftPanelCollapsed = !this.isLeftPanelCollapsed;
  this.leftSplitter?.setCollapsed(this.isLeftPanelCollapsed);

  // Toggle rotation class on collapse button
  this.elements.collapseLeftBtn?.classList.toggle('collapsed', this.isLeftPanelCollapsed);

  if (!this.isLeftPanelCollapsed) {
    this.elements.leftPanel.style.width = `${this.leftPanelWidth}px`;
  }

  // Redraw tables after animation completes
  setTimeout(() => {
    this.leftTable?.table?.redraw?.();
    this.rightTable?.table?.redraw?.();
  }, 350);
}
```

### How It Works

1. **LithiumSplitter** adds `.lithium-panel-collapsed` class to the panel and `.lithium-splitter-collapsed` class to the splitter when collapsed
2. **Manager toggle method** adds `.collapsed` class to the collapse button itself
3. **CSS transition** smoothly rotates the icon from 0deg to 180deg using `var(--transition-delay)` timing
4. **Icon direction**: 0deg = pointing left (expand), 180deg = pointing right (collapse)

### Current Implementations

This pattern is used in:
- **Style Manager** (`style-manager.js`) — Left and middle panel collapse buttons
- **Lookups Manager** (`lookups.js`) — Left and middle panel collapse buttons  
- **Version Manager** (`version-history.js`) — Left panel collapse button

---

## Related Documentation

- [LITHIUM-MGR.md](LITHIUM-MGR.md) — Manager system overview
- [LITHIUM-MGR-QUERY.md](LITHIUM-MGR-QUERY.md) — Query Manager (first LithiumTable implementation)
- [LITHIUM-MGR-LOOKUPS.md](LITHIUM-MGR-LOOKUPS.md) — Lookups Manager (dual-table example)
- [LITHIUM-LUT.md](LITHIUM-LUT.md) — Lookup Tables integration
- [LITHIUM-CSS.md](LITHIUM-CSS.md) — CSS architecture, layers, and theming

---

## Implementation History

The LithiumTable component was extracted from the Query Manager implementation to provide a reusable table solution for all managers. It represents a significant architectural improvement over inline table code.

**Key Design Decisions:**

1. **Mixin Pattern** — Functionality split into logical modules (base, ops, UI)
2. **JSON-Driven** — Column configuration externalized to JSON files
3. **CSS Two-Tier System** — Navigator always uses `lithium-*` base classes for styling; element IDs use `cssPrefix` for uniqueness; sort icons use `cssPrefix` via auto-injection
4. **Tri-State Sort** — Always enabled; `headerSortElement` callback receives `(column, dir)` from Tabulator
5. **Template System** — User preferences persisted to localStorage
6. **Edit Mode Gate** — Editors only active in edit mode, ensuring consistent row selection
7. **`loadStaticData()` method** — Encapsulates blockRedraw/setData/discoverColumns pattern for hardcoded or pre-fetched data
