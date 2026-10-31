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

- [ ] 11.1 Re-read
       [`/docs/He/GUIDE.md`](/docs/He/GUIDE.md) and the highest
       Acuranzo file, QueryRef, and lookup key. Take the next file
       number. Do not edit 1086 in place.
- [ ] 11.2 One forward migration for lookup 053 only. Fix the `Iion`
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
- [ ] 11.3 Hand the packet over. Record the file number in this
       phase's Status after Andrew reports the apply.

### Done means

Andrew reports the packet applied. Keys 7 and 16 store `icon`. Keys
61, 62, and 63 exist. The v1 attribute lists include the properties
Phase 14 edits.

### Exit gate

Andrew's apply report. Test 31 and Test 98 if he ran them. The agent
does not mark this complete from a clean local lint alone.

### Status

**not started**

---

## Phase 12 — Lookup 054 fonts

### Goal

Add Vanadium Sans to the report font list. One family, one migration.
Do not apply it.

### Entry gate

Phase 0 Status complete. May be prepared with Phase 11. Applied before
Phase 14 starts. Re-check the next file number the day it is written.

### Work items

- [ ] 12.1 Re-read the highest Acuranzo file. Do not edit
       [`acuranzo_1087.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1087.lua)
       in place.
- [ ] 12.2 One forward migration for lookup 054 only. The font list
       gains Vanadium Sans. Existing faces stay, so an old row still
       resolves. Key 4's list becomes `Left`, `Center`, and `Right`.
       Key 17's list becomes `Normal`, `Bold`, `Italic`, and
       `Bold + Italic`. Reverse removes the added face and restores
       the previous JSON for keys 4 and 17.
- [ ] 12.3 Hand the packet over. Record the file number after Andrew
       reports the apply.

### Done means

Andrew reports Vanadium Sans on lookup 054, the faces that were
already there still resolve, and keys 4 and 17 list the v1 captions.

### Exit gate

Andrew's apply report.

### Status

**not started**

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
list on its lookup 053 row.

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

- [ ] 17.1 Read a recent QueryRef seed (the 1260 / 1108 pattern in
       [`/docs/He/GUIDE.md`](/docs/He/GUIDE.md)) and
       [`acuranzo_1015.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1015.lua)
       before writing SQL.
- [ ] 17.2 Three report migrations, one QueryRef each, `TYPE_SQL` so
        the SPA can call them. List latest revision per `report_id`.
        Get one `(report_id, rev_id)`. Insert the next `rev_id` for a
        name and a definition. Insert allocates `report_id` the way a
        sibling insert migration allocates its id. Copy that pattern.
        Do not invent one. The document-image read is the fourth
        migration, in 17.3.
- [ ] 17.3 A fourth QueryRef reads one document image by `doc_id`:
        `file_data` and `file_name` from `documents`
        ([`acuranzo_1010.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1010.lua)).
        The report writer's `reference` image kind resolves through it
        in Phase 51, so the picker and compose share one read. If the
        documents table shape has changed by the day this is written,
        re-read it before numbering.
- [ ] 17.4 Reverse deletes the QueryRef row only.
- [ ] 17.5 Hand the packets over. Record the QueryRef integers in
        this phase's Status after Andrew reports the apply.

### Done means

Andrew reports the four QueryRefs applied. A manual call of the
insert returns the row he inserted, and a manual call of the
document read returns `file_data` for a `doc_id`.

### Exit gate

Andrew's apply report. The agent does not mark this complete on Test
31 alone.

### Status

**not started**

---

## Phase 18 — Revisions in the manager

### Goal

Save the open definition as a new revision and reopen it.

### Entry gate

Phase 16 and Phase 17 Status complete.

### Work items

- [ ] 18.1 List view of latest revisions via the list QueryRef.
       Opening a row loads that revision into the document model.
- [ ] 18.2 Save appends a revision. It does not update the open
       `rev_id` in place. The canvas then shows the new revision.
- [ ] 18.3 A failed call toasts the conduit error and keeps the
       document dirty.
- [ ] 18.4 Browser: save, reload the manager, open the row, and see
       the same items. Save again and see `rev_id` increase.

### Done means

The definition round-trips through `reports.report` as a new
revision.

### Exit gate

Lint, unit tests with a mocked conduit, and the browser check
against the applied QueryRefs.

### Status

**not started**

