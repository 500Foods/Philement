# Lithium Tabulator Component

This document describes the standard Tabulator component used throughout Lithium, including the integrated "Navigator" block and the JSON-driven column configuration system.

---

## Overview

Tabulator is the primary data grid library used in Lithium. While powerful, it requires significant configuration to achieve a consistent and usable state across different managers. To address this, we are developing a standardized, reusable component that combines a Tabulator instance with a custom "Navigator" control bar.

This component will be modular, accepting parameters such as JSON data, event callbacks for updates, and configuration options.

---

## Column Configuration System

Lithium uses a two-layer JSON configuration system to define how data is displayed in Tabulator tables. This eliminates hardcoded column definitions and allows reuse across dozens of tables.

### Layer 1: Column Types (`coltypes.json`)

**File:** `config/tabulator/coltypes.json`

Defines how each **data type** should be displayed and edited. A coltype controls:

| Property | Purpose |
|----------|---------|
| `align` | Horizontal text alignment (`left`, `center`, `right`) |
| `vertAlign` | Vertical alignment (`top`, `middle`, `bottom`) |
| `formatter` | Tabulator formatter (e.g., `number`, `plaintext`, `tickCross`, `datetime`) |
| `formatterParams` | Formatter configuration (thousands separator, date format, etc.) |
| `editor` | Tabulator editor (e.g., `number`, `input`, `list`, `tickCross`) |
| `editorParams` | Editor configuration (min/max values, autocomplete, etc.) |
| `sorter` | Sort comparison function (`number`, `alphanum`, `date`, `boolean`) |
| `blank` | Display value when cell is null/undefined/empty |
| `zero` | Display value when cell is zero |
| `bottomCalc` | Footer calculation (`sum`, `avg`, `count`, `min`, `max`, or `null`) |
| `cssClass` | CSS class applied to cells of this type |
| `width` / `minWidth` | Column sizing defaults |

**Available coltypes:**

| Coltype | Description | Align | Editor |
|---------|-------------|-------|--------|
| `integer` | Whole numbers (IDs, counts) | right | `number` |
| `decimal` | Floating-point (prices, rates) | right | `number` |
| `currency` | Monetary values with symbol | right | `number` |
| `percent` | Percentage values (0–100) | right | `number` |
| `string` | General text | left | `input` |
| `text` | Long/multiline content | left | `textarea` |
| `html` | Rich HTML content | left | `textarea` |
| `boolean` | True/false toggle | center | `tickCross` |
| `date` | Date only | center | `date` |
| `datetime` | Date + time | center | `datetime-local` |
| `time` | Time only | center | `time` |
| `lookup` | Foreign key → lookup table | left | `list` |
| `enum` | Fixed set of values | left | `list` |
| `email` | Clickable mailto link | left | `input` |
| `url` | Clickable hyperlink | left | `input` |
| `image` | Thumbnail from URL | center | – |
| `color` | Color swatch (hex) | center | `input` |
| `progress` | Progress bar (0–100) | left | `number` |
| `star` | Star rating (0–5) | center | `star` |
| `rownum` | Auto-incrementing row number | right | – |
| `json` | JSON object (truncated display) | left | `textarea` |

### Layer 2: Table Definitions (`tabledef`)

**Directory:** `config/tabulator/queries/` (one file per table/query)

Defines how a specific query's result set maps to Tabulator columns. Each column references a coltype and adds query-specific settings:

| Property | Purpose |
|----------|---------|
| `display` | Column header title |
| `field` | JSON field name from API response |
| `coltype` | Reference to coltype in `coltypes.json` |
| `visible` | Whether column shows by default (column chooser can toggle) |
| `sort` | Whether column is sortable |
| `filter` | Whether column has a header filter |
| `group` | Whether column can be used for row grouping |
| `editable` | Whether column is editable (subject to table `readonly` and user permissions) |
| `calculated` | Whether value is server-computed (always non-editable) |
| `primaryKey` | Whether field is the primary key (used for API operations) |
| `lookupRef` | For lookup columns: the lookup table reference (e.g., `a27`) |
| `overrides` | Any coltype property can be overridden per-column |

### Resolution Order

When building a Tabulator column definition:

1. Start with the **coltype** defaults from `coltypes.json`
2. Apply **column-level overrides** from the tabledef
3. Apply **runtime** overrides (Navigator size buttons, user preferences)

### Example: How `query_ref` Resolves

```pseudocoe
coltypes.json → integer:
  align: "right", formatter: "number", bottomCalc: "sum"

query-manager.json → query_ref:
  coltype: "integer", display: "Ref", visible: true
  overrides: { width: 80, bottomCalc: "count" }

Final Tabulator column:
  title: "Ref", field: "query_ref", hozAlign: "right",
  formatter: "number", width: 80, bottomCalc: "count"
```

### JSON Schema Validation

Both file formats have corresponding JSON schemas:

- `config/tabulator/coltypes-schema.json` — validates `coltypes.json`
- `config/tabulator/tabledef-schema.json` — validates each table definition

---

## The Navigator Block

The Navigator is a control bar positioned below the Tabulator grid. It provides standard functions for interacting with the table data, especially for read-write tables where rows or values can be edited.

### Navigator Controls

The Navigator consists of four distinct control groups:

#### 1. Control Block

Utility actions for the table:

- **Refresh** — Reload data from the source
- **Menu** — Table options popup (column filters, expand/collapse all)
- **Print** — Print the table
- **Email** — Email table data
- **Export** — Download as PDF, CSV, TXT, or XLS
- **Import** — Upload from CSV, TXT, or XLS

#### 2. Move Block

Record navigation buttons:

- **First** (`|<`)
- **Previous Page** (`<<`)
- **Previous Record** (`<`)
- **Next Record** (`>`)
- **Next Page** (`>>`)
- **Last** (`>|`)

#### 3. Manage Block

Data modification buttons:

- **Add** — Create a new row
- **Duplicate** — Copy the selected row
- **Edit** — Enter edit mode for the selected row
- **Save** — Commit changes
- **Cancel** — Revert unsaved changes
- **Delete** — Remove the selected row(s)

#### 4. Search Block

- Magnifying-glass icon button
- Text input field for searching table contents
- Clear (×) button

---

## Component Architecture

The goal is to create a reusable class or module that encapsulates both the Tabulator initialization and the Navigator UI.

### Proposed Structure

```javascript
// Conceptual example
class LithiumTable {
  constructor(containerId, options) {
    this.container = document.getElementById(containerId);
    this.options = options; // Data, columns, callbacks, etc.
    
    this.initUI();
    this.initTabulator();
    this.bindEvents();
  }

  initUI() {
    // Create the DOM structure for the table container and the Navigator block
  }

  initTabulator() {
    // Initialize Tabulator with standard Lithium defaults + custom options
  }

  bindEvents() {
    // Wire up Navigator buttons to Tabulator API methods
    // (e.g., Add button -> table.addRow(), Save button -> emit save event)
  }
}
```

### Key Features

- **Modularity:** Easily instantiated in any manager.
- **Consistency:** Ensures all tables look and behave the same way.
- **Read/Write Support:** Handles both display-only and editable grids.
- **Event-Driven:** Emits standard events (e.g., `rowSelected`, `dataChanged`, `saveRequested`) for the parent manager to handle.
- **JSON-Configured:** Column definitions loaded from `config/tabulator/` files — no hardcoded columns in JavaScript.

---

## File Layout

```structure
config/tabulator/
├── coltypes.json              # Column type definitions (global)
├── coltypes-schema.json       # JSON Schema for coltypes.json
├── tabledef-schema.json       # JSON Schema for table definitions
└── queries/
    └── query-manager.json     # Table definition for Query Manager
```

Future table definitions go in the `queries/` directory (or a parallel directory for non-query tables):

```structure
config/tabulator/
├── queries/
│   ├── query-manager.json
│   ├── lookup-manager.json     # (future)
│   └── user-manager.json       # (future)
└── managers/
    └── style-manager.json      # (future)
```

---

## Implementation Notes

- The Navigator UI should be styled using standard Lithium CSS variables and icons (Font Awesome).
- Consider how the Navigator adapts to smaller screens (responsive design).
- The component should handle loading states (spinners) during data fetch or save operations.
- Column configuration is loaded at runtime via `fetch()` — supports hot-reconfiguration without rebuild.
- The `blank` and `zero` coltype properties allow custom null/zero handling via a Tabulator `formatter` wrapper.
- Lookup columns (`coltype: "lookup"`) require a runtime lookup resolver that maps integer IDs to display labels.
