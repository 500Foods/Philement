# LithiumTable — Implementation Plan (Round 2)

This is the second major implementation plan for LithiumTable. The first plan took us from "nothing" to "functional three-stage column resolution with editable tables, templates, and a Column Manager." That plan is complete and archived under `docs/Li/Plans-Complete/`. This plan takes us from "functional" to "solid, complete, and honest about what it is."

**Last updated:** April 2026

---

## How this plan works

### Serial execution

Phases are executed **one at a time**, in order. No phase begins until the previous phase's gate has passed. There is no parallel work. This is deliberate: each phase narrows the codebase's wobble before the next phase adds load to it.

### One phase = one task = one hand-off

Each phase is scoped to be approachable by a single session (human or AI) without needing to hold the entire rest of the plan in working memory. The phase document should give a model enough context to do the phase and nothing more. Phases deliberately repeat essential context (key file paths, the Acuranzo reference, the property vocabulary) so a fresh session has what it needs.

### Phase structure

Every phase has the same five-section layout:

1. **Goal** — a single sentence stating what changes at the end.
2. **Prerequisites — docs to review** — the minimum reading to understand the phase. Models starting a phase should read these before writing code.
3. **Work items** — the concrete changes to make, numbered.
4. **Gate** — the conditions that must all hold before the next phase begins.
5. **Docs to update on completion** — the canonical places that need to reflect the new reality before we close the phase.

### The gate

The gate is the same shape for every phase:

1. **Tests pass.** Either existing tests still pass, OR tests that break are fixed — by fixing the test when the code is right, or fixing the code when the test is right. Tests are not gospel; code is not gospel. Both must agree with reality.
2. **Clean build.** `npm run lint` and `npm run build` produce zero errors and zero warnings that are attributable to this phase's changes.
3. **Docs updated.** Every file in `docs/Li/` that relates to the phase's changes reflects the new reality. No "we'll update the docs later."
4. **File-size discipline.** No file touched or created in the phase exceeds 1,000 lines (hard limit from [LITHIUM-INS.md](LITHIUM-INS.md)). The project target is 750 lines — phases explicitly devoted to refactoring bring offending files under 750 before feature work touches them.
5. **Operational state improved.** The app at the end of the phase runs as well as or better than it did at the phase's start. No temporary regressions carried forward.

### Coding standards (already canonical, restated here for phase gates)

See [LITHIUM-INS.md](LITHIUM-INS.md) and [LITHIUM-DEV.md](LITHIUM-DEV.md) for the full set. The most relevant during these phases:

- **Prefer CSS classes; no inline styles** (INS.md §4, "CSS-First Styling"). No `element.style.property = value` except for dynamic positioning (drag handles, popup anchors). Define classes in CSS files; toggle with `classList`.
- **CSS uses existing variables** (INS.md §6). Check `base.css`, `components.css`, `layout.css`, and existing manager CSS before adding new variables.
- **No fallback code for primary features** (INS.md §1). When we change something in this plan, we change it — no "for backward compat" branches.
- **One class per file, pure functions where possible** (INS.md §3). When a phase refactors, it extracts along responsibility lines, not by slicing.
- **File size target 750, hard limit 1000** (INS.md §2, TAB.md target). Refactor phases exist in this plan specifically to get the oversized files down before feature work runs through them.

### No backward compatibility

There are zero customers on this code. Every change is clean. No deprecation shims. No "legacy" branches. If a property is renamed, every callsite is updated and the old name is deleted.

---

## Ground truth references

These are the upstream sources of truth we consult repeatedly.

### Acuranzo migrations (the database)

The canonical index is at [`elements/002-helium/acuranzo/README.md`](../../elements/002-helium/acuranzo/README.md).

Migrations relevant to LithiumTable:

| Migration | Lookup / Key | Purpose |
|-----------|--------------|---------|
| [1153](../../elements/002-helium/acuranzo/migrations/acuranzo_1153.lua) | 059 / 0 | **Column Types** (`column-types`) — the coltype stanzas with the "default" foundation |
| [1154](../../elements/002-helium/acuranzo/migrations/acuranzo_1154.lua) | 059 / 1 | **JSON Schema** for tableDef validation |
| [1155](../../elements/002-helium/acuranzo/migrations/acuranzo_1155.lua) | QueryRef 060 | `Get Tabulator Schemas` — loads all of Lookup 59 |
| [1156](../../elements/002-helium/acuranzo/migrations/acuranzo_1156.lua) | 059 / 11 | `style-manager-base-vars` tableDef |
| [1157](../../elements/002-helium/acuranzo/migrations/acuranzo_1157.lua) | 059 / 12 | `style-manager-semantic-vars` tableDef |
| [1179](../../elements/002-helium/acuranzo/migrations/acuranzo_1179.lua) | 059 / 2 | `column-manager` tableDef |
| [1180](../../elements/002-helium/acuranzo/migrations/acuranzo_1180.lua) | 059 / 3 | `column-manager-manager` tableDef |
| [1181](../../elements/002-helium/acuranzo/migrations/acuranzo_1181.lua) | 059 / 4 | `user-profile-sections` tableDef |
| [1182](../../elements/002-helium/acuranzo/migrations/acuranzo_1182.lua) | 059 / 5 | `query-manager` tableDef |
| [1183](../../elements/002-helium/acuranzo/migrations/acuranzo_1183.lua) | 059 / 6 | `lookups-manager-list` tableDef |
| [1184](../../elements/002-helium/acuranzo/migrations/acuranzo_1184.lua) | 059 / 7 | `lookups-manager-values` tableDef |
| [1185](../../elements/002-helium/acuranzo/migrations/acuranzo_1185.lua) | 059 / 8 | `style-manager-list` tableDef |
| [1186](../../elements/002-helium/acuranzo/migrations/acuranzo_1186.lua) | 059 / 9 | `style-manager-sections` tableDef |
| [1187](../../elements/002-helium/acuranzo/migrations/acuranzo_1187.lua) | 059 / 10 | `version-manager` tableDef |

When a phase changes the shape of the coltype or tableDef JSON, the corresponding migration must be updated in the same commit as the code change.

### Terminology disambiguation (critical — do not conflate)

Two terms that sound similar and are **very different**:

- **Lookup** (capital L) / **Lookups** — our database mechanism for storing key-value-data (Lookup 1 = Lookup Status, Lookup 28 = Query Type, Lookup 59 = Tabulator Schemas, etc.). Identified by `LOOKUP_ID`; individual entries by `KEY_IDX`. Backed by QueryRef 34 (`Get Lookup List`). Documented in [LITHIUM-LUT.md](LITHIUM-LUT.md).
- **lookup coltype** (lowercase) — a column type in a tableDef that presents a dropdown/combobox editor and displays a resolved label rather than a raw integer. A lookup coltype *uses* a Lookup to get its values, but the coltype itself is just a presentation pattern.

A column whose stored value is an integer (e.g., `query_status_a27`) is an **integer field** in the database. When we render it with coltype `"lookup"` and `"lookupRef": "a27"`, we look up integer values in Lookup 27 to show the label and populate the dropdown. The column did not become a "Lookup" — it remains an integer field *rendered* through a Lookup.

### Tabulator property preference

Whenever Tabulator has a native property for a concept we're modeling, we **prefer Tabulator's name**. Examples where we currently diverge and should fix:

| Our name | Tabulator name | Plan decision |
|----------|----------------|---------------|
| `display` | `title` | Keep `title`, retire `display` (Phase 3) |
| `align` | `hozAlign` | Already `hozAlign` ✓ |

Lithium-only properties (no Tabulator equivalent) that stay as Lithium-named:

- `coltype` — references Lookup 59 Key 0 entries
- `columnPri` — display-order priority
- `groupable`, `groupPri`, `groupOrd` — grouping (Phase 9)
- `sortPri` — multi-column sort priority (Phase 9)
- `filterPri` — reserved for future; not implemented unless needed
- `primaryKey` — identifies PK columns for row ID composition
- `calculated` — flag for non-stored, derived columns
- `lookupRef`, `lookupStyle`, `lookupEdit` — lookup-coltype family (Phase 11)

### Current oversized files (>750 lines) in `src/tables/` and dependents

As a reference point for the refactor phases:

**Before Phase 4:**
- `src/tables/lithium-table-base.js` — 2,129 lines
- `src/tables/lithium-table.js` — 1,245 lines
- `src/tables/lithium-table-ui.js` — 1,158 lines
- `src/tables/lithium-table.css` — 1,059 lines
- `src/tables/lithium-column-manager.css` — 773 lines
- `tests/unit/lithium-table.test.js` — 1,667 lines (exempt from the 750 target per INS.md §2 rationale, but noted)

**After Phase 4:**
- `src/tables/lithium-table-base.js` — 871 lines
- `src/tables/lithium-table.js` — 482 lines ✅
- `src/tables/lithium-table-ui.js` — 731 lines ✅
- `src/tables/lithium-table.css` — 1,059 lines (Phase 18 target)
- `src/tables/lithium-column-manager.css` — 773 lines (Phase 18 target)

Larger managers that consume LithiumTable (touched by some phases):

- `src/managers/lookups/lookups.js` — 1,784 lines
- `src/managers/style-manager/style-manager.js` — 1,595 lines
- `src/managers/queries/queries.js` — 1,162 lines

---

## Phase list (serial execution)

The phases below run in this exact order. Each phase's gate must close before the next begins.

1. **Phase 1** — Project prep: coding standards note + plan mechanics
2. **Phase 2** — Refactor `lithium-table-base.js` (2,129 → under 750)
3. **Phase 3** — Canonicalize the column property schema (`display` → `title`, wire validation)
4. **Phase 4** — Refactor `lithium-table.js` and `lithium-table-ui.js` (both > 750)
5. **Phase 5** — Single canonical template extractor
6. **Phase 6** — Fix three-stage merge semantics
7. **Phase 7** — Fix string editing
8. **Phase 8** — Fix lookup-coltype editing
9. **Phase 9** — Grouping, sorting, and filtering trifecta
10. **Phase 10** — Primary key discipline
11. **Phase 11** — Lookup coltype family expansion (icon variants)
12. **Phase 12** — Date/time coltypes: Luxon integration
13. **Phase 13** — Date/time coltypes: Flatpickr editor integration
14. **Phase 14** — Navigator Refresh button isolation
15. **Phase 15** — Exhaustive tableDef generation from live tables
16. **Phase 16** — Column Manager cleanup and "tableDef editor" realization
17. **Phase 17** — Table-wide settings section in the Column Manager
18. **Phase 18** — CSS file audit and refactor (`lithium-table.css` > 750)
19. **Phase 19** — Test coverage and documentation closure

---

## Phase 1 — Project prep: coding standards note and plan mechanics

**Status:** ✅ **COMPLETE** — April 17, 2026

**Goal:** Add the missing "code hygiene" cross-references to `LITHIUM-DEV.md` so every subsequent phase has a clear single place to look, and confirm the plan mechanics are wired into the doc set.

### Prerequisites — docs to review

- [LITHIUM-INS.md](LITHIUM-INS.md) (primary — already has the standards)
- [LITHIUM-DEV.md](LITHIUM-DEV.md) (destination for cross-references)
- [LITHIUM-TOC.md](LITHIUM-TOC.md) (confirm this plan is indexed)

### Work items

1. In `LITHIUM-DEV.md`, add a short **Code Hygiene** section near the top (or under "Coding Patterns") that **links to INS.md for the canonical rules** and adds these reminders:
   - Prefer CSS over inline styles (no `element.style.X = ...` except dynamic positioning)
   - File-size target is 750 lines; hard limit is 1,000 (INS.md §2)
   - No fallback branches for primary features (INS.md §1)
   - Every phase in the active plan requires a clean `npm run lint` and `npm run build`
2. In `LITHIUM-TOC.md`, confirm the entry for this plan is prominent and correctly described.
3. Delete the empty directory `elements/003-lithium/src/tables/base/` (leftover from prior refactor).
4. In `LITHIUM-MGR.md`, update the LithiumTable "Architecture → Files" block — it currently lists `src/core/lithium-table-*.js` paths but the files moved to `src/tables/`. Point to the canonical list in [LITHIUM-TAB.md](LITHIUM-TAB.md) rather than duplicating it.

### Gate

- `LITHIUM-DEV.md` has the new hygiene section linking to INS.md.
- `LITHIUM-MGR.md` paths are correct.
- Empty `src/tables/base/` directory is gone.
- `npm test` and `npm run lint` both clean (they should be — no code changes this phase).

### Docs to update on completion

- `LITHIUM-DEV.md` (section added)
- `LITHIUM-MGR.md` (file paths corrected)
- `LITHIUM-TOC.md` (verified)

---

## Phase 2 — Refactor `lithium-table-base.js`

**Status:** ✅ **COMPLETE** — April 18, 2026

**Goal:** Bring `lithium-table-base.js` from 2,129 lines to under 1,000 by extracting self-contained responsibilities into sibling modules, with zero functional change.

This phase is deliberately early because every subsequent feature phase touches `base.js`. Doing surgery on a 2,129-line file is hard; doing it on a 1,000-line file is more manageable.

**Result:** Base file reduced from 2,129 to 872 lines (-1,257 lines). All major functional areas are now extracted into focused modules.

### Prerequisites — docs to review

- [LITHIUM-TAB.md](LITHIUM-TAB.md) — especially the "File Structure" and "Architecture" sections
- [LITHIUM-INS.md](LITHIUM-INS.md) §2 (file size) and §3 (module architecture)
- Current state of `src/tables/lithium-table-base.js`
- Current state of `src/tables/lithium-table-ui.js` — it duplicates `applyPanelWidth`; that duplicate also gets removed here

### Work items

1. ✅ **Extract group-icon animation** into `src/tables/visual/group-icon-animator.js` (159 lines)
2. ✅ **Extract column header tooltip init** into `src/tables/visual/column-tooltips.js` (112 lines)
3. ✅ **Consolidate panel-width persistence** into `src/tables/persistence/panel-width.js` (97 lines). **Deleted duplicate `applyPanelWidth` from `lithium-table-ui.js`**.
4. ✅ **Extract refresh orchestration** into `src/tables/refresh-orchestrator.js` (149 lines)
5. ✅ **Extract template capture** into `src/tables/template/capture.js` (132 lines)
6. ✅ **Extract data loading** into `src/tables/data/data-loading.js` (314 lines) — `loadData`, `autoSelectRow`, `getSelectedRowId`, `loadStaticData`
7. ✅ **Extract navigation** into `src/tables/navigation/navigation.js` (157 lines) — `navigateFirst`, `navigateLast`, `navigatePrevRec`, `navigateNextRec`, etc.
8. ✅ **Extract row selection** into `src/tables/selection/row-selection.js` (156 lines) — `handleRowSelected`, `selectDataRow`, `getEditingRow`, `isCalcRow`, `rowsMatch`
9. ✅ **Extract column management** into `src/tables/columns/column-management.js` (337 lines) — `discoverColumns`, `buildSelectorColumn`, `applyEditModeGate`, `_buildColumnsFromData`
10. ✅ **Extract event wiring** into `src/tables/events/table-events.js` (136 lines) — All Tabulator event handlers (`rowClick`, `cellClick`, `rowSelected`, etc.)
11. ✅ **Update `lithium-table-base.js` imports** and confirmed public API unchanged
12. ✅ **Tests** — All existing tests pass (618 passed)

### Gate

| Condition | Status |
|-----------|--------|
| `lithium-table-base.js` is under 1,000 lines | ✅ 872 lines (down from 2,129) |
| `lithium-table-ui.js` no longer has duplicate `applyPanelWidth` | ✅ Deleted 28-line duplicate |
| Each newly created module is under 750 lines | ✅ All 9 new modules under 400 lines |
| Full test suite passes | ✅ 618 tests passed |
| Clean `npm run lint` | ✅ No errors |
| Clean `npm run build` | ✅ Build successful |
| Docs updated | ✅ LITHIUM-TAB.md updated |

### Extracted Modules Summary

| Module | Lines | Responsibility |
|--------|-------|----------------|
| `visual/group-icon-animator.js` | 159 | Group expand/collapse arrow animation |
| `visual/column-tooltips.js` | 112 | Column header FloatingUI tooltips |
| `persistence/panel-width.js` | 97 | Panel width & collapsed state persistence |
| `refresh-orchestrator.js` | 149 | Table configuration reload orchestration |
| `template/capture.js` | 132 | Template state capture/extraction |
| `data/data-loading.js` | 314 | Data loading, row ID management, static data |
| `navigation/navigation.js` | 157 | Navigation methods (First, Last, Prev, Next, Page) |
| `selection/row-selection.js` | 156 | Row selection, matching, calc row detection |
| `columns/column-management.js` | 337 | Column discovery, building, merging, selector column |
| `events/table-events.js` | 136 | Tabulator event wiring (rowClick, cellClick, etc.) |

**Total extracted:** ~1,750 lines across 10 modules

### Docs to update on completion

- ✅ [LITHIUM-TAB.md](LITHIUM-TAB.md) — File Structure, Modular UI Architecture tables updated with all new modules
- ✅ [LITHIUM-TOC.md](LITHIUM-TAB.md) — no change required

---

## Phase 3 — Canonicalize the column property schema

**Status:** ✅ **COMPLETE** — April 18, 2026

**Goal:** Retire `display`; `title` is canonical. Wire `validateTableDef()` into every tableDef load path so schema drift is caught immediately.

### Prerequisites — docs to review

- [LITHIUM-TAB-TYPES.md](LITHIUM-TAB-TYPES.md) — the canonical property reference
- [LITHIUM-TAB-TYPES-DEFAULT.md](LITHIUM-TAB-TYPES-DEFAULT.md) — the default stanza (already uses `title`)
- [LITHIUM-TAB-TABLES.md](LITHIUM-TAB-TABLES.md) — Stage 1/2/3 examples (currently use `display` — must change)
- [LITHIUM-TAB.md](LITHIUM-TAB.md) — "Column Properties" section
- Acuranzo migrations 1153 and every 117X/118X tableDef migration (verify each)

### Work items

1. ✅ Decide and apply: **`title` is the canonical header property** everywhere in code, tests, docs, and migrations.
2. ✅ Remove `display` from:
   - `resolveColumn()` and `resolveColumns()` in `lithium-table.js`
   - `_buildColumnsFromData()` and `discoverColumns()` in `lithium-table-base.js`
   - `COLUMN_VALID_PROPS` in `lithium-table.js` (also fix the duplicate `'display'` entry)
   - Template extractors (in the new `template/capture.js` from Phase 2)
   - All Column Manager code paths
3. ✅ Update every affected **Acuranzo migration** (1153, 1156, 1157, 1179–1187) to emit `title` in the JSON. Version-bump the migration files and ensure their embedded `value_txt` remains accurate.
4. ✅ **Wire `validateTableDef()`** into `loadTableDef()` and `_buildColumnsFromData()`:
   - Unknown top-level properties → `log(Status.WARN)`, drop the property
   - Unknown column properties → `log(Status.WARN)`, drop the property
   - Unknown coltype name → `log(Status.ERROR)`, substitute `"string"`
   - Malformed structure (e.g., `columns` not an object) → `log(Status.ERROR)`, treat as empty
5. ✅ Add tests for the validator wiring — at least one test per failure mode.
6. ✅ Remove the declaration of `'display'` everywhere including the JSON schema in Lookup 59 Key 1 (migration 1154).

### Gate

| Condition | Status |
|-----------|--------|
| No source file contains `display` as a column property (grep clean) | ✅ Verified - only CSS `display` property and comments remain |
| All tests pass; tests that asserted `display` now assert `title` | ✅ 618 tests passed |
| Validator warnings appear in the session log when a bad tableDef is loaded | ✅ Wired in `loadTableDef()` at lines 327-330 and 424-427 |
| All listed migrations have been read and confirmed to use `title` | ✅ Verified |
| Clean lint and build | ✅ No errors |

### Docs to update on completion

- ✅ [LITHIUM-TAB-TABLES.md](LITHIUM-TAB-TABLES.md) — Stage 1/2/3 examples
- ✅ [LITHIUM-TAB-TYPES.md](LITHIUM-TAB-TYPES.md) — strike `display` from property reference
- ✅ [LITHIUM-TAB-TYPES-DEFAULT.md](LITHIUM-TAB-TYPES-DEFAULT.md) — already correct, verified
- ✅ [LITHIUM-TAB.md](LITHIUM-TAB.md) — "Column Properties" table
- ✅ [LITHIUM-FAQ.md](LITHIUM-FAQ.md) — add entry: "Canonical property names"

---

## Phase 4 — Refactor `lithium-table.js` and `lithium-table-ui.js`

**Status:** ✅ **COMPLETE** — April 17, 2026

**Goal:** Bring the remaining two oversized table-core files under 750 lines before Phases 5–17 add more work to them.

**Result:** 
- `lithium-table.js`: 1,253 → **482 lines** (-771 lines)
- `lithium-table-ui.js`: 1,128 → **731 lines** (-397 lines)

**Additional Phase 4 Part 2 changes:**
- Removed legacy column chooser (~95 lines)
- Extracted event handlers to `events/event-handlers.js` (~187 lines)
- Extracted Column Manager integration to `column-manager/cm-integration.js` (~114 lines)
- Removed `useColumnManager` option (no longer needed)

### Prerequisites — docs to review

- [LITHIUM-TAB.md](LITHIUM-TAB.md) — Modular UI Architecture table
- [LITHIUM-INS.md](LITHIUM-INS.md) §2, §3
- Current state of `src/tables/lithium-table.js` (1,245 lines)
- Current state of `src/tables/lithium-table-ui.js` (1,158 lines)

### Work items

1. ✅ **Split `lithium-table.js`** (the resolution engine). Extracted modules:
   - `tables/resolution/coltype-loader.js` — `loadColtypes`, `_coltypesCache`
   - `tables/resolution/tabledef-loader.js` — `loadTableDef`, `_tableDefCache`, `createAutoDiscoverTableDef`, `clearCache`, `clearLookup59Cache`, path-candidate helpers, `LOOKUPS_LOADED` subscription
   - `tables/resolution/lookup-loader.js` — `loadLookup`, `getLookup`, `resolveLookupLabel`, `createLookupFormatter`, `createLookupEditor`, `preloadLookups`
   - `tables/resolution/formatters.js` — `wrapFormatter`, `needsBlankZeroWrapper`, `formatBuiltinValue`, `LITHIUM_CALCULATIONS`, `lithiumCount/Sum/Avg/Min/Max`
   - `tables/resolution/validator.js` — `validateTableDef`, `TABLEDEF_VALID_PROPS`, `COLUMN_VALID_PROPS`, `VALID_COLTYPES`
   - `lithium-table.js` now re-exports from all above and retains `resolveColumn`, `resolveColumns`, `resolveTableOptions`, `getPrimaryKeyField`, `getQueryRefs` — the resolution API
2. ✅ **Split `lithium-table-ui.js`** (the UI mixin):
   - Removed no-op stubs: `showNavPopup`, `positionNavPopup`, `buildStandardNavPopup`, `refreshTemplatePopup`, `createTemplateMenuAction`, `getPopupItems`
   - Moved `generateTemplateJSON` to `template/capture.js` as the canonical extractor
   - The mixin now delegates to the capture module
3. ✅ Updated all imports across `src/`, `tests/` — all imports still go through `lithium-table.js` which re-exports
4. ✅ No circular imports introduced.

### Gate

| Condition | Status |
|-----------|--------|
| `lithium-table.js` under 750 lines | ✅ 482 lines |
| `lithium-table-ui.js` under 750 lines | ✅ 731 lines |
| Each new module under 750 lines | ✅ All modules under 400 lines |
| All tests pass | ✅ 21 test files passed, 622 tests |
| Clean lint | ✅ No errors |
| Clean build | ✅ Build successful |
| No circular imports | ✅ Verified |
| Legacy column chooser removed | ✅ Deleted from lithium-table-ui.js |
| Event handlers extracted | ✅ Moved to events/event-handlers.js |
| Column Manager integration extracted | ✅ Moved to column-manager/cm-integration.js |

### Docs to update on completion

- ✅ [LITHIUM-TAB.md](LITHIUM-TAB.md) — File Structure, Modular UI Architecture tables updated with:
  - All resolution modules
  - events/event-handlers.js
  - column-manager/cm-integration.js

---

## Phase 5 — Single canonical template extractor

**Status:** ✅ **COMPLETE** — April 17, 2026

**Goal:** One function produces a tableDef column entry from a live Tabulator column. Every caller uses it.

### Prerequisites — docs to review

- [LITHIUM-TAB.md](LITHIUM-TAB.md) — Templates section
- [LITHIUM-COL.md](LITHIUM-COL.md) — Column Manager
- [LITHIUM-TAB-TABLES.md](LITHIUM-TAB-TABLES.md) — especially the tableDef column property list
- The new `template/capture.js` from Phase 2
- `src/tables/lithium-table-template.js` (previous extractor)
- Column Manager's `cm-actions.js` and `cm-columns.js`

### Work items

1. **Audit the extractors that currently exist** after Phase 2's extraction:
   - `template/capture.js` (ex-base.js, no whitelist — copies everything including `lithium*` echoes)
   - `lithium-table-template.js` (has a whitelist but different shape)
   - The `generateTemplateJSON` path that landed in `template/capture.js` in Phase 4
2. **Define the canonical output shape.** A tableDef column entry contains:
   - Canonical Tabulator properties (from `COLUMN_VALID_PROPS`, after Phase 3): `title`, `field`, `width`, `minWidth`, `maxWidth`, `hozAlign`, `vertAlign`, `visible`, `formatter`, `formatterParams`, `editor`, `editorParams`, `sorter`, `sorterParams`, `headerSort`, `headerFilter`, `headerFilterFunc`, `headerFilterPlaceholder`, `headerFilterParams`, `headerHozAlign`, `headerVertical`, `headerSortTristate`, `resizable`, `frozen`, `rowHandle`, `cssClass`, `tooltip`, `headerTooltip`, `bottomCalc`, `bottomCalcFormatter`, `bottomCalcFormatterParams`
   - Lithium-only properties: `coltype`, `columnPri`, `primaryKey`, `calculated`, `description`, `lookupRef`, `lookupStyle`, `lookupEdit`
   - No function references. No `lithium*` echoes. No `_`-prefixed internals.
3. **Build a single `extractTableDefColumn(column, originalColDef, options)`** function. Inputs: the Tabulator column component (runtime state), the original tableDef colDef (base JSON, source of Lithium-only properties), options for `{ includeRuntimeOnly, includeHiddenColumns }`. Output: the canonical JSON-serializable column entry.
4. **Build a companion `extractTableDef(table, tableDef, options)`** function that returns the full table-level object (including table-wide properties) by iterating columns via `extractTableDefColumn`.
5. **Replace every caller** of the now-obsolete extractors. Delete the obsolete code.
6. **Round-trip test**: tableDef → resolved → Tabulator → extracted → compared. Equal, modulo explicit runtime-only exclusions.

### Gate

| Condition | Status |
|-----------|--------|
| Every extractor code path funnels through `extractTableDefColumn()` | ✅ Implemented in `template/capture.js` |
| Round-trip test passes | ✅ 631 tests passed |
| Saving a template produces clean JSON | ✅ No functions, no `lithium*` noise, no `_`-prefixed internals |
| `overrides` pattern eradicated | ✅ All properties now flattened |
| Lithium metadata moved to `_columnMeta` | ✅ Eliminates Tabulator console warnings |
| Clean lint and build | ✅ Pass |

### Completed Work

1. ✅ **Created `extractTableDefColumn()`** — Single canonical extractor with `CANONICAL_COLUMN_PROPS` whitelist
2. ✅ **Created `extractTableDef()`** — Full table extraction with options support
3. ✅ **Created `generateMigrationSeed()`** — Migration-ready JSON output (groundwork for Phase 15)
4. ✅ **Moved metadata to `_columnMeta`** — Lithium-specific properties stored separately from Tabulator columns
5. ✅ **Eliminated `lithium*` properties** — No more console warnings from Tabulator
6. ✅ **Flattened `overrides` pattern** — Properties are now directly on column definitions
7. ✅ **Deleted `lithium-table-template.js`** — All functionality consolidated into `template/capture.js`
8. ✅ **Updated Column Manager** — Uses `_columnMeta` and canonical extractor
9. ✅ **Updated tests** — All 631 tests pass

### Docs to update on completion

- ✅ [LITHIUM-TAB.md](LITHIUM-TAB.md) — Phase 5 completion noted in Implementation History
- ✅ [LITHIUM-COL.md](LITHIUM-COL.md) — Added "Property Structure (Phase 5)" section with flattened properties and `_columnMeta` documentation

---

## Phase 6 — Fix three-stage merge semantics ✅ COMPLETED

**Goal:** The three-stage merge produces what the docs promise — a complete, usable tableDef at every stage, with deep-merged params and correct overlay order.

### Prerequisites — docs to review

- [LITHIUM-TAB-TABLES.md](LITHIUM-TAB-TABLES.md) — the "Stage 1/2/3" narrative
- [LITHIUM-TAB.md](LITHIUM-TAB.md) — Column Definition Merge Order
- `lithium-table-base.js` `initTable` method (the merge site)
- `lithium-table-base.js` `_applyDefaultTemplate` (deep merge fix)
- `lithium-table-base.js` `_mergeColumnsWithTableDef` (deep merge fix)

### Work items — COMPLETED

1. ✅ **Deep merge nested param objects** across stage overlays: Added `deepMergeParams()` helper with 10 param keys (`formatterParams`, `editorParams`, `headerFilterParams`, `sorterParams`, `accessorParams`, `mutatorParams`, `bottomCalcFormatterParams`, `downloadFormatterParams`, `downloadCalcParams`, `clipboardParams`). Scalar properties and arrays continue to replace (not merge).
2. ✅ **Applied deep merge to `_applyDefaultTemplate`**: Default templates now use `deepMergeParams()` instead of shallow spread, preserving coltype defaults.
3. ✅ **Sequential `columnPri` retained**: Kept `fieldIndex + 1` for explicit ordering (clarified with user — intentional design).
4. ✅ **Remove the double-discover**: Deleted `dataLoaded` callback for `_autoDiscover` tables; `loadData()` already calls `discoverColumns()`.
5. ✅ **Added auth logout hook**: TableDef cache clears on `AUTH_LOGOUT` event for true session-only caching.
6. ✅ **Fixed row selection after refresh**: Modified `reloadConfiguration()` to explicitly restore the captured row ID after data load. Disabled `autoSelectRow` during `loadData` (to prevent first-row fallback), then explicitly called `autoSelectRow(capturedRowId)` after data loaded.
7. ✅ **Fixed refresh clearing all tableDefs**: Fixed `refreshTabulatorSchemas()` in `lookups.js` to use `authQuery()` instead of `fetchBatchQueries()`. The previous implementation didn't include JWT authentication, causing the server to return empty results and corrupting the schemas cache.
8. ✅ **Fixed parent/child table refresh coordination**: Added `onRefreshComplete()` callback to `reloadConfiguration()`. Managers with parent/child tables (like Lookups Manager) can clear child state in this callback, ensuring child data reloads when parent refresh completes.
9. ✅ **Added tests**:
   - Deep-merge of `formatterParams` across stages
   - `mergeColumnsWithTableDef` with param preservation
   - All 10 DEEP_MERGE_PARAM_KEYS verified

### Implementation Notes

- **Architecture clarification**: `_applyDefaultTemplate` stays in `loadConfiguration()` (Stage 2.5), not moved to post-render. It modifies tableDef before Tabulator instantiation, which is the correct and efficient approach.
- **Hard refresh flow preserved**: `reloadConfiguration()` still clears cache, reloads from Lookup 59, and reapplies captured state — now with deep merge ensuring param objects combine properly.
- **Row selection fix**: After data load, we explicitly restore selection using the `capturedRowId` from before the refresh. We disable `autoSelectRow` during `loadData` to prevent it from selecting the first row, then explicitly restore the correct row afterward.

### Gate — ALL PASSED

- ✅ All tests pass (642 tests).
- ✅ Clean lint and build.

### Docs updated

- ✅ [LITHIUM-TAB-TABLES.md](LITHIUM-TAB-TABLES.md) — Added "Deep Merge for Param Objects" section with complete param key list and examples
- ✅ [LITHIUM-TAB.md](LITHIUM-TAB.md) — Updated Column Definition Merge Order with deep merge bullet list
- ✅ [LITHIUM-FAQ.md](LITHIUM-FAQ.md) — Added entries for param merge fix, double-discover fix, cache logout fix, and row selection fix

---

## Phase 7 — Fix string editing ✅ COMPLETED

**Goal:** Editing a string cell works end to end — via Enter, F2, double-click, and the Edit button.

### Prerequisites — docs to review

- [LITHIUM-TAB.md](LITHIUM-TAB.md) — Edit Mode section
- [LITHIUM-MGR-QUERY.md](LITHIUM-MGR-QUERY.md) — the first manager with editable strings
- `lithium-table-base.js` — `applyEditModeGate`, `initTable` editTriggerEvent setting
- `lithium-table-ops.js` — `enterEditMode`, `queueCellEdit`, `openActiveCellEditor`, `commitActiveCellEdit`
- `lithium-table.js` — `resolveColumn` editor-assignment block

### Work items — COMPLETED

1. ✅ **Reproduced the failure:** String columns would not open editors; clicking cells would cancel edit mode.
2. ✅ **Identified root causes:**
   - TableDefs used `editor: "string"` instead of `editor: "input"` — Tabulator doesn't recognize `"string"` as a valid editor type
   - Edit button had toggle behavior instead of "save and exit" behavior
   - `cellMouseDown` was interfering with edit mode by selecting rows during editing
   - Same-row clicks were triggering edit mode exit
   - CodeMirror was capturing ESC and Ctrl+Enter keys
   - **Composite primary keys not handled correctly** — `editingRowId` was `undefined` for tables with composite keys
3. ✅ **Applied fixes:**
   - Changed table definitions to use `editor: "input"` for string columns
   - Modified `handleEdit()` to always exit edit mode when clicked while editing (save if dirty, then exit)
   - Modified `cellMouseDown` to skip row selection when already in edit mode
   - Enhanced `cellClick` to only select row when clicking a different row during edit mode
   - Added defensive check in `handleRowSelected` to prevent exit on same-row selection
   - Added CodeMirror keymap bindings for Escape (cancel) and Ctrl+Enter (save)
   - **Fixed composite key handling** — Updated `enterEditMode()` and `queueCellEdit()` to use `_getCompositeRowId()`
   - Added `commitActiveCellEdit()` call before dirty check in `handleRowSelected` to ensure cell changes are captured
4. ✅ **Verified via entry points:** Edit button, double-click, Enter key all work correctly.
5. ✅ **Tests pass:** All 642 tests pass.

### Implementation Notes

**Edit Button Behavior Change:**
```javascript
// Before: Toggle behavior with row ID check
if (this.isEditing && this.editingRowId === selectedId) {
  if (actuallyDirty) await this.handleSave();
  else await this.exitEditMode('toggle');
}

// After: Always exit when in edit mode
if (this.isEditing) {
  if (actuallyDirty) await this.handleSave();
  else await this.exitEditMode('toggle');
}
```

**Composite Key Handling:**
```javascript
// Before (broken):
const pkField = this.primaryKeyField || 'id';
const rowId = row.getData()?.[pkField]; // undefined when pkField is an array

// After (fixed):
const pkFields = this.primaryKeyField;
const rowId = this._getCompositeRowId(row.getData(), pkFields); // "43::11"
```

**Cell Interaction Improvements:**
- `cellMouseDown` no longer selects rows when in edit mode (prevents interference)
- `cellClick` now distinguishes between same-row clicks (stay in edit mode, open editor) and different-row clicks (trigger auto-save via `handleRowSelected`)
- `rowClick` also checks for same-row to prevent unwanted row change logic

### Gate — ALL PASSED

- ✅ All three entry points open the editor for a string cell
- ✅ Typing updates cell value; clicking away commits
- ✅ Escape cancels (from both table and CodeMirror)
- ✅ Ctrl+Enter saves from CodeMirror
- ✅ Edit button toggles edit mode correctly (enter/exit)
- ✅ Clicking same-row cells stays in edit mode
- ✅ Composite primary keys work correctly
- ✅ Changes save when clicking different row
- ✅ Tests pass (642 tests)
- ✅ `LITHIUM-FAQ.md` updated with root causes
- ✅ Clean lint and build

### Docs updated

- ✅ [LITHIUM-FAQ.md](LITHIUM-FAQ.md) — Added "Phase 7 — String Editing Fixes" section with root causes and fixes
- ✅ [LITHIUM-TAB.md](LITHIUM-TAB.md) — Edit Mode interaction flow documented

---

## Phase 8 — Fix lookup-coltype editing (COMBINED with Phase 11)

**Status:** ✅ **COMPLETE** — April 18, 2026

**Goal:** Editing a column with `coltype: "lookup"` opens a dropdown populated from the referenced Lookup and commits the correct integer ID (not the label). Also implements full icon support for lookup columns.

This phase was combined with Phase 11 (Lookup coltype family expansion) because the work naturally overlapped once we understood the requirements.

### What was delivered

**Core lookup editing fix:**
- Fixed `createLookupEditor()` to return `{id: label}` map instead of `[labels]` array — this ensures selecting a label stores the integer ID, not the label text
- Added mutator to ensure lookup values are always stored as integers (handles edge case where Tabulator returns label string)
- Changed fallback from `'input'` to `'number'` editor when lookup data unavailable
- Normalized `lookupRef` to string for consistent cache keys (handles both `27` and `"27"`)

**Icon support (Phase 11 scope):**
- Extended `loadLookup()` to extract `icon` from `collection.icon` in lookup data
- Added `createIconFormatter()` for icon-only cell display
- Added `lookupStyle` property: `"icon"` | `"label"` (default)
- Added `lookupEdit` property: `"icon"` | `"iconLabel"` | `"label"` (default)
- Six display combinations supported (A-F): icon/iconLabel/label for both cell and dropdown
- CSS styling for flexbox alignment and hozAlign support in dropdowns
- Zero value handling (lookup ID 0 is valid and displays correctly)

**State persistence:**
- Added `lookupStyle` and `lookupEdit` to `CANONICAL_COLUMN_PROPS`
- Added to `extractColumnMeta()` so these properties survive template saves and refresh

### Files changed

| File | Changes |
|------|---------|
| `src/tables/resolution/lookup-loader.js` | Icon extraction, ID→label map, number fallback |
| `src/tables/lithium-table.js` | `lookupStyle`/`lookupEdit` support, mutator, CSS classes |
| `src/tables/template/capture.js` | Added `lookupStyle`/`lookupEdit` to canonical props |
| `src/styles/vendor-fixes.css` | Lookup dropdown styling, flexbox alignment |
| `tests/unit/lithium-table.test.js` | Updated tests for new values format |

### Test results

- 642 tests pass
- Clean lint
- Clean build
- Manually verified with Query Manager (`query_type_a28`, `query_status_a27`, `query_dialect_a30`)

### Docs to update on completion

- ✅ `LITHIUM-TAB-TYPES-LOOKUP.md` — updated with `lookupStyle` and `lookupEdit` properties
- ✅ `Phase-8-11-Plan.md` — comprehensive implementation notes (created during development)
- ✅ `LITHIUM-FAQ.md` — root cause entry for the label-vs-ID bug

---

## Phase 9 — Grouping, sorting, and filtering trifecta

**Goal:** The three discoverable table operations (group, sort, filter) are first-class in the tableDef and in the runtime, with `groupable`/`groupPri`/`groupOrd`/`sortPri` properly implemented.

The Column Manager UI for these lands in later phases (16–17); this phase is about the data path.

### Prerequisites — docs to review

- [LITHIUM-TAB-TYPES.md](LITHIUM-TAB-TYPES.md) — Grouping Properties, Sorting Properties, Filtering Properties
- [LITHIUM-TAB-TABLES.md](LITHIUM-TAB-TABLES.md) — table-level properties
- `lithium-table.js` `resolveTableOptions` (currently uses single `group` property)
- Acuranzo migration 1153 (coltype defaults)

### Work items

1. **Implement `groupable`, `groupPri`, `groupOrd`:**
   - `groupable: boolean` — column is eligible for grouping
   - `groupPri: number | null` — priority when multiple columns group simultaneously (1 = outermost)
   - `groupOrd: 'asc' | 'desc'` — sort direction within each group
2. **Update `resolveTableOptions`** to build `groupBy` from columns where `groupable === true && groupPri != null`, sorted by `groupPri`. Retire the legacy single `group` property.
3. **Implement `sortPri`** for multi-column initial sort. Build `initialSort` from columns where `sortPri != null`, sorted by `sortPri`, with direction from `groupOrd` (for grouped cols) or a new column-level `sortDir` (default `asc`). An explicit `tableDef.initialSort` array still works and wins if both are present.
4. **Keep header-filter behavior as-is for now.** Filtering is already per-column via `filter: true`; a multi-column filter priority (`filterPri`) is deferred unless we find a concrete need.
5. **Update Lookup 59 Key 0 (migration 1153)** to advertise `groupable: false`, `groupPri: null`, `groupOrd: "asc"`, `sortPri: null`, `sortDir: "asc"` in the default stanza.
6. **Update the JSON Schema (migration 1154)** to validate the new property names.
7. **Remove the old `group` property** from any tableDef migration that uses it.
8. **Tests:**
   - A tableDef with `groupable + groupPri` on two columns produces nested groups
   - A tableDef with `sortPri` on two columns produces a multi-column initial sort
   - `groupOrd` controls within-group sort order
   - The existing group-arrow animation still works

### Gate

- Tests pass.
- A manually edited tableDef (via the Lookups Manager — editing Lookup 59 key 5's JSON) with the new properties renders correctly.
- No regressions in Queries Manager or Lookups Manager.
- Clean lint and build.

### Docs to update on completion

- [LITHIUM-TAB-TYPES.md](LITHIUM-TAB-TYPES.md) — Grouping, Sorting sections rewritten
- [LITHIUM-TAB-TABLES.md](LITHIUM-TAB-TABLES.md) — example updates
- [LITHIUM-TAB.md](LITHIUM-TAB.md) — Column Properties

---

## Phase 10 — Primary key discipline

**Goal:** Every table declares its primary key. The silent fallback chain becomes a loud warning, then is removed entirely.

### Prerequisites — docs to review

- [LITHIUM-TAB.md](LITHIUM-TAB.md) — Primary Key Configuration
- All Acuranzo tableDef migrations (1179–1187, 1156–1157)
- `lithium-table-base.js` `_getCompositeRowId` fallback chain
- `lithium-table.js` `getPrimaryKeyField`

### Work items

1. **Audit every tableDef migration** for `primaryKey: true` flags. Fix any missing.
2. **Downgrade the fallback chain in `_getCompositeRowId` to a warning** first: when the chain picks a field because `primaryKeyField` is null/empty, `log(Status.WARN)` with the picked field name and the tablePath.
3. **Run the app** and observe the session log. Confirm zero warnings after the migration audit is done.
4. **Remove the fallback entirely.** Tables without a declared primary key either:
   - Declare one now (preferred)
   - Opt in explicitly with `primaryKey: false` on all columns → no row-selection persistence (loud info message at load)
5. **Simplify `rowsMatch`, `autoSelectRow`, `saveSelectedRowId`, and the composite-ID construction** around the assumption that `primaryKeyField` is a known array or explicitly empty.

### Gate

- No source code reads from `['id', 'query_id', 'lookup_id', ...]` fallback arrays.
- Every migration listed in the reference table declares a primary key.
- Session log has no primary-key-fallback warnings during normal use.
- Tests pass.
- Clean lint and build.

### Docs to update on completion

- [LITHIUM-TAB.md](LITHIUM-TAB.md) — Primary Key Configuration says declaration is mandatory
- [LITHIUM-MGR-NEW.md](LITHIUM-MGR-NEW.md) — new-manager checklist reminds to declare primary key

---

## Phase 11 — Lookup coltype family expansion (icon variants)

**Status:** ✅ **COMPLETE** — Combined with Phase 8, April 18, 2026

**Goal:** The lookup variants render and edit correctly, with `lookupStyle` and `lookupEdit` controlling cell/dropdown display independently.

**Note:** This phase was merged into Phase 8 once we understood the requirements better. Rather than four separate coltypes, we implemented a single `lookup` coltype with `lookupStyle` and `lookupEdit` properties that control display modes.

### Implementation approach

Instead of separate coltypes (`lookupIcon`, `lookupIconText`, `lookupIconList`), we use:

| Combination | Cell Display | Dropdown Display | Column Properties |
|-------------|--------------|------------------|-------------------|
| A | Icon | Icon | `lookupStyle: "icon"`, `lookupEdit: "icon"` |
| B | Icon | Icon+Label | `lookupStyle: "icon"`, `lookupEdit: "iconLabel"` |
| C | Icon | Label | `lookupStyle: "icon"`, `lookupEdit: "label"` |
| D | Label | Icon | `lookupStyle: "label"`, `lookupEdit: "icon"` |
| E | Label | Icon+Label | `lookupStyle: "label"`, `lookupEdit: "iconLabel"` |
| F | Label | Label | `lookupStyle: "label"` (default) or omitted |

This provides the same six display combinations with simpler configuration and less code duplication.

### What was implemented

- `lookupStyle` property: `"icon"` | `"label"` (default)
- `lookupEdit` property: `"icon"` | `"iconLabel"` | `"label"` (default)
- `createIconFormatter()` for icon-only cell display
- `itemFormatter` for dropdown with icons
- hozAlign support in dropdowns
- Zero value handling for lookup ID 0

---

## Phase 12 — Date/time coltypes: Luxon integration

**Goal:** `date`, `datetime`, and `time` coltypes format and parse correctly via Luxon. (Editor improvements land in Phase 13.)

### Prerequisites — docs to review

- [LITHIUM-TAB-TYPES-DATE.md](LITHIUM-TAB-TYPES-DATE.md)
- [LITHIUM-TAB-TYPES-DATETIME.md](LITHIUM-TAB-TYPES-DATETIME.md)
- [LITHIUM-TAB-TYPES-TIME.md](LITHIUM-TAB-TYPES-TIME.md)
- [LITHIUM-LIB.md](LITHIUM-LIB.md) — library list (add Luxon here)
- [LITHIUM-MGR-USERPROFILE.md](LITHIUM-MGR-USERPROFILE.md) — locale/format preferences

### Work items

1. **Add `luxon`** to `package.json`. Add entry to `LITHIUM-LIB.md`.
2. **Design property surface** for date/time coltypes:
   - `format` — Luxon display format (defaults: date = `yyyy-MM-dd`, datetime = `yyyy-MM-dd HH:mm:ss`, time = `HH:mm:ss`)
   - `inputFormat` — for parsing incoming data (defaults to ISO)
   - `timezone` — optional IANA zone
3. **Implement a custom formatter** in the formatters module that uses Luxon to parse `inputFormat` and format `format`. Handle null/empty.
4. **Implement a matching sorter** that compares Luxon `DateTime` objects; accessor converts incoming data to ISO once so Tabulator's built-in `datetime` sorter works too.
5. **Respect user locale.** When `format` is the sentinel string `"user"`, read the user's preferred date/time format from Profile Manager settings (account_settings — migrations 1175–1177). Plumb this through the app instance.
6. **Update migration 1153** (coltype defaults) with the new properties.
7. **Tests** — Luxon parsing and formatting across multiple sample values, including zone conversions and null handling.

### Gate

- Tables with `date`/`datetime`/`time` coltypes render formatted values.
- Sorting and filtering work (filtering uses the raw input input for now; Phase 13 adds picker-based filtering).
- Tests pass.
- Clean lint and build.

### Docs to update on completion

- [LITHIUM-TAB-TYPES-DATE.md](LITHIUM-TAB-TYPES-DATE.md), `-DATETIME.md`, `-TIME.md` — reflect implementation
- [LITHIUM-LIB.md](LITHIUM-LIB.md) — Luxon entry with rationale
- [LITHIUM-TAB-TYPES.md](LITHIUM-TAB-TYPES.md) — date/time section

---

## Phase 13 — Date/time coltypes: Flatpickr editor integration

**Goal:** Editing a date/datetime/time cell opens a Flatpickr picker. Header filters for these columns use pickers too.

### Prerequisites — docs to review

- The same date/time docs from Phase 12
- [LITHIUM-TAB.md](LITHIUM-TAB.md) — Edit Mode (for editor lifecycle)
- [LITHIUM-CSS.md](LITHIUM-CSS.md) — for picker theming
- `src/tables/filters/filter-editor.js`

### Work items

1. **Add `flatpickr`** to `package.json`. Add entry to `LITHIUM-LIB.md`.
2. **Design the editor property**: `pickerOptions` — pass-through to Flatpickr. Sensible defaults per coltype (enableTime for datetime, noCalendar for time, etc.).
3. **Implement a custom Tabulator editor** per coltype that mounts Flatpickr inside the cell, handles commit (Flatpickr's `onClose`), and cancel (Escape → close without commit).
4. **Implement a Flatpickr-backed header filter** for these coltypes. Reuse the editor structure.
5. **Theme Flatpickr** to match Lithium's dark theme. Prefer CSS classes + variables; don't inline-style. Lithium's CSS variables and dark-mode-first theming take precedence over Flatpickr's defaults.
6. **Tests** — editor mount, commit, cancel; filter mount and value commit.

### Gate

- Date/datetime/time cells open Flatpickr in edit mode.
- Committing the picker updates the cell; Escape cancels.
- Header filters for these columns use picker UI.
- Picker styling matches the app theme.
- No regressions in Phase 12 formatting.
- Tests pass.
- Clean lint and build.

### Docs to update on completion

- Date/time type docs — add picker notes
- [LITHIUM-LIB.md](LITHIUM-LIB.md) — Flatpickr entry
- [LITHIUM-CSS.md](LITHIUM-CSS.md) — picker theming notes

---

## Phase 14 — Navigator Refresh button isolation

**Goal:** Refresh in one LithiumTable instance does not disturb any other instance.

### Prerequisites — docs to review

- [LITHIUM-TAB.md](LITHIUM-TAB.md) — Refresh Button Enhanced Behavior
- `src/tables/refresh-orchestrator.js` (from Phase 2)
- `lithium-table.js` or its Phase-4 split — the `_tableDefCache` and its event subscriptions
- `src/shared/lookups.js` — `refreshTabulatorSchemas` and the `LOOKUPS_LOADED` event

### Work items

1. **Reproduce** in Lookups Manager (dual-table): select a row in parent, edit something in child, click Refresh in child — observe what happens in parent.
2. **Audit every global event listener** in `src/tables/`. Document each: what triggers it, what it touches, whether it affects all tables or just the triggering table. Expected outcomes:
   - `LOOKUPS_LOADED` → `clearLookup59Cache()` is currently global. Options:
     - Keep the cache clear (other tables will lazy-refetch) but ensure the other tables **stay operational during the refetch** — no teardown
     - Or, scope the clear to only the tableDef the Refresh is aimed at
3. **Separate cache-clear from instance-reset.** Clearing a cache entry should not trigger other instances to rebuild. Rebuild happens only when a specific instance's user requests it.
4. **Audit other nav buttons** (Filter, Menu, Width, Layout, Template) for any cross-instance reach.
5. **Test** — a minimal two-instance harness that exercises Refresh in one, then asserts zero mutation in the other.

### Gate

- Refresh in one LithiumTable never mutates visible state of another LithiumTable.
- Dual-table managers (Lookups) operate cleanly through parent/child refresh cycles.
- Tests pass.
- Clean lint and build.

### Docs to update on completion

- [LITHIUM-TAB.md](LITHIUM-TAB.md) — Refresh Button Enhanced Behavior clarifies instance-local scope
- [LITHIUM-FAQ.md](LITHIUM-FAQ.md) — root cause entry

---

## Phase 15 — Exhaustive tableDef generation from live tables

**Goal:** The single extractor from Phase 5 captures *everything* needed to round-trip a live table — to save a template, populate the Column Manager, or seed a database migration.

### Prerequisites — docs to review

- [LITHIUM-TAB-TABLES.md](LITHIUM-TAB-TABLES.md) — all documented table-level and column-level properties
- [LITHIUM-COL.md](LITHIUM-COL.md) — Column Manager requirements
- `template/capture.js` from Phases 2/5
- Phase 9's new group/sort properties

### Work items

1. **Catalog what we need to capture.** Start with the full property list from Phase 3 and Phase 9's additions. Match against the extractor's current output; list gaps.
2. **Capture table-level runtime state:**
   - `layout` (current mode)
   - `tableWidthMode`
   - `initialSort` (from `table.getSorters()`)
   - `_filterValues` (from `table.getHeaderFilters()`)
   - `_filtersVisible`
   - Any group-expand state we want round-trippable
3. **Capture column-level runtime state:**
   - Current width (runtime, after user resize)
   - Current visibility
   - Current `columnPri` (as displayed after user reorder)
   - Current `groupable`/`groupPri`/`sortPri` values (preserved from original colDef — Tabulator doesn't know about these)
4. **Capture Lithium-only properties** via documented `lithium*` echo properties on the Tabulator definition: `lithiumColtype`, `lithiumPrimaryKey`, `lithiumCalculated`, `lithiumDescription`, `lithiumGroupable`, `lithiumGroupPri`, `lithiumGroupOrd`, `lithiumSortPri`, `lithiumSortDir`, `lithiumLookupRef`, `lithiumLookupStyle`, `lithiumLookupEdit`. Add echoes for any Lithium-only properties not already echoed in `resolveColumn`.
5. **Build a `generateMigrationSeed()` helper** that outputs the JSON stanza suitable for pasting into an `acuranzo_11XX.lua` migration: drops `_templateMeta` and all `_`-prefixed runtime flags, includes the `$schema` reference, uses stable key ordering.
6. **Add a "Copy as Migration Seed" action** to the Column Manager or Template popup.
7. **Round-trip test.** tableDef → resolved → user edits (hide column, resize another, sort by two columns) → generated → parsed → resolved. Second resolution matches the post-edit state.

### Gate

- Round-trip test passes.
- Manual: open Queries Manager, make visible edits, click "export". Paste into a new tableDef; hand-edit into Lookup 59 Key 5; refresh the app — the table renders exactly as edited.
- Tests pass.
- Clean lint and build.

### Docs to update on completion

- [LITHIUM-COL.md](LITHIUM-COL.md) — documents "Column Manager as tableDef editor" philosophy
- [LITHIUM-TAB-TABLES.md](LITHIUM-TAB-TABLES.md) — new section on using generation to seed migrations
- [LITHIUM-TAB.md](LITHIUM-TAB.md) — Templates section references the exhaustive generator

---

## Phase 16 — Column Manager cleanup

**Goal:** The Column Manager (and the nested Column Manager Manager) behave as ordinary LithiumTable instances, with only the necessary differences (draggable popup, always-editable cells, host-owned save/cancel).

### Prerequisites — docs to review

- [LITHIUM-COL.md](LITHIUM-COL.md)
- [LITHIUM-TAB.md](LITHIUM-TAB.md) — the `alwaysEditable` notes
- Migrations 1179 (column-manager), 1180 (column-manager-manager)
- All files under `src/tables/column-manager/`

### Work items

1. **Audit every use of `alwaysEditable`** in the base class and mixins. Each one is a special case. Document whether it's genuinely necessary:
   - Skipping footer save/cancel — yes, host owns this
   - Skipping Add/Duplicate/Edit/Delete nav buttons — yes, those aren't for column manipulation
   - Skipping Enter/F2/double-click edit entry — yes, cells are always-editable
   - Anything else — review individually
2. **Confirm the Column Manager uses the same three-stage resolution** (Phase 6) from its tableDef (Lookup 59 Key 2). No hand-built column arrays outside of what the resolver produces.
3. **Confirm the Column Manager uses Phase 5's single extractor** for its save-changes path.
4. **Confirm the Column Manager's Navigator works the same** (templates, width preset, layout) — the Column Manager deserves its own template list.
5. **Confirm the Column Manager Manager** (Lookup 59 Key 3) loads and edits correctly.
6. **Retire any cruft** found during the audit.

### Gate

- Opening the Column Manager works from any LithiumTable.
- Opening the Column Manager Manager from inside the Column Manager works.
- Editing column properties in the Column Manager saves correctly to the parent table.
- The `alwaysEditable` codepath is minimal and documented.
- Tests pass.
- Clean lint and build.

### Docs to update on completion

- [LITHIUM-COL.md](LITHIUM-COL.md) — "Column Manager is a LithiumTable" framing + enumerated list of differences
- [LITHIUM-TAB.md](LITHIUM-TAB.md) — `alwaysEditable` section updated

---

## Phase 17 — Table-wide settings section in the Column Manager

**Goal:** The Column Manager has a first-class UI area for properties that are *table-wide* rather than *per-column*.

### Prerequisites — docs to review

- [LITHIUM-COL.md](LITHIUM-COL.md)
- Phase 9's work on `sortPri`/`groupPri`
- Phase 15's exhaustive tableDef shape
- Current Column Manager popup layout

### Work items

1. **Identify the table-wide properties** that need UI:
   - `layout` (fitColumns, fitData, fitDataFill, fitDataTable)
   - Width preset
   - `readonly` (locking the whole table)
   - `persistSort`, `persistFilter`
   - `selectableRows` — kept at 1; probably not exposed
   - Group-start-open default
   - Any cross-column sort/filter/group priorities that didn't land per-column
2. **Design a compact header section** for the Column Manager popup that shows these settings, editable inline. Use CSS classes from the existing manager-UI toolkit; no inline styles.
3. **Implement the UI** and wire each field to the appropriate LithiumTable method (`setTableLayout`, `setTableWidth`, etc.) and to the tableDef being edited.
4. **Ensure the table-wide settings are captured** by Phase 15's exhaustive extractor.
5. **Tests** for the UI's field-to-tableDef round-trip.

### Gate

- Column Manager popup shows the table-wide settings area.
- Changing a setting updates the table immediately.
- The change is persisted in the saved template.
- The change round-trips through Phase 15's extractor.
- Tests pass.
- Clean lint and build.

### Docs to update on completion

- [LITHIUM-COL.md](LITHIUM-COL.md) — new UI section documented with a screenshot placeholder
- [LITHIUM-TAB.md](LITHIUM-TAB.md) — cross-reference

---

## Phase 18 — CSS file audit and refactor

**Goal:** Bring `lithium-table.css` (1,059 lines) and `lithium-column-manager.css` (773 lines) under 750 lines each by splitting along responsibility lines. No visual change.

### Prerequisites — docs to review

- [LITHIUM-CSS.md](LITHIUM-CSS.md) — architecture, layers, variables
- [LITHIUM-INS.md](LITHIUM-INS.md) §6 (CSS reuse)
- Current CSS files

### Work items

1. **Audit `lithium-table.css`** for extraction candidates:
   - Navigator styles → `navigator.css`
   - Popup styles → `table-popups.css`
   - Filter styles → `table-filters.css`
   - Edit-mode styles → `table-edit-mode.css`
   - Group/calc-row styles → `table-groups.css`
   - Core table container + selector column stays in `lithium-table.css`
2. **Audit `lithium-column-manager.css`** similarly.
3. **Extract along responsibility lines**, using `@import` or bundler-level concatenation to preserve load order. Confirm CSS layer ordering (`@layer` usage) is preserved.
4. **Remove any orphaned rules** that don't match live HTML (quick grep for selectors not found anywhere in `src/`).
5. **Confirm no visual regressions** — eyeball Queries, Lookups, Style, Column Manager.

### Gate

- Every CSS file under 750 lines.
- No visual regressions (manual smoke check).
- Clean lint and build.

### Docs to update on completion

- [LITHIUM-CSS.md](LITHIUM-CSS.md) — new file list, responsibility map
- [LITHIUM-TAB.md](LITHIUM-TAB.md) — CSS Architecture section

---

## Phase 19 — Test coverage and documentation closure

**Goal:** Close the loop. Tests cover the new surfaces. Docs are consistent. The plan is archived.

### Prerequisites — docs to review

- All of `docs/Li/`
- `npm run test:coverage` output after this phase's test additions

### Work items

1. **Run the full test suite** and capture coverage. Identify new gaps from Phases 2–18.
2. **Add integration-level tests** exercising end-to-end flows:
   - Load a manager with a saved template
   - Edit a row
   - Save
   - Refresh
   - Switch to another manager and back
   - Verify state preserved
3. **Cross-check `LITHIUM-TOC.md`** — every file referenced from a topic exists; every significant file is in the TOC.
4. **Update `LITHIUM-FAQ.md`** with a "LithiumTable Round 2 refactor" entry summarizing the canonical property changes, the schema validation wiring, the date/time integration, and any notable gotchas discovered during the phases.
5. **Move this plan** to `docs/Li/Plans-Complete/LITHIUM-TAB-PLAN.md` with the completion date, and note the completion date at the top of [LITHIUM-TAB.md](LITHIUM-TAB.md).

### Gate

- Full test suite passes.
- Coverage for `src/tables/` is at least as good as before this plan started.
- `LITHIUM-TOC.md` and every individual doc file are internally consistent.
- This plan is moved to `Plans-Complete/`.
- Clean lint and build.

### Docs to update on completion

- [LITHIUM-FAQ.md](LITHIUM-FAQ.md)
- [LITHIUM-TOC.md](LITHIUM-TOC.md)
- [LITHIUM-TAB.md](LITHIUM-TAB.md) — completion note
- This plan → archived

---

## Non-goals

Explicitly **out of scope**:

- Multi-row selection. `selectableRows: 1` stays hardcoded.
- Server-side pagination.
- Row-reorder drag beyond what the Column Manager already does for columns.
- New Managers (Job, Sync, Role, Security, etc.) — post-plan.
- Column types beyond the lookup family (Phase 11) and date/time family (Phases 12–13). HTML, image, color, star, progress, rownum, json, enum remain as-is.
- Tour integration for LithiumTable features (that's the Tour Manager's domain).

---

## Related documentation

- [LITHIUM-TOC.md](LITHIUM-TOC.md) — Documentation index
- [LITHIUM-INS.md](LITHIUM-INS.md) — Coding standards (file size, CSS-first, no fallbacks)
- [LITHIUM-DEV.md](LITHIUM-DEV.md) — Development setup and code hygiene cross-reference
- [LITHIUM-TAB.md](LITHIUM-TAB.md) — LithiumTable component reference
- [LITHIUM-TAB-TABLES.md](LITHIUM-TAB-TABLES.md) — Three-stage resolution and table definitions
- [LITHIUM-TAB-TYPES.md](LITHIUM-TAB-TYPES.md) — Column type reference
- [LITHIUM-COL.md](LITHIUM-COL.md) — Column Manager
- [LITHIUM-LUT.md](LITHIUM-LUT.md) — Lookups (the database mechanism)
- [LITHIUM-ICN.md](LITHIUM-ICN.md) — Icon pipeline (relevant for lookup-icon coltypes)
- [LITHIUM-LIB.md](LITHIUM-LIB.md) — Third-party libraries
- [LITHIUM-FAQ.md](LITHIUM-FAQ.md) — Gotchas and lessons learned
- [Acuranzo migration index](../../elements/002-helium/acuranzo/README.md) — Source of truth for Lookup 59 entries
