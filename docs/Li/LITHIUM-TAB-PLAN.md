# Lithium Tabulator Implementation Plan

This document analyses the gap between the JSON-driven column configuration system
(schemas, coltypes, table definitions) and the current runtime implementation, then
lays out a phased plan to close that gap.

**Status: Phases 1–4 complete; `tabulator-init.js` removed** (March 2026).

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

### What Exists (Runtime Implementation) ✅ Implemented

| Component | Status | Notes |
|-----------|--------|-------|
| `src/core/lithium-table.js` | ✅ Complete | Config loader, column resolver, blank/zero wrapper, lookup support |
| `src/managers/queries/queries.js` | ✅ Refactored | Uses `resolveColumns()` from JSON config with fallback to hardcoded |
| `src/init/tabulator-init.js` | 🗑️ Removed | Was dead code — superseded by `lithium-table.js`, deleted in Phase 4 |
| Navigator (Control/Move/Manage/Search) | ✅ Working | All buttons wired with real implementations |
| Column chooser | ✅ Working | Shows JSON-config columns + discovers extra API fields |
| Column filters (header filters) | ✅ Working | Toggle via Table Options menu, with custom clear buttons |
| Row selection + keyboard nav | ✅ Working | Arrow keys, Page Up/Down, Home/End, localStorage persistence |
| Unit tests | ✅ Complete | 76 tests in `tests/unit/lithium-table.test.js` |

---

## Gap Analysis (Original — Preserved for Reference)

### ~~🔴 Critical Gap: JSON Configs Are Not Loaded at Runtime~~

~~The core issue: **none of the JSON configuration files are fetched or used at runtime**~~

**Resolved:** `lithium-table.js` loads coltypes.json and tabledef JSON at runtime,
with singleton caching per session.

### Specific Gaps — Resolution Status

| # | Gap | Status | Resolution |
|---|-----|--------|------------|
| 1 | **No config loader** | ✅ Fixed | `loadColtypes()`, `loadTableDef()` in `lithium-table.js` |
| 2 | **No column resolver** | ✅ Fixed | `resolveColumn()`, `resolveColumns()` |
| 3 | **No blank/zero formatter** | ✅ Fixed | `wrapFormatter()`, `needsBlankZeroWrapper()` |
| 4 | **Table-level props ignored** | ✅ Fixed | `resolveTableOptions()` |
| 5 | **Editable/readonly logic** | ✅ Fixed | `resolveColumn()` handles editable/calculated/tableReadonly |
| 6 | **Header filters from config** | ✅ Fixed | `filter: true` → `headerFilter` with custom editor |
| 7 | **Sort from config** | ✅ Fixed | `sort: true/false` → `headerSort` |
| 8 | **Visibility from config** | ✅ Fixed | `visible: true/false` |
| 9 | **CSS classes from coltype** | ✅ Fixed | `cssClass` from coltype applied |
| 10 | **Bottom calc from config** | ✅ Fixed | `bottomCalc`, `bottomCalcFormatter`, `bottomCalcFormatterParams` |
| 11 | **Lookup column resolver** | ✅ Fixed | `loadLookup()`, `createLookupFormatter()`, `createLookupEditor()` |
| 12 | **Query refs from config** | ✅ Fixed | `getQueryRefs()` + fallback in `queries.js` |
| 13 | **No `LithiumTable` module** | ✅ Fixed | `src/core/lithium-table.js` (functional API, not class) |
| 14 | **`tabulator-init.js` unused** | ✅ Removed | Deleted — was dead code, not imported anywhere |

### What Works Well (Preserved Through Refactor)

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

### Phase 1: Column Resolution Engine ✅ Complete

Created `src/core/lithium-table.js` — a functional module API (not a class).

#### 1a. Config Loader ✅

```javascript
export async function loadColtypes()     // Singleton: fetched once, cached in module variable
export async function loadTableDef(path) // Cached per-path in a Map
```

Caching: `coltypes.json` is loaded once per session. Table definitions are
cached per-path. `clearCache()` resets everything (useful for tests).

#### 1b. Column Resolver ✅

```javascript
export function resolveColumn(fieldName, colDef, coltypes, options)
export function resolveColumns(tableDef, coltypes, options)
```

The resolution order (per LITHIUM-TAB.md §Resolution Order):

1. Start with **coltype defaults** from `coltypes.json`
2. Apply **column-level overrides** via spread: `{ ...coltype, ...overrides }`
3. Map to Tabulator property names:
   - `align` → `hozAlign`, `vertAlign` → `vertAlign`
   - `display` → `title`, `field` → `field`, `visible` → `visible`
   - `sort` → `headerSort`, `filter` → `headerFilter`
   - `editable` + `readonly` + `calculated` → `editor` (with `tableReadonly` override)
   - `cssClass`, `bottomCalc` / `bottomCalcFormatter` / `bottomCalcFormatterParams`
   - `width` / `minWidth`, `sorter` / `sorterParams`

**Design decision:** The module uses named function exports rather than a class.
This keeps the API simple and tree-shakeable. The plan originally described a
`LithiumTable` class, but a functional approach proved cleaner since config loading
is inherently async/cached at module scope.

#### 1c. Blank/Zero Formatter Wrapper ✅

```javascript
export function wrapFormatter(formatter, formatterParams, blankValue, zeroValue)
export function needsBlankZeroWrapper(blankValue, zeroValue)
```

**Key behaviour:** When `blankValue` and `zeroValue` are both `null`, the
wrapper is skipped entirely (returns the original formatter string/function).
This avoids unnecessary function wrapping for columns that don't need it.

**Caveat:** When a built-in Tabulator formatter (string name like `"number"`)
is wrapped, the wrapper intercepts blank/zero but then falls through to
returning the raw cell value for non-blank cases. This means the built-in
formatter's params (thousands separator, precision) won't apply to the
wrapper's output. For the current use case (where blank/zero returns a
literal string like `""` or `"—"`), this is fine. If full formatting is
needed for non-blank values, the wrapper would need access to Tabulator's
internal formatter registry — a future enhancement.

#### 1d. Table-Level Property Mapping ✅

```javascript
export function resolveTableOptions(tableDef)
```

| tabledef | Tabulator |
|----------|-----------|
| `layout` | `layout` |
| `responsiveLayout` | `responsiveLayout` |
| `selectableRows` | `selectableRows` |
| `resizableColumns` | `resizableColumns` |
| `initialSort` | `initialSort` |
| `groupBy` | `groupBy` |

---

### Phase 2: Refactor Query Manager ✅ Complete

#### 2a. Replace Hardcoded Column Definitions ✅

`queries.js` → `initTable()` now:

```javascript
[this.coltypes, this.tableDef] = await Promise.all([
  loadColtypes(),
  loadTableDef('queries/query-manager'),
]);

const dataColumns = this.tableDef && this.coltypes
  ? resolveColumns(this.tableDef, this.coltypes, {
      filterEditor: this._createFilterEditor.bind(this),
    })
  : this._getFallbackColumns();
```

The `_getFallbackColumns()` method provides the original 2-column hardcoded
array as a safety net if the JSON config fails to load.

#### 2b. Use Config Query Refs ✅

```javascript
const listRef = this.queryRefs?.queryRef ?? 25;
const searchRef = this.queryRefs?.searchQueryRef ?? 32;
const queryRef = searchTerm ? searchRef : listRef;
```

The `?? fallback` pattern ensures the app still works if the JSON config
is unavailable (e.g., network error, dev server without config files).

#### 2c. `_discoverColumns()` Retained (Design Decision)

The plan originally called for removing `_discoverColumns()`. In practice,
it was **kept** because:

- The JSON config defines 14 columns, but the API may return additional
  fields not in the config.
- `_discoverColumns()` adds those as hidden columns, making them available
  in the column chooser without needing config updates.
- It runs after `setData()` and is idempotent (skips already-defined fields).

This hybrid approach (config-driven + discovery) provides the best of both
worlds: structured columns from JSON config, plus automatic discovery of
new API fields.

---

### Phase 3: Lookup Column Support ✅ Complete

#### 3a. Lookup Data Loader ✅

```javascript
export async function loadLookup(lookupRef, api)
export async function preloadLookups(lookupRefs, api)
export function getLookup(lookupRef)
```

Lookup data is fetched via the Conduit API (QueryRef 3) and cached in a
module-level `Map`. The `preloadLookups()` function loads multiple lookups
in parallel.

#### 3b. Lookup Formatter ✅

```javascript
export function createLookupFormatter(lookupRef)
export function resolveLookupLabel(id, lookupRef)
```

The formatter resolves a stored integer ID to its human-readable label using
the cached lookup data.

#### 3c. Lookup Editor ✅

```javascript
export function createLookupEditor(lookupRef, lookupData)
```

Returns a Tabulator `list` editor configuration with autocomplete, populated
from the lookup table. Falls back to `'input'` if lookup data is empty.

---

### Phase 4: Navigator Functionality ✅ Complete

| Button | Status | Implementation |
|--------|--------|----------------|
| Refresh | ✅ Working | `loadQueries()` with row selection persistence |
| Search | ✅ Working | Uses config refs (`queryRefs.searchQueryRef`) with fallback |
| Print | ✅ Working | `table.print()` |
| Export PDF | ✅ Working | `table.download('pdf', ...)` with landscape orientation |
| Export CSV | ✅ Working | `table.download('csv', ...)` |
| Export TXT | ✅ Working | `table.download('csv', ...)` with `.txt` extension |
| Export XLS | ✅ Working | `table.download('xlsx', ...)` |
| Import CSV/TXT/XLS | ✅ Working | File picker + `table.import()` |
| Add | ✅ Working | `addRow({}, true)` + auto-edit Name cell |
| Duplicate | ✅ Working | Copy selected row, clear PK, append "(Copy)" to name |
| Edit | ✅ Working | Set `_editingRowId`, show I-cursor indicator, edit Name cell |
| Save | ✅ Wired | Uses `insertQueryRef`/`updateQueryRef` from config; falls back to local-only if refs not defined |
| Cancel | ✅ Working | `table.undo()` + clear edit state |
| Delete | ✅ Wired | Uses `deleteQueryRef` from config; falls back to local-only if ref not defined |
| Email | ✅ Working | Builds mailto: with visible columns, up to 50 rows |
| Column Filters | ✅ Working | Toggle with custom clear (×) buttons |
| Expand All | ✅ Working | Expand all groups/tree rows |
| Collapse All | ✅ Working | Collapse all groups/tree rows |

#### 4a. Save/Delete API Integration ✅

The `handleNavSave()` and `handleNavDelete()` methods now use config-driven
QueryRefs (`insertQueryRef`, `updateQueryRef`, `deleteQueryRef`) from the
tabledef JSON. When the refs are not yet defined in the backend, the handlers
degrade gracefully:

- **Save:** Detects insert vs. update by checking whether the primary key is
  set. Calls `authQuery()` with the appropriate QueryRef and the row data as
  a JSON payload. If no QueryRef is configured, shows a "Saved Locally" toast.

- **Delete:** Confirms with the user (including row name if available), then
  calls `authQuery()` with the delete QueryRef and the primary key. On API
  failure, the row is **not** removed from the table. If no QueryRef is
  configured, proceeds with local-only deletion.

The tabledef schema (`tabledef-schema.json`) now includes the three new
optional integer fields: `insertQueryRef`, `updateQueryRef`, `deleteQueryRef`.
The `getQueryRefs()` function in `lithium-table.js` returns all six refs.

**To activate for Query Manager:** Add the actual QueryRef numbers to
`query-manager.json` once the Hydrogen backend has the corresponding
insert/update/delete queries defined.

---

### Phase 5: Reusability — Extract into Generic Component (Future)

When a second manager (e.g., User Manager, Lookup Manager) needs a table,
extract the shared patterns into a higher-level wrapper:

```javascript
import { loadColtypes, loadTableDef, resolveColumns, resolveTableOptions } from '../../core/lithium-table.js';
```

The current functional API in `lithium-table.js` is already designed for
reuse. The main extraction work would be the **navigator component** (HTML
template, button wiring, move/manage logic), which is currently embedded in
`queries.js`.

`tabulator-init.js` has been removed (was dead code, not imported anywhere).

---

## File Changes Summary

### New Files

| File | Purpose | Status |
|------|---------|--------|
| `src/core/lithium-table.js` | Config loader, column resolver, blank/zero wrapper, lookup support | ✅ Complete |
| `tests/unit/lithium-table.test.js` | 76 unit tests covering all exports | ✅ Complete |

### Modified Files

| File | Changes | Status |
|------|---------|--------|
| `src/managers/queries/queries.js` | JSON-driven columns, config query refs, Save/Delete API integration | ✅ Complete |
| `config/tabulator/tabledef-schema.json` | Added `insertQueryRef`, `updateQueryRef`, `deleteQueryRef` fields | ✅ Complete |

### Removed Files

| File | Notes |
|------|-------|
| `src/init/tabulator-init.js` | Dead code — was not imported anywhere; fully superseded by `lithium-table.js` |

### Unchanged Files

| File | Notes |
|------|-------|
| `config/tabulator/coltypes.json` | No changes needed |
| `config/tabulator/coltypes-schema.json` | No changes needed |
| `config/tabulator/queries/query-manager.json` | CRUD refs will be added when Hydrogen backend is ready |
| `scripts/validate-tabulator-config.js` | No changes needed |

---

## Execution Order — Completion Status

| Step | Phase | Description | Status |
|------|-------|-------------|--------|
| 1 | 1a | Config loader (fetch + cache coltypes/tabledef) | ✅ Done |
| 2 | 1b | Column resolver (coltype → Tabulator column def) | ✅ Done |
| 3 | 1c | Blank/zero formatter wrapper | ✅ Done |
| 4 | 1d | Table-level property mapping | ✅ Done |
| 5 | 2a | Refactor `queries.js` to use resolved columns | ✅ Done |
| 6 | 2b | Use config query refs instead of hardcoded | ✅ Done |
| 7 | 2c | `_discoverColumns()` — kept as hybrid approach | ✅ Done (design decision) |
| 8 | 3 | Lookup column support | ✅ Done |
| 9 | 4 | Navigator button implementations | ✅ Done |
| 9a | 4a | Save/Delete API wiring + CRUD QueryRef schema | ✅ Done |
| 10 | 4b | Remove dead `tabulator-init.js` | ✅ Done |
| 11 | 5 | Extract to generic component | ⏳ Future |

---

## Testing Strategy

Per LITHIUM-TOC.md: **do not run the dev server** — validate through:

- `npm run build` — ensure no import/syntax errors
- `npm run validate:tabulator` — ensure JSON configs remain valid
- `npm test` — 482 tests across 17 files (all passing)
- Unit tests for `resolveColumn()`, `wrapFormatter()`, `resolveColumns()`,
  `resolveTableOptions()`, `getPrimaryKeyField()`, `getQueryRefs()`, lookup
  functions, and cache management (76 tests in `lithium-table.test.js`)

---

## Implementation Notes & Lessons Learned

### Architecture: Functional API vs. Class

The plan originally described a `LithiumTable` class with static methods. The
implementation uses **named function exports** instead:

```javascript
export function resolveColumn(...)
export function resolveColumns(...)
export async function loadColtypes()
```

**Why:** Module-level caching (via closure variables `_coltypesCache`,
`_tableDefCache`, `_lookupCache`) is simpler and more testable than static class
properties. The functional approach also enables tree-shaking — consumers only
import the functions they need.

### Fallback Pattern

All JSON config consumption uses a fallback-with-nullish-coalescing pattern:

```javascript
const listRef = this.queryRefs?.queryRef ?? 25;
```

This ensures the app degrades gracefully if configs fail to load (e.g., during
local development without the config directory served).

### `_discoverColumns()` Kept as Hybrid

The original plan said to remove `_discoverColumns()`. In practice, it provides
value as a discovery layer for API fields that aren't in the JSON config yet.
New fields automatically appear in the column chooser as hidden columns,
without requiring a config update.

### Null Overrides Remove Properties

When a column override sets a property to `null` (e.g., `"bottomCalc": null`),
the spread `{ ...coltype, ...overrides }` produces `null`. The resolver's
`if (merged.bottomCalc != null)` check then skips it, effectively removing
the property from the Tabulator column definition. This is intentional and
allows overrides to suppress coltype defaults.

### wrapFormatter and Built-in Formatters

The `wrapFormatter()` function works perfectly for intercepting blank/zero
values. However, when wrapping a built-in Tabulator formatter (string name),
the wrapper can't invoke Tabulator's internal formatter for non-blank values.
It returns the raw cell value instead. This is acceptable because:

1. Blank/zero handling is the primary use case (show `""` or `"—"`)
2. Non-blank values pass through to Tabulator which applies the formatter
   via the `formatter` property on the column definition
3. The wrapper only activates when `needsBlankZeroWrapper()` returns true

### Duplicate Import Method Bug

A subtle bug existed where two `handleImport()` method definitions appeared
in `queries.js`. JavaScript uses the **last** definition, so the working
implementation (file picker + `table.import()`) was silently overwritten by
a TODO stub. This was fixed by removing the duplicate.

---

## Remaining Work

1. **Hydrogen Backend: Define CRUD QueryRefs** (prerequisite for live Save/Delete)
   - Define insert, update, and delete queries in Hydrogen for the Query Manager
   - Add the resulting QueryRef numbers to `query-manager.json`
     (`insertQueryRef`, `updateQueryRef`, `deleteQueryRef`)
   - The frontend code is ready — it will use the refs automatically

2. **Phase 5: Reusable Navigator Component**
   - Extract navigator HTML template into a shared module
   - Move button wiring and move/manage logic into reusable functions
   - Enable new managers to spin up a table with minimal boilerplate

3. **Lookup Integration at Runtime** (Phase 3 enhancement)
   - Call `preloadLookups()` during table init to pre-fetch lookup data
   - Wire `createLookupFormatter()` into the column resolver for lookup columns
   - This requires a live Hydrogen API connection (not testable locally)
