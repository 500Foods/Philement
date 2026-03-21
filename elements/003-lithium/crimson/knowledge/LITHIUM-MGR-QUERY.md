# Lithium Query Manager

This document describes the Query Manager — a fully implemented manager for viewing, editing, and managing database queries.

---

## Overview

The Query Manager (Manager ID: 4) provides a dual-panel interface for managing database queries. It was the first manager to implement the reusable `LithiumTable` component.

**Location:** `src/managers/queries/`

**Key Files:**

- `queries.js` — Main manager class
- `queries.css` — Styling
- `queries.html` — HTML template

---

## UI Layout

```diagram
┌─────────────────┬──────────────────────────────────────────┐
│                 │  [SQL] [Summary] [Collection] [Preview] │
│  Query List     ├──────────────────────────────────────────┤
│  (Tabulator)    │                                          │
│                 │  SQL Editor (CodeMirror)                 │
│  ┌───────────┐  │  ┌──────────────────────────────────┐    │
│  │ Query 001 │  │  │ SELECT * FROM users WHERE...     │    │
│  │ Query 002 │  │  │                                  │    │
│  │ Query 003 │  │  │                                  │    │
│  │ ...       │  │  └──────────────────────────────────┘    │
│  └───────────┘  │                                          │
│                 │  [Toolbar: Undo Redo Font Prettify]      │
│  [Navigator]    │                                          │
│                 ├──────────────────────────────────────────┤
│                 │  Summary (Markdown) - other tab          │
│                 │  Collection (JSON) - other tab           │
│                 │  Preview (Rendered) - other tab          │
└─────────────────┴──────────────────────────────────────────┘
```

### Left Panel: Query List

A Tabulator table showing all available queries.

**Features:**

- LithiumTable component with full Navigator
- Row selection loads query details in right panel
- Columns: Ref, Name, Type, Dialect, Status
- Full CRUD operations on queries

**QueryRefs:**

- **25** — Query List (all queries)
- **32** — Query Search

### Right Panel: Query Details

Tabbed interface showing different aspects of the selected query.

#### SQL Tab

- **CodeMirror editor** with SQL syntax highlighting
- Line numbers (padded to 4 digits)
- Undo/redo support
- Font size controls
- SQL prettify button
- Read-only until edit mode

#### Summary Tab

- **CodeMirror editor** with Markdown highlighting
- Documentation and notes for the query
- Renders in Preview tab

#### Collection Tab

- **vanilla-jsoneditor** tree view
- JSON configuration/parameters for the query
- Expandable/collapsible nodes

#### Preview Tab

- Renders Markdown from Summary tab
- Uses `marked` library
- Read-only preview

### Collapsible Left Panel

The left panel can be collapsed to maximize the editor:

- **Collapse button:** Arrow icon in right panel header
- **Animation:** Smooth CSS transition
- **Tables redraw** after panel resize

---

## Architecture

### Class Structure

```javascript
export default class QueriesManager {
  constructor(app, container) {
    // Store app reference and container
    // Initialize state (editing, dirty tracking, etc.)
  }

  async init() {
    // Render HTML from template
    // Setup event listeners
    // Setup splitter
    // Initialize LithiumTable
    // Setup footer controls
    // Load queries
  }

  // Row selection
  loadQueryDetails(queryData) { /* ... */ }
  clearQueryDetails() { /* ... */ }

  // Tab switching
  switchTab(tabId) { /* ... */ }

  // Edit mode
  enterEditMode(row) { /* ... */ }
  exitEditMode(reason) { /* ... */ }

  // Dirty state tracking
  isAnyDirty() { /* Check all editors */ }
  setDirty(type, isDirty) { /* ... */ }

  // Lifecycle
  onActivate() { /* Redraw table */ }
  cleanup() { /* Destroy editors */ }
}
```

### LithiumTable Integration

The Query Manager uses a single `LithiumTable` instance:

```javascript
await this.initTable();

// Called within init()
async initTable() {
  // Load column configuration from JSON
  [this.coltypes, this.tableDef] = await Promise.all([
    loadColtypes(),
    loadTableDef('queries/query-manager'),
  ]);

  // Extract query refs from config
  this.queryRefs = getQueryRefs(this.tableDef);
  this.primaryKeyField = getPrimaryKeyField(this.tableDef);

  // Build columns with selector
  const selectorColumn = this.buildSelectorColumn();
  const dataColumns = resolveColumns(this.tableDef, this.coltypes, {
    filterEditor: this.createFilterEditor.bind(this),
  });

  // Create Tabulator instance
  this.table = new Tabulator(this.elements.tableContainer, {
    columns: [selectorColumn, ...dataColumns],
    // ... other options
  });
}
```

### Dirty State Tracking

The manager tracks changes across multiple editors:

```javascript
this.isDirty = {
  table: false,      // Tabulator row edited
  sql: false,        // SQL editor changed
  summary: false,    // Summary editor changed
  collection: false, // Collection editor changed
};
```

**Tracking:**

- **Table:** `cellEdited` event
- **SQL:** CodeMirror `docChanged` listener
- **Summary:** CodeMirror `docChanged` listener
- **Collection:** vanilla-jsoneditor `onChange` callback

**Behavior:**

- Save/Cancel buttons only enabled when dirty AND in edit mode
- Cancel reverts all changes to original values
- Save commits to API

---

## Footer Controls

The Query Manager adds custom footer buttons:

| Button | Action |
|--------|--------|
| 🖨️ Print | Print current table view |
| ✉️ Email | Email table data as formatted text |
| 📤 Export | Popup with PDF/CSV/TXT/XLS options |
| **Select** | View Mode / Data Mode toggle |

**Export Options:**

- **PDF** — Landscape orientation
- **CSV** — Comma-separated values
- **TXT** — Plain text (tab-separated)
- **XLS** — Excel format

**View vs Data Mode:**

- **View** — Uses current filters/sort (what user sees)
- **Data** — Uses all loaded data (ignores filters)

---

## QueryRefs

| Ref | Purpose | Parameters |
|-----|---------|------------|
| 25 | Query List | None |
| 27 | Get System Query | `QUERYID` (integer) |
| 28 | Update System Query | Multiple params |
| 32 | Query Search | `SEARCH` (string) |

### Important Note on QueryRef 27

When fetching full query details, use `query_id` (primary key), NOT `query_ref` (display number):

```javascript
// CORRECT
const queryDetails = await authQuery(api, 27, {
  INTEGER: { QUERYID: queryData.query_id },
});

// INCORRECT (was a common mistake)
const queryDetails = await authQuery(api, 27, {
  INTEGER: { QUERY_REF: queryData.query_ref },
});
```

The table displays `query_ref` (human-readable like "001"), but the API requires `query_id` (database primary key).

---

## Template System

The Query Manager includes a template system for saving column configurations:

### Features

- **Save templates** — Current column layout, visibility, widths
- **Load templates** — Apply saved configuration
- **Set default** — Auto-load template on startup
- **Delete templates** — Remove unwanted templates

### Storage

Templates are stored in localStorage:

- Key: `lithium_queries_templates`
- Format: Array of template objects

```javascript
{
  name: "My Template",
  columns: [...],
  sorters: [...],
  filters: [...],
  panelWidth: 350,
  layoutMode: "fitColumns",
  createdAt: "2026-03-17T..."
}
```

---

## Column Chooser

Click the selector column header (three dots) to open the column chooser:

**Features:**

- Checkboxes to show/hide columns
- Lists all columns including discovered ones
- Changes persist per session
- Can be saved to template

---

## Keyboard Shortcuts

### Table Navigation

| Key | Action |
|-----|--------|
| `↑` / `↓` | Previous/Next record |
| `Page Up` / `Page Down` | Previous/Next page |
| `Home` / `End` | First/Last record |
| `Enter` | Enter edit mode |

### Global

| Shortcut | Action |
|----------|--------|
| `Ctrl/Cmd + S` | Save (when editing) |
| `Escape` | Cancel (when editing) |

### SQL Editor

| Shortcut | Action |
|----------|--------|
| `Ctrl/Cmd + Z` | Undo |
| `Ctrl/Cmd + Shift + Z` | Redo |
| `Ctrl/Cmd + P` | Prettify SQL |

---

## Edit Mode

### Entering Edit Mode

1. Select a query row
2. Click **Edit** button (or double-click row)
3. All editors become editable
4. I-cursor appears in selector column

### During Edit Mode

- **Tabulator cells** — Click to edit inline
- **SQL editor** — Type SQL code
- **Summary editor** — Type Markdown
- **Collection editor** — Edit JSON tree

### Exiting Edit Mode

- **Save** (`💾` or `Ctrl+S`) — Commits to API via QueryRef 28
- **Cancel** (`🚫` or `Escape`) — Reverts all changes

### Auto-Save on Navigation

When navigating to a different row while dirty:

1. Automatically saves current row
2. If save fails, stays on current row
3. Shows toast notification

---

## CSS Architecture

### Key Classes

| Class | Purpose |
|-------|---------|
| `.queries-manager-container` | Flex container |
| `.queries-left-panel` | Query list panel |
| `.queries-right-panel` | Details panel |
| `.queries-splitter` | Draggable divider |
| `.queries-edit-mode` | Applied during editing |
| `.queries-filters-visible` | Header filters shown |

### Layout Variables

- **Default panel width:** 350px (314px compact mode)
- **Min width:** 157px
- **Max width:** Container width - 313px
- **Transition:** 350ms ease

---

## Related Documentation

- [LITHIUM-MGR.md](LITHIUM-MGR.md) — Manager system overview
- [LITHIUM-TAB.md](LITHIUM-TAB.md) — LithiumTable component (extracted from this manager)
- [LITHIUM-MGR-LOOKUPS.md](LITHIUM-MGR-LOOKUPS.md) — Similar dual-panel design

---

## Implementation Notes

### First LithiumTable Implementation

The Query Manager was the first to use the Tabulator+Navigator pattern. The reusable `LithiumTable` component was extracted from this implementation.

**Key innovations:**

1. **Edit mode gating** — Editors only active in edit mode
2. **Multi-editor dirty tracking** — SQL, Summary, Collection tracked separately
3. **Template system** — Save/load column configurations
4. **Loading overlay** — Custom spinner pattern

### CodeMirror Integration

Editors are created dynamically when tabs are first shown:

```javascript
switchTab(tabId) {
  if (tabId === 'sql' && !this.sqlEditor) {
    this.initSqlEditor(content);
  }
  // ... similar for other tabs
}
```

This improves initial load performance.

### Panel State Persistence

- **Selected row:** Stored in localStorage by primary key
- **Panel width:** Persisted across sessions
- **Layout mode:** fitColumns, fitData, etc.
- **Column visibility:** Via template system

All state is restored on manager load.
