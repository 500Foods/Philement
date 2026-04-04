# Lithium Query Manager

This document describes the Query Manager — a fully implemented manager for viewing, editing, and managing database queries.

---

## Overview

The Query Manager (Manager ID: 4) provides a dual-panel interface for managing database queries. It was the first manager to implement the reusable `LithiumTable` component.

**Location:** `src/managers/queries/`

**Key Files:**

- `queries.js` — Main manager class (LithiumTable + ManagerEditHelper + editors)
- `queries-editors.js` — CodeMirror editor initialization (SQL, Summary, Collection)
- `queries.css` — Styling
- `queries.html` — HTML template

**Retired Files** (functionality moved to LithiumTable + ManagerEditHelper):

- `queries-dirty.js` — Replaced by ManagerEditHelper snapshot tracking
- `queries-edit-mode.js` — Replaced by LithiumTable.isEditing + editHelper
- `queries-navigation.js` — Replaced by LithiumTable nav + `onExecuteSave` callback

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

- **CodeMirror editor** with JSON syntax highlighting
- JSON configuration/parameters for the query
- Fold/unfold support for nested structures

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

The Query Manager follows the **standard manager pattern** — identical to Lookups and Style managers:

```javascript
export default class QueriesManager {
  constructor(app, container) {
    this.queryTable = null;              // LithiumTable instance
    this.editHelper = new ManagerEditHelper({ name: 'Queries' });
    this.editorManager = createEditorManager(this);  // CodeMirror init
    // ... panel state, font popup, editor refs ...
  }

  async init() {
    await this._render();
    this._setupEventListeners();
    await this._initQueryTable();        // LithiumTable + registerTable + registerEditor
    this._setupSplitters();
    this._setupFooter();                 // wireFooterButtons through editHelper
    this._restorePanelState();
  }

  // Custom save: assembles table row data + editor content into API params
  async _executeSave(row, editHelper) { /* ... */ }

  // Row selection → loads detail data into editors
  async _handleRowSelected(rowData) { /* ... */ }

  // Tab switching → lazy-init editors
  async switchTab(tabId) { /* ... */ }

  // Lifecycle
  onActivate() { /* Redraw table */ }
  cleanup() { /* editHelper.destroy(), editors, table */ }
}
```

### LithiumTable Integration

The Query Manager uses a single `LithiumTable` instance with `ManagerEditHelper`:

```javascript
async _initQueryTable() {
  this.queryTable = new LithiumTable({
    container: this.elements.tableContainer,
    navigatorContainer: this.elements.navigatorContainer,
    tablePath: 'queries/query-manager',
    queryRef: 25,
    searchQueryRef: 32,
    app: this.app,
    // Custom save assembles params from table + editors
    onExecuteSave: (row, editHelper) => this._executeSave(row, editHelper),
  });

  // Register with editHelper — auto-wires onEditModeChange + onDirtyChange
  this.editHelper.registerTable(this.queryTable);

  // Register editors for dirty tracking + cancel restore
  this.editHelper.registerEditor('sql', {
    getContent:  () => this.sqlEditor?.state?.doc?.toString() || '',
    setContent:  (content) => { /* dispatch to CodeMirror */ },
    setEditable: (editable) => this.editorManager._setCodeMirrorEditable(editable),
    boundTable:  this.queryTable,
  });
  // ... summary, collection editors similarly ...
}
```

### Dirty State Tracking

Dirty tracking uses the **standard `ManagerEditHelper` snapshot system** (shared with all managers):

- **On edit mode enter:** `editHelper._takeSnapshot()` captures table row data + all registered editor content
- **On cell edit:** Tabulator `cellEdited` → `notifyDirtyChange()` → `editHelper.checkDirtyState()`
- **On CodeMirror change:** `update.docChanged` → `editHelper.checkDirtyState()`
- **Comparison:** Current table row + editor content vs. snapshot — field-by-field and string equality
- **Buttons:** Save/Cancel enabled only when dirty AND in edit mode; revert to disabled if changes are undone
- **Cancel:** Reverts table row via `originalRowData` + editors via `editHelper.restoreEditorSnapshots()`

See [LITHIUM-TAB.md](LITHIUM-TAB.md#footer-savecancel-buttons-and-manageredithelper) for the full pattern.

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
- **Default template by name** — A saved template named `Default` auto-loads on startup
- **Delete templates** — Remove unwanted templates

### Storage

Templates are stored in localStorage:

- Key: `lithium_queries_templates`
- Format: Array of template objects

The current LithiumTable template menu actions are `Save Template`, `Clear Template`, `Copy to Clipboard`, and `Delete Template`. There is no separate "Make default" action; using the exact template name `Default` is what marks a saved local template for auto-load at startup.

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
- **SQL editor** — Type SQL code (CodeMirror)
- **Summary editor** — Type Markdown (CodeMirror)
- **Collection editor** — Edit JSON (CodeMirror)

### Exiting Edit Mode

- **Save** (`💾` or `Ctrl+S`) — Commits to API via QueryRef 28
- **Cancel** (`🚫` or `Escape`) — Reverts all changes (table row + all CodeMirror editors)
- **Click another row** — Auto-saves if dirty, then loads new row's details
- **Click Edit button** (while editing) — Auto-saves if dirty, otherwise exits edit mode
- **Navigator buttons** (First/Last/Prev/Next/Page) — Auto-saves if dirty, then navigates

### Auto-Save on Row Change

When clicking a different row or navigating while in edit mode:

1. Performs a **synchronous dirty check** (`isActuallyDirty()`) — bypasses the rAF-deferred `isDirty` flag to handle fast type-then-click scenarios
2. If dirty: saves the **editing row** (not the newly-clicked row), shows "Changes Saved" toast
3. If save fails: reverts selection back to the editing row and stays in edit mode
4. If not dirty: exits edit mode silently
5. Proceeds to select the new row and load its details

### Undo/Redo Toolbar Buttons

The undo/redo buttons in the toolbar operate on the **active tab's CodeMirror editor** (SQL, Summary, or Collection):

- **Disabled** when not in edit mode
- **Enabled/disabled** based on the active editor's undo/redo history depth
- When all changes are undone via undo, the dirty comparison returns to "not dirty" and Save/Cancel buttons automatically revert to disabled
- **Programmatic content loads** (row selection, cancel restore) are excluded from the undo history via `setEditorContentNoHistory()`, so undo only covers user edits made during the editing session

See [LITHIUM-TAB.md](LITHIUM-TAB.md#undoredo-toolbar-buttons-with-codemirror) for the full implementation pattern.

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

### Consolidated Architecture

The Query Manager was originally the first LithiumTable implementation, with custom modules for dirty tracking (`queries-dirty.js`), edit mode (`queries-edit-mode.js`), and navigation/save (`queries-navigation.js`). These have been **retired** — all functionality now flows through the standard `LithiumTable` + `ManagerEditHelper` pattern shared by every manager.

**Key architectural decisions:**

1. **`onExecuteSave` callback** — Custom save logic lives in the manager, not in a separate module. The callback assembles table row data + editor content into API params.
2. **`editHelper.registerEditor()`** — Every external editor (CodeMirror, SunEditor, form fields, etc.) provides `getContent`, `setContent`, and `setEditable` callbacks. The editHelper handles snapshot comparison, dirty detection, and cancel restore uniformly.
3. **`editHelper.wireFooterButtons()`** — Save/Cancel buttons are wired through the editHelper, which delegates to `activeEditingTable.handleSave()` / `handleCancel()`.
4. **No parallel systems** — One dirty tracker (editHelper snapshots), one edit mode source of truth (`LithiumTable.isEditing`), one button state manager (editHelper).

### CodeMirror Integration

Editors are created dynamically when tabs are first shown (`queries-editors.js`). Each editor's `onUpdate` does two things:
- If `docChanged` and editing: calls `editHelper.checkDirtyState()` to update Save/Cancel buttons
- If transactions occurred: calls `_updateUndoRedoButtons()` to update undo/redo button state

**No-history dispatches:** All programmatic content loads (row selection in `switchTab()`, cancel restore via `setContent` callbacks, clearing in `_clearQueryDetails()`) use `setEditorContentNoHistory()` from `codemirror-setup.js`. This ensures the undo history only contains user edits made during the editing session — undoing all changes returns the editor to its snapshot state, which automatically disables Save/Cancel.

**`onAfterEditModeChange` callback:** The editHelper fires this after entering/exiting edit mode. The Query Manager uses it to update undo/redo button state (disabled when not editing).

### Panel State Persistence

- **Selected row:** Stored in localStorage by primary key
- **Panel width:** Persisted across sessions
- **Layout mode:** fitColumns, fitData, etc.
- **Column visibility:** Via template system

All state is restored on manager load.
