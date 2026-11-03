<!-- markdownlint-disable MD007 MD024 MD013 MD036 -->

# Reporting — Save

**Parent:** [`/docs/Li/plans/REPORTING_PLAN.md`](/docs/Li/plans/REPORTING_PLAN.md)
**Phases:** 11–18, with labeled Phases 13b and 14b
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

- Follow-ups (2026-11-03): element group buttons are radio-style, lit
  when chosen, saved at `reports.category` on the profile (default
  Static). Band resize stops at the bottom of the band's lowest item.
  Reports footer audit info waits on draft migration `acuranzo_1443.lua`
  (QueryRef #165 returns created/updated id and time); not wired yet.

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

Fix the Letter landscape label and add the missing page sizes
(Legal and Tabloid both orientations, A4 and A3 portrait). One
family, one migration. Do not apply it.

### Entry gate

Phase 0 Status complete. May be prepared with Phases 11 and 12.
Applied before Phase 14 starts. Re-check the next file number the
day it is written.

### Work items

- [x] 13.1 Re-read the highest Acuranzo file. Do not edit
       [`acuranzo_1090.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1090.lua)
       in place.
- [x] 13.2 One forward migration for lookup 057 only,
       `acuranzo_1438.lua`. Relabel key 1 `US Letter Landscape`.
       Insert keys 2–7: `US Legal Portrait` (8.5 × 14 in),
       `US Legal Landscape`, `Tabloid Portrait` (11 × 17 in),
       `Tabloid Landscape`, `A4 Portrait` (210 × 297 mm), and
       `A3 Portrait` (297 × 420 mm). Margins 0.25 in or 6 mm. Page
       Size strings `US Legal`, `Tabloid`, `A4`, `A3`; `Tabloid` and
       `A3` are not yet in lookup 054 key 30 and may need a later add.
       Reverse restores the label and deletes only keys 2–7.
- [x] 13.3 Hand the packet over. Record the file number after Andrew
       reports the apply.

### Done means

Andrew reports a correctly labeled Letter landscape row and the six
new page sizes.

### Exit gate

Andrew's apply report.

### Status

**complete — 2026-11-02.** Andrew reported
[`acuranzo_1438.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1438.lua)
applied and the lookup 057 checks good. Key 1 reads US Letter
Landscape. Keys 2–7 hold US Legal and Tabloid in portrait and
landscape and A4 and A3 portrait. The next Acuranzo file is 1439. The
next caller-facing QueryRef is 173.

### Accomplished

- One lookup-only packet with no caller-facing QueryRef. 1090 was not
  edited in place. Keys 0 and 1 keep their geometry.
- The goal was widened at Andrew's request (2026-11-02) from portrait
  Legal, A4, and Tabloid to Legal and Tabloid in both orientations plus
  A4 and A3 portrait.
- A4 and A3 landscape were skipped by Andrew's choice. A later
  lookup-only file can add them as keys 8 and up.

### Lessons learned

- 1090's key 1 label `US Letter Legal` hid a Letter landscape row; the
  JSON, not the label, is the source of truth when repairing a lookup.
- Lookup 054 key 30 lists page-size names separately from 057 and
  lacks `Tabloid` and `A3`. Nothing in Lithium reads it for page size;
  057 and `page.sizeKey` drive the control.

### Handoff

- Phase 13b (section lookups) followed and is complete (1439–1441). Phase 14
  then decides whether 054 key 30 is dropped from the report panel
  or fed from 057.
- Lookups 053, 054, and 057 are applied.
- Nothing is waiting on Andrew for Phase 13.

---

## Phase 13b — Section lookups

### Goal

Hand Andrew the section lookups the property panel reads. Lookup
074 names the lookup 053 palette groups. Lookup 075 names the
lookup 054 attribute sections. Lookup 054 rows gain a numeric
`section` key. Do not apply them.

### Entry gate

Phase 13 Status complete. Andrew named this labeled phase on
2026-11-02. Re-check the next file number and the highest lookup id
the day it is written (073 was the highest on 2026-11-02).

### Work items

- [x] 13b.1 One migration per lookup family. Lookup 074 (053 palette
       groups): one row per 053 `value_int` group in use (0 hidden,
       1 Static, 6 Bands, 7 System, and any other on disk), each with
       `value_txt` label, `sort_seq`, and `collection` `{"icon": ...}`.
       Register it in lookup 0 like
       [`acuranzo_1090.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1090.lua).
- [x] 13b.2 Lookup 075 (054 attribute sections): one row per 054
       `Group` value in use, the same shape.
- [x] 13b.3 Lookup 054: add `"section": <075 key>` to each row's
       collection. Keep `Group`. Reverse restores the prior JSON.
- [x] 13b.4 Ask Andrew for every label, icon, and order before
       writing. Hand the packets over. Record the file numbers after
       Andrew reports the apply.

### Done means

Andrew sees lookups 074 and 075 in the Lookups manager, and every
054 row carries a `section` that names a 075 row.

### Exit gate

Andrew's apply report.

### Status

**complete — 2026-11-02.** Andrew reported
[`acuranzo_1439.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1439.lua),
[`acuranzo_1440.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1440.lua), and
[`acuranzo_1441.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1441.lua) applied. The next Acuranzo
file is 1442. The next caller-facing QueryRef is 173.

### Accomplished

- Lookup 074 Report Palette Groups (1439): lookup 0 registry row plus
  keys 0–7, `key_idx` = the 053 `value_int`, icon in `collection`.
  Order: Static 1 (`fa-cube`), Field 2 (`fa-database`), Calculated 3
  (`fa-function`), Aggregate 4 (`fa-sigma`), System 7 (`fa-gear`),
  Bands 6 (`fa-layer-group`), CrossTab 5 (`fa-table-cells`, hidden in
  v1), Structural 0 (`fa-sitemap`, never on the palette).
- Lookup 075 Report Attribute Sections (1440): registry row plus keys
  1–9, the nine 054 `Group` strings: Report, Page Layout, Page
  Margins, Object, Position, Fonts, Formatting, Grid, Ruler.
- Lookup 054 (1441): keys 0–40 gained `"section": <075 key>`; `Group`
  kept. Mapping: 1 Report 18–19; 2 Page Layout 30–34; 3 Page Margins
  35–39; 4 Object 6, 7, 8, 13, 29; 5 Position 0–5, 14; 6 Fonts 15–17;
  7 Formatting 9–12; 8 Grid 20–24, 40; 9 Ruler 25–28.
- Andrew's answers: labels, icons, and order approved; Structural row
  included for 074 key 0; 054 `sort_seq` left alone. Lookup 053 and
  its lowercase `group` (the subtype) are unchanged. The Outline
  navigator stays hard-coded in the panel, not a 075 row.

### Lessons learned

- 053 needs no column for its palette group: `value_int` already is
  one, so 074 keys on it. 053's `group` string is a subtype, not the
  palette group.
- 1441 was built from 1087 with 1437's keys 4, 15, and 17 laid over,
  so the reverse restores the live JSON, not the original 1087.
- 054 editor oddities remain for Phase 14: keys 21 and 24 use `Text`
  for colours while key 25 uses `Color`; keys 18–19 have `sort_seq` 0.

### Handoff

- Phase 14 (property panel, `LithiumProperties`) is next, in a fresh
  chat. Its entry gate is met: 053, 054, 057, 074, and 075 are applied.
- Group the panel by 054 `section` (075 key) and order sections by
  075 `sort_seq`; fall back to `Group` only if `section` is missing.
  Keep 054 `sort_seq` order within a section.
- Open with Andrew in 14.4/14.8: 054 ids 24 and 29, key 30 (drop or
  feed from 057), Color editor for keys 21 and 24, and where name and
  title edit.
- Nothing is waiting on Andrew for Phase 13b.

---

## Phase 14 — Property panel

### Goal

Edit the selected object, or the report and page when nothing is
selected, in a shared `LithiumProperties` panel built on Tabulator.
Rows come from lookup 054, limited to the attribute list on the
selected type's lookup 053 row, grouped by lookup 075 sections. The
panel sits left of the Editor canvas and right of the report table,
with its own splitter (Manager chrome in the parent). It stays hidden on the other pages. The Property panel lock
in the parent governs layout, ids, labels, and editors.

### Entry gate

Phases 11, 12, 13, and 13b are complete, including the applies.

### Work items

- [x] 14.1 Load lookup ids 053, 054, 057, 074, and 075 with QueryRef
       026 (Get Lookup, `LOOKUPID`). Do not read them from
       `lookups.js`. That module's QueryRefs 053 and 054 are Get
       Themes (lookup 41) and Get Icons (lookup 40). Leave those
       categories alone. Cache the report lookups on the manager.
- [x] 14.2 `LithiumProperties` under `src/properties/`: a two-column
       Tabulator with no header, rows grouped by section with icon
       and collapse, a search box (label, section, id), a draggable
       name/value splitter, condensed rows, and both sides centred
       when a row wraps. It takes rows and editors as data and knows
       nothing about reports.
- [x] 14.3 One label resolver for rows: `lang` exact locale, then
       language, then the plain label; list captions by position
       under `lang`. The profile language wins over the browser.
       Rows ship in English.
- [x] 14.4 Editors follow lookup 054's `Editor`: Text, Number, List,
       Boolean, and Color (Coloris, ported from Pavillion with its
       zoom and swatches). `None` is read-only. An unknown editor
       toasts and skips that row. Keys 9–12 are not on the lists.
       The mask string is edited in work item 15.5 and is not a
       lookup row. Decide 054 ids 24 and 29 and key 30 with Andrew.
- [x] 14.5 `reports-panel.js` wires the component to the selection.
       Empty selection shows the report and page properties. Edits
       commit through the model, one Undo step each. Undo reverts the
       panel and the canvas. Page size picks a lookup 057 row and
       copies its width, height, and margins into the definition.
       `reports.js` stays under 1000 lines.
- [x] 14.6 An omitted key shows the Defaults table value and a note
       that the file omits it. Editing the control writes the key.
       A number item with `mask` `""` shows the plain-decimal rule.
       An item with no `font` shows the report font and says the
       file omits the override.
- [x] 14.8 Outline section and placement. Put the panel left of the
       canvas with its own splitter. Its first section lists the
       report, the bands, and each band's items. Clicking an entry
       selects it, and canvas selection highlights it. Remove
       `.reports-summary` from `paintReportStage` and `reports.css`.
       Ask Andrew whether name and title edit here, on the Report
       page, or both.
- [x] 14.7 Browser (after 14.8): select from the Outline, change the static text, the font size, a colour,
       and the page size. Search, collapse, and drag the splitter.
       Export. Import into a fresh session and see the same values.

### Done means

The panel sits left of the canvas, the summary under the canvas is
gone, and the Outline selects bands and items. The panel shows the
effective value in sections with icons, including
a default the file left out, and edits undo. The properties come from
the lookups and the Defaults table.

### Exit gate

Lint, unit tests for "attribute list filters the panel", the label
resolver, and section grouping, and the browser check against a
migrated database (the new lookup rows have to be visible).

### Status

**complete — 2026-11-03.** Andrew closed the phase after the round-5
browser checks and the band-drag ruler fix ("Lets wrap this phase").
No migration was written for Phase 14; the next Acuranzo file is
still 1442 and the next caller-facing QueryRef is still 173.

Andrew continued into Phase 14 in the same Sitting after 13b closed
(2026-11-02).

Decisions locked by Andrew (2026-11-02):

1. 054 key 24 (Grid Background Color) shows on the report/page only
   (empty selection). Lithium drops 24 from item attribute lists
   (`REPORT_ONLY_KEYS`). Deferred lookup-only cleanup: remove 24 from
   the 053 item `attributes` lists. Not written.
2. Key 29 Link is editable text (Lithium override of `Editor: None`).
3. Key 30 Page Size fills from lookup 057 and ignores its own `List`.
   Picking a row copies width, height, margins, and orientation.
4. Keys 21 and 24 use the Color editor in Lithium by key. Deferred
   cleanup: set their 054 `Editor` to `Color`. 1442 not written.
5. Name and title edit in both the panel (empty selection) and the
   Report page form. One document; each edit is one Undo step; each
   surface repaints from the document.

QueryRef: lookups load with QueryRef 26 (Get Lookup, the plan's
"026"), the path the rest of Lithium uses. The lookup loader's
QueryRef 34 is the Lookups manager list query; not used here.

Done:

- `src/managers/reports/report-lookups.js` loads 053, 054, 057, 074,
  075 (a failed one comes back empty); cached on the manager.
- `src/managers/reports/report-properties.js` (pure): attribute keys
  per selection (053 item `attributes` minus 24; 053 key 9 names for
  the report, with a fallback list), rows with effective value and
  `omitted`, editor overrides, and `applyProperty` (report fields,
  item box through `moveItem`/`resizeItem`, align, rotation, link,
  item font override, static text body row).
- `src/properties/lithium-properties.js`: Tabulator grid grouped by
  section (sections.js), Text/Number/List/PageSize editors, Color via
  `@melloware/coloris` (new dependency), italics for defaults.
- `src/managers/reports/reports-panel.js` mixin: panel and Outline
  left of the canvas with its own `LithiumSplitter`; Outline click
  selects, canvas selection highlights; edits commit through
  `_commitDocument`; read-only unless the record is in edit mode.
- `.reports-summary` removed from `paintReportStage` and `reports.css`.
  `reports.js` is 928 lines.
- Tests: `tests/unit/report-properties.test.js` (attribute list filters
  the panel, editor overrides, defaults, edits, undo steps, grouping
  and Outline); shell, chrome, items, revisions tests moved from the
  summary to the Outline. Full unit suite green; lint clean on the
  touched files; `vite build` passes.

Gaps closed (2026-11-02):

- 14.2: search box (label, section caption, stable id), no header,
  section icons from 075 `icon` in the group header, header click
  collapses, name/value splitter on the name cell's edge (Tabulator
  cell resize, since there is no header), condensed rows, and both
  sides centred when a row wraps.
- 14.4: Boolean is a checkbox. A 054 row with a `List` (Visible, key
  20) stays a dropdown because the lookup says List. Number is a
  decimal input; a length key keeps or adds the report unit and a bad
  value toasts and leaves the file. An unknown editor toasts once
  and skips the row. Field and Datasource show read-only until Phase
  15. Coloris is the stock `@melloware/coloris`; Pavillion's source is
  not in this repo, so its zoom and swatches are not ported. Zoom is
  the Lithium-wide zoom item; swatches can follow with 14b.
- 14.6: an omitted value shows the default in italics with the note
  "Not in the file; default shown." An item with no font shows the
  report font and says the file has no item override. A number item
  (types 21, 29, 35, 61, 62) with mask `""` or none shows a read-only
  Mask row with the plain-decimal rule.

Chrome pass (2026-11-02, Andrew's first look):

- Regression fixed. The polish pass put the panel rules outside
  `@layer managers`, and set Tabulator cells to `display: flex`,
  which is block-level, so a row's name and value cells stacked. Each
  default also got a second visible note line, which read as extra
  fields. Cells are now `inline-flex` inside the layer, and the note
  is a tooltip on the italic default. Text and select editors commit
  once (no double commit on Enter plus blur). Boolean is a Yes/No
  select again, not a checkbox.
- The panel is one framed object. A top bar (rounded top) holds
  Search, Outline, Expand all, and Collapse all. Thin side lines in
  the toolbar colour run down to a bottom bar (rounded bottom) with
  Favourites and Hidden as stubs that toast "Phase 14b". Left margin
  matches the toolbars; the right splitter has the same gutters as
  the report list splitter.
- Search sits above the Outline when on. The Outline toggles, scrolls,
  and resizes vertically from a handle above the grid. Hidden regions
  give their space to the grid.
- Editors have no chrome: they edit in the cell.
- Section headers have a caret (right closed, down open), the 075
  icon, and a right-aligned count. Decision for this pass: the whole
  header toggles, not only the caret. Open and closed sections
  survive a repaint.

Not done yet:

- 14.7: Andrew's browser check. It is the only open item; Phase 14
  stays In progress until it passes.
- Second pass (2026-11-02): expand/collapse fix, band scope in both
  directions, shared toggle class. Details are in the REPORTING_PLAN journal.
- Third pass (2026-11-02): header spacing, refit on resize, divider
  bar, search clear X. Details are in the REPORTING_PLAN journal.
- Fourth pass (2026-11-02): Outline density, LithiumTable colours,
  value width = inner width minus name width. Details are in the REPORTING_PLAN journal.
- Split fix (2026-11-02): the name width sticks after a drag (redraw first, then widths).
- Chrome pass done (Andrew, 2026-11-02). PP layout prefs persisted (width, name split, search and outline toggles, outline height, closed sections).
- 14.7 closed by Andrew on 2026-11-03 after the tweak rounds below.

### Accomplished

- `LithiumProperties` (`src/properties/`): a shared, report-agnostic
  Tabulator property grid grouped by lookup 075 sections, with search,
  Outline, name/value splitter, Text/Number/List/Boolean/Color/PageSize
  editors, italic defaults, and Undo through the model.
- Chrome polish: one framed panel with top and bottom bars, tight
  Expand/Collapse/Prev/Next buttons, carets and counts on section
  headers, full-width headers when every section is closed, and the
  same corner radius (`--border-radius-sm`) as the toolbars on the
  panel bars, the editor stage, and the report sheet.
- Layout prefs in localStorage: panel width (`PanelStateManager`),
  name split, search and outline toggles, outline height, and closed
  sections (`lithium_reports_props_prefs`).
- Keyboard and navigation: Up/Down step the Outline; Tab and
  Shift+Tab move between value editors and skip read-only rows;
  Prev/Next in the bottom bar step the selection.
- End of Report: a zero-height marker with its own Outline entry and
  a PP section (read-only height).
- Rulers on four sides: the Report's Ruler section saves
  `rulers: { top, left, right, bottom }` in the definition (a side is
  on unless `false`). Right and bottom ticks face the page. Clicking
  a ruler selects the report. A session-only Ruler toolbar toggle
  hides them all.
- Crosshair toolbar toggle (session only) shows or hides the cursor
  guide lines.
- Band-expand fix: dragging a band edge now resizes and redraws the
  right ruler as well as the left, so new ticks and numbers appear
  during the drag.
- Unit suite at close: 1383 passed, 12 skipped. ESLint and stylelint
  add no new problems.

### Lessons learned

- New rules must sit inside `@layer managers`; unlayered
  `lithium-toolbar.css` outranks the layer, so the tight button
  padding needs `!important`.
- Tabulator 6.7.1 `navigateNext` stops on a read-only row; Tab is
  handled in a capture-phase keydown that commits, then edits the
  next editable row.
- End of Report is not a `.reports-band`, so it stays out of band
  loops and selects through its own flag.
- Any code that resizes one side ruler must resize every side ruler;
  the right ruler was missed when it was added.
- `--border-radius-sm` and `-md` are both 6px today; use `-sm` to
  match the toolbars so a future change to either stays consistent.
- `npm run icon:registry` errors at the end on the dist copy but still
  updates `config/icons-dev.txt`.

### Handoff

- Phase 14b (property panel preferences) is next, in a fresh chat. Its
  entry gate (Phase 14 complete) is now met. Do not start it until
  Andrew names it.
- 14b starts from: the Favourites and Hidden bottom-bar stubs (they
  toast "Phase 14b"); layout prefs already in localStorage, which
  14b.3 moves to the user profile per panel; and the Flatpickr date
  editor (14b.4).
- Open questions carried forward: with the left ruler off, the unit
  corner shows on the bottom ruler only when the top is also off;
  the deferred lookup-only cleanups (054 keys 21 and 24 `Editor` to
  `Color`; drop key 24 from 053 item lists) are still unwritten.
- Nothing is waiting on Andrew for Phase 14.

## Phase 14b — Property panel preferences

### Goal

The panel remembers how each user likes it, and dates get an editor.

### Entry gate

Phase 14 Status complete. Andrew named this labeled phase on
2026-11-02.

### Work items

- [x] 14b.1 Favourites and hidden properties, stored on the user
       profile by panel id and row id. Favourites show first; hidden
       rows have a show-hidden toggle.
- [x] 14b.2 Reorder within a section by drag. The order is a profile
       preference. Lookup `sort_seq` stays the default.
- [x] 14b.3 Remember open sections and the splitter position per
       panel on the profile.
- [x] 14b.4 Date editor on the existing Flatpickr
       (`shared/flatpickr-picker.js`). No new package.
- [x] 14b.5 Write `docs/Li/LITHIUM-PROPS.md` for the component: rows,
       sections, editors, the resolver, preferences, and how another
       manager adopts it.
- [x] 14b.6 Browser: favourite, hide, reorder, collapse, and resize,
       then sign out and in and see the same panel.

### Done means

Preferences survive a new session, and a date property edits with
Flatpickr.

### Exit gate

Lint, unit tests for preference storage by id, and Andrew's browser
check.

### Status

**complete — 2026-11-03.** Andrew said "Pretty good. Let's close
this phase." after his browser check. Work items 14b.1–14b.6 are
checked. `npm test`: 81 files, 1397 passed, 12 skipped. ESLint is at
the baseline (163 in src, 8 in tests). Stylelint is clean. The CSS and
HTML are copied to `public/`.

### Accomplished

- Panel prefs live on the user profile (`window.lithiumSettings`,
  synced to the account) at `24.props.designer`, keyed by panel id and
  row id. The old `lithium_reports_props_prefs` blob and
  `lithium_reports_props_width` are copied there once and removed.
  With no profile service, prefs fall back to localStorage.
- Right-click a row for checkable **Favourite** and **Hidden** items.
  Rows always stay in their own section.
- Bottom-bar filters: Favourites shows only favourites. Hidden off
  drops hidden rows; Hidden on includes them in the regular list.
- Hovering Favourites shows a right-justified star in the name cell of
  favourite rows. Hovering Hidden, or turning it on, shows an
  eye-slash on hidden rows. No other styling marks either.
- Drag a row by its name to reorder within its section. A drop in
  another section snaps back. Right-click Reset order returns to lookup
  `sort_seq`.
- Open sections, name width, outline height, the Search and Outline
  toggles, and the panel width are remembered on the profile.
- A `Date` editor on `shared/flatpickr-picker.js` stores `YYYY-MM-DD`.
  No new package.
- [`/docs/Li/LITHIUM-PROPS.md`](/docs/Li/LITHIUM-PROPS.md) documents
  rows, sections, editors, the resolver, preferences, and adoption.

### Lessons learned

- The "user profile" in Lithium is GlobalSettingsService on
  `window.lithiumSettings`. It is synced to the account at login, so
  profile prefs need no new QueryRef or migration.
- A separate Favourites section moved rows out of context. Andrew
  preferred filters plus hover icons, with rows left in place.
- Favourites and Hidden are not symmetric filters. Favourites
  restricts the list, and Hidden includes rows. Ask about each toggle's
  meaning rather than mirroring the first one.
- Tabulator `movableRows` with `rowHandle` on the name column leaves
  value cells free to edit. A drop across groups is undone by
  repainting from the panel rows.
- With `groupBy`, each section's name prefix is its sort key. Adding or
  removing a synthetic section renumbers every other one, so tests that
  pin section names change with it.

### Handoff

Phase 14b is complete. Do not start the next phase until Andrew names
it. Phase 15 (field items and a sample datasource) is next in order,
and Phase 16 has not started. Still deferred, and not part of 14b:
setting Editor to Color on 054 keys 21 and 24, and dropping key 24
from the 053 item lists. The code still maps both keys by name. No
`Date` key exists in 054 yet. The next Acuranzo file is 1442. The next
caller-facing QueryRef is 173.

**2026-11-03 — `acuranzo_1442.lua` drafted, waiting for apply.** Andrew
chose to do the cleanup before Phase 15. 1442 sets `Editor` to `Color`
on 054 keys 21 and 24 and removes 24 from the `attributes` lists of 053
keys 0, 16, 20–22, 24, 28, 29, 32, 34, 35, 38, 52, and 61–63. The
reverse restores the 1441 and 1436 JSON. luacheck is clean. The Lithium
special-cases for 21 and 24 stay until Andrew applies it. After apply:
strip them, then check in the browser that Coloris opens and saves on
keys 21, 24, and 25. The next Acuranzo file is 1443.

**2026-11-03 — `acuranzo_1442.lua` applied.** Andrew reported it
applied. Lithium now reads the Color editor from 054 `Editor` (the
`COLOR_KEYS` override is gone) and takes item attribute lists from 053
as-is (`REPORT_ONLY_KEYS` is gone). Coloris fix: the picker opens on
click, not focus, so the Color editor now clicks its new input when it
renders, and it commits on Coloris `close` as well as `change`, so
closing without a change no longer leaves the cell stuck in edit.
A chrome test checks the picker opens and the value saves once.
`npm test`: 81 files, 1398 passed, 12 skipped. ESLint at baseline (163
src, 8 tests). Stylelint clean. Browser check for keys 21, 24, and 25
waits for Andrew. Phase 15 not started. The next Acuranzo file is 1443.
The next caller-facing QueryRef is 173.

**2026-11-03 — Retained colour swatches (Andrew: "swatches shown on all instances
of the color picker", after Pavillion). New `src/shared/coloris-picker.js`
is Lithium's one Coloris entry, ported from Pavillion's
`ui/src/common/coloris.js`: swatches are the theme's
`--lithium-picker-swatches` list (none defined yet) then the user's own,
capped at 24, saved on the profile at `colorPicker.swatches`. Under the
swatches a row holds + (add the current colour), − (remove a colour the
user added) and ✕ (Coloris's close, moved there). Round swatches, Hex /
RGB / HSL toggle. `src/styles/vendor-coloris.css` holds the styles. The
property panel's Color editor now goes through it. New
`coloris-picker.test.js` (5 tests). `npm test`: 82 files, 1403 passed,
12 skipped. ESLint at baseline (163 src, 8 tests). Stylelint clean.
Pavillion's Ctrl+wheel picker zoom is not ported. Phase 15 not started.

**2026-11-03 — Coloris wrapped, Phase 15 started.** Swatches and picker polish done after Andrew's browser check (row layout, white / #9a9a9a buttons, 12px value field). Phase 15 In progress: `reports-sample.js` (sample parse, columns, datasource names, field labels, unbound list) with 6 tests. UI wiring next.
No migration; next Acuranzo file 1443.

---

## Phase 15 — Field items and a sample datasource

### Goal

Place a field and show which column it is bound to.

### Entry gate

Phase 14 Status complete.

### Work items

- [x] 15.1 A session-only sample JSON datasource. The operator pastes
       or imports an array of objects. Column names are the union of
       keys. This JSON is not part of the definition export.
- [x] 15.2 Add field text, number, and datetime items. The property
       panel's Field editor lists those column names. Datasource
       names come from the definition's `datasources` array.
- [x] 15.3 The canvas label is the field name. An empty binding shows
        the type caption and a toast when the operator exports, and
        export is refused for that item. Compose will warn later. The
        designer does not invent a value. (Per `LITHIUM-INS.md` rule 1,
        no silent fallback — the operator must pick or fill a binding.)
- [x] 15.4 Browser: bind `customer`, export, confirm `field` and
       `typeKey` 20, reload, and see the binding.
- [x] 15.5 Number and datetime items edit a `mask` string. The
       canvas label stays the field name. An empty mask means the
       default in Value format. Export round-trips `mask`.

### Done means

A field item round-trips, and the sample data never appears in the
exported definition. Empty bindings block export with a toast.

### Exit gate

Lint, unit tests, and the browser check.

### Accomplished

- Second toolbar: Field category (`fa-database`) after Static. Third
  toolbar swaps by `data-reports-category`: Field shows Text (20),
  Number (21), Datetime (24), Sample (import a JSON file) and Paste
  (clipboard). `selectCategory('static'|'field')`.
- `reports-sample.js`: `parseSample` (array of objects; columns are the
  union of keys, first seen), `datasourceNames`, `fieldItemLabel`,
  `unboundFields`, `FIELD_TYPES`. `reports-fields.js` is the manager
  mixin: field buttons, `loadSample` / `sampleColumns`, export check.
- The sample lives on the manager (`_sample`) for the session only. It
  is never written to the definition, so export and save cannot carry it.
- Canvas paints field items with the bound name, or the type caption in
  grey italic when unbound (`data-unbound`).
- Property panel: 054 `Editor` Field becomes a List of sample columns
  (the current value stays listed; typed Text when no sample is loaded);
  Datasource lists the definition's `datasources`. Mask is an editable
  Text row for every mask type (empty: number shows the plain-decimal
  note, datetime the Value-format default note). Mask is trimmed.
- Export with any unbound field is refused with an error toast naming
  the count and the first item.
- Tests: `reports-sample.test.js` (6), `reports-fields.test.js` (4);
  updated items/chrome/gaps tests. 84 files, 1413 passed, 12 skipped.
  ESLint baseline (163 src, 8 tests); stylelint clean.

- Follow-up (audit footer): with `acuranzo_1443.lua` applied, the
  Reports footer now carries `AuditInfoFooter` after save/cancel, as in
  Query Manager. `reportAuditRow` shows `updated_*` when set, otherwise
  `created_*` (each save is a new revision, so the saver is
  `created_id`). Save refreshes the row and footer. Test:
  `reports-audit-footer.test.js` (4). 84 files, 1418 passed.

### Browser check for Andrew (15.4)

Field → Number or Datetime on Detail; Export is refused while unbound.
Sample or Paste a JSON array (e.g. `[{"customer":"A","total":3}]`), pick
`customer` in Field, export, confirm `field` and `typeKey`, reload, and
see the binding. Set a Mask and confirm it round-trips.

### Status

**complete — 2026-11-03.** Andrew checked fields, the sample, the band
floor, and the audit footer (after `acuranzo_1443.lua`): "Everything
looks good." 84 files, 1418 passed; ESLint baseline.

### Lessons learned

- Reports save appends a revision, so `updated_id` stays 0. The footer
  shows `updated_*` when set and falls back to `created_*`.
- QueryRef 165 needed the audit columns (1443) before the footer could
  show anything; check list queries for audit columns before wiring.

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
