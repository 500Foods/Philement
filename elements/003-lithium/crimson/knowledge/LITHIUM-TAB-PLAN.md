# LithiumTable Implementation Plan

This document outlines the plan to align the runtime implementation with the documented tableDef resolution process.

---

## Quick Start (Start Here)

For a new developer or anyone needing to understand the system quickly:

1. **Read [LITHIUM-TAB-TABLES.md](LITHIUM-TAB-TABLES.md)** — The core process document. Focus on:
   - Stage 1: Auto-detection from data
   - Stage 2: Table definitions from Lookup 059
   - Stage 3: User customization via Column Manager
   - Property precedence (later stages override earlier)

2. **Read [LITHIUM-TAB-TYPES.md](LITHIUM-TAB-TYPES.md)** — The coltype reference. Key sections:
   - Table Definition Properties (`coltype`, `primaryKey`, `calculated`)
   - Base Properties table (all available properties)
   - Quick Reference (all 25 coltypes at a glance)

3. **Read [LITHIUM-COL.md](LITHIUM-COL.md)** — Column Manager is Stage 3. Key sections:
   - Stage 3: User Customization
   - Export/Import Workflow (how to capture tableDef as JSON)

**TL;DR:** Data comes in → Stage 1 detects coltype → Stage 2 applies tableDef from Lookup 059 → Stage 3 user edits. All stages produce a usable tableDef.

---

## Architecture: Async Non-Blocking Data-First Approach

### Critical: Always Async, Never Blocking

The implementation MUST be fully async throughout. Key principles:

1. **Prefetch data before init** — When possible, start fetching data before table initialization begins
2. **Never block** — All operations (data loading, column resolution, table init) are async
3. **Pipeline pattern** — Load config → Fetch data → Detect columns → Build tableDef → Init table

```
Manager.create()                 // Returns immediately
  .prefetchData()                // Starts async, returns promise
  .init();                      // Continues with data ready
```

### Initialization Flow

```javascript
// NEW: Data-first async flow
async init() {
  // 1. Load config (async, non-blocking)
  await this.loadConfiguration();
  
  // 2. Prefetch data ASAP (can start before init if manager knows queryRef)
  const dataPromise = this.loadData();
  
  // 3. Resolve columns from config WHILE data loads
  const columns = resolveColumns(this.tableDef, this.coltypes);
  
  // 4. Wait for data, then detect/discover columns
  const data = await dataPromise;
  const stage1TableDef = detectColumnsFromData(data, this.coltypes);
  
  // 5. Merge with Lookup 059 (Stage 2)
  const stage2TableDef = mergeWithTableDef(stage1TableDef, this.tableDef);
  
  // 6. Apply user template (Stage 3)
  const finalTableDef = applyUserTemplate(stage2TableDef);
  
  // 7. Init Tabulator with complete tableDef
  await this.initTable(finalTableDef);
}
```

This avoids the costly column add/remove operations after table creation.

---

## Database Structure

### Lookup 059 Key 0 - coltypes (acuranzo_1153.lua)

Migration `acuranzo_1153.lua` defines the coltypes structure in Lookup 059 Key 0:

- **Location**: `elements/002-helium/acuranzo/migrations/acuranzo_1153.lua`
- **Lookup ID**: 059
- **Key Index**: 0
- **Schema Name**: column-types

The JSON structure has a `"default"` stanza (base properties) plus each coltype with ONLY properties that differ from default:

```json
{
  "coltypes": {
    "default": {
      "description": "Default column configuration",
      "title": "Column",
      "field": null,
      "visible": false,
      "hozAlign": "left",
      "vertAlign": "middle",
      "formatter": "plaintext",
      "editor": "input",
      "sorter": "alphanum",
      "headerSort": true,
      "headerFilter": false,
      "groupable": false,
      "columnPri": null,
      "blank": "",
      "zero": null,
      // ... all base properties
    },
    "boolean": {
      "description": "True/false toggle values",
      "hozAlign": "center",
      "formatter": "tickCross",
      // ... only properties that differ from default
    },
    "integer": {
      "description": "Whole numbers (IDs, counts, references)",
      "hozAlign": "right",
      "formatter": "number",
      // ...
    }
    // ... all 25 coltypes
  }
}
```

### Lookup 059 Keys 1+ - Table Definitions

| Key | Name | Purpose |
|-----|------|---------|
| 0 | column-types | All coltype definitions (with "default" stanza) |
| 1 | query-manager | Query Manager table definition |
| 2 | lookups-list | Lookups List table definition |
| 3 | lookup-values | Lookup Values table definition |
| ... | ... | Additional table definitions |

---

## Current State

### Documentation ✅ Complete

| Document | Status |
|----------|--------|
| [LITHIUM-TAB-TABLES.md](LITHIUM-TAB-TABLES.md) | Complete - documents Stage 1/2/3 resolution |
| [LITHIUM-TAB-TYPES.md](LITHIUM-TAB-TYPES.md) | Complete - all 25 coltypes documented |
| [LITHIUM-TAB-TYPES-*.md](LITHIUM-TAB-TYPES-DEFAULT.md) | Complete - individual type docs |
| [LITHIUM-TAB-TYPES.schema.json](LITHIUM-TAB-TYPES.schema.json) | Complete - validation schema |
| [LITHIUM-COL.md](LITHIUM-COL.md) | Complete - Column Manager docs |

### Database/Migrations ✅ Complete

| Migration | Status |
|-----------|--------|
| acuranzo_1153.lua | ✅ Complete - coltypes with "default" stanza (Key 0) |
| acuranzo_1154.lua | ✅ Complete - query-manager tableDef (Key 1) |
| acuranzo_1156.lua | ✅ Complete - lookups-list tableDef (Key 2) |
| acuranzo_1157.lua | ✅ Complete - lookup-values tableDef (Key 3) |

### Current Runtime Implementation ⚠️ Needs Updates

| Component | Status | Notes |
|-----------|--------|-------|
| `loadColtypes()` | ✅ Works | Loads from Lookup 059 Key 0 |
| `loadTableDef()` | ✅ Works | Loads from Lookup 059, has auto-discover fallback |
| `resolveColumn()` | ⚠️ Partial | Works but does NOT merge from coltypes.default first |
| `discoverColumns()` | ⚠️ Partial | Basic fallback exists, no coltype detection |
| `columnPri` ordering | ❌ Missing | Not implemented |
| Schema validation | ❌ Missing | Not implemented |
| Data-first init | ⚠️ Partial | Loads config, then data - should load data FIRST |

---

## The Target Architecture

```stages
Stage 1: Auto-Detection (Runtime)
├── Input: Query data (JSON array)
├── Process: 
│   1. Detect coltype from JSON data type (string → string, number → integer/decimal, boolean → boolean)
│   2. Load coltypes.default + detected coltype from Lookup 059 Key 0
│   3. Merge: default → coltype → derived (title from field name, columnPri from order)
└── Output: Fully populated tableDef

Stage 2: Table Definition (Lookup 059 Keys 1+)
├── Input: Stage 1 tableDef + tableDef from Lookup 059 Key N
├── Process: Merge per-column overrides from table definition
└── Output: Enhanced tableDef with developer-defined defaults

Stage 3: User Customization (Column Manager)
├── Input: Stage 2 tableDef + user preferences
├── Process: Apply user template overrides
└── Output: Final personalized tableDef
```

---

## Implementation Phases (Sequenced with Testing Gates)

**IMPORTANT:** Each phase MUST have passing tests before continuing to the next phase.

---

### Phase 1: Default Merge Engine (FOUNDATION)

**Goal:** Implement the critical merge order: `coltypes.default` → `{coltype}`

**Why first:** This is the foundation. Nearly everything builds on top of this hierarchy being correct.
- The "default" stanza in Lookup 059 Key 0 is the root of everything
- Each coltype stanza only contains properties that DIFFER from default
- We MUST merge default FIRST, then overlay the specific coltype

**Testing Gate:** Unit tests MUST pass for merge logic before Phase 2

1. **Default + Coltype Merge Engine** - The critical merge order:
   ```
   // WRONG (current):
   merged = { ...coltype[colDef.coltype], ...colDef }
   
   // CORRECT (new):
   const defaultCol = coltypes.default || {};
   const typeCol = coltypes[colDef.coltype] || {};
   merged = { ...defaultCol, ...typeCol, ...colDef };
   ```
   Each coltype stanza only contains properties that differ from default.

2. **Resolve Column Refactor** - Update `resolveColumn()` to use correct merge order

3. **Coltypes Caching** - After first merge, cache resolved coltypes per session

4. **Lookup 059 Fresh Fetch** - Always fetch from DB, only cache merged result

**Validation:**
- Unit test: merge produces expected properties (default + type combined)
- Unit test: unknown properties pass through unchanged
- Unit test: derived values (title) correct

---

### Phase 2: Auto-Discovery with Coltype Detection

**Goal:** Implement `discoverColumns()` with JSON type → coltype detection

**Testing Gate:** Unit tests MUST pass for detection logic before Phase 3

1. **JSON Type Detection** - Map JSON types to coltypes:
   - `string` → `string`
   - `number` (no decimals) → `integer`
   - `number` (with decimals) → `decimal`
   - `boolean` → `boolean`
   - `null` → `string` (treated as empty string)

2. **discoverColumns() Enhancement** - Detect coltypes from data, not just generic columns:
   - Sample each field's values to determine type
   - Apply default merge (from Phase 1) for each discovered column
   - Derive `title` from field name (e.g., `query_id` → "Query Id")

3. **Derived Values** - For auto-detected columns:
   - `title`: Derive from field name
   - `columnPri`: Assign based on field order in data (1, 2, 3...)

**Validation:**
- Unit test: JSON type → coltype mapping for each type
- Unit test: derived title matches field name pattern
- Unit test: columnPri assigned in field order

---

### Phase 3: Schema Validation

**Goal:** Validate tableDef after each stage

**Testing Gate:** Unit tests MUST pass for validation before Phase 4

1. **Schema Validation** - Validate tableDef after each stage (dev mode only):
   - Fetch schema from `LITHIUM-TAB-TYPES.schema.json` or Lookup 059
   - Validate Stage 1 auto-detected tableDef
   - Validate Stage 2 merged tableDef
   - Validate Stage 3 user-modified tableDef
   - Unknown properties ignored (pass through for extensibility)

2. **Dev Mode Flag:** `localStorage.getItem('li_dev_validate') === true`
   - Production skips validation for performance

**Validation:**
- Unit test: schema validation rejects invalid tableDef
- Unit test: schema validation accepts valid tableDef
- Unit test: unknown properties pass through

---

### Phase 4: Data-First Initialization

**Goal:** Refactor initialization to load data BEFORE init table

**Why:** In Tabulator, redefining columns is a costly operation.
- If we need to do all this to get the tableDef in shape anyway,
  there's no point drawing the table and then redoing it again.
- Just get the tableDef ready FIRST, then touch Tabulator.

**Testing Gate:** Integration tests MUST pass before Phase 5

1. **Data Prefetch** - Load data BEFORE config/table init
   - Get data and determine the FULL tableDef before Tabulator
   - This includes Stage 1 (auto-detection), Stage 2 (Lookup 059), Stage 3 (user)

2. **Async Pipeline** - Full async flow (never block):
   ```
   async init() {
     // 1. Start loading data FIRST (async, non-blocking)
     const dataPromise = this.loadData();
     
     // 2. Wait for data, then do ALL column resolution
     const data = await dataPromise;
     
     // 3. Stage 1: Detect columns from data types
     const stage1TableDef = detectColumnsFromData(data, this.coltypes);
     
     // 4. Stage 2: Merge with Lookup 059 table definition
     const stage2TableDef = mergeWithTableDef(stage1TableDef, this.tableDef);
     
     // 5. Stage 3: Apply user template (if saved)
     const finalTableDef = applyUserTemplate(stage2TableDef);
     
     // 6. NOW init Tabulator with complete tableDef - no columns to add!
     await this.initTable(finalTableDef);
   }
   ```

3. **columnPri Sorting** - Sort columns by `columnPri` before passing to Tabulator:
   ```
   columns.sort((a, b) => {
     const priA = a.columnPri ?? Infinity;
     const priB = b.columnPri ?? Infinity;
     return priA - priB;
   });
   ```

**Validation:**
- Integration test: Table renders with complete tableDef on first draw
- Integration test: No column add/remove operations after initial render

---

### Phase 5: Lookup 059 Integration

**Goal:** Full table definition loading from database

**Testing Gate:** Integration tests MUST pass before Phase 6

1. **Database Loader** - Fetch from lookups table:
   - Key 0: `column-types` (coltypes with "default" stanza)
   - Keys 1+: Table definitions

2. **No Caching of Lookup Data** - Fetch fresh each time
3. **Fallback** - If Lookup 059 unavailable, fall back to Stage 1 auto-detection

**Validation:**
- Integration test: Lookup 059 Key 0 loads correctly
- Integration test: Table definitions load from Keys 1+
- Manual: Verify existing tables (Query Manager, Lookups List) render with DB config

---

### Phase 6: Column Manager (Stage 3)

**Goal:** Full property editing via Column Manager

**Testing Gate:** Manual tests MUST pass before Phase 7

1. **Expand Properties** - Support all coltype properties:
   - `title`, `visible`, `width`, `columnPri`
   - `coltype` (change type)
   - `formatter`, `formatterParams`
   - `editor`, `editorParams`
   - `bottomCalc`, `bottomCalcFormatter`

2. **Export/Import** - "Copy tableDef as JSON" → paste into Lookup 059

3. **Column Manager TableDef** - The Column Manager itself has a Lookup 059 entry

**Validation:**
- Manual: Open Column Manager, change a property, verify it applies
- Manual: Export tableDef as JSON, verify it's valid

---

### Phase 7: Migrate Internal Coltypes

**Goal:** Remove hardcoded coltypes from Column Manager

**Testing Gate:** Unit tests MUST pass before complete

1. **Remove Hardcoded Coltypes** - No `lookupFixed`, `stringList` hardcoded
2. **Use Standard Coltypes** - Use existing coltypes from Lookup 059

**Validation:**
- Unit test: All coltypes come from database
- Manual: Verify Column Manager dropdowns still work

---

---

## Dependencies & Sequencing

```stages
Phase 1 (Default Merge Engine - FOUNDATION)
    ↓ Testing Gate
Phase 2 (Auto-Discovery + Coltype Detection)
    ↓ Testing Gate
Phase 3 (Schema Validation)
    ↓ Testing Gate
Phase 4 (Data-First Initialization)
    ↓ Testing Gate
Phase 5 (Lookup 059 Integration)
    ↓ Testing Gate
Phase 6 (Column Manager)
    ↓ Testing Gate
Phase 7 (Migrate Internal Coltypes)
```

**Critical Path:** Phases 1-4 are critical. Phases 5-7 can happen in parallel or after.

---

## Property Mapping Reference

New implementation uses clean property names:

| New | Description |
|-----|-------------|
| `title` | Column header |
| `hozAlign` | Horizontal alignment |
| `headerSort` | Header click to sort |
| `headerFilter` | Header filter input |
| `groupable` | Enable grouping |
| `coltype` | References Lookup 059 Key 0 |
| `columnPri` | Display order (1, 2, 3... based on field order) |
| `primaryKey` | DB primary key marker |
| `calculated` | Runtime-computed value marker |

---

## Test Plan

### Test Infrastructure

- **Framework**: Vitest (`npm run test` = `vitest run`)
- **Location**: `elements/003-lithium/tests/unit/`
- **Existing tests**:
  - `lithium-table.test.js` - Core table resolution tests (123 tests)
  - `lithium-table-template.test.js` - Template tests (3 tests)
  - `lithium-column-manager.test.js` - Column Manager tests (19 tests)
  - Multiple other unit tests

**Current test status** (pre-change):
- 21 test files passed
- 597 tests passed

### Running Tests

```bash
# Run unit tests (Vitest)
npm run test

# Run unit tests in watch mode
npm run test:watch

# Run with coverage
npm run test:coverage

# Run CI test suite
npm run test:ci
```

### Phase 1 Tests (Default Merge Engine)

- Unit: merge produces expected properties (default + type combined)
- Unit: unknown properties pass through unchanged

### Phase 2 Tests (Auto-Discovery)

- Unit: JSON type → coltype mapping for each type
- Unit: derived title matches field name pattern
- Unit: columnPri assigned in field order

### Phase 3 Tests (Schema Validation)

- Unit: schema validation rejects invalid tableDef
- Unit: schema validation accepts valid tableDef
- Unit: unknown properties pass through

### Phase 4 Tests (Data-First Init)

- Integration: Table renders with complete tableDef on first draw
- Integration: No column add/remove operations after initial render
- Manual: Verify existing tables work

### Phase 5 Tests (Lookup 059 Integration)

- Integration: Lookup 059 Key 0 loads correctly
- Integration: Table definitions load from Keys 1+
- Manual: Verify existing tables (Query Manager, Lookups List) render with DB config

### Phase 6 Tests (Column Manager)

- Manual: Open Column Manager, change a property, verify it applies
- Manual: Export tableDef as JSON, verify it's valid

### Phase 7 Tests (Migrate Internal)

- Unit: All coltypes come from database
- Manual: Column Manager dropdowns still work

**Pre-change baseline:** 597 tests passing

---

## Implementation Details

### Stage 1: Data-First Initialization (Reference)

1. **Load data** (via queryRef)
2. **Detect columns** from data types → Stage 1 tableDef
3. **Merge with Lookup 059** table definition → Stage 2 tableDef
4. **Apply user template** (if saved) → Stage 3 tableDef
5. **Resolve to Tabulator format** and init table

### columnPri and Column Ordering

Tabulator uses the columns array order for display (first in array = leftmost column). Implementation:

```javascript
// Resolve columns to array, then sort by columnPri before passing to Tabulator
columns.sort((a, b) => {
  const priA = a.columnPri ?? Infinity;
  const priB = b.columnPri ?? Infinity;
  return priA - priB;
});
```

The `columnPri` property flow:
1. **Stage 1 assignment:** `columnPri` is set to field order (1, 2, 3...) based on `Object.keys(data[0])`
2. **Stage 2 override:** Table definition in Lookup 059 can set explicit `columnPri` values
3. **Stage 3 user override:** Column Manager or drag-to-reorder updates `columnPri`

### Caching Strategy

1. **Never cache raw Lookup 059 data** - Always fetch fresh from database
2. **Cache merged resolved coltypes** - After merging default + coltype stanzas, cache the result
3. **On Refresh button click:** Always re-fetch Lookup 059 (invalidate cache before fetch)

### Schema Validation

Validate tableDef after each stage using `LITHIUM-TAB-TYPES.schema.json`:

- Stage 1: Validate auto-detected tableDef
- Stage 2: Validate merged tableDef from Lookup 059
- Stage 3: Validate user-modified tableDef from Column Manager

Unknown properties should be ignored (pass through) to allow future extensibility.

**Dev mode only:** Validation enabled via flag (e.g., `localStorage.getItem('li_dev_validate') === true`). Production skips validation for performance.

---

## Notes

- **"default" stanza is the ROOT of everything** - all coltypes inherit from it
- **Merge order:** Start with `coltypes.default`, then overlay `coltypes.{type}` (NOT the reverse)
- Each coltype stanza only contains variances from default
- Implementation should ignore unknown properties (pass through)
- **No caching of Lookup 059 data** - fetch fresh from database each time
- **Cache merged coltypes** - after first resolve, cache the merged result for the session
- **Data-first:** Get tableDef ready BEFORE touching Tabulator
- **`columnPri`** is the property for column display order (Stage 1 assigns from field order)
- The Column Manager is our primary tableDef editor
- Export JSON → paste to Lookup 059 = way to capture Stage 2 defaults

---

## Migration Reference

| Migration | File | Key | Purpose |
|------------|------|-----|---------|
| acuranzo_1153.lua | elements/002-helium/acuranzo/migrations/acuranzo_1153.lua | 0 | coltypes with "default" stanza |
| acuranzo_1154.lua | — | 1 | query-manager tableDef |
| acuranzo_1156.lua | — | 2 | lookups-list tableDef |
| acuranzo_1157.lua | — | 3 | lookup-values tableDef |

(End of file)