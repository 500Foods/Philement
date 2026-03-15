# Lithium Tabulator Implementation Plan

This document analyses the gap between the JSON-driven column configuration system
(schemas, coltypes, table definitions) and the current runtime implementation, then
lays out a phased plan to close that gap.

**Status: Phases 1–4 complete; Phase 4b (lookup wiring + detailQueryRef + persistSort/Filter) complete; March 2026 edit-mode follow-up applied for row-scoped editors, consistent row selection, header-only resize handles, non-selectable footer calcs, same-row edit handoff without detail-query churn, stronger queued cell-editor activation, and row-change popup cleanup; `tabulator-init.js` removed** (March 2026).

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
| `persistSort` | `persistSort` |
| `persistFilter` | `persistFilter` |

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

#### 3d. Resolver Integration ✅

`resolveColumn()` now detects `lookupRef` on a column definition and, if the
lookup data is in cache, automatically wires:

- **Formatter:** `createLookupFormatter(lookupRef)` instead of the coltype's
  default formatter — resolves integer IDs → human-readable labels
- **Editor:** `createLookupEditor(lookupRef, lookupData)` with `list` +
  autocomplete instead of the coltype's default editor

When the lookup data is **not** cached (e.g., API unavailable), the column
falls back to the coltype's standard formatter and editor — no runtime error.

`queries.js` → `initTable()` now calls `preloadLookups()` with all unique
`lookupRef` values from the column definitions, so lookup data is in cache
before `resolveColumns()` runs:

```javascript
const lookupRefs = Object.values(this.tableDef.columns || {})
  .map(col => col.lookupRef)
  .filter(Boolean);
const uniqueRefs = [...new Set(lookupRefs)];
if (uniqueRefs.length > 0 && this.app?.api) {
  await preloadLookups(uniqueRefs, this.app.api);
}
```

---

### Phase 4: Navigator Functionality ✅ Complete

| Button | Status | Implementation |
|--------|--------|----------------|
| Refresh | ✅ Working | `loadQueries()` with row selection persistence |
| Search | ✅ Working | Uses config refs (`queryRefs.searchQueryRef`) with fallback |
| Width | ✅ Working | Popup: Narrow (160px), Compact (314px), Normal (468px), Wide (622px), Auto (calculated) |
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
| Layout | ✅ Working | Popup: Fit Columns, Fit Data, Fit Fill, Fit Stretch, Fit Table (persisted to localStorage) |
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
| `src/managers/queries/queries.js` | JSON-driven columns, config query refs (incl. detailQueryRef), Save/Delete API integration, lookup pre-loading, async row-scoped editor activation, consistent selection-before-edit, row-change popup cleanup, stronger queued same-row cell handoff | ✅ Complete |
| `src/core/lithium-table.js` | Lookup auto-wiring in resolveColumn(), persistSort/persistFilter in resolveTableOptions() | ✅ Complete |
| `src/managers/queries/queries.css` | Restrict resize handles to header row only; suppress body/footer handle hit areas | ✅ Complete |
| `tests/unit/queries-manager.test.js` | Covers edit gating, async editor enable/disable, consistent selection/edit click behaviour, row-change popup cleanup, and queued same-row handoff | ✅ Complete |
| `config/tabulator/tabledef-schema.json` | Added `insertQueryRef`, `updateQueryRef`, `deleteQueryRef` fields | ✅ Complete |
| `config/tabulator/queries/query-manager.json` | Added `detailQueryRef: 27` | ✅ Complete |

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
| 4 | 1d | Table-level property mapping (incl. persistSort/persistFilter) | ✅ Done |
| 5 | 2a | Refactor `queries.js` to use resolved columns | ✅ Done |
| 6 | 2b | Use config query refs instead of hardcoded (incl. detailQueryRef) | ✅ Done |
| 7 | 2c | `_discoverColumns()` — kept as hybrid approach | ✅ Done (design decision) |
| 8 | 3 | Lookup column support (load, format, edit) | ✅ Done |
| 8a | 3d | Lookup resolver integration + preloadLookups() in initTable | ✅ Done |
| 9 | 4 | Navigator button implementations | ✅ Done |
| 9a | 4a | Save/Delete API wiring + CRUD QueryRef schema | ✅ Done |
| 10 | 4b | Remove dead `tabulator-init.js` | ✅ Done |
| 11 | 5 | Extract to generic component | ⏳ Future |

---

## Testing Strategy

Per LITHIUM-TOC.md: **do not run the dev server** — validate through:

- `npm run build` — ensure no import/syntax errors
- `npm run validate:tabulator` — ensure JSON configs remain valid
- `npm test` — 490 tests across 17 files (all passing)
- Unit tests for `resolveColumn()`, `wrapFormatter()`, `resolveColumns()`,
  `resolveTableOptions()`, `getPrimaryKeyField()`, `getQueryRefs()`, lookup
  functions, lookup resolver integration, persistSort/persistFilter, and
  cache management (84 tests in `lithium-table.test.js`)

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

### Edit Mode Follow-up: Row-Scoped Editors + Selection Stability

The first pass at inline editing exposed three related issues in the Queries
manager:

1. Column editor updates were asynchronous, so a call to `cell.edit()` could
   run before the new editor definition had been attached to the column.
2. Row selection could vary by cell type because some cell interactions reached
   Tabulator's selection flow differently once edit-capable columns were present.
3. Resize handles could still appear in non-header sections, making resizing
   feel available from the wrong place.

The follow-up fix in `src/managers/queries/queries.js` now:

- makes `_enterEditMode()` / `_exitEditMode()` await async
  `updateColumnDefinition()` work,
- forces row selection consistently on `cellMouseDown` and `cellClick` before
  any edit attempt,
- defers `cell.edit()` with `requestAnimationFrame()` so the newly attached
  editor is available before Tabulator opens it.

The companion CSS update in `src/managers/queries/queries.css` hides
non-header `.tabulator-col-resize-handle` elements with `!important`, leaving
column resizing active only in header cells.

An additional follow-up tightened event handling around footer calcs and
same-row edit handoff:

- footer calc rows/cells are now ignored by selection/edit handlers and have
  pointer events disabled so they cannot become the active row,
- switching from one editable cell to another on the same row now blurs the
  current editor first so Tabulator saves the value before opening the next
  field,
- detail reloads are suppressed while the currently edited row remains selected,
  preventing redundant REST calls during intra-row editing.

One more pass addressed two remaining rough edges observed during manual use:

- row-selection changes now explicitly close transient popups (column chooser,
  navigator popup, footer export popup, font popup), which covers cases where a
  document-level click handler never fires because the selection change came from
  keyboard navigation or a Tabulator event path that stops propagation,
- programmatic cell-editor activation now uses a tokenised double
  `requestAnimationFrame()` handoff so number/list editors have more time to
  open cleanly after the previous inline editor blurs and commits.

### detailQueryRef Wiring

`query-manager.json` now includes `detailQueryRef: 27`, and `queries.js` →
`fetchQueryDetails()` reads this from `this.queryRefs?.detailQueryRef ?? 27`
instead of hardcoding `27`. This ensures the detail-fetch query reference is
config-driven, consistent with all other QueryRefs.

### persistSort / persistFilter

The tabledef schema already supported `persistSort` and `persistFilter` as
boolean properties, and `query-manager.json` had them set to `true`. However,
`resolveTableOptions()` did not map them. Now it does — both properties are
passed through to the Tabulator constructor when present.

### Table Width Presets

The Print button was removed from the first nav block (Control) and replaced
with a **Table Width** popup button (`fa-left-right` icon). Print, Email, and
Export are now handled exclusively by the query manager footer's data-source
controls (which support View vs. Data modes).

The width popup offers five presets based on the nav-block unit:

| Preset | Blocks | Pixel Width | Formula |
|--------|--------|-------------|---------|
| Narrow | 1 | 160px | 150 + 10 |
| Compact | 2 | 314px | 300 + 4 + 10 |
| Normal | 3 | 468px | 450 + 8 + 10 |
| Wide | 4 | 622px | 600 + 12 + 10 |
| Auto | — | calculated | sum of visible column widths + 28px margin |

Where: nav block = 150px, gap = 4px (`--space-2`), container pad = 10px (6 + 4).

**Auto** mode sums visible column widths via the Tabulator column API
(`col.getWidth()`) after a forced redraw. When the current layout is
`fitColumns` (columns stretched to fill), it temporarily switches to
`fitDataTable` and expands the panel to 2000px so columns can spread to
their natural content widths, measures, then restores the original layout.
When a data-fitting layout (`fitData`, `fitDataFill`, `fitDataStretch`,
`fitDataTable`) is active, columns are already at natural width and no
layout switching is needed.

A double `requestAnimationFrame` ensures the browser has fully committed
the redraw before measuring. This fixes a timing issue where the previous
single-rAF approach could measure stale layout dimensions.

**Persistence:** The panel width is saved to `localStorage` (`PANEL_WIDTH_KEY`)
after every preset selection **and** every manual splitter resize. On init, the
stored value is restored and the width mode indicator is auto-detected (±8px
tolerance per preset, falling back to `'custom'`).

### Table Layout Mode

The Email button was removed from the first nav block (Control) — email is
now handled exclusively by the footer's data-source controls — and replaced
with a **Table Layout** popup button (`fa-table-columns` icon).

The layout popup offers five Tabulator layout modes:

| Popup Label | Tabulator Setting | Behaviour |
|-------------|-------------------|-----------|
| Fit Columns | `fitColumns` | Columns stretch proportionally to fill the table width (default) |
| Fit Data | `fitData` | Columns size to their data content; table may be narrower than panel |
| Fit Fill | `fitDataFill` | Size to data, then stretch the **last** column to fill remaining space |
| Fit Stretch | `fitDataStretch` | Size to data, then stretch **all** columns proportionally to fill |
| Fit Table | `fitDataTable` | Size to data; table element itself shrinks to match column widths |

**Persistence:** The layout mode is saved to `localStorage` (`LAYOUT_MODE_KEY`)
and restored on init, overriding the JSON config default. This lets the user's
preferred layout survive refresh and re-login.

**Auto width integration:** When the layout mode changes while the panel is in
auto-width mode, `setTableLayout()` automatically triggers `_applyAutoWidth()`
to resize the panel to match the new natural column widths. This enables a
workflow of: set layout to Fit Data → set width to Auto → panel sizes itself
to exactly fit the table content.

### Lookup Resolver Auto-Wiring

`resolveColumn()` now checks for a `lookupRef` property on the column
definition. If the lookup data is in the module cache (populated by
`preloadLookups()`), the resolver automatically assigns:

- `formatter` → `createLookupFormatter(lookupRef)` (replaces the coltype
  default)
- `editor` → list editor from `createLookupEditor()` with autocomplete

If the lookup is **not** cached, the column falls through to the coltype's
standard formatter/editor. This means the feature is entirely non-breaking:
without a live API to populate lookups, columns render their raw integer IDs
using the coltype formatter.

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

3. ~~**Lookup Integration at Runtime** (Phase 3 enhancement)~~ ✅ Done
   - ~~Call `preloadLookups()` during table init to pre-fetch lookup data~~
   - ~~Wire `createLookupFormatter()` into the column resolver for lookup columns~~
   - `resolveColumn()` now auto-detects `lookupRef` + cached data → wires
     formatter and editor automatically
   - `queries.js` → `initTable()` collects `lookupRef` values from the
     tabledef and calls `preloadLookups()` before resolving columns
   - Falls back gracefully to coltype defaults when lookups are not cached
   - Requires a live Hydrogen API for lookup data to be available
