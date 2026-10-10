<!-- markdownlint-disable MD007 MD024 MD013 MD036 -->

# Reporting Plan

**Date:** 2026-10-08
**Status:** Phase 0 complete. Approved 2026-10-10.
**Home:** Manager **24**, Report Manager, already registered.
**Design:** Acuranzo (shared product database). Lookups 0–199. Migrations
1000–1999.

This is the gated plan for a report writer. Lithium is the designer. The
designer writes a JSON report definition. A Helium Lua script reads that
definition plus zero or more JSON datasources, builds an intermediate
document, and emits a format. The exports are PDF (the default), HTML,
SVG, PNG, and CSV, in that implementation order. PDF comes from the
pure-Lua PDFKit port, barcodes from the pure-Lua zint port, charts from
the pure-Lua D3 port, and image sizing from Hydrogen's existing
ImageMagick endpoint. The end state for charts and barcodes in the PDF
is the vector objects the zint and d3 ports emit natively; the raster
stopgap before that lands is named per phase. A label-sized report
with no pagination is a bare SVG or PNG. Authors store their own Lua
functions in the definition. The same script compiles those functions
and runs them.

PDFs will run past the 1 MiB script-result cap. That cap, and what the
operator raises to lift it, is part of the Emit contract below.

CATCHUP stays the active Lithium plan
([`/elements/003-lithium/CATCHUP.md`](/elements/003-lithium/CATCHUP.md)).
This plan starts when Andrew says so. It does not move Course Manager,
Scripting invoke, or the lifecycle fix.

Argent Phase 15 "Report queries" in
[`/docs/H/plans/ARGENT_PLAN.md`](/docs/H/plans/ARGENT_PLAN.md) is a set of
bookkeeping QueryRefs. It is a different piece of work.

## How to use this document

- Work **one phase at a time**, top to bottom.
- Do not start a phase until the previous phase Status is complete and
  its Exit gate is green.
- Each phase has one **Done means** line. That is the testable state.
- Mark a work item `[x]` only when that item's verification passed.
- Defer with `[~]` plus one line and the phase it moves to.
- After each phase: fill Status, append the Working Log, and stop.
- A Helium phase ends when the packet is handed over. It is complete
  only after Andrew reports the apply. The agent does not run SchemaTool
  or SchemaHelper apply, and does not apply a migration.

## Implementor workflow

Each phase is its own conversation.

1. Re-read the previous phase Status and Exit gate.
2. Re-read this phase Goal, Work items, Done means, and Exit gate.
   Ask before editing if a lock is ambiguous.
3. Wait for approval before editing source on a phase that has not
   been approved.
4. Ask when a requirement is ambiguous. Do not guess a schema change.
5. Update the Working Log when a major piece lands.
6. Record lessons learned for the phase.
7. Check work items and set Status complete only after the named
   command passed. Intent to verify is not verification.
8. Never apply a database migration. Prepare the packet and hand it
   over. Re-check the next free Acuranzo file, QueryRef, and lookup
   id on disk before numbering anything. The snapshot in this file
   goes stale.
9. Lithium rules win on the SPA:
   [`/docs/Li/LITHIUM-INS.md`](/docs/Li/LITHIUM-INS.md) and
   [`/elements/003-lithium/AGENTS.md`](/elements/003-lithium/AGENTS.md).
   Files stay at or under 1000 lines. CSS first. `element.style` is
   limited to drag geometry (`top`, `left`, `width`, `height`).
   No `alert`, `confirm`, or `prompt`. Log through `log(...)`.
   Preview HTML goes through DOMPurify and a sandboxed iframe.
10. Helium rules win on packets:
    [`/elements/002-helium/AGENTS.md`](/elements/002-helium/AGENTS.md),
    then Designs, packs, and numbers in
    [`/docs/He/GUIDE.md`](/docs/He/GUIDE.md). One file, one change.
    Copy an existing seed. Do not invent macros.
11. Do not add a Hydrogen `test_NN` script, a new npm dependency, or a
    new manager id unless a work item in this plan says so.
12. Do not log JWTs, script params that contain report data, or
    datasource rows in normal logs.

### Verification

| Change | Check |
| --- | --- |
| Lithium JS / CSS | `npm test`, `npm run lint`, `npm run lint:css` from `elements/003-lithium` |
| HTML template | `npm run templates:copy` after the edit |
| Designer UI | Browser pass of the phase's flow (click, drag, save). A screenshot is not the gate |
| Helium packet | Test 31 and Test 98 when Andrew asks. Payload regenerate (`mka` or `payload-generate.sh`) before tests 30–38 or 71. `mkt` does not refresh the archive |
| Live render | Andrew runs the conduit script call after the seed is applied and reports the result |

## Resume here

**Pause point (2026-10-10):** Phase 0 is approved. Phase 1 (Contracts and
fixtures) can start.

## Priority

| | |
| --- | --- |
| **Band** | P2. Starts when Andrew pulls it in. CATCHUP remains P0. |
| **Effort** | XL across the phases. Each phase is meant to be one sitting. |
| **Done** | 0%. This document is the first pass. |
| **Why now** | Hydrogen's host work is close to done. The report writer is the next product surface, and the lookups and table already exist. |
| **Do not start casually** | The JSON contract has to be agreed before the designer and the Lua script both grow around it. |

## Phases

**Effort** is easy / medium / hard: how large the phase is as a single
sitting, and a hint about model strength. Easy and medium phases are
comfortable for a mid-range model. Hard phases are where a stronger
model pays for itself — they carry fixtures plus a migration plus a
hand-over, or they extend another repository.

On session sizing: 28 of the 35 phases are one sitting each — easy or
medium, in one tree, ending in a check that a browser, a fixture call,
or a lint pass can confirm. The hard phases are hard for a stated
reason each, not for size:

| Phase | Why it is hard |
| --- | --- |
| 17 | The Lua engine, three fixtures, a seed packet, and a hand-over in one session. |
| 20 | Same shape, plus a first port wired into compose. |
| 27 | Four chart scopes plus the d3 draw call plus five fixtures. |
| 28 | The compiler contract, `stage=check`, the editor wiring, and a fixture that fails at runtime on purpose. |
| 30 | Custom breaks change paging, so the fixture has to prove the page list. |
| 34 | Three repositories, split into three sittings by the scope note in its section. |

A phase that hands a packet to a human is one sitting of agent work
plus the wait for the apply, and its Status block records the wait.
No phase is over-sized beyond Phase 34's, which is split in place
rather than renumbered.

| Phase | What | Kind | Effort | Status |
| --- | --- | --- | --- | --- |
| 0 | Design lock | Decision | Easy | Complete |
| 1 | Definition, intermediate, and emit contracts | Docs + fixtures | Medium | Not started |
| 2 | Units and geometry | JS + Vitest | Easy | Not started |
| 3 | Document model and undo | JS + Vitest | Medium | Not started |
| 4 | Manager shell, import, export | Lithium UI | Medium | Not started |
| 5 | Page surface | Lithium UI | Easy | Not started |
| 6 | Bands and resize | Lithium UI | Medium | Not started |
| 7 | Section grid | Lithium UI | Easy | Not started |
| 8 | Rulers and the mouse line | Lithium UI | Medium | Not started |
| 9 | View scale | Lithium UI | Easy | Not started |
| 10 | Static text | Lithium UI | Medium | Not started |
| 11 | Lookup repair packet | Helium, human apply | Medium | Not started |
| 12 | Property panel | Lithium UI | Medium | Not started |
| 13 | Field items and a sample datasource | Lithium UI | Easy | Not started |
| 14 | One group level | Lithium UI | Easy | Not started |
| 15 | Save and load packet | Helium, human apply | Medium | Not started |
| 16 | Revisions in the manager | Lithium UI | Medium | Not started |
| 17 | `Reports.Render` compose + HTML | Helium seed, human apply | Hard | Not started |
| 18 | Preview | Lithium UI | Medium | Not started |
| 19 | Seed lua-pdfkit | Helium library pack | Easy | Not started |
| 20 | PDF emit | Helium amendment | Hard | Not started |
| 21 | Images through `image_scale` | Lua + designer | Medium | Not started |
| 22 | Seed lua-zint | Helium library pack | Easy | Not started |
| 23 | Barcodes through lua-zint | Lua + designer | Medium | Not started |
| 24 | Seed d3.lua | Helium library pack | Easy | Not started |
| 25 | Chart design lock | Decision | Easy | Not started |
| 26 | Charts in the designer | Lithium UI | Medium | Not started |
| 27 | Charts in compose | Helium amendment | Hard | Not started |
| 28 | User Lua in the definition | Helium + designer | Hard | Not started |
| 29 | Primitive aggregates | Helium + designer | Medium | Not started |
| 30 | Custom breaks and band context | Helium + designer | Hard | Not started |
| 31 | SVG emit | Helium amendment | Easy | Not started |
| 32 | PNG emit | Helium amendment | Easy | Not started |
| 33 | CSV emit | Helium amendment | Easy | Not started |
| 34 | PDF-native output from the zint and d3 ports | Helium amendment | Hard | Not started |

Later work is listed after the phases. It is not scheduled.

## Scope

| Tree | Role in this plan |
| --- | --- |
| `elements/003-lithium/src/managers/reports/` | Manager 24. Shell, canvas, rulers, properties |
| `elements/003-lithium/src/managers/reports/model/` | Pure units, document, undo. Vitest imports these |
| `elements/003-lithium/tests/unit/` | Tests for the model and, later, `invokeScript` |
| `elements/003-lithium/tests/fixtures/reporting/` | Definition, datasources, expected intermediate, expected HTML |
| `docs/Li/` | Contract note written in Phase 1. This plan stays the gate list |
| `elements/002-helium/acuranzo/migrations/` | Lookup repair, QueryRefs, `Reports.Render`, lua-pdfkit, lua-zint, d3.lua |
| `elements/001-hydrogen/hydrogen/src/` | No new C. Image work calls the existing Reporting endpoint |

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
   after Phase 16, through the `reports` table.
3. One Lua program, `Reports.Render`, with two stages. Compose maps
   datasources onto the definition and assigns pages. Emit turns that
   intermediate document into the requested format.
4. PDF as the default export, then HTML, then SVG, then PNG, then CSV,
   in that implementation order. SVG is a structured document with page
   breaks marked. PNG is that SVG rasterized per page through
   `image_scale`, which is what a label-sized, non-paginated report
   wants. HTML is an export and the preview medium at once.
5. PDF from lua-pdfkit, barcodes from lua-zint, charts from d3.lua, and
   photographs sized by `POST /api/reporting/image_scale`. Hydrogen does
   not grow a PDF, barcode, or chart library.
6. Charts and barcodes are first-class items. They render as SVG in the
   HTML, preview, and SVG paths, and the end state for PDF is the
   vector objects the zint and d3 ports emit natively. Any raster
   stopgap before that lands is named per phase, not assumed.
7. CSV as the tabular download. A native spreadsheet writer waits.
8. Report authors write Lua functions in the definition JSON.
   `Reports.Render` compiles and runs them. A field formula receives
   the current record. A header, footer, or break receives the
   context for that band.
9. The existing lookup families stay the catalog of object types,
   attributes, and page sizes.
10. Built-in aggregates: count, unique, sum, avg, weighted mean,
    mean, mode, and median.
11. Chart items bind data with full scope flexibility: the band's own
    datasource, a named datasource, the current group, or a custom
    function, placed on any band.
12. Image elements carry their own resolution and reuse options, so a
    repeated image is stored once in an emitted file.

## Out of scope until a later plan

- Crosstabs, subreports, packages, shapes, and memos. The lookup rows
    can stay. The palette hides them. Chart keys 42, 43, 44, and 45
    are v1.
- Chart interactivity. The exported chart is static SVG or a PDF
    vector object; hover, zoom, and tooltips are a later idea.
- A user's own SVG entering a PDF as a vector. Phase 34's native
  output serves charts and barcodes first; a user-supplied SVG
  rasterizes through `image_scale` until a later plan extends the
  ports' native output to imported art.
- A native XLS writer. CSV is the spreadsheet stand-in.
- A SQL catalog inside the designer. v1 datasources are JSON.
- Browser PDF generation (`jsPDF`) and a PDF thumbnail viewer.
- Mailing a report. Mail Relay already exists for mail.
- A new Hydrogen subsystem, config letter, or C route. PDF, barcodes,
  and image bytes stay in Lua plus the Reporting endpoint that
  already shipped.
- Rewriting CATCHUP, Course Builder, or Argent's financial queries.
- Measuring wrapped text inside Lua. v1 bands have a fixed height.
- Handing a user formula the Hydrogen host API. `H` stays in
  `Reports.Render`. `require` stays the loader for lua-pdfkit and
  lua-zint.
- Datasource payloads over the conduit script param cap (256 KiB
  default, `ClientInvokeMaxParamsBytes`).

## What already exists

### Lithium

Manager 24 is a 49-line placeholder. It tells the operator the module
is under development. `conduit.js` can run QueryRefs.
`invokeScript` is not in the client. Scripting Manager does not call
`POST /api/conduit/script`. CATCHUP Phases 7–10 are the planned home
for that helper. Phase 18 of this plan adds it if those phases have
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
what removes the need to wait on the Phase 11 repair packet for them.
Most object types in lookup 053 advertise only
`attributes: [0, 1, 2, 3, 4, 14, 24, 29]`
(Top, Left, Width, Height, Align, Rotation, Grid Background Color,
Link). Page Header and Detail advertise a longer list that includes
fonts. A field cannot show its datasource or its font with the lists
as they stand. Phase 11 repairs the v1 types.

Known seed defects, also Phase 11:

- Lookup 053 key 7 (System Server) and key 16 (Group Footer) store
  `"Iion"` where the other rows store `"icon"`.
- Lookup 053 key 52 (CrossTab DateTime) has an unclosed `<fa>` tag.
- Lookup 057 key 1 is labeled `US Letter Legal` and its collection is
  US Letter landscape (`11 in` by `8.5 in`, page size `US Letter`).
- Lookup 057 has no Legal portrait, A4, or Tabloid row.
- Lookup 054 font list is Helvetica, Open Sans, Cairo, Courier New,
  Lexend. Vanadium is the product face and is absent.
- There is no Page Number or Page Count type. Keys 0–60 are taken.
  The next keys are 61 and 62 if Phase 0 wants them. Re-check the
  max key before writing the file.

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
`invokable = 1`, `mcp_access = 0`, unless Phase 0 says otherwise.
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

## Proposed locks

Phase 0 approves these or amends them in place. Later phases implement
the approved text.

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
  "font": { "family": "Vanadium Sans", "size": "10 pt", "style": "normal" },
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
          "style": {}
        }
      ]
    }
  ],
  "functions": []
}
```

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

Rulers, Report Top, and Report Bottom are canvas chrome. They are not
bands in the file. Grid and ruler settings on the definition are view
defaults, stored so the next session opens the same page.

Chart and barcode items add a `chart` or `barcode` object described
under Charts and in Phase 23. Their series, scope, and options bind to
datasources, and their option panels are bespoke, not lookup 054 rows.

`functions` holds the author's Lua. The shape is in the next section.
An early definition stores `[]`.

A group band names one field or, from Phase 30, one Lua function that
returns the key. Compose sorts that key with Lua's ordinary `<`
unless the band sets `sort` to false, in which case it breaks only
when consecutive keys differ. Nested groups are later.

### User Lua

Report authors write functions. The source is stored on the definition
and runs inside `Reports.Render`. It is not a separate `scripts` row.

```json
{
  "name": "pack_code",
  "source": "function(ctx)\n  return ctx.row.month\nend"
}
```

`name` is a Lua identifier, unique in the report. `source` is a text
chunk that returns a function of one argument. `Reports.Render`
compiles it with `load(source, name, "t", env)` under Lua 5.5. Mode
`"t"` accepts source text.

`env` contains `string`, `math`, `table`, `utf8`, `tonumber`,
`tostring`, `type`, `pairs`, `ipairs`, `next`, `select`, `pcall`,
`error`, `date` (`os.date`), and `time` (`os.time`). The chunk's
environment is that table, so the formula sees those names and the
other functions in the same report. It does not see the worker
globals. `H` stays in `Reports.Render`. The scripting progress hook
already bounds the worker by `max_runtime_seconds`. This plan adds
no hook and no C.

`ctx` depends on where the function runs:

| Call | What `ctx` holds |
| --- | --- |
| Calculated item on a detail | `row` (the current record), `prev`, `index` (1-based), `band` |
| Group header or footer | Those, plus `group.key` and `rows` for the group |
| Page header or footer | `page.number`, and `page.count` once the page list exists |
| Report header or footer | `report.name` and `report.parameters` |
| Group, page, or line break | `row`, `prev`, and `index` |

A calculated item stores `"function": "pack_code"`. Lookup 053 keys
28 (Calculated Text) and 29 (Calculated Number) are the v1 calculated
items. The return value is substituted as text. A number uses
`tostring` when the function did not already return a string.

A compile error stops compose and returns the message. A runtime
error on one call is a warning and an empty value, so one bad row
does not drop the rest of the report. An unknown function name is a
compose error.

Breaks, implemented in Phase 30:

- `groupKey` names a function. Its return value is the group key.
  The one-field group remains the path with no Lua. One level.
- `pageBreak` names a function. A true result starts a new page
  before the current row. Fixed-height paging still applies.
- `lineBreak` names a function. A true result inserts one blank
  detail-height before the current row. A newline inside a
  calculated value is kept by HTML (`white-space: pre-line`) and by
  the PDF text drawer. The stored band height does not change.
  Overflow clips.

The pack-date fixture in Phase 28 is an invented pattern (month
letter, week digit, day), standing in for the grower-specific codes
on a PTI label. It is not a PTI specification.

### Primitive aggregates

Lookup 053 key 35 (Aggregate Number) stores `op` and `field`.
`weighted` also stores `weight`. The engine computes these. An author
who needs a different reduction writes a footer function and reads
`ctx.aggregates`.

| op | Result |
| --- | --- |
| `count` | Rows in the band's scope. Empty scope is 0. |
| `unique` | Distinct values of `field`, compared as text. Empty scope is 0. |
| `sum` | Sum of numeric `field`. |
| `avg` | Arithmetic mean. |
| `mean` | The same arithmetic mean as `avg`. |
| `weighted` | Sum of `field * weight`, divided by the sum of `weight`. |
| `median` | Middle value after a numeric sort. Even count: mean of the two central numbers. |
| `mode` | Most common value. Text uses key 34 (Aggregate Text). A number uses key 35. A tie keeps the smallest value under Lua `<` and records a warning. |

Group footer scope is the current group. Report footer scope is every
detail row. A non-numeric value in a numeric op warns and is skipped.
An empty numeric result is an empty string. Phase 29 places the
results on `ctx.aggregates` for footer formulas, keyed by op and
field name.

### Charts

A chart item is lookup 053 key 42 (Static Chart) or key 44 (Field
Chart). A Static Chart plots a datasource with fixed options. A Field
Chart is bound like a field barcode: the current record supplies a key
or a value. Both carry a `chart` object.

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

**Scopes.** All four ship in v1:

| scope | Data the chart plots |
| --- | --- |
| `band` | Every row of `datasource`, which defaults to the band's own. |
| `group` | The rows of the current group. |
| `record` | The rows of `datasource` whose `keyField` equals the current record's. |
| `custom` | The rows a function from `functions[]` returns for the current context. |

`datasource` may name any datasource on the definition, so a chart can
plot something completely different from the band it sits on. A chart
may be placed on any band. It draws on the first instance of its band
inside its window: once per group on a group band, once per page on a
detail or page band, once per report on a report band.

**Ports.** The SVG form comes from d3.lua. The end state for PDF is
the port's native PDF output (Phase 34): vector objects lua-pdfkit
places directly, with no SVG converted. Until Phase 34 lands, the
PDF path rasterizes the chart SVG through `image_scale` at the chart's
own `dpi` option (a chart defaults higher than text, because axis
labels suffer at 72), and Phase 34 replaces that detour. The two
ports stay SVG producers for the HTML, preview, and SVG paths, and the
PNG path rasterizes that SVG the same way. Composing happens once;
the chart's data is resolved per the scope above and then drawn in
whichever form the stage needs.

**Pie.** The v1 set includes pie, and the port does not have it yet.
The port as it stands is `scaleBand`, `scaleLinear`,
`axisLeft`/`axisRight`/`axisBottom`, `stack`, `curveBasis`, `line`,
and `range`. Phase 25 decides between extending the port with an arc
generator and pinning pie behind that extension. Everything else in
the v1 set is already in the port.

**Fonts.** A chart uses the definition's face in the SVG paths and a
standard face in the PDF, the same rule as text.

### Intermediate shape

Compose output is JSON. One page object per page. Each page has the
bands that landed on it, in order. Item text is the substituted
string. Boxes stay in physical units relative to the band. A group
break starts a group header and ends with a group footer. Page header
and page footer repeat. Report header is on page 1. Report footer is
on the last page.

Page breaks use fixed band heights against the content box (page size
minus margins). An item keeps the height stored on it. Text that does
not fit is the emit format's clipping problem. Lua does not measure
glyphs in v1.

### Emit

`params.stage` selects the stage on `Reports.Render`:

| stage | Result |
| --- | --- |
| `pdf` | Default. Base64 PDF bytes inside the JSON result. |
| `html` | One section per page with `break-after: page`. Export and preview medium. |
| `svg` | Compose, then one SVG document per report with page breaks marked. |
| `png` | Compose, then one PNG per page through `image_scale`, which takes SVG input and applies `dpi` before it reads. |
| `csv` | Phase 33. One row per detail record. Data only. |
| `compose` | Intermediate JSON via `H.set_result_json`. |
| `check` | Phase 28. Compile the posted functions and return diagnostics. No rows are read. |

PDF is the default stage and the highest-priority export. HTML is both
an export and the preview medium: the preview is the HTML export
artifact, rendered as a deliberate action into a sandboxed iframe, with
DOMPurify on it before `srcdoc` (Phase 18). The parent page never
assigns it to `innerHTML`. SVG and PNG are file exports. Charts and
barcodes are SVG in the HTML, preview, and SVG paths; in the PDF they
rasterize through `image_scale` at an explicit dpi until Phase 34
replaces that with the ports' native vector output.

**The PDF will not always fit the result cap.** `H.set_result_json`
is capped by `ClientInvokeMaxResultBytes`, 1 MiB by default, and a
multi-page PDF with images exceeds that. The contract is:

- A PDF that fits returns `{ "pdf_base64": "...", "warnings": [] }`.
- A PDF that does not fit returns an error through
  `H.set_result_json` naming the cap and the actual size. Compose
  already ran; only delivery failed, and the call may be retried once
  the operator raises the cap.
- Raising `ClientInvokeMaxResultBytes` is operator config in
  hydrogen.json. It is the v1 answer for large PDFs, and it needs no
  new C. The phase that writes this contract also confirms the conduit
  response path has no second, smaller body cap.
- A stored-output path — writing the file once and returning a
  location through `H.set_result(type, location)`, which is not capped
  — waits on a store. Hydrogen has no generic file or blob store; the
  `/api/files/local` upload endpoint is gcode-specific and is not that
  store. Recorded in "Later, not scheduled".

`Reports.Render` is one invokable row. lua-pdfkit, lua-zint, and
d3.lua are other rows, loaded with `require("group.script")`. Author
formulas are not rows. Hydrogen's DB searcher splits on the first dot:
group, then the rest of the name as `script_name`. `AllowDBModuleLoad`
has to be on for those three libraries. `require("lua-pdfkit")` is
rejected because it has no dot. The entry points are `lua-pdfkit.init`
and `zint.barcode`; the d3.lua entry point is pinned at the Phase 24
probe.

### Lua libraries

All three ports are pure Lua. None is in this repo yet. They were
written to run inside a host that has no native modules and no `io`.

| Library | Where it lives today | What Reports.Render calls |
| --- | --- | --- |
| lua-pdfkit | `/home/asimard/DA/triton4/triton-toolkit-5b12870/tasks/lua-pdfkit/solution/lua-pdfkit/` (pure Lua; its own repo planned) | `PDFDocument.new`, then `doc:finish()`, then `doc:output()` for the bytes. Port of PDFKit 0.19.1. JPEG and PNG from raw bytes or a `data:` URI. |
| lua-zint | `/home/asimard/DA/triton3/triton-toolkit-e8fb24a/tasks/lua-zint/solution/` (pure Lua; its own repo planned) | `zint.barcode(opts)` returns an SVG string. Symbologies in the port: QR, Code 39, Code 128, UPC-A, GS1-128. |
| d3.lua | Poseidon job artifact `/home/asimard/DA/poseidon1/jobs/lua-run10/` (pure Lua; its own repo planned). Validated byte-identical to D3.js on the reference pair `d3_chart.js` / `lua_chart.lua` | `d3.create_root`, `d3.scaleBand`, `d3.scaleLinear`, `d3.axisLeft` / `d3.axisRight` / `d3.axisBottom`, `d3.stack`, `d3.curveBasis`, `d3.line`, `d3.range`. Entry point pinned at the Phase 24 probe. |

**Status:** None of the three libraries is in this repository. They are
pure-Lua ports Andrew owns, each moving to its own repository: the zint
and pdfkit ports ship `tests/test.sh` plus `PORTING-NOTES.md` /
`AUTHOR-NOTES.md`, and the D3 port's validation is byte-equivalence
against D3.js itself. Each library gets its own seed phase and probe
(Phases 19, 22, and 24), and the packet records the repo commit or
version it was generated from. If a port reaches LuaRocks, that helps
only if Hydrogen's sandbox can read `package.path`; the seeded
`group.script` module is the path this plan builds on, and the probe
confirms which one is live.

All three are producers. The report mechanism treats SVG as the form
the HTML export and the preview embed inline and the SVG and PNG emit
paths return or rasterize, and it treats the ports' native PDF output
(Phase 34) as the form the PDF path embeds. Until that lands, charts
and barcodes reach the PDF through `image_scale` at an explicit dpi —
a stopgap named in Phases 23 and 27, not the architecture. The raster
path is also the permanent home of photographs and a user's own
imported art.

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
  in this plan stay under the cap. Raising it is operator config.

**Note:** `ClientInvokeMaxParamsBytes` (default 256 KiB, confirmed in
`config_scripting.c:47` and `conduit/script/script.c:39`) is a separate cap
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
needs no new C. Raising it does **not** lift
`ClientInvokeMaxResultBytes` (1 MiB), which is a separate key and still
governs every stage's output. A datasource or a rendered output past
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
a PDF writer can insert the streams. lua-pdfkit today accepts JPEG and
PNG, not XO. Phase 21 teaches the port to embed XO, which is why that
format exists. Until that lands, a PNG from the same endpoint still
embeds through `doc:image`.

**SVG input and the raster paths.** `image_scale` documents SVG as a
supported input, with `dpi` applied before the read so rasterization
honors it, and `png` as a supported output. That one fact carries the
whole PNG emit (Phase 32), and it is also where a user's own imported
art goes. A user-supplied SVG placed as a static image is rasterized
this way; it does not enter the PDF as a vector until a later plan
extends the ports' native output to imported art.

**Image element options.** Photographs are the one part of a report
that is unavoidably raster, so the image item carries its own options,
chosen in the designer: `dpi` (default 72 for PDF, the designer can
raise it), `format` (`png` or `xo`), a downsample cap in pixels, and a
reuse rule. The reuse rule matters more than it sounds: an image that
repeats on every detail row would otherwise be stored once per row. An
image element resolves to one stored object per unique source, placed
as many times as the item appears. Phase 21 confirms lua-pdfkit's
image caching does that, and if it does not, `Reports.Render` keeps a
per-render cache keyed by a content hash of the source. Stored once,
at a suitable resolution, is the rule for every embedded image.

**Where the bytes come from.** All three source kinds are v1, and they
are all resolved to bytes at the start of generation, before any item
is placed:

| kind | Stored on the item | Resolved how | Trade-off |
| --- | --- | --- | --- |
| `embedded` | Base64 data URI in the definition, deduplicated in a content-hash-keyed registry | Already in hand | Portable, one file; makes the definition big |
| `reference` | A document id | A QueryRef reads `documents` (`acuranzo_1010.lua`: `(doc_id, rev_id)`, `file_data TEXT_BIG`, `file_name`) | Swapping the document changes the report without editing it; the report is no longer portable alone |
| `url` | A URL | `H.http.request_sync` at the start of generation, the same call the endpoint step uses | Live and small; the URL must stay available, and it slows every generation |

The designer's image browser loads a file from the local filesystem
or takes a paste from the clipboard, and both become `embedded`. A
`reference` picker lists documents through the Phase 15 read QueryRef.
A failed reference or an unreachable URL is a warning and an empty
image box, the same contract as the endpoint's errors.

Which kind an author picks is a size decision. Embedded base64 is the
reason a definition can outgrow the 256 KiB params cap; reference and
url kinds keep the definition small. Phase 18 previews an over-cap
definition by saving it first and rendering by `report_id` — the
params cap does not have to stop the author, it just decides the path.

Lua calls the endpoint with `H.http.request_sync` using the operator's
bearer JWT in the `Authorization` header. There is no `H.system_token`
in the Hydrogen scripting API — `H.system_token` was a mistake in the
first draft. The JWT comes from the user's Lithium session (the
conduit query helper already attaches it). The base URL is read from
the process's API prefix, not hard-coded. No new C wrapper is added.

The designer and the PDF emitter use `pt` (1/72 inch), matching
`DefaultDPI` 72. The on-screen designer stays at 96 CSS pixels per
inch for the unzoomed view. Those are different conversions of the
same physical unit.

`POST /api/reporting/image_scale` returns:
`{ "success": true, "image": "<base64>", "format": "png",
"mime_type": "image/png", "width": N, "height": N,
"input_format": "png", "input_dimensions": { "width": N, "height": N } }`.
Errors return HTTP 400/401/503 with `{ "success": false, "error": "...",
"details": "..." }`. On any non-200, `Reports.Render` logs a warning
and yields an empty image box.

### Call shape

```json
{
  "script": "Reports.Render",
  "params": {
    "stage": "html",
    "definition": { "format": 1 },
    "sources": { "orders": [{ "customer": "North" }] }
  },
  "wait": true,
  "timeout_seconds": 15
}
```

A stored report may send `report_id` and `rev_id` instead of
`definition`. The script loads the row through the Phase 15 QueryRef.
`stage` is optional and defaults to `pdf`. Client params stay under
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
| `parameters` | JSON object. v1 stores `{}`. |
| `scripts` | Unused in v1. Store `[]`. Author Lua lives in `report`. |
| `design` | Unused. Leave null. The old column name is ambiguous and this pass found no writer. |
| `thumbnail` | Unused in v1. |
| `collection` | Standard audit JSON. The designer does not read it. |

### Designer rules

- No new npm package for v1, except `@codemirror/lint` in Phase 28.
  Pointer events do the dragging and the resize. Zoom is a CSS
  variable on the canvas.
- Palette and property editors read lookups 053, 054, and 057.
  Captions and icons come from those rows. The file stores keys.
- Sample datasource JSON in the designer is session state. It is how
  field items show a column name. It is not written into the
  definition.
- Import and export of the definition file work before the database
  save exists. The file is the artifact Phase 4 already proves.
- Split files before 1000 lines. Expected split: `reports.js` (shell),
  `reports.html`, `reports.css`, `canvas.js`, `rulers.js`, `grid.js`,
  `properties.js`, `functions.js`, `chart-panel.js`, and
  `model/units.js`, `model/document.js`, `model/history.js`.
- **DOMPurify is in `package.json`** (v3.3.2) but unused as of the last
  review. Preview HTML is sanitized with DOMPurify before assignment to
  a   sandboxed iframe `srcdoc` (Phase 18). Barcode SVG must survive that
  sanitizer (Phase 23). This is the one place HTML from a script crosses
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
in the lockfile transitively. Phase 28 adds it to the direct
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
fail when the line runs, and Phase 28 turns that into a warning and
an empty value.

### v1 render behavior

Given a definition and a source array, compose:

- emits a static report when `sources` is empty and the detail band
  has no datasource
- repeats the detail band once per row
- substitutes field items from the row
- prints system date and system user from `H.system` and
  `params._hydrogen` when those items exist
- breaks pages on fixed heights
- opens and closes one group level when a group band is present

Phase 17 implements that list. User formulas (Phase 28), primitive
aggregates (Phase 29), and custom breaks (Phase 30) extend it.
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
      and Page Count are new lookup 053 keys 61 and 62. (See Q1.)
- [x] 0.4 Confirm Vanadium Sans (and any other face) is added to
      lookup 054's font list in the Phase 11 packet. (See Q2.)
- [x] 0.5 Confirm `Reports.Render` stages `pdf` (default), `html`,
      `svg`, `png`, `csv`, `compose`, and `check`, and that
      lua-pdfkit, lua-zint, and d3.lua are seeded as `require`
      libraries (`lua-pdfkit.*` with group `lua-pdfkit`, `zint.*`
      with group `zint`, and `require("d3.lua")` with group `d3`
      and script name `lua`), `invokable = 0`, one seed phase and
      probe per library.
- [x] 0.6 Confirm the export order PDF, HTML, SVG, PNG, CSV, with
      PDF as the default stage. Fixed band heights. Lua does not
      measure glyphs. CSV is data only and stands in for a
      spreadsheet. HTML is both an export and the preview medium.
      SVG is a document export, PNG is its per-page raster through
      `image_scale`, and a label-sized report is a bare file.
- [x] 0.7 Confirm no new Hydrogen C. Image sizing stays on
      `POST /api/reporting/image_scale`. PNG emit uses that
      endpoint's documented SVG input. PDF embeds the `xo` result
      once lua-pdfkit can, and PNG until then. Charts and barcodes
      reach the PDF through the ports' native PDF output, with no
      SVG conversion. The one new direct npm dependency is
      `@codemirror/lint` for the formula editor.
- [x] 0.8 Confirm this plan waits behind CATCHUP until Andrew pulls
      it in, and that Phase 18 may add `invokeScript` if CATCHUP
      Phases 7–10 have not.
- [x] 0.9 Confirm manager 24, append-only revisions, sandboxed
      preview, the 256 KiB param cap, and the 1 MiB result cap.
      A PDF that fits is base64 inside that JSON. A PDF that does
      not fit returns an error naming the cap, and raising
      `ClientInvokeMaxResultBytes` is the v1 answer; a stored
      output returned by `H.set_result` with a location is later
      work.
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
      panel rather than lookup 054 rows, and Phase 25 as the chart
      design lock.
- [x] 0.12 Confirm the library homes. Each port lives in its own
      repository with its tests and porting notes, the seed packet
      records the commit or version, and LuaRocks is a possibility
      only if the sandbox can read `package.path`.
- [x] 0.13 Confirm image elements carry `dpi`, `format`, downsample,
      and reuse options, and store once per unique source.
- [x] 0.14 Confirm the phase order: render content, then the bare
       format emits, then CSV.
- [x] 0.15 Confirm the image source model: embedded, reference, and
       url are all v1, resolved at the start of generation, with the
       definition's base64 registry deduplicated by content hash.
- [x] 0.16 The effort column in the phase table matches the phases as
       scoped, and Phase 34's split (see its note) is what ships.
- [x] 0.17 The 2026-10-10 amendments are applied to this file.
      Re-read the amended locks and confirm they match the approval.
      (See Q3.)

### Done means

The locks in this file are the ones Andrew approved.

### Exit gate

Andrew's explicit approval of Phase 0. No source edited.

**Status: complete — approved 2026-10-10** (2026-10-08; fact-check pass
2026-10-10; plan-review incorporation pass 2026-10-10)

Work items 0.1, 0.2, and 0.5–0.17 are confirmed and checked off. All
three open questions were resolved during this session:

- **Q1 (Phase 11 lookup family split):** Helium AGENTS.md states "One
  lookup family per migration." Lookups 053 (Report Object Types) and 057
  (Report Page Sizes) are distinct families. Phase 11 splits into two
  migrations: one for 053 (icon fixes + attribute updates + page-number
  keys) and one for 057 (label fix + new page sizes).
- **Q2 (Allocation of lookup 053 keys 61/62):** Confirmed the max `key_idx`
  in lookup 053 is 60 (highest row in `acuranzo_1086.lua`), so keys 61 and
  62 are free. Page Number = key 61, Page Count = key 62. These join the
  v1 type allowlist at entry point.
- **Q3 (Phase 0.17 amendment sign-off):** Approved by Andrew on 2026-10-10.
`ClientInvokeMaxParamsBytes` (256 KiB) and `ClientInvokeMaxResultBytes`
(1 MiB) are both real and confirmed in `config_scripting.c`/`config_defaults.c`.
`H.http.request_sync` has a 16 MiB default body cap. DOMPurify is present in
`package.json` but unused. These corrections do not change the approved locks;
they remove the "verify the cap" uncertainty.

The 2026-10-10 scope refinement pass scoped exports to PDF, SVG, and CSV only;
HTML is a preview helper, not an export. Scheduling and permissions were
confirmed as separate modules that do not block the writer. Queries can
supply parameters as well as datasources.

The 2026-10-10 format amendment supersedes the paragraph above. Exports are
PDF (default), HTML, SVG, PNG, and CSV, in that implementation order; HTML is
both an export and the preview medium. Charts and barcodes are v1, with a
chart design lock of their own. Each Lua port gets its own seed phase and
lives in its own repository. The zint and d3 ports emit native PDF output, so
no SVG is ever converted to PDF. The port's SVG input at
`POST /api/reporting/image_scale` carries the PNG emit. `H.set_result(type,
location)` is not capped and is the shape a stored-output path will use once a
store exists; the gcode upload endpoint is not that store. This amendment
reflects Andrew's answers to the phase-0 questions and still awaits his
sign-off on the amended locks.

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
  ImageMagick endpoint, including the XO format meant for PDF.
  The script result cap is 1 MiB.
- Author formulas are source in the definition, compiled with
  `load` into a small environment. `require` remains the loader for
  lua-pdfkit and lua-zint. Built-in aggregates are engine ops.
  `avg` and `mean` are the arithmetic mean.

---

## Phase 1 — Contracts and fixtures

### Goal

Write the format version 1 contracts down as documents and checked-in
JSON fixtures, so later phases test against files.

### Entry gate

Phase 0 Status complete.

### Work items

- [ ] 1.1 Add a contract note under `docs/Li/` that restates the
      approved definition, intermediate, HTML, and call shapes.
      Absolute links. No line-number references.
- [ ] 1.2 Add `elements/003-lithium/tests/fixtures/reporting/` with
      one static definition, one detail definition, one grouped
      definition, matching source JSON, the expected intermediate
      for each, and a short expected HTML skeleton for the detail
      case.
- [ ] 1.3 Point this plan's Phase 2, 17, 20, and 31 at those paths.
       Chart fixtures land in Phase 25.

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

Replace the placeholder with a manager that loads a fixture and
downloads the definition file.

### Entry gate

Phase 3 Status complete.

### Work items

- [ ] 4.1 `reports.html` / `reports.css` / `reports.js`. Fetch the
      HTML template. `templates:copy`. Stay under 1000 lines.
- [ ] 4.2 `init`, `destroy`, and `cleanup`. Both destroy paths
      release listeners. Drop the "under development" copy.
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

### Done means

Manager 24 round-trips the detail fixture through a file. Lint and
unit tests pass.

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
      control that Phase 12 will replace) redraws the page.
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
- [ ] 7.3 Spacing follows the unit string. Default from the fixture
      is `0.125 in`.
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

- [ ] 10.1 Add a static text item to the selected band. Default box
       `1.5 in` by `0.2 in`, centered in the band.
- [ ] 10.2 Drag moves `left` and `top`. Corner resize changes `width`
       and `height`. Commit on pointerup through the model. Clamp to
       the band.
- [ ] 10.3 Selection draws an inset outline. Delete removes the item.
       Undo restores position and existence.
- [ ] 10.4 The item shows its text. v1 text is the item's `text`
       property, edited in Phase 12. Until then a new item reads
       "Text".
- [ ] 10.5 Browser: add, drag, resize, undo, export. The file has
       `typeKey` 0 and unit strings.

### Done means

A static text item survives export, import, and undo.

### Exit gate

Lint, unit tests for the clamp, and the browser check.

### Status

**not started**

---

## Phase 11 — Lookup repair packet

### Goal

Hand Andrew a migration that fixes the v1 lookup rows. Do not apply
it.

### Entry gate

Phase 0 Status complete. This phase may be prepared while Phases 1–10
are still open, and it is applied before Phase 12 starts. Re-check
disk the day the file is numbered.

**Lookup family note:** Helium AGENTS.md states "One lookup family per
migration." Lookup 053 (Report Object Types) and lookup 057 (Report Page
Sizes) are distinct families. Phase 11 is split into two migrations:

- Migration A (lookup 053): icon fixes for keys 7 and 16, attribute list
  updates for static text and field items, and insertion of Page Number
  (key 61) and Page Count (key 62) — the keys Phase 0 approved.
- Migration B (lookup 057): label fix for key 1, insertion of Legal
  portrait, A4, and Tabloid rows, and font list additions to lookup 054
  (Vanadium Sans plus any faces Phase 0 named). Lookup 054 is updated as
  a separate family migration if the rule requires it; otherwise it is
  bundled with A only if the rule permits multiple families per packet.
  Re-check the rule the day the files are numbered.

### Work items

- [ ] 11.1 Re-read
       [`/docs/He/GUIDE.md`](/docs/He/GUIDE.md) and the highest
       Acuranzo file, QueryRef, and lookup key. Take the next file
       number. Do not edit 1086, 1087, or 1090 in place.
- [ ] 11.2 Two forward migrations (Phase 0 Q1 decided: split). Migration
       A updates the broken 053 icons (keys 7 and 16), inserts page-number
       keys 61 and 62 (Page Number, Page Count), and updates v1 attribute
       lists so static text and field items include position, font, text
       or field, and datasource as Phase 0 requires. Migration B fixes the
       057 key 1 label and inserts the missing page sizes (Legal portrait,
       A4, Tabloid). Neither packet adds chart or barcode attributes:
       those types have none in lookup 054 today and their options live in
       bespoke panels. Reverse restores the previous JSON and deletes only
       the inserted keys.
- [ ] 11.3 Font list gains the faces Phase 0 named. Existing faces
       stay, so an old row still resolves.
- [ ] 11.4 Hand the packet over. Record the file number in this
       phase's Status after Andrew reports the apply.

### Done means

Andrew reports the packets applied. Lookup 053 keys 7 and 16 have `icon`.
Lookup 053 has new keys 61 (Page Number) and 62 (Page Count). Lookup 057
has a correctly labeled Letter landscape row and the new page sizes.
Lookup 054 font list gains Vanadium Sans. v1 attribute lists include the
properties Phase 12 will edit.

### Exit gate

Andrew's apply report. Test 31 and Test 98 if he ran them. The agent
does not mark this complete from a clean local lint alone.

### Status

**not started**

---

## Phase 12 — Property panel

### Goal

Edit the selected object from lookup 054, limited to the attribute
list on its lookup 053 row.

### Entry gate

Phase 11 Status complete, including the apply.

### Work items

- [ ] 12.1 Load lookups 053, 054, and 057 through the existing lookup
        cache. Read `src/shared/lookups.js` first and follow its shape.
   - **Gap:** `lookups.js` currently fetches `[1, 30, 53, 54, 60]` and
     stores 053 as `themes` and 054 as `icons`. It does **not** fetch
     lookup 057 (`page_sizes`), and there is no accessor for it. Add 057
     to the batch query and expose a `getPageSizes()` accessor so the page
     size picker in 12.3 has data. Rename the category comments for 053
     and 054 from "Themes/Icons" to "Report Object Types" and "Report
     Object Attributes" (or add aliases); the stale names are confusing
     for anyone reading the plan.
- [ ] 12.2 The panel lists the attributes advertised for the selected
       `typeKey`. Editors follow lookup 054's `Editor` value for the
       v1 set: text, number, list, boolean. An unknown editor toasts
       and skips that row.
- [ ] 12.3 Edits commit through the model. Undo reverts the panel and
       the canvas. Page size picks a lookup 057 row and copies its
       width, height, and margins into the definition.
- [ ] 12.4 Browser: change the static text, the font size, and the
       page size. Export. Import into a fresh session and see the
       same panel values.

### Done means

The properties the operator can see are the properties in the file,
and they come from the lookups.

### Exit gate

Lint, unit tests for "attribute list filters the panel", and the
browser check against a migrated database (the new lookup rows have
to be visible).

### Status

**not started**

---

## Phase 13 — Field items and a sample datasource

### Goal

Place a field and show which column it is bound to.

### Entry gate

Phase 12 Status complete.

### Work items

- [ ] 13.1 A session-only sample JSON datasource. The operator pastes
       or imports an array of objects. Column names are the union of
       keys. This JSON is not part of the definition export.
- [ ] 13.2 Add field text, number, and datetime items. The property
       panel's Field editor lists those column names. Datasource
       names come from the definition's `datasources` array.
- [ ] 13.3 The canvas label is the field name. An empty binding shows
        the type caption and a toast when the operator exports, and
        export is refused for that item. Compose will warn later. The
        designer does not invent a value. (Per `LITHIUM-INS.md` rule 1,
        no silent fallback — the operator must pick or fill a binding.)
- [ ] 13.4 Browser: bind `customer`, export, confirm `field` and
       `typeKey` 20, reload, and see the binding.

### Done means

A field item round-trips, and the sample data never appears in the
exported definition. Empty bindings block export with a toast.

### Exit gate

Lint, unit tests, and the browser check.

### Status

**not started**

---

## Phase 14 — One group level

### Goal

Add a group header and footer tied to one field of the detail
datasource.

### Entry gate

Phase 13 Status complete.

### Work items

- [ ] 14.1 The group header stores `field`. The footer shares that
       grouping. A second group level is refused with a toast.
- [ ] 14.2 The canvas shows the three bands in order: group header,
       detail, group footer.
- [ ] 14.3 Browser: set the group field, export, undo.

### Done means

The grouped fixture's shape can be built from the UI and the file
matches the Phase 1 grouped definition's keys.

### Exit gate

Lint, unit tests, and the browser check.

### Status

**not started**

---

## Phase 15 — Save and load packet

### Goal

QueryRefs that list the latest revision, read one revision, and
insert a new revision. Hand them over. Do not apply them.

### Entry gate

Phase 0 column lock still matches this file. Re-check numbers the
day the files are written. Phases 4–14 may still be open. Phase 16
waits for the apply.

### Work items

- [ ] 15.1 Read a recent QueryRef seed (the 1260 / 1108 pattern in
       [`/docs/He/GUIDE.md`](/docs/He/GUIDE.md)) and
       [`acuranzo_1015.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1015.lua)
       before writing SQL.
- [ ] 15.2 Three migrations, one QueryRef each, `TYPE_SQL` so the SPA
        can call them. List latest revision per `report_id`. Get one
        `(report_id, rev_id)`. Insert the next `rev_id` for a name and
        a definition. Insert allocates `report_id` the way a sibling
        insert migration allocates its id. Copy that pattern. Do not
        invent one.
- [ ] 15.3 A fourth QueryRef reads one document image by `doc_id`:
        `file_data` and `file_name` from `documents`
        ([`acuranzo_1010.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1010.lua)).
        The report writer's `reference` image kind resolves through it
        in Phase 21, so the picker and compose share one read. If the
        documents table shape has changed by the day this is written,
        re-read it before numbering.
- [ ] 15.4 Reverse deletes the QueryRef row only.
- [ ] 15.5 Hand the packets over. Record the QueryRef integers in
        this phase's Status after Andrew reports the apply.

### Done means

Andrew reports the three QueryRefs applied, and a manual call returns
the row he inserted.

### Exit gate

Andrew's apply report. The agent does not mark this complete on Test
31 alone.

### Status

**not started**

---

## Phase 16 — Revisions in the manager

### Goal

Save the open definition as a new revision and reopen it.

### Entry gate

Phase 14 and Phase 15 Status complete.

### Work items

- [ ] 16.1 List view of latest revisions via the list QueryRef.
       Opening a row loads that revision into the document model.
- [ ] 16.2 Save appends a revision. It does not update the open
       `rev_id` in place. The canvas then shows the new revision.
- [ ] 16.3 A failed call toasts the conduit error and keeps the
       document dirty.
- [ ] 16.4 Browser: save, reload the manager, open the row, and see
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

## Phase 17 — `Reports.Render` compose and HTML

### Goal

Seed one invokable script that composes the intermediate document and
emits the HTML export. The HTML export is also the preview medium.

### Entry gate

Phase 1 fixtures exist. Phase 15 is applied if the script loads a
stored id. The fixture call, which passes `definition` inline, can be
written before Phase 15. Re-check the next file number.

### Work items

- [ ] 17.1 Copy the `scripts` insert shape from
       [`acuranzo_1376.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1376.lua).
       Group `Reports`, name `Render`, `invokable = 1`,
       `mcp_access = 0`.
- [ ] 17.2 `stage=compose` implements the structural render behavior
       (the bullet list under v1 render behavior). User formulas,
       primitive aggregates, custom breaks, barcodes, and charts are
       Phases 23, 27, 28, 29, and 30. Output matches the Phase 1
       intermediate fixtures for the static, detail, and grouped
       cases.
- [ ] 17.3 `stage=html` emits one section per page with
       `break-after: page`, the substituted text, and positions in
       physical CSS units. `stage` defaults to `pdf`; the preview and
       the HTML export both request `html` explicitly, so the bytes
       Andrew previews are the bytes he exports.
- [ ] 17.4 Missing datasource, unknown `typeKey`, and empty `field`
       become warnings on the result. The script still returns a
       document. A `format` other than 1 returns an error through
       `H.set_result_json` and does not throw out of the worker.
- [ ] 17.5 Hand the packet over. After apply, Andrew runs the three
       fixture calls and the results match the fixture files.

### Done means

Andrew reports the three fixture calls matched. The script is
`Reports.Render`.

### Exit gate

Andrew's apply report and the three calls. Test 98 on the new
migration if he ran it.

### Status

**not started**

---

## Phase 18 — Preview

### Goal

The designer runs `Reports.Render` and shows the composed pages in a
sandboxed iframe. The preview is the HTML export artifact: the same
stage, the same bytes. The designer and the preview are not co-visible,
so rendering the whole report is a deliberate action, never a re-render
chasing every drag.

### Entry gate

Phase 17 Status complete. The open definition's datasources are the
session sample from Phase 13, or an empty object for a static report.

### Work items

- [ ] 18.1 If `src/shared/conduit.js` still has no `invokeScript`,
         add it for `POST /api/conduit/script` (JWT-bearer, params shape
         per the call contract above) and cover it with a unit test.
         If CATCHUP already added it, call that function. Note that
         `conduit.js` currently exposes `query`/`authQuery` only — there
         is no `invokeScript` helper today.
- [ ] 18.2 Preview requests `stage=html` with the current definition
         and the session sample. DOMPurify runs on the HTML. The result
         is assigned to a sandboxed iframe `srcdoc`. Because preview
         and export are the same stage, the preview matches the export
         by construction, not by a second renderer.
- [ ] 18.3 A render button renders the whole report. Whether auto-render
         on change is worth the round trips is decided by the browser
         trial in 18.5 — if it wins, it debounces, and it never fires
         while the pointer is dragging or resizing.
- [ ] 18.4 Errors and warnings from the script toast. The canvas
        stays as it was.
- [ ] 18.5 Browser: preview the detail fixture with two sample rows
        and see two detail bands. Preview a report whose sample is
        missing and see the warning.

### Done means

The operator can preview the report the Lua script produced.

### Exit gate

Lint, the conduit unit test, and the browser check against an applied
`Reports.Render`.

### Status

**not started**

---

## Phase 19 — Seed lua-pdfkit

### Goal

Put the pure-Lua PDFKit port into `scripts` so `require` loads it.
No report output yet. The source is the port's own repository, and the
packet records the commit or version it was generated from.

### Entry gate

Phase 0 Status complete. Re-check the next Acuranzo file number the
day this is written. Phases 1–18 may still be open. Phases 20–21 wait
for the apply.

### Work items

- [ ] 19.1 Copy the tree named in the library lock into one migration.
       One migration seeds every `lua-pdfkit.*` module (`init`,
       `document`, `page`, `path`, `image`, `font.afm`, …).
       `invokable = 0`, `mcp_access = 0`. Group name `lua-pdfkit`. The
       script name is the require path after the first dot. One library
       per file, the exception to one-script-per-migration. Reverse
       deletes only those rows.
- [ ] 19.2 Confirm a standard face (`Helvetica`) renders with `io` nil.
       If the port still `io.open`s an AFM file, bundle that AFM text
       into the seeded module before handing the packet over.
- [ ] 19.3 Hand the packet over. After apply, Andrew runs a one-page
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

## Phase 20 — PDF emit

### Goal

`stage=pdf` returns a PDF of the composed pages. This is the default
stage and the highest-priority export. Text only; images, barcodes,
and charts join in Phases 21, 23, and 27.

### Entry gate

Phase 19 Status complete.

### Work items

- [ ] 20.1 Amend `Reports.Render` in a new migration that replaces
       the script body the way a sibling script-update migration
       does. Re-read one before writing. Do not edit the Phase 17
       file after it has been applied.
- [ ] 20.2 `PDFDocument.new` uses the definition page size. One
       `addPage` per intermediate page. Text uses a standard face
       the port already carries. Positions are in PDF points.
- [ ] 20.3 The JSON result is `{ "pdf_base64": "...", "warnings": [] }`.
       Write the result-cap contract into the docs: a PDF that does not
       fit `ClientInvokeMaxResultBytes` returns an error naming the cap
       and the actual size, compose having already run. Confirm the
       conduit response path has no second, smaller body cap, and say
       so in the contract note.
- [ ] 20.4 A fixture pair: detail definition, source, expected PDF page
       count. The fixture PDF is measured against the cap.
- [ ] 20.5 Hand the packet over. Andrew's call for the detail fixture
       opens as a PDF with the right page count.

### Done means

The detail fixture renders to a PDF that opens with the right page
count, and the result-cap behavior is written down.

### Exit gate

Andrew's apply report and the fixture call.

### Status

**not started**

---

## Phase 21 — Images through image_scale

### Goal

A static image and a field image are sized by Hydrogen and shown in
the HTML export, the preview, and the PDF, with per-element options and
a store-once rule.

### Entry gate

Phase 18 and Phase 19 Status complete. `Reporting.Enabled` is true
on the server Andrew uses for the gate.

### Work items

- [ ] 21.1 Palette shows lookup 053 keys 2 (Static Image) and 22
       (Field Image). The item stores its source per the Images
       section (`embedded` base URI, a `doc_id` reference, or a URL),
       the box, and the image options: `dpi`, `format`, a downsample
       cap in pixels, and the reuse rule.
- [ ] 21.2 The designer's image browser loads a local file or takes a
       clipboard paste, and both become `embedded`. The reference
       picker lists documents through the Phase 15 read QueryRef.
       Embedded bytes dedupe into the definition's content-hash
       registry, so a repeated image costs one copy.
- [ ] 21.3 `Reports.Render` resolves all three kinds to bytes at the
       start of generation: references through the Phase 15 QueryRef,
       URLs through `H.http.request_sync` with the JWT, before any
       item is placed. A failed reference or unreachable URL is a
       warning and an empty box.
- [ ] 21.4 `stage=html` and `stage=pdf` then call
       `POST /api/reporting/image_scale` with `units=pt`, the box size,
       and the item's format. An error from the endpoint is a warning
       and an empty box.
- [ ] 21.5 The HTML path uses the returned image (`result.image`
       base64) as a data URI inside the item box. DOMPurify keeps
       that image.
- [ ] 21.6 Confirm lua-pdfkit stores one image object per unique
       source. If the port does not, `Reports.Render` keeps a
       per-render cache keyed by a content hash of the source, so a
       repeated image is stored once and placed many times. Say which
       shipped in the Working Log.
- [ ] 21.7 Teach the port to embed the endpoint's `xo` payload, or
       record in the Working Log that PDF images ship as PNG through
       `doc:image` and why.
- [ ] 21.8 Fixture image stays under the 1 MiB result cap after
       base64. Browser: a sample PNG lands at the box size, and a
       reference source and a URL source each render once their bytes
       resolve.

### Done means

The preview shows the scaled image, the call went through
`image_scale`, all three source kinds resolve, and a repeated image
stores once.

### Exit gate

The script update applied, the fixture call, and the browser check.
Reporting and the Phase 15 QueryRefs were enabled for that check.

### Status

**not started**

---

## Phase 22 — Seed lua-zint

### Goal

Put the pure-Lua zint port into `scripts` so `require` loads it. No
report output yet. The source is the port's own repository, and the
packet records the commit or version it was generated from.

### Entry gate

Phase 0 Status complete. Re-check the next Acuranzo file number the
day this is written. Phases 1–21 may still be open. Phases 23 and 34
wait for the apply.

### Work items

- [ ] 22.1 Copy the tree named in the library lock into one migration.
       One migration seeds every `zint.*` module (`init`, `barcode`,
       `common`, …). `invokable = 0`, `mcp_access = 0`. Group name
       `zint`. The script name is the require path after the first dot.
       One library per file, the exception to one-script-per-migration.
       Reverse deletes only those rows.
- [ ] 22.2 The port writes a file when `opts.output` is set, and the
       sandbox leaves `io` nil. `Reports.Render` leaves that field
       unset and uses the returned SVG string.
- [ ] 22.3 Hand the packet over. After apply, Andrew runs a probe:
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

## Phase 23 — Barcodes through lua-zint

### Goal

Static and field barcodes render as SVG in the HTML export, the
preview, and the SVG export, and as vector objects in the PDF once
Phase 34 lands.

### Entry gate

Phase 22 Status complete.

### Work items

- [ ] 23.1 Palette shows lookup 053 keys 43 (Static Barcode) and 45
       (Field Barcode). The item stores `symbology` as one of
       `qr`, `code39`, `code128`, `upca`, `gs1-128`, plus the data
       string or the field name. Other symbologies are refused with a
       warning.
- [ ] 23.2 Compose calls `zint.barcode` with no `output` path. The
       SVG inlines into the item box in the HTML preview, the HTML
       export, and the SVG export.
- [ ] 23.3 DOMPurify is configured so this SVG survives the preview
       sandbox. A stripped SVG is a failed gate, not a pass.
- [ ] 23.4 Browser: a Code 128 of a known string and a QR of a known
       string appear in the preview. A field barcode follows the
       sample row.
- [ ] 23.5 The PDF path: until Phase 34 lands the port's native
       output, the barcode SVG rasterizes through `image_scale` at the
       item's `dpi` (default 300, which scans fine) and embeds as a PNG.
       Phase 34 replaces this with the native vector object. Either
       way, what shipped is named in the Working Log.

### Done means

Those two symbols render from Lua and survive the preview sanitizer,
and a Code 128 appears in the PDF by whichever path shipped.

### Exit gate

Applied script update, fixture call, and the browser check.

### Status

**not started**

---

## Phase 24 — Seed d3.lua

### Goal

Put the pure-Lua D3 port into `scripts` so `require` loads it. No
report output yet. The source is the port's own repository, and the
packet records the commit or version it was generated from.

### Entry gate

Phase 0 Status complete. Re-check the next Acuranzo file number the
day this is written. Phases 1–23 may still be open. Phases 25–27 and
34 wait for the apply.

### Work items

- [ ] 24.1 Copy the module named in the library lock into one
       migration. `invokable = 0`, `mcp_access = 0`. Hydrogen's
       searcher splits on the first dot, so the group and module names
       must produce a require path with a dot; `require("d3.lua")` is
       the working hypothesis and the probe pins it. Reverse deletes
       only those rows.
- [ ] 24.2 The demo writes its SVG with `io.open`, and the sandbox
       leaves `io` nil. Confirm the module returns its SVG as a string
       with no file written, the way `zint` leaves `opts.output`
       unset.
- [ ] 24.3 The regression oracle: the port ships a reference pair — the
       D3.js chart (`d3_chart.js`) and the Lua chart (`lua_chart.lua`)
       — whose SVGs are byte-identical. After apply, Andrew's probe
       reproduces the reference SVG from inside Hydrogen, and that
       string is recorded as the seeded library's checksum.
- [ ] 24.4 Hand the packet over. The probe also draws one chart of
       each locked kind (Phases 25 and 27 depend on those kinds
       existing in the port, minus pie if Phase 25 defers it).

### Done means

Andrew reports the reference SVG reproduced byte-for-byte inside
Hydrogen, with `io` left off, plus one SVG per chart kind.

### Exit gate

Andrew's apply report and the probe. Test 98 on the new migration
if he ran it.

### Status

**not started**

---

## Phase 25 — Chart design lock

### Goal

Agree the chart contract before the designer and the Lua grow around
it. This is a mini design lock: the taxonomy, the scope model, and the
v1 set. No source edits.

### Entry gate

Phase 0 Status complete. This phase may run alongside Phases 19–24.

### Work items

- [ ] 25.1 Lock the chart kinds for v1: bar, stacked-bar, line,
       dual-axis, combinations (a stacked bar with an overlaid line is
       one of the fixture cases, not a special case), and pie. The
       series-first model in Charts is the rule; `kind` stays as
       optional shorthand only.
- [ ] 25.2 Lock the options hierarchy: chart kind, then one or more
       series datasources, then per-chart options (title, legend,
       legend position, axes, grid, gradient, rounded bars) and
       per-series options (label, role, stack, axis side, color,
       weight). Adding a series is how an author also picks that
       series' format.
- [ ] 25.3 Lock the scope model as written in Charts. All four scopes
       ship in v1: `band`, `group`, `record`, and `custom`.
- [ ] 25.4 Decide pie. The port has no arc generator. Either the port
       is extended with one — in the port's repo, recorded as a
       version the seed packet pins — or pie is pinned behind that
       extension and the rest of the v1 set ships without it.
- [ ] 25.5 Fixtures: one stacked bar with an overlaid line, one
       grouped bar, one dual-axis line, and pie or its deferral note.

### Done means

The chart contract in this file is the one Andrew approved.

### Exit gate

Andrew's explicit approval. No source edited.

### Status

**not started**

---

## Phase 26 — Charts in the designer

### Goal

Place a chart on a band, add its series, bind each series to data,
and set the chart's options, with the series-first flow Phase 25
locked.

### Entry gate

Phase 25 Status complete.

### Work items

- [ ] 26.1 Palette shows lookup 053 keys 42 (Static Chart) and 44
       (Field Chart). The item stores a `chart` object per the locked
       contract, plus its box.
- [ ] 26.2 `chart-panel.js` walks the options hierarchy: the chart's
       options first, then an add-series row. Each series names its
       datasource, field, role, stack, axis, and label — picking a
       series is also picking that series' format. A kind shorthand
       fills the common cases in one move.
- [ ] 26.3 The panel follows the property panel's commit rules:
       through the document model, undo, export, import. Options are
       designer state until commit; the browser does not draw the
       chart.
- [ ] 26.4 An empty series binding blocks export with a toast, the
       same as a field item (LITHIUM-INS.md rule 1 — no silent
       fallback).
- [ ] 26.5 Browser: place a chart, add a bar series and a line series
       over the session sample, set the dual axis, export, reload, and
       see the bindings.

### Done means

A chart item round-trips through the definition file with its series
and options.

### Exit gate

Lint, unit tests, and the browser check.

### Status

**not started**

---

## Phase 27 — Charts in compose

### Goal

Compose resolves a chart's scope, calls d3.lua with the resolved rows
and the item's box, and puts the chart into the intermediate document,
so it appears in the HTML export, the preview, and the SVG export.

### Entry gate

Phase 17, Phase 24, and Phase 26 Status complete. A chart in the PDF
rasterizes through `image_scale` at the chart's `dpi` until Phase 34
replaces that with the port's native output.

### Work items

- [ ] 27.1 Resolve the scope per the locked model. `band` and `group`
       use the rows compose already has. `record` filters the named
       datasource on the current record's `keyField`. `custom` runs a
       function from `definition.functions`.
- [ ] 27.2 The `custom` scope function compiles with the same `load`
       contract as User Lua (Phase 28). This phase lands that compile
       for the chart scope only; the detail formula context is
       Phase 28's.
- [ ] 27.3 d3.lua draws with the resolved rows, the `categoryField`,
       the series definitions, and the box in points. The returned SVG
       is stored on the intermediate as the chart item's render. The
       intermediate never re-derives the chart.
- [ ] 27.4 The HTML export and the preview inline the SVG, and
       DOMPurify keeps it — the same gate as barcodes in 23.3. The SVG
       export (Phase 31) inlines the same string.
- [ ] 27.5 Fixtures from Phase 25: the stacked bar with an overlaid
       line, the grouped bar, the dual-axis line, and pie or its
       deferral note. One fixture per scope, including a `record` chart
       and a `custom` chart.
- [ ] 27.6 Hand the script update over. After apply, Andrew's compose
       call for each fixture matches the expected intermediate, and the
       preview shows the charts.

### Done means

Each chart fixture renders in the preview and matches the expected
intermediate document.

### Exit gate

Applied script update, fixture call, and the browser preview.

### Status

**not started**

---

## Phase 28 — User Lua in the definition

### Goal

An author stores a Lua function on the report, and a calculated item
runs it against the current record.

### Entry gate

Phase 13, Phase 17, and Phase 18 Status complete. lua-pdfkit,
lua-zint, and d3.lua stay on `require`. Author formulas do not.

### Work items

- [ ] 28.1 Implement the User Lua contract. Compile every function at
       the start of compose. A compile error returns through
       `H.set_result_json` and does not throw out of the worker. A
       runtime error warns and yields an empty value.
- [ ] 28.2 Palette shows lookup 053 keys 28 and 29. The item stores
       the function name. The function panel is the formula editor
       locked above: CodeMirror 6, `language: 'lua'`, Vanadium Mono,
       footer, and `@codemirror/lint`. Source is saved on the
       definition when the editor blurs. The browser does not run
       the Lua.
- [ ] 28.3 `stage=check` compiles the posted functions with the same
       `load` path as compose and returns `{ name, line, message }`
       for each failure. The editor debounces that call for the open
       function and underlines the line. Browser: type `function(ctx`
       with no `end`, see the diagnostic, close the chunk, and see
       the diagnostic clear.
- [ ] 28.4 Fixture function `pack_code`, a stand-in for a grower pack
       date. Letters `A` through `L` are months 1 through 12. The
       return value is that letter, the week digit, and the day as
       two digits. Row `month=3`, `week=2`, `day=5` renders `C205`.
       A second row raises a runtime error and comes back empty with
       a warning. An unknown function name is a compose error.
- [ ] 28.5 Hand the script update over. After apply, the fixture call
       matches. Browser: change the letter table, preview, and see
       the new text on the detail. The repaired `pack_code` still
       renders `C205`.

### Done means

The pack-code fixture renders `C205`, the bad row warns, and the
designer round-trips the source in the definition file. A broken
chunk shows a CodeMirror diagnostic and, once repaired, previews.

### Exit gate

Andrew's apply report, the fixture call, lint, and the browser
preview. The document model stores the source and does not execute
Lua. Unit tests cover that storage if a pure helper exists.

### Status

**not started**

---

## Phase 29 — Primitive aggregates

### Goal

Group footer and report footer can show the built-in reductions of
one field.

### Entry gate

Phase 14 and Phase 17 Status complete.

### Work items

- [ ] 29.1 Aggregate items store `op` and `field`. `weighted` also
       stores `weight`. Ops are the set in Primitive aggregates.
       Numeric results use lookup 053 key 35. A text mode uses key 34.
- [ ] 29.2 Compose computes the value for the current group and for
       the whole detail source, and places it on `ctx.aggregates`.
       A non-numeric value in a numeric op warns and is skipped. An
       empty numeric result is an empty string. `count` and `unique`
       of an empty scope are 0.
- [ ] 29.3 The designer can place the item on a group footer or a
       report footer and pick the op.
- [ ] 29.4 Fixture: two groups. One has an odd count and a single
       mode. A second case covers an even median and a mode tie.
       `avg` and `mean` match. Andrew's compose call matches after
       the script update is applied.

### Done means

The grouped fixture's footers show each op's expected value in the
intermediate document and in the HTML preview.

### Exit gate

Lint, the script packet apply, and the fixture call. Aggregate math
lives in the script. The document model stores `op` and does not
recompute it. Preview checked in the browser.

### Status

**not started**

---

## Phase 30 — Custom breaks and band context

### Goal

A Lua function can supply the group key, force a page break, or
insert a blank line. A calculated item on a header or footer sees
that band's context.

### Entry gate

Phase 28 and Phase 14 Status complete. When Phase 29 is already
complete, footer formulas can read `ctx.aggregates`. This phase's
fixture does not require that.

### Work items

- [ ] 30.1 A group band may set `groupKey` to a function name. The
       return value is the key. `sort` defaults to true. `sort`
       false breaks only when consecutive keys differ. One level.
- [ ] 30.2 A detail band may set `pageBreak`. A true result starts a
       new page before that row. Fixed-height paging still applies.
- [ ] 30.3 A detail band may set `lineBreak`. A true result inserts
       one blank detail-height before that row. Newlines in a
       calculated value are preserved by the HTML and PDF emitters.
       The band height does not grow.
- [ ] 30.4 Calculated items on a group, page, or report header or
       footer receive the context in the User Lua table.
       `page.count` is filled after compose finishes the page list,
       before those formulas run.
- [ ] 30.5 Fixture: group by the pack-code month letter, start a new
       page when `ctx.row.break_before` is true, and insert a blank
       line when `ctx.row.blank_before` is true. A group footer
       formula reads `ctx.group.key`. Andrew's compose call matches.
- [ ] 30.6 Designer: the group band can choose a function as its key,
       and the detail band can name the two break functions.
       Browser: preview the fixture and see the group split, the
       extra page, and the blank line.

### Done means

The fixture intermediate document shows the Lua group split, the
forced page, and the blank line.

### Exit gate

Script packet apply, fixture call, lint, and the browser preview.

### Status

**not started**

---

## Phase 31 — SVG emit

### Goal

`stage=svg` returns one SVG document per report: the composed pages
as sections with page breaks marked, in physical units. A label-sized
report with no pagination is this file unwrapped — page size is the
label size, margins are zero, and the document is the bare artwork.

### Entry gate

Phase 17 Status complete. Barcodes and charts inline from Phases 23
and 27 when those have landed; a text-only fixture otherwise.

### Work items

- [ ] 31.1 Amend `Reports.Render` with `stage=svg`. One page per
       section, positions in physical CSS units, the definition's font
       face, and page breaks marked per the Emit contract.
- [ ] 31.2 Chart and barcode items inline their SVG from the
       intermediate document. Images inline as data URIs at their
       scaled size.
- [ ] 31.3 A fixture pair: detail definition, source, expected SVG
       skeleton. The intermediate fixture stays the oracle for the
       text and the structure.
- [ ] 31.4 Hand the packet over. Andrew's call matches the fixture,
       and a browser check embeds the SVG and shows the pages.

### Done means

The detail fixture renders to the expected SVG, and it embeds in a
page as one image with the page breaks marked.

### Exit gate

Applied script update, fixture call, and the browser check.

### Status

**not started**

---

## Phase 32 — PNG emit

### Goal

`stage=png` returns one PNG per page, through `image_scale`, which
takes SVG as input and applies `dpi` before it reads. The result is
the label case: one page, one image, no pagination.

### Entry gate

Phase 31 Status complete.

### Work items

- [ ] 32.1 Amend `Reports.Render` with `stage=png`. Compose renders
       the page SVG, and each page goes to
       `POST /api/reporting/image_scale` with `units=px`, the page box,
       a `dpi` param (default 150 for output, the item's own dpi for
       a chart), and `format=png`.
- [ ] 32.2 The result is
       `{ "pages": [{"page": 1, "png_base64": "..."}], "warnings": [] }`.
       The label fixture is the one-page case and is the fixture this
       phase gates on.
- [ ] 32.3 Multi-page PNG lists every page against the same 1 MiB
       result cap. A report whose pages do not fit returns an error
       naming the cap, the same contract as PDF in Phase 20, and the
       note in the contract doc says so.
- [ ] 32.4 Hand the packet over. Andrew's call for the label fixture
       returns one PNG at the label size.

### Done means

The label fixture renders to one PNG at the label's physical size,
rasterized through `image_scale`.

### Exit gate

Applied script update, fixture call, and the browser check.

### Status

**not started**

---

## Phase 33 — CSV emit

### Goal

`stage=csv` returns the data of the report: one row per detail
record, text-compatible fields only. No charts, no barcodes, no
images.

### Entry gate

Phase 17 Status complete.

### Work items

- [ ] 33.1 Amend `Reports.Render` with `stage=csv`. Columns are the
       field items on the detail band, in item order. Header and
       footer bands are not rows. A datetime item exports in the
       format its item options carry; a number exports unformatted
       unless the item carries a mask.
- [ ] 33.2 Decide the grouping representation and write it into the
       contract note. The recommendation is flat rows with the group
       key repeated as leading columns, because that is what a
       spreadsheet consumer wants; the alternatives are group header
       rows or blank-line-separated sections. Whichever is chosen,
       `avg`/`mean`-style aggregates do not become CSV rows.
- [ ] 33.3 Values are escaped per RFC 4180 unless the work item above
       picks a looser text format and says why. A newline inside a
       value is quoted, not split.
- [ ] 33.4 A fixture pair: detail definition, source, expected CSV.
- [ ] 33.5 Hand the packet over. Andrew's call matches the fixture.

### Done means

The detail fixture renders to the expected CSV, and the grouping
representation is written down.

### Exit gate

Andrew's apply report and the fixture call.

### Status

**not started**

---

## Phase 34 — PDF-native output from the zint and d3 ports

### Goal

Charts and barcodes enter the PDF as the vector objects the two ports
emit natively. No SVG is converted to PDF anywhere, and nothing is
rasterized to get there.

### Entry gate

Phase 20, Phase 23, and Phase 27 Status complete. This phase extends
the ports in their own repositories, so it is the one phase whose
library work lands upstream of the packet.

### Work items

**Scope note:** as written this is three sittings, not one, because it
touches three repositories. The session ordering is: (a) agree the
contract and extend one port, with its probe; (b) the second port,
same shape; (c) the lua-pdfkit placement plus the `Reports.Render`
amendment and both fixture sets. Split it at those lines rather than
by port, so the contract is settled before either port is written
against it.

- [ ] 34.1 Agree the native-output contract with both ports. Each
       port gains a PDF emit that returns the report's chart or
       barcode as a Form-XObject-ready payload — a bounding box, the
       resources it needs, and a content stream — that lua-pdfkit can
       place with the machinery it already has (`Page:xobjects()`,
       `Document:ref`, the path runners). The exact shape,
       `zint.barcode_pdf(opts)` and the d3 module's chart emit, is
       pinned at the port, not guessed at here.
- [ ] 34.2 Extend lua-pdfkit where the payload needs it: a placeable
       vector resource with a matrix, and the shared resources the
       two payloads reference (standard faces for labels, the
       gradient patterns the D3 chart uses).
- [ ] 34.3 Amend `Reports.Render`: chart and barcode items in
       `stage=pdf` call the native emits and place the objects. The
       SVG paths keep their SVG from the same compose, unchanged.
- [ ] 34.4 Fixtures: the chart fixtures and the barcode fixtures from
       Phases 23 and 27, now as PDFs. Andrew opens each and the
       chart and the barcode are vector — selectable text on the axis
       labels, crisp bars at any zoom.
- [ ] 34.5 Record in the Working Log which port versions the packet
       pins, and that the seed packet regenerates against them.

### Done means

A chart PDF and a barcode PDF open with vector objects the ports
emitted, and the PDF path no longer rasterizes any SVG.

### Exit gate

Applied script updates, the fixture PDFs, and Andrew's confirmation
they are vector.

### Status

**not started**

---

## Later, not scheduled

Pull one of these into a numbered phase only by amending this file.

| Topic | Why it waits |
| --- | --- |
| Memos and shapes | Each is a `typeKey` with its own box behavior. Images are Phase 21. |
| Other barcode keys | Phase 23 places lookup 053 keys 43 and 45. Aggregate, calculated, and crosstab barcode keys stay hidden. |
| Chart kinds beyond the v1 set | Pie is in v1 behind the port extension. Anything past bar, stacked bar, line, dual-axis, combos, and pie is a port extension plus a panel pass. |
| Chart interactivity | Hover, zoom, and tooltips need scripted SVG, and the exports are static files. |
| Crosstabs | A second layout engine. |
| Subreports and packages | Nested definitions and their own datasources. |
| Multiple group levels | v1 is one level. The key is a field, or one Lua function from Phase 30. |
| Related datasources | v1 is one list. A link between two lists is a new contract. |
| Stored blobs past the caps | A datasource over 256 KiB, or a PDF or PNG set whose base64 JSON exceeds the 1 MiB result cap. v1 answers both by operator config: `ClientInvokeMaxParamsBytes` and `ClientInvokeMaxResultBytes`. The stored-output shape is `H.set_result(type, location)`, which is not capped — but Hydrogen has no generic file store yet, and the gcode upload endpoint is not one. Raising the caps is operator config. |
| Parameter form | The `parameters` column is reserved and empty. |
| Native XLS | CSV is Phase 33. A spreadsheet writer waits until that path is the boring one. The retired client loaded SheetJS in the browser. |
| Thumbnails | `thumbnail` stays null until a preview snapshot has a home. |
| Text measurement and stretching bands | Needs a font engine shared by compose and the designer. |
| Vanadium in PDF | The v1 PDF face is Helvetica. Embedding the product face needs the font bytes inside the sandbox, with `io` left off. |
| Live QueryRef datasources | A datasource `kind` of `query` that runs a QueryRef and uses the rows. Deferred per discussion — a query can also supply a parameter value, not just a datasource. |
| Mail and scheduled runs | Mail Relay and the script scheduler. The report plan only has to leave `Reports.Render` invokable. Scheduling is a separate Lithium module covering reports, adhoc queries, and emails — not baked into the report writer itself. |
| Scheduling and recurring runs | Handled by the separate scheduling module (see Mail and scheduled runs). `Reports.Render` is invokable, but there is no scheduler UI or `scripts` schedule row in this plan. |
| Report permissions and sharing | Who can generate or view which report. Hydrogen JWT auth exists; a row-level ACL model is not defined here. This waits on the permissions pass; the report writer does not block on it. |
| Text search and filtering of report output | Full-text search over rendered HTML/PDF, or filtering the detail set by a text query. v1 datasources are plain JSON arrays. |
| Incremental and delta reports | "Rows changed since the last run." Needs a watermark column on the datasource and a "last run" marker. v1 is a full snapshot. |
| Conditional formatting | Text color, background, or font overrides driven by a field value or a formula. A style rule is a condition plus a set of property changes. v1 renders unstyled values. |
| Data filtering | Exclude or include detail rows by a condition before compose assigns pages. A single filter expression on the datasource, applied before grouping. v1 renders every row. |
| Detail row sorting | Order the detail rows within a band by one or more keys before compose lays them out. v1 preserves datasource order. |
| Template inheritance | A base definition that another definition extends, overriding bands or items by id. v1 definitions are standalone. |

## Working log

### 2026-10-08 — First pass

Surveyed Lithium manager 24 (placeholder, id confirmed in
`manager-loader.js` and `lithium.json`), Helium `acuranzo_1015`,
`acuranzo_1086`, `acuranzo_1087`, and `acuranzo_1090`, and the retired
client at `/mnt/extra/Projects/acuranzo/`.

The compiled designer (`AcuranzoClient_6_1_5045.js`) contains
`getReportPixels`, `addBackgroundPattern`, `createRuler`, section
resize, item drag, lookup-filtered properties, and JSON undo. The
source zip does not contain the Pascal units. No write to the
`reports` table turned up in that build. PDF viewing was pdf.js, and
the page loaded `jsPDF`.

The first reading of the scripting API was that it can return JSON
and run `H.query`, and that it had no PDF library. Conduit script
params default to a 256 KiB cap. The same-day amendment below
replaces the PDF and module conclusions.

Phase 0 is not approved. No product source was edited for the report
writer in this pass. Index pointers were added so the next session
can find this file.

### 2026-10-08 — Lua PDF, barcodes, and image scale

Andrew pointed at two pure-Lua ports that are not in this repo.

- lua-pdfkit lives at
  `/home/asimard/DA/triton4/triton-toolkit-5b12870/tasks/lua-pdfkit/solution/lua-pdfkit/`.
  `PDFDocument.new`, then `doc:finish()`, then `doc:output()` returns
  the PDF bytes. JPEG and PNG come from raw bytes or a `data:` URI.
  File paths and some AFM loads call `io.open`. Emit does not turn
  `Sandbox.AllowIo` on.
- lua-zint lives at
  `/home/asimard/DA/triton3/triton-toolkit-e8fb24a/tasks/lua-zint/solution/`.
  `zint.barcode(opts)` returns SVG. The port draws QR, Code 39,
  Code 128, UPC-A, and GS1-128. `opts.output` writes a file, so
  `Reports.Render` leaves that field unset.

Hydrogen `require("group.script")` splits on the first dot
(`H_lua_package_searcher` in `scripting_api_scoreboard.c`). The
entry points that fit are `lua-pdfkit.init` and `zint.barcode`.
`require("lua-pdfkit")` has no dot and is rejected. Each library is
one migration, many script rows, `invokable = 0`, `mcp_access = 0`.

Image sizing is the shipped Reporting endpoint
`POST /api/reporting/image_scale` (ImageMagick / MagickWand),
including format `xo` for a PDF XObject. Lua calls it with
`H.http.request_sync` using the operator's bearer JWT, the same
header that conduit query calls already attach. No new C. Phase 23 prefers `xo`
and falls back to PNG plus `doc:image`. HTML barcodes stay inline
SVG. PDF barcodes are rasterized through the same endpoint.
Designer scale stays 96 CSS pixels per inch. Emit and `image_scale`
use points (72 per inch).

CSV is the spreadsheet emit (Phase 19). A native XLS writer stays
unscheduled. `H.set_result_json` defaults to 1 MiB, so a PDF inside
that JSON is about three quarters of a megabyte. Fixtures stay
under the cap.

Phases 20–23 are the library seed, images, barcodes, and PDF.
This entry sketched calculated Lua as `require` and Count and Sum
as Phase 25. The next entry replaces that. Phase 0 items 0.5–0.7
and 0.9 were rewritten to match. Phase 0 is still awaiting
approval. No product source was edited.

### 2026-10-08 — Author Lua in the definition

Andrew described the report author as the person who writes the
function. Growers invent their own pack-date codes on PTI labels
(a letter for the month, a week digit, a day split their way). That
source has to travel with the report.

The lock is `definition.functions[]`, each entry a name plus a Lua
chunk that returns `function(ctx)`. `Reports.Render` compiles it
with `load(source, name, "t", env)` on Lua 5.5. The environment is
the string, math, table, and utf8 libraries, the usual pure
functions, and `os.date` / `os.time` under the names `date` and
`time`. The chunk does not receive the worker globals. `H` and
`require` stay in `Reports.Render`, which is what loads lua-pdfkit
and lua-zint. Hydrogen's sandbox logs `AllowOsExecute` and does not
yet strip `os.execute` (`lua_context.c`). The separate environment
is what keeps a formula away from it. No new C, and `debug` stays
off. The existing progress hook still enforces
`max_runtime_seconds`.

A detail formula receives the current record. Group, page, and
report headers and footers receive that band's context. Phase 26
adds a group-key function, a page-break predicate, and a line-break
predicate. The line-break predicate inserts one blank detail-height.
Newlines inside a value are kept by the emitter. Band height stays
fixed.

Primitive ops are count, unique, sum, avg, weighted, mean, mode,
and median. `avg` and `mean` are both the arithmetic mean. The
engine computes them. A footer formula can read `ctx.aggregates`.

Phase 24 is the field formula and the `C205` pack-code fixture.
Phase 25 is the primitive set. Phase 26 is breaks and band context.
Phase 0 item 0.10 asks for approval of this lock, and 0.11 is the
amend step. Phase 0 is still awaiting approval. No product source
was edited.

### 2026-10-08 — Formula editor

The function panel is a CodeMirror 6 view with the Lua personality
already in `src/core/codemirror.js` (`lua()`, a stream language).
Scripting manager builds its Lua buffer through
`buildEditorExtensions({ language: 'lua' })`. Reports uses that
path, including the shared footer, Vanadium Mono, brackets, and
comment continuation. The older `codemirror-init.js` wrapper is not
the one.

That personality highlights. It does not parse. Syntax diagnostics
are `@codemirror/lint`, added as a direct dependency in Phase 24
(it is already transitive in the lockfile). The messages come from
a new `Reports.Render` stage, `check`, which `load`s the chunk in
Lua 5.5 and returns the line and message. luacheck stays the repo
file linter. A JavaScript Lua parser would drift from the engine
that actually runs the report.

The editor debounces `check` and does not block typing. Report undo
commits the source on blur. Undefined names such as `io` or `H` are
runtime, so the gutter does not flag them while typing. Phase 0
item 0.7 now names `@codemirror/lint` as the one new direct
dependency. Phase 0 is still awaiting approval.

### 2026-10-10 — Fact-check corrections

Applied corrections from a codebase read pass against
`elements/001-hydrogen/hydrogen/src/`, `elements/002-helium/acuranzo/`,
and `elements/003-lithium/`:

- **`H.system_token` does not exist.** Replaced with `H.http.request_sync`
  carrying the operator's bearer JWT (the same header conduit query calls
  attach). Verified `H.http.request_sync` in the scripting API and in
  `argent_2034.lua`/`argent_2047.lua`; confirmed no `system_token`,
  `system-token`, or `SYSTEM_TOKEN` anywhere in `src/`.
- **`H.http.post` was wrong.** The real API is `H.http.request_sync`.
  Updated Phases 21.2 and the working log.
- **Lookup 057 gap in `lookups.js`.** The batch query at line 363 is
  `[1, 30, 53, 54, 60]` — lookup 057 (page sizes) is never fetched and
  has no accessor. Phase 12 now carries a work item to add 057 to the
  fetch and expose a `getPageSizes()` accessor.
- **Stale lookup category names.** `lookups.js` lines 10–15 comment
  lookup 053 as "Themes" and 054 as "Icons". Phase 12 notes the rename
  to "Report Object Types" / "Report Object Attributes".
- **`invokeScript` absent.** `conduit.js` exposes only
  `query`/`authQuery`. Phase 18.1 updated to state this explicitly.
- **LITHIUM-INS.md rule 1 violation.** Phase 13.3 no longer lets an
  empty field binding silently succeed export; it now refuses with a
  toast. Updated Done means accordingly.
- **`image_scale` return contract.** Added the documented success and
  error response shapes to the Images section.
- **D3.js is Hydrogen-only.** Confirmed `d3` is used solely by
  `hbm_browser`. No D3 port exists in Lithium or Helium. The plan
  defers charts to "Later, not scheduled." The upcoming D3.lua SVG
  port is noted as pending from the user; it is not in the repo yet.
- **zint / lua-pdfkit ports pending.** Neither library is in-repo; they
  live on Andrew's disk outside the repo. Phase 20 seeds them. Added
  status note and a note that SVG generators (zint + D3.lua) share an
  intermediate representation that the HTML path embeds directly and
  the PDF path rasterizes via `image_scale`.

### 2026-10-10 — Body cap and DOMPurify verification

- **`ClientInvokeMaxParamsBytes` is real.** Confirmed at 256 KiB default in
  `config_scripting.c:47` and `conduit/script/script.c:39`. This is the conduit
  script params cap, not the HTTP response limit. The 256 KiB reference in the
  plan is correct and was clarified to not be confused with the `request_sync`
  HTTP body limit (16 MiB, `SCRIPTING_HTTP_DEFAULT_MAX_BODY` in `http_client.h`).
- **`ClientInvokeMaxResultBytes` is real.** Confirmed at 1 MiB default in
  `config_scripting.h:62`. The PDF-in-JSON ceiling math holds.
- **DOMPurify is in `package.json`** (v3.3.2). It was present but unused as of
  the last Lithium review. Designer rules updated: Phase 18 sanitizes with
  DOMPurify before `srcdoc`; Phase 22 ensures barcode SVG survives.
- **Four expectation gaps** added to "Later, not scheduled": scheduling and
  recurring runs, report permissions and sharing, text search and filtering of
  report output, and incremental/delta reports.

### 2026-10-10 — Export format and scope refinements

- **Exports scoped to PDF, SVG, and CSV.** SVG is now the first emitted format
  (Goal 4, Emit table `stage=svg` as default). HTML is repositioned as a preview-
  only helper stage (`stage=html`) that consumes the same intermediate document
  as the SVG export. The HTML path stays for the sandboxed iframe preview; it is
  not an export format. Updated Phase 18 goal and work item 18.2, and Phase 22
  text to reflect SVG as the primary path.
- **Scheduling and permissions confirmed as separate.** The "Later, not scheduled"
  rows for scheduling and report permissions were clarified: scheduling is a
  separate Lithium module (covering reports, adhoc queries, and emails), not
  baked into the report writer; permissions waits on the ACL pass and does not
  block the writer.
- **Queries as parameter sources noted.** The "Live QueryRef datasources" row in
  "Later, not scheduled" now also notes that a query can supply a parameter
  value, not just a datasource.
- **DOMPurify rationale.** Added a note that even though the preview HTML is
  fully generated by `Reports.Render` with no external HTML surface, the
  sanitizer stays as defense-in-depth for the iframe.

### 2026-10-10 — Format, chart, and library amendment

Andrew's answers to the phase-0 review, applied to the locks and the
phase list. Supersedes the same-day "HTML is a preview helper" refinement.

- **Exports are PDF (default), HTML, SVG, PNG, and CSV**, in that
  implementation order. HTML is both an export and the preview medium;
  the preview is the HTML export artifact. SVG is a document export,
  PNG is the per-page SVG through `image_scale`, and a label-sized
  report with no pagination is a bare file.
- **Phases renumbered.** Render content lands before the bare format
  emits so no format is built twice: 17 compose + HTML, 18 preview,
  19 seed lua-pdfkit, 20 PDF, 21 images, 22 seed lua-zint, 23
  barcodes, 24 seed d3.lua, 25 chart design lock, 26 charts in the
  designer, 27 charts in compose, 28 user Lua, 29 aggregates, 30
  breaks, 31 SVG, 32 PNG, 33 CSV, 34 PDF-native port output. Excel
  stays unscheduled.
- **Charts are v1, series-first.** A chart item is lookup 053 key 42
  or 44, carries a `chart` object, and its shape falls out of per-series
  roles — a line beside a bar on two axes is the combo, first-class.
  All four scopes ship: `band`, `group`, `record`, `custom`. Chart
  options live in a bespoke panel because lookup 054 has no chart
  rows, which also removes any dependency on the Phase 11 packet.
- **PDFs run past the result cap.** Phase 20 writes the contract: a
  PDF that fits is base64 in the result JSON; one that does not returns
  an error naming `ClientInvokeMaxResultBytes` and the actual size, and
  raising the cap is operator config. `H.set_result(type, location)` is
  uncapped and is the shape a stored output will take, but there is no
  generic store yet — `/api/files/local` is gcode-only.
- **No SVG-to-PDF conversion, as the architecture.** The zint and d3
  ports gain a native PDF emit (a Form-XObject-ready payload: bbox,
  resources, content stream) that lua-pdfkit places directly. Phase 34
  owns that work, in the ports' own repos. Until it lands, charts and
  barcodes reach the PDF through `image_scale` at an explicit dpi — a
  stopgap named in Phases 23 and 27, because a label PDF without its
  barcode is worse than a sharp raster. Raster through `image_scale`
  is the permanent home of photographs and imported art.
- **Image elements carry options.** `dpi`, `format`, a downsample cap,
  and a reuse rule, so a repeated image stores once per unique source.
  Phase 21 confirms lua-pdfkit's image caching, or a content-hash cache
  in `Reports.Render`.
- **Library homes.** Each port is its own repository with its tests;
  the seed packet pins the version. LuaRocks matters only if the
  sandbox can read `package.path`.

Verified during this pass, against disk:

- The D3 port is real and done, not upcoming: two poseidon trials both
  scored 1.0, and the artifact patch is `d3.lua` plus `lua_chart.lua`
  and `d3_chart.js` with byte-identical SVGs. Its API is `create_root`,
  `scaleBand`, `scaleLinear`, `axisLeft/Right/Bottom`, `stack`,
  `curveBasis`, `line`, `range` — no pie arc yet. The demo writes with
  `io.open`, so the probe confirms an in-memory SVG path.
- lua-pdfkit already parses SVG path data (`path.lua` runners) and
  already builds Form XObjects (`gradient.lua`), with
  `Page:xobjects()` for the resource dict. The native emit is an
  extension, not a rewrite.
- `image_scale` documents SVG input, applies `dpi` before the read,
  and emits `png`. That carries Phase 32.
- Lookup 053 chart keys are 42/44/46/40/54 and barcode keys 43/45; no
  lookup 054 row describes either.
- `H.set_result` and `H.set_result_json` are distinct; only the JSON
  one is capped. No `H.*` file or blob API exists.

### 2026-10-10 — Image sources, cap raise, and effort sizing

Second amendment pass, same day. Answers two questions raised after
the format amendment, plus the effort column.

- **Three image sources, all v1.** An image item is `embedded`
  (base64 in the definition, deduplicated by content hash), a
  `reference` (a `doc_id`, read at generation time through a new
  Phase 15 QueryRef), or a `url` (fetched through
  `H.http.request_sync` before any item is placed). All three resolve
  to bytes at the start of generation. The `documents` table
  (`acuranzo_1010.lua`: `(doc_id, rev_id)`, `file_data TEXT_BIG`,
  `file_name`, `doc_type_a51`) is the reference target, which Andrew
  identified and which was confirmed on disk. The designer's image
  browser loads a local file or takes a clipboard paste into
  `embedded`.
- **Over-cap definitions preview by saving first.** Phase 18 saves
  the definition and previews by `report_id` when the inline params
  would not fit — one toast explaining the detour, no silent fallback.
- **The 256 KiB params cap is config-only to raise to 10 MB.** The
  enforcement is a length compare against
  `Scripting.ClientInvokeMaxParamsBytes` with no fixed buffer behind
  it, and the API POST body ceiling is already 10 MiB
  (`API_MAX_POST_SIZE`, `src/api/api_utils.h` — its "64KB default"
  comment is stale). The scoreboard strdups the params per job, so the
  cap is a memory dial too. Recommendation recorded: set it to
  10485760. It does not lift the separate 1 MiB result cap.
- **Effort column.** easy / medium / hard per phase, doubling as a
  model-strength hint. 28 of 35 phases are one sitting; the hard ones
  are hard for stated reasons, and Phase 34 carries a scope note
  splitting it into three sittings by contract, port, port, then
  placement.

Verified during this pass, against disk:

- `ClientInvokeMaxParamsBytes` is parsed from
  `Scripting.ClientInvokeMaxParamsBytes` (`config_scripting.c`), the
  checks live in `conduit/script/script.c` (raw and merged), and
  `params_too_large` already maps to HTTP 413.
- The `documents` table and the Phase 15 list/get QueryRefs that
  already touch it (acuranzo_1138, 1139) exist; a read-by-`doc_id`
  QueryRef is the new one.

Phase 0 is still not approved. No product source was edited for the
report writer in this pass.

### 2026-10-10 — Phase 0 completion

Andrew approved Phase 0. Three decisions were recorded:

- **Q1 resolved.** Phase 11 splits into two forward migrations per Helium
  AGENTS.md "one lookup family per migration" rule: Migration A touches
  lookup 053 (icon fixes, page-number keys 61/62, attribute list updates);
  Migration B touches lookup 057 (label fix, new page sizes). Lookup 054
  font additions ride with one of the two, depending on final rule reading.
  Phase 11 work items and Done means updated to match.
- **Q2 resolved.** Verified max `key_idx` in `acuranzo_1086.lua` is 60.
  Page Number = key 61, Page Count = key 62. v1 type allowlist updated.
- **Q3 resolved.** The 2026-10-10 amendments (exports PDF/HTML/SVG/PNG/CSV;
  charts v1 series-first; no SVG-to-PDF conversion; three image sources;
  10 MB params cap via config) are approved as written.

Phase 0 Status set to complete. No source was edited for the report writer
in this pass; the plan is now the gate for Phase 1.

### 2026-10-10 — Plan review incorporation

Incorporated all items from the review session:

- **Phase 0.5 wording fixed.** Clarified that `require("d3.lua")` means
  group `d3` and script name `lua`, matching Hydrogen's first-dot-split
  searcher in `scripting_api_scoreboard.c:293`. `lua-pdfkit.*` is group
  `lua-pdfkit`; `zint.*` is group `zint`.
- **Phase 0.5-0.17 checked off** for all items confirmed by the
  2026-10-10 fact-check pass. Items 0.3, 0.4, and 0.17 remain open
  pending user decisions (see Q1–Q3 below).
- **Phase 15.3 QueryRef overlap resolved.** Read `acuranzo_1138.lua`
  (QueryRef #047 "Get Documents"): it returns all documents with no
  `doc_id` filter. Phase 15.3 needs a new read-by-`doc_id` QueryRef,
  which is a distinct query — no conflict.
- **Phase 11 lookup family noted.** Helium AGENTS.md says "One lookup
  family per migration." Phase 11 touches both lookup 053 and 057.
  Added an entry-gate note pending the Q1 decision on whether to split.
- **LITHIUM-INS.md §4 resolved.** The Anti-Patterns section (line 301)
  explicitly allows `element.style.property = value` for "dynamic
  positioning (e.g., drag handles)." Phase 6's
  `element.style.height` drag preview is compliant.
- **Phase 34 effort sizing confirmed.** 28 of 35 phases are one sitting;
  Phase 34 splits into three sittings by its scope note. No change.
- **v2 features added** to "Later, not scheduled": conditional
  formatting, data filtering, detail row sorting, and template
  inheritance.
- **Open questions:** See Q1 (Phase 11 lookup family split), Q2 (Page
  Number/Count keys 61/62), and Q3 (Phase 0.17 amendment sign-off).
