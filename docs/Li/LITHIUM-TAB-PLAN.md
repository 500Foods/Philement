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
| acuranzo_1153.lua | ✅ Updated - coltypes with "default" stanza |
| acuranzo_1154.lua | ✅ Updated - query-manager tableDef |
| acuranzo_1156.lua | ✅ Updated - lookups-list tableDef |
| acuranzo_1157.lua | ✅ Updated - lookup-values tableDef |

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

## Implementation Tasks (Sequenced)

### Phase 1: Runtime Coltype Resolution

**Goal:** Implement Stage 1 - auto-detection from query data

**Prerequisites:** None - starts from scratch

1. **JSON Type Detection** - Map JSON types to coltypes:
   - `string` → `string`
   - `number` (no decimals) → `integer`
   - `number` (with decimals) → `decimal`
   - `boolean` → `boolean`
   - `null` → `string`

2. **Merge Engine** - Build complete column from:
   - `coltypes.default` (base)
   - `coltypes.{detectedType}` (override)
   - Derived values (title from field name)
   - `columnPri` (1, 2, 3... based on field order)

3. **Validation** - Validate loaded JSON against schema (LITHIUM-TAB-TYPES.schema.json)

**Validation:**

- Unit test: detection logic for each JSON type
- Unit test: merge produces expected properties
- Manual: render a table with raw data, verify defaults work

---

### Phase 2: Lookup 059 Integration

**Goal:** Load from database instead of config files

**Prerequisites:** Phase 1 must be complete first

1. **Database Loader** - Fetch from lookups table:
   - Key 0: `column-types` (coltypes with "default" stanza)
   - Keys 1+: Table definitions

2. **Caching** - Singleton per session (existing pattern)

3. **Fallback** - If Lookup 059 unavailable, fall back to Stage 1 auto-detection

**Validation:**

- Integration test: Lookup 059 Key 0 loads correctly
- Integration test: Table definitions load from Keys 1+
- Manual: Verify existing tables (Query Manager, Lookups List) render with DB config

---

### Phase 3: Column Manager (Stage 3)

**Goal:** Full property editing via Column Manager

**Prerequisites:** Phase 2 must be complete first (needs Lookup 059 data)

1. **Expand Properties** - Currently limited subset; expand to support all coltype properties:
   - `title`, `visible`, `width`, `columnPri`
   - `coltype` (change type)
   - `formatter`, `formatterParams`
   - `editor`, `editorParams`
   - `bottomCalc`, `bottomCalcFormatter`
   - Any other property from coltype system

2. **Export/Import** - "Copy tableDef as JSON" → paste into Lookup 059 entry

3. **Column Manager TableDef** - The Column Manager itself has a Lookup 059 entry (customize via Column Manager Manager)

**Validation:**

- Manual: Open Column Manager, change a property, verify it applies
- Manual: Export tableDef as JSON, verify it's valid
- Manual: Customize a table, export JSON, paste to Lookup 059, reload → verify Stage 2 defaults

---

### Phase 4: Migrate Internal Coltypes

**Goal:** Remove hardcoded coltypes from Column Manager

**Prerequisites:** Phase 2 must be complete (needs Lookup 059 working)

1. **Identify** - `lookupFixed`, `stringList` are currently hardcoded in cm-table.js
2. **Create Lookup** - Add proper lookup entries in Lookup 059 or separate lookup tables
3. **Replace** - Use standard `lookup` coltype with predefined values

**Validation:**

- Unit test: All coltypes come from database
- Manual: Verify Column Manager dropdowns still work

---

## Dependencies & Sequencing

```stages
Phase 1 (Stage 1 Engine)
    ↓
    Required before Phase 2
Phase 2 (Load from DB)
    ↓
    Required before Phase 3
Phase 3 (User Customization)
    ↓
    Required before Phase 4
Phase 4 (Cleanup)
```

**Critical Path:** Phase 1 → Phase 2 → Phase 3

- Phase 4 is cleanup that can happen in parallel or after

---

## Property Mapping Reference

Old → New names (for implementation):

| Old | New | Notes |
|-----|-----|-------|
| `display` | `title` | Column header |
| `align` | `hozAlign` | Horizontal alignment |
| `sort` | `headerSort` | Header click to sort |
| `filter` | `headerFilter` | Header filter input |
| `group` | `groupable` | Enable grouping |
| — | `coltype` | **Required** - references Lookup 059 Key 0 |
| — | `columnPri` | Display order |
| — | `primaryKey` | DB primary key marker |
| — | `calculated` | Runtime-computed value marker |

---

## Test Plan

### Phase 1 Tests

- Unit: JSON type → coltype mapping
- Unit: Merge logic (default + coltype + derived)
- Unit: Schema validation

### Phase 2 Tests  

- Integration: Lookup 059 loading
- Integration: Table definition loading
- Manual: Verify existing tables work

### Phase 3 Tests

- Manual: Column Manager property changes apply
- Manual: Export/Import workflow
- Manual: Stage 2 capture works

### Phase 4 Tests

- Unit: No hardcoded coltypes
- Manual: All coltypes from database

---

## Notes

- Implementation should ignore unknown properties (pass through)
- "default" stanza must be first when resolving (root of everything)
- The Column Manager is our primary tableDef editor
- Export JSON → paste to Lookup 059 = way to capture Stage 2 defaults