# LithiumTable Component

This document describes the `LithiumTable` reusable component — a standardized Tabulator data grid with integrated Navigator control bar.

**Related Documentation:**

- [LITHIUM-COL.md](LITHIUM-COL.md) — Column Manager documentation

---

## Overview

The `LithiumTable` component combines:

- **Tabulator** — Feature-rich data grid library
- **Navigator** — Custom control bar with Control, Move, Manage, Search blocks
- **JSON-driven configuration** — Column definitions from `config/tabulator/`
- **Edit mode** — Inline editing with dirty state tracking
- **Templates** — Save/load column configurations
- **Column Manager** — Runtime column customization (see [LITHIUM-COL.md](LITHIUM-COL.md))

The component is modular, reusable across all managers, and provides consistent table behavior throughout Lithium.

---

## Architecture

### File Structure (Refactored)

As of the latest refactoring, all table-related files are organized under `src/tables/`:

```structure
src/tables/
├── lithium-table-main.js       # Combined LithiumTable class export
├── lithium-table.js            # JSON-driven column resolution engine
├── lithium-table-base.js       # Core functionality (init, events, navigation)
├── lithium-table-ops.js        # CRUD operations mixin
├── lithium-table-ui.js         # Navigator UI mixin (delegated to modules)
├── lithium-table-template.js   # Template system
├── lithium-column-manager.js   # Column manager (now modular)
├── navigator/
│   └── navigator-builder.js    # Navigator HTML & button wiring
├── popups/
│   ├── popup-manager.js        # Standard nav popups (menu, width, layout)
│   └── template-popup.js       # Template menu popup
├── filters/
│   └── filter-editor.js        # Header filter editor creation
├── settings/
│   └── table-settings.js       # Width/layout mode management
├── visual/
│   ├── visual-updates.js       # Column boundary classes
│   └── loading-indicator.js    # Loading spinner
├── persistence/
│   └── persistence.js          # localStorage helpers
└── column-manager/             # Modular column manager components
    ├── cm-state.js             # Dirty tracking, persistence
    ├── cm-drag-resize.js       # Resize handle, keyboard events
    ├── cm-data.js              # Column data loading
    ├── cm-columns.js           # Column definition builder
    ├── cm-actions.js           # Apply/discard changes
    ├── cm-ui.js                # Popup DOM, positioning
    └── cm-table.js             # Inner LithiumTable setup

src/core/                       # Core utilities (unchanged)
├── lithium-splitter.js         # Panel splitter component
├── manager-edit-helper.js      # Consolidated edit/dirty helper
├── panel-collapse.js           # Panel collapse utility
├── panel-state-manager.js      # Panel state persistence
└── ...

src/styles/
├── lithium-table.css           # Component styles
└── ...
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
- **LithiumTableUIMixin** — Navigator UI, popups, column chooser, filter editors (delegates to modules)

### Modular UI Architecture

The `LithiumTableUIMixin` now delegates to focused modules rather than containing all logic inline:

| Module | Responsibility | Key Exports |
|--------|----------------|-------------|
| `navigator-builder.js` | Build navigator HTML, wire buttons | `buildNavigator()`, `updateMoveButtonState()` |
| `popup-manager.js` | Standard nav popups | `toggleNavPopup()`, `closeNavPopup()` |
| `template-popup.js` | Template menu with save/load/delete | `buildTemplatePopup()`, `getSavedTemplates()` |
| `table-settings.js` | Width/layout mode persistence | `setTableWidth()`, `setTableLayout()` |
| `filter-editor.js` | Header filter editor factory | `createFilterEditorFunction()` |
| `visual-updates.js` | Column boundary CSS classes | `updateVisibleColumnClasses()` |
| `loading-indicator.js` | Loading spinner show/hide | `showLoading()`, `hideLoading()` |
| `persistence.js` | Row selection, filters state | `saveSelectedRowId()`, `restoreFiltersVisible()` |

---

## Usage

### Basic Usage

```javascript
import { LithiumTable } from '../../tables/lithium-table-main.js';

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
| `storageKey` | string | ❌ | localStorage base key for templates, width/layout state, and row persistence |
| `readonly` | boolean | ❌ | Disable editing (default: `false`) |
| `tableWidthMode` | string | ❌ | Default width preset (default: `'compact'`) |
| `alwaysEditable` | boolean | ❌ | Make cells edit directly on click and disable standard Lithium edit-mode workflow |
| `useColumnManager` | boolean | ❌ | Enable the shared runtime Column Manager popup (default: `true`) |
| `searchQueryRef` | number | ⚪ | QueryRef for search |
| `detailQueryRef` | number | ⚪ | QueryRef for detail loading |
| `updateQueryRef` | number | ⚪ | QueryRef for updates |
| `insertQueryRef` | number | ⚪ | QueryRef for inserts |
| `deleteQueryRef` | number | ⚪ | QueryRef for deletes |
| `onRowSelected` | Function | ❌ | Called when row selected |
| `onRowDeselected` | Function | ❌ | Called when row deselected |
| `onDataLoaded` | Function | ❌ | Called when data loaded |
| `onEditModeChange` | Function | ❌ | Called when edit mode changes |
| `onDirtyChange` | Function | ❌ | Called when dirty state changes — `(isDirty, rowData) => {}` |
| `onExecuteSave` | Function | ❌ | Custom save logic — `async (row, editHelper) => {}` |
| `onDuplicate` | Function | ❌ | Custom duplicate logic — `async (rowData) => newRowData` |
| `onRefresh` | Function | ❌ | Custom refresh (e.g. re-query with params) |

---

## The Navigator Block

The Navigator provides standard table controls in four groups:

### 1. Control Block

| Button | Action |
|--------|--------|
| 🔄 Refresh | Reload data from source |
| 🔽 Filter | Toggle column header filters |
| ☰ Menu | Table options popup (expand/collapse all, toggle row height) |
| ↔ Width | Table width presets (Narrow/Compact/Normal/Wide/Auto) |
| ⊞ Layout | Layout mode (fitColumns/fitData/fitDataFill/etc) |
| 🛠 Template | Save/load column configurations |

**Width Presets:** The width button uses CSS custom properties defined in `base.css`:

- `--table-width-narrow: 160px`
- `--table-width-compact: 314px`
- `--table-width-normal: 468px`
- `--table-width-wide: 622px`

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
| 📋 Duplicate | Copy selected row (disabled when no row selected) |
| ✏️ Edit | Enter edit mode |
| 🚩 Flag | Optional manager callback |
| 📝 Annotate | Optional manager callback |
| 🗑 Delete | Remove selected row |

The Manage block is shown only for standard editable LithiumTables. Tables created with `alwaysEditable: true` hide the block entirely and do not expose the standard Add/Duplicate/Edit/Delete workflow.

**Duplicate Button Behavior:**

- Disabled when no row is selected
- Default behavior: copies row data, removes primary key, appends " (Copy)" to name
- Custom behavior: provide `onDuplicate` callback for manager-specific clone logic

**Implementing Custom Duplicate Logic:**

For tables that need server-side clone operations (e.g., calling an insert QueryRef):

```javascript
this.myTable = new LithiumTable({
  // ... other options ...
  onDuplicate: async (rowData) => this._executeCustomDuplicate(rowData),
});

async _executeCustomDuplicate(rowData) {
  // 1. Fetch any additional data not in the table (like detail data)
  const fullData = await this._fetchDetailData(rowData.id);

  // 2. Call server to create the clone (e.g., QueryRef 42)
  const result = await authQuery(this.app.api, 42, {
    INTEGER: { /* params */ },
    STRING: { /* params including summary, code, collection */ },
  });

  // 3. Save the new ID for selection after refresh
  const newId = result?.[0]?.key_idx;
  this.myTable.saveSelectedRowId(newId);

  // 4. Refresh the table to show the new row
  await this.loadTableData();

  // 5. Return null to abort default duplicate behavior
  return null;
}
```

**Key points for custom duplicate:**

- Return `null` to abort default behavior (you handled the insert)
- Save the new row ID so it gets selected after refresh
- Refresh the table data to include the newly created row
- Fetch any detail data needed that's not in the table row (summary, code, collection, etc.)

**Managers with custom duplicate implemented:**

- **Lookups Manager (child table)**: Uses QueryRef 42, fetches detail via QueryRef 35
- **Queries Manager**: Uses QueryRef 29, fetches detail via QueryRef 27
- **Style Manager**: Uses default duplicate (no custom callback needed for lookup table)

To add custom duplicate to a new manager:

1. Add `onDuplicate: (rowData) => this._executeDuplicate(rowData)` to LithiumTable options
2. Implement `_executeDuplicate(rowData)` method following the pattern above
3. Ensure your insert QueryRef returns the new primary key

**Edit Mode Behavior:**

- Save/Cancel footer buttons only enabled when in edit mode AND changes exist
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
  "updateQueryRef": 28,
  "insertQueryRef": 29,
  "deleteQueryRef": 31,
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

At runtime, LithiumTable forces `responsiveLayout: false` for all tables. When a panel is too narrow for the visible columns, the table scrolls horizontally instead of letting Tabulator collapse or hide columns.

### QueryRef Resolution (Constructor + Table Definition)

QueryRefs can be specified in two places: the LithiumTable constructor and the JSON table definition file. They are merged during `loadConfiguration()`:

1. **Constructor options** (highest priority) — non-null values explicitly passed to `new LithiumTable({ ... })`
2. **Table definition** (base) — loaded from the database lookup cache (Lookup 059 via `loadTableDef()`)

Only non-null constructor values override the table definition. If a manager doesn't pass a particular queryRef, the value from the database schema is used.

#### Why CRUD QueryRefs Must Be Passed Explicitly

The table definition is loaded from a database lookup (Lookup 059 "Tabulator Schemas") via QueryRef 060. That runtime schema may not contain the CRUD queryRefs (`updateQueryRef`, `insertQueryRef`, `deleteQueryRef`), and the `config/tabulator/*.json` files on the filesystem are **not** served by either the Vite dev server or the production web server.

**You must always pass CRUD queryRefs explicitly in the LithiumTable constructor.** This is the same pattern used by the Query Manager, Lookups Manager, and Style Manager.

```javascript
this.table = new LithiumTable({
  tablePath: 'mymanager/my-table',
  queryRef: 50,          // Read — list data
  searchQueryRef: 51,    // Read — search
  updateQueryRef: 52,    // Update — save existing row
  insertQueryRef: 53,    // Create — save new row
  deleteQueryRef: 54,    // Delete — remove row
  // ...
});
```

The `config/tabulator/*.json` files serve as the **design-time documentation** for which QueryRefs a table uses. The constructor is the **runtime source of truth**.

Additionally, if your manager provides an `onExecuteSave` callback, include hardcoded fallbacks as a safety net (same pattern as Query Manager):

```javascript
const queryRef = isInsert
  ? (this.myTable.queryRefs?.insertQueryRef ?? 53)   // fallback
  : (this.myTable.queryRefs?.updateQueryRef ?? 52);   // fallback
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

These entry points apply to standard LithiumTables. Tables configured with `alwaysEditable: true` do not enter the shared Lithium edit-mode state.

### Edit Mode Features

- **Inline editors** appear in cells
- **I-cursor indicator** shows in selector column
- **Save/Cancel footer buttons** become available
- **Dirty tracking** — changes detected automatically
- **Auto-save** when navigating away from a dirty row (always enabled)

### Exiting Edit Mode

- **Save** — Commit changes to API
- **Cancel** — Revert to original values
- **Click another row** — Auto-saves if dirty, then switches to new row
- **Click Edit button** — Auto-saves if dirty, otherwise exits edit mode
- **Navigate (First/Last/Prev/Next/Page)** — Auto-saves if dirty, then navigates

All exit paths except **Cancel** will auto-save when dirty. Cancel reverts all changes (table row data via `originalRowData` + external editors via `editHelper.restoreEditorSnapshots()`).

### Auto-Save on Row Change

When the user clicks a different row while in edit mode, the system:

1. **Checks dirty state synchronously** via `isActuallyDirty()` — this bypasses the rAF-deferred `isDirty` flag to handle the case where the user types in a CodeMirror editor and immediately clicks another row
2. **If dirty:** calls `autoSaveBeforeRowChange()` which saves the **editing** row (not the newly-clicked row), shows a "Changes Saved" toast on success, then exits edit mode
3. **If not dirty:** exits edit mode silently
4. **Proceeds to select** the new row and fire `onRowSelected()`

If the save fails, the system reverts selection back to the editing row and remains in edit mode.

**Implementation:** `autoSaveBeforeRowChange()` uses `this.getEditingRow()` to find the correct row to save. By the time this method runs, Tabulator has already selected the new row, so `getSelectedRows()` would return the wrong row.

### Always-Editable Tables

`alwaysEditable: true` is a special LithiumTable mode used by the Column Manager popup.

- Cells open editors directly on single click
- The standard Manage block is hidden from the Navigator
- `Enter`, `F2`, double-click, and toolbar Edit do not enter Lithium edit mode
- `ManagerEditHelper` footer Save/Cancel flow is not used
- The owner component is responsible for batching changes and providing its own Save/Cancel controls

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

- **Saved templates** — List with checkmark for the currently active template
- **Save Template** — Save current configuration to localStorage
- **Clear Template** — Reload the runtime default schema from Lookup 059
- **Copy to Clipboard** — Export configuration as JSON (see [LITHIUM-COL.md](LITHIUM-COL.md))
- **Delete Template** — Remove the selected saved template

A saved local template named `Default` auto-loads on startup. There is no separate "Make default" action in the current menu.

### Template Format

Templates are stored in Lookup 059 compatible format. See [LITHIUM-COL.md](LITHIUM-COL.md) for the complete JSON schema.

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

**Exception:** Managers that use raw Tabulator (not LithiumTable), such as the Login Manager, must still define their own sort icon styles in their CSS files.

**Collapse Button CSS:** The collapse button icon rotation CSS is provided **shared** by `lithium-table.css` via the `.lithium-collapse-btn` and `.lithium-collapse-icon` classes. Add these classes to your HTML — no manager-specific CSS needed. See [Collapsible Panels with Animated Icons](#collapsible-panels-with-animated-icons) below.

### Splitter CSS Classes

| Class | Element | Description |
|-------|---------|-------------|
| `.lithium-splitter` | Splitter element | Draggable panel divider |
| `.lithium-splitter-resizing` | Applied during drag | Visual feedback during resize |
| `.lithium-splitter-collapsed` | When splitter hidden | When left panel is collapsed |

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
| `enterEditMode(row?)` | Enter edit mode for standard tables |
| `exitEditMode(reason)` | Exit edit mode for standard tables |
| `saveRow()` | Save changes |
| `cancelEdit()` | Cancel changes |
| `deleteRow()` | Delete selected row |

### Column Management

| Method | Description |
|--------|-------------|
| `toggleColumnChooser(e, column)` | Show/hide column chooser |
| `toggleColumnFilters()` | Show/hide header filters |
| `setTableWidth(mode)` | Set width preset (uses CSS vars) |
| `setTableLayout(mode)` | Set layout mode |
| `discoverColumns(rows)` | Auto-add hidden columns |
| `applyTemplateColumns(templateColumns)` | Apply column configuration from template/Column Manager |
| `injectSortIconStyles()` | Inject sort icon CSS for custom cssPrefix |

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
- `rowDblClick` — Enter edit mode for standard tables
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

## Related Documentation

- [LITHIUM-MGR.md](LITHIUM-MGR.md) — Manager system overview
- [LITHIUM-MGR-QUERY.md](LITHIUM-MGR-QUERY.md) — Query Manager (first LithiumTable implementation)
- [LITHIUM-MGR-LOOKUPS.md](LITHIUM-MGR-LOOKUPS.md) — Lookups Manager (dual-table example)
- [LITHIUM-COL.md](LITHIUM-COL.md) — **Column Manager** — Runtime column customization UI
- [LITHIUM-LUT.md](LITHIUM-LUT.md) — Lookup Tables integration
- [LITHIUM-CSS.md](LITHIUM-CSS.md) — CSS architecture, layers, and theming
- [LITHIUM-ICN.md](LITHIUM-ICN.md) — Icon system, three-stage pipeline, rotation pattern

---

## Implementation History

The LithiumTable component was extracted from the Query Manager implementation to provide a reusable table solution for all managers. It represents a significant architectural improvement over inline table code.

**2025 Refactoring:** The table system was reorganized into a modular architecture:

- Files moved from `src/core/` to `src/tables/`
- `lithium-table-ui.js` split into focused modules (`navigator/`, `popups/`, `settings/`, etc.)
- `lithium-column-manager.js` split into `column-manager/` submodules
- All modules now under 750 lines for maintainability

**Key Design Decisions:**

1. **Mixin Pattern** — Functionality split into logical modules (base, ops, UI)
2. **JSON-Driven** — Column configuration externalized to JSON files
3. **CSS Two-Tier System** — Navigator always uses `lithium-*` base classes for styling; element IDs use `cssPrefix` for uniqueness; sort icons use `cssPrefix` via auto-injection
4. **Tri-State Sort** — Always enabled; `headerSortElement` callback receives `(column, dir)` from Tabulator
5. **Template System** — User preferences persisted to localStorage
6. **Edit Mode Gate** — Editors only active in edit mode, except for explicit `alwaysEditable` tables such as the Column Manager popup
7. **`loadStaticData()` method** — Encapsulates blockRedraw/setData/discoverColumns pattern for hardcoded or pre-fetched data
8. **CSS Variable Width Presets** — Width modes use CSS custom properties (`--table-width-narrow`, etc.) for consistent theming
