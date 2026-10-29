# LithiumTable implementation checklist

How to add one LithiumTable. A new manager still follows [LITHIUM-MGR-NEW.md](LITHIUM-MGR-NEW.md). This page is only the grid.

The constructor options in [LITHIUM-TAB.md](LITHIUM-TAB.md) are the reference. The live instances are [LITHIUM-TAB-SURVEY.md](LITHIUM-TAB-SURVEY.md). Where those two disagree with this page, this page matches the source as of 2026-10-28. Import the class from `src/tables/lithium-table-main.js`.

Copy a grid that already matches the data shape:

| Shape | Copy | Why |
|-------|------|-----|
| Server list, server search | Queries (`queries.js`) or Scripting | `queryRef` plus `searchQueryRef`, then `loadData()` |
| Server list, search the rows already loaded | Course Manager or Course Builder | `localSearch` and `localSearchFields` |
| List needs a parameter such as `LOOKUPID` | Style elements (`initLookupTable`) | `onRefresh` runs the query and calls `loadStaticData` |
| Rows are built in the browser | Profile sections or Luxon tokens | `loadStaticData`, and `onRefresh` rebuilds those rows |
| Read-only | Version history, login history, Luxon tokens | `readonly: true` hides the Manage block |

Leave these alone when choosing a model: Lookups (both grids use `cssPrefix: 'lithium'`), Crimson citations (Lookup 059 key 13 is the Scripting definition), Style sections (`setColumns` after `init`), the login language picker (raw Tabulator), and the Column Manager (hand-built `tableDef`).

---

## 1. Identity

- [ ] Pick a `cssPrefix` that no other grid uses. It becomes every Navigator id (`${cssPrefix}-nav-refresh`, `${cssPrefix}-search-input`). `lithium` is already used by the Lookups list, Lookup values, Crimson citations, and the Column Manager.
- [ ] Pick a `storageKey` that no other grid uses. Templates, width, layout, and the selected row are stored under it.
- [ ] Pick a `tablePath` (`manager/table-name`). The running app loads the definition from Lookup 059, not from `config/tabulator/`.
- [ ] Reserve the next free Lookup 059 key and pass it as `lookupKeyIdx`. Keys already used for grids are listed at the bottom. Passing `null` makes the loader guess by path. Scripting does that. A direct key does not.
- [ ] Put `primaryKey: true` on the key column in the table definition. Pass the same field as `primaryKeyField: ['id']`. A compound key is `primaryKeyField: ['lookup_id', 'key_idx']` and `primaryKey: true` on each of those columns. Selection after refresh needs this even on a read-only grid.

## 2. Definition

- [ ] Write the Lookup 059 migration (Helium). Hand the packet over. Do not apply it.
- [ ] Keep a `config/tabulator/<path>.json` only as the design note. Vite does not serve it.
- [ ] Pass `lookupKeyIdx` from the constructor. A caller-supplied `tableDef` wins over Lookup 059. Only the Column Manager does that, and its columns are built in `cm-columns.js`.
- [ ] Leave column building to `init()`. Style sections and Crimson citations call `setColumns` or replace formatters afterwards, and those columns sit outside the resolver.

## 3. Data

Pick one load path and use it for the first paint and for Refresh.

- [ ] Server list. Pass `queryRef`. Call `loadData()` after `init()`. Queries uses 25. Scripting uses 89. Course Manager uses 4500.
- [ ] Parameterized list. Pass `onRefresh` that runs `authQuery` with the parameter and then `loadStaticData`. Style elements loads Lookup 41 through QueryRef 26. Lookup values loads QueryRef 34 with `LOOKUPID`. Refresh calls `onRefresh` when `lookupKeyIdx` is set, because that path goes through `reloadConfiguration`.
- [ ] In-memory list. Call `loadStaticData`. Point `onRefresh` at the same builder. Luxon tokens passes `app: null`, so `loadData()` returns immediately.

## 4. Search

The search box calls `loadData(term)` unless `localSearch` is true.

- [ ] Server search: pass `searchQueryRef`. A non-empty term uses that ref and sends `STRING.SEARCH` in upper case. Queries uses 32. Scripting uses 90. The Lookups list uses 31.
- [ ] Local search: pass `localSearch: true` and `localSearchFields`. Course Manager, Course Builder, Profile sections, and Luxon tokens do this. Rewrite the placeholder after `init()` if the default text is wrong. Course Manager and Course Builder do that, because the input does not exist until the Navigator is built.
- [ ] A parameterized list needs `localSearch`, or an `onRefresh` that the search box actually calls. Today the search box does not call `onRefresh`. Style elements and Lookup values load through `onRefresh` and do not set `localSearch`, so Search issues the list QueryRef with no `LOOKUPID`.

## 5. Editing

- [ ] `readonly: true` for a grid that only selects a row. The Manage block is omitted. Version history, Profile sections, login history, Luxon tokens, and Crimson citations do this.
- [ ] `readonly: false` shows Add, Duplicate, Edit, Flag, Annotate, and Delete. Flag and Annotate call `handleFlag` and `handleAnnotate`. Course Builder is the only grid that assigns them. On every other editable grid the click does nothing.
- [ ] Register the table with the manager's `ManagerEditHelper` when the footer Save and Cancel belong to this grid. Queries, Lookups, Style, Scripting, and Profile do this. `registerTable` wires `onEditModeChange` and `onDirtyChange`.
- [ ] Pass `onExecuteSave` when the save is not the default `INTEGER` primary key plus `JSON` row. Queries, Lookups, Style sections, Scripting, and both course grids do this.
- [ ] Pass `onDuplicate` when copy is not a local clone. Queries, Lookup values, Scripting, and Course Manager do this.
- [ ] Replace `table.handleDelete` when the key is compound or the delete QueryRef is not a single integer parameter. The default reads `row[primaryKeyField]` as one property. Scripting, Course Manager, and Course Builder replace it. Lookup values still uses the default with `['lookup_id', 'key_idx']`, so the server delete is skipped and the row is removed only in the browser.

## 6. Mount

Order used by Queries and by Course Manager:

- [ ] Construct `LithiumTable` with `container`, `navigatorContainer`, `app`, `tablePath`, `lookupKeyIdx`, `cssPrefix`, and `storageKey`.
- [ ] Register the edit helper and any external editors before `init()` when this manager has them.
- [ ] `await table.init()`, then `await table.loadData()` or `loadStaticData`.
- [ ] Create the splitter after the table exists. Pass the table in `tables`. Redraw on `onResizeEnd`.
- [ ] Pass `panel` and `panelStateManager` when the grid lives in a collapsible pane. Pass `tableWidthMode` when the default width is not compact (`wide` on Queries, Scripting, and both course grids; `narrow` on Profile sections).
- [ ] After an HTML or CSS template edit, run `npm run templates:copy`.

## 7. Check

- [ ] One row selects. Refresh reselects it. That is the primary-key check.
- [ ] Search returns the rows you expect, including a second search and a clear.
- [ ] Refresh reloads the same parameter the screen used the first time.
- [ ] Add, Duplicate, Edit, and Delete either work or are absent. Flag and Annotate are assigned or the screen has no Manage block.
- [ ] A second grid on the same screen uses a different `cssPrefix`. Lookups mounts two Navigators that share `lithium-nav-*`.
- [ ] `npm test` and `npm run lint` from `elements/003-lithium`.

---

## Lookup 059 keys already used

Re-check `elements/002-helium/acuranzo/migrations/` before taking a key. As of `acuranzo_1412.lua`, grid definitions occupy:

| Key | Path |
|-----|------|
| 0 | `column-types` |
| 1 | tableDef JSON schema |
| 2 | `column-manager` |
| 3 | `column-manager-manager` |
| 4 | `profile-manager/user-options` |
| 5 | `queries/query-manager` |
| 6 | `lookups-manager-list` |
| 7 | `lookups-manager-values` |
| 8 | `style-manager/lookup-41` |
| 9 | `style-manager-sections` (the live Style sections grid does not load this key) |
| 10 | `version-manager/version-history` |
| 11 | style base variables |
| 12 | style semantic variables |
| 13 | `scripts/script-manager` (Crimson citations also passes 13) |
| 14 | `courses/course-manager` |
| 15 | `courses/course-builder` |
| 16 | `profile-manager/luxon-tokens` |
| 17 | `profile-manager/login-history` |

The next free key is 18.
