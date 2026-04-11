# Lithium Column Manager

The Column Manager provides a user interface for managing table columns at runtime. It allows users to reorder columns, toggle visibility, and edit column properties without modifying the underlying table definition.

## Table of Contents

1. [Overview](#overview)
2. [Stage 3: User Customization](#stage-3-user-customization)
3. [Architecture](#architecture)
4. [Column Manager UI](#column-manager-ui)
5. [Export/Import Workflow](#exportimport-workflow)
6. [Coltypes Reference](#coltypes-reference)
7. [Implementation Details](#implementation-details)
8. [API Reference](#api-reference)

## Overview

The Column Manager is a popup interface that displays all columns from a parent table and allows users to:

- **Reorder columns** via drag-and-drop
- **Toggle visibility** (show/hide columns)
- **Edit properties** like title, format, alignment, width, summary
- **Group columns** by category
- **Save changes** to apply to the parent table

Changes are tracked but not applied until the user clicks "Save". The "Cancel" button discards all pending changes.

The embedded LithiumTable runs in `alwaysEditable: true` mode. That means Column Manager cells edit directly on click, while the normal LithiumTable edit-mode workflow (Navigator manage block, Enter/F2/double-click edit entry, ManagerEditHelper footer Save/Cancel) is intentionally disabled inside the popup.

---

## Stage 3: User Customization

The Column Manager is the **Stage 3** component in the [tableDef resolution process](LITHIUM-TAB-TABLES.md). It applies user preferences on top of the Stage 2 table definition:

| Stage | Source | Description |
|-------|--------|-------------|
| Stage 1 | Query data + Lookup 059 Key 0 | Auto-detected coltype + merged defaults |
| Stage 2 | Lookup 059 Keys 1+ (table definitions) | Per-column overrides from tabledef |
| **Stage 3** | **Column Manager** | User preferences (customized view) |

The Column Manager allows users to override virtually **any** property from Stage 2, including:
- Column visibility
- Column width
- Column order (`columnPri`)
- `title`
- `coltype`
- `formatter`, `formatterParams`
- `editor`, `editorParams`
- `bottomCalc`, `bottomCalcFormatter`
- Any other property from the coltype system

Once a user customizes a table via the Column Manager, their preferences are stored as a user template and applied to subsequent table renders.

---

## Architecture

### Components

```
LithiumColumnManager
├── Popup Container (positioned relative to anchor element)
│   ├── Header (title + save/cancel/close buttons)
│   ├── Content Area
│   │   └── LithiumTable (the column manager table)
│   └── Resize Handle
└── State Management
    ├── pendingChanges (Map of field -> changes)
    ├── _pendingReorder (array of field names in new order)
    └── isDirty (boolean tracking if changes exist)
```

### Data Flow

1. **Open**: Load current column state from parent table
2. **Edit**: Track changes in `pendingChanges` Map
3. **Reorder**: Track new order in `_pendingReorder` array
4. **Save**: Build a template-state patch and apply it to the parent LithiumTable
5. **Cancel**: Discard changes, reload original data

---

## Column Manager UI

### Columns Displayed

| Column | Property | Type | Editable | Description |
|--------|----------|------|----------|-------------|
| Order | `columnPri` | integer | No | Display order (1, 2, 3...) |
| Drag Handle | — | string | No | Drag handle (⠿) for reordering |
| Visible | `visible` | boolean | Yes | Show/hide column |
| Field Name | `field` | string | No | Database field name |
| Column Name | `title` | string | Yes | Display title |
| Format | `coltype` | lookupFixed | Yes | Data type (string, integer, etc.) |
| Summary | `bottomCalc` | lookupFixed | Yes | Bottom calculation (count, sum, etc.) |
| Alignment | `hozAlign` | lookupFixed | Yes | Horizontal alignment |
| Category | `category` | stringList | Yes | Grouping category |
| Width | `width` | integer | Yes | Column width in pixels |

### Grouping

Columns are grouped by the "Category" field. Default category is "Default" if not specified.

### Row Reordering

Drag-and-drop is enabled via `movableRows: true`. The `rowMoved` event updates the order tracking.

### Embedded LithiumTable Behavior

- Cells are directly editable on click via `alwaysEditable: true`
- The standard Navigator manage block is hidden inside the popup
- Header Save/Cancel buttons batch the whole popup state instead of saving a single row
- Nested "Column Manager Manager" instances reuse the same behavior, but disable recursive column-manager launching at depth 2

---

## Export/Import Workflow

The Column Manager serves as our primary tableDef editor. The workflow for capturing and storing table definitions:

### Step 1: Customize in Column Manager

Use the Column Manager UI to adjust the table:
- Reorder columns (drag-and-drop)
- Toggle visibility
- Edit properties (title, width, alignment, format, summary)
- Group by category

### Step 2: Export as JSON

From the Templates menu, select "Copy tableDef as JSON". This copies the current tableDef to the clipboard:

```javascript
// Example exported tableDef
{
  "title": "My Table",
  "columns": {
    "query_id": {
      "title": "ID#",
      "field": "query_id",
      "coltype": "index",
      "visible": true,
      "width": 60,
      "columnPri": 1
    },
    "name": {
      "title": "Name",
      "field": "name",
      "coltype": "string",
      "visible": true,
      "width": 200,
      "columnPri": 2
    }
  }
}
```

### Step 3: Import to Lookup 059

The exported JSON can be pasted directly into a Lookup 059 entry:

- **Lookup 059 Key 1+**: Store as a table definition (Stage 2)
- This becomes the new default for that table

### Why This Matters

This workflow gives you absolute control:
1. Design the table with full UI (Column Manager)
2. Export the configuration
3. Store as the database default
4. Future renders use your custom configuration as Stage 2

---

## Coltypes Reference

### Standard Coltypes

These coltypes are defined in the coltypes system (Lookup 059 Key 0) and used throughout Lithium tables.

### Column Manager Coltypes (To Be Migrated)

The Column Manager currently uses hardcoded internal coltypes. These should be replaced with proper lookup table references (to Lookup entries in the database):

| Internal Coltype | Current Use | Should Be Replaced With |
|-----------------|-------------|------------------------|
| `lookupFixed` | Format, Summary, Alignment columns | Lookup table with predefined values |
| `stringList` | Category column | Lookup table or enum |

This is a known cleanup item. The goal is to remove hardcoded coltypes and use the standard lookup system for all dropdown/picklist functionality.

> **Note:** These internal coltypes will be migrated to proper lookup references in a future update. After that, all coltypes will be defined in Lookup 059 Key 0.

---

## Implementation Details

### Column Manager is a Table Too

The Column Manager is itself a LithiumTable, meaning:
- It has its own Lookup 059 entry (table definition)
- It can be customized via the Column Manager Manager
- All standard coltype system features apply

### Column Manager Manager

To customize the Column Manager itself, use the "Column Manager Manager" (recursion depth 2):
- Opens the Column Manager's table definition
- Allows editing its columns, visibility, order, etc.
- At depth 2, recursive column manager launching is disabled

This gives you complete control over the tool that controls all other tables.

### Column Manager Table Definition

```javascript
{
  title: 'Column Manager',
  readonly: false,
  layout: 'fitColumns',
  rowHeight: 32,
  movableRows: true,
  groupBy: 'category',
  groupStartOpen: true,
  columns: {
    // ... column definitions
  }
}
```

### Embedded LithiumTable Configuration

The popup wraps the shared `LithiumTable` component with a small set of special options:

```javascript
this.columnTable = new LithiumTable({
  tableDef: columnDef,
  readonly: false,
  alwaysEditable: true,
  storageKey: `${this.storageKey}_table`,
  useColumnManager: !isDepth2,
});
```

This keeps Column Manager on the shared LithiumTable/LithiumSplitter stack while cleanly opting out of the standard row edit-mode workflow.

### Change Tracking

```javascript
// Pending changes stored as Map: field -> { property: value }
this.pendingChanges = new Map();

// Pending reorder stored as array of field names
this._pendingReorder = [];

// Dirty state tracking
this.isDirty = false;
```

### Applying Changes

When Save is clicked:

1. Read the current popup rows, including edited widths/order/visibility
2. Build a template-style patch with `buildPendingTemplateColumns()`
3. Call `parentTable.applyTemplateColumns(templateColumns)`
4. Clear dirty state and close

```javascript
async applyAllChangesToParent() {
  const templateColumns = this.buildPendingTemplateColumns();
  if (Object.keys(templateColumns).length > 0) {
    this.parentTable.applyTemplateColumns(templateColumns);
  }

  this.clearDirty();
}
```

### Parent Table Update Process

The Column Manager no longer mutates parent columns through ad hoc remove/add logic. It routes changes through the shared LithiumTable template application path, which keeps behavior consistent with saved templates and runtime width/order persistence.

```javascript
buildPendingTemplateColumns() {
  // Merge current popup row values into a template-state patch
  // and preserve column order, visibility, width, alignment,
  // summary, title, editable flag, and format.
}
```

Because this patch is built from the live parent table state plus current popup edits, resized column widths are preserved when changes are applied or later saved into a table template.

---

## API Reference

### LithiumColumnManager

#### Constructor Options

| Option | Type | Required | Description |
|--------|------|----------|-------------|
| `parentContainer` | HTMLElement | Yes | Container for positioning |
| `anchorElement` | HTMLElement | Yes | Element to anchor popup to |
| `parentTable` | LithiumTable | Yes | Table to manage |
| `app` | Object | Yes | App instance for API access |
| `managerId` | string | Yes | Unique ID for this instance |
| `cssPrefix` | string | No | CSS class prefix (default: 'col-manager') |

#### Methods

| Method | Description |
|--------|-------------|
| `init()` | Initialize the column manager |
| `show()` | Show the popup |
| `hide()` | Hide the popup |
| `close()` | Close and cleanup |
| `refresh()` | Reload column data from parent |
| `applyAllChangesToParent()` | Apply pending changes |
| `discardAllChanges()` | Revert all changes |

#### Events

| Event | Callback | Description |
|-------|----------|-------------|
| `onColumnChange` | `(field, property, value) => {}` | Fired when a column property changes |
| `onClose` | `() => {}` | Fired when column manager closes |

---

**See Also:**
- [LITHIUM-TAB.md](LITHIUM-TAB.md) - LithiumTable documentation
- [LITHIUM-TAB-TABLES.md](LITHIUM-TAB-TABLES.md) - tableDef resolution process
- [LITHIUM-MGR.md](LITHIUM-MGR.md) - Manager development guide