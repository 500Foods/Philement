<!-- markdownlint-disable MD007 MD024 MD013 MD036 -->

# Reporting Plan

**Date:** 2026-10-10
**Status:** Phase 0 complete. Approved 2026-10-10. Next is Phase 1.
**Home:** Manager **24**, Report Manager, already registered.
**Design:** Acuranzo (shared product database). Lookups 0–199. Migrations
1000–1999.

Lithium is the designer. It writes a versioned JSON definition. The
Helium script `Reports.Render` reads that definition plus zero or more
JSON datasources, builds an intermediate document, and emits a format.
PDF is the default stage. The build order is this index. CATCHUP stays
the active Lithium plan until Andrew pulls this one in
([`/elements/003-lithium/CATCHUP.md`](/elements/003-lithium/CATCHUP.md)).
This plan does not move Course Manager, Scripting invoke, or the
lifecycle fix.

Argent Phase 15 "Report queries" in
[`/docs/H/plans/ARGENT_PLAN.md`](/docs/H/plans/ARGENT_PLAN.md) is a set of
bookkeeping QueryRefs. It is a different piece of work.

## Phases

One phase per conversation, top to bottom. A phase whose entry gate
says earlier phases may still be open follows that gate. Every other
phase waits until the previous phase's Exit gate is green. A packet
phase ends when the packet is handed over, and it is complete when
Andrew reports the apply.

| Phase | What | Kind | Effort | Status |
| --- | --- | --- | --- | --- |
| 0 | Design lock | Decision | Easy | Complete |
| 1 | Contracts and fixtures | Docs + fixtures | Medium | Not started |
| 2 | Units and geometry | JS + Vitest | Easy | Not started |
| 3 | Document model and undo | JS + Vitest | Medium | Not started |
| 4 | Manager shell, import, export | Lithium UI | Medium | Not started |
| 5 | Page surface | Lithium UI | Easy | Not started |
| 6 | Bands and resize | Lithium UI | Medium | Not started |
| 7 | Section grid | Lithium UI | Easy | Not started |
| 8 | Rulers and the mouse line | Lithium UI | Medium | Not started |
| 9 | View scale | Lithium UI | Easy | Not started |
| 10 | Static text | Lithium UI | Medium | Not started |
| 11 | Lookup 053 repair | Helium, human apply | Medium | Not started |
| 12 | Lookup 054 fonts | Helium, human apply | Easy | Not started |
| 13 | Lookup 057 page sizes | Helium, human apply | Easy | Not started |
| 14 | Property panel | Lithium UI | Medium | Not started |
| 15 | Field items and a sample datasource | Lithium UI | Easy | Not started |
| 16 | One group level | Lithium UI | Easy | Not started |
| 17 | Save and load packet | Helium, human apply | Medium | Not started |
| 18 | Revisions in the manager | Lithium UI | Medium | Not started |
| 19 | Compose skeleton | Helium seed, human apply | Medium | Not started |
| 20 | Value format | Helium amendment | Medium | Not started |
| 21 | Paging | Helium amendment | Easy | Not started |
| 22 | System datetime and system user | Lithium UI + Lua | Medium | Not started |
| 23 | Page number and page count | Lithium UI + Lua | Medium | Not started |
| 24 | HTML emit | Helium amendment | Easy | Not started |
| 25 | Preview | Lithium UI | Medium | Not started |
| 26 | Generation log | Lithium UI | Medium | Not started |
| 27 | Over-cap preview | Lithium UI | Easy | Not started |
| 28 | Query designer | Lithium UI | Medium | Not started |
| 29 | Query execution | Helium amendment | Medium | Not started |
| 30 | Seed lua-pdfkit | Helium library pack | Easy | Not started |
| 31 | PDF text | Helium amendment | Medium | Not started |
| 32 | Result cap | Helium amendment | Easy | Not started |
| 33 | Text formula | Helium amendment | Medium | Not started |
| 34 | Formula editor | Lithium UI | Medium | Not started |
| 35 | Formula check | Lithium UI + Lua | Easy | Not started |
| 36 | Number and datetime formula | Helium amendment | Medium | Not started |
| 37 | Formula runtime errors | Helium amendment | Easy | Not started |
| 38 | Count, unique, and sum | Helium + designer | Medium | Not started |
| 39 | Page totals | Helium + designer | Medium | Not started |
| 40 | Average and weighted | Helium amendment | Medium | Not started |
| 41 | Median | Helium amendment | Medium | Not started |
| 42 | Mode | Helium amendment | Medium | Not started |
| 43 | Min and max | Helium amendment | Medium | Not started |
| 44 | First and last | Helium amendment | Medium | Not started |
| 45 | Group-key function | Helium + designer | Medium | Not started |
| 46 | Page-break predicate | Helium + designer | Easy | Not started |
| 47 | Line-break predicate | Helium + designer | Easy | Not started |
| 48 | Header and footer context | Helium amendment | Medium | Not started |
| 49 | CSV emit | Helium amendment | Easy | Not started |
| 50 | Embedded image | Lithium UI + Lua | Medium | Not started |
| 51 | Reference and URL images | Helium amendment | Medium | Not started |
| 52 | PDF image stored once | Helium amendment | Easy | Not started |
| 53 | Seed lua-zint | Helium library pack | Easy | Not started |
| 54 | Barcode SVG | Lithium UI + Lua | Medium | Not started |
| 55 | Barcode PDF raster | Lithium UI | Easy | Not started |
| 56 | Seed d3.lua | Helium library pack | Easy | Not started |
| 57 | Chart design lock | Decision | Easy | Not started |
| 58 | Charts in the designer | Lithium UI | Medium | Not started |
| 59 | Bar chart | Helium amendment | Medium | Not started |
| 60 | Group and record chart scopes | Helium amendment | Medium | Not started |
| 61 | Line charts | Helium amendment | Medium | Not started |
| 62 | Combo chart | Helium amendment | Medium | Not started |
| 63 | Pie chart | Helium amendment | Medium | Not started |
| 64 | Custom chart | Helium amendment | Medium | Not started |
| 65 | Chart PDF raster | Lithium UI | Easy | Not started |
| 66 | SVG emit | Helium amendment | Easy | Not started |
| 67 | PNG export | Lithium UI | Easy | Not started |
| 68 | Native PDF contract | Decision | Easy | Not started |
| 69 | Zint PDF payload | Port repository | Medium | Not started |
| 70 | D3 PDF payload | Port repository | Medium | Not started |
| 71 | Place native PDF objects | Helium amendment | Medium | Not started |

Seventy-two phases, numbered 0 through 71. Each one is Easy or
Medium, and each one closes on a single behavior. Easy is a
fixture, a probe, or a decision. Medium adds a designer surface
or a browser pass. A sitting that ends early marks that phase in
progress and keeps its number.

A phase that hands a packet to a human is one sitting of agent work
plus the wait for the apply. Its Status block records the wait.

What a finished phase lets an author do. Later phases add to this.
They do not replace it.

| After | The author can |
| --- | --- |
| 10 | Place static text and export the definition file. |
| 18 | Save a revision and open it again. |
| 25 | Preview the HTML export. |
| 26 | Read the generation log and its three counts. |
| 29 | Run a QueryRef from a stored report with no sources array. |
| 31 | Download a text PDF. |
| 35 | See a syntax error in the formula editor. |
| 36 | See a pack-date code and a masked number. |
| 44 | Put the built-in totals on a footer. |
| 49 | Download CSV. |
| 54 | See a barcode in the preview. A PDF barcode is a raster until Phase 71. |
| 71 | Get charts and barcodes as vectors in the PDF. |

**Resume:** Phase 1 is next when Andrew pulls reporting in. CATCHUP
stays P0 until then. Phase 0's Exit gate is green.

## How to use this document

- Work **one phase at a time**, top to bottom.
- This plan follows [`/docs/PLANNING.md`](/docs/PLANNING.md). The
  steps below match that file.
- A phase whose entry gate says earlier phases may still be open
  follows that gate. Every other phase waits until the previous
  Status is complete and its Exit gate is green.
- Each phase has one **Done means** line. That is the testable state.
- Mark a work item `[x]` only when that item's verification passed.
- Defer with `[~]` plus one line and the phase it moves to.
- Before marking a phase complete, write Status, Accomplished,
  Lessons learned, and Handoff, and append the Working Log. Then
  stop. The next phase starts in a new conversation.
- A sitting that ends early sets Status to **in progress**, names
  the last finished work item, and does not mark the phase complete.
- A Helium phase ends when the packet is handed over. It is complete
  only after Andrew reports the apply. The agent does not run SchemaTool
  or SchemaHelper apply, and does not apply a migration.
- The locks and the phase body are the contract. A Working Log entry
  keeps the numbers and the decisions of the day it was written.

## Implementor workflow

Each phase is its own conversation. A new conversation may start with
only a pointer to this file.

1. Read Resume, the phase-index Status column, and the previous
   phase's Status, Accomplished, Lessons learned, and Handoff.
2. Re-read this phase Goal, Work items, Done means, and Exit gate.
   Open a lock when the phase cites it.
3. Ask the qualifying questions before editing. Ask when a lock names
   a behavior and leaves a stored string, a column, or an order
   unpinned.
4. Wait for approval before editing source on a phase that has not
   been approved.
5. Ask again when a requirement is ambiguous. Do not guess a schema
   change, a stored string, or a column name.
6. Update the Working Log when a major piece lands.
7. The named command has passed. Then write Status, Accomplished,
   Lessons learned, and Handoff. Check work items and set Status
   complete only after that. Intent to verify is not verification.
   A sitting that ends before Done means writes in-progress Status
   and Handoff, and leaves unfinished items unchecked.
8. Stop. The next phase starts in a new conversation.
9. Never apply a database migration. Prepare the packet and hand it
   over. Re-check the next free Acuranzo file, QueryRef, and lookup
   id on disk before numbering anything. The snapshot in this file
   goes stale.
10. Lithium rules win on the SPA:
   [`/docs/Li/LITHIUM-INS.md`](/docs/Li/LITHIUM-INS.md) and
   [`/elements/003-lithium/AGENTS.md`](/elements/003-lithium/AGENTS.md).
   Files stay at or under 1000 lines. CSS first. `element.style` is
   limited to drag geometry (`top`, `left`, `width`, `height`).
   No `alert`, `confirm`, or `prompt`. Log through `log(...)`.
   Preview HTML goes through DOMPurify and a sandboxed iframe.
11. Helium rules win on packets:
    [`/elements/002-helium/AGENTS.md`](/elements/002-helium/AGENTS.md),
    then Designs, packs, and numbers in
    [`/docs/He/GUIDE.md`](/docs/He/GUIDE.md). One file, one change.
    Copy an existing seed. Do not invent macros.
12. Do not add a Hydrogen `test_NN` script, a new npm dependency, or a
    new manager id unless a work item in this plan says so.
13. Do not log JWTs, script params that contain report data, or
    datasource rows in normal logs.

### Verification

| Change | Check |
| --- | --- |
| Lithium JS / CSS | `npm test`, `npm run lint`, `npm run lint:css` from `elements/003-lithium` |
| HTML template | `npm run templates:copy` after the edit |
| Designer UI | Browser pass of the phase's flow (click, drag, save). A screenshot is not the gate |
| Helium packet | Test 31 and Test 98 when Andrew asks. Payload regenerate (`mka` or `payload-generate.sh`) before tests 30–38 or 71. `mkt` does not refresh the archive |
| Live render | Andrew runs the conduit script call after the seed is applied and reports the result |

## Priority

| | |
| --- | --- |
| **Band** | P2. Starts when Andrew pulls it in. CATCHUP remains P0. |
| **Effort** | XL. Each phase is one behavior and one gate. |
| **Done** | Phase 0 of 72. Phases 1–71 have not started. |
| **Why now** | Hydrogen's host work is close to done. The report writer is the next product surface, and the lookups and table already exist. |
| **Do not start casually** | The JSON contract has to be agreed before the designer and the Lua script both grow around it. |

## Scope

| Tree | Role in this plan |
| --- | --- |
| `elements/003-lithium/src/managers/reports/` | Manager 24. Shell, canvas, rulers, properties |
| `elements/003-lithium/src/managers/reports/model/` | Pure units, document, undo. Vitest imports these |
| `elements/003-lithium/tests/unit/` | Tests for the model and, later, `invokeScript` |
| `elements/003-lithium/tests/fixtures/reporting/` | Definition, datasources, expected intermediate, expected HTML |
| `docs/Li/` | Contract note written in Phase 1. This plan stays the gate list |
| `elements/002-helium/acuranzo/migrations/` | Lookup repair, QueryRefs, `Reports.Render`, lua-pdfkit, lua-zint, d3.lua |
| `elements/001-hydrogen/hydrogen/src/` | No new C. Lithium calls the existing Reporting endpoint. The script does not. |

Runtime truth for the manager id is `manager-loader.js` and
`config/lithium.json`: id **24**, key `024.Report Manager`, module
`src/managers/reports/reports.js`. The placeholder implements
`teardown()` only. [`/docs/Li/LITHIUM-MGR-MAIN.md`](/docs/Li/LITHIUM-MGR-MAIN.md)
still says Reports is id 12. Id 12 is Calendar. Fix that sentence in
Phase 4. Do not renumber the manager.

## Goals

1. A designer that places bands and items on a page, with a dot or
   line grid, resizable bands, and rulers that track the pointer.
2. A versioned JSON definition that round-trips through a file and,
   after Phase 18, through the `reports` table.
3. One Lua program, `Reports.Render`. Compose maps datasources onto
   the definition and assigns pages. Emit turns that intermediate
   document into the requested stage. `pdf` is the default.
4. PDF is the default export. HTML, SVG, and CSV are the other script
   stages, in the phase-index order. SVG is one document with page
   breaks marked. PNG is that SVG rasterized by Lithium through
   `image_scale`. A label-sized report is one page. HTML is an export
   and the preview medium at once.
5. PDF from lua-pdfkit, barcodes from lua-zint, charts from d3.lua.
   The script embeds image bytes it has already resolved. Lithium calls
   `POST /api/reporting/image_scale` when a raster is required.
   Hydrogen does not grow a PDF, barcode, or chart library.
6. Charts and barcodes are first-class items. They render as SVG in the
   HTML, preview, and SVG paths, and the end state for PDF is the
   vector objects the zint and d3 ports emit natively. Any raster
   stopgap before that lands is named per phase, not assumed.
7. CSV as the tabular download. A native spreadsheet writer waits.
8. Report authors write Lua functions in the definition JSON.
   Each function names its return type. `Reports.Render` compiles
   and runs them, then formats a number or a datetime with the item
   mask. A field formula receives the current record. A header,
   footer, or break receives the context for that band. Group and
   report totals are known before those headers are formatted.
9. The existing lookup families stay the catalog of object types,
   attributes, and page sizes.
10. Built-in aggregates: count, unique, sum, avg, weighted mean,
    mean, mode, and median.
11. Chart items bind data with full scope flexibility: the band's own
    datasource, a named datasource, the current group, or a custom
    function, placed on any band.
12. Image elements carry their own resolution. A repeated image is
    stored once in the PDF and each placement references it.
13. A number, date, aggregate, or parameter prints through a format
    mask. Null is blank. Align and font are honored. Text clips to
    its box. A group header stays with its first detail row.
14. A datasource may name a QueryRef and the report's parameters.
    The canvas still designs against sample rows. Phase 29
    runs the query.

## Out of scope until a later plan

- Crosstabs, subreports, packages, shapes, and memos. The lookup rows
    can stay. The palette hides them. Chart keys 42 and 44, and barcode
    keys 43 and 45, are v1.
- Chart interactivity. The exported chart is static SVG or a PDF
    vector object; hover, zoom, and tooltips are a later idea.
- A user's own SVG entering a PDF as a vector. Phase 71's native
  output serves charts and barcodes first; a user-supplied SVG
  rasterizes through `image_scale` until a later plan extends the
  ports' native output to imported art.
- A native XLS writer. CSV is the spreadsheet stand-in.
- A SQL catalog inside the designer. A datasource is a JSON sample
  or a named QueryRef. The designer does not edit SQL.
- Browser PDF generation (`jsPDF`) and a PDF thumbnail viewer.
- Mailing a report. Mail Relay already exists for mail.
- A new Hydrogen subsystem, config letter, or C route. PDF, barcodes,
  and charts stay in Lua. Lithium calls the Reporting endpoint that
  already shipped when a raster is required.
- Rewriting CATCHUP, Course Builder, or Argent's financial queries.
- Measuring wrapped text inside Lua. v1 bands have a fixed height.
- Handing a user formula the Hydrogen host API. `H` stays in
  `Reports.Render`. `require` stays the loader for lua-pdfkit,
  lua-zint, and d3.lua.
- Datasource payloads over the conduit script param cap (256 KiB
  default, `ClientInvokeMaxParamsBytes`).

## What already exists

### Lithium

Manager 24 is a 49-line placeholder. It tells the operator the module
is under development. `conduit.js` can run QueryRefs.
`invokeScript` is not in the client. Scripting Manager does not call
`POST /api/conduit/script`. CATCHUP Phases 7–10 are the planned home
for that helper. Phase 25 of this plan adds it if those phases have
not landed, and reuses it if they have.

`closeManager` calls `destroy()` and ignores `cleanup()` and
`teardown()`. The report manager implements `destroy()` and
`cleanup()` as the same function so either lifecycle path releases
the canvas listeners.

### Helium

| Object | Where | What it holds |
| --- | --- | --- |
| `reports` table | [`acuranzo_1015.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1015.lua) | `(report_id, rev_id)` primary key. Columns `report`, `thumbnail`, `name`, `design`, `scripts`, `parameters`, `summary`, plus the usual audit and `collection` JSON. Created 2025-09-13. No QueryRef reads or writes it. |
| Lookup 053 | [`acuranzo_1086.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1086.lua) | Report Object Types, keys 0–60. |
| Lookup 054 | [`acuranzo_1087.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1087.lua) | Report Object Attributes, keys 0–40. |
| Lookup 057 | [`acuranzo_1090.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1090.lua) | Two page-size rows. |

Lithium QueryRefs 053 and 054 are not these lookup ids. QueryRef 053
([`acuranzo_1146.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1146.lua))
is Get Themes and reads lookup 41. QueryRef 054
([`acuranzo_1148.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1148.lua))
is Get Icons and reads lookup 40. The reader for a lookup id is
QueryRef 026, Get Lookup, parameter `LOOKUPID`
([`acuranzo_1117.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1117.lua)).

Lookup 053 `value_int` groups the rows. This reading is from the
names, and Phase 0 confirms it:

| value_int | Group | Examples |
| --- | --- | --- |
| 0 | Structural | Report Top, Package, Report Bottom, Report, Section |
| 1 | Static | Text, Number, Image, Memo, DateTime, Shape, Chart, Barcode, SubReport |
| 2 | Field | Text, Number, Image, Memo, DateTime, Shape, Chart, Barcode |
| 3 | Calculated | Counter, Text, Number, Image, Memo, DateTime, Shape, Chart, Barcode |
| 4 | Aggregate | Text through Barcode |
| 5 | CrossTab | Text through Barcode |
| 6 | Band | Package through Package Footer, plus Ruler |
| 7 | System | DateTime, User, Server, Version |

`key_idx` is the identity. `sort_seq` repeats inside a group (Field
Shape and Field Chart are both 6). Persist `key_idx`.

Lookup 054 is the property catalog. Keys 0–5 are position (Top, Left,
Width, Height, Align, Fit). Key 7 is Field, key 8 is Object Datasource.
Keys 15–17 are font family, size, and style. Keys 18–19 are report
title and template. Keys 20–24 and 40 are the grid. Keys 25–28 are the
ruler. Keys 30–39 are page size, orientation, units, and margins.

Lookup 053 carries chart keys 42 (Static Chart), 44 (Field Chart), 46
(Calculated Chart), 40 (Aggregate Chart), and 54 (CrossTab Chart), and
barcode keys 43 (Static Barcode) and 45 (Field Barcode). No lookup 054
row describes a chart or a barcode, and none of the chart or barcode
types advertises font or field attributes. Chart and barcode options
therefore live in their own panels beside the property panel, which is
what removes the need to wait on Phases 11–13 for them.
Most object types in lookup 053 advertise only
`attributes: [0, 1, 2, 3, 4, 14, 24, 29]`
(Top, Left, Width, Height, Align, Rotation, Grid Background Color,
Link). Page Header and Detail advertise a longer list that includes
fonts. A field cannot show its datasource or its font with the lists
as they stand. Phases 11–13 repair the v1 types.

Known seed defects, fixed in Phases 11–13:

- Lookup 053 key 7 (System Server) and key 16 (Group Footer) store
  `"Iion"` where the other rows store `"icon"`.
- Lookup 053 key 52 (CrossTab DateTime) has an unclosed `<fa>` tag.
- Lookup 057 key 1 is labeled `US Letter Legal` and its collection is
  US Letter landscape (`11 in` by `8.5 in`, page size `US Letter`).
- Lookup 057 has no Legal portrait, A4, or Tabloid row.
- Lookup 054 font list is Helvetica, Open Sans, Cairo, Courier New,
  Lexend. Vanadium is the product face and is absent.
- The seed has no Page Number or Page Count type. Keys 0–60 are
  taken. Phase 0 assigned Page Number to key 61 and Page Count to
  key 62. Phase 11 writes them. Re-check the max key before writing
  the file.

Number snapshot on 2026-10-08, for orientation only:

- Highest Acuranzo file: `acuranzo_1385.lua`.
- Highest lookup id: **068**.
- Caller-facing QueryRefs in the low series run through **155**
  (`acuranzo_1381.lua`). `acuranzo_1145.lua` also sets
  `cfg.QUERY_REF = "1145"` for auth test data. Do not treat 156 as
  free without listing every `cfg.QUERY_REF` again.

Script seeds copy the `scripts` insert shape in
[`acuranzo_1376.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1376.lua)
(`System.Info`). That row is `invokable = 0` and `mcp_access = 1`
because it is an MCP tool. `Reports.Render` is the opposite:
`invokable = 1`, `mcp_access = 0`. Phase 0 confirmed that.
Primary key of `scripts` is `(group_name, script_name)`.

### Retired Acuranzo client

The TMS WEB Core app is at `/mnt/extra/Projects/acuranzo/`. Pascal
sources are not on this machine. `Acuranzo-Client-Source.zip` contains
the `.dpr` and the README, not `Units/`. The useful record is the
compiled program `AcuranzoClient_6_1_5045.js` and
`CSS/AcuranzoClient-Reports.css`. `UnitReports.html` in that tree is
empty. The `.dpr` names `UnitReports.pas` as the form.

What that build actually did:

- A report list with new, clone, delete, search, upload, download,
  mail, and print controls, plus an info pane (SunEditor) and a JSON
  pane.
- A designer. `Reports.jsonData` held `Report` (page and grid
  properties, keys matching lookup labels such as `"Page Width"`) and
  `Sections[]`. Each section had `Name`, `Title`, `Link`, `Height`,
  grid properties, and `Items[]` with `Name`, `Title`, `Link`, `Top`,
  `Left`, `Width`, `Height`.
- Sections rendered as a header bar plus a resizable body. `Ruler`
  sections were skipped. A synthetic Report Top and Report Bottom
  framed the canvas.
- `getReportPixels` parsed strings like `"8.5 in"` at 96 CSS pixels
  per inch, and also `mm`, `cm`, and `px`.
- `addBackgroundPattern` drew an SVG tile: dots as circles, grid /
  dashed / dotted as paths. A `ResizeObserver` redrew it. Grid style,
  spacing, color, and weight were CSS variables.
- `createRuler` drew an SVG ruler in inches or centimetres, with a
  `.cursor-line` that followed `mousemove` and hid on `mouseleave`.
- Items used absolute `top` / `left` / `width` / `height`. Section
  resize and item drag used interact.js. Zoom used a separate
  panzoom object (`pzReportDesigner`).
- Selection filtered the property grid with lookup 053's attribute
  list. The compiled code reads `Lookups[-53]` for the family header.
  Do not assume Lithium's lookup cache uses a negative id. Read
  `src/shared/lookups.js` in the phase that binds the panel.
- Undo stored a string diff of the JSON and reapplied it.
- A PDF viewer (pdf.js) with thumbnails, zoom, rotate, crop, and
  column layouts. `jsPDF` was loaded on the page. This plan does not
  bring either library back.
- This pass did not find a write of the `reports` row. `report_id`
  appears as a hidden list column. Upload and download were the
  portable path that is visible in the compiled program. Treat the
  table as unused storage, not as a format we have to match byte for
  byte.

The behavior worth keeping is the page, the bands, the grid, the
rulers, and direct manipulation. The JSON keys are worth replacing.
Label strings break when a lookup caption changes. Format version 1
stores `key_idx` and stable item ids.

## Approved locks

Phase 0 approved these on 2026-10-10. Later phases implement this
text.

### Three documents

1. **Definition.** The file the designer exports. Format version 1.
2. **Datasources.** A JSON object. Each key is a datasource name from
   the definition. Each value is an array of objects. Zero keys is a
   legal static report.
3. **Intermediate.** The compose result. Pages of bands of items, with
   text already substituted and boxes in physical units. Emit reads
   this and does not see the original datasources.

### Definition shape

```json
{
  "format": 1,
  "name": "Orders",
  "title": "Orders",
  "page": {
    "sizeKey": 0,
    "orientation": "portrait",
    "units": "in",
    "width": "8.5 in",
    "height": "11 in",
    "margin": {
      "left": "0.25 in",
      "right": "0.25 in",
      "top": "0.25 in",
      "bottom": "0.25 in"
    }
  },
  "grid": {
    "visible": true,
    "style": "dots",
    "spacing": "0.125 in",
    "weight": "0.5",
    "color": "silver",
    "background": "white"
  },
  "font": { "family": "Vanadium Sans", "size": "10 pt", "style": "Normal" },
  "datasources": [
    { "name": "orders", "kind": "json", "shape": "list" }
  ],
  "bands": [
    {
      "id": "ph",
      "typeKey": 13,
      "height": "0.4 in",
      "items": []
    },
    {
      "id": "detail",
      "typeKey": 15,
      "datasource": "orders",
      "height": "0.25 in",
      "items": [
        {
          "id": "i1",
          "typeKey": 20,
          "field": "customer",
          "box": {
            "left": "0 in",
            "top": "0 in",
            "width": "2 in",
            "height": "0.2 in"
          },
          "align": "Left"
        }
      ]
    }
  ],
  "functions": []
}
```

A field text item stores `align`. Omitted align is `Left`. A number,
datetime, aggregate, or parameter item also stores `mask`. An item
stores `font` only when it overrides the definition font, in the same
shape. The style captions below are legal in both places.

`typeKey` is lookup 053 `key_idx`. Lengths are strings with a unit
suffix (`in`, `mm`, `cm`, `pt`, `px`). The designer converts at
96 CSS pixels per inch for the unzoomed view. Emitted HTML uses the
physical unit in CSS so print does not depend on that 96.

v1 `typeKey` allowlist:

| key_idx | Role |
| --- | --- |
| 12 | Report header |
| 13 | Page header |
| 14 | Group header. One group field. |
| 15 | Detail. One datasource name. |
| 16 | Group footer |
| 17 | Page footer |
| 18 | Report footer |
| 0 | Static text |
| 20 | Field text |
| 21 | Field number |
| 24 | Field datetime |
| 5 | System datetime |
| 6 | System user |
| 42 | Static chart |
| 44 | Field chart |
| 43 | Static barcode |
| 45 | Field barcode |
| 61 | Page number |
| 62 | Page count |
| 63 | Parameter. One declared report parameter. |
| 28 | Calculated text. Names a function whose `returns` is `text`. |
| 29 | Calculated number. Names a function whose `returns` is `number`. The item mask formats the raw number. |
| 32 | Calculated datetime. Names a function whose `returns` is `datetime`. The item mask formats the raw value. |
| 34 | Aggregate text. Mode, min, max, first, and last of a text value. |
| 35 | Aggregate number. |
| 38 | Aggregate datetime. Min, max, first, and last of a datetime. |

Rulers, Report Top, and Report Bottom are canvas chrome. They are not
bands in the file. Grid and ruler settings on the definition are view
defaults, stored so the next session opens the same page.

Chart and barcode items add a `chart` or `barcode` object described
under Charts and in Phase 54. Their series, scope, and options bind to
datasources, and their option panels are bespoke, not lookup 054 rows.

`functions` holds the author's Lua. The shape is in the next section.
An early definition stores `[]`.

A group band names one field or, from Phase 45, one Lua function that
returns the key. Compose sorts that key with Lua's ordinary `<`
unless the band sets `sort` to false, in which case it breaks only
when consecutive keys differ. Nested groups are later.

### Value format

Compose turns a value into text before it builds the intermediate
document. Emit, including CSV, prints that text and does not format
again. The item property is `mask`, a string. It is not a lookup
054 key. Text fields omit it. An image item's `format` is the
raster it stores (`png`, or `jpeg` when the source is already
JPEG). It is not a mask, and it is not an XO payload.

Null and a missing value are an empty string. The mask is not
applied. The page never shows `nil` or `null`. A column that is
absent from the row warns once per item, not once per row. A present
column whose value is null does not warn.

An omitted mask prints a number as a plain decimal, with no thousands
separator and no scientific notation, and a datetime as `yyyy-mm-dd`
when it has no time and as `yyyy-mm-dd hh:nn` when it does. Anything
else is the string value.

A number mask uses `#` for an optional digit, `0` for a required
digit, `,` as the thousands separator, and `.` as the decimal point.
A trailing `%` multiplies by 100 and suffixes `%`. Wrapping the
pattern in parentheses prints a negative in parentheses and drops the
minus sign. A positive value prints without the parentheses. A leading
`$` is a literal prefix. `#,##0.00`,
`$#,##0.00`, `(#,##0.00)`, and `0%` are the v1 set's shapes. There
is no locale. The comma and the dot mean the same thing on every
machine.

A date mask uses `yyyy`, `yy`, `mmm`, `mm`, `d`, `dd`, `hh`, `nn`,
and `ss`. `mm` is the month. `nn` is the minutes. `mmm` is the
English abbreviation `Jan` through `Dec`. `d` is the day without a
leading zero. `dd` is the day from 01 to 31. `hh` is the hour from
00 to 23. `yyyy-mm-dd` and `d mmm yyyy` are enough for v1. The value
may be `yyyy-mm-dd`, `yyyy-mm-ddThh:nn:ss`, or a number of unix
seconds. A string is the civil time written in it, with no zone
shift. A number is unix seconds, formatted in UTC. A mask that omits
the time tokens drops the time. A mask that asks for time on a
date-only value uses `00:00:00`. Anything else warns once and prints
the raw value.

A number rounds half away from zero. `1.235` with `#,##0.00` prints
`1.24`. `-1.235` prints `-1.24`. Zero prints as the mask says,
including `0.00`. Blank-if-zero is lookup 054 key 10 and stays off
the v1 attribute lists.

Aggregate numbers use the same mask. A parameter item uses it too.
A calculated number and a calculated datetime use the same mask on
the raw value the function returned. The function does not format
the text. An aggregate reduces those raw values, and the aggregate
item's own mask formats the reduction. A mask that does not parse
warns once, and the raw value is printed.

| Value | Mask | Prints |
| --- | --- | --- |
| null | `#,##0.00` | empty string |
| `1234.5` | omitted | `1234.5` |
| `1234.5` | `#,##0.00` | `1,234.50` |
| `1.235` | `#,##0.00` | `1.24` |
| `-1.235` | `#,##0.00` | `-1.24` |
| `-1234.5` | `(#,##0.00)` | `(1,234.50)` |
| `1234.5` | `(#,##0.00)` | `1,234.50` |
| `1234.5` | `$#,##0.00` | `$1,234.50` |
| `0.25` | `0%` | `25%` |
| `5` | `0.00` | `5.00` |
| `0` | `#,##0.00` | `0.00` |
| `no` | `#,##0.00` | `no`, and one warning |
| `2026-03-05` | omitted | `2026-03-05` |
| `2026-03-05T14:07:09` | omitted | `2026-03-05 14:07` |
| `2026-03-05T14:07:09` | `d mmm yyyy` | `5 Mar 2026` |
| `2026-03-05` | `yyyy-mm-dd hh:nn` | `2026-03-05 00:00` |
| `1772719629` | `yyyy-mm-dd hh:nn` | `2026-03-05 14:07` |

System datetime is not a field value. Compose reads one instant for
the whole generation, and every system-datetime item uses it. When
`params.now` is a unix number, that is the instant. When the call
omits it, the instant is `H.system.now()`. A normal preview omits
`params.now`. The Phase 22 fixture sends `1772719629`, the same
instant as the mask row above. `params.now` is a call parameter. It
is not a report parameter, it is not stored on the definition, and
it is not a field of `params._hydrogen`. The item adds
`params._hydrogen.tzoffset` minutes, where a negative offset is west
of UTC, and formats that civil time with the item mask. The shift is
the claim value. Compose does not look up the IANA name in `tz`. A
missing `tzoffset` formats the instant in UTC and adds an info log
entry. Field datetimes keep the rules above.

### Page rules

Align and the font keys are real. Compose reads Align and font
family, size, and style from the item, then from the definition
font. On an item, `align` is `Left`, `Center`, or `Right`. Omitted
means `Left`. `font`, when present, is `{ family, size, style }`
and overrides the definition font. `style` is `Normal`, `Bold`,
`Italic`, or `Bold + Italic`. Those four are the v1 set. Underline,
superscript, subscript, and strikethrough stay in Later. Phase 12
sets lookup 054 key 4's list to `Left`, `Center`, and `Right`, and
key 17's list to the four style captions. HTML uses the family,
the size, and the style. PDF uses align, size, and style as bold
and italic flags, and keeps the standard face the port already
carries. Embedding Vanadium stays in Later. Rotation, link, and
conditional color stay in Later.

Lookup 054 keys 9, 10, 11, and 12 (Number Format, Blank if Zero,
Zero if Blank, Negative Parentheses) stay off every v1 attribute
list. The `mask` string is the format.

Text clips to the item box. The canvas clips the same way, so the
cutoff is visible while designing. The band does not grow. Measuring
wrapped text stays in Later.

A group header is not left alone at the bottom of a page. If the
header plus one detail row do not fit in the remaining content box,
both move to the next page. If they do not fit on an empty page
either, compose warns and places them anyway. Heights are fixed, so
this is arithmetic.

A page header or footer sees the group that is open on that page and
the detail rows placed on that page. Page totals are reduced from
those rows after the page list exists. Both the page header and the
page footer can show them.

Compose is not a single pass. Group and report totals are known
before the headers that display them are formatted.

1. Resolve rows. An array in `sources` wins. A query datasource with
   no array waits for Phase 29, and until that phase it warns and
   contributes no rows.
2. Evaluate each detail-row function. Store the raw value. Format
   each detail item from that raw value and the item mask. A detail
   `ctx` has no aggregates, so a total is a header or footer item.
3. Reduce each group, and the whole report, from raw field values
   and those raw function results. Format the group header, the
   group footer, the report header, and the report footer from
   those totals. A calculated item on those bands may read
   `ctx.aggregates`. Its return is masked by its own type, and it
   is not fed back into the totals.
4. Paginate on fixed heights and the stored text. A group header
   that does not fit with its first detail row moves with that row.
   The header is not repeated on a later page. Its summary is every
   row of the group, including rows that land on a later page.
5. Fill `page.count` and `page.rows`. Reduce the page totals from
   the raw values of the detail rows on that page. Format the page
   header and the page footer.

Keep-together changes which rows land on a page, so step 5 reads the
finished list.

### Datasource kinds

`kind` `json` is an array passed in `sources`, or the session sample
while designing. That array is not stored in the definition. `kind`
`query` names a caller-facing QueryRef and the binds that fill it.
Sample rows still design the canvas. A real run uses the QueryRef.

```json
{
  "parameters": [
    { "name": "from", "type": "date", "default": "2026-01-01" }
  ],
  "datasources": [
    {
      "name": "orders",
      "kind": "query",
      "queryRef": 120,
      "params": { "from": { "parameter": "from" } },
      "shape": "list"
    }
  ]
}
```

`parameters` live on the definition. Each one has `name`, `type`
(`text`, `number`, `date`, or `boolean`), and `default`. A bind is
`{ "parameter": "<name>" }` or `{ "literal": "<value>" }`. The
`reports.parameters` column stays `{}`. The run strip's current
values are session state, passed as `params.parameters`, and they
override the defaults. Saved presets wait with the parameter form.

An array in `sources` wins, which is how fixtures and the canvas
sample run. When that array is absent, Phase 29 runs the
QueryRef. It loads the installed row the way
[`argent_2059.lua`](/elements/002-helium/argent/migrations/argent_2059.lua)
does: `SELECT code FROM queries WHERE query_ref = :QREF AND
query_type_a28 = 1`, then `H.altquery_sync(params._hydrogen.database, code, binds)`
with the resolved binds. `query_type_a28 = 1` is the installed
query. The database name is the session claim. When that name is
empty or is not a configured connection, compose adds a warn entry
and returns no rows for that datasource. The worker login is the
one configured for that connection. The script does not send the
session JWT. Row filters, detail sort, and code-to-caption columns
stay on the query. The report already sorts a group by its key.

### User Lua

Report authors write functions. The source is stored on the definition
and runs inside `Reports.Render`. It is not a separate `scripts` row.

```json
{
  "name": "pack_code",
  "returns": "text",
  "source": "return function(ctx)\n  return ctx.row.month\nend"
}
```

`name` is a Lua identifier, unique in the report. `returns` is the
value type below. `source` is a text chunk that returns a function
of one argument. The chunk starts with `return`. A bare
`function(ctx)` is not a Lua chunk. `Reports.Render` compiles it
with `load(source, name, "t", env)` under Lua 5.5. Mode `"t"`
accepts source text.

`env` contains `string`, `math`, `table`, `utf8`, `tonumber`,
`tostring`, `type`, `pairs`, `ipairs`, `next`, `select`, `pcall`,
`error`, `date` (`os.date`), and `time` (`os.time`). The chunk's
environment is that table, so the formula sees those names and the
other functions in the same report. It does not see the worker
globals. `H` stays in `Reports.Render`. The scripting progress hook
already bounds the worker by `max_runtime_seconds`. This plan adds
no hook and no C.

`returns` is the value type compose expects. The v1 value types are
`text`, `number`, `datetime`, and `boolean`. A chart scope of
`custom` uses `rows`. Lookup 053 stays the object catalog: a
calculated number is key 29, and a memo is a later object type.
Image, memo, and shape are not function returns in v1. Parameter
type `date` is a datetime value.

| `returns` | Who may name the function | What compose does with the raw value |
| --- | --- | --- |
| `text` | Calculated text, key 28. A group key. | Prints the string. No mask. |
| `number` | Calculated number, key 29. A group key. An aggregate. | The item mask formats the display. An aggregate reduces the raw number. |
| `datetime` | Calculated datetime, key 32. | The datetime mask formats the display. Min, max, first, and last use that same mask. |
| `boolean` | `pageBreak` and `lineBreak`. | True or false. It is not a printed item. |
| `rows` | Chart scope `custom`. | The rows the chart plots. No mask. |

A missing `returns` means `text`. The panel shows `text` and notes
the omission. Creating a function writes `returns`. The item type
and `returns` must agree. A calculated number names a function
whose `returns` is `number`. That function returns the raw number,
such as `1234.5`. Compose applies the item's `mask` with the Value
format rules, so `1234.5` with `#,##0.00` prints `1,234.50`. The
function does not insert the separators. A datetime function
returns a value the datetime mask already accepts. A text function
returns the string that is printed.

A value of the wrong type warns once per item, and that call prints
empty. A compile error stops compose and returns the message. A
runtime error on one call is a warning and an empty value, so one
bad row does not drop the rest of the report. An unknown function
name is a compose error.

Every `ctx` includes `parameters`, the run values with defaults
filled in. `ctx` otherwise depends on where the function runs:

| Call | What else `ctx` holds |
| --- | --- |
| Calculated item on a detail | `row` (the current record), `prev`, `index` (1-based in the detail source), `band`. No aggregates. |
| Group header or footer | `row` is the first row of the group. `prev` is the last row of the previous group, or nil. `index` is that first row's index in the detail source. `rows` is the group's records. `group.key`. `aggregates` for the whole group. `band`. |
| Page header or footer | `page.number`, `page.count`, `group` for the group open on that page, `page.rows`, and `aggregates` for those rows. |
| Report header or footer | `report.name`, and `aggregates` for every detail row. |
| Group, page, or line break | `row`, `prev`, and `index`. |

A calculated item stores `"function": "pack_code"`. The item does
not store the source.

Breaks, implemented in Phases 45, 46, and 47:

- `groupKey` names a text or number function. A text key compares
  as a string. A number key compares as a number, so the display
  mask does not split one value into two groups. The one-field
  group remains the path with no Lua. One level.
- `pageBreak` names a boolean function. A true result starts a new
  page before the current row. Any other value warns once and
  counts as false. Fixed-height paging still applies.
- `lineBreak` names a boolean function. A true result inserts one
  blank detail-height before the current row. Any other value
  warns once and counts as false. A newline inside a calculated
  value is kept by HTML (`white-space: pre-line`) and by the PDF
  text drawer. The stored band height does not change. Overflow
  clips.

The pack-date fixture in Phase 33 is an invented pattern (month
letter, week digit, day), standing in for the grower-specific codes
on a PTI label. It is not a PTI specification.

### Primitive aggregates

Lookup 053 key 35 (Aggregate Number) stores `op` and either `field`
or `function`. `weighted` also stores `weight`. The engine computes
these. An author who needs a different reduction writes a header or
footer function and reads `ctx.aggregates`.

| op | Result |
| --- | --- |
| `count` | Rows in the band's scope. Empty scope is 0. |
| `unique` | Distinct values. Numbers compare as numbers. Text compares as text. A datetime compares as its raw value. Empty scope is 0. |
| `sum` | Sum of the raw numeric values. |
| `avg` | Arithmetic mean. |
| `mean` | The same arithmetic mean as `avg`. |
| `weighted` | Sum of `value * weight`, divided by the sum of `weight`. `value` is the field or the function result. `weight` is a field. A zero sum of weights, or no numeric pair left, prints an empty string and adds one warn entry. |
| `median` | Middle value after a numeric sort. Even count: mean of the two central numbers. |
| `mode` | Most common value. Text uses key 34 (Aggregate Text). A number uses key 35. A tie keeps the smallest value under Lua `<` and records a warning. |
| `min` | Smallest value. Numbers compare as numbers. Text compares with Lua `<`, which is code-point order. A datetime uses the rule below. |
| `max` | Largest value, by the same comparison as `min`. |
| `first` | First non-null value in the scope's row order. This is not `min`. |
| `last` | Last non-null value in that same row order. This is not `max`. |
| `filled` | How many values are non-null. Empty scope is 0. |

An aggregate names a row column with `field`, or a detail function
with `function`. Exactly one of those is set. The function runs on
each detail row with that row's context. The reduction uses the raw
return values, not the masked text. `weight` stays a field name.

| Input | Ops the designer offers |
| --- | --- |
| Text, or a text function | `count`, `filled`, `unique`, `min`, `max`, `first`, `last`, `mode` |
| Number, or a number function | `count`, `filled`, `unique`, `sum`, `avg`, `mean`, `weighted`, `median`, `mode`, `min`, `max`, `first`, `last` |
| Datetime, or a datetime function | `count`, `filled`, `unique`, `min`, `max`, `first`, `last` |

`count`, `filled`, and `unique` are numbers and use key 35. So do
`sum`, `avg`, `mean`, `weighted`, and `median`. `mode`, `min`,
`max`, `first`, and `last` keep the input's type: text uses key 34,
a number uses key 35, and a datetime uses key 38. A text result has
no mask. A number result uses the number mask. A datetime result
uses the datetime mask.

`ctx.aggregates` stores the raw result. The mask is applied when
the aggregate item is formatted, so a header formula can read two
raw results and subtract them. A range is that formula. There is
no `range` op, and no standard-deviation op.

Nulls are skipped by every op except `count`. A skipped null does
not warn. `count` still counts the row. `first` and `last` walk
the scope in its row order: the query's order, then the group sort
when the band sorts. Page scope walks the rows in the order they
were placed on that page. An empty `min`, `max`, `first`, or
`last` prints an empty string.

A datetime is either a civil string or a unix number, using the
same two forms as Value format. The first non-null value picks
which one this total uses. A later value of the other kind warns
once per item and is skipped. Strings compare as written, which
is chronological for `yyyy-mm-dd` and `yyyy-mm-ddThh:nn:ss`.
Numbers compare as numbers.

A field has no declared type, so a value that does not fit the op
warns and is skipped at runtime. A function has `returns`, so the
designer hides the ops that type cannot support. Boolean functions
and `rows` functions are not aggregate inputs.

Group scope is the current group. Report scope is every detail row.
Page scope is the detail rows placed on that page. The group header
and the group footer show the same group totals. The report header
and the report footer show the same report totals. The page header
and the page footer show the same page totals. A non-numeric value
in a numeric op warns and is skipped. An empty numeric result is an
empty string. Phase 38 places the results on `ctx.aggregates` for
the header and footer formulas of that scope, keyed by the op and
the field or function name.

### Charts

A chart item is lookup 053 key 42 (Static Chart) or key 44 (Field
Chart). A Static Chart plots a datasource with fixed options. A Field
Chart uses scope `record`. The current record of the chart's window
supplies `keyField`. The chart does not repeat on every detail row.
A field barcode does. Both chart kinds carry a `chart` object.

```json
{
  "id": "c1",
  "typeKey": 44,
  "box": {
    "left": "0 in",
    "top": "0 in",
    "width": "6 in",
    "height": "3 in"
  },
  "chart": {
    "datasource": "orders",
    "scope": "group",
    "keyField": "grower",
    "categoryField": "date",
    "series": [
      { "field": "amount", "label": "Amount", "role": "bar",
        "stack": "s1", "axis": "left", "color": "Color-A" },
      { "field": "target", "label": "Target", "role": "line",
        "axis": "right", "weight": 4 }
    ],
    "options": {
      "title": "Orders by day",
      "legend": true,
      "legendPosition": "top-left",
      "yLeft": { "label": "Amount", "min": 0, "max": 100, "ticksEvery": 10 },
      "yRight": { "label": "Target", "ticksEvery": 5 },
      "grid": true,
      "gradient": true,
      "roundedBars": true,
      "dpi": 150
    }
  }
}
```

**Series first.** There is no single chart type to pick up front. The
author adds series, and each series picks a role: `bar` (optionally a
stacked member), `line`, `area`, or the category series of a pie. The
chart's shape falls out of the roles — all `bar` is a bar chart, `bar`
members sharing a `stack` are a stacked bar, all `line` is a line
chart, and a `line` beside `bar` on two `axis` sides is the combo.
`kind` survives as optional shorthand for the common cases and is
derived at compose otherwise. The combo case is first-class, not an
exception: the fixture set includes a stacked bar with an overlaid
line, which is also the shape of the D3 port's own demo chart.

**Scopes.** All four ship in v1. Phases 59 and 60 draw `band`,
`group`, and `record`. Phase 64 draws `custom` by calling the one
compiler Phase 33 owns:

| scope | Data the chart plots |
| --- | --- |
| `band` | Every row of `datasource`, which defaults to the band's own. |
| `group` | The rows of the current group. |
| `record` | The rows of `datasource` whose `keyField` equals the current record's. |
| `custom` | The rows a function from `functions[]` returns for the current context. That function's `returns` is `rows`. |

`datasource` may name any datasource on the definition, so a chart can
plot something completely different from the band it sits on. A chart
may be placed on any band. It draws on the first instance of its band
inside its window: once per group on a group band, once per page on a
detail or page band, once per report on a report band.

That window is the chart rule. A chart on a detail band is drawn
once on the page, on the first detail of that page, not once per
row. Scope `record` uses that first row. Scope `band` plots the
named datasource. A field barcode is a different item and repeats
on every detail row.

**Ports.** The SVG form comes from d3.lua. The end state for PDF is
the port's native PDF output (Phase 71): vector objects lua-pdfkit
places directly. Until Phase 71, Lithium rasterizes the chart SVG
through `image_scale` at the chart's own `dpi` option (a chart
defaults higher than text, because axis labels suffer at 72) and
passes the PNG in `params.rasters`. The script embeds those bytes.
The ports stay SVG producers for the HTML, preview, and SVG paths.
PNG export is Lithium rasterizing that SVG. Composing happens once;
the chart's data is resolved per the scope above and then drawn in
whichever form the stage needs.

**Pie.** Pie is in v1. The port does not have an arc yet. The
port as it stands is `scaleBand`, `scaleLinear`,
`axisLeft`/`axisRight`/`axisBottom`, `stack`, `curveBasis`, `line`,
and `range`. Phase 57 names the arc this plan will add. Phase 63
adds that arc in the port and previews one pie. Everything else in
the v1 set is already in the port.

**Fonts.** A chart uses the definition's face in the SVG paths and a
standard face in the PDF, the same rule as text.

### Intermediate shape

Compose output is JSON. One page object per page. Each page has the
bands that landed on it, in order. Item text is the substituted
string, already passed through the mask. Boxes stay in physical
units relative to the band. Emit reads `text`, `align`, `font`, and
`box`, and clips to the box. Emit does not format again.

```json
{
  "id": "amount",
  "typeKey": 21,
  "text": "1,234.50",
  "align": "Left",
  "font": { "family": "Vanadium Sans", "size": "10 pt", "style": "Normal" },
  "box": { "left": "0 in", "top": "0 in", "width": "2 in", "height": "0.2 in" }
}
```

A group break starts a group header and ends with a group footer.
Page header and page footer repeat. Report header is on page 1.
Report footer is on the last page.

Page breaks use fixed band heights against the content box (page size
minus margins). An item keeps the height stored on it. Text that does
not fit is the emit format's clipping problem. Lua does not measure
glyphs in v1.

### Emit

`params.stage` selects the stage on `Reports.Render`:

| stage | Result |
| --- | --- |
| `pdf` | Default. Base64 PDF bytes inside the JSON result when they fit. |
| `html` | One section per page with `break-after: page`. Export and preview medium. |
| `svg` | One SVG document. Page breaks are marked. |
| `csv` | Phase 49. One flat row per detail record. Data only. |
| `compose` | Intermediate JSON via `H.set_result_json`. |
| `check` | Phase 35. Compile the posted functions and return diagnostics. No rows are read. |

There is no script stage `png`. Phase 67 posts the SVG result to
`image_scale`.

PDF is the default stage. HTML is an export and the preview medium.
The preview is that HTML, rendered on purpose into a sandboxed iframe,
with DOMPurify before `srcdoc` (Phase 25). The parent page never
assigns it to `innerHTML`. Charts and barcodes are SVG in the HTML,
preview, and SVG paths. In the PDF they are native vectors at
Phase 71. Until then, Lithium rasterizes the item SVG through
`image_scale` and passes the PNG bytes back in `params.rasters`,
keyed by item id. The script embeds those bytes. It does not call
`image_scale`. A missing raster omits the item and adds a warning.

**The result cap is 1 MiB, and config cannot raise it.**
`scoreboard_update_result_json` starts at `SCOREBOARD_RESULT_JSON_MAX`
(1 MiB) and lets `ClientInvokeMaxResultBytes` only shrink that.
`H.set_result_json` returns no Lua values on success or on rejection.
A rejected body is a log line, and the job is left without a result.
The script measures the encoded JSON before it calls.

- A PDF that fits returns `{ "pdf_base64": "...", "log": [] }`.
  Base64 puts the ceiling near three quarters of a megabyte of PDF.
- A PDF that does not fit returns a small table:
  `{ "error": "result_too_large", "cap": N, "size": N, "log": [] }`.
  Compose already ran. Delivery failed. Raising the config key past
  1 MiB does nothing. Changing the ceiling is a C change, and this
  plan does not add C.
- `H.set_result(type, location)` stores two strings. It does not store
  the PDF. A stored-output path waits on a store. Hydrogen has no
  generic file or blob store. `/api/files/local` is gcode-specific.
  Recorded in "Later, not scheduled".


### Generation log

Every result includes `log`, an array of `{ "level", "message" }`.
`level` is `info`, `warn`, or `error`. Where this plan says a warning,
that is one entry with `level` `warn`. A note that does not change the
output is `info`. A failure that still returns a document is `warn`.
A failure that returns `error` also appends one `error` entry and does
not throw out of the worker.

Lithium counts the three levels. It does not expect a second counts
object. Phase 19 starts returning `log`. Phase 26 shows the entries
and flags the open report with the three counts. The counts are the
last generation's session state. They are not columns on `reports`.

`Reports.Render` is one invokable row. lua-pdfkit, lua-zint, and
d3.lua are other rows, loaded with `require("group.script")`. Author
formulas are not rows. Hydrogen's DB searcher splits on the first dot:
group, then the rest of the name as `script_name`. `AllowDBModuleLoad`
has to be on for those three libraries. `require("lua-pdfkit")` is
rejected because it has no dot. The entry points are `lua-pdfkit.init`
and `zint.barcode`; the d3.lua entry point is pinned at the Phase 56
probe.

### Lua libraries

All three ports are pure Lua. None is in this repo yet. They were
written to run inside a host that has no native modules and no `io`.

| Library | Where it lives today | What Reports.Render calls |
| --- | --- | --- |
| lua-pdfkit | `/home/asimard/DA/triton4/triton-toolkit-5b12870/tasks/lua-pdfkit/solution/lua-pdfkit/` (pure Lua; its own repo planned) | `PDFDocument.new`, then `doc:finish()`, then `doc:output()` for the bytes. Port of PDFKit 0.19.1. JPEG and PNG from raw bytes or a `data:` URI. |
| lua-zint | `/home/asimard/DA/triton3/triton-toolkit-e8fb24a/tasks/lua-zint/solution/` (pure Lua; its own repo planned) | `zint.barcode(opts)` returns an SVG string. Symbologies in the port: QR, Code 39, Code 128, UPC-A, GS1-128. |
| d3.lua | Poseidon job artifact `/home/asimard/DA/poseidon1/jobs/lua-run10/` (pure Lua; its own repo planned). Validated byte-identical to D3.js on the reference pair `d3_chart.js` / `lua_chart.lua` | `d3.create_root`, `d3.scaleBand`, `d3.scaleLinear`, `d3.axisLeft` / `d3.axisRight` / `d3.axisBottom`, `d3.stack`, `d3.curveBasis`, `d3.line`, `d3.range`. Entry point pinned at the Phase 56 probe. |

**Status:** None of the three libraries is in this repository. They are
pure-Lua ports Andrew owns, each moving to its own repository: the zint
and pdfkit ports ship `tests/test.sh` plus `PORTING-NOTES.md` /
`AUTHOR-NOTES.md`, and the D3 port's validation is byte-equivalence
against D3.js itself. Each library gets its own seed phase and probe
(Phases 30, 53, and 56), and the packet records the repo commit or
version it was generated from. The path in the table is the fallback
when that repository does not exist yet. The seed phase stops and
asks when neither the repository nor the path is there. If a port
reaches LuaRocks, that helps
only if Hydrogen's sandbox can read `package.path`; the seeded
`group.script` module is the path this plan builds on, and the probe
confirms which one is live.

All three are producers. HTML, preview, and SVG use the SVG those
ports return. PNG is Lithium rasterizing that SVG. PDF embeds the
ports' native output at Phase 71. Until then, charts and barcodes
reach the PDF only when Lithium passes rasters in `params.rasters`,
the stopgap named in Phases 55 and 65. Photographs and imported art
stay on that raster path.

The sandbox nils `io` unless `Sandbox.AllowIo` is set. Emit does not
turn that on. lua-pdfkit writes the document with `output()` and never
needs a file. Passing `opts.output` to zint would call `io.open`, so
Reports.Render leaves that field unset and uses the returned string.
Image and font paths inside lua-pdfkit do call `io.open`. The report
path passes image bytes, and the first PDF faces are the standard
ones carried as AFM data. A phase that needs `io` stops and asks.

- `H.set_result_json` is capped by `ClientInvokeMaxResultBytes`
  (default 1 MiB). A text report fits. A PDF is base64 inside that JSON,
  so the ceiling is about three quarters of a megabyte of PDF. Fixtures
  in this plan stay under the cap. That ceiling is the compile-time
  `SCOREBOARD_RESULT_JSON_MAX`. The config key can only shrink it.

**Note:** `ClientInvokeMaxParamsBytes` (default 256 KiB, confirmed in
`config_scripting.c` and `conduit/script/script.c`) is a separate cap
on the conduit **script params** JSON, not on HTTP response bodies. The 256 KiB
figure in this plan refers to the params cap, which is the binding constraint
for a definition + datasource passed inline to `Reports.Render`.

**Raising the params cap to 10 MB is config-only.** Three things were
verified against the source:

- The enforcement point compares the serialized params length against
  `Scripting.ClientInvokeMaxParamsBytes` (`conduit/script/script.c`, both
  the raw and merged-params checks). There is no fixed buffer in that
  path, and the config field is an `int`, so 10 MiB fits.
- The API POST body ceiling is already **10 MiB**: `API_MAX_POST_SIZE`
  is `10240 * 1024` (`src/api/api_utils.h`). The HTTP layer never capped
  conduit script at 256 KiB — the scripting config is the only gate.
  (The header comment saying "64KB default" is stale and wrong.)
- The scoreboard strdups `params_json` per job and holds it until the
  entry is pruned, so the cap is also a memory dial: 10 MB per in-flight
  job. Fine for a few authenticated users on an appliance.

The recommendation is `Scripting.ClientInvokeMaxParamsBytes: 10485760`
in hydrogen.json, which matches the body ceiling exactly and leaves
embedded-base64 definitions workable inline. It is operator config and
needs no new C. Raising it does **not** lift the result ceiling.
`ClientInvokeMaxResultBytes` can only shrink `SCOREBOARD_RESULT_JSON_MAX`. A datasource or a rendered output past
either cap is still deferred to "Later, not scheduled" for the
stored-blob path.

### Images

Hydrogen's Reporting subsystem is the image scaler. It is disabled
until the operator sets `Reporting.Enabled`. Implementation plan:
[`/docs/H/plans/complete/IMAGE_PLAN_COMPLETE.md`](/docs/H/plans/complete/IMAGE_PLAN_COMPLETE.md).
Endpoint:
[`/docs/H/api/reporting/reporting_endpoints.md`](/docs/H/api/reporting/reporting_endpoints.md).

`POST /api/reporting/image_scale` takes a base64 image plus width,
height, `units` (`px` or `pt`), `dpi`, and a format. ImageMagick
(MagickWand) decodes and scales. Format `xo` is a FlateDecode PDF
XObject payload (width, height, color space, optional alpha) built so
a PDF writer can insert the streams. lua-pdfkit accepts JPEG and PNG. Photographs are those bytes.
Phase 52 embeds each unique image once, through `doc:image`, and
every later placement references that object. The script does not
call this endpoint, and it does not grow an XO embedder. Lithium
calls `image_scale` for SVG rasterization (Phase 67, and charts and
barcodes until Phase 71). A PNG that comes back from that call
follows the same rule: one object, then references.

**SVG input and the raster paths.** `image_scale` documents SVG as a
supported input, with `dpi` applied before the read so rasterization
honors it, and `png` as a supported output. That one fact carries the
whole PNG emit (Phase 67), and it is also where a user's own imported
art goes. A user-supplied SVG placed as a static image is rasterized
this way; it does not enter the PDF as a vector until a later plan
extends the ports' native output to imported art.

**Image element options.** Photographs are raster. The item stores
`dpi` (default 72) and `format` (`png`, or `jpeg` when the stored
bytes are JPEG). Embedding a file or a paste opens a designer panel,
not a browser confirm. The panel computes a suggested pixel size
from the physical box and that dpi, rounding half away from zero,
and shows the width and the height. The author can change either
number. Each side is at least 1 pixel. A side that rounds to zero,
or that the author sets below 1, is kept at 1. The panel states
that confirming stores those pixels and does not keep the original
file. Confirm calls `image_scale` unless
the chosen size is the source's own pixel size, in which case the
original bytes are stored. Changing the box or the dpi later does
not resample. Opening the panel again does. The script embeds the
stored bytes and does not call `image_scale`. An image that repeats
is one PDF image object when every placement is handed the same
stored bytes (`doc._imageRegistry`). Different stored bytes are two
objects.

**Where the bytes come from.** All three source kinds are v1, and they
are all resolved to bytes at the start of generation, before any item
is placed:

| kind | Stored on the item | Resolved how | Trade-off |
| --- | --- | --- | --- |
| `embedded` | Base64 data URI in the definition, deduplicated in a content-hash-keyed registry | Already in hand | Portable, one file; makes the definition big |
| `reference` | A document id | `H.altquery_sync` on the session database reads `documents` (`acuranzo_1010.lua`: `(doc_id, rev_id)`, `file_data TEXT_BIG`, `file_name`) | Swapping the document changes the report without editing it; the report is no longer portable alone |
| `url` | A URL | `H.http.request_sync` at the start of generation, the same call the endpoint step uses | Live and small; the URL must stay available, and it slows every generation |

The designer's image browser loads a file from the local filesystem
or takes a paste from the clipboard. Both go through the confirm
panel above, and confirming stores them as `embedded`. A
`reference` picker lists documents through the Phase 17 read QueryRef.
A failed reference or an unreachable URL is a warning and an empty
image box.

Which kind an author picks is a size decision. Embedded base64 is the
reason a definition can outgrow the 256 KiB params cap; reference and
url kinds keep the definition small. Phase 27 previews an over-cap
definition by saving it first and rendering by `report_id`. The
params cap decides the path. It does not drop the preview quietly.

The script does not call `image_scale`. `params._hydrogen` carries
claims and does not carry the bearer token, and `H.system_token`
does not exist. Lithium holds the session JWT and is the only caller
of that endpoint. Reference bytes come from `H.altquery_sync` on
`params._hydrogen.database` and the Phase 17 document QueryRef, the
same database rule as a query datasource. URL bytes come from
`H.http.request_sync` against that URL, with no Hydrogen JWT. The
worker fetches whatever absolute `http://` or `https://` URL the
item stores. No new C wrapper is added.

The designer and the PDF emitter use `pt` (1/72 inch), matching
`DefaultDPI` 72. The on-screen designer stays at 96 CSS pixels per
inch for the unzoomed view. Those are different conversions of the
same physical unit.

`POST /api/reporting/image_scale` returns:
`{ "success": true, "image": "<base64>", "format": "png",
"mime_type": "image/png", "width": N, "height": N,
"input_format": "png", "input_dimensions": { "width": N, "height": N } }`.
Errors return HTTP 400/401/503 with `{ "success": false, "error": "...",
"details": "..." }`. On any non-200, the Lithium caller toasts and
omits that raster. A photograph that fails to resolve is a script
warning and an empty box (Phase 51).

### Call shape

```json
{
  "script": "Reports.Render",
  "params": {
    "stage": "html",
    "definition": { "format": 1 },
    "parameters": { "from": "2026-01-01" },
    "sources": { "orders": [{ "customer": "North" }] }
  },
  "wait": true,
  "timeout_seconds": 15
}
```

A stored report may send `report_id` and `rev_id` instead of
`definition`. The script loads the row through the Phase 17 QueryRef.
`stage` is optional and defaults to `pdf`. `params.now`, when it is
a unix number, is the generation instant from Value format. A
fixture may send it. A preview omits it. Client params stay under
`ClientInvokeMaxParamsBytes` (256 KiB unless the operator changed it).
A fixture in this plan fits. A larger datasource is a later phase that
stores the JSON and passes an id.

`params._hydrogen` is reserved. The client must not send it.

### `reports` columns

| Column | v1 use |
| --- | --- |
| `report_id`, `rev_id` | Identity. A save appends a revision. |
| `name` | List label. |
| `summary` | Short description. |
| `report` | Definition JSON text. |
| `parameters` | JSON object. Declarations live in the definition. v1 stores `{}`. |
| `scripts` | Unused in v1. Store `[]`. Author Lua lives in `report`. |
| `design` | Unused. Leave null. The old column name is ambiguous and this pass found no writer. |
| `thumbnail` | Unused in v1. |
| `collection` | Standard audit JSON. The designer does not read it. |

### Defaults

Compose and the designer share this table. A phase that adds a
property adds its row in that same sitting. Chart options other than
`dpi` get their rows in Phase 57. An embedded image stores the bytes
the author confirmed in Phase 50. There is no separate
downsample-cap property. Reuse is not a property. Same stored bytes
are one object.

The property panel shows the effective value. When the file omits the
key, the control still shows the default and notes that the file omits
it. The note is a label. Import accepts the omission. Compose uses the
value the panel showed. The panel does not reject a file for leaving
a key out.

Creating a report, a band, or an item writes the stored defaults into
the definition, so a person editing the JSON can see the value. A key
whose default is "omit" stays off the object. Import and export leave
an omitted key omitted until the operator edits that control. Editing
writes the key. Minimum band height stays `0.15 in`. That is a clamp,
not a default.

| Object | Property | On create | When the key is absent |
| --- | --- | --- | --- |
| Report | `name`, `title` | `Report` | `Report` |
| Report | `page.sizeKey` | `0` | Lookup 057 key 0, US Letter portrait, `8.5 in` by `11 in` |
| Report | `page.orientation` | `portrait` | `portrait` |
| Report | `page.units` | `in` | `in` |
| Report | each margin | `0.25 in` | `0.25 in` |
| Report | `grid.visible` | `true` | `true` |
| Report | `grid.style` | `dots` | `dots` |
| Report | `grid.spacing` | `0.125 in` | `0.125 in` |
| Report | `grid.weight` | `0.5` | `0.5` |
| Report | `grid.color` | `silver` | `silver` |
| Report | `grid.background` | `white` | `white` |
| Report | `font` | Vanadium Sans, `10 pt`, `Normal` | the same |
| Report | `datasources`, `functions` | `[]` | `[]` |
| Function | `returns` | `text`, or the type of the item or break that added it | `text`. The panel says so |
| Function | `source` | `return function(ctx)\n  return ""\nend` for text. Number returns `0`. Datetime returns nil. Boolean returns `false`. `rows` returns `{}` | empty source, which checks clean |
| Report | bands | page header, detail, page footer | a missing band is not invented |
| Band | height for report header, page header, page footer, report footer | `0.4 in` | `0.4 in` |
| Band | height for group header, detail, group footer | `0.25 in` | `0.25 in` |
| Item | `align` | `Left` | `Left` |
| Item | `font` | omitted | the report font. The panel says so |
| Item | `box` | `1.5 in` by `0.2 in`, centered in the band | that box |
| Item | static `text` | `Text` | `Text` |
| Item | `mask` on number, datetime, calculated number, calculated datetime, aggregate number, aggregate datetime, parameter | `""` | the omitted-mask rule in Value format. A datetime uses the datetime rule. The panel says which rule |
| Item | `mask` on text and aggregate text | omitted | the string itself. The panel shows no mask |
| Image | `dpi` | `72` | `72` |
| Image | `format` | `png` | `png` |
| Chart | `dpi` | `150` | `150` |
| Barcode | raster `dpi` | `300` | `300` |
| Call | `stage` | not stored on the definition | `pdf` |
| Call | `now` | not stored on the definition | `H.system.now()` when the call omits it |

### Designer rules

- No new npm package for v1, except `@codemirror/lint` in Phase 35.
  Pointer events do the dragging and the resize. Zoom is a CSS
  variable on the canvas.
- Palette and property editors read lookups 053, 054, and 057.
  Captions and icons come from those rows. The file stores keys.
  One palette lists the v1 type keys. Hidden types stay off it.
  Adding an item or a band is a palette choice. Phases 10, 16, 22,
  23, 28, 34, 38, 50, 54, and 58 use that palette.
- The property panel shows the Defaults table, including a key the
  file omits. The canvas uses the faces Lithium already loads.
  Vanadium Sans is the SPA face in
  [`/elements/003-lithium/src/styles/base.css`](/elements/003-lithium/src/styles/base.css).
- Sample datasource JSON in the designer is session state. It is how
  field items show a column name, including after a QueryRef run.
  It is not written into the definition.
- Import and export of the definition file work before the database
  save exists. The file is the artifact Phase 4 already proves.
- Split files before 1000 lines. Expected split: `reports.js` (shell),
  `reports.html`, `reports.css`, `canvas.js`, `rulers.js`, `grid.js`,
  `properties.js`, `functions.js`, `chart-panel.js`, and
  `model/units.js`, `model/document.js`, `model/history.js`.
- **DOMPurify is in `package.json`** (v3.3.2) but unused as of the last
  review. Preview HTML is sanitized with DOMPurify before assignment to
  a sandboxed iframe `srcdoc` (Phase 25). Barcode SVG must survive that
  sanitizer (Phase 54). This is the one place HTML from a script crosses
  into the DOM. Even though the preview HTML is fully generated by
  `Reports.Render` and there is no external HTML surface, the sanitizer
  stays as defense-in-depth for the iframe.

### Formula editor

The function editor is a CodeMirror 6 `EditorView` from
[`/elements/003-lithium/src/core/codemirror.js`](/elements/003-lithium/src/core/codemirror.js),
built with `buildEditorExtensions` and `language: 'lua'`. The Lua
personality is the in-repo stream language, `lua()` in that file:
keywords, comments, long strings, numbers, and builtins. The
Scripting manager already edits scripts with it. Reports uses the
same personality. It does not use
[`/elements/003-lithium/src/init/codemirror-init.js`](/elements/003-lithium/src/init/codemirror-init.js),
and it does not add a second Lua mode.

The buffer also has line numbers, folding, search, bracket matching,
`--` comment continuation, and a `LithiumEditorFooter`. The face is
Vanadium Mono, same as the other editors.

Syntax diagnostics use `@codemirror/lint`. That package is already
in the lockfile transitively. Phase 35 adds it to the direct
dependencies so the editor owns the import. The messages come from
Lua 5.5 `load` via `stage=check`, not from a JavaScript parser and
not from luacheck. The editor debounces a check of the open
function. A compile error underlines the line `load` reports. Empty
source is clean. Typing is never blocked, and a bad chunk can still
be saved. Compose remains the gate that refuses to render it.

Report undo records the function source when the editor blurs, not
on each keystroke. CodeMirror keeps its own undo while the buffer
is focused.

An undefined name is a runtime event in Lua. The editor does not
flag `io`, `H`, or a misspelled field while the author types. Those
fail when the line runs, and Phase 37 turns that into a warning and
an empty value.

The Functions list and the item share one buffer. `functions.js`
lists every function on the report: its name, its `returns`, and
the items and bands that name it. Selecting a row opens the
CodeMirror buffer. A calculated item, a group key, a break, and a
chart scope of `custom` store the function name only. The property
picker lists functions whose `returns` matches that use, and Edit
focuses the same buffer. A helper that no item names still appears
in the list. Renaming a function updates those references. Adding
a function from the list writes `returns` `text` and the text chunk
from the Defaults table. Adding one from a calculated item, a
break, or a custom chart writes that use's type and chunk.

### v1 render behavior

Given a definition and a source array, compose. An array in
`sources` wins. A query datasource with no array waits for
Phase 29 to fill it, and until that phase it warns and
contributes no rows:

- emits a static report when `sources` is empty and the detail band
  has no datasource
- repeats the detail band once per row
- substitutes field items from the row, already passed through the
  value-format rules, with null as an empty string
- honors Align and the font keys, and clips text to the item box
- prints system user from `params._hydrogen.username` (Phase 22)
- prints system datetime from the one generation instant, shifted
  by `params._hydrogen.tzoffset` minutes, then the item mask
  (Phase 22)
- prints page number and page count as integers through the number
  mask (Phase 23)
- prints a parameter item from `params.parameters` or the definition
  default
- breaks pages on fixed heights, keeping a group header with its
  first detail row
- opens and closes one group level when a group band is present

The order is the five steps under Page rules. Phase 19 emits the
structural intermediate. Phase 20 applies the mask. Phase 21
paginates. Phase 24 emits HTML. Barcodes (Phase 54), charts
(Phases 59–63), user formulas (Phase 33), primitive aggregates
(Phase 38), and custom breaks (Phases 45–47) extend it.
Emit HTML reproduces the page list. The same intermediate fixture is
the oracle for both the Lua result and the preview.

## Phase 0 — Design lock

### Goal

Agree the locks above. No source edits.

### Entry gate

This file is the plan under discussion.

### Work items

- [x] 0.1 Confirm format version 1 stores lookup `key_idx` and unit
      strings, and that the retired label-keyed JSON is evidence, not
      the file we write.
- [x] 0.2 Confirm the `reports` column roles, including `design` and
      `thumbnail` left unused.
- [x] 0.3 Confirm the v1 type allowlist, and whether Page Number
      and Page Count are lookup 053 keys 61 and 62.
- [x] 0.4 Confirm Vanadium Sans (and any other face) is added to
      lookup 054's font list in Phase 12.
- [x] 0.5 Confirm `Reports.Render` stages `pdf` (default), `html`,
      `svg`, `csv`, `compose`, and `check`. PNG is the manager
      posting the SVG to `image_scale`. Confirm that
      lua-pdfkit, lua-zint, and d3.lua are seeded as `require`
      libraries (`lua-pdfkit.*` with group `lua-pdfkit`, `zint.*`
      with group `zint`, and `require("d3.lua")` with group `d3`
      and script name `lua`), `invokable = 0`, one seed phase and
      probe per library.
- [x] 0.6 Confirm PDF as the default stage. The build order is the
      phase index: compose and HTML, then PDF, then the later
      exports. Fixed band heights. Lua does not measure glyphs.
      CSV is flat data and stands in for a spreadsheet. HTML is
      both an export and the preview medium. SVG is a document
      export. PNG is Lithium rasterizing that SVG, and a
      label-sized report is one file.
- [x] 0.7 Confirm no new Hydrogen C. Lithium calls
      `POST /api/reporting/image_scale`. The script does not. PNG
      uses that endpoint's SVG input. Charts and barcodes reach
      the PDF as native vectors in Phases 68–71. Until Phase 71,
      Lithium may pass rasters in `params.rasters`. The one new
      direct npm dependency is `@codemirror/lint`.
- [x] 0.8 Confirm this plan waits behind CATCHUP until Andrew pulls
      it in, and that Phase 25 may add `invokeScript` if CATCHUP
      Phases 7–10 have not.
- [x] 0.9 Confirm manager 24, append-only revisions, sandboxed
      preview, the 256 KiB param cap, and the 1 MiB result cap.
      The result cap is compile-time. Config can only shrink it.
      A PDF that fits is base64 inside the JSON. A PDF that does
      not fit returns a small error table. `H.set_result` with a
      location is later work and needs a store.
- [x] 0.10 Confirm author Lua lives in `definition.functions` and
      runs in the environment in User Lua. A detail formula sees
      the current record. Headers, footers, and breaks see the band
      context. Confirm the primitive ops, with `avg` and `mean` as
      the same arithmetic mean, and a line-break predicate that
      inserts one blank detail-height. The formula editor is
      CodeMirror 6 with the in-repo Lua personality, and syntax
      diagnostics come from `stage=check`.
- [x] 0.11 Confirm the chart locks: series-first roles with the combo
      as a first-class case, all four scopes in v1, a bespoke chart
      panel rather than lookup 054 rows, and Phase 57 as the chart
      design lock.
- [x] 0.12 Confirm the library homes. Each port lives in its own
      repository with its tests and porting notes, the seed packet
      records the commit or version, and LuaRocks is a possibility
      only if the sandbox can read `package.path`.
- [x] 0.13 Confirm image elements carry `dpi` and `format`. The
      author confirms the stored pixel size at embed time. The
      original file is not kept. A repeated image stores once per
      distinct byte string.
- [x] 0.14 Confirm the phase order: paging, then the system and
       page items, then HTML and PDF, then formulas, totals, and
       CSV, then images, barcodes, and charts, then the native PDF
       objects.
- [x] 0.15 Confirm the image source model: embedded, reference, and
       url are all v1, resolved at the start of generation, with the
       definition's base64 registry deduplicated by content hash.
- [x] 0.16 The effort column matches the phase index. The native PDF
       work is Phases 68, 69, 70, and 71, one sitting each.
- [x] 0.17 The 2026-10-10 amendments are applied to this file and
      match the approval.

### Done means

The locks in this file are the ones Andrew approved.

### Exit gate

Andrew's explicit approval of Phase 0. No source edited.

### Status

**complete — approved 2026-10-10.** Andrew approved the locks in this
file. No source was edited. This close-out records that approval. It
does not reopen it.

### Accomplished

The locks above are the approved contract.

- One lookup family per migration. Object types are Phase 11, fonts
  are Phase 12, and page sizes are Phase 13.
- Page Number is lookup 053 key 61. Page Count is key 62.
  Parameter is key 63.
- A value has a format mask. Null is blank. Align and font are
  honored. Text clips to its box. A group header stays with its
  first detail row. Page bands see the open group and the rows on
  the page. A datasource may name a QueryRef and the report's
  parameters.
- `align` is `Left`, `Center`, or `Right`. Font style is `Normal`,
  `Bold`, `Italic`, or `Bold + Italic`. Lookup 054 keys 9–12 stay
  off the v1 attribute lists. Compose resolves rows, formats text,
  paginates, then fills page fields. A query datasource reads
  `queries.code` by `query_ref` and runs `H.altquery_sync` on
  `params._hydrogen.database`. The mask table in Value format is
  the oracle.
- Script stages are `pdf` (default), `html`, `svg`, `csv`, `compose`,
  and `check`. PNG is a manager step. Native PDF objects are
  Phases 68–71.
- The Defaults table is the omitted-key contract. The panel shows
  the effective value. New writes the stored defaults into the file.
- Compose uses one generation instant. A fixture sends `params.now`.
  A preview omits it and compose uses `H.system.now()`.
- A file load and a paste both go through the embed panel. Each
  stored side is at least 1 pixel. Photographs are JPEG or PNG.
  `image_scale` can return an XO payload, and this plan does not
  use it.

### Lessons learned

- The retired designer is real: page, bands, SVG dot grid, rulers
  with a mouse line, item drag, property grid, JSON undo. Persistence
  to `reports` was not found. Pascal is not on disk.
- Lookup 053/054/057 are rich and uneven. Attribute lists on most
  types omit the field and the font. Page sizes stop at two rows and
  the second is mislabeled.
- PDF and barcodes are pure Lua ports Andrew already has
  (lua-pdfkit, lua-zint). Hydrogen require is `group.script` with
  the split on the first dot. Image sizing is the existing
  ImageMagick endpoint. That endpoint can return an XO payload.
  This plan does not use it. Photographs are JPEG or PNG.
  The script result cap is 1 MiB.
- Author formulas are source in the definition, compiled with
  `load` into a small environment. `require` remains the loader for
  lua-pdfkit and lua-zint. Built-in aggregates are engine ops.
  `avg` and `mean` are the arithmetic mean.
- A close-out that names only Status and the Working Log drops what
  the next session needs. Accomplished, Lessons learned, and Handoff
  are written before the phase is marked complete. The repo copy is
  [`/docs/PLANNING.md`](/docs/PLANNING.md).

### Handoff

Phase 1 is next, and only when Andrew pulls reporting in. CATCHUP
stays P0 until then. Re-check the next free Acuranzo file, QueryRef,
and lookup id on disk before numbering a packet. Hand the packet
over. Do not apply it. Phase 68 still asks before a native PDF
payload field is pinned. A fixture that needs a fixed clock sends
`params.now`. A preview omits it.

---

## Phase 1 — Contracts and fixtures

### Goal

Write the format version 1 contracts down as documents and checked-in
JSON fixtures, so later phases test against files.

### Entry gate

Phase 0 Status complete.

### Work items

- [ ] 1.1 Add a contract note under `docs/Li/` that restates the
      approved definition, intermediate, HTML, and call shapes,
      and the Defaults table. Absolute links. No line-number
      references.
- [ ] 1.2 Add `elements/003-lithium/tests/fixtures/reporting/` with
      one static definition, one detail definition, one grouped
      definition, and matching source JSON. Each case has an expected
      structural intermediate: plain text, one page. The detail
      definition includes `1234.5` with mask `#,##0.00` and a null.
      A second expected file for that detail case is the formatted
      intermediate, `1,234.50` and a blank null, matching the
      value-format table. HTML, page-split, PDF, and chart fixtures
      are written by the phase that owns that gate.
- [ ] 1.3 Point this plan's Phases 2, 19, and 20 at those paths.
       Chart fixtures land in Phase 57.

### Done means

The three fixtures exist and the contract note matches Phase 0.

### Exit gate

Andrew has read the contract note. `npm test` still passes (no new
test required in this phase).

### Status

**not started**

---

## Phase 2 — Units and geometry

### Goal

Pure functions for length strings and the content box. No DOM.

### Entry gate

Phase 1 Status complete.

### Work items

- [ ] 2.1 `model/units.js`: parse and format `in`, `mm`, `cm`, `pt`,
      `px`. Convert to CSS pixels at 96 per inch. Reject a bare
      number and an unknown unit with a thrown error the caller can
      show.
- [ ] 2.2 Content-box helper: page size minus margins, in the
      document unit and in CSS pixels.
- [ ] 2.3 Vitest covers the Phase 1 page sizes, a bad string, and a
      zero margin.

### Done means

`npm test` and `npm run lint` pass, and the new tests fail if 1 inch
is not 96 CSS pixels.

### Exit gate

Those two commands, run from `elements/003-lithium`, exit 0.

### Status

**not started**

---

## Phase 3 — Document model and undo

### Goal

Pure create, import, export, and edit of a definition, with undo.

### Entry gate

Phase 2 Status complete.

### Work items

- [ ] 3.1 `model/document.js` loads a fixture, rejects `format` other
      than 1, and exports canonical JSON (stable key order is not
      required; semantic equality is).
- [ ] 3.2 Add, move, resize, and delete a band and an item. Ids are
      stable across undo.
- [ ] 3.3 `model/history.js` snapshots the document. Undo and redo
      restore it. A new edit drops the redo tail.
- [ ] 3.4 Vitest uses the Phase 1 fixtures.
- [ ] 3.5 `createBlank()` returns the new-report document from the
      Defaults table: Letter portrait, the grid and font rows, and
      page header, detail, and page footer at the table heights.
      Vitest checks those written keys. Detail has no datasource.

### Done means

A test imports the detail fixture, moves a field, undoes the move,
and the export matches the fixture again.

### Exit gate

`npm test` and `npm run lint` exit 0.

### Status

**not started**

---

## Phase 4 — Manager shell, import, export

### Goal

Replace the placeholder with a manager that starts a blank report,
loads a fixture, and downloads the definition file.

### Entry gate

Phase 3 Status complete.

### Work items

- [ ] 4.1 `reports.html` / `reports.css` / `reports.js`. Fetch the
      HTML template. `templates:copy`. Stay under 1000 lines.
- [ ] 4.2 `init`, `destroy`, and `cleanup`. `destroy` releases
      listeners itself, because `closeManager` calls `destroy` only
      until CATCHUP's lifecycle fix. `cleanup` releases the same
      set. Drop the "under development" copy.
- [ ] 4.3 Import picks a local JSON file, runs it through the
      document model, and shows the report name. A bad file toasts
      the model error and leaves the previous document in place.
- [ ] 4.4 Export downloads the current definition as
      `<name>.report.json`.
- [ ] 4.5 Correct the Reports id sentence in
      [`/docs/Li/LITHIUM-MGR-MAIN.md`](/docs/Li/LITHIUM-MGR-MAIN.md)
      from 12 to 24.
- [ ] 4.6 Browser: import the detail fixture, export it, import the
      export, and confirm the name survived. Confirm a bad file
      toasts and the name stays.
- [ ] 4.7 New calls `createBlank()`. Export of that document contains
      the written defaults, including `page.sizeKey` 0,
      `font.style` `Normal`, `grid.style` `dots`, and the three
      bands. An imported file that omits a key still omits it on
      export.

### Done means

Manager 24 round-trips the detail fixture through a file, and New
exports the written defaults. Lint and unit tests pass.

### Exit gate

`npm test`, `npm run lint`, `npm run lint:css`, `templates:copy`, and
the browser check in 4.6.

### Status

**not started**

---

## Phase 5 — Page surface

### Goal

Show one page at the definition's size, with margins.

### Entry gate

Phase 4 Status complete.

### Work items

- [ ] 5.1 Draw the page from the document model. Unzoomed scale is
      96 CSS pixels per inch. Margins are guides.
- [ ] 5.2 Changing the page in the model (test hook or a temporary
      control that Phase 14 will replace) redraws the page.
- [ ] 5.3 Browser: Letter portrait is larger than a 4 in custom page,
      and the margin guides sit inset by 0.25 in on the fixture.

### Done means

The open fixture is visible as a page, and the unit tests still pass.

### Exit gate

Phase 4 commands plus the browser check.

### Status

**not started**

---

## Phase 6 — Bands and resize

### Goal

Render the v1 bands and let the operator change a band's height.

### Entry gate

Phase 5 Status complete.

### Work items

- [ ] 6.1 Paint each band as a labeled strip and a body of the stored
      height. Order is the array order.
- [ ] 6.2 Drag the bottom edge. Write the new height back in the
      document unit. `element.style.height` is the drag preview;
      mouseup commits through the model and pushes history.
- [ ] 6.3 Minimum height 0.15 in. Undo restores the previous height.
- [ ] 6.4 Browser: resize Detail, export, and confirm the file's
      height changed. Undo restores the canvas and a second export.

### Done means

Band height round-trips through the definition file.

### Exit gate

Lint, unit tests, and the browser check.

### Status

**not started**

---

## Phase 7 — Section grid

### Goal

Recreate the dot, line, dotted, and dashed backgrounds.

### Entry gate

Phase 6 Status complete.

### Work items

- [ ] 7.1 `grid.js` draws an SVG pattern from the definition's grid
      object: dots, grid, dotted, dashed, and none.
- [ ] 7.2 A `ResizeObserver` redraws when the band size changes.
      Disconnect it in `cleanup` / `destroy`.
- [ ] 7.3 Spacing follows the unit string. The default is the
      Defaults table: `0.125 in`, style `dots`.
- [ ] 7.4 Browser: switch the fixture grid style across the five
      values and resize a band. The pattern stays aligned to the
      band origin.

### Done means

The five grid styles render, and cleanup removes the observer.

### Exit gate

Lint, unit tests for the pattern geometry that can run without a
browser, and the browser check.

### Status

**not started**

---

## Phase 8 — Rulers and the mouse line

### Goal

Top and left rulers in the document unit, with a hairline that
follows the pointer.

### Entry gate

Phase 7 Status complete.

### Work items

- [ ] 8.1 `rulers.js` builds an SVG ruler for `in` and `cm`. Major
      ticks labeled, minor ticks at eighths of an inch or millimetres
      of a centimetre. Match the retired ruler's density closely
      enough that a 1 in mark lands on 96 CSS pixels when zoom is 1.
- [ ] 8.2 `mousemove` moves a horizontal and a vertical cursor line.
      `mouseleave` hides them. Listeners disconnect on cleanup.
- [ ] 8.3 The ruler is chrome. Export does not gain a Ruler band.
- [ ] 8.4 Browser: the 1 inch tick aligns with a band edge placed at
      `1 in`. The hairline tracks and disappears outside the page.

### Done means

Rulers track the pointer and agree with `model/units.js`.

### Exit gate

Lint, unit tests, and the browser check.

### Status

**not started**

---

## Phase 9 — View scale

### Goal

Fit the page to the canvas width, and zoom around that fit.

### Entry gate

Phase 8 Status complete.

### Work items

- [ ] 9.1 A CSS variable scales the page, the grid, and the rulers
      together. The definition's physical units do not change.
- [ ] 9.2 Fit-width, 100%, zoom in, and zoom out. 100% is 96 CSS
      pixels per inch.
- [ ] 9.3 Browser: fit-width keeps the whole Letter page visible in
      the designer pane. 100% makes the ruler's 1 inch mark match a
      CSS measurement of 96 pixels.

### Done means

Zoom changes pixels on screen and leaves the exported JSON alone.

### Exit gate

Lint, a unit test that fit-width math returns the expected scale for
a known pane width, and the browser check.

### Status

**not started**

---

## Phase 10 — Static text

### Goal

Place, move, resize, and select a static text item.

### Entry gate

Phase 9 Status complete.

### Work items

- [ ] 10.1 The palette adds a static text item to the selected band.
       The new item writes the Defaults table: box `1.5 in` by
       `0.2 in`, centered in the band, `align` `Left`, `text`
       `Text`. It omits `font` and `mask`.
- [ ] 10.2 Drag moves `left` and `top`. Corner resize changes `width`
       and `height`. Commit on pointerup through the model. Clamp to
       the band.
- [ ] 10.3 Selection draws an inset outline. Delete removes the item.
       Undo restores position and existence.
- [ ] 10.4 The item shows its text. v1 text is the item's `text`
       property, edited in Phase 14. Until then a new item reads
       "Text".
- [ ] 10.5 Browser: add, drag, resize, undo, export. The file has
       `typeKey` 0 and unit strings.
- [ ] 10.6 The canvas clips item text to the item box. A string
       longer than the box is cut off on screen. Emit clips the same
       way. The band does not grow.

### Done means

A static text item survives export, import, and undo.

### Exit gate

Lint, unit tests for the clamp, and the browser check.

### Status

**not started**

---

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

Phase 0 column lock still matches this file. Re-check numbers the
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

---

## Phase 19 — Compose skeleton

### Goal

Seed `Reports.Render` and return the structural intermediate for a
static report, a detail list, and one group level.

### Entry gate

Phase 1 fixtures exist. Phase 17 is applied if the script loads a
stored id. The fixture call, which passes `definition` inline, can be
written before Phase 17. Re-check the next file number.

### Work items

- [ ] 19.1 Copy the `scripts` insert shape from
       [`acuranzo_1376.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1376.lua).
       Group `Reports`, name `Render`, `invokable = 1`,
       `mcp_access = 0`.
- [ ] 19.2 `stage=compose` emits the structural intermediate. A static
       report renders when `sources` is empty and the detail band has
       no datasource. The detail band repeats once per row. Field text
       is the plain value, so `1234.5` prints `1234.5` even when a
       mask is stored. One group level opens and closes, on one page.
       The result includes `log`. A query datasource with no
       array adds a warn entry and contributes no rows. The fixture calls
       pass `stage=compose`. Phase 31 switches the default stage to
       `pdf`.
- [ ] 19.3 A missing datasource, an unknown `typeKey`, and an empty
       `field` become warn entries on `log`. The script still returns a document.
       A `format` other than 1 returns an error through
       `H.set_result_json` and does not throw out of the worker.
- [ ] 19.4 Hand the packet over. After apply, the three structural
       fixture calls match. The formatted `1,234.50` file is Phase 20.

### Done means

Andrew reports the three structural fixture calls matched, and the
script is `Reports.Render`.

### Exit gate

Andrew's apply report and those three calls. Test 98 on the new
migration if he ran it.

### Status

**not started**

---

## Phase 20 — Value format

### Goal

Compose applies the mask table. Null prints blank.

### Entry gate

Phase 19 Status complete.

### Work items

- [ ] 20.1 Apply the Value format table. A number rounds half away
       from zero. An omitted number mask prints a plain decimal. A
       mask that does not parse warns once, and the raw value is
       printed. Dates follow the same table.
- [ ] 20.2 The formatted detail fixture prints `1,234.50` and a blank
       null. A second fixture walks the rest of the mask table.
- [ ] 20.3 Hand the amendment over. Do not edit the applied Phase 19
       file. After apply, both fixture calls match.

### Done means

The formatted detail fixture and the mask-table fixture both match.

### Exit gate

Andrew's apply report and the two fixture calls.

### Status

**not started**

---

## Phase 21 — Paging

### Goal

Fixed-height paging keeps a group header with its first detail row.

### Entry gate

Phase 20 Status complete. Masks do not change band height.

### Work items

- [ ] 21.1 Page breaks use the stored band heights against the content
       box. If the group header plus one detail row do not fit in the
       remaining box, both move to the next page. If they do not fit
       on an empty page either, compose warns and places them anyway.
       The header is not repeated on a later page. Its summary is
       every row of the group.
- [ ] 21.2 Add an overflow fixture beside the Phase 1 files. The
       header moves with its first detail row.
- [ ] 21.3 Hand the amendment over. After apply, the overflow fixture
       matches.

### Done means

The overflow fixture's page list keeps the group header with its
first detail row.

### Exit gate

Andrew's apply report and the fixture call.

### Status

**not started**

---

## Phase 22 — System datetime and system user

### Goal

The palette can place a system datetime and a system user, and compose prints them.

### Entry gate

Phase 21 Status complete.

### Work items

- [ ] 22.1 The palette adds lookup 053 key 5 (System datetime) and key 6 (System user). A new datetime item stores the omitted-mask default. A new user item stores no mask.
- [ ] 22.2 System user prints `params._hydrogen.username`. An empty username prints blank and adds a warn log entry.
- [ ] 22.3 System datetime uses the one generation instant from
       Value format. Two items on the same report share it. It adds
       `params._hydrogen.tzoffset` minutes, where a negative offset
       is west of UTC, and formats that civil time with the item
       mask. The mask grammar does not change. `tz` is the IANA name
       Hydrogen stored on the claim, and compose does not look it up.
       A missing `tzoffset` formats the instant in UTC and adds an
       info log entry. Field datetimes keep the Value format rules.
- [ ] 22.4 Fixture: username `ada` prints `ada`. An empty username
       prints blank and adds a warn entry. The call sends
       `params.now` `1772719629` and does not read the live clock.
       With `tzoffset` `-420` and mask `yyyy-mm-dd hh:nn` it prints
       `2026-03-05 07:07`. The same instant with no `tzoffset` prints
       `2026-03-05 14:07` and adds an info entry. Hand the amendment
       over.

### Done means

The fixture prints the username and the timezone-shifted system datetime.

### Exit gate

Andrew's apply report and the fixture call.

### Status

**not started**

---

## Phase 23 — Page number and page count

### Goal

Page number and page count print on the page that owns them.

### Entry gate

Phase 22 Status complete.

### Work items

- [ ] 23.1 The palette adds lookup 053 key 61 (Page number) and key 62 (Page count). Both use the number mask. An omitted mask prints a plain integer.
- [ ] 23.2 Compose writes the page's number and the report's page count into those items after the page list exists.
- [ ] 23.3 Fixture: a two-page report prints `1` and `2`, and page count `2` on both pages. Hand the amendment over.

### Done means

The two-page fixture shows the page number and the page count.

### Exit gate

Andrew's apply report and the fixture call.

### Status

**not started**

---

## Phase 24 — HTML emit

### Goal

`stage=html` emits one section per page of the composed document.

### Entry gate

Phase 23 Status complete.

### Work items

- [ ] 24.1 `stage=html` emits one section per page with
       `break-after: page`, the composed text, and positions in
       physical CSS units. Align, font family, size, and style come
       from the item, then the definition font. Text clips to the
       item box. A call that omits `stage` still runs `compose`
       until Phase 31.
- [ ] 24.2 Add the expected HTML skeleton for the detail fixture.
- [ ] 24.3 Hand the amendment over. After apply, the HTML fixture
       matches.

### Done means

The detail fixture's HTML matches the expected skeleton.

### Exit gate

Andrew's apply report and the fixture call.

### Status

**not started**

---

## Phase 25 — Preview

### Goal

The designer runs `Reports.Render` at `stage=html` and shows that
HTML in a sandboxed iframe.

### Entry gate

Phase 24 Status complete. The open definition's datasources are the
session sample from Phase 15, or an empty object for a static report.

### Work items

- [ ] 25.1 If `src/shared/conduit.js` still has no `invokeScript`,
       add it for `POST /api/conduit/script` (JWT-bearer, params shape
       per the call contract) and cover it with a unit test. If
       CATCHUP already added it, call that function.
- [ ] 25.2 Preview requests `stage=html` with the current definition
       and the session sample. DOMPurify runs on the HTML. The result
       is assigned to a sandboxed iframe `srcdoc`. The parent page
       does not assign report HTML to `innerHTML`.
- [ ] 25.3 A render button renders the whole report. Auto-render on
       change is decided by the browser trial in 23.5. If it wins, it
       debounces, and it never fires while the pointer is dragging
       or resizing.
- [ ] 25.4 A result that carries `error` toasts once and leaves
       the canvas as it was. `log` stays on the result. This phase
       does not toast each entry. The generation-log phase shows them.
- [ ] 25.5 Browser: preview the detail fixture with two sample rows
       and see two detail bands. Preview a report whose sample is
       missing and see one warn entry on `log`.

### Done means

The operator can preview the HTML the script produced for the detail
fixture.

### Exit gate

Lint, the conduit unit test, and the browser check against an applied
`Reports.Render`.

### Status

**not started**

---

## Phase 26 — Generation log

### Goal

A generation shows its info, warn, and error messages, and the open report shows the three counts.

### Entry gate

Phase 25 Status complete.

### Work items

- [ ] 26.1 The preview keeps the script's `log` array. The panel lists each entry's level and message. Lithium counts `info`, `warn`, and `error`. The report header shows those three counts for the last generation.
- [ ] 26.2 A non-zero error count flags the open report as an error. A warn count with zero errors flags it as a warning. Info only flags it as info. Zero entries show no flag. The counts are session state. They are not columns on `reports`.
- [ ] 26.3 Browser: the missing-sample preview shows one warn entry, a warn count of 1, and the warn flag. A clean detail preview shows an empty log and no flag.

### Done means

The operator can read the generation messages and see the three counts on the open report.

### Exit gate

Lint and the browser check.

### Status

**not started**

---

## Phase 27 — Over-cap preview

### Goal

A definition that does not fit in the params cap still previews, by
saving first and rendering by `report_id`.

### Entry gate

Phase 26 Status complete. Phase 18 is already applied.

### Work items

- [ ] 27.1 When the inline params would exceed
       `ClientInvokeMaxParamsBytes`, save the definition and preview
       by `report_id`. One toast names that path. Do not send the
       session JWT as a script param.
- [ ] 27.2 Browser: an over-cap definition previews after that toast,
       and the canvas is unchanged when the save fails.

### Done means

An over-cap definition previews by `report_id` after one toast.

### Exit gate

Lint and the browser check.

### Status

**not started**

---

## Phase 28 — Query designer

### Goal

The designer stores a QueryRef datasource and its parameters, runs
that query with the session, and previews the rows it returned.

### Entry gate

Phase 27 Status complete.

### Work items

- [ ] 28.1 The definition stores `parameters`: `name`, `type`
       (`text`, `number`, `date`, or `boolean`), and `default`. A
       query datasource stores `kind` `query`, `queryRef`, and
       `params` mapping each bind to a parameter or a literal.
       `kind` `json` stays the fixture and paste path. Sample rows
       stay session state and are not exported.
- [ ] 28.2 The designer edits that list and shows a run strip: one
       control per parameter, defaults filled in. Run calls each
       QueryRef through conduit `authQuery` with the session JWT
       and stores the rows as the session sample. A failed query
       toasts and leaves the previous sample.
- [ ] 28.3 A parameter item (lookup 053 key 63) prints one parameter
       with the same `mask` as a field. It may sit on any band.
       Phase 20 formats that mask.
- [ ] 28.4 Browser: a small QueryRef returns two rows and the preview
       shows two detail bands. The preview request includes those
       rows in `sources`.

### Done means

Andrew runs a QueryRef from the designer and the preview shows two
detail bands.

### Exit gate

Lint and the browser check.

### Status

**not started**

---

## Phase 29 — Query execution

### Goal

A render that does not include rows runs the same QueryRef inside
`Reports.Render`.

### Entry gate

Phase 28 Status complete. Phase 17 is applied when the call uses
`report_id`. Re-check that `H.altquery_sync` still accepts a database name,
SQL, and named binds.

### Work items

- [ ] 29.1 When `sources` omits a query datasource, `Reports.Render`
       loads `queries.code` with `query_ref` and
       `query_type_a28 = 1`, then runs
       `H.altquery_sync(params._hydrogen.database, code, binds)`
       with the resolved binds. `params.parameters` overrides
       defaults. When `sources` includes the array, the script does
       not query. When the database name is empty or is not a
       configured connection, the script adds a warn entry and
       returns no rows. The worker login is the one on that
       connection. Row-level permissions stay in Later.
- [ ] 29.2 Hand the amendment over. A `report_id` render with no
       `sources` returns the same rows the designer run returned.

### Done means

A `report_id` render with no `sources` returns the same two rows as
the designer run.

### Exit gate

Andrew's apply report and that call.

### Status

**not started**

---

## Phase 30 — Seed lua-pdfkit

### Goal

Put the pure-Lua PDFKit port into `scripts` so `require` loads it.
No report output yet.

### Entry gate

Phase 0 Status complete. Re-check the next Acuranzo file number the
day this is written. Phases 1–29 may still be open. Phases 31 and 52
wait for the apply.

### Work items

- [ ] 30.1 Copy the tree named in the library lock into one migration.
       One migration seeds every `lua-pdfkit.*` module (`init`,
       `document`, `page`, `path`, `image`, `font.afm`, …).
       `invokable = 0`, `mcp_access = 0`. Group name `lua-pdfkit`.
       The script name is the require path after the first dot. One
       library per file, the exception to one-script-per-migration.
       Reverse deletes only those rows. The packet records the commit.
       If neither the port repository nor the task-tree path is
       present, stop and ask.
- [ ] 30.2 Confirm a standard face (`Helvetica`) renders with `io`
       nil. If the port still `io.open`s an AFM file, bundle that AFM
       text into the seeded module before handing the packet over. A
       phase that needs `io` stops and asks.
- [ ] 30.3 Hand the packet over. After apply, Andrew runs a one-page
       probe: `require("lua-pdfkit.init")`, `doc:text`, `doc:finish()`,
       and `doc:output()`. The probe returns a PDF byte length.
       `AllowDBModuleLoad` is on for that call.

### Done means

Andrew reports the probe PDF length from inside Hydrogen, with `io`
left off.

### Exit gate

Andrew's apply report and the probe. Test 98 on the new migration
if he ran it.

### Status

**not started**

---

## Phase 31 — PDF text

### Goal

`stage=pdf` returns a text PDF of the composed pages, and `pdf`
becomes the default stage.

### Entry gate

Phase 30 and Phase 21 Status complete.

### Work items

- [ ] 31.1 Amend `Reports.Render` in a new migration. Do not edit an
       applied file. Re-read one script-update migration before
       writing.
- [ ] 31.2 `PDFDocument.new` uses the definition page size. One
       `addPage` per intermediate page. Text uses a standard face
       the port already carries. Positions are in PDF points. Align,
       font size, and font style come from the item, then the
       definition font. Bold and italic are the PDF flags. The face
       stays that standard face. Text clips to the item box.
- [ ] 31.3 A call that omits `stage` now runs `pdf`. The detail
       fixture returns `{ "pdf_base64": "...", "log": [] }` and
       opens with the right page count. The result-cap behavior is
       Phase 32.
- [ ] 31.4 Hand the packet over. Andrew's call opens as a PDF with
       the right page count.

### Done means

The detail fixture opens as a PDF with the right page count.

### Exit gate

Andrew's apply report and the fixture call.

### Status

**not started**

---

## Phase 32 — Result cap

### Goal

A PDF whose JSON does not fit the result cap returns
`result_too_large` instead of a rejected body.

### Entry gate

Phase 31 Status complete.

### Work items

- [ ] 32.1 The script measures the encoded JSON before
       `H.set_result_json`. When it does not fit, the script submits
       `{ "error": "result_too_large", "cap": N, "size": N }`.
       The ceiling is `SCOREBOARD_RESULT_JSON_MAX` (1 MiB).
       `ClientInvokeMaxResultBytes` can only lower it. Write that
       into the contract note.
- [ ] 32.2 One probe builds a result over the cap and receives that
       error object. The Phase 31 detail fixture still returns
       `pdf_base64`.
- [ ] 32.3 Hand the amendment over.

### Done means

The over-cap probe returns `result_too_large` with `cap` and `size`,
and the detail fixture still returns `pdf_base64`.

### Exit gate

Andrew's apply report and the two calls.

### Status

**not started**

---

## Phase 33 — Text formula

### Goal

`Reports.Render` compiles an author function and a calculated text
item prints what it returns.

### Entry gate

Phase 32 Status complete. lua-pdfkit, lua-zint, and d3.lua stay on
`require`. Author formulas do not.

### Work items

- [ ] 33.1 Implement the User Lua contract. Compile every function at
       the start of compose with `load(source, name, "t", env)`. A
       compile error returns through `H.set_result_json` and does not
       throw out of the worker. A runtime error warns and yields an
       empty value. An unknown function name is a compose error.
       This is the one compiler. Chart `custom` waits for Phase 64.
- [ ] 33.2 Fixture function `pack_code`, `returns` `text`. Letters
       `A` through `L` are months 1 through 12. The return value is
       that letter, the week digit, and the day as two digits. Row
       `month=3`, `week=2`, `day=5` renders `C205`. The fixture
       definition is JSON. The editor is Phase 34.
- [ ] 33.3 Hand the amendment over. After apply, the fixture renders
       `C205`.

### Done means

The pack-code fixture renders `C205`.

### Exit gate

Andrew's apply report and the fixture call.

### Status

**not started**

---

## Phase 34 — Formula editor

### Goal

The designer edits a function's source and `returns`, and the
definition file round-trips them.

### Entry gate

Phase 33 and Phase 14 Status complete.

### Work items

- [ ] 34.1 Palette shows lookup 053 keys 28, 29, and 32. The item
       stores the function name. The Functions list and that item
       share one CodeMirror 6 buffer, `language: 'lua'`, Vanadium
       Mono, and the shared footer. The list shows name, `returns`,
       and references. The item picker offers functions whose
       `returns` matches the item, and Edit focuses the same buffer.
       A new function writes the Defaults table's `returns` and
       `source`. Source is saved when the editor blurs. The browser
       does not run the Lua. Diagnostics are Phase 35.
- [ ] 34.2 Browser: edit the pack-code source, export, reload, and
       see the same source and `returns`. The document model stores
       the source and does not execute it.
- [ ] 34.3 Unit tests cover that storage if a pure helper exists.

### Done means

The designer round-trips function source and `returns` in the
definition file.

### Exit gate

Lint, the unit test, and the browser export/reload.

### Status

**not started**

---

## Phase 35 — Formula check

### Goal

`stage=check` underlines a Lua compile error in the formula editor.

### Entry gate

Phase 33 and Phase 34 Status complete.

### Work items

- [ ] 35.1 `stage=check` compiles the posted functions with the same
       `load` path as compose and returns `{ name, line, message }`
       for each failure. No rows are read.
- [ ] 35.2 Add `@codemirror/lint` to the direct dependencies. The
       editor debounces that call for the open function and underlines
       the line. Empty source is clean. Typing is never blocked, and
       a bad chunk can still be saved. Compose still refuses to render
       a chunk that does not compile. Undefined names such as `io` or
       `H` are not gutter diagnostics.
- [ ] 35.3 Browser: type `function(ctx` with no `end`, see the
       diagnostic, close the chunk, and see the diagnostic clear.

### Done means

An unclosed chunk shows a CodeMirror diagnostic, and closing the
chunk clears it.

### Exit gate

Lint and the browser check.

### Status

**not started**

---

## Phase 36 — Number and datetime formula

### Goal

A calculated number and a calculated datetime print through the item
mask.

### Entry gate

Phase 35 Status complete.

### Work items

- [ ] 36.1 A function with `returns` `number` returns the raw number.
       The item mask formats it. Fixture: return `1234.5` with mask
       `#,##0.00` and print `1,234.50`. A function with `returns`
       `datetime` returns a value the datetime mask accepts, and the
       item mask formats it. The calculated number and calculated
       datetime use the mask editor from work item 15.5.
- [ ] 36.2 Hand the amendment over. Browser: the masked number prints
       `1,234.50`.

### Done means

The number fixture prints `1,234.50`, and the datetime fixture prints
through its mask.

### Exit gate

Andrew's apply report, the fixture call, and the browser preview.

### Status

**not started**

---

## Phase 37 — Formula runtime errors

### Goal

One bad function call warns and prints empty, and the rest of the
report still renders.

### Entry gate

Phase 36 Status complete.

### Work items

- [ ] 37.1 A row whose function raises comes back empty with a
       warning. A number function that returns a string warns once
       per item and prints empty.
- [ ] 37.2 Hand the amendment over. The fixture matches, and the
       other rows of that report still render.

### Done means

The raising row and the type-mismatch row each warn and print empty,
and the other rows still render.

### Exit gate

Andrew's apply report and the fixture call.

### Status

**not started**

---

## Phase 38 — Count, unique, and sum

### Goal

Group and report headers and footers can show count, filled, unique,
and sum.

### Entry gate

Phase 37 Status complete.

### Work items

- [ ] 38.1 An aggregate item stores `op` and either `field` or
       `function`. This phase's ops are `count`, `filled`, `unique`,
       and `sum`. Numeric results use lookup 053 key 35. A text
       result uses key 34. The picker offers a function only when
       its `returns` allows the op. The fixture uses a field.
- [ ] 38.2 Compose reduces the raw values for the current group and
       for the whole detail source, and places them on
       `ctx.aggregates`. `count` counts every row. `filled`,
       `unique`, and `sum` skip nulls. An empty `count`, `filled`,
       or `unique` is 0. An empty `sum` is an empty string. A
       non-numeric value in `sum` warns and is skipped. The header
       and the footer of one scope show the same total. The item
       mask formats the reduction.
- [ ] 38.3 The designer places the item on a group header, a group
       footer, a report header, or a report footer.
- [ ] 38.4 Fixture: two groups. Hand the amendment over. The preview
       shows each op this phase names, and the header matches the
       footer.

### Done means

The two-group fixture shows count, filled, unique, and sum, and the
header matches the footer.

### Exit gate

Lint, the script packet apply, the fixture call, and the browser
preview.

### Status

**not started**

---

## Phase 39 — Page totals

### Goal

A page header or page footer shows the sum of the detail rows on
that page.

### Entry gate

Phase 38 and Phase 21 Status complete.

### Work items

- [ ] 39.1 A page-footer aggregate reduces the detail rows placed on
       that page. The same window serves later ops. The page header
       and the page footer show the same page total.
- [ ] 39.2 The designer can place the aggregate on a page header or
       a page footer.
- [ ] 39.3 Fixture: the page sum sits beside the report sum, and the
       two differ when the rows span two pages. Hand the amendment
       over.

### Done means

The fixture shows a page sum and a report sum, and they differ across
a two-page report.

### Exit gate

The script packet apply, the fixture call, and the browser preview.

### Status

**not started**

---

## Phase 40 — Average and weighted

### Goal

`avg`, `mean`, and `weighted` reduce a numeric field.

### Entry gate

Phase 39 Status complete.

### Work items

- [ ] 40.1 `avg` and `mean` are the same arithmetic mean. `weighted`
       stores `weight` as a field and computes field times weight,
       divided by the sum of the weights. A non-numeric value warns
       and is skipped. A zero sum of weights, or no numeric pair
       left, prints an empty string and adds one warn entry. An
       empty result is an empty string.
- [ ] 40.2 Add the three ops to the picker Phase 38 built. Fixture:
       `avg` and `mean` match, and `weighted` matches its inputs.
       Hand the amendment over. The preview shows the mean.

### Done means

The fixture shows `avg` equal to `mean`, and the weighted total
matches its inputs.

### Exit gate

The script packet apply, the fixture call, and the browser preview.

### Status

**not started**

---

## Phase 41 — Median

### Goal

`median` reduces a numeric field. An even count uses the mean of the
two central numbers.

### Entry gate

Phase 40 Status complete.

### Work items

- [ ] 41.1 Implement `median` on the raw values. An even count is the
       mean of the two central numbers. An empty result is an empty
       string.
- [ ] 41.2 Add `median` to the picker. One fixture has an odd count
       and one has an even count. Hand the amendment over. The
       preview shows the odd-count median.

### Done means

The odd-count median and the even-count median both match, and the
preview shows the odd-count median.

### Exit gate

Andrew's apply report, the fixture call, and the browser preview.

### Status

**not started**

---

## Phase 42 — Mode

### Goal

`mode` reduces a field. A tie picks the smallest value under Lua `<`
and warns.

### Entry gate

Phase 41 Status complete.

### Work items

- [ ] 42.1 Implement `mode`. A single mode returns that value. A tie
       returns the smallest value under Lua `<` and adds a warning.
       A text mode uses lookup 053 key 34.
- [ ] 42.2 Add `mode` to the picker. One fixture has a single mode
       and one has a tie. Hand the amendment over. The preview shows
       the single mode.

### Done means

The single-mode fixture matches, the tie fixture returns the
smallest value plus a warning, and the preview shows the single mode.

### Exit gate

Andrew's apply report, the fixture call, and the browser preview.

### Status

**not started**

---

## Phase 43 — Min and max

### Goal

`min` and `max` reduce a number or a datetime.

### Entry gate

Phase 42 Status complete.

### Work items

- [ ] 43.1 `min` and `max` compare numbers as numbers and datetimes
       as one kind per total. The first non-null value picks the
       kind. A later value of the other kind warns once and is
       skipped. Nulls are skipped and do not warn. An empty result
       is an empty string. A datetime result uses lookup 053 key 38.
       A number uses key 35.
- [ ] 43.2 Add the ops to the picker. The fixture records `min` and
       `max` for a number and for a datetime. Hand the amendment
       over. The preview shows both.

### Done means

The number fixture and the datetime fixture each show `min` and
`max`.

### Exit gate

The script packet apply, the fixture call, and the browser preview.

### Status

**not started**

---

## Phase 44 — First and last

### Goal

`first` and `last` follow row order and skip nulls.

### Entry gate

Phase 43 Status complete.

### Work items

- [ ] 44.1 `first` and `last` walk row order and skip nulls. `first`
       is not the minimum. A null in the middle is skipped. An empty
       result is an empty string. Text uses key 34. A number uses
       key 35. A datetime uses key 38.
- [ ] 44.2 Add the ops to the picker. The fixture shows `first` and
       `last` for a number and for a datetime, with a null in the
       middle. Hand the amendment over.

### Done means

The fixture shows `first` and `last` in row order, and the null in
the middle is skipped.

### Exit gate

The script packet apply, the fixture call, and the browser preview.

### Status

**not started**

---

## Phase 45 — Group-key function

### Goal

A group band can take its key from a text or number function.

### Entry gate

Phase 44 Status complete.

### Work items

- [ ] 45.1 A group band may set `groupKey` to a text or number
       function. A text key compares as a string. A number key
       compares as a number. `sort` defaults to true and uses Lua
       `<`. `sort` false breaks only when consecutive keys differ.
       One level. The one-field group remains the path with no Lua.
- [ ] 45.2 The group band's key picker lists text and number
       functions. Edit focuses the same Functions buffer.
- [ ] 45.3 Fixture: group by the pack-code month letter. Hand the
       amendment over. Browser: the preview shows that split.

### Done means

The fixture intermediate shows the Lua group split.

### Exit gate

Script packet apply, the fixture call, and the browser preview.

### Status

**not started**

---

## Phase 46 — Page-break predicate

### Goal

A boolean function can start a new page before a detail row.

### Entry gate

Phase 45 Status complete.

### Work items

- [ ] 46.1 A detail band may set `pageBreak` to a boolean function.
       A true result starts a new page before that row. Any other
       value warns once and counts as false. Fixed-height paging
       still applies.
- [ ] 46.2 The detail band's picker lists boolean functions.
- [ ] 46.3 Fixture: start a new page when `ctx.row.break_before` is
       true. Hand the amendment over. Browser: the preview shows the
       extra page.

### Done means

The fixture intermediate shows the forced page.

### Exit gate

Script packet apply, the fixture call, and the browser preview.

### Status

**not started**

---

## Phase 47 — Line-break predicate

### Goal

A boolean function can insert one blank detail-height before a row.

### Entry gate

Phase 46 Status complete.

### Work items

- [ ] 47.1 A detail band may set `lineBreak` to a boolean function.
       A true result inserts one blank detail-height before that
       row. Any other value warns once and counts as false. A
       newline inside a calculated value is kept by HTML
       (`white-space: pre-line`) and by the PDF text drawer. The
       stored band height does not change. Overflow clips.
- [ ] 47.2 The detail band's picker lists boolean functions.
- [ ] 47.3 Fixture: insert a blank line when `ctx.row.blank_before`
       is true. Hand the amendment over. Browser: the preview shows
       the blank line.

### Done means

The fixture intermediate shows the blank line.

### Exit gate

Script packet apply, the fixture call, and the browser preview.

### Status

**not started**

---

## Phase 48 — Header and footer context

### Goal

A calculated item on a header or footer sees that band's context.

### Entry gate

Phase 47 Status complete. Header and footer formulas can read
`ctx.aggregates`. This phase's fixture does not require an aggregate.

### Work items

- [ ] 48.1 Calculated items on a group, page, or report header or
       footer receive the context in the User Lua table. On a group
       band, `ctx.row` is the first row of the group and `ctx.rows`
       is the whole group. Every `ctx` includes `parameters`.
       `page.count` is filled after the page list exists, before the
       page formulas run.
- [ ] 48.2 Fixture: a group footer formula reads `ctx.group.key` and
       prints it. Hand the amendment over.

### Done means

The group-footer formula prints `ctx.group.key`.

### Exit gate

Script packet apply and the fixture call.

### Status

**not started**

---

## Phase 49 — CSV emit

### Goal

`stage=csv` returns one row per detail record, as the text compose
already stored.

### Entry gate

Phase 48 Status complete. The mask table is already applied.

### Work items

- [ ] 49.1 Amend `Reports.Render` with `stage=csv`. Columns are the
       field items on the detail band, in item order. Header and
       footer bands are not rows. Every value exports as the
       formatted text compose stored.
- [ ] 49.2 Grouped reports export flat rows. The group key is
       repeated as leading columns. Aggregates are not CSV rows.
       Write that shape into the contract note.
- [ ] 49.3 Values are escaped per RFC 4180. A newline inside a value
       is quoted, not split.
- [ ] 49.4 Add the expected CSV for the detail fixture. Hand the
       packet over. Andrew's call matches.

### Done means

The detail fixture renders to the expected CSV.

### Exit gate

Andrew's apply report and the fixture call.

### Status

**not started**

---

## Phase 50 — Embedded image

### Goal

An embedded image shows in the designer and in the HTML preview.

### Entry gate

Phase 49 Status complete.

### Work items

- [ ] 50.1 Palette shows lookup 053 keys 2 (Static Image) and 22
       (Field Image). A new item stores `dpi` `72`. `format` is
       `png`, or `jpeg` when the stored bytes are JPEG. Reuse is the
       rule, not a switch. There is no downsample-cap property.
- [ ] 50.2 The image browser loads a local file or takes a clipboard
       paste. A designer panel shows the suggested pixel size from
       the physical box and the item dpi, and lets the author change
       the width and the height. Each side is at least 1 pixel. A
       side that rounds to zero, or that the author sets below 1,
       is kept at 1. The panel states that confirming stores those
       pixels and does not keep the original file.
       Confirm calls `image_scale` unless the chosen size is the
       source's own pixel size, in which case the original bytes are
       stored. Embedded bytes dedupe into the definition's
       content-hash registry. Changing the box later does not
       resample.
- [ ] 50.3 `stage=html` repeats the data URI of the stored bytes in
       each placement. DOMPurify keeps that data URI. The script
       does not call `image_scale`.
- [ ] 50.4 Browser: the panel shows a suggested size, an edited size
       is what gets stored, and the same PNG on two detail rows
       shows in both rows of the preview, at the box size.

### Done means

The preview shows the same embedded PNG on two detail rows.

### Exit gate

The script update applied, lint, and the browser check.

### Status

**not started**

---

## Phase 51 — Reference and URL images

### Goal

A reference image and a URL image each resolve to bytes and render.

### Entry gate

Phase 50 Status complete. The Phase 17 document QueryRef is applied.

### Work items

- [ ] 51.1 The reference picker lists documents through the Phase 17
       read QueryRef. A URL item stores the URL.
- [ ] 51.2 `Reports.Render` resolves both kinds to bytes before any
       item is placed. References use `H.altquery_sync` on
       `params._hydrogen.database` and the Phase 17 QueryRef. A
       missing or unconfigured database name adds a warn entry and
       leaves the box empty. URLs use `H.http.request_sync` against
       that URL, with no Hydrogen JWT. A failed reference or an
       unreachable URL is a warn entry and an empty box.
- [ ] 51.3 Browser: a reference source and a URL source each render
       once their bytes resolve.

### Done means

A reference image and a URL image each render once.

### Exit gate

The script update applied, and the browser check. The Phase 17
document QueryRef was applied before that check.

### Status

**not started**

---

## Phase 52 — PDF image stored once

### Goal

A repeated image is one PDF image object, and each placement
references it.

### Entry gate

Phase 51 Status complete.

### Work items

- [ ] 52.1 `stage=pdf` calls `doc:image` with the same resolved bytes
       for every placement of that image. The port stores one image
       object and each placement references it. JPEG sources stay
       JPEG. Everything else stores as PNG. There is no XO embedder.
       The script does not call `image_scale`.
- [ ] 52.2 Fixture: the same PNG on two detail rows. The PDF contains
       one image object and two drawing references. A second,
       different PNG is a second object. The fixture stays under the
       1 MiB result cap after base64. Say in the Working Log that the
       port registry supplied the reuse.
- [ ] 52.3 Hand the amendment over. After apply, the fixture matches.

### Done means

The repeated PNG is one PDF image object with two drawing references.

### Exit gate

Andrew's apply report and the fixture call.

### Status

**not started**

---

## Phase 53 — Seed lua-zint

### Goal

Put the pure-Lua zint port into `scripts` so `require` loads it. No
report output yet.

### Entry gate

Phase 0 Status complete. Re-check the next Acuranzo file number the
day this is written. Phases 1–52 may still be open. Phases 54 and 69
wait for the apply.

### Work items

- [ ] 53.1 Copy the tree named in the library lock into one migration.
       One migration seeds every `zint.*` module (`init`, `barcode`,
       `common`, …). `invokable = 0`, `mcp_access = 0`. Group name
       `zint`. The script name is the require path after the first
       dot. One library per file. Reverse deletes only those rows.
       The packet records the commit. If neither the port repository
       nor the task-tree path is present, stop and ask.
- [ ] 53.2 The port writes a file when `opts.output` is set, and the
       sandbox leaves `io` nil. The probe leaves that field unset and
       uses the returned SVG string.
- [ ] 53.3 Hand the packet over. After apply, Andrew runs a probe:
       `require("zint.barcode")` for a Code 128 and a QR code. Each
       probe returns an SVG that contains the data string.
       `AllowDBModuleLoad` is on for that call.

### Done means

Andrew reports a Code 128 SVG and a QR SVG from inside Hydrogen, with
`io` left off.

### Exit gate

Andrew's apply report and the probe. Test 98 on the new migration
if he ran it.

### Status

**not started**

---

## Phase 54 — Barcode SVG

### Goal

Static and field barcodes render as SVG in the HTML preview.

### Entry gate

Phase 53 and Phase 25 Status complete.

### Work items

- [ ] 54.1 Palette shows lookup 053 keys 43 (Static Barcode) and 45
       (Field Barcode). A new barcode writes raster `dpi` `300`.
       The item stores `symbology` as one of `qr`, `code39`,
       `code128`, `upca`, `gs1-128`, plus the data string or the
       field name. Other symbologies are refused with a warning.
- [ ] 54.2 Compose calls `zint.barcode` with no `output` path. The
       SVG inlines into the item box in the HTML preview and the
       HTML export.
- [ ] 54.3 DOMPurify is configured so this SVG survives the preview
       sandbox. A stripped SVG is a failed gate.
- [ ] 54.4 Browser: a Code 128 of a known string and a QR of a known
       string appear in the preview. A field barcode follows the
       sample row.

### Done means

The Code 128 and the QR survive the preview sanitizer, and the field
barcode follows the sample row.

### Exit gate

Applied script update, lint, and the browser check.

### Status

**not started**

---

## Phase 55 — Barcode PDF raster

### Goal

Until Phase 71, a barcode in the PDF is a PNG Lithium rasterized and
passed in `params.rasters`.

### Entry gate

Phase 54 and Phase 31 Status complete.

### Work items

- [ ] 55.1 `stage=pdf` embeds a barcode PNG only when Lithium passes
       it in `params.rasters` under the item id. Lithium rasterizes
       the barcode SVG through `image_scale` at the item's dpi
       (default 300). A missing raster omits the symbol and adds a
       warning. The Working Log names this raster path.
- [ ] 55.2 A Code 128 of a known string appears in the PDF.

### Done means

A Code 128 appears in the PDF as a raster.

### Exit gate

The browser check, or Andrew's fixture PDF if the browser cannot
open it. Lint passes.

### Status

**not started**

---

## Phase 56 — Seed d3.lua

### Goal

Put the pure-Lua D3 port into `scripts` so `require` loads it. No
report output yet.

### Entry gate

Phase 0 Status complete. Re-check the next Acuranzo file number the
day this is written. Phases 1–55 may still be open. Phase 57 may
run alongside this one. Phases 59 and 70 wait for the apply.

### Work items

- [ ] 56.1 Copy the module named in the library lock into one
       migration. `invokable = 0`, `mcp_access = 0`. Phase 0 locked
       `require("d3.lua")`: group `d3`, script name `lua`. Reverse
       deletes only those rows. The packet records the commit. If
       neither the port repository nor the task-tree path is present,
       stop and ask.
- [ ] 56.2 The demo writes its SVG with `io.open`, and the sandbox
       leaves `io` nil. Confirm the module returns its SVG as a
       string with no file written.
- [ ] 56.3 The regression oracle is the port's reference pair, the
       D3.js chart and the Lua chart, whose SVGs are byte-identical.
       After apply, Andrew's probe reproduces the reference SVG from
       inside Hydrogen. The probe stops at that SVG.
- [ ] 56.4 Hand the packet over. Per-kind charts wait for Phase 57
       and the draw phases.

### Done means

Andrew reports the reference SVG reproduced byte-for-byte inside
Hydrogen, with `io` left off.

### Exit gate

Andrew's apply report and the probe. Test 98 on the new migration
if he ran it.

### Status

**not started**

---

## Phase 57 — Chart design lock

### Goal

Agree the chart contract before the designer and the Lua grow around
it. No source edits.

### Entry gate

Phase 0 Status complete. This phase may run alongside Phases 1–56.
Phase 58 waits for this phase.

### Work items

- [ ] 57.1 Lock the chart kinds for v1: bar, stacked bar, line,
       dual-axis, combinations, and pie. The series-first model in
       Charts is the rule. `kind` stays optional shorthand.
- [ ] 57.2 Lock the options hierarchy: chart options, then one or
       more series. Adding a series picks that series' format.
       Record the option defaults in the Defaults table. Chart `dpi`
       stays 150.
- [ ] 57.3 Lock the four scopes as written in Charts: `band`,
       `group`, `record`, and `custom`.
- [ ] 57.4 Record the arc the port will add. Pie is in v1. This
       phase does not edit the port. Phase 63 adds the arc and
       previews one pie.
- [ ] 57.5 Name the fixtures: one bar, one grouped-scope bar, one
       record-scope bar, one line, one dual-axis line, one stacked
       bar with an overlaid line, and one pie.

### Done means

The chart contract in this file is the one Andrew approved.

### Exit gate

Andrew's explicit approval. No source edited.

### Status

**not started**

---

## Phase 58 — Charts in the designer

### Goal

Place a chart, add its series, and round-trip the bindings through
the definition file.

### Entry gate

Phase 57 Status complete.

### Work items

- [ ] 58.1 Palette shows lookup 053 keys 42 (Static Chart) and 44
       (Field Chart). The item stores a `chart` object per the locked
       contract, plus its box. A new chart writes `dpi` `150` and the
       option defaults Phase 57 added.
- [ ] 58.2 `chart-panel.js` walks the options hierarchy: the chart's
       options first, then an add-series row. Each series names its
       datasource, field, role, stack, axis, and label. A kind
       shorthand fills the common cases.
- [ ] 58.3 The panel follows the property panel's commit rules.
       Options are designer state until commit. The browser does not
       draw the chart.
- [ ] 58.4 An empty series binding blocks export with a toast.
- [ ] 58.5 Browser: place a chart, add a bar series and a line series
       over the session sample, set the dual axis, export, reload,
       and see the bindings.

### Done means

A chart item round-trips through the definition file with its series
and options.

### Exit gate

Lint, unit tests, and the browser check.

### Status

**not started**

---

## Phase 59 — Bar chart

### Goal

A band-scope bar chart renders in the HTML preview.

### Entry gate

Phase 19, Phase 56, and Phase 58 Status complete.

### Work items

- [ ] 59.1 Resolve scope `band` as every row of the chart's
       datasource. A `custom` scope warns and draws nothing.
       Phase 64 draws that scope.
- [ ] 59.2 d3.lua draws the bar with the resolved rows, the
       `categoryField`, the series, and the box in points. The
       returned SVG is stored on the intermediate. The intermediate
       does not re-derive the chart.
- [ ] 59.3 The HTML export and the preview inline that SVG.
       DOMPurify keeps it. A stripped SVG is a failed gate.
- [ ] 59.4 One band-scope bar fixture from Phase 57. Hand the
       amendment over. After apply, the fixture matches and the
       preview shows the bar.

### Done means

The band-scope bar fixture matches the expected intermediate and
renders in the preview.

### Exit gate

Applied script update, the fixture call, and the browser preview.

### Status

**not started**

---

## Phase 60 — Group and record chart scopes

### Goal

The same bar draws for a group window and for a record window.

### Entry gate

Phase 59 Status complete.

### Work items

- [ ] 60.1 Scope `group` uses the rows of the current group. Scope
       `record` filters the named datasource on the current record's
       `keyField`. A chart on a detail band draws once per page, on
       the first detail of that page, not once per row.
- [ ] 60.2 One fixture for `group` and one for `record`, using the
       bar from Phase 59.
- [ ] 60.3 Hand the amendment over. After apply, both fixtures match
       and both preview.

### Done means

The group fixture and the record fixture both match and both preview.

### Exit gate

Applied script update, the two fixture calls, and the browser preview.

### Status

**not started**

---

## Phase 61 — Line charts

### Goal

A line chart and a dual-axis line chart render in the preview.

### Entry gate

Phase 60 Status complete.

### Work items

- [ ] 61.1 Draw the Phase 57 line fixture and the dual-axis line
       fixture with the same d3 path Phase 59 uses. Roles `line` on
       one axis, then `line` on two axes.
- [ ] 61.2 Hand the amendment over. After apply, both fixtures match
       and both preview.

### Done means

The line fixture and the dual-axis fixture both match and both
preview.

### Exit gate

Applied script update, the two fixture calls, and the browser preview.

### Status

**not started**

---

## Phase 62 — Combo chart

### Goal

A stacked bar with an overlaid line renders in the preview.

### Entry gate

Phase 61 Status complete.

### Work items

- [ ] 62.1 Draw the Phase 57 combo fixture: bar series sharing a
       `stack`, plus a line series. This is the shape of the port's
       demo chart, drawn with the report's series, not by pasting
       the demo.
- [ ] 62.2 Hand the amendment over. After apply, the fixture matches
       and the preview shows the combo.

### Done means

The combo fixture matches the expected intermediate and renders in
the preview.

### Exit gate

Applied script update, the fixture call, and the browser preview.

### Status

**not started**

---

## Phase 63 — Pie chart

### Goal

Add the pie arc to the port and preview one pie.

### Entry gate

Phase 62 Status complete.

### Work items

- [ ] 63.1 Add the arc in the port's repository, record the commit,
       and preview one pie fixture.
- [ ] 63.2 Hand the script amendment over. After apply, the fixture
       matches and the preview shows the pie.

### Done means

The pie fixture matches the expected intermediate and renders in
the preview.

### Exit gate

Applied script update, the fixture call, and the browser preview.

### Status

**not started**

---
## Phase 64 — Custom chart

### Goal

A chart scope of `custom` plots the rows its function returns.

### Entry gate

Phase 63 Status complete. Phase 33 Status complete.

### Work items

- [ ] 64.1 A `custom` scope names a function whose `returns` is
       `rows`. Compose calls it with the band context, using the
       Phase 33 compiler, and plots the rows it returns.
- [ ] 64.2 The Phase 57 custom fixture renders in the preview. Hand
       the amendment over.

### Done means

The custom-chart fixture renders in the preview from the Phase 33
compiler.

### Exit gate

Applied script update, the fixture call, and the browser preview.

### Status

**not started**

---

## Phase 65 — Chart PDF raster

### Goal

Until Phase 71, a chart in the PDF is a PNG Lithium rasterized and
passed in `params.rasters`.

### Entry gate

Phase 64 Status complete.

### Work items

- [ ] 65.1 Lithium rasterizes the chart SVG through `image_scale` at
       the chart `dpi` (default 150) and passes the PNG in
       `params.rasters` under the item id. A missing raster omits the
       chart and adds a warning. HTML, preview, and SVG keep the
       vector.
- [ ] 65.2 The band-scope bar from Phase 59 appears in the PDF.

### Done means

The band-scope bar appears in the PDF as a raster.

### Exit gate

The browser check, or Andrew's fixture PDF. Lint passes.

### Status

**not started**

---

## Phase 66 — SVG emit

### Goal

`stage=svg` returns one SVG document of the composed pages.

### Entry gate

Phase 65 Status complete. The fixture is the text detail. Chart and
barcode SVG inline from the intermediate.

### Work items

- [ ] 66.1 Amend `Reports.Render` with `stage=svg`. One page per
       section, positions in physical CSS units, the definition's
       font face, and page breaks marked per the Emit contract. A
       label-sized report is this file with the page size set to the
       label and the margins set to zero.
- [ ] 66.2 Chart and barcode items inline their SVG from the
       intermediate. Images inline as data URIs at their scaled size.
- [ ] 66.3 Add the expected SVG skeleton for the detail fixture. Hand
       the packet over. Andrew's call matches, and a browser check
       embeds the SVG and shows the pages.

### Done means

The detail fixture's SVG matches the expected skeleton and embeds in
a page with the page breaks marked.

### Exit gate

Applied script update, the fixture call, and the browser check.

### Status

**not started**

---

## Phase 67 — PNG export

### Goal

The manager turns a `stage=svg` result into one PNG per page.

### Entry gate

Phase 66 Status complete. `Reporting.Enabled` is true on the server
used for the gate.

### Work items

- [ ] 67.1 The export action requests `stage=svg`, then posts each
       page's SVG to `image_scale` with `units=px`, the page box,
       `dpi` 150, and `format=png`. The script does not call that
       endpoint.
- [ ] 67.2 A label downloads as one PNG. More than one page downloads
       as a zip of PNGs. The script result stays the SVG, so the
       1 MiB result cap applies to that SVG.
- [ ] 67.3 Browser: the label fixture downloads one PNG at the
       label's physical size.

### Done means

The label fixture becomes one PNG, rasterized by `image_scale` from
the SVG the script returned.

### Exit gate

Lint and the browser check. Reporting was enabled for that check.

### Status

**not started**

---

## Phase 68 — Native PDF contract

### Goal

Agree the payload lua-zint and d3.lua will return, before either
port grows a PDF writer. No port code in this phase.

### Entry gate

Phase 67 Status complete. Phase 54 Status complete. The barcode SVG
and the chart SVG are what this payload replaces.

### Work items

- [ ] 68.1 Agree the payload both ports will return: a bounding box,
       the resources it needs, and a content stream that lua-pdfkit
       can place. Pin the entry name in the contract note. Do not
       guess the table in this plan. Ask when a field is still open.
- [ ] 68.2 Andrew approves that note. No migration is written.

### Done means

The contract note names the payload Andrew approved.

### Exit gate

Andrew's explicit approval. No source edited.

### Status

**not started**

---

## Phase 69 — Zint PDF payload

### Goal

lua-zint returns the Phase 68 payload for a Code 128 and a QR code.

### Entry gate

Phase 68 Status complete. The work is in the zint repository.

### Work items

- [ ] 69.1 Extend lua-zint so a Code 128 probe and a QR probe return
       the Phase 68 payload. Record the commit.
- [ ] 69.2 No Helium packet in this phase. Phase 71 ships the
       modules.

### Done means

Andrew reports the two zint probes from the port's tests, and each
probe matches the Phase 68 payload.

### Exit gate

The port probe. No migration is applied in this phase.

### Status

**not started**

---

## Phase 70 — D3 PDF payload

### Goal

d3.lua returns the Phase 68 payload for the port's reference chart.

### Entry gate

Phase 68 and Phase 56 Status complete. The work is in the d3
repository. This phase may run before Phase 69.

### Work items

- [ ] 70.1 Implement the chart PDF emit against the Phase 68 payload,
       including the gradient and the standard face the demo chart
       uses.
- [ ] 70.2 Probe with the port's reference chart. Record the commit.
       No Helium packet in this phase.

### Done means

Andrew reports the d3 probe payload from the port's tests.

### Exit gate

The port probe. No migration is applied in this phase.

### Status

**not started**

---

## Phase 71 — Place native PDF objects

### Goal

`stage=pdf` places the payloads from Phases 69 and 70. Charts and
barcodes in the PDF are vectors.

### Entry gate

Phases 31, 55, 65, 69, and 70 are complete.

### Work items

- [ ] 71.1 Extend lua-pdfkit where the payload needs a placeable
       vector resource, a matrix, and the shared resources the two
       payloads reference. Record the commit. Ship the changed
       modules in a new migration. Do not edit an applied seed in
       place.
- [ ] 71.2 Amend `Reports.Render`. Chart and barcode items in
       `stage=pdf` call the native emits and place the objects.
       `params.rasters` is not required for these items. The SVG
       paths keep their SVG from the same compose.
- [ ] 71.3 A chart fixture and a barcode fixture, rendered as PDFs.
       Andrew opens each one. Axis text is selectable. Bars stay
       crisp when the page is zoomed.
- [ ] 71.4 Record the port commits the packet pins.

### Done means

A chart PDF and a barcode PDF open as vectors the ports emitted.

### Exit gate

Applied script update, the fixture PDFs, and Andrew's confirmation
that the objects are vectors.

### Status

**not started**

---
## Later, not scheduled

Pull one of these into a numbered phase only by amending this file.

| Topic | Why it waits |
| --- | --- |
| Memos and shapes | Each is a `typeKey` with its own box behavior. Images are Phases 50–52. |
| Other barcode keys | Phase 54 places lookup 053 keys 43 and 45. Aggregate, calculated, and crosstab barcode keys stay hidden. |
| Chart kinds beyond the v1 set | Pie is in v1. Phase 63 extends the port. Anything past bar, stacked bar, line, dual-axis, combos, and pie is a port extension plus a panel pass. |
| Chart interactivity | Hover, zoom, and tooltips need scripted SVG, and the exports are static files. |
| Crosstabs | A second layout engine. |
| Subreports and packages | Nested definitions and their own datasources. |
| Multiple group levels | v1 is one level. The key is a field, or one Lua function from Phase 45. |
| Related datasources | v1 is one list. A link between two lists is a new contract. |
| Stored blobs past the caps | A datasource over the params cap, or a PDF whose base64 JSON exceeds 1 MiB. The params cap is operator config, up to the 10 MiB POST ceiling. The result cap is the compile-time 1 MiB scoreboard ceiling. `H.set_result(type, location)` stores two strings and needs a store Hydrogen does not have. The gcode upload endpoint is not that store. |
| Parameter form | Declarations and the run strip are in this plan. Saved presets, lookup-driven pickers, and validation beyond type wait here. |
| Native XLS | CSV is Phase 49. A spreadsheet writer waits until that path is the boring one. The retired client loaded SheetJS in the browser. |
| Thumbnails | `thumbnail` stays null until a preview snapshot has a home. |
| Text measurement and stretching bands | v1 clips to the item box, on the canvas and in emit. Growing a band needs a font engine shared by compose and the designer. |
| Vanadium in PDF | The v1 PDF face is Helvetica. Embedding the product face needs the font bytes inside the sandbox, with `io` left off. |
| Mail and scheduled runs | Mail Relay and the script scheduler. `Reports.Render` stays invokable. Scheduling is a separate Lithium module covering reports, ad hoc queries, and emails. This plan has no scheduler UI and no `scripts` schedule row. |
| Report permissions and sharing | Who can generate or view which report. Hydrogen JWT auth exists; a row-level ACL model is not defined here. This waits on the permissions pass; the report writer does not block on it. |
| Text search and filtering of report output | Full-text search over rendered HTML or PDF. Which rows come back stays on the query. |
| Incremental and delta reports | "Rows changed since the last run." Needs a watermark column on the datasource and a "last run" marker. v1 is a full snapshot. |
| Conditional formatting | Text color, background, or font overrides driven by a field value or a formula. A style rule is a condition plus a set of property changes. v1 renders unstyled values. |
| Data filtering | The query's parameters decide which rows come back. A report-side filter expression stays here. v1 renders every row it was given. |
| Detail row sorting | The query owns order. v1 also sorts one group by its key. A designer sort of the detail rows stays here. |
| Template inheritance | A base definition that another definition extends, overriding bands or items by id. v1 definitions are standalone. |

## Working log

### 2026-10-10 — Plan locked

Phase 0 is approved. The locks in this file are the contract. Phase 1
is next when Andrew pulls reporting in. CATCHUP stays P0 until then.
