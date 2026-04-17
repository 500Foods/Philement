# LithiumTable — Implementation Plan (Round 2)

This is the second major implementation plan for LithiumTable. The first plan took us from "nothing" to "functional three-stage column resolution with editable tables, templates, and a Column Manager." That plan is complete. This plan takes us from "functional" to "solid, complete, and honest about what it is."

**Last updated:** April 2026

---

## How this plan is organized

The plan is a sequence of **phases**, each with:

- A **focused area** of concern
- A clear **goal**
- A list of **concrete work items**
- A **gate** that must pass before we move on

The gate is always the same shape:

1. **Tests pass.** Either existing tests still pass, OR tests that break are fixed — by fixing the test when the code is right, or fixing the code when the test is right. Tests are not gospel; code is not gospel. Both must agree with reality.
2. **Docs updated.** Every file in `docs/Li/` that relates to the phase's changes reflects the new reality. No "we'll update the docs later."
3. **Operational state improved.** The app at the end of a phase runs as well as or better than before. No temporary regressions carried forward between phases.

No backward-compat wrappers or legacy fallbacks. We have zero customers on this code. We change it cleanly and we keep the docs current.

---

## Ground truth references

These are the upstream sources of truth we need to consult repeatedly.

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

When a phase changes the shape of the coltype or tableDef JSON, the corresponding migration must be updated in the same commit as the code change. This keeps the source of truth coherent.

### Terminology disambiguation (critical — do not conflate)

Throughout this plan we use two words that sound similar and are **very different**:

- **Lookup** (capital L) / **Lookups** — our database mechanism for storing key-value-data (Lookup 1 = Lookup Status, Lookup 28 = Query Type, Lookup 59 = Tabulator Schemas, etc.). Identified by `LOOKUP_ID` and individual entries by `KEY_IDX`. Backed by QueryRef 34 (`Get Lookup List`). Documented in [LITHIUM-LUT.md](LITHIUM-LUT.md).
- **lookup coltype** (lowercase) — a column type in a tableDef that presents a dropdown/combobox editor and displays a resolved label rather than a raw integer. A lookup coltype *uses* a Lookup to get its values, but the coltype itself is just a presentation pattern.

A column whose stored value is an integer (e.g., `query_status_a27`) is an **integer field** in the database. When we render it with coltype `"lookup"` and `"lookupRef": "a27"`, we look up integer values in Lookup 27 to show the label and to populate the dropdown. The column did not become a "Lookup" — it remains an integer field that is *rendered* through a Lookup.

### Tabulator property preference

Whenever Tabulator has a native property for a concept we're modeling, we **prefer Tabulator's name**. Examples where we currently diverge and should fix:

| Our name | Tabulator name | Status |
|----------|----------------|--------|
| `display` | `title` | Keep `title`, retire `display` |
| (none) | `width`, `minWidth`, `maxWidth` | Already preferred ✓ |
| (none) | `hozAlign`, `vertAlign` | Already preferred ✓ |
| (none) | `formatter`, `formatterParams` | Already preferred ✓ |

Lithium-only properties (no Tabulator equivalent) that stay as-is:

- `coltype` — references Lookup 59 Key 0 entries
- `columnPri` — our display-order priority
- `groupable` (proposed) — flags a column as user-selectable for grouping
- `groupPri` (proposed) — priority when multiple groupings are active
- `groupOrd` (proposed) — sort direction within group
- `sortPri` (proposed) — priority in multi-column sort
- `filterPri` (proposed) — priority in multi-column filter (if needed)
- `primaryKey` — identifies PK columns for row ID composition
- `calculated` — flag for non-stored, derived columns
- `lookupRef`, `lookupStyle`, `lookupEdit` — how the lookup coltype family works

---

## Phases

### Phase 1 — Canonicalize the column property schema

**Goal:** Establish one, unambiguous column vocabulary that matches what the code actually does, retire aliases, and add schema validation to the pipeline so drift is caught immediately.

**Work items:**

1. **Decide `title` is the canonical header property.** Remove every code path that emits `display`. Update `_buildColumnsFromData`, `discoverColumns`, `resolveColumn`, template extractors, and the Column Manager's column-definition builder.
2. **Remove `display` from `COLUMN_VALID_PROPS`** and from the deduplicated listing in `lithium-table.js`. Also fix the duplicate `'display'` entry present in that set.
3. **Update Lookup 59 Key 0 (migration 1153)** so the coltype defaults use `title` consistently. Verify no tableDef (keys 2+) stores `display`; update any that do (migrations 1179–1187, 1156–1157).
4. **Update docs:**
   - `LITHIUM-TAB-TABLES.md` — all Stage 1/2/3 examples using `display` change to `title`
   - `LITHIUM-TAB-TYPES.md` — strike the `display` mention, confirm `title` as the one canonical
   - `LITHIUM-TAB-TYPES-DEFAULT.md` — already correct, verify
5. **Wire `validateTableDef()` into `loadTableDef()` and `_buildColumnsFromData()`** so every tableDef that enters the system goes through it once. Log warnings for unknown properties, errors for unknown coltypes. Today this function exists but is never called.
6. **Define the Lithium-only property allow-list explicitly** and separate it from the Tabulator-property pass-through. The current `lithiumMeta` / `tabulatorProps` split inside `resolveColumn` does this informally; make it a declared list at the module top.

**Gate:**

- All existing tests pass. Tests that assert `display` on resolved columns are updated to assert `title`.
- `lithium-table.test.js` gets new tests for `validateTableDef` wiring (warnings for unknown props, errors for unknown coltypes).
- Every tableDef migration in the list above has been read and confirmed to use `title`.
- `docs/Li/LITHIUM-TAB-TABLES.md`, `LITHIUM-TAB-TYPES.md`, and `LITHIUM-TAB-TYPES-DEFAULT.md` updated.
- The app loads Queries Manager, Lookups Manager, and Style Manager with all column headers correct (no "undefined" or missing headers).

---

### Phase 2 — Fix string and lookup editing

**Goal:** Editing a string cell or a lookup-coltype cell should work. Today numeric fields work; strings and lookups don't.

This phase is deliberately early because it's the user-visible pain point you called out.

**Work items:**

1. **Trace string editing** from `handleEdit` → `enterEditMode` → `queueCellEdit` → `cell.edit()`. The `editTriggerEvent: false` setting at `lithium-table-base.js:414` means only programmatic activation works. Confirm that for the `input` editor the double-rAF sequence in `queueCellEdit` is actually opening the editor, that the cell is finding itself as `editable`, and that `columnEditors.set(col.field, ...)` is actually happening for string columns.
2. **Trace lookup editing** similarly. The `resolveColumn` path at `lithium-table.js:706` only assigns a `list` editor when `_lookupCache.has(colDef.lookupRef)`. If `preloadLookups` didn't populate the cache before the tableDef was resolved (race condition during initial load), the column gets no editor. Confirm the timing: `loadConfiguration` awaits `preloadLookups` before `initTable` — but does the column resolver get called *after* that await, or is it using a stale resolution?
3. **Verify `applyEditModeGate`** correctly installs the `editable` predicate for string and lookup columns. The predicate relies on `columnEditors.has(field)` — which relies on `col.editor` being truthy at that point. If a string column's `editable: true` was dropped earlier in the merge stack, the editor never gets registered.
4. **Add focused tests** that load a tableDef with one string column and one lookup column, call `enterEditMode`, simulate the rAF ticks, and assert the cell enters its editor DOM state.
5. **Document the editing flow** in `LITHIUM-TAB.md` with a sequence diagram (in ASCII/Markdown) — the current docs describe behavior but not the internal dispatch, which makes bugs like this hard to pin down.

**Gate:**

- In the running app, editing a string cell in Queries Manager (`name`, `summary`) works via Enter, F2, double-click, and the Edit button.
- Editing a lookup cell in Lookups Manager (child table, `value_int` or similar lookup-bound columns) opens a dropdown with the right values.
- New tests for both flows pass.
- The gotcha that caused the bug is captured in `LITHIUM-FAQ.md`.
- No regressions in numeric or boolean editing.

---

### Phase 3 — Single canonical template extraction

**Goal:** One function takes "a live Tabulator column plus its original tableDef colDef" and produces a clean JSON-serializable tableDef column entry. Every caller uses it. Today three near-duplicates exist.

**Work items:**

1. **Audit the three extractors:**
   - `lithium-table-base.js` `_extractTemplateColumnFromColumn` / `_createTemplateColumnFromTableDef` (no whitelist — copies everything including `lithium*` junk)
   - `lithium-table-template.js` `extractTemplateColumnFromColumn` / `createTemplateColumnFromTableDef` (whitelisted)
   - `lithium-table-ui.js` `generateTemplateJSON` (100-line inline reimplementation)
2. **Define the canonical output shape.** A tableDef column entry has exactly the documented columns (from `COLUMN_VALID_PROPS` plus the Lithium-only list from Phase 1). Anything else is dropped. Function references (formatter, editor, validator when function-valued) are *not* serialized — the coltype reference is how we recover them.
3. **Build a single extractor** in `lithium-table-template.js` (or a new `tables/template/capture.js`) that takes:
   - The Tabulator column component (runtime state: width, visibility, current sort/filter values)
   - The original tableDef colDef (base JSON)
   - An optional options bag (include-runtime-only, include-hidden-columns, etc.)
4. **Delete the other two implementations.** Update `captureCurrentState`, `generateTemplateJSON`, and all callers to use the single function.
5. **Handle the `primaryKey`, `calculated`, `coltype`, `description`, `columnPri`, `lookupRef` round-tripping** explicitly — these are Lithium-only properties that Tabulator discards, so we pull them from `lithiumXxx` echo properties on the runtime column or from the original colDef.
6. **Add a round-trip test**: tableDef → resolved → Tabulator → extracted → compared. The extracted result must equal the input tableDef (modulo properties that are explicitly runtime-only).

**Gate:**

- All template-related tests pass.
- A new round-trip test passes.
- Saving a template and reloading it produces an identical table.
- Copying a template to clipboard produces valid JSON (no function refs, no `lithium*` noise, no lost properties).
- `LITHIUM-TAB.md` and `LITHIUM-COL.md` describe the single extractor and its contract.

---

### Phase 4 — Fix the three-stage merge semantics

**Goal:** Make the merge actually produce what the docs promise — a complete, usable tableDef at every stage, with deep-merged params and correct overlay order.

**Work items:**

1. **Switch overlays from shallow to deep-merge-with-care** for nested param objects (`formatterParams`, `editorParams`, `headerFilterParams`, `sorterParams`, `accessorParams`, `mutatorParams`, `bottomCalcFormatterParams`). Scalar properties and arrays continue to replace.
2. **Move `_applyDefaultTemplate` out of Stage 2.** Today it runs inside `loadConfiguration` before `initTable`, which effectively makes the localStorage "Default" template override Stage 2 (Lookup 59). Move it to Stage 3 — the genuine "user template" stage — by applying it *after* the Tabulator table is built, via the same `loadTemplate()` path the explicit template menu uses. This also means Stage 2 changes to Lookup 59 show up in the app immediately, without the user needing to clear their local template.
3. **Preserve coltype-derived defaults through template overlay.** When Stage 3 changes only `width`, we shouldn't lose Stage 2's `formatterParams`. The deep merge handles this; verify with a specific test.
4. **Auto-discovered `columnPri`.** Today every auto-discovered column gets `columnPri = fieldIndex + 1`. The docs say "null means displayed after numbered columns." Change auto-discovery to emit `columnPri = null` and sort by `?? Infinity` (already the case). Columns then fall in data order, but with sortable tie-breaking. If a tableDef defines `columnPri: 5` for one column, it goes first.
5. **Remove the double-discover race.** `createAutoDiscoverTableDef` sets `_autoDiscover: true`, and `initTable` wires a `dataLoaded` callback that re-runs `discoverColumns`. But `loadData` *already* calls `discoverColumns` after `setData`. Remove the callback.

**Gate:**

- All existing tests pass; update any that asserted shallow-merge behavior.
- New tests for deep-merge of `formatterParams` across stages.
- Test that `columnPri = null` is emitted for auto-discovered columns and that they sort after numbered columns.
- Manually verify: change a coltype's `formatterParams.precision` in Lookup 59 Key 0, refresh a table — the new precision shows up even if the user has a "Default" template saved.
- `LITHIUM-TAB-TABLES.md` merge-strategy section updated to describe deep-merge semantics for the documented param properties.

---

### Phase 5 — Grouping, sorting, and filtering trifecta

**Goal:** The three discoverable table operations (group, sort, filter) are first-class in the tableDef, in the Column Manager, and in the runtime — with consistent property names and a consistent multi-column priority mechanism.

**Work items:**

1. **Implement `groupable`, `groupPri`, `groupOrd`.** Today code uses a single `group` numeric property and ignores the documented trio. Replace:
   - `groupable: boolean` — column is eligible for grouping (user can pick it)
   - `groupPri: number | null` — priority when multiple columns group simultaneously (1 = outermost)
   - `groupOrd: 'asc' | 'desc'` — sort direction within each group
2. **Update `resolveTableOptions`** to build `groupBy` from columns where `groupable === true && groupPri != null`, sorted by `groupPri`. Pass `groupOrd` through Tabulator's `groupValues`/sorter plumbing where applicable.
3. **Implement `sortPri`** for multi-column initial sort. Build an `initialSort` array from columns where `sortPri != null`, sorted by `sortPri`, using the column's `groupOrd`-equivalent for direction. This replaces the current "tableDef.initialSort array" approach — keep `initialSort` as an explicit override but let per-column `sortPri` build it automatically.
4. **Implement `filterPri`** (optional — only if we decide we need it). If not needed now, document it as future.
5. **Column Manager grouping/sort/filter controls.** The Column Manager is *the* UI for toggling groupable, setting groupPri, setting sortPri, and setting headerFilter on/off per column. Review what's currently editable there and add what's missing.
6. **Table-wide options section in the Column Manager.** A dedicated area at the top (above the column list) for properties that are table-wide, not column-wide:
   - Layout mode (`fitColumns`, `fitData`, `fitDataFill`, `fitDataTable`)
   - Width preset
   - Default group-start-open
   - `readonly`
   - `selectableRows` (currently hardcoded to 1; consider if we want to expose)
   - `persistSort`, `persistFilter`
   - Any global filter/sort priorities that don't fit on a column
7. **Migration update.** If any new column-level properties become canonical (groupable trio, sortPri), make sure Lookup 59 Key 0 advertises them as defaults and Key 1 (JSON Schema) validates them.
8. **Docs update** across `LITHIUM-TAB-TYPES.md`, `LITHIUM-TAB-TABLES.md`, and `LITHIUM-COL.md`.

**Gate:**

- A tableDef with `groupable: true` + `groupPri: 1` on two columns produces a nested group display.
- A tableDef with `sortPri` on two columns produces a multi-column sort on initial load.
- The Column Manager UI reflects and edits all three (groupable, sortPri, headerFilter) per column.
- The Column Manager has a visible, functional table-wide settings area at the top.
- New tests cover the resolution from per-column `groupPri/sortPri` to Tabulator options.
- Existing group-arrow animation behavior (the hard-won prior-state-pinning) still works.
- No regressions in Queries Manager's or Lookups Manager's grouping behavior.

---

### Phase 6 — tableDef generation from live tables

**Goal:** The extract-from-live-table function (from Phase 3) becomes exhaustive, trustworthy, and the basis for: Templates, the Column Manager's "load from current table" operation, and seeding new database entries in Lookup 59.

This extends Phase 3 — Phase 3 gave us one function; Phase 6 makes that function capture *everything* we care about.

**Work items:**

1. **Catalog what we need to capture.** Start with the full Phase-1 property list (canonical column properties + Lithium-only properties + table-wide properties). Match each against what the current extractor captures, and fill gaps.
2. **Capture `initialSort`** from `table.getSorters()`.
3. **Capture filter values** from `table.getHeaderFilters()`.
4. **Capture layout mode** from the current runtime setting (`this.tableLayoutMode`), not the stored localStorage value.
5. **Capture width mode** similarly.
6. **Capture `groupBy`, expand/collapse state** (if we want it to be round-trippable).
7. **Capture all Lithium-only properties** from the column's stored `lithium*` echoes. Move these to a documented set of `lithiumColtype`, `lithiumPrimaryKey`, `lithiumCalculated`, `lithiumDescription`, `lithiumGroupable`, `lithiumGroupPri`, `lithiumSortPri`, etc. — one echo per Lithium-only property.
8. **Provide a `generateMigrationSeed()` helper** that outputs the JSON stanza suitable for pasting into an `acuranzo_11XX.lua` migration — i.e., drop the `_templateMeta` and runtime-only flags, include the `$schema` reference, use stable key ordering.
9. **Column Manager "export current table as tableDef" button.** Places the generated JSON on the clipboard, suitable for:
   - Saving as a template
   - Pasting into a migration file
10. **Test the round-trip.** tableDef → resolved → edited (change width, change sort) → generated → parsed → resolved. The second resolution must match what the user saw after editing.

**Gate:**

- The extractor passes the round-trip test.
- Manually: open Queries Manager, make visual changes (hide a column, resize another, sort by two columns), click "export". Paste the result into a new tableDef JSON; hand-edit it into a Lookup 59 entry; refresh — the table renders exactly as it looked.
- `LITHIUM-COL.md` documents the "Column Manager as tableDef editor" philosophy and the export flow.
- `LITHIUM-TAB-TABLES.md` has a migration-authoring section showing how to use the export to seed Lookup 59 entries.

---

### Phase 7 — Lookup coltype family expansion

**Goal:** The four documented lookup variants (`lookup`, `lookupIcon`, `lookupIconText`, `lookupIconList`) all render and edit correctly, with `lookupStyle` and `lookupEdit` controlling cell/dropdown display independently.

This is a significant surface. We already have `lookup` basic rendering + editing via `createLookupEditor` / `createLookupFormatter`. We don't have icon-aware variants.

**Work items:**

1. **Extend the lookup cache entries** to include the `collection` JSON from the Lookup (which holds `icon` markup). Today `loadLookup` maps to `{ id, label }`; extend to `{ id, label, icon, metadata }`.
2. **Implement the four cell formatters:**
   - `lookup` → text only (current behavior)
   - `lookupIcon` → icon only
   - `lookupIconText` → icon + text
   - `lookupIconList` → icon in the cell, icon+text in the dropdown
3. **Implement `lookupStyle`** — overrides the default cell-display mode for the coltype
4. **Implement `lookupEdit`** — overrides the default dropdown-display mode
5. **Replace the current `createLookupEditor`'s plain `list` editor** with one that renders the chosen display mode in the dropdown. This likely involves either a custom editor (replacing Tabulator's built-in `list`) or an `editorParams.itemFormatter` callback.
6. **Handle icon lifecycle.** Our Font Awesome pipeline (see [LITHIUM-ICN.md](LITHIUM-ICN.md)) has a three-stage `<fa>` → `<i>` → `<svg>` pass. When a lookup dropdown shows icons, those icons need to hit the pipeline. The group-header icon animation work done recently is the template here.
7. **Migration update.** Confirm Lookup 59 Key 0 has all four coltype stanzas with the right defaults. If the stanzas are incomplete, update migration 1153.
8. **Test fixtures** for an integer column with `coltype: lookup` / `lookupRef: "a1"` (Lookup Status, which has small values 0–2) across all four display styles.

**Gate:**

- In a test manager (pick one with a lookup field, e.g., Queries Manager `query_status_a27`), switching the column's coltype from `integer` to `lookup`, `lookupIcon`, `lookupIconText`, and `lookupIconList` each renders correctly.
- Editing the cell opens a dropdown that matches the configured `lookupEdit`.
- Tests pass including new icon-formatter tests.
- Docs `LITHIUM-TAB-TYPES-LOOKUP*.md` all reflect the implementation (including any nuance discovered during implementation).
- `LITHIUM-LUT.md` gets a "how lookups are consumed by LithiumTable" section.

---

### Phase 8 — Date/time coltypes with Luxon (+ Flatpickr)

**Goal:** The `date`, `datetime`, and `time` coltypes are actually usable. Today they use Tabulator's built-in `datetime` formatter and the HTML5 `date` editor — minimal and clunky. We want Luxon-powered formatting and parsing, and Flatpickr-powered pickers.

**Work items:**

1. **Add dependencies.** `luxon` (formatting/parsing/zones) and `flatpickr` (picker UI). Update `package.json`, `LITHIUM-LIB.md`.
2. **Design the property surface:**
   - `format` — Luxon display format string (default per coltype: date = `yyyy-MM-dd`, datetime = `yyyy-MM-dd HH:mm:ss`, time = `HH:mm:ss`)
   - `inputFormat` — for parsing incoming data (defaults to ISO)
   - `timezone` — optional IANA timezone; if set, display is converted; if unset, stored offset or system default
   - `pickerOptions` — Flatpickr options pass-through (enableTime, altInput, minDate, etc.)
3. **Custom formatter** per coltype that uses Luxon to parse+format.
4. **Custom editor** per coltype that opens a Flatpickr instance inside the cell. Integrate with Tabulator's editor lifecycle (mount, commit, cancel).
5. **Sorting.** Make sure Tabulator's `datetime` sorter receives a consistent ISO string via an accessor, regardless of display format.
6. **Filtering.** Header filter for date columns should be a date picker (or a simple ISO text filter if the picker is heavy).
7. **Respect user locale.** Pull date/time format preferences from Profile Manager's settings so a single tableDef property can say "use the user's preferred date format" rather than a hardcoded string.
8. **Migration update.** Ensure Lookup 59 Key 0 has the new properties documented as date/datetime/time coltype defaults.

**Gate:**

- Tables with date columns render formatted dates correctly.
- Clicking into a date cell in edit mode opens Flatpickr; picking a date commits the value.
- Sorting and filtering work on date columns.
- Tests pass (new Luxon-based formatter/parser tests, editor commit/cancel tests).
- Docs `LITHIUM-TAB-TYPES-DATE.md`, `-DATETIME.md`, `-TIME.md` reflect the implementation.
- `LITHIUM-LIB.md` includes Luxon and Flatpickr with rationale.

---

### Phase 9 — Navigator button isolation and Refresh correctness

**Goal:** The Refresh button (and every other nav button) acts on the instance it belongs to. No cross-instance side effects.

**Work items:**

1. **Trace the Refresh bug.** The user report is "refresh in one LithiumTable borks other instances." Likely suspects:
   - `_tableDefCache` is shared module-level; `reloadConfiguration` deletes its own entry but `clearLookup59Cache()` is also called elsewhere (via the `LOOKUPS_LOADED` event bus subscription) and clears the whole set
   - `refreshTabulatorSchemas()` re-fires `LOOKUPS_LOADED` which triggers the subscribed `clearLookup59Cache` in `lithium-table.js:1236` — every other table's tableDef is dropped from cache, and on the next render of any of them, re-fetch happens
   - The `_coltypesCache` singleton may get stale in some scenarios
2. **Audit every global event listener in `src/tables/`.** Each one must document whether it affects all tables or just one. `LOOKUPS_LOADED` → `clearLookup59Cache` is a legitimate pattern (stale data should be invalidated), but other tables should *re-fetch lazily*, not be torn down.
3. **Remove any instance-affecting side effects** from Refresh beyond the current instance. If a Refresh needs to invalidate shared state (e.g., to pick up Lookup 59 changes), the *other instances* should handle that invalidation via a soft event that says "next time you're asked for data, fetch fresh." They should not be torn down or reset.
4. **Audit the other nav buttons** (Filter, Menu, Width, Layout, Template) for cross-instance effects. Each one should be confirmed pure-local.
5. **Test:** build a two-table scenario (dual Lookups Manager setup). Make edits in one, click Refresh in the other, verify the first table is unaffected in visible state.

**Gate:**

- Two-table manager (Lookups) — selecting rows, editing, and Refresh in one table has zero visible effect on the other table.
- New tests for cross-instance isolation (mock two tables, call Refresh on one, assert the other's state).
- `LITHIUM-TAB.md` "Refresh Button Enhanced Behavior" section updated to clarify the instance-local scope.

---

### Phase 10 — Refactor the behemoth (`lithium-table-base.js`)

**Goal:** Bring `lithium-table-base.js` under the documented 750-line target by extracting the pieces that don't belong there. This is architecture cleanup, made safer by doing it after Phases 1–9 have stabilized the schema and behavior.

**Work items:**

1. **Extract `visual/group-icon-animator.js`** — `_groupPathKey`, `_collectAllGroups`, `updateGroupIcons`, `_syncGroupIconsNow`, plus the state Maps. ~160 lines.
2. **Extract `visual/column-tooltips.js`** — `initColumnHeaderTooltips`, `getColumnDefinition`, `buildColumnTooltipContent`, `escapeHtml`. ~100 lines. (`getColumnDefinition` may stay on base since it's about the tableDef; consider.)
3. **Extract `persistence/panel-width.js`** or fold into existing `persistence/persistence.js` — `getWidthForMode`, `applyPanelWidth`, `savePanelPixelWidth`, `loadCollapsedState`, `saveCollapsedState`. Remove the duplicate `applyPanelWidth` in `lithium-table-ui.js`. ~80 lines.
4. **Extract `tables/refresh-orchestrator.js`** — `reloadConfiguration` and the helper `_getCurrentlySelectedRowId`. This becomes the single-source-of-truth for the refresh pipeline, addressing Phase 9's isolation work by having one code path rather than two (`handleRefresh` + `reloadConfiguration`). ~150 lines.
5. **Extract `tables/template/capture.js`** — `captureCurrentState`, along with Phase 6's generator. The base class keeps a thin `captureCurrentState()` that just delegates.
6. **Remove the no-op stubs in `lithium-table-ui.js`** (`showNavPopup`, `positionNavPopup`, `buildStandardNavPopup`, `refreshTemplatePopup`, `createTemplateMenuAction`, `getPopupItems`). Replace with a clear "these features are owned by module X" comment or just delete if unused.
7. **Move the template-popup-positioning block** out of `toggleNavPopup` into `popups/template-popup.js` where it belongs.
8. **Delete the empty `src/tables/base/` directory** from the earlier refactor.

**Gate:**

- `lithium-table-base.js` is under 750 lines.
- `lithium-table-ui.js` is under 750 lines.
- All tests pass; test file imports updated for moved code.
- `LITHIUM-TAB.md` "File Structure" section updated to match reality.

---

### Phase 11 — Column Manager cleanup

**Goal:** The Column Manager and its Column Manager Manager (the nested one) behave as ordinary LithiumTables, with only the necessary differences (popup container, always-editable cells, save/cancel owned by the manager).

**Work items:**

1. **Audit `alwaysEditable` handling** across the mixin and base class. Every place we check `this.alwaysEditable` and skip something is a place where we're treating the Column Manager specially. Some of these are genuinely necessary (no footer save/cancel); some may be cruft.
2. **Confirm the Column Manager uses the same three-stage resolution** (Phase 4) from its own tableDef (Lookup 59 Key 2) — not a hand-built column array.
3. **Confirm the Column Manager Manager** (Lookup 59 Key 3) loads and edits correctly.
4. **Confirm the Column Manager uses Phase 3's single template extractor** to produce its "applied changes" — no bespoke extractor.
5. **Confirm the Column Manager's navigator works the same** (templates, width preset, layout) as any other LithiumTable. There's no reason the Column Manager shouldn't have its own templates.
6. **Update `LITHIUM-COL.md`** to clearly state "Column Manager is a LithiumTable" and enumerate the minimal set of differences (it's in a popup; its save/cancel is owned by the host).

**Gate:**

- Opening the Column Manager from any LithiumTable works.
- Opening a Column Manager from *inside* the Column Manager (the nested "manager manager") works to the same depth we currently support.
- Editing column properties in the Column Manager saves correctly to the parent table.
- No `alwaysEditable`-gated code path exists without a clear, documented reason.

---

### Phase 12 — Primary key discipline and selection integrity

**Goal:** Every table knows its primary key explicitly. The fallback chain becomes a loud warning, not a silent guess.

**Work items:**

1. **Audit every tableDef migration** (Lookup 59 keys 2–12) for `primaryKey: true` flags. Fix any that are missing.
2. **Downgrade the fallback chain in `_getCompositeRowId`** to a warning-on-use. When the chain picks a field because `primaryKeyField` is null, log `Status.WARN` with the picked field name.
3. **Remove the fallback entirely** once every tableDef is verified — tables without a primary key use some other identification strategy (composite from all fields? a generated index?) rather than guessing.
4. **Simplify `rowsMatch`, `autoSelectRow`, `saveSelectedRowId`, etc.** around the assumption that `primaryKeyField` is always set (or explicitly null for no-PK tables).

**Gate:**

- Every migration 1179–1187, 1156–1157 declares its primary key.
- No fallback `['id', 'query_id', ...]` chain remains in the code.
- Selection persistence works correctly across all implemented managers.
- `LITHIUM-TAB.md` "Primary Key Configuration" section states that PK declaration is mandatory.

---

### Phase 13 — Test coverage and documentation closure

**Goal:** Close the loop on test coverage for the surfaces modified in phases 1–12 and make sure the docs are internally consistent.

**Work items:**

1. **Run the full test suite** and capture coverage. Identify new gaps introduced by the refactors.
2. **Add integration-level tests** that exercise a manager end-to-end:
   - Load with a saved template
   - Edit a row
   - Save
   - Refresh
   - Switch to another manager and back
   - Verify state preserved
3. **Cross-link docs.** The `LITHIUM-TOC.md` already lists doc files; make sure every file referenced from a topic actually exists (this plan file itself, for instance) and every significant file is in the TOC.
4. **Reconcile `LITHIUM-MGR.md`** — it still references `src/core/lithium-table-*.js` paths that moved to `src/tables/`.
5. **Update `LITHIUM-FAQ.md`** with a "LithiumTable Round 2 refactor" section noting the canonical property changes, the schema validation now in place, and any other gotchas encountered during this plan.
6. **Archive this plan** — when complete, move it to `docs/Li/Plans-Complete/` and note the completion date at the top of `LITHIUM-TAB.md`.

**Gate:**

- Full test suite passes.
- Coverage for `src/tables/` is at least as good as it was before this plan started.
- `LITHIUM-TOC.md` and every individual doc file are internally consistent.
- This plan is moved to `Plans-Complete/` and the project is ready for its next plan.

---

## Phase dependency graph

Some phases depend on earlier ones; others could run in parallel if we had multiple hands. Here's the minimum dependency chain:

```
Phase 1 (property canonicalization) is the foundation.
         │
         ├── Phase 2 (editing) — can start in parallel; no hard dependency on Phase 1
         │
         ├── Phase 3 (single extractor) — depends on Phase 1
         │          │
         │          └── Phase 6 (exhaustive generation) — depends on Phases 3, 4, 5
         │
         ├── Phase 4 (merge semantics) — depends on Phase 1
         │          │
         │          └── Phase 5 (group/sort/filter) — depends on Phases 1, 4
         │
         ├── Phase 7 (lookup family) — depends on Phase 1; benefits from Phase 2
         │
         ├── Phase 8 (date/time + Luxon) — depends on Phase 1; independent of Phase 7
         │
         ├── Phase 9 (refresh isolation) — mostly independent; before Phase 10
         │
         ├── Phase 10 (refactor) — depends on Phases 1–9 stabilizing
         │
         ├── Phase 11 (Column Manager) — depends on Phases 3, 4, 5
         │
         ├── Phase 12 (primary key) — mostly independent; can run any time
         │
         └── Phase 13 (test + docs closure) — last
```

Recommended execution order if serial: **1 → 2 → 3 → 4 → 5 → 7 → 8 → 9 → 11 → 6 → 12 → 10 → 13.**

Phase 6 is positioned late because it extends Phase 3 and benefits from having Phases 4, 5, and 11 done — the more the Column Manager knows, the better the generator it backs.

Phase 10 is positioned near the end because the refactor is safer once behavior has stabilized.

---

## Non-goals

These are explicitly **out of scope** for this plan:

- New column types beyond the lookup family expansion (Phase 7) and the date/time expansion (Phase 8). HTML, image, color, star, progress, rownum etc. remain as-is.
- Multi-row selection. `selectableRows: 1` stays hardcoded.
- Server-side pagination. Tables remain fully client-side.
- Row reordering (drag) beyond what the Column Manager already does for column reordering.
- Inline column resize behavior changes.
- Tour integration for LithiumTable features (that's the Tour Manager's domain).
- New Managers (Job Manager, Sync Manager, etc.) — those come after this plan.

---

## Related documentation

- [LITHIUM-TOC.md](LITHIUM-TOC.md) — Documentation index
- [LITHIUM-TAB.md](LITHIUM-TAB.md) — LithiumTable component reference
- [LITHIUM-TAB-TABLES.md](LITHIUM-TAB-TABLES.md) — Three-stage resolution and table definitions
- [LITHIUM-TAB-TYPES.md](LITHIUM-TAB-TYPES.md) — Column type reference
- [LITHIUM-COL.md](LITHIUM-COL.md) — Column Manager
- [LITHIUM-LUT.md](LITHIUM-LUT.md) — Lookups (the database mechanism)
- [LITHIUM-ICN.md](LITHIUM-ICN.md) — Icon pipeline (relevant for lookup-icon coltypes)
- [LITHIUM-LIB.md](LITHIUM-LIB.md) — Third-party libraries
- [LITHIUM-FAQ.md](LITHIUM-FAQ.md) — Gotchas and lessons learned
- [Acuranzo migration index](../../elements/002-helium/acuranzo/README.md) — Source of truth for Lookup 59 entries
