# Lithium Column Manager

The Column Manager provides a user interface for managing table columns at runtime. It allows users to reorder columns, toggle visibility, and edit column properties without modifying the underlying table definition.

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Column Manager UI](#column-manager-ui)
4. [Coltypes Reference](#coltypes-reference)
5. [Implementation Details](#implementation-details)
6. [API Reference](#api-reference)

## Overview

The Column Manager is a popup interface that displays all columns from a parent table and allows users to:

- **Reorder columns** via drag-and-drop
- **Toggle visibility** (show/hide columns)
- **Edit properties** like title, format, alignment, width, summary
- **Group columns** by category
- **Save changes** to apply to the parent table

Changes are tracked but not applied until the user clicks "Save". The "Cancel" button discards all pending changes.

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
4. **Save**: Apply all changes to parent table via remove/add
5. **Cancel**: Discard changes, reload original data

## Column Manager UI

### Columns Displayed

| Column | Type | Editable | Description |
|--------|------|----------|-------------|
| Order | integer | No | Display order (1, 2, 3...) |
| Drag Handle | string | No | Drag handle (⠿) for reordering |
| Visible | boolean | Yes | Show/hide column |
| Field Name | string | No | Database field name |
| Column Name | string | Yes | Display title |
| Format | lookupFixed | Yes | Data type (string, integer, etc.) |
| Summary | lookupFixed | Yes | Bottom calculation (count, sum, etc.) |
| Alignment | lookupFixed | Yes | Horizontal alignment |
| Category | stringList | Yes | Grouping category |
| Width | integer | Yes | Column width in pixels |

### Grouping

Columns are grouped by the "Category" field. Default category is "Default" if not specified.

### Row Reordering

Drag-and-drop is enabled via `movableRows: true`. The `rowMoved` event updates the order tracking.

## Coltypes Reference

### Standard Coltypes

These coltypes are defined in the coltypes system and used throughout Lithium tables.

### Dropdown Coltypes (New)

These coltypes provide dropdown/select functionality with different data sources:

#### `lookup`

Foreign key reference to a lookup table. Displays the label, stores the ID.

```json
{
  "coltype": "lookup",
  "lookupRef": "27",
  "formatter": "lookup",
  "editor": "list",
  "editorParams": {
    "valuesLookup": "27"
  }
}
```

**Characteristics:**
- Dropdown populated from database lookup table
- Shows text label in cell and dropdown
- Stores integer ID
- Supports autocomplete

#### `lookupFixed`

Fixed list of options defined in the coltype itself. No database lookup required.

```json
{
  "coltype": "lookupFixed",
  "editor": "select",
  "editorParams": {
    "values": {
      "left": "Left",
      "center": "Center",
      "right": "Right"
    }
  },
  "formatter": "lookup",
  "formatterParams": {
    "lookup": {
      "left": "Left",
      "center": "Center",
      "right": "Right"
    }
  }
}
```

**Characteristics:**
- Dropdown values defined in column definition
- Shows text label in cell and dropdown
- Stores the key value (usually string)
- Good for small, fixed lists (alignment, format, summary)

**Used in Column Manager for:**
- Format column (string, integer, decimal, etc.)
- Summary column (none, count, sum, avg, min, max)
- Alignment column (left, center, right)

#### `stringList`

Editable text lookup where values come from another column in the table. Supports creating new values.

```json
{
  "coltype": "stringList",
  "sourceColumn": "category",
  "editor": "list",
  "editorParams": {
    "autocomplete": true,
    "freetext": true,
    "valuesLookup": "column"
  }
}
```

**Characteristics:**
- Dropdown shows unique values from specified column
- Allows typing new values
- Stores text value
- Good for category/tag fields

**Used in Column Manager for:**
- Category column (groups columns by category)

#### `lookupIcon`

Like `lookup` but displays an icon in the cell, shows icons only in dropdown.

```json
{
  "coltype": "lookupIcon",
  "lookupRef": "42",
  "formatter": "icon",
  "editor": "list",
  "editorParams": {
    "valuesLookup": "42",
    "itemFormatter": "iconOnly"
  }
}
```

**Characteristics:**
- Shows icon in cell
- Shows icons only (no text) in dropdown
- Stores integer ID
- Good for status indicators

#### `lookupIconText`

Like `lookup` but displays icon + name in cell and dropdown.

```json
{
  "coltype": "lookupIconText",
  "lookupRef": "42",
  "formatter": "iconText",
  "editor": "list",
  "editorParams": {
    "valuesLookup": "42",
    "itemFormatter": "iconWithText"
  }
}
```

**Characteristics:**
- Shows icon + text label in cell
- Shows icon + text label in dropdown
- Stores integer ID
- Good for priority levels, types with icons

#### `lookupIconList`

Like `lookup` but displays only icon in cell, icon + name in dropdown.

```json
{
  "coltype": "lookupIconList",
  "lookupRef": "42",
  "formatter": "icon",
  "editor": "list",
  "editorParams": {
    "valuesLookup": "42",
    "itemFormatter": "iconWithText"
  }
}
```

**Characteristics:**
- Shows icon only in cell (compact)
- Shows icon + text label in dropdown
- Stores integer ID
- Good for compact status displays

## Implementation Details

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

1. Iterate through `pendingChanges` and apply each to parent table
2. If `_pendingReorder` exists, reorder columns
3. Clear dirty state and close

```javascript
async applyAllChangesToParent() {
  // Apply individual column changes
  for (const [field, changes] of this.pendingChanges) {
    await this.updateParentColumn(field, changes);
  }

  // Apply reorder if pending
  if (this._pendingReorder) {
    await this.reorderParentColumns(this._pendingReorder);
  }

  this.clearDirty();
}
```

### Column Update Process

Tabulator requires remove/add for proper column updates:

```javascript
async updateParentColumn(field, changes) {
  const column = this.parentTable.table.getColumn(field);
  const def = column.getDefinition();
  const updatedDef = { ...def, ...changes };

  // Get current position
  const allColumns = this.parentTable.table.getColumns();
  const columnIndex = allColumns.findIndex(col => col.getField() === field);

  // Remove old column
  column.delete();

  // Add updated column at same position
  this.parentTable.table.addColumn(updatedDef, false, columnIndex);
}
```

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
- [LITHIUM-MGR.md](LITHIUM-MGR.md) - Manager development guide
