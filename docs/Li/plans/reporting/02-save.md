<!-- markdownlint-disable MD007 MD024 MD013 MD036 -->

# Reporting — Save

**Parent:** [`/docs/Li/plans/REPORTING_PLAN.md`](/docs/Li/plans/REPORTING_PLAN.md)
**Phases:** 11–18
**Stop when:** The author can save a revision and open it again.

The locks, the phase index, Phase 0, the backlog, and the Working Log
live in the parent. This file holds these phase bodies. Work one phase
per conversation. Write Status, Accomplished, Lessons learned, and
Handoff on the phase in this file, set the same Status in the parent's
phase table, and append the Working Log in the parent.

**Entry:** Phase 10 in [`/docs/Li/plans/reporting/01-designer.md`](/docs/Li/plans/reporting/01-designer.md) is complete.
**Next file:** [`/docs/Li/plans/reporting/03-preview.md`](/docs/Li/plans/reporting/03-preview.md) after Phase 18.

## Phase 11 — Lookup 053 repair

### Goal

Hand Andrew one migration for lookup 053. Do not apply it.

### Entry gate

Phase 0 Status complete. This phase may be prepared while Phases 1–10
are still open. It is applied before Phase 14 starts. Re-check disk
the day the file is numbered.

### Work items

- [x] 11.1 Re-read
       [`/docs/He/GUIDE.md`](/docs/He/GUIDE.md) and the highest
       Acuranzo file, QueryRef, and lookup key. Take the next file
       number. Do not edit 1086 in place.
- [x] 11.2 One forward migration for lookup 053 only. Fix the `Iion`
       keys 7 and 16. Fix the unclosed tag on key 52. Insert Page
       Number (key 61), Page Count (key 62), and Parameter (key 63).
       Update the v1 attribute lists so static text, field text, field
       number, field datetime, field image, calculated text, calculated
       number, calculated datetime, aggregate text, aggregate number,
       aggregate datetime, and parameter include position, font,
       text or field, and
       datasource. Leave lookup 054 keys 9, 10, 11, and 12 off every
       v1 attribute list. The `mask` string is the format. Do not add
       chart or barcode attribute rows. Reverse
       restores the previous JSON and deletes only the inserted keys.
- [x] 11.3 Hand the packet over. Record the file number in this
       phase's Status after Andrew reports the apply.

### Done means

Andrew reports the packet applied. Keys 7 and 16 store `icon`. Keys
61, 62, and 63 exist. The v1 attribute lists include the properties
Phase 14 edits.

### Exit gate

Andrew's apply report. Test 31 and Test 98 if he ran them. The agent
does not mark this complete from a clean local lint alone.

### Status

**complete — 2026-11-02.** Andrew reported
[`acuranzo_1436.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1436.lua)
applied and confirmed lookup 053 in the Lookups manager. Keys 7 and 16
store `icon`. Key 52 closes its tag. Keys 61 Page Number, 62 Page Count,
and 63 Parameter exist. The v1 attribute lists match. Test 31 and
Test 98 pass. The next Acuranzo file is 1437. The next caller-facing
QueryRef is 173.

### Accomplished

- One lookup-only packet with no caller-facing QueryRef. 1086 was not
  edited in place.
- Static text, Page Number, and Page Count carry
  `[0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 24, 29]`. Text is native to the
  item (Andrew, 2026-11-02). Field, calculated, and aggregate text,
  number, datetime, and field image add 7 (Field) and 8 (Datasource).
  Parameter adds 8.
- Lookup 054 keys 9–12 are on no list. Chart and barcode rows are
  untouched. Reverse deletes 61–63 and restores the 1086 JSON.

### Lessons learned

- 1086 put lookup 054 ids 24 (Grid Background Color) and 29 (Link) on
  every item list. 1436 kept them to stay minimal.
- The icon typos only showed when reading the seed file. The designer
  does not read lookup 053 yet, so the check is the Lookups manager.

### Handoff

- Phase 12 (lookup 054 fonts) re-checks the next free Acuranzo file
  (1437), QueryRef (173), and lookup key on disk.
- Phase 14 decides whether 054 ids 24 and 29 stay on the item lists.
  The 053 packet is applied, so Phase 14's entry condition on it is met.
- Nothing is waiting on Andrew for Phase 11.

---

## Phase 12 — Lookup 054 fonts

### Goal

Add Vanadium Sans and Vanadium Mono to the report font list. Two
families, one migration. Do not apply it. (Amended 2026-11-02: Andrew
added Vanadium Mono; the goal was one family.)

### Entry gate

Phase 0 Status complete. May be prepared with Phase 11. Applied before
Phase 14 starts. Re-check the next file number the day it is written.

### Work items

- [x] 12.1 Re-read the highest Acuranzo file. Do not edit
       [`acuranzo_1087.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1087.lua)
       in place.
- [x] 12.2 One forward migration for lookup 054 only. The font list
       gains Vanadium Sans, then Vanadium Mono, ahead of the existing
       faces. The hint default becomes Vanadium Sans. Existing faces stay, so an old row still
       resolves. Key 4's list becomes `Left`, `Center`, and `Right`.
       Key 17's list becomes `Normal`, `Bold`, `Italic`, and
       `Bold + Italic`. Reverse restores the previous JSON for keys 4, 15,
       and 17.
- [x] 12.3 Hand the packet over. Record the file number after Andrew
       reports the apply.

### Done means

Andrew reports Vanadium Sans and Vanadium Mono on lookup 054, the faces that were
already there still resolve, and keys 4 and 17 list the v1 captions.

### Exit gate

Andrew's apply report.

### Status

**complete — 2026-11-02.** Andrew reported
[`acuranzo_1437.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1437.lua)
applied and confirmed lookup 054 in the Lookups manager. Key 15 lists
Vanadium Sans, Vanadium Mono, and the five existing faces, with hint
`Default: Vanadium Sans`. Key 4 lists Left, Center, Right. Key 17 lists
the four v1 styles. The next Acuranzo file is 1438. The next
caller-facing QueryRef is 173.

### Accomplished

- One lookup-only packet with no caller-facing QueryRef. 1087 was not
  edited in place. Existing faces stay, so old rows still resolve.
- Vanadium Mono was added at Andrew's request (2026-11-02). The goal
  was amended from one family to two.

### Lessons learned

- 1087's key 4 had no `List`, so the reverse restores a bare editor
  row. 1087's key 17 carried the typo `Bold + Underlilne`; the reverse
  restores it as stored.
- PDF still uses Helvetica in v1. Both Vanadium faces show in HTML only
  until embedding leaves Later.

### Handoff

- Phase 13 (lookup 057 page sizes) re-checks the next free Acuranzo file
  (1438), QueryRef (173), and the lookup 057 max key on disk.
- Lookups 053 and 054 are applied, so Phase 14 waits only on Phase 13.
- Nothing is waiting on Andrew for Phase 12.

---

## Phase 13 — Lookup 057 page sizes

### Goal

Fix the Letter landscape label and add the missing page sizes. One
family, one migration. Do not apply it.

### Entry gate

Phase 0 Status complete. May be prepared with Phases 11 and 12.
Applied before Phase 14 starts. Re-check the next file number the
day it is written.

### Work items

- [ ] 13.1 Re-read the highest Acuranzo file. Do not edit
       [`acuranzo_1090.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1090.lua)
       in place.
- [ ] 13.2 One forward migration for lookup 057 only. Relabel key 1
       as US Letter landscape. Insert Legal portrait, A4, and Tabloid.
       Reverse restores the label and deletes only the inserted keys.
- [ ] 13.3 Hand the packet over. Record the file number after Andrew
       reports the apply.

### Done means

Andrew reports a correctly labeled Letter landscape row and the three
new page sizes.

### Exit gate

Andrew's apply report.

### Status

**not started**

---

## Phase 14 — Property panel

### Goal

Edit the selected object from lookup 054, limited to the attribute
list on its lookup 053 row. The panel is the Editor-page column
named in Manager chrome in the parent. It stays hidden on the
other pages.

### Entry gate

Phases 11, 12, and 13 are complete, including the applies.

### Work items

- [ ] 14.1 Load lookup ids 053, 054, and 057 with QueryRef 026
       (Get Lookup, `LOOKUPID`). Do not read them from `lookups.js`.
       That module's QueryRefs 053 and 054 are Get Themes (lookup 41)
       and Get Icons (lookup 40). Leave those categories alone.
       Cache the three report lookups on the manager.
- [ ] 14.2 The panel lists the attributes advertised for the selected
       `typeKey`. Editors follow lookup 054's `Editor` value for the
       v1 set: text, number, list, boolean. An unknown editor toasts
       and skips that row. Keys 9–12 are not on the lists. The mask
       string is edited in work item 15.5 and is not a lookup row.
- [ ] 14.3 Edits commit through the model. Undo reverts the panel and
       the canvas. Page size picks a lookup 057 row and copies its
       width, height, and margins into the definition.
- [ ] 14.4 Browser: change the static text, the font size, and the
       page size. Export. Import into a fresh session and see the
       same panel values.
- [ ] 14.5 An omitted key shows the Defaults table value and a note
       that the file omits it. Editing the control writes the key.
       A number item with `mask` `""` shows the plain-decimal rule.
       An item with no `font` shows the report font and says the
       file omits the override.

### Done means

The panel shows the effective value, including a default the file
left out. The properties come from the lookups and the Defaults
table.

### Exit gate

Lint, unit tests for "attribute list filters the panel", and the
browser check against a migrated database (the new lookup rows have
to be visible).

### Status

**not started**

---

## Phase 15 — Field items and a sample datasource

### Goal

Place a field and show which column it is bound to.

### Entry gate

Phase 14 Status complete.

### Work items

- [ ] 15.1 A session-only sample JSON datasource. The operator pastes
       or imports an array of objects. Column names are the union of
       keys. This JSON is not part of the definition export.
- [ ] 15.2 Add field text, number, and datetime items. The property
       panel's Field editor lists those column names. Datasource
       names come from the definition's `datasources` array.
- [ ] 15.3 The canvas label is the field name. An empty binding shows
        the type caption and a toast when the operator exports, and
        export is refused for that item. Compose will warn later. The
        designer does not invent a value. (Per `LITHIUM-INS.md` rule 1,
        no silent fallback — the operator must pick or fill a binding.)
- [ ] 15.4 Browser: bind `customer`, export, confirm `field` and
       `typeKey` 20, reload, and see the binding.
- [ ] 15.5 Number and datetime items edit a `mask` string. The
       canvas label stays the field name. An empty mask means the
       default in Value format. Export round-trips `mask`.

### Done means

A field item round-trips, and the sample data never appears in the
exported definition. Empty bindings block export with a toast.

### Exit gate

Lint, unit tests, and the browser check.

### Status

**not started**

---

## Phase 16 — One group level

### Goal

Add a group header and footer tied to one field of the detail
datasource.

### Entry gate

Phase 15 Status complete.

### Work items

- [ ] 16.1 The palette adds one group header and one group footer.
       Each is `0.25 in` tall. The header stores `field`. The footer
       shares that grouping. A second group level is refused with a
       toast.
- [ ] 16.2 The canvas shows the three bands in order: group header,
       detail, group footer.
- [ ] 16.3 Browser: set the group field, export, undo.

### Done means

The grouped fixture's shape can be built from the UI and the file
matches the Phase 1 grouped definition's keys.

### Exit gate

Lint, unit tests, and the browser check.

### Status

**not started**

---

## Phase 17 — Save and load packet

### Goal

QueryRefs that list the latest revision, read one revision, and
insert a new revision. Hand them over. Do not apply them.

### Entry gate

Phase 0 column lock still matches [`/docs/Li/plans/REPORTING_PLAN.md`](/docs/Li/plans/REPORTING_PLAN.md). Re-check numbers the
day the files are written. Phases 4–16 may still be open. Phase 18
waits for the apply.

### Work items

- [x] 17.1 Read a recent QueryRef seed (the 1260 / 1108 pattern in
       [`/docs/He/GUIDE.md`](/docs/He/GUIDE.md)) and
       [`acuranzo_1015.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1015.lua)
       before writing SQL. The id allocation copies
       [`courses_4103.lua`](/elements/002-helium/courses/migrations/courses_4103.lua):
       `COALESCE(MAX(id), 0) + 1`.
- [x] 17.2 Three report migrations, one QueryRef each, `TYPE_SQL` so
        the SPA can call them. List latest revision per `report_id`.
        Get one `(report_id, rev_id)`. Insert the next `rev_id` for a
        name and a definition. Insert allocates `report_id` the way a
        sibling insert migration allocates its id. Copy that pattern.
        Do not invent one. The document-image read is the fourth
        migration, in 17.3. Andrew also asked for the list's search
        query. That is a fifth file, between the list and the get.
        The list and the search omit `report` and `thumbnail`.
        The search matches `UPPER(name)` and uses `:SEARCH` once.
- [x] 17.3 A fourth QueryRef reads one document image by `doc_id`:
        `file_data` and `file_name` from `documents`
        ([`acuranzo_1010.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1010.lua)).
        The report writer's `reference` image kind resolves through it
        in Phase 51, so the picker and compose share one read. If the
        documents table shape has changed by the day this is written,
        re-read it before numbering. The read is the latest `rev_id`
        for that `doc_id`.
- [x] 17.4 Reverse deletes the QueryRef row only.
- [x] 17.5 Hand the packets over. Record the QueryRef integers in
        this phase's Status after Andrew reports the apply. Andrew
        reported the five files applied on 2026-10-31.

### Done means

Andrew reports the five QueryRefs applied. A manual call of the
insert returns `report_id` and `rev_id`, a manual call of the get
returns that row, and a manual call of the document read returns
`file_data` for a `doc_id`. The search returns the latest revisions
whose name contains the term.

### Exit gate

Andrew's apply report. The agent does not mark this complete on Test
31 alone.

### Status

**complete — 2026-10-31.** Andrew reported the five QueryRefs
applied. `npm test` was not this phase's gate. The expand check
from the writing sitting stays the file check. Phase 18 is in
progress in the same conversation because he asked to wire them.

| File | QueryRef | Name |
| --- | --- | --- |
| [`acuranzo_1422.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1422.lua) | 165 | List Reports |
| [`acuranzo_1423.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1423.lua) | 166 | Search Reports |
| [`acuranzo_1424.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1424.lua) | 167 | Get Report |
| [`acuranzo_1425.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1425.lua) | 168 | Insert Report Revision |
| [`acuranzo_1426.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1426.lua) | 169 | Get Document Image |

`REPORT_ID` 0 on 168 allocates the next `report_id` and stores
`rev_id` 1. Any other `REPORT_ID` appends the next `rev_id` for that
report. 168 returns `report_id` and `rev_id`. Delete Report
is `acuranzo_1427.lua`, QueryRef 170, written after this
phase and not applied. The next Acuranzo file is 1428. The
next caller-facing QueryRef is 171. Re-check both before
taking them.

### Accomplished

- Five `TYPE_SQL` files, one QueryRef each. List and search return
  the latest revision and omit `report` and `thumbnail`. Search
  uses `:SEARCH` once and matches `UPPER(name)`. Get reads one
  `(report_id, rev_id)`. Insert allocates like
  `courses_4103.lua`. The document read returns the latest
  `documents` revision for `doc_id`.
- Reverse deletes the QueryRef row only.

### Lessons learned

- No report list, search, get, or insert QueryRef existed before
  this packet. `acuranzo_1015.lua` only creates the table.
  Scripts 089 and 090 are the script list and search.
- A second use of a named parameter is not portable. Search wraps
  `:SEARCH` once inside the engine branch.
- The inner SQL of a long query is stored compressed. An empty
  scan of the outer file is not a failed expand.

### Handoff

Phase 17 is complete. Andrew reported the apply on 2026-10-31.
QueryRefs 165 through 169 are the applied numbers. Phase 18 is
in progress and points the manager at 165, 166, 167, and 168.
QueryRef 169 stays unused until Phase 51. Phase 16 has not
started. Delete Report is `acuranzo_1427.lua`, QueryRef 170,
written and not applied. The next Acuranzo file is 1428.
The next caller-facing QueryRef is 171.

---

## Phase 18 — Revisions in the manager

### Goal

Save the open definition as a new revision and reopen it.
The Revisions page lists the saved revisions of that report,
copies one onto a new report, and reverts by deleting the
newer rows.

### Entry gate

Phase 16 and Phase 17 Status complete.

### Work items

- [x] 18.1 List view of latest revisions via the list QueryRef.
       Opening a row loads that revision into the document model.
       Search uses the search QueryRef. LithiumTable sends
       `STRING.SEARCH` uppercased.
- [x] 18.2 Save appends a revision. It does not update the open
       `rev_id` in place. The list row then shows the returned
       `report_id` and `rev_id`. The open definition stays on
       the canvas.
- [x] 18.3 A failed call toasts the conduit error and keeps the
       document dirty. The table save path does that when the
       insert throws.
- [x] 18.4 Browser: save, reload the manager, open the row, and see
       the same items. Save again and see `rev_id` increase.
- [x] 18.5 Save writes the open definition. A non-empty trail is
       stored in `reports.report`, and opening that revision
       restores Undo and Redo. Clear before the save stores the
       current report with `history` omitted.
- [x] 18.6 The Revisions page lists saved revisions for the
       open report. Copy loads that body and inserts a new
       report. Revert deletes newer revisions and reloads
       the chosen one. Both wait for edit mode. QueryRefs
       171 and 172, and the insert rewrite that stores the
       signed-in account, are written and not applied.

### Done means

The definition round-trips through `reports.report` as a new
revision.

### Exit gate

Lint, unit tests with a mocked conduit, and the browser check
against the applied QueryRefs.

### Status

**complete — 2026-10-31.** Andrew said the changes are
working. Phase 16 has not started. He brought this phase
forward and closed it on that check. Work items 18.1–18.6
are checked. He also asked for the Revisions page inside
this phase. `npm test` exited 0
(73 files, 1335 passed, 12 skipped). The skipped cases
are the auth integration file. `npm run lint`,
`npm run lint:css`, and `npx eslint` on
`src/managers/reports` and the report unit tests exited 0.
`templates:copy` copied 137 templates. `luacheck` on
1433–1435 exited 0. `reports-save.test.js` has 8 tests.
`reports-chrome.test.js` has 11.
`reports-revisions.test.js` has 3. `reports.js` is 982
lines. `reports-save.js` is 301. `reports-revisions.js`
is 398. `reports-pages.js` is 43. `reports-table.js` is
332. `reports-json.js` is 228. Andrew reported Delete
Report applied. The list calls QueryRef 170. Category
is a reports row column. `acuranzo_1428.lua` through `acuranzo_1435.lua` are
applied on that check. He did not recite the file
numbers. Duplicate still has no query.
The next Acuranzo file is 1436. The next caller-facing
QueryRef is 173. The Acuranzo index is 360 files and
1918 statements.

### Accomplished

- The list calls QueryRef 165. Search calls 166. Opening a row
  with `report_id` and `rev_id` calls 167 and imports
  `reports.report`. A row that already carries the definition
  uses that text. A new row does not call 167.
- Save calls QueryRef 168. `REPORT_ID` 0 allocates a report.
  An open report id appends the next revision. The returned
  `report_id` and `rev_id` land on the list row. The canvas
  keeps the definition that was saved. A non-empty `history`
  trail is inside that JSON. Clear removes `history` before
  the save. Thumbnail, design, scripts, and parameters from
  the open revision are sent again. A blank large column is
  `''`.
- A failed insert throws. The table toasts `serverError` and
  leaves the row dirty. A save while the get is still in
  flight is refused. Duplicate still has no query. List
  Delete calls QueryRef 170 after confirmation and removes
  every revision. A draft stays local. QueryRef 169
  is not called. The designer stays read-only until the
  row is edited. An edit marks the row dirty. A JSON edit
  marks the row dirty before it is committed, and leaving
  JSON for the Editor commits that buffer as one undo step
  and redraws the page.
- The Revisions page lists QueryRef 171 for a saved report.
  The label is the revision, the save time, and the account
  name. A draft does not call the server. Copy confirms,
  loads that revision through 167, and inserts a new report
  through 168 with `REPORT_ID` 0. The new list row is not
  selected. Revert confirms, loads the body first, then
  QueryRef 172 deletes newer revisions and the kept body
  is imported. The edit snapshot is taken again. Revert
  stays off on the newest revision. Both buttons wait for
  edit mode. Save sends `INTEGER.USERID` from
  `app.auth.user`. A missing id is 0.

### Lessons learned

- The list and the search omit `report`. Opening a row has to
  call the get. Storing the definition on the list row would
  skip that read.
- `onExecuteSave` owns the save. The table clears dirty only
  after it resolves, so a thrown conduit error keeps the
  document dirty and toasts the server error.
- Phase 16 is still open. The definition saved here is the
  one the designer can write today. A later group band is
  another revision of the same `report_id`.
- QueryRef 170 deletes every revision of the report. Revert
  needs its own delete, QueryRef 172, limited to newer
  `rev_id` values. The get runs before that delete.
- `:USERID` can appear once. The insert stores it as
  `created_id` and leaves `updated_id` at 0. A new revision
  is an insert, not an update.

### Handoff

Phase 18 is complete. Andrew said the changes are working
on `https://lithium.500courses.com` on 2026-10-31. That
check covers save, reload, a rising `rev_id`, and the
Revisions page. It is also the apply report for
`acuranzo_1428.lua` through `acuranzo_1435.lua`. He did
not recite the file numbers. Duplicate still has no query.
QueryRef 169 stays unused until Phase 51. The next
Acuranzo file is 1436. The next caller-facing QueryRef is
173. Phase 16 has not started. Phase 11 has not started.
It is the lookup 053 packet at the top of this file. Do
not start Phase 11, Phase 16, or Phase 19 until he names
one. Phase 10-UI is complete in
[`/docs/Li/plans/reporting/01-designer.md`](/docs/Li/plans/reporting/01-designer.md).

