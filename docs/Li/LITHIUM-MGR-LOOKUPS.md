# Lithium Lookups Manager

This document describes the Lookups Manager, a dual-table interface for managing lookup tables in Lithium.

---

## Overview

The Lookups Manager (Manager ID: 5) provides a two-panel interface for managing lookup tables — key-value pairs used throughout the Philement platform for enumerations, status codes, and reference data.

**Key Files:**

- `src/managers/lookups/lookups.js` — Main manager class
- `src/managers/lookups/lookups.css` — Styling
- `src/managers/lookups/lookups.html` — HTML template (optional reference)

---

## UI Layout

The interface is divided into two primary panels with a resizable splitter between them:

```diagram
┌─────────────────┬──────────────────────────────────────────┐
│                 │                                          │
│  Parent Table   │         Right Panel (Child)              │
│  (Lookups List) │                                          │
│                 │  ┌────────────────────────────────────┐  │
│  ┌───────────┐  │  │ [◀] Lookup Name (ID)          │  │
│  │ Lookup 1  │  │  └────────────────────────────────────┘  │
│  │ Lookup 2  │  │                                          │
│  │ Lookup 3  │  │  ┌────────────────────────────────────┐  │
│  │ ...       │  │  │                                    │  │
│  └───────────┘  │  │     Child Table (Values)           │  │
│                 │  │                                    │  │
│  [Navigator]    │  │  ┌────────────────────────────────┐  │
│                 │  │  │ ID │ Key │ Value │ Seq │ ...  │  │
│                 │  │  └────────────────────────────────┘  │
│                 │  │                                    │  │
│                 │  └────────────────────────────────────┘  │
│                 │                                          │
│                 │  [Navigator]                             │
│                 │                                          │
└─────────────────┴──────────────────────────────────────────┘
       ▲
       │ (resizable splitter)
```

### Left Panel: Parent Table

Displays all lookup tables available in the system.

**Features:**

- Tabulator data grid with selectable rows
- LithiumTable Navigator (Control, Move, Manage, Search blocks)
- Row selection triggers child table update
- Full CRUD operations (Create, Read, Update, Delete lookups)

**Data Source:**

- **QueryRef 30** — Lookup Names

**Columns:**

- `LOOKUPID` — Internal lookup identifier
- `LOOKUPNAME` — Human-readable lookup name
- Additional metadata columns as configured

### Right Panel: Child Table

Displays the values (key-value pairs) for the selected lookup.

**Features:**

- Tabulator data grid with full editing capabilities
- Header with collapse/expand button and lookup name display
- Independent LithiumTable Navigator
- Full CRUD operations on lookup values

**Data Source:**

- **QueryRef 034** — Get Lookup List (with `LOOKUPID` parameter)

**Columns:**

- `LOOKUPVALUEID` — Value record identifier
- `KEY_IDX` — The key/enum value
- `VALUE_TXT` — The display label
- `SORT_SEQ` — Sort order
- Additional fields as configured

### Splitter

A draggable splitter separates the two panels:

- **Width Range:** 200px (minimum) to 600px (maximum)
- **Default:** 350px
- **Persistence:** Current width is maintained during session
- **Touch Support:** Works on mobile devices

**Visual Feedback:**

- Changes color on hover
- Shows grab handle (vertical dots)
- Cursor changes to `col-resize` during drag

### Collapse/Expand

The left panel can be collapsed to maximize space for the child table:

- **Button:** Chevron left/right icon in child header
- **Animation:** Smooth CSS transition (350ms)
- **State:** Maintained during session
- **Tables:** Both tables redraw after animation completes

---

## Architecture

### Class Structure

```javascript
export default class LookupsManager {
  constructor(app, container) {
    // Store app reference and container
    // Initialize state variables
  }

  async init() {
    // Render HTML structure
    // Setup event listeners
    // Setup splitter
    // Initialize parent table (LithiumTable)
    // Initialize child table (LithiumTable)
    // Setup footer controls
  }

  // Parent/Child relationship handlers
  async handleParentRowSelected(rowData) { /* ... */ }
  handleParentRowDeselected() { /* ... */ }
  async loadChildData(lookupId) { /* ... */ }

  // UI controls
  toggleLeftPanel() { /* ... */ }
  setupSplitter() { /* ... */ }
  setupFooter() { /* ... */ }

  // Lifecycle
  onActivate() { /* Redraw tables */ }
  onDeactivate() { /* ... */ }
  cleanup() { /* Destroy tables */ }
}
```

### Dual LithiumTable Instances

The manager creates two independent `LithiumTable` instances, both registered with a shared `ManagerEditHelper`:

```javascript
import { ManagerEditHelper } from '../../core/manager-edit-helper.js';

// In constructor:
this.editHelper = new ManagerEditHelper({ name: 'Lookups' });

// Parent table (lookups list)
this.parentTable = new LithiumTable({
  container: this.elements.parentTableContainer,
  navigatorContainer: this.elements.parentNavigator,
  tablePath: 'lookups/lookups-list',
  queryRef: 30,
  cssPrefix: 'lithium',
  storageKey: 'lookups_parent_table',
  app: this.app,
  onRowSelected: (rowData) => this.handleParentRowSelected(rowData),
  onRowDeselected: () => this.handleParentRowDeselected(),
});
this.editHelper.registerTable(this.parentTable);

// Child table (lookup values)
this.childTable = new LithiumTable({
  container: this.elements.childTableContainer,
  navigatorContainer: this.elements.childNavigator,
  tablePath: 'lookups/lookup-values',
  queryRef: 34,
  cssPrefix: 'lithium',
  storageKey: 'lookups_child_table',
  app: this.app,
});
this.editHelper.registerTable(this.childTable);

// Register external editors bound to the child table
this.editHelper.registerEditor('json', {
  getContent:  () => this._getJsonEditorContent(),
  setEditable: (editable) => this.setEditorsEditable(editable),
  boundTable:  this.childTable,
});
this.editHelper.registerEditor('summary', {
  getContent:  () => this._getSummaryEditorContent(),
  setEditable: () => {},  // Handled by 'json' editor registration
  boundTable:  this.childTable,
});
```

Both tables are fully independent with:

- Separate Navigator control bars
- Independent column configurations
- Independent templates (saved to localStorage with different keys)
- **Shared edit mode tracking** — only one table can be in edit mode at a time (enforced by `ManagerEditHelper`)
- **Snapshot-based dirty tracking** — table row data + JSON editor + SunEditor content compared against snapshot

---

## Parent/Child Relationship

### Selection Flow

1. User clicks a row in the **parent table**
2. `handleParentRowSelected(rowData)` is called
3. Extract `LOOKUPID` and `LOOKUPNAME` from row data
4. Update child header title: `"LookupName (ID)"`
5. Call `loadChildData(lookupId)` to fetch values
6. Child table displays the lookup values

### Deselection Flow

1. User deselects the current lookup row
2. `handleParentRowDeselected()` is called
3. Clear child header title: `"Select a lookup"`
4. Clear child table data (empty array)

### Data Loading

```javascript
async loadChildData(lookupId) {
  const rows = await authQuery(this.app.api, 34, {
    INTEGER: { LOOKUPID: lookupId },
  });

  this.childTable.table.setData(rows);

  // Auto-select first row
  const activeRows = this.childTable.table.getRows('active');
  if (activeRows.length > 0) {
    activeRows[0].select();
  }
}
```

---

## Footer Controls

The footer contains four select dropdowns:

| Select | ID | Options | Purpose |
|--------|----|---------|---------|
| Parent View | `lookups-parent-select` | Lookup List View / Lookup List Data | Toggle parent table display mode |
| Child View | `lookups-child-select` | Lookup Values View / Lookup Values Data | Toggle child table display mode |

**Implementation Note:** The View/Data toggles are placeholders for future functionality that would allow switching between the formatted table view and a raw JSON/data view.

---

## Query References

### QueryRef 030 — Lookup Names

Returns all lookup tables in the system.

**Parameters:** None

**Response:** Array of lookup definitions

```javascript
[
  { LOOKUPID: 1, LOOKUPNAME: "Status Codes", ... },
  { LOOKUPID: 2, LOOKUPNAME: "Priority Levels", ... },
  ...
]
```

### QueryRef 034 — Get Lookup List

Returns all values for a specific lookup.

**Parameters:**

```javascript
{
  INTEGER: {
    LOOKUPID: <lookup_id>  // The lookup to fetch values for
  }
}
```

**Response:** Array of lookup values

```javascript
[
  { LOOKUPVALUEID: 101, KEY_IDX: 1, VALUE_TXT: "Active", SORT_SEQ: 10, ... },
  { LOOKUPVALUEID: 102, KEY_IDX: 2, VALUE_TXT: "Inactive", SORT_SEQ: 20, ... },
  ...
]
```

---

## CSS Architecture

### Layout Classes

| Class | Purpose |
|-------|---------|
| `.lookups-manager-container` | Flex container for entire manager |
| `.lookups-left-panel` | Parent table container (flex: 0 0 auto) |
| `.lookups-right-panel` | Child table container (flex: 1) |
| `.lookups-splitter` | Draggable divider between panels |
| `.lookups-table-container` | Parent table wrapper |
| `.lookups-child-table-container` | Child table wrapper |
| `.lookups-navigator-container` | Navigator bar containers |

### State Classes

| Class | Applied When |
|-------|--------------|
| `.collapsed` | Left panel is collapsed |
| `.resizing` | Splitter is being dragged |

### CSS Variables Used

All styling uses standard Lithium CSS variables:

- `--bg-primary`, `--bg-secondary`, `--bg-tertiary`
- `--text-primary`, `--text-secondary`, `--text-muted`
- `--accent-primary`, `--accent-hover`, `--accent-focus`
- `--border-color`, `--border-standard`
- `--space-2`, `--space-3` (spacing)
- `--border-radius-md`, `--border-radius-sm`

---

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl/Cmd + Shift + L` | Focus parent table |
| `Ctrl/Cmd + Shift + R` | Focus child table |
| `Ctrl/Cmd + B` | Toggle left panel collapse |

**Table Navigation (when table focused):**

- `↑` / `↓` — Previous/Next record
- `Page Up` / `Page Down` — Previous/Next page
- `Home` / `End` — First/Last record
- `Enter` — Enter edit mode (on selected row)

---

## Lifecycle

### Initialization

1. Manager instantiated by `app.loadManager(5)`
2. `new LookupsManager(app, workspaceEl)` called
3. `init()` called:
   - Renders HTML into workspace
   - Caches DOM element references
   - Sets up event listeners
   - Initializes splitter
   - Creates parent LithiumTable instance
   - Creates child LithiumTable instance
   - Loads parent data
   - Sets up footer controls

### Activation

When user switches to Lookups Manager:

- `onActivate()` called
- Both tables redraw to ensure proper layout

### Deactivation

When user switches to another manager:

- `onDeactivate()` called
- State is preserved (selected lookup, panel width, etc.)

### Cleanup

When manager is destroyed (rare):

- `cleanup()` called
- Both LithiumTable instances destroyed
- Event listeners removed
- DOM cleaned up

---

## Related Documentation

- [LITHIUM-MGR.md](LITHIUM-MGR.md) — Manager system overview
- [LITHIUM-TAB.md](LITHIUM-TAB.md) — LithiumTable component documentation
- [LITHIUM-MGR-QUERY.md](LITHIUM-MGR-QUERY.md) — Query Manager (similar dual-panel design)
- [LITHIUM-LUT.md](LITHIUM-LUT.md) — Lookup Tables system
- [LITHIUM-DEV.md](LITHIUM-DEV.md) — Shared UI utilities (createFontPopup, etc.)

---

## Future Enhancements

Potential improvements for the Lookups Manager:

1. **Hierarchical Lookups** — Support for nested/parent-child lookup relationships
2. **Bulk Operations** — Multi-select and bulk edit of lookup values
3. **Import/Export** — CSV/JSON import and export of lookup data
4. **Validation Rules** — Configurable validation for lookup keys and values
5. **Audit Trail** — Track changes to lookup definitions and values
6. **Localization** — Support for multi-language lookup values
