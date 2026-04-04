# Adding a New Manager to Lithium

> **Read this document before creating any new manager.** It is the definitive step-by-step guide for adding a LithiumTable-based manager to the application. The Query Manager and Lookups Manager are the canonical reference implementations.

---

## Overview

A "manager" in Lithium is a self-contained feature module that occupies a workspace slot. Most managers follow one of two layout patterns:

| Pattern | Panels | Example Managers |
|---------|--------|------------------|
| **Single-table** | Left table + Right detail | Query Manager |
| **Dual-table (parent/child)** | Left table + Middle table + Right detail | Lookups Manager, Style Manager |

Both patterns use the same core components:

- **`LithiumTable`** — Reusable Tabulator data grid with Navigator
- **`ManagerEditHelper`** — Consolidates edit mode, dirty tracking, save/cancel buttons
- **`LithiumSplitter`** — Resizable panel dividers
- **`PanelStateManager`** — Persists panel width + collapsed state to localStorage

This guide walks through creating a manager from scratch.

---

## Checklist

Use this checklist to track progress. Every item is covered in detail below.

- [ ] 1. Reserve a Manager ID
- [ ] 2. Create the file structure (`js`, `html`, `css`)
- [ ] 3. Create the JSON table definition(s) in `config/tabulator/`
- [ ] 4. Write the HTML template
- [ ] 5. Write the CSS
- [ ] 6. Implement the manager class
- [ ] 7. Register with the sidebar
- [ ] 8. Wire up the footer
- [ ] 9. Test the build

---

## Step 1: Reserve a Manager ID

Each manager has a unique integer ID used by the sidebar and `app.loadManager()`. Check existing IDs in `src/managers/main/main.js` and pick the next available number.

| ID | Manager |
|----|---------|
| 1 | Login |
| 2 | Menu |
| 3 | Profile |
| 4 | Session |
| 5 | Crimson |
| 6 | Tour |
| 11 | Version Manager |
| 22 | Style Manager |
| 23 | Lookups Manager |
| 29 | Query Manager |
| ... | ... |

---

## Step 2: Create The File Structure

```
src/managers/<your-manager>/
├── <your-manager>.js        # Main manager class
├── <your-manager>.html      # HTML template
├── <your-manager>.css       # Manager-specific styles
└── <your-manager>-editors.js  # (Optional) CodeMirror editor init
```

Example for a "Contacts" manager:

```
src/managers/contacts/
├── contacts.js
├── contacts.html
├── contacts.css
```

---

## Step 3: Create JSON Table Definition(s)

Create one JSON file per table in `config/tabulator/<your-manager>/`:

```
config/tabulator/contacts/
├── contacts-list.json      # Main table (or parent table)
└── contact-details.json    # (Optional) Child table
```

### JSON Table Definition Template

```json
{
  "$schema": "../tabledef-schema.json",
  "$version": "1.0.0",
  "$description": "Column definitions for the Contacts table.",

  "table": "contacts-list",
  "title": "Contacts",
  "queryRef": 50,
  "searchQueryRef": 51,
  "updateQueryRef": 52,
  "insertQueryRef": 53,
  "deleteQueryRef": 54,
  "readonly": false,
  "selectableRows": 1,
  "layout": "fitColumns",
  "responsiveLayout": "collapse",
  "resizableColumns": true,
  "persistSort": true,
  "persistFilter": true,
  "initialSort": [
    { "column": "name", "dir": "asc" }
  ],

  "columns": {
    "contact_id": {
      "display": "ID#",
      "field": "contact_id",
      "coltype": "index",
      "visible": false,
      "sort": true,
      "filter": true,
      "editable": false,
      "primaryKey": true,
      "description": "Database primary key",
      "overrides": { "bottomCalc": null }
    },

    "name": {
      "display": "Name",
      "field": "name",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": true,
      "description": "Contact display name"
    }
  }
}
```

**Key fields:**

| Field | Purpose |
|-------|---------|
| `queryRef` | QueryRef for loading all rows |
| `searchQueryRef` | QueryRef for text search (optional) |
| `updateQueryRef` | QueryRef for saving existing rows |
| `insertQueryRef` | QueryRef for creating new rows |
| `deleteQueryRef` | QueryRef for deleting rows |
| `primaryKey: true` | Marks the primary key column (exactly one column) |

**Important:** The JSON files serve as **design-time documentation** of which QueryRefs a table uses. At runtime, the table definition is loaded from a database lookup (Lookup 059), which may not contain these CRUD fields. **You must always pass CRUD queryRefs explicitly in the LithiumTable constructor** — see Step 6 below. See [LITHIUM-TAB.md — QueryRef Resolution](LITHIUM-TAB.md#queryref-resolution-constructor--table-definition).

---

## Step 4: Write the HTML Template

### Single-Table Layout (like Query Manager)

```html
<div class="manager-panels mymanager-manager-container">
  <!-- Left Panel: Table -->
  <div class="mymanager-left-panel" data-panel="left">
    <div class="mymanager-table-container lithium-table-container" id="mymanager-table-container">
      <!-- LithiumTable injected here -->
    </div>
    <div class="mymanager-navigator-container lithium-navigator-container" id="mymanager-navigator">
      <!-- Navigator injected here -->
    </div>
  </div>

  <!-- Splitter -->
  <div class="lithium-splitter" data-splitter="left" id="mymanager-splitter"></div>

  <!-- Right Panel: Detail/Editor -->
  <div class="mymanager-right-panel" data-panel="right">
    <!-- Toolbar header with tabs -->
    <div class="mymanager-tabs-header">
      <div class="subpanel-header-group" style="flex:1;min-width:0;">
        <!-- Collapse button -->
        <button type="button"
          class="subpanel-header-btn subpanel-header-close mymanager-collapse-btn lithium-collapse-btn"
          id="mymanager-collapse-btn" data-collapse-target="left" title="Toggle Table">
          <fa fa-angles-left id="mymanager-collapse-icon" class="lithium-collapse-icon"></fa>
        </button>
        <!-- Tab buttons -->
        <button type="button" class="subpanel-header-btn subpanel-header-primary mymanager-tab-btn active"
          data-tab="editor"><fa fa-database></fa><span>Editor</span></button>
        <button type="button" class="subpanel-header-btn subpanel-header-primary mymanager-tab-btn"
          data-tab="collection"><fa fa-brackets-curly></fa><span>Collection</span></button>
        <!-- Placeholder fills space -->
        <button type="button" class="subpanel-header-btn subpanel-header-primary mymanager-toolbar-placeholder"
          tabindex="-1" aria-hidden="true"></button>
        <!-- Toolbar buttons (right side) -->
        <button type="button" class="subpanel-header-btn subpanel-header-close mymanager-toolbar-btn"
          id="mymanager-undo-btn" title="Undo" disabled><fa fa-rotate-left></fa></button>
        <button type="button" class="subpanel-header-btn subpanel-header-close mymanager-toolbar-btn"
          id="mymanager-redo-btn" title="Redo" disabled><fa fa-rotate-right></fa></button>
        <button type="button" class="subpanel-header-btn subpanel-header-close mymanager-toolbar-btn"
          id="mymanager-font-btn" title="Font Settings"><fa fa-font-case></fa></button>
      </div>
    </div>
    <!-- Tab panes -->
    <div class="mymanager-tabs-content">
      <div class="mymanager-tab-pane active" id="mymanager-tab-editor">
        <div class="mymanager-editor" id="mymanager-editor"></div>
      </div>
      <div class="mymanager-tab-pane" id="mymanager-tab-collection">
        <!-- Collection (JSON) editor -->
      </div>
    </div>
  </div>
</div>
```

### Dual-Table Layout (like Lookups Manager)

```html
<div class="manager-panels mymanager-manager-container">
  <!-- Left Panel: Parent Table -->
  <div class="mymanager-left-panel" data-panel="left" id="mymanager-left-panel">
    <div class="mymanager-table-container lithium-table-container" id="mymanager-parent-table-container"></div>
    <div class="mymanager-navigator-container lithium-navigator-container" id="mymanager-parent-navigator"></div>
  </div>

  <div class="lithium-splitter" data-splitter="left" id="mymanager-splitter-left"></div>

  <!-- Middle Panel: Child Table -->
  <div class="mymanager-middle-panel" data-panel="middle" id="mymanager-middle-panel">
    <div class="mymanager-child-table-container lithium-table-container" id="mymanager-child-table-container"></div>
    <div class="mymanager-navigator-container lithium-navigator-container" id="mymanager-child-navigator"></div>
  </div>

  <div class="lithium-splitter" data-splitter="right" id="mymanager-splitter-right"></div>

  <!-- Right Panel: Editor -->
  <div class="mymanager-right-panel" data-panel="right" id="mymanager-right-panel">
    <!-- Toolbar + tab panes as above, with TWO collapse buttons -->
  </div>
</div>
```

### Critical HTML Requirements

| Requirement | Reason |
|-------------|--------|
| Outer `<div>` must have class `manager-panels` | Shared layout CSS |
| Table container must have class `lithium-table-container` | LithiumTable shared styles |
| Navigator container must have class `lithium-navigator-container` | Navigator shared styles |
| Collapse buttons must have class `lithium-collapse-btn` | Shared rotation CSS |
| Icon inside collapse button must have class `lithium-collapse-icon` | Rotation target |
| `data-panel="left"` / `data-panel="right"` | Required by LithiumSplitter |
| `data-collapse-target="left"` (or `"middle"`) | Identifies which panel the button controls |

---

## Step 5: Write the CSS

Manager CSS should be minimal. Most styling is provided by shared classes.

```css
/* manager-specific layout */
.mymanager-manager-container { display: flex; height: 100%; }

.mymanager-left-panel {
  display: flex;
  flex-direction: column;
  width: 280px;
  min-width: 157px;
  flex: 0 0 auto;
}

.mymanager-right-panel {
  display: flex;
  flex-direction: column;
  flex: 1 1 auto;
  min-width: 0;
}

/* Table container fills available space */
.mymanager-table-container.lithium-table-container {
  margin: 0;
  border: none;
  border-radius: 0;
  flex: 1;
  min-height: 0;
}

.mymanager-table-container .tabulator {
  border: none;
  height: 100% !important;
}

/* Tab panes */
.mymanager-tabs-content { flex: 1; min-height: 0; position: relative; }
.mymanager-tab-pane { display: none; height: 100%; }
.mymanager-tab-pane.active { display: flex; flex-direction: column; }

/* Toolbar placeholder fills remaining space */
.mymanager-toolbar-placeholder { flex: 1 !important; cursor: default !important; }
```

**Do NOT duplicate:**

- Navigator styles (`.lithium-nav-*`) — provided by `lithium-table.css`
- Sort icon styles — auto-injected by LithiumTable for custom `cssPrefix` values
- Save/Cancel button styles — provided by `manager-ui.css`
- Collapse icon rotation — provided by `lithium-table.css`

---

## Step 6: Implement the Manager Class

### Imports

```javascript
import { LithiumTable } from '../../core/lithium-table-main.js';
import { LithiumSplitter } from '../../core/lithium-splitter.js';
import { PanelStateManager } from '../../core/panel-state-manager.js';
import { togglePanelCollapse } from '../../core/panel-collapse.js';
import { authQuery } from '../../shared/conduit.js';
import { toast } from '../../shared/toast.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { processIcons } from '../../core/icons.js';
import { setupManagerFooterIcons, createFontPopup } from '../../core/manager-ui.js';
import { ManagerEditHelper } from '../../core/manager-edit-helper.js';
import '../../styles/vendor-tabulator.css';
import './mymanager.css';

// Optional: CodeMirror (if you have editors)
import { EditorState, EditorView } from '../../core/codemirror.js';
import {
  buildEditorExtensions,
  createReadOnlyCompartment,
  setEditorEditable,
  setEditorContentNoHistory,
  updateUndoRedoButtons,
} from '../../core/codemirror-setup.js';
```

### Constructor

```javascript
export default class MyManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};

    // Table instances
    this.myTable = null;

    // Edit helper — one per manager, shared across all tables
    this.editHelper = new ManagerEditHelper({
      name: 'MyManager',
      onAfterEditModeChange: (isEditing) => this._updateUndoRedoButtons(),
    });

    // Panel state persistence
    this.leftPanelState = new PanelStateManager('lithium_mymanager_left');
    this.leftPanelWidth = this.leftPanelState.loadWidth(280);
    this.isLeftPanelCollapsed = this.leftPanelState.loadCollapsed(false);

    // Editor instances (optional)
    this.myEditor = null;           // CodeMirror instance
    this.collectionEditor = null;   // JSON editor
  }
```

### init() Method

**Order matters.** Tables must be created before splitters, and splitters before `restorePanelState()`.

```javascript
  async init() {
    await this._render();              // 1. Load HTML, cache elements
    this._setupEventListeners();       // 2. Bind buttons, tabs, shortcuts
    await this._initTable();           // 3. Create LithiumTable + register
    this._setupSplitters();            // 4. Create splitters (needs table)
    this._setupFooter();               // 5. Wire footer buttons
    this._restorePanelState();         // 6. Restore collapsed state (needs splitter)
  }
```

### Table Initialization

```javascript
  async _initTable() {
    this.myTable = new LithiumTable({
      container: this.elements.tableContainer,
      navigatorContainer: this.elements.navigatorContainer,
      tablePath: 'mymanager/my-table',     // → config/tabulator/mymanager/my-table.json
      queryRef: 50,                         // QueryRef for loading data
      searchQueryRef: 51,                   // QueryRef for search (optional)
      updateQueryRef: 52,                   // QueryRef for saving existing rows
      insertQueryRef: 53,                   // QueryRef for creating new rows
      deleteQueryRef: 54,                   // QueryRef for deleting rows
      cssPrefix: 'mymanager',               // CSS class prefix
      storageKey: 'mymanager_table',        // localStorage key
      app: this.app,
      readonly: false,
      panel: this.elements.leftPanel,
      panelStateManager: this.leftPanelState,

      // Callbacks
      onRowSelected: (rowData) => this._handleRowSelected(rowData),
      onRowDeselected: () => this._handleRowDeselected(),
      onDataLoaded: (rows) => {
        log(Subsystems.TABLE, Status.INFO, `[MyManager] Loaded ${rows.length} rows`);
      },

      // Custom save (required when you have external editors)
      onExecuteSave: (row, editHelper) => this._executeSave(row, editHelper),
    });

    // Register with editHelper — auto-wires onEditModeChange + onDirtyChange
    this.editHelper.registerTable(this.myTable);

    // Register external editors for dirty tracking + cancel restore
    this.editHelper.registerEditor('collection', {
      getContent:  () => this.collectionEditor?.state?.doc?.toString() || '{}',
      setContent:  (content) => setEditorContentNoHistory(this.collectionEditor, content),
      setEditable: (editable) => setEditorEditable(
        this.collectionEditor, this._readOnlyCompartment, editable, this.elements.collectionEditor
      ),
      boundTable:  this.myTable,
    });

    await this.myTable.init();
    await this.myTable.loadData();
  }
```

### Custom Save Logic

If your manager has external editors (CodeMirror, SunEditor, etc.), you **must** provide `onExecuteSave` to include their content in the save. Without it, only table cell data is saved.

```javascript
  async _executeSave(row, _editHelper) {
    const rowData = row.getData();
    const pkField = this.myTable.primaryKeyField || 'contact_id';
    const pkValue = rowData[pkField];
    const isInsert = pkValue == null || pkValue === '' || pkValue === 0;
    // Hardcoded fallbacks as safety net (same pattern as Query Manager)
    const queryRef = isInsert
      ? (this.myTable.queryRefs?.insertQueryRef ?? 53)
      : (this.myTable.queryRefs?.updateQueryRef ?? 52);

    if (!queryRef) {
      throw Object.assign(new Error('No queryRef configured for save'), {
        serverError: {
          error: 'Save Not Configured',
          message: `No ${isInsert ? 'insertQueryRef' : 'updateQueryRef'} configured.`,
        },
      });
    }

    // Gather content from external editors
    const collectionContent = this.collectionEditor?.state?.doc?.toString() || '{}';

    const params = {
      INTEGER: { [pkField.toUpperCase()]: pkValue },
      STRING:  { COLLECTION: collectionContent },
      JSON:    rowData,
    };

    const result = await authQuery(this.app.api, queryRef, params);

    // Update row with server-returned PK on insert
    if (isInsert && result?.[0]?.[pkField] != null) {
      row.update({ [pkField]: result[0][pkField] });
    }
  }
```

**If your table has no external editors**, you can skip `onExecuteSave` entirely. The default save path in LithiumTable sends `{ INTEGER: { PK: value }, JSON: rowData }` to `updateQueryRef`/`insertQueryRef` from the JSON config.

### Child Table with Parameters (Parent/Child Pattern)

When a child table requires a parameter (e.g., a parent ID) for its data query:

```javascript
  this.childTable = new LithiumTable({
    // ...
    queryRef: 34,
    onExecuteSave: (row, editHelper) => this._executeChildSave(row, editHelper),
    // Required: re-query with the correct parameter on refresh
    onRefresh: () => {
      if (this.selectedParentId != null) {
        this.loadChildData(this.selectedParentId);
      }
    },
  });
```

Without `onRefresh`, the Navigator refresh button calls `loadData()` with no parameters, which will fail for parameterised queries.

### Splitter Setup

```javascript
  _setupSplitters() {
    this.splitter = new LithiumSplitter({
      element: this.elements.splitter,
      leftPanel: this.elements.leftPanel,
      minWidth: 157,
      maxWidth: 1000,
      tables: this.myTable,    // or [this.parentTable, this.childTable]
      onResize: (width) => { this.leftPanelWidth = width; },
      onResizeEnd: (width) => {
        this.leftPanelState.saveWidth(width);
        this.myTable?.table?.redraw?.();
      },
    });

    // Bind splitter to table for width mode clearing on drag
    this.myTable?.setSplitter(this.splitter);
  }
```

### Footer Setup

```javascript
  _setupFooter() {
    const slot = this.container.closest('.manager-slot');
    if (!slot) return;
    const footer = slot.querySelector('.manager-slot-footer');
    if (!footer) return;
    const group = footer.querySelector('.subpanel-header-group');
    if (!group) return;
    const placeholder = group.querySelector('.slot-footer-placeholder');

    const footerElements = setupManagerFooterIcons(group, {
      onPrint: () => this._handlePrint(),
      onEmail: () => this._handleEmail(),
      onExport: (e) => this._toggleExportPopup(e),
      reportOptions: [
        { value: 'mymanager-view', label: 'My Table View' },
        { value: 'mymanager-data', label: 'My Table Data' },
      ],
      fillerTitle: 'MyManager',
      anchor: placeholder,
      showSaveCancel: true,
    });

    this._footerDatasource = footerElements.reportSelect;

    // Wire save/cancel buttons through editHelper (standard pattern)
    this.editHelper.wireFooterButtons(
      footerElements.saveBtn,
      footerElements.cancelBtn,
      footerElements.dummyBtn,
    );
  }
```

### Lifecycle Methods

```javascript
  onActivate() {
    this.myTable?.table?.redraw?.();
  }

  onDeactivate() { /* preserve state */ }

  cleanup() {
    this.editHelper?.destroy();
    this.myEditor?.destroy();
    this.collectionEditor?.destroy();
    this.splitter?.destroy();
    this.myTable?.destroy();
  }
```

---

## Step 7: Register with the Sidebar

Add the manager to the sidebar configuration in `src/managers/main/main.js`. The sidebar entry maps the Manager ID to an icon and label:

```javascript
{ id: <your-manager-id>, icon: 'fa-address-book', label: 'Contacts' }
```

And add the lazy-import in the manager loader:

```javascript
case <your-manager-id>:
  module = await import('../contacts/contacts.js');
  break;
```

---

## Step 8: Wire Up the Footer

The footer is part of the manager slot, not the manager itself. The `setupManagerFooterIcons()` utility creates Print, Email, Export, Save, Cancel, and Dummy buttons and inserts them into the slot footer. The manager wires Save/Cancel through `editHelper.wireFooterButtons()`.

The flow when Save is clicked:

1. `editHelper.wireFooterButtons()` adds a click listener
2. Click calls `this.activeEditingTable.handleSave()`
3. `handleSave()` (in `lithium-table-ops.js`) checks `isDirty`, then calls `executeSave()`
4. `executeSave()` calls `this.onExecuteSave(row, editHelper)` if provided
5. If `onExecuteSave` is not provided, the default path uses `updateQueryRef`/`insertQueryRef`
6. On success, edit mode exits and a toast is shown

---

## Step 9: Verify the Build

```bash
cd elements/003-lithium
npm run build
```

The build must complete without errors. Since Lithium requires a running Hydrogen backend, runtime testing is done in a configured environment — not via `npm run dev`.

---

## Common Patterns Reference

### Editor Registration (for dirty tracking + cancel restore)

Every external editor must provide four callbacks:

| Callback | Purpose |
|----------|---------|
| `getContent` | Returns current content as string (for snapshot comparison) |
| `setContent` | Restores content from snapshot string (called on cancel) |
| `setEditable` | Called with `(boolean)` when edit mode enters/exits |
| `boundTable` | Associates this editor with a specific LithiumTable |

**All four are required for full functionality.** Without `setContent`, Cancel will only revert table row data — the editor will retain dirty content.

```javascript
this.editHelper.registerEditor('myeditor', {
  getContent:  () => this.myEditor?.state?.doc?.toString() || '',
  setContent:  (content) => setEditorContentNoHistory(this.myEditor, content),
  setEditable: (editable) => setEditorEditable(this.myEditor, this._compartment, editable, container),
  boundTable:  this.myTable,
});
```

### Dirty Check from Editor Changes

```javascript
// CodeMirror updateListener
buildEditorExtensions({
  onUpdate: (update) => {
    if (update.docChanged && this.myTable?.isEditing) {
      this.editHelper.checkDirtyState();
    }
    if (update.transactions.length > 0) {
      this._updateUndoRedoButtons();
    }
  },
});

// SunEditor onChange
this.sunEditor.onChange = () => {
  if (this.myTable?.isEditing) {
    this.editHelper.checkDirtyState();
  }
};
```

### Double-Click to Enter Edit Mode (on editors)

```javascript
this.elements.editorContainer?.addEventListener('dblclick', () => {
  if (!this.myTable?.isEditing && this.myTable?.table) {
    const selected = this.myTable.table.getSelectedRows();
    if (selected.length > 0) void this.myTable.enterEditMode(selected[0]);
  }
});
```

### Panel Collapse/Expand

```javascript
import { togglePanelCollapse, restorePanelState } from '../../core/panel-collapse.js';

_toggleLeftPanel() {
  this.isLeftPanelCollapsed = togglePanelCollapse({
    panel: this.elements.leftPanel,
    splitter: this.splitter,
    collapseBtn: this.elements.collapseBtn,
    panelWidth: this.leftPanelWidth,
    isCollapsed: this.isLeftPanelCollapsed,
    onAfterToggle: () => this.myTable?.table?.redraw?.(),
  });
  this.leftPanelState.saveCollapsed(this.isLeftPanelCollapsed);
}
```

---

## Key Mistakes to Avoid

### 1. Forgetting `setContent` in editor registrations

Without `setContent`, Cancel only reverts the table row — editors keep dirty content.

### 2. Not providing `onRefresh` for parameterised child tables

The Navigator refresh button calls `loadData()` with no parameters by default. Child tables that require a parent ID will fail silently.

### 3. Not providing `onExecuteSave` when you have external editors

Without `onExecuteSave`, only Tabulator cell data is saved. Editor content (CodeMirror, SunEditor) is ignored.

### 4. NOT passing CRUD queryRefs in the constructor

The `config/tabulator/*.json` files are **not served** at runtime — they are design-time documentation only. The database lookup schema (Lookup 059) may also be missing CRUD refs. You **must** always pass `updateQueryRef`, `insertQueryRef`, and `deleteQueryRef` explicitly in the LithiumTable constructor. Also include hardcoded fallbacks in `onExecuteSave`.

### 5. Wrong init order

Tables must be created **before** splitters (the splitter's `tables` option requires the LithiumTable instance). Splitters must be created **before** `restorePanelState()`.

### 6. Duplicate CSS — do not re-declare shared styles

Navigator, sort icon, save/cancel button, and collapse icon styles are all shared. Do not duplicate them in your manager's CSS file.

---

## Related Documentation

| Document | What to read |
|----------|-------------|
| [LITHIUM-TAB.md](LITHIUM-TAB.md) | LithiumTable component, Navigator, edit mode, queryRef resolution, collapsible panels, ManagerEditHelper |
| [LITHIUM-MGR-QUERY.md](LITHIUM-MGR-QUERY.md) | Reference single-table implementation |
| [LITHIUM-MGR-LOOKUPS.md](LITHIUM-MGR-LOOKUPS.md) | Reference dual-table (parent/child) implementation |
| [LITHIUM-MGR.md](LITHIUM-MGR.md) | Manager system overview |
| [LITHIUM-DEV.md](LITHIUM-DEV.md) | Shared UI utilities (`createFontPopup`, `setupManagerFooterIcons`, etc.) |
| [LITHIUM-ICN.md](LITHIUM-ICN.md) | Icon system and `<fa>` tags |
| [LITHIUM-CSS.md](LITHIUM-CSS.md) | CSS variables and architecture |

---

Last updated: April 2026
