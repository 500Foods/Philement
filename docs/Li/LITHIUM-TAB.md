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

**Collapse Button CSS:** The collapse button icon rotation CSS is provided **shared** by `lithium-table.css` via the `.lithium-collapse-btn` and `.lithium-collapse-icon` classes. Add these classes to your HTML — no manager-specific CSS needed. See [Collapsible Panels with Animated Icons](#collapsible-panels-with-animated-icons) below.

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

Managers using LithiumTable need collapsible panels (left panel, middle panel) with collapse/expand buttons in the toolbar. The shared `panel-collapse.js` utility and CSS in `lithium-table.css` handle all animation logic consistently.

### Implementation Pattern

When adding collapsible panels to a manager, follow this pattern:

#### 1. HTML Structure

Add the `lithium-collapse-btn` class, `lithium-collapse-icon` to the `<fa>` icon, and `data-collapse-target` to identify which panel this button controls:

```html
<!-- Single panel (left) -->
<button type="button" class="subpanel-header-btn subpanel-header-close my-collapse-btn lithium-collapse-btn"
        id="mymanager-collapse-left-btn" data-collapse-target="left" title="Toggle Left Panel">
  <fa fa-angles-left id="mymanager-collapse-left-icon" class="lithium-collapse-icon"></fa>
</button>

<!-- Dual panels (left + middle) — each button needs its own target -->
<button type="button" class="subpanel-header-btn subpanel-header-close my-collapse-left-btn lithium-collapse-btn"
        id="mymanager-collapse-left-btn" data-collapse-target="left" title="Toggle Left Panel">
  <fa fa-angles-left id="mymanager-collapse-left-icon" class="lithium-collapse-icon"></fa>
</button>
<button type="button" class="subpanel-header-btn subpanel-header-close my-collapse-middle-btn lithium-collapse-btn"
        id="mymanager-collapse-middle-btn" data-collapse-target="middle" title="Toggle Middle Panel">
  <fa fa-angles-left id="mymanager-collapse-middle-icon" class="lithium-collapse-icon"></fa>
</button>
```

> **Why `data-collapse-target`?** In managers with multiple panels (Style, Lookups), the CSS uses this attribute to rotate only the correct icon when a specific panel collapses. Without it, all icons would rotate together.

#### 2. CSS — None Needed

The shared classes in `lithium-table.css` handle everything using `--transition-duration`:

```css
/* Provided by lithium-table.css — managers do NOT need to duplicate this */
.lithium-collapse-btn {
  flex: 0 0 auto;
  padding: var(--space-1);
}

.lithium-collapse-icon,
.lithium-collapse-btn > i,
.lithium-collapse-btn > svg {
  transition: transform var(--transition-duration, 350ms) ease;
  transform: rotate(0deg);
}

.lithium-collapse-btn.collapsed .lithium-collapse-icon,
.lithium-collapse-btn.collapsed > i,
.lithium-collapse-btn.collapsed > svg {
  transform: rotate(180deg);
}
```

Panel width transitions are handled by `manager-panels.css`:

```css
/* Panel width animation uses --transition-duration */
[data-panel="left"] {
  transition: width var(--transition-duration, 350ms) ease,
              min-width var(--transition-duration, 350ms) ease;
}
```

> **Why cover all stages?** `processIcons()` converts `<fa>` → `<i>` and then Font Awesome's SVG+JS replaces `<i>` → `<svg>`. CSS targets `> i`, and `> svg` to work at any pipeline stage. See [LITHIUM-ICN.md](LITHIUM-ICN.md#icon-rotation-pattern).

#### 3. JavaScript Toggle Method (Using Shared Utility)

Use the `togglePanelCollapse()` function from `panel-collapse.js`:

```javascript
import { togglePanelCollapse, restorePanelState } from '../../core/panel-collapse.js';

toggleLeftPanel() {
  this.isLeftPanelCollapsed = togglePanelCollapse({
    panel: this.elements.leftPanel,
    splitter: this.leftSplitter,
    collapseBtn: this.elements.collapseLeftBtn,
    panelWidth: this.leftPanelWidth,
    isCollapsed: this.isLeftPanelCollapsed,
    onAfterToggle: () => {
      this.leftTable?.table?.redraw?.();
      this.rightTable?.table?.redraw?.();
    },
  });

  // Save collapsed state to persistence
  this.leftPanelState.saveCollapsed(this.isLeftPanelCollapsed);
}
```

#### 4. Restore State (Using Shared Utility)

```javascript
import { togglePanelCollapse, restorePanelState } from '../../core/panel-collapse.js';

restorePanelState() {
  // Re-read collapsed state from localStorage
  this.isLeftPanelCollapsed = this.leftPanelState.loadCollapsed(this.isLeftPanelCollapsed);

  // Restore collapsed state using shared utility
  restorePanelState({
    panel: this.elements.leftPanel,
    splitter: this.leftSplitter,
    collapseBtn: this.elements.collapseLeftBtn,
    isCollapsed: this.isLeftPanelCollapsed,
  });

  // Panel width is handled by LithiumTable's setupPersistence():
  // it calls onSetTableWidth(mode) or onSetTableWidth(null)
  // Your setTableWidth handler applies the right width.
}
```

> **Init order matters:** Tables must be created **before** splitters, and splitters **before** `restorePanelState()`. The splitter's `tables` option (for width-mode clearing) requires the LithiumTable instance to exist at construction time.
>
> ```javascript
> async init() {
>   await this.render();
>   this.setupEventListeners();
>   await this.initTable();     // 1. Create LithiumTable first
>   this.setupSplitters();      // 2. Create splitter (passes table via `tables` option)
>   this.restorePanelState();   // 3. Restore collapsed state last
> }
> ```

### Shared Utility API

The `panel-collapse.js` module exports two functions:

| Function | Description |
|----------|-------------|
| `togglePanelCollapse(options)` | Toggles panel collapse/expand with smooth animation |
| `restorePanelState(options)` | Restores persisted collapsed state on init |

**`togglePanelCollapse()` Options:**

| Option | Type | Description |
|--------|------|-------------|
| `panel` | HTMLElement | The panel element to collapse/expand |
| `splitter` | LithiumSplitter | LithiumSplitter instance for the panel |
| `collapseBtn` | HTMLElement | The collapse button element |
| `panelWidth` | number | Target width when expanding (pixels) |
| `isCollapsed` | boolean | Current collapsed state (will be toggled) |
| `onAfterToggle` | Function | Optional callback after animation completes |

**Returns:** `boolean` — The new collapsed state

**`restorePanelState()` Options:**

| Option | Type | Description |
|--------|------|-------------|
| `panel` | HTMLElement | The panel element |
| `splitter` | LithiumSplitter | LithiumSplitter instance |
| `collapseBtn` | HTMLElement | The collapse button element |
| `isCollapsed` | boolean | The persisted collapsed state |

### Transition Timing

All panel collapse/expand animations use the `--transition-duration` CSS variable defined in `base.css`:

```css
:root {
  --transition-duration: 350ms; /* Default, configurable per theme */
}
```

This ensures consistent animation timing across:
- Panel width transitions (expand/collapse)
- Collapse button icon rotation

### Width Persistence Across Collapse/Expand

When a user sets a table width via the Width popup (Narrow/Compact/Normal/Wide/Auto) and then collapses the panel, the width should be preserved. This is handled by two systems working together:

#### 1. LithiumTable Width Mode (saved in `storageKey_width_mode`)

LithiumTable's `setupPersistence()` restores the saved width mode on init:
- If a mode was saved (e.g., "wide"), it calls `onSetTableWidth("wide")` via `requestAnimationFrame`
- If **no** mode was saved, it calls `onSetTableWidth(null)` — signaling the manager to apply its fallback

#### 2. Manager's `setTableWidth` handler

The manager handler must handle both cases:

```javascript
setTableWidth(mode) {
  const panel = this.elements.leftPanel;
  if (!panel) return;

  // mode === null: LithiumTable has no saved mode, use PanelStateManager fallback
  if (mode === null) {
    panel.style.width = `${this.leftPanelWidth}px`;
    return;
  }

  // mode === 'auto': use CSS default
  if (mode === 'auto') {
    panel.style.width = '';
    this.leftPanelWidth = panel.offsetWidth;
    this.leftPanelState.saveWidth(this.leftPanelWidth);
    return;
  }

  // Named mode: apply the preset width
  const widths = { narrow: 160, compact: 314, normal: 468, wide: 622 };
  const widthPx = widths[mode];
  panel.style.width = `${widthPx}px`;
  this.leftPanelWidth = widthPx;
  this.leftPanelState.saveWidth(widthPx);
}
```

#### 3. Splitter Drag Clears Width Mode

When the user drags the splitter, the width no longer matches any preset. `LithiumSplitter` handles this automatically — pass the LithiumTable instance(s) via the `tables` option:

```javascript
this.splitter = new LithiumSplitter({
  element: this.elements.splitter,
  leftPanel: this.elements.leftPanel,
  tables: this.queryTable,  // or [tableA, tableB] for multiple
  onResize: (width) => { this.leftPanelWidth = width; },
  onResizeEnd: (width) => { this.leftPanelState.saveWidth(width); },
});
```

The splitter sets `table.tableWidthMode = null` during drag, so the Width popup shows no checkmark.

#### 4. `PanelStateManager` for Pixel Width

The `PanelStateManager` persists the actual pixel width (from either the Width popup or splitter drag). On reload:
- Constructor loads `this.leftPanelWidth = this.leftPanelState.loadWidth(defaultWidth)`
- LithiumTable calls `onSetTableWidth(null)` if no mode was saved → manager applies `this.leftPanelWidth`

#### Persistence Flow

1. User sets width via **Width popup** → `setTableWidth("wide")` updates `leftPanelWidth` + saves to `PanelStateManager` + LithiumTable saves "wide" to `storageKey_width_mode`
2. User drags **splitter** → `leftPanelWidth` updated + saved to `PanelStateManager` + LithiumTable `tableWidthMode` cleared to `null`
3. User **collapses** → `toggleLeftPanel()` saves collapsed state to `PanelStateManager`
4. Page **reloads**:
   - If LithiumTable mode saved → `onSetTableWidth("wide")` restores preset width
   - If no LithiumTable mode → `onSetTableWidth(null)` → manager applies `PanelStateManager` pixel width
   - Collapsed state restored via `loadCollapsed()`
5. User **expands** → `toggleLeftPanel()` restores from `this.leftPanelWidth`

### Current Implementations

All managers use the shared `panel-collapse.js` utility for consistent behavior:

| Manager | File | Panels | Collapse Buttons |
|---------|------|--------|------------------|
| **Version Manager** | `version-history.js` | Left | `data-collapse-target="left"` |
| **Query Manager** | `queries.js` | Left | `data-collapse-target="left"` |
| **Style Manager** | `style-manager.js` | Left + Middle | `data-collapse-target="left"`, `data-collapse-target="middle"` |
| **Lookups Manager** | `lookups.js` | Left + Middle | `data-collapse-target="left"`, `data-collapse-target="middle"` |

---

## Footer Save/Cancel Buttons and ManagerEditHelper

All managers with LithiumTables include Save/Cancel buttons in the manager footer. These buttons start **disabled** and only become enabled when a record is "dirty" (has changes compared to the snapshot taken at edit mode entry). The transition from disabled → enabled uses a smooth 350ms CSS animation: Save turns green, Cancel turns red. If changes are undone (returning to the original state), the buttons revert to disabled.

### ManagerEditHelper (`manager-edit-helper.js`)

The `ManagerEditHelper` class consolidates all edit mode, dirty tracking, and save/cancel button logic that was previously duplicated across every manager. It handles:

- **`activeEditingTable` tracking** — Only one LithiumTable in edit mode at a time
- **`onEditModeChange` / `onDirtyChange` callbacks** — Auto-wired when tables are registered
- **Snapshot-based dirty comparison** — Table row data + registered external editors
- **Footer save/cancel button state** — Disabled → green/red on dirty, with transitions
- **External editor enable/disable** — Called automatically on edit mode transitions

### Button Layout

The footer layout follows this order (left to right):

```
[Print] [Email] [Export] [Select] [placeholder flex:1] [Save] [Cancel] [Dummy] [Crimson...] [fixed action icons]
```

### CSS Classes (Shared)

All Save/Cancel button styles are defined in `manager-ui.css` and are shared across all managers:

| Class | Purpose | Disabled State | Enabled State |
|-------|---------|----------------|---------------|
| `.manager-footer-save-btn` | Save button | `--accent-disabled` bg, `--text-disabled` color, `cursor: not-allowed` | `--accent-success` green bg, white color, `cursor: pointer` |
| `.manager-footer-cancel-btn` | Cancel button | `--accent-disabled` bg, `--text-disabled` color, `cursor: not-allowed` | `--accent-danger` red bg, white color, `cursor: pointer` |
| `.manager-footer-dummy-btn` | Spacer button (60px wide) | `--accent-primary` bg, `cursor: default` | — |

Both buttons include `transition: background 350ms ease, color 350ms ease` for smooth disabled↔enabled animation.

**Do not duplicate these styles** in manager-specific CSS files — they are inherited from `manager-ui.css`.

### JavaScript Implementation

#### 1. Import ManagerEditHelper and Footer Setup

```javascript
import { setupManagerFooterIcons } from '../../core/manager-ui.js';
import { ManagerEditHelper } from '../../core/manager-edit-helper.js';
```

#### 2. Create EditHelper in Constructor

```javascript
constructor(app, container) {
  // ... other properties ...
  this.editHelper = new ManagerEditHelper({ name: 'MyManager' });
}
```

#### 3. Register Tables After Creation

```javascript
async initMyTable() {
  this.myTable = new LithiumTable({
    container: this.elements.tableContainer,
    navigatorContainer: this.elements.navigatorContainer,
    // ... other options ...
    // NOTE: Do NOT pass onEditModeChange/onDirtyChange — editHelper wires these automatically
  });

  // Register with editHelper — auto-wires onEditModeChange + onDirtyChange
  this.editHelper.registerTable(this.myTable);

  await this.myTable.init();
}
```

#### 4. Register External Editors (Optional)

For managers with CodeMirror, SunEditor, or other external editors bound to a table:

```javascript
// After table creation, register editors
this.editHelper.registerEditor('css', {
  getContent:  () => this.cssEditor?.state?.doc?.toString() || '',
  setEditable: (editable) => this.setCssEditorEditable(editable),
  boundTable:  this.lookupTable,  // Which table this editor relates to
});
```

The `setEditable` callback is called automatically when the bound table enters/exits edit mode. The `getContent` callback is used for snapshot comparison.

#### 5. Wire Footer Buttons in `setupFooter()`

```javascript
setupFooter() {
  const slot = this.container.closest('.manager-slot');
  if (!slot) return;

  const footer = slot.querySelector('.manager-slot-footer');
  if (!footer) return;

  const group = footer.querySelector('.subpanel-header-group');
  if (!group) return;

  const placeholder = group.querySelector('.slot-footer-placeholder');

  const footerElements = setupManagerFooterIcons(group, {
    onPrint: () => this.handleFooterPrint(),
    onEmail: () => this.handleFooterEmail(),
    onExport: (e) => this.toggleFooterExportPopup(e),
    reportOptions: [
      { value: 'mymanager-view', label: 'My Table View' },
      { value: 'mymanager-data', label: 'My Table Data' },
    ],
    fillerTitle: 'MyManager',
    anchor: placeholder,
    showSaveCancel: true,
  });

  this._footerDatasource = footerElements.reportSelect;

  // Wire save/cancel buttons to the editHelper (handles all state management)
  this.editHelper.wireFooterButtons(
    footerElements.saveBtn,
    footerElements.cancelBtn,
    footerElements.dummyBtn,
  );
}
```

#### 6. Trigger Dirty Check from Editor Change Listeners

When external editors fire change events, call `checkDirtyState()`:

```javascript
// CodeMirror updateListener
EditorView.updateListener.of((update) => {
  if (update.docChanged && this.editHelper.isEditing()) {
    this.editHelper.checkDirtyState();
  }
}),

// SunEditor onChange
this.sunEditor.onChange = () => {
  if (this.childTable?.isEditing) {
    this.editHelper.checkDirtyState();
  }
};
```

#### 7. Clean Up in `cleanup()`

```javascript
cleanup() {
  this.editHelper?.destroy();
  // ... other cleanup ...
}
```

### How Dirty Tracking Works

1. **Edit mode entered** → `editHelper._takeSnapshot()` captures table row data + all registered editors' content
2. **Editor changes** → `editHelper.checkDirtyState()` compares current state to snapshot
3. **Anything differs** → Save/Cancel buttons become enabled (green/red)
4. **Changes undone** → Comparison shows no difference → buttons return to disabled
5. **Save clicked** → `activeEditingTable.handleSave()` called
6. **Cancel clicked** → `activeEditingTable.handleCancel()` called
7. **Edit mode exited** → Snapshot cleared, buttons disabled

### Dual/Multi Table Behavior

When a manager has two or more LithiumTables, only one can be in edit mode at a time. The `editHelper` handles this automatically: entering edit mode on one table exits edit mode on any other.

### Current Implementations

| Manager | File | Tables | External Editors |
|---------|------|--------|------------------|
| **Version Manager** | `version-history.js` | 1 (readonly) | None |
| **Query Manager** | `queries.js` | 1 | SQL, Summary, Collection (CodeMirror) |
| **Style Manager** | `style-manager.js` | 2 | CSS (CodeMirror) |
| **Lookups Manager** | `lookups.js` | 2 | JSON (CodeMirror tree), Summary (SunEditor) |
| **Column Manager** | `lithium-column-manager.js` | 1 (self-contained) | None |

---

## Related Documentation

- [LITHIUM-MGR.md](LITHIUM-MGR.md) — Manager system overview
- [LITHIUM-MGR-QUERY.md](LITHIUM-MGR-QUERY.md) — Query Manager (first LithiumTable implementation)
- [LITHIUM-MGR-LOOKUPS.md](LITHIUM-MGR-LOOKUPS.md) — Lookups Manager (dual-table example)
- [LITHIUM-LUT.md](LITHIUM-LUT.md) — Lookup Tables integration
- [LITHIUM-CSS.md](LITHIUM-CSS.md) — CSS architecture, layers, and theming
- [LITHIUM-ICN.md](LITHIUM-ICN.md) — Icon system, three-stage pipeline, rotation pattern

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
