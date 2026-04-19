# LithiumTable Tables

This document describes how the column type system creates the LithiumTable UI, how table definitions are structured, and the complete resolution process from raw data to rendered table.

---

## The Three Layers

The column type system has three layers:

1. **Documentation** — `docs/Li/LITHIUM-TAB-TYPES-*.md` files with human-readable descriptions
2. **JSON Schema** — `LITHIUM-TAB-TYPES.schema.json` for validation and tooling
3. **Database** — Lookup 059 in the `lookups` table stores the runtime definitions

---

## Database Schema

### Lookup 059 — Tabulator Schemas

The `lookups` table stores all column type and table definitions:

| key_idx | value_txt | Description |
|---------|-----------|-------------|
| 0 | `column-types` | All column type defaults (the "coltypes") **including the "default" stanza** |
| 1 | `query-manager` | Query Manager table definition |
| 2 | `lookups-list` | Lookups List table definition |
| 3 | `lookup-values` | Lookup Values table definition |
| ... | ... | Additional table definitions |

Each lookup entry stores its JSON definition in the `collection` column.

> **Important:** The "default" stanza in Lookup 059 Key 0 is the root of everything. All coltypes inherit from it. When resolving a column's properties, start with `default`, then overlay the specific coltype (e.g., "string", "integer") on top.

---

## The tableDef Resolution Process

When LithiumTable displays data, it builds a `tableDef` through three stages. At the completion of every stage, there is a **fully usable tableDef** — not a partial configuration. Each stage layers additional properties on top of the previous stage's result.

---

### Stage 1: Create Base from Data (Auto-Detection)

The system starts with raw data (query results) and creates a fully populated tableDef using only the data itself and the coltype defaults from Lookup 059 Key 0:

```javascript
// Input: data from query (array of objects)
// Example: [{ query_id: 1, name: "Test", status: 1 }, ...]
```

For each field in the data, the system:
1. **Detects coltype from JSON data type** — Maps JSON type to coltype:
   - `string` → coltype `"string"`
   - `number` (no decimals) → coltype `"integer"`
   - `number` (with decimals) → coltype `"decimal"`
   - `boolean` → coltype `"boolean"`
   - `null` → coltype `"string"` (treated as empty string)
2. **Loads coltype defaults** — Fetches `coltypes.default` first, then merges the detected coltype stanza (e.g., `coltypes.string`) on top
3. **Merges properties** — The "default" stanza is the root of everything; each specific coltype adds/overrides properties from that base
4. **Derives title/description** — Creates `title` from field name (e.g., `query_id` → "Query Id")
5. **Assigns columnPri** — Sets display order based on field detection order (first field = 1, second = 2, etc.)

```javascript
// Stage 1: Fully populated tableDef (usable on its own)
const stage1 = {
  table: { /* table-level defaults */ },
  columns: {
    query_id: {
      field: "query_id",
      coltype: "integer",
      title: "Query Id",                    // Derived from field name
      description: "query_id field",        // Derived from field name
      columnPri: 1,                         // First field = display order 1
      // ALL properties from coltype.default stanza:
      formatter: "plaintext",
      editor: "input",
      minWidth: 20,
      hozAlign: "left",
      vertAlign: "middle",
      headerSort: true,
      headerFilter: false,
      groupable: false,
      // THEN properties from coltype.integer (override/add):
      formatter: "number",
      formatterParams: { "thousand": ",", "precision": 0 },
      sorter: "number",
      minWidth: 40,
      hozAlign: "right",
      blank: "",
      zero: "",
      bottomCalc: "sum"
      // ... complete merged result
    },
    name: {
      field: "name",
      coltype: "string",
      title: "Name",
      description: "name field",
      columnPri: 2,                         // Second field = display order 2
      // ALL properties from default + string:
      formatter: "plaintext",
      minWidth: 80,
      headerFilter: true,
      groupable: true,
      // ... complete merged result
    },
    status: {
      field: "status",
      coltype: "integer",                   // Detected from numeric data
      title: "Status",
      columnPri: 3,                         // Third field = display order 3
      // ... complete merged result
    }
  }
};
```

**Stage 1 produces a complete, usable tableDef** that will render a functional table with sensible defaults based on the data types detected.

---

### Stage 2: Apply Table Definition (Developer Customization)

Load the table definition from Lookup 059, Keys 1+ (e.g., Key 1 = `query-manager`). The table definition contains table-level properties plus per-column properties that override the Stage 1 defaults:

```javascript
// Load table definition from database
const tableDef = lookup59.key1.collection;
// tableDef = { "title": "Query Manager", "queryRef": 25, "columns": {...} }

// Stage 2: Table definition applies per-column overrides on top of Stage 1
const stage2 = merge(stage1, tableDef.columns);
```

The table definition (from Lookup 059, Keys 1+) can override ANY property from Stage 1:

```javascript
// Example: query-manager table definition (Lookup 059, Key 1)
{
  "title": "Query Manager",
  "queryRef": 25,
  "layout": "fitColumns",
  "responsiveLayout": "collapse",
  "columns": {
    "query_id": {
      "title": "ID#",           // Override title derived in Stage 1
      "coltype": "index",
      "headerFilter": true,     // Override default (false)
      "primaryKey": true,        // Add tabledef property
      "description": "Database primary key"
    },
    "query_ref": {
      "title": "Ref",
      "coltype": "index",
      "visible": true,          // Override default (true from index)
      "width": 80,              // Add specific width
      "headerFilter": true,
      "bottomCalc": "count",    // Add footer calculation
      "bottomCalcFormatter": "number"
    },
    "name": {
      "title": "Name",
      "coltype": "string",
      "visible": true,
      "headerFilter": true,
      "editable": true          // Enable editing
    }
  }
}
```

**Stage 2 produces an enhanced, usable tableDef** tuned to the specific table's purpose based on the developer's table definition in Lookup 059.

---

### Stage 3: Apply User Template (Optional)

If the user has a saved template (their personalized view from the Column Manager), apply those overrides:

```javascript
// Template stores user customizations
const template = userSavedTemplate; // { columns: { query_ref: { width: 100, visible: false }, ... } }

const stage3 = merge(stage2, template.columns);
```

Templates typically customize:
- Column visibility
- Column width
- Column order (`columnPri`)
- Sort preferences
- Filter values

But templates can override ANY property — users can change coltypes, formatters, editors, etc. via the Column Manager UI.

**Stage 3 produces the final, personalized tableDef** reflecting the user's preferences.

---

### Result: Final tableDef

The final `tableDef` is applied to Tabulator:

```javascript
// Final merged configuration
const finalTableDef = {
  table: {
    title: "Query Manager",
    layout: "fitColumns",
    // ... table-level properties
  },
  columns: [
    // Array format for Tabulator
    { field: "query_id", title: "ID#", coltype: "index", ... },
    { field: "query_ref", title: "Ref", coltype: "index", visible: true, width: 80, ... },
    { field: "name", title: "Name", coltype: "string", visible: true, ... },
    // ...
  ]
};

// Pass to Tabulator
table.setConfig(finalTableDef);
```

---

## Property Precedence

Properties flow through stages with later stages overriding earlier ones. At every stage, the result is a **complete, usable tableDef**:

| Stage | Source | What It Provides | Result |
|-------|--------|------------------|--------|
| Stage 1 | Query data + Lookup 059 Key 0 (coltypes) | Auto-detected coltype + merged defaults | Fully usable tableDef |
| Stage 2 | Lookup 059 Keys 1+ (table definitions) | Per-column overrides from tabledef | Enhanced tableDef |
| Stage 3 | User template (Column Manager) | User preferences | Final personalized tableDef |

At ANY stage, virtually **any property** can be customized. Examples:

```javascript
// Stage 1: Auto-detected coltype can be overridden
{ field: "amount", coltype: "currency" }

// Stage 2: Table definition can override formatterParams
{ field: "amount", coltype: "decimal", formatterParams: { precision: 4 } }

// Stage 3: User can change everything via Column Manager
{ field: "amount", coltype: "progress", width: 200, visible: false }
```

### Deep Merge for Param Objects

Most properties are **replaced** (scalar/array values), but **param objects are deep-merged** across stages. This preserves coltype defaults while allowing partial overrides at each stage:

```javascript
// Stage 1 (coltype.integer defaults):
{ field: "amount", formatter: "number", formatterParams: { thousand: ",", precision: 0 } }

// Stage 2 (tableDef override - only precision changes):
{ field: "amount", formatterParams: { precision: 2 } }

// Stage 2 Result (deep merge - both properties present):
{ field: "amount", formatter: "number", formatterParams: { thousand: ",", precision: 2 } }
```

**Properties that deep-merge:**
- `formatterParams`
- `editorParams`
- `headerFilterParams`
- `sorterParams`
- `accessorParams`
- `mutatorParams`
- `bottomCalcFormatterParams`
- `downloadFormatterParams`
- `downloadCalcParams`
- `clipboardParams`

**Properties that replace (not merge):**
- Scalar values: `title`, `width`, `visible`, `coltype`, `formatter`, `editor`, etc.
- Arrays: `values`, `cssClass` (as array), etc.

This behavior ensures that changing `formatterParams.precision` in Lookup 059 Key 0 will propagate through all stages, even when templates or tableDefs specify other formatterParams properties.

---

## Table Definition Structure

Table definitions (stored in Lookup 059, Keys 1+) use this structure:

### Top-Level Table Properties

```json
{
  "table": "query-manager",
  "title": "Query Manager",
  "queryRef": 25,
  "searchQueryRef": 32,
  "detailQueryRef": 27,
  "readonly": false,
  "selectableRows": 1,
  "layout": "fitColumns",
  "responsiveLayout": "collapse",
  "resizableColumns": true,
  "persistSort": true,
  "persistFilter": true,
  "initialSort": [
    { "column": "query_ref", "dir": "asc" }
  ],
  "columns": { ... }
}
```

### Column Properties

Each column in `columns` object:

```json
{
  "field": "query_ref",
  "title": "Ref",
  "coltype": "index",
  "visible": true,
  "headerFilter": true,
  "editable": false,
  "calculated": false,
  "primaryKey": false,
  "description": "Human-readable query reference number",
  "width": 80,
  "bottomCalc": "count",
  "bottomCalcFormatter": "number",
  "bottomCalcFormatterParams": { "thousand": "" },
  "lookupRef": "28"
}
```

Required properties:
- `field` — JSON key in data
- `coltype` — Reference to type in Lookup 059 Key 0
- `title` — Column header text

Optional properties (any from coltype system):
- `visible`, `width`, `minWidth`, `hozAlign`, etc.
- `formatter`, `formatterParams`
- `editor`, `editorParams`
- `sorter`, `sorterParams`
- `headerFilter`, `headerFilterFunc`, `headerFilterPlaceholder`
- `groupable`, `groupPri`, `groupDir`
- `bottomCalc`, `bottomCalcFormatter`, `bottomCalcFormatterParams`
- `lookupRef`, `lookupStyle`, `lookupEdit`
- `primaryKey`, `calculated`, `description`
- Any other property from the coltype system

---

## Process: Creating Column Types (Migration 1153)

This process creates the column type definitions in the database (Lookup 059, Key 0).

### Step 1: Document the Types

Each column type has a dedicated markdown file:
- `LITHIUM-TAB-TYPES-DEFAULT.md` — Base default properties
- `LITHIUM-TAB-TYPES-STRING.md`, `INTEGER.md`, etc. — Individual types

### Step 2: Create JSON Schema

The JSON Schema (`LITHIUM-TAB-TYPES.schema.json`) validates the structure and enumerates valid values.

### Step 3: Generate Database JSON

The database JSON includes a "default" stanza plus all type definitions with only the properties that differ from defaults:

```json
{
  "default": {
    "title": "Column",
    "field": null,
    "visible": true,
    "hozAlign": "left",
    "formatter": "plaintext",
    "editor": "input",
    "headerSort": true,
    "headerFilter": false,
    "groupable": false,
    "minWidth": 20,
    ...
  },
  "string": {
    "description": "General text/string values",
    "minWidth": 80,
    "formatter": "plaintext",
    "headerFilter": true,
    "groupable": true
  },
  "integer": {
    "description": "Whole numbers (IDs, counts, references)",
    "hozAlign": "right",
    "minWidth": 40,
    "formatter": "number",
    "formatterParams": { "thousand": ",", "precision": 0 },
    "sorter": "number",
    "bottomCalc": "sum"
  },
  "lookup": {
    "description": "Foreign key reference to a lookup table",
    "minWidth": 80,
    "formatter": "lookup",
    "editor": "list",
    "headerFilterFunc": "like"
  },
  ...
}
```

### Step 4: Create Migration

The migration inserts the JSON into the database:

```lua
-- Migration acuranzo_1153.lua
INSERT INTO lookups (lookup_id, key_idx, collection)
VALUES (59, 0, ${JSON_INGEST_START}[===[{...column types JSON...}]===]${JSON_INGEST_END});
```

---

## Process: Creating Table Definitions (Lookup 059, Keys 1+)

These migrations create specific table configurations (Query Manager, Lookups List, etc.).

### Structure

Table definitions in the database include table-level config plus per-column definitions:

```json
{
  "table": "query-manager",
  "title": "Query Manager",
  "queryRef": 25,
  "layout": "fitColumns",
  "columns": {
    "query_ref": {
      "field": "query_ref",
      "title": "Ref",
      "coltype": "index",
      "visible": true,
      "width": 80,
      "bottomCalc": "count"
    },
    "name": {
      "field": "name",
      "title": "Name",
      "coltype": "string",
      "visible": true,
      "headerFilter": true,
      "editable": true
    }
  }
}
```

Each column definition uses `coltype` to reference the type from Lookup 059 Key 0, then overrides only the properties that differ from those defaults.

---

## Process: Creating Simple Lookups (Migrations 1053-1057)

These migrations create basic lookup tables (not table definitions).

### Lookup Header Entry

First, insert the lookup metadata (lookup_id = 0):

```lua
INSERT INTO lookups (lookup_id, key_idx, value_txt, collection)
VALUES (0, 28, 'Query Type', '{"Default": "HTMLEditor", ...}');
```

### Lookup Value Entries

Then insert individual values:

```lua
INSERT INTO lookups (lookup_id, key_idx, value_txt, sort_seq, collection)
VALUES 
  (28, 0, 'SQL - Internal', 0, '{"icon":"<fa fa-code></fa>"}'),
  (28, 1, 'SQL - System', 1, '{"icon":"<fa fa-code></fa>"}');
```

The `collection` field stores:
- Icon definitions for display
- Editor configuration (which editors are available)

---

## Lookup Reference

### Lookup 059 — Tabulator Schemas

| Key | Name | Purpose |
|-----|------|---------|
| 0 | column-types | All column type defaults (with "default" stanza) |
| 1 | query-manager | Query Manager table config |
| 2 | lookups-list | Lookups list table config |
| 3 | lookup-values | Lookup values table config |

### Common Lookup Editor Configurations

```json
{
  "Default": "HTMLEditor",
  "CSSEditor": false,
  "HTMLEditor": true,
  "JSONEditor": false,
  "LookupEditor": false
}
```

| Editor | Description |
|--------|-------------|
| HTMLEditor | Rich text editor (default for most lookups) |
| CSSEditor | CSS code editor |
| JSONEditor | JSON code editor |
| LookupEditor | Dropdown for lookup values |

---

## Related Documentation

- [LITHIUM-TAB.md](LITHIUM-TAB.md) — LithiumTable component
- [LITHIUM-TAB-TYPES.md](LITHIUM-TAB-TYPES.md) — Column types overview
- [LITHIUM-TAB-TYPES-DEFAULT.md](LITHIUM-TAB-TYPES-DEFAULT.md) — Default type definition
- [LITHIUM-TAB-TYPES.schema.json](LITHIUM-TAB-TYPES.schema.json) — JSON Schema
- [LITHIUM-LUT.md](LITHIUM-LUT.md) — Lookup tables system