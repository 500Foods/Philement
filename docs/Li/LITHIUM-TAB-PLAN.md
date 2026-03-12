# Lithium Tabulator Implementation Plan

This document analyses the gap between the JSON-driven column configuration system
(schemas, coltypes, table definitions) and the current runtime implementation, then
lays out a phased plan to close that gap.

---

## Current State Summary

### What Exists (JSON Configuration Layer) ✅

The configuration layer is **complete and well-designed**:

| File | Status | Notes |
|------|--------|-------|
| `config/tabulator/coltypes.json` | ✅ Complete | 21 column types with full formatting, editing, sorting, blank/zero rules |
| `config/tabulator/coltypes-schema.json` | ✅ Complete | JSON Schema validation for coltypes |
| `config/tabulator/tabledef-schema.json` | ✅ Complete | JSON Schema for per-table definitions |
| `config/tabulator/queries/query-manager.json` | ✅ Complete | 14 columns defined for Query Manager |
| `scripts/validate-tabulator-config.js` | ✅ Complete | Build-time validation with cross-reference checks |

### What Exists (Runtime Implementation) ⚠️ Partially Done

| Component | Status | Notes |
|-----------|--------|-------|
| `src/init/tabulator-init.js` | ⚠️ Legacy | Generic utility class — **not used** by any manager. Does not load JSON configs. |
| `src/managers/queries/queries.js` | ⚠️ Hardcoded | Creates Tabulator directly with 2 hardcoded columns (Ref, Name) |
| Navigator (Control/Move/Manage/Search) | ✅ UI done | Buttons rendered and wired, but most actions are stubs (TODO comments) |
| Column chooser | ✅ Working | Uses `_discoverColumns()` from API data, not from JSON config |
| Column filters (header filters) | ✅ Working | Toggle via Table Options menu, with custom clear buttons |
| Row selection + keyboard nav | ✅ Working | Arrow keys, Page Up/Down, Home/End, localStorage persistence |

---

## Gap Analysis

### 🔴 Critical Gap: JSON Configs Are Not Loaded at Runtime

The core issue: **none of the JSON configuration files are fetched or used at runtime**.
The entire coltype → column resolution pipeline described in LITHIUM-TAB.md does not exist.

### Specific Gaps

| # | Gap | JSON Property | Current Code |
|---|-----|---------------|--------------|
| 1 | **No config loader** | `coltypes.json`, `query-manager.json` | Never fetched |
| 2 | **No column resolver** | coltype defaults → column overrides → final Tabulator def | Hardcoded 2-column array |
| 3 | **No blank/zero formatter** | `blank`, `zero` per coltype | Not implemented |
| 4 | **Table-level props ignored** | `layout`, `selectableRows`, `initialSort`, etc. | Hardcoded in JS |
| 5 | **Editable/readonly logic** | `editable`, `calculated`, `readonly` | Not implemented |
| 6 | **Header filters from config** | `filter: true/false` per column | Hardcoded on 2 columns |
| 7 | **Sort from config** | `sort: true/false` per column | Hardcoded on 2 columns |
| 8 | **Visibility from config** | `visible: true/false` per column | Hardcoded (2 visible) |
| 9 | **CSS classes from coltype** | `cssClass` per coltype | Not applied |
| 10 | **Bottom calc from config** | `bottomCalc`, `bottomCalcFormatter` | Hardcoded on Ref only |
| 11 | **Lookup column resolver** | `lookupRef`, `formatter: "lookup"` | Not implemented |
| 12 | **Query refs from config** | `queryRef: 25`, `searchQueryRef: 32` | Hardcoded in `loadQueries()` |
| 13 | **No `LithiumTable` class** | Described in LITHIUM-TAB.md | Does not exist |
| 14 | **`tabulator-init.js` unused** | Generic utility | Not imported anywhere in production |

### What Works Well (Keep As-Is)

- Navigator UI layout and button groups (Control, Move, Manage, Search)
- Popup menu system (Table Options, Export, Import)
- Column chooser popup with checkbox list
- Custom header-filter editor with clear (×) button
- Selector column with ▸ / I-cursor indicators
- Custom dual-arrow sort indicator (▲/▼)
- Keyboard navigation (arrows, PageUp/Down, Home/End)
- Row selection persistence (localStorage)
- CSS styling for all Tabulator elements

---

## Implementation Plan

### Phase 1: Column Resolution Engine (The Core)

Create `src/core/lithium-table.js` — the reusable `LithiumTable` class.

#### 1a. Config Loader

```javascript
// Fetch and cache coltypes.json (singleton — loaded once)
static async loadColtypes()

// Fetch a table definition by path (e.g., 'queries/query-manager')
static async loadTableDef(tablePath)
```

**Caching strategy:** `coltypes.json` is loaded once and cached in a module-level
variable. Table definitions are cached per-path in a `Map`.

#### 1b. Column Resolver

```javascript
// Merge coltype defaults + column overrides → Tabulator column definition
static resolveColumn(colDef, coltypes)

// Build the full columns array from a table definition
static resolveColumns(tableDef, coltypes)
```

The resolution order (per LITHIUM-TAB.md §Resolution Order):

1. Start with **coltype defaults** from `coltypes.json`
2. Apply **column-level overrides** from the tabledef's `overrides` object
3. Map to Tabulator property names:
   - `align` → `hozAlign`
   - `vertAlign` → `vertAlign`
   - `display` → `title`
   - `field` → `field`
   - `visible` → `visible`
   - `sort` → `headerSort`
   - `filter` → `headerFilter` (if true, attach the filter editor)
   - `editable` + `readonly` + `calculated` → `editor` (or `false`)
   - `cssClass` → `cssClass`
   - `bottomCalc` / `bottomCalcFormatter` / `bottomCalcFormatterParams`
   - `width` / `minWidth`

#### 1c. Blank/Zero Formatter Wrapper

```javascript
// Wraps any Tabulator formatter to handle blank and zero values
static wrapFormatter(originalFormatter, formatterParams, blankValue, zeroValue)
```

Before calling the original formatter:

- If value is `null`, `undefined`, or `""` → return `blankValue`
- If value is `0` and `zeroValue` is not `null` → return `zeroValue`
- Otherwise → call the original formatter

#### 1d. Table-Level Property Mapping

Map tabledef top-level properties to Tabulator constructor options:

| tabledef | Tabulator |
|----------|-----------|
| `layout` | `layout` |
| `responsiveLayout` | `responsiveLayout` |
| `selectableRows` | `selectableRows` |
| `resizableColumns` | `resizableColumns` |
| `initialSort` | `initialSort` |
| `groupBy` | `groupBy` |

---

### Phase 2: Refactor Query Manager to Use `LithiumTable`

#### 2a. Replace Hardcoded Column Definitions

In `queries.js` → `initTable()`:

**Before (current):**

```javascript
const dataColumns = [
  { title: "Ref", field: "query_ref", width: 80, ... },
  { title: "Name", field: "name", ... },
];
this.table = new Tabulator(this.elements.tableContainer, {
  layout: "fitColumns",
  ...
  columns: [selectorColumn, ...dataColumns],
});
```

**After:**

```javascript
// Load config
const tableDef = await LithiumTable.loadTableDef('queries/query-manager');
const coltypes = await LithiumTable.loadColtypes();

// Resolve columns
const dataColumns = LithiumTable.resolveColumns(tableDef, coltypes, {
  filterEditor: this._createFilterEditor.bind(this),
});

// Build Tabulator options from tabledef
const tableOptions = LithiumTable.resolveTableOptions(tableDef);

this.table = new Tabulator(this.elements.tableContainer, {
  ...tableOptions,
  columns: [selectorColumn, ...dataColumns],
});
```

#### 2b. Use Config Query Refs

Replace hardcoded query references:

```javascript
// Before
const queryRef = searchTerm ? 32 : 25;

// After
const queryRef = searchTerm
  ? this.tableDef.searchQueryRef
  : this.tableDef.queryRef;
```

#### 2c. Remove `_discoverColumns()`

Once all columns come from the JSON config, the `_discoverColumns()` method
becomes unnecessary. The column chooser will show all columns defined in the
tabledef (including hidden ones), not just those discovered at runtime.

If there are unexpected fields in the API response that aren't in the config,
they can be silently ignored or optionally added as hidden/untyped columns.

---

### Phase 3: Lookup Column Support

#### 3a. Lookup Data Loader

```javascript
// Fetch lookup table data (e.g., "a27" → [{id: 1, label: "Active"}, ...])
static async loadLookup(lookupRef)
```

Lookup data comes from the Hydrogen API. Cache by `lookupRef`.

#### 3b. Lookup Formatter

When a column has `coltype: "lookup"`, the formatter replaces the stored integer
ID with the human-readable label from the lookup table.

#### 3c. Lookup Editor

When editing a lookup column, populate the Tabulator `list` editor with values
from the lookup table.

---

### Phase 4: Navigator Functionality

Wire up the remaining navigator stubs:

| Button | Action | Priority |
|--------|--------|----------|
| Refresh | ✅ Already working | — |
| Search | ⚠️ Partially working (uses hardcoded ref 32) | High |
| Print | `table.print()` | Medium |
| Export PDF | `table.download("pdf", ...)` | Medium |
| Export CSV | `table.download("csv", ...)` | Medium |
| Export TXT | `table.download("csv", ...)` with `.txt` extension | Low |
| Export XLS | `table.download("xlsx", ...)` (needs SheetJS) | Low |
| Import CSV | File picker + `table.import("csv")` | Low |
| Add | `table.addRow({})` → new empty row at top | Medium |
| Duplicate | Copy selected row data → `table.addRow(copiedData)` | Medium |
| Edit | Enter inline edit mode on selected row | Medium |
| Save | Collect changed data → API call | High |
| Cancel | `table.undo()` or re-fetch | Medium |
| Delete | Confirm → API delete → `table.deleteRow()` | Medium |
| Email | Build mailto: with table data summary | Low |
| Column Filters | ✅ Already working | — |

---

### Phase 5: Reusability — Extract into Generic Component

Once the Query Manager is fully using `LithiumTable`, extract the component so any
future manager can spin up a table with:

```javascript
import { LithiumTable } from '../../core/lithium-table.js';

const myTable = new LithiumTable('my-container', {
  tableDefPath: 'queries/user-manager',
  filterEditor: this._createFilterEditor.bind(this),
  onRowSelected: (data) => this.loadDetails(data),
  onDataChanged: (changes) => this.saveChanges(changes),
});
await myTable.init();
```

This replaces `tabulator-init.js` entirely (which can be deprecated/removed).

---

## File Changes Summary

### New Files

| File | Purpose |
|------|---------|
| `src/core/lithium-table.js` | Reusable LithiumTable class — config loader, resolver, factory |

### Modified Files

| File | Changes |
|------|---------|
| `src/managers/queries/queries.js` | Replace hardcoded columns with `LithiumTable.resolveColumns()` |
| `src/init/tabulator-init.js` | Deprecate or remove (replaced by `lithium-table.js`) |

### Unchanged Files

| File | Notes |
|------|-------|
| `config/tabulator/coltypes.json` | No changes needed |
| `config/tabulator/coltypes-schema.json` | No changes needed |
| `config/tabulator/tabledef-schema.json` | No changes needed |
| `config/tabulator/queries/query-manager.json` | May add `detailQueryRef: 27` |
| `scripts/validate-tabulator-config.js` | No changes needed |

---

## Execution Order

| Step | Phase | Description | Est. Complexity |
|------|-------|-------------|-----------------|
| 1 | 1a | Config loader (fetch + cache coltypes/tabledef) | Small |
| 2 | 1b | Column resolver (coltype → Tabulator column def) | Medium |
| 3 | 1c | Blank/zero formatter wrapper | Small |
| 4 | 1d | Table-level property mapping | Small |
| 5 | 2a | Refactor `queries.js` to use resolved columns | Medium |
| 6 | 2b | Use config query refs instead of hardcoded | Small |
| 7 | 2c | Remove/replace `_discoverColumns()` | Small |
| 8 | 3 | Lookup column support | Medium |
| 9 | 4 | Navigator button implementations | Medium |
| 10 | 5 | Extract to generic `LithiumTable` constructor | Medium |

Steps 1–7 are the core work. Steps 8–10 can follow in later iterations.

---

## Testing Strategy

Per LITHIUM-TOC.md: **do not run the dev server** — validate through:

- `npm run build` — ensure no import/syntax errors
- `npm run validate:tabulator` — ensure JSON configs remain valid
- Manual review of resolved column definitions (log output during dev)
- Unit tests for `resolveColumn()` and `wrapFormatter()` (Vitest)

---

## Notes

- The existing navigator UI, CSS, popup system, column chooser, and keyboard
  navigation are all solid and should be preserved through this refactor.
- The `_createFilterEditor()` method produces excellent custom filter inputs
  and should continue to be used — `LithiumTable.resolveColumns()` should accept
  a `filterEditor` callback that gets applied to columns where `filter: true`.
- The selector column (row indicator + column chooser header) is Query Manager
  specific and should remain in `queries.js`, prepended to the resolved columns.
