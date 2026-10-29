# Lithium table survey

Checklist of every Tabulator grid in Lithium. Surveyed 2026-10-28 from `elements/003-lithium/src`.

A change to grid behavior belongs in `src/tables/` and then has to be walked against this list. Several screens replace a Navigator handler, load data outside `loadData()`, or paint columns after `init()`. Those are the rows a shared edit misses.

Related: [LITHIUM-TAB.md](LITHIUM-TAB.md), [LITHIUM-TAB-CHECKLIST.md](LITHIUM-TAB-CHECKLIST.md), [LITHIUM-COL.md](LITHIUM-COL.md).

---

## Where `new Tabulator` lives

| Site | File | What it is |
|------|------|------------|
| Shared engine | `elements/003-lithium/src/tables/lithium-table-base.js` | The only constructor the LithiumTable instances below use |
| Login language picker | `elements/003-lithium/src/managers/login/login-language-panel.js` | Raw Tabulator. No LithiumTable, no Navigator, no Lookup 059. Columns are `_getColumns()` |

`public/src/managers/` holds fetched HTML shells. It does not construct another grid.

---

## LithiumTable instances

`cssPrefix` is also the Navigator id prefix (`${cssPrefix}-nav-refresh`, `${cssPrefix}-search-input`). Four grids use `lithium`, so those ids are not unique. See the Navigator notes below.

| # | Screen | Constructed in | cssPrefix | Lookup 059 | Data | Mode and hooks |
|---|--------|----------------|-----------|------------|------|----------------|
| 1 | Queries | `src/managers/queries/queries.js` | `queries` | key 5, `queries/query-manager` | List 25, search 32, through `loadData` | Editable. Custom save and duplicate. Edit helper |
| 2 | Lookups list | `src/managers/lookups/lookups.js` `initParentTable` | `lithium` | key 6, `lookups-manager-list` | List 30, search 31, update 43, insert 42 | Editable. Custom save. Same prefix as row 3 |
| 3 | Lookup values | `src/managers/lookups/lookups.js` `initChildTable` | `lithium` | key 7, `lookups-manager-values` | List 34, which needs `LOOKUPID`. Update 43, insert 42. `onRefresh` calls `loadChildData` | Editable. Compound key `lookup_id` + `key_idx`. Custom save and duplicate. Default delete |
| 4 | Style elements | `src/managers/style-manager/style-manager.js` `initLookupTable` | `style-lookup` | key 8, `style-manager/lookup-41` | `onRefresh` → QueryRef 26 with `LOOKUPID` 41, then `loadStaticData` | Editable. Edit helper. No `searchQueryRef` |
| 5 | Style sections | `src/managers/style-manager/style-manager.js` `initSectionsTable` | `style-sections` | path `style-manager/sections` (not in the path map) | `localStorage`, then `setColumns` of two raw columns, then `loadStaticData` | Editable. Save writes `localStorage`. No QueryRef |
| 6 | Version history | `src/managers/version-history/version-history.js` | `version-history` | key 10, `version-manager/version-history` | `onRefresh` → `loadVersionList` | Read-only. `wireNavigatorButtons()` is never called |
| 7 | Scripting | `src/managers/scripting/scripting.js` | `scripting` | path `scripts/script-manager`. `lookupKeyIdx` is passed as `null`. Key 13 is that definition (`acuranzo_1394.lua`) | List 89, search 90 | Editable. Compound key `group_name` + `script_name`. Custom save, duplicate, and delete |
| 8 | Course Manager | `src/managers/course-manager/course-manager.js` | `course` | key 14, `courses/course-manager` | List 4500. Local search on code, slug, title, Canvas id | Editable. Custom save, duplicate, and delete. Placeholder rewritten after build |
| 9 | Course Builder | `src/managers/course-builder/course-builder.js` | `builder` | key 15, `courses/course-builder` | List 4515. Local search | Editable. `course-builder-actions.js` replaces Flag, Annotate, and Delete. Placeholder rewritten after build |
| 10 | Profile sections | `src/managers/profile-manager/profile-manager-table.js` | `profile-options` | key 4, `profile-manager/user-options` | `onRefresh` → QueryRef 26 with `LOOKUPID` 60, then `loadStaticData` (`autoSelect: false`) | Read-only. Local search. Collapse-all after load |
| 11 | Login history | `src/managers/profile-manager/pages/login-history/page-login-history.js` | `profile-login-history` | key 17, `profile-manager/login-history` | `onRefresh` → QueryRefs 159 and 160 inside the page, then static rows | Read-only. Nested in Profile, not its own manager |
| 12 | Luxon tokens | `src/shared/luxon-token-table.js`, opened from `page-date-formats.js` | `profile-luxon-tokens` | key 16, `profile-manager/luxon-tokens` | In-memory token rows. `app` is `null`. `onRefresh` rebuilds them | Read-only. Local search. Nested in Profile → Date Formats |
| 13 | Crimson citations | `src/managers/crimson/crimson-citations.js` | `lithium` | key 13, `crimson/citations` | Static citation rows (`autoSelect: false`). Then `url` and `score` formatters are replaced, and `rowDblClick` is added on the Tabulator | Read-only. Key 13 is the Scripting tableDef (`acuranzo_1394.lua`). No citations definition exists under Helium |
| 14 | Column Manager | `src/tables/column-manager/cm-table.js` | `lithium` | Path `column-manager/column-manager`, but `tableDef` is the object from `cm-columns.js`, so Lookup 059 key 2 is not loaded | Parent columns copied in `cm-data.js`, then `replaceData` | Editable through the normal edit mode (`alwaysEditable` is false). Add, Duplicate, Flag, Annotate, and Delete are disabled in `configureNavigatorButtons` |
| 15 | Column Manager Manager | same `initColumnTable`, when the popup is already inside a Column Manager | `lithium` | Path `column-manager/column-manager-manager`. Same hand-built `tableDef` | Same as row 14, for the Column Manager's own columns | Depth 2. Storage key `lithium_column_manager_manager` |

Rows 14 and 15 are not on screen until the ellipsis in the frozen selector column is clicked (`buildSelectorColumn` in `src/tables/columns/column-management.js`).

---

## What a shared change has to re-check

**Search box.** `wireSearchButtons` calls `loadData(term)` unless `localSearch` is set. Rows 8, 9, 10, and 12 set `localSearch`. Rows 3 and 4 need a parameter (`LOOKUPID`) and load through `onRefresh`, so the search box does not use that path. Row 5 has no QueryRef, so search logs and returns.

**Refresh.** With `lookupKeyIdx` set, Refresh runs `reloadConfiguration`, which then calls `onRefresh` when that hook exists. Without `lookupKeyIdx`, Refresh calls `onRefresh` or else `loadData`. Row 5 has neither a key nor `onRefresh`. Row 14 has no key and no QueryRef, so Refresh does not reload the column list from the parent.

**Manage block.** Hidden when `readonly` or `alwaysEditable`. Flag and Annotate are wired to `handleFlag` / `handleAnnotate`. Only row 9 assigns those. The other editable grids show the buttons and the click does nothing. Row 14 disables them.

**Delete.** The default `handleDelete` reads `primaryKeyField` as one column. Row 3 keeps the default with a two-column key, so the QueryRef is skipped and the row is removed only in the browser. Rows 7, 8, and 9 replace `handleDelete`.

**Columns applied after `init`.** Row 5 calls `setColumns` with two raw Tabulator definitions. Row 13 replaces two formatters. A resolver or template change does not pass through those assignments.

**Duplicate Navigator ids.** Rows 2, 3, 13, and 14 share `cssPrefix: 'lithium'`. Rows 2 and 3 are mounted together.

---

## Navigator surface (one implementation)

Built by `getNavigatorHTML` / `buildNavigator` in `src/tables/navigator/navigator-builder.js`.

| Block | Buttons | Handler |
|-------|---------|---------|
| Control | Refresh, Filter, Grouping, Width, Layout, Templates | `handleRefresh`, `handleFilter`, `toggleNavPopup` |
| Move | First, Page up, Previous, Next, Page down, Last | `navigation.js` |
| Manage | Add, Duplicate, Edit, Flag, Annotate, Delete | `lithium-table-ops.js`, except Flag and Annotate |
| Search | Search, input, clear | `wireSearchButtons` |

There is no Menu button. `configureNavigatorButtons` still looks up `#${prefix}-nav-menu`.
