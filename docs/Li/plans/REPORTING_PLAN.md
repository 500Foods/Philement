<!-- markdownlint-disable MD007 MD024 MD013 -->

# Reporting Plan

**Date:** 2026-10-08
**Status:** Phase 0 draft. Awaiting approval. No Lithium source, Helium
migration, or Hydrogen C has been written for this plan.
**Home:** Manager **24**, Report Manager, already registered.
**Design:** Acuranzo (shared product database). Lookups 0–199. Migrations
1000–1999.

This is the gated plan for a report writer. Lithium is the designer. The
designer writes a JSON report definition. A Helium Lua script reads that
definition plus zero or more JSON datasources, builds an intermediate
document, and emits a format. HTML is the first format. PDF comes from
the pure-Lua PDFKit port, barcodes from the pure-Lua zint port, and
image sizing from Hydrogen's existing ImageMagick endpoint. CSV is the
spreadsheet export. Authors store their own Lua functions in the
definition. The same script compiles those functions and runs them.

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

**Pause point (2026-10-08):** Phase 0 is a proposal. Nothing is approved.
The next session re-reads the locks, amends them with Andrew, and stops.
Do not open `src/managers/reports/` for implementation in that session.

## Priority

| | |
| --- | --- |
| **Band** | P2. Starts when Andrew pulls it in. CATCHUP remains P0. |
| **Effort** | XL across the phases. Each phase is meant to be one sitting. |
| **Done** | 0%. This document is the first pass. |
| **Why now** | Hydrogen's host work is close to done. The report writer is the next product surface, and the lookups and table already exist. |
| **Do not start casually** | The JSON contract has to be agreed before the designer and the Lua script both grow around it. |

## Phases

| Phase | What | Kind | Status |
| --- | --- | --- | --- |
| 0 | Design lock | Decision | Draft |
| 1 | Definition, intermediate, and emit contracts | Docs + fixtures | Not started |
| 2 | Units and geometry | JS + Vitest | Not started |
| 3 | Document model and undo | JS + Vitest | Not started |
| 4 | Manager shell, import, export | Lithium UI | Not started |
| 5 | Page surface | Lithium UI | Not started |
| 6 | Bands and resize | Lithium UI | Not started |
| 7 | Section grid | Lithium UI | Not started |
| 8 | Rulers and the mouse line | Lithium UI | Not started |
| 9 | View scale | Lithium UI | Not started |
| 10 | Static text | Lithium UI | Not started |
| 11 | Lookup repair packet | Helium, human apply | Not started |
| 12 | Property panel | Lithium UI | Not started |
| 13 | Field items and a sample datasource | Lithium UI | Not started |
| 14 | One group level | Lithium UI | Not started |
| 15 | Save and load packet | Helium, human apply | Not started |
| 16 | Revisions in the manager | Lithium UI | Not started |
| 17 | `Reports.Render` compose + HTML | Helium seed, human apply | Not started |
| 18 | Preview | Lithium UI | Not started |
| 19 | CSV emit | Helium seed amendment | Not started |
| 20 | Seed lua-pdfkit and lua-zint | Helium library pack | Not started |
| 21 | Images through `image_scale` | Lua + designer | Not started |
| 22 | Barcodes through lua-zint | Lua + designer | Not started |
| 23 | PDF emit through lua-pdfkit | Lua | Not started |
| 24 | User Lua in the definition | Helium + designer | Not started |
| 25 | Primitive aggregates | Helium + designer | Not started |
| 26 | Custom breaks and band context | Helium + designer | Not started |

Later work is listed after the phases. It is not scheduled.

## Scope

| Tree | Role in this plan |
| --- | --- |
| `elements/003-lithium/src/managers/reports/` | Manager 24. Shell, canvas, rulers, properties |
| `elements/003-lithium/src/managers/reports/model/` | Pure units, document, undo. Vitest imports these |
| `elements/003-lithium/tests/unit/` | Tests for the model and, later, `invokeScript` |
| `elements/003-lithium/tests/fixtures/reporting/` | Definition, datasources, expected intermediate, expected HTML |
| `docs/Li/` | Contract note written in Phase 1. This plan stays the gate list |
| `elements/002-helium/acuranzo/migrations/` | Lookup repair, QueryRefs, `Reports.Render`, lua-pdfkit, lua-zint |
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
4. HTML as the first emitted format, including a page break between
   composed pages.
5. PDF from lua-pdfkit, barcodes from lua-zint, and photographs sized
   by `POST /api/reporting/image_scale`. Hydrogen does not grow a PDF
   or barcode library.
6. CSV as the tabular download. A native spreadsheet writer waits.
7. Report authors write Lua functions in the definition JSON.
   `Reports.Render` compiles and runs them. A field formula receives
   the current record. A header, footer, or break receives the
   context for that band.
8. The existing lookup families stay the catalog of object types,
   attributes, and page sizes.
9. Built-in aggregates: count, unique, sum, avg, weighted mean,
   mean, mode, and median.

## Out of scope until a later plan

- Charts, crosstabs, subreports, packages, shapes, and memos. The
  lookup rows can stay. The palette hides them until a later phase.
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

Page number and page count join this list if Phase 0 adds lookup keys.
Rulers, Report Top, and Report Bottom are canvas chrome. They are not
bands in the file. Grid and ruler settings on the definition are view
defaults, stored so the next session opens the same page.

`functions` holds the author's Lua. The shape is in the next section.
An early definition stores `[]`.

A group band names one field or, from Phase 26, one Lua function that
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

Breaks, implemented in Phase 26:

- `groupKey` names a function. Its return value is the group key.
  The one-field group remains the path with no Lua. One level.
- `pageBreak` names a function. A true result starts a new page
  before the current row. Fixed-height paging still applies.
- `lineBreak` names a function. A true result inserts one blank
  detail-height before the current row. A newline inside a
  calculated value is kept by HTML (`white-space: pre-line`) and by
  the PDF text drawer. The stored band height does not change.
  Overflow clips.

The pack-date fixture in Phase 24 is an invented pattern (month
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
An empty numeric result is an empty string. Phase 25 places the
results on `ctx.aggregates` for footer formulas, keyed by op and
field name.

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
| `compose` | Intermediate JSON via `H.set_result_json` |
| `html` | Compose, then HTML. Default. |
| `csv` | Phase 19. One row per detail record. |
| `pdf` | Phase 23. Base64 PDF bytes inside the JSON result. |
| `check` | Phase 24. Compile the posted functions and return diagnostics. No rows are read. |

HTML is a document with one `<section class="page">` per intermediate
page and CSS `break-after: page`. The preview puts that string in a
sandboxed iframe after DOMPurify. The parent page does not assign it
to `innerHTML`. Barcode SVG has to survive that sanitizer (Phase 22).

`Reports.Render` is one invokable row. lua-pdfkit and lua-zint are
other rows, loaded with `require("group.script")`. Author formulas
are not rows. Hydrogen's DB searcher splits on the first dot: group,
then the rest of the name as `script_name`. `AllowDBModuleLoad` has
to be on for those two libraries. `require("lua-pdfkit")` is rejected
because it has no dot. The entry points are `lua-pdfkit.init` and
`zint.barcode`.

### Lua libraries

Both ports are pure Lua. They are not in this repo. They were written
to run inside a host that has no native modules and no `io`.

| Library | Where it lives today | What Reports.Render calls |
| --- | --- | --- |
| lua-pdfkit | `/home/asimard/DA/triton4/triton-toolkit-5b12870/tasks/lua-pdfkit/solution/lua-pdfkit/` | `PDFDocument.new`, then `doc:finish()`, then `doc:output()` for the bytes. Port of PDFKit 0.19.1. JPEG and PNG from raw bytes or a `data:` URI. |
| lua-zint | `/home/asimard/DA/triton3/triton-toolkit-e8fb24a/tasks/lua-zint/solution/` | `zint.barcode(opts)` returns an SVG string. Symbologies in the port: QR, Code 39, Code 128, UPC-A, GS1-128. |

The sandbox nils `io` unless `Sandbox.AllowIo` is set. Emit does not
turn that on. lua-pdfkit writes the document with `output()` and never
needs a file. Passing `opts.output` to zint would call `io.open`, so
Reports.Render leaves that field unset and uses the returned string.
Image and font paths inside lua-pdfkit do call `io.open`. The report
path passes image bytes, and the first PDF faces are the standard
ones carried as AFM data. A phase that needs `io` stops and asks.

`H.set_result_json` is capped by `ClientInvokeMaxResultBytes`
(default 1 MiB). A text report fits. A PDF is base64 inside that JSON,
so the ceiling is about three quarters of a megabyte of PDF. Fixtures
in this plan stay under the cap. Raising it is operator config.

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
PNG, not XO. Phase 23 teaches the port to embed XO, which is why that
format exists. Until that lands, a PNG from the same endpoint still
embeds through `doc:image`.

Lua reaches the endpoint with `H.system_token` and `H.http.post`. The
phase reads the base URL from what the process already exposes. It
does not add a C wrapper and it does not hard-code a host. The
designer and the PDF emitter use `pt` (1/72 inch), matching
`DefaultDPI` 72. The on-screen designer stays at 96 CSS pixels per
inch for the unzoomed view. Those are different conversions of the
same physical unit.

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
Client params stay under `ClientInvokeMaxParamsBytes` (256 KiB unless
the operator changed it). A fixture in this plan fits. A larger
datasource is a later phase that stores the JSON and passes an id.

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

- No new npm package for v1, except `@codemirror/lint` in Phase 24.
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
  `properties.js`, `functions.js`, and `model/units.js`,
  `model/document.js`, `model/history.js`.

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
in the lockfile transitively. Phase 24 adds it to the direct
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
fail when the line runs, and Phase 24 turns that into a warning and
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

Phase 17 implements that list. User formulas (Phase 24), primitive
aggregates (Phase 25), and custom breaks (Phase 26) extend it.
Emit HTML reproduces the page list. The same intermediate fixture is
the oracle for both the Lua result and the preview.

## Phase 0 — Design lock

### Goal

Agree the locks above. No source edits.

### Entry gate

This file is the plan under discussion.

### Work items

- [ ] 0.1 Confirm format version 1 stores lookup `key_idx` and unit
      strings, and that the retired label-keyed JSON is evidence, not
      the file we write.
- [ ] 0.2 Confirm the `reports` column roles, including `design` and
      `thumbnail` left unused.
- [ ] 0.3 Confirm the v1 type allowlist, and whether Page Number and
      Page Count are new lookup 053 keys.
- [ ] 0.4 Confirm Vanadium Sans (and any other face) is added to
      lookup 054's font list in the Phase 11 packet.
- [ ] 0.5 Confirm `Reports.Render` stages `compose`, `html`, `csv`,
      and `pdf`, and that lua-pdfkit and lua-zint are seeded as
      `require` libraries (`lua-pdfkit.*`, `zint.*`), `invokable = 0`.
- [ ] 0.6 Confirm HTML first, then images, barcodes, and PDF. Fixed
      band heights. Lua does not measure glyphs. CSV stands in for
      a spreadsheet.
- [ ] 0.7 Confirm no new Hydrogen C. Image sizing stays on
      `POST /api/reporting/image_scale`. PDF embeds the `xo` result
      once lua-pdfkit can, and PNG until then. The one new direct
      npm dependency is `@codemirror/lint` for the formula editor.
- [ ] 0.8 Confirm this plan waits behind CATCHUP until Andrew pulls
      it in, and that Phase 18 may add `invokeScript` if CATCHUP
      Phases 7–10 have not.
- [ ] 0.9 Confirm manager 24, append-only revisions, sandboxed
      preview, the 256 KiB param cap, and the 1 MiB result cap.
      A PDF is base64 inside that JSON, so fixtures stay smaller.
- [ ] 0.10 Confirm author Lua lives in `definition.functions` and
      runs in the environment in User Lua. A detail formula sees
      the current record. Headers, footers, and breaks see the band
      context. Confirm the primitive ops, with `avg` and `mean` as
      the same arithmetic mean, and a line-break predicate that
      inserts one blank detail-height. The formula editor is
      CodeMirror 6 with the in-repo Lua personality, and syntax
      diagnostics come from `stage=check`.
- [ ] 0.11 Amend this file so the locks match the approval.

### Done means

The locks in this file are the ones Andrew approved.

### Exit gate

Andrew's explicit approval of Phase 0. No source edited.

### Status

**draft — awaiting approval** (2026-10-08)

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
- [ ] 1.3 Point this plan's Phase 2 and Phase 17 at those paths.

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

### Work items

- [ ] 11.1 Re-read
       [`/docs/He/GUIDE.md`](/docs/He/GUIDE.md) and the highest
       Acuranzo file, QueryRef, and lookup key. Take the next file
       number. Do not edit 1086, 1087, or 1090 in place.
- [ ] 11.2 One migration. Forward updates the broken 053 icons and
       the 057 key 1 label, and inserts the missing page sizes and
       any Phase 0 page-number keys. Update v1 attribute lists so
       static text and field items include position, font, text or
       field, and datasource as Phase 0 requires. Reverse restores
       the previous JSON and deletes only the inserted keys.
- [ ] 11.3 Font list gains the faces Phase 0 named. Existing faces
       stay, so an old row still resolves.
- [ ] 11.4 Hand the packet over. Record the file number in this
       phase's Status after Andrew reports the apply.

### Done means

Andrew reports the packet applied. Lookup 053 key 7 has `icon`.
Lookup 057 has a correctly labeled Letter landscape row and the new
page sizes. v1 attribute lists include the properties Phase 12 will
edit.

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
- [ ] 13.3 The canvas label is the field name. Empty binding shows
       the type caption and a toast when the operator exports, and
       the export still succeeds. Compose will warn later. The
       designer does not invent a value.
- [ ] 13.4 Browser: bind `customer`, export, confirm `field` and
       `typeKey` 20, reload, and see the binding.

### Done means

A field item round-trips, and the sample data never appears in the
exported definition.

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
- [ ] 15.3 Reverse deletes the QueryRef row only.
- [ ] 15.4 Hand the packets over. Record the QueryRef integers in
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

## Phase 17 — `Reports.Render`

### Goal

Seed one invokable script that composes the intermediate document
and emits HTML.

### Entry gate

Phase 1 fixtures exist. Phase 15 is applied if the script loads a
stored id. The fixture call, which passes `definition` inline, can
be written before Phase 15. Re-check the next file number.

### Work items

- [ ] 17.1 Copy the `scripts` insert shape from
       [`acuranzo_1376.lua`](/elements/002-helium/acuranzo/migrations/acuranzo_1376.lua).
       Group `Reports`, name `Render`, `invokable = 1`,
       `mcp_access = 0`.
- [ ] 17.2 `stage=compose` implements the structural render behavior
       (the bullet list under v1 render behavior). User formulas,
       primitive aggregates, and custom breaks are Phases 24–26.
       Output matches the Phase 1 intermediate fixtures for the
       static, detail, and grouped cases.
- [ ] 17.3 `stage=html` (default) emits one section per page with
       `break-after: page`, the substituted text, and positions in
       physical CSS units.
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

The designer runs `Reports.Render` and shows the HTML in a sandbox.

### Entry gate

Phase 17 Status complete. The open definition's datasources are the
session sample from Phase 13, or an empty object for a static report.

### Work items

- [ ] 18.1 If `src/shared/conduit.js` still has no `invokeScript`,
       add it for `POST /api/conduit/script` and cover it with a
       unit test. If CATCHUP already added it, call that function.
- [ ] 18.2 Preview requests `stage=html` with the current definition
       and the session sample. DOMPurify runs on the HTML. The result
       is assigned to a sandboxed iframe `srcdoc`.
- [ ] 18.3 Errors and warnings from the script toast. The canvas
       stays as it was.
- [ ] 18.4 Browser: preview the detail fixture with two sample rows
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

## Phase 19 — CSV emit

### Goal

`stage=csv` returns one row per detail record.

### Entry gate

Phase 17 Status complete.

### Work items

- [ ] 19.1 Amend `Reports.Render` in a new migration that replaces
       the script body the way a sibling script-update migration
       does. Re-read one before writing. Do not edit the Phase 17
       file after it has been applied.
- [ ] 19.2 CSV columns are the field items on the detail band, in
       item order. Group headers are not rows. Values are escaped.
- [ ] 19.3 A fixture pair: detail definition, source, expected CSV.
- [ ] 19.4 Hand the packet over. Andrew's call matches the fixture.

### Done means

The detail fixture renders to the expected CSV.

### Exit gate

Andrew's apply report and the fixture call.

### Status

**not started**

---

## Phase 20 — Seed lua-pdfkit and lua-zint

### Goal

Put both pure-Lua libraries into `scripts` so `require` loads them.
No report output yet.

### Entry gate

Phase 0 Status complete. Re-check the next Acuranzo file number the
day this is written. Phases 1–19 may still be open. Phases 21–23 wait
for the apply.

### Work items

- [ ] 20.1 Copy the trees named in the library lock into two
       migrations. One migration seeds every `lua-pdfkit.*` module.
       One seeds every `zint.*` module. `invokable = 0`,
       `mcp_access = 0`. Group names `lua-pdfkit` and `zint`. The
       script name is the require path after the first dot
       (`init`, `document`, `barcode`, `common`, `font.afm`, …).
       This is one library per file, which is the exception to
       one-script-per-migration. Reverse deletes only those rows.
- [ ] 20.2 Confirm a standard face (`Helvetica`) renders with `io`
       nil. If the port still `io.open`s an AFM file, bundle that
       AFM text into the seeded module before handing the packet over.
- [ ] 20.3 Hand both packets over. After apply, Andrew runs a
       one-page probe: `require("lua-pdfkit.init")`,
       `doc:text`, `doc:output()`, and `require("zint.barcode")`
       for a Code 128. The probe returns a PDF byte length and an
       SVG that contains the data string. `AllowDBModuleLoad` is on
       for that call.

### Done means

Andrew reports the probe PDF length and a Code 128 SVG from inside
Hydrogen, with `io` left off.

### Exit gate

Andrew's apply report and the probe. Test 98 on the new migrations
if he ran it.

### Status

**not started**

---

## Phase 21 — Images through image_scale

### Goal

A static image and a field image are sized by Hydrogen and shown in
the HTML report.

### Entry gate

Phase 18 and Phase 20 Status complete. `Reporting.Enabled` is true
on the server Andrew uses for the gate.

### Work items

- [ ] 21.1 Palette shows lookup 053 keys 2 (Static Image) and 22
       (Field Image). The item stores the source (static bytes as a
       data URI, or a field name whose value is a data URI) and the
       box.
- [ ] 21.2 `Reports.Render` `stage=html` calls
       `POST /api/reporting/image_scale` with `units=pt`, the box
       size, and `format=png`. Auth is `H.system_token`. The base
       URL comes from the process. An error from the endpoint
       becomes a warning and an empty box.
- [ ] 21.3 The HTML uses the returned PNG as a data URI inside the
       item box. DOMPurify keeps that image.
- [ ] 21.4 Fixture image stays under the 1 MiB result cap after
       base64. Browser: a sample PNG lands at the box size.

### Done means

The HTML preview shows the scaled image, and the call went through
`image_scale`.

### Exit gate

The script update applied, the fixture call, and the browser check.
Reporting was enabled for that check.

### Status

**not started**

---

## Phase 22 — Barcodes through lua-zint

### Goal

Static and field barcodes render as SVG in the HTML report.

### Entry gate

Phase 21 Status complete.

### Work items

- [ ] 22.1 Palette shows lookup 053 keys 43 (Static Barcode) and 45
       (Field Barcode). The item stores `symbology` as one of
       `qr`, `code39`, `code128`, `upca`, `gs1-128`, plus the data
       string or the field name. Other zint symbologies are refused
       with a warning.
- [ ] 22.2 Compose calls `zint.barcode` with no `output` path.
       HTML inlines the SVG in the item box.
- [ ] 22.3 DOMPurify is configured so this SVG survives the preview
       sandbox. A stripped SVG is a failed gate, not a pass.
- [ ] 22.4 Browser: a Code 128 of a known string and a QR of a known
       string appear in the preview. A field barcode follows the
       sample row.

### Done means

Those two symbols render from Lua and survive the preview sanitizer.

### Exit gate

Applied script update, fixture call, and the browser check.

### Status

**not started**

---

## Phase 23 — PDF emit through lua-pdfkit

### Goal

`stage=pdf` returns a PDF of the composed pages, including the images
and barcodes from the previous two phases.

### Entry gate

Phase 22 Status complete.

### Work items

- [ ] 23.1 `PDFDocument.new` uses the definition page size. One
       `addPage` per intermediate page. Text uses a standard face
       the port already carries. Positions are in PDF points.
- [ ] 23.2 Images: call `image_scale` with `format=xo` and embed the
       XObject streams from lua-pdfkit. If that embedder is more
       than a small addition to the port, use `format=png` and
       `doc:image` on the bytes, and say so in the Working Log.
- [ ] 23.3 Barcodes: scale the SVG through `image_scale` to PNG (or
       XO) at the item box and embed that. The HTML path keeps the
       SVG.
- [ ] 23.4 The JSON result is `{ "pdf_base64": "...", "warnings": [] }`.
       The fixture PDF is under `ClientInvokeMaxResultBytes`.
- [ ] 23.5 Andrew's call for the detail fixture, the image fixture,
       and the Code 128 fixture opens as a PDF with the right page
       count.

### Done means

Three fixture PDFs open and match the composed pages.

### Exit gate

Andrew's apply report and the three calls.

### Status

**not started**

---

## Phase 24 — User Lua in the definition

### Goal

An author stores a Lua function on the report, and a calculated item
runs it against the current record.

### Entry gate

Phase 13, Phase 17, and Phase 18 Status complete. lua-pdfkit and
lua-zint stay on `require`. Author formulas do not.

### Work items

- [ ] 24.1 Implement the User Lua contract. Compile every function at
       the start of compose. A compile error returns through
       `H.set_result_json` and does not throw out of the worker. A
       runtime error warns and yields an empty value.
- [ ] 24.2 Palette shows lookup 053 keys 28 and 29. The item stores
       the function name. The function panel is the formula editor
       locked above: CodeMirror 6, `language: 'lua'`, Vanadium Mono,
       footer, and `@codemirror/lint`. Source is saved on the
       definition when the editor blurs. The browser does not run
       the Lua.
- [ ] 24.3 `stage=check` compiles the posted functions with the same
       `load` path as compose and returns `{ name, line, message }`
       for each failure. The editor debounces that call for the open
       function and underlines the line. Browser: type `function(ctx`
       with no `end`, see the diagnostic, close the chunk, and see
       the diagnostic clear.
- [ ] 24.4 Fixture function `pack_code`, a stand-in for a grower pack
       date. Letters `A` through `L` are months 1 through 12. The
       return value is that letter, the week digit, and the day as
       two digits. Row `month=3`, `week=2`, `day=5` renders `C205`.
       A second row raises a runtime error and comes back empty with
       a warning. An unknown function name is a compose error.
- [ ] 24.5 Hand the script update over. After apply, the fixture call
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

## Phase 25 — Primitive aggregates

### Goal

Group footer and report footer can show the built-in reductions of
one field.

### Entry gate

Phase 14 and Phase 17 Status complete.

### Work items

- [ ] 25.1 Aggregate items store `op` and `field`. `weighted` also
       stores `weight`. Ops are the set in Primitive aggregates.
       Numeric results use lookup 053 key 35. A text mode uses key 34.
- [ ] 25.2 Compose computes the value for the current group and for
       the whole detail source, and places it on `ctx.aggregates`.
       A non-numeric value in a numeric op warns and is skipped. An
       empty numeric result is an empty string. `count` and `unique`
       of an empty scope are 0.
- [ ] 25.3 The designer can place the item on a group footer or a
       report footer and pick the op.
- [ ] 25.4 Fixture: two groups. One has an odd count and a single
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

## Phase 26 — Custom breaks and band context

### Goal

A Lua function can supply the group key, force a page break, or
insert a blank line. A calculated item on a header or footer sees
that band's context.

### Entry gate

Phase 24 and Phase 14 Status complete. When Phase 25 is already
complete, footer formulas can read `ctx.aggregates`. This phase's
fixture does not require that.

### Work items

- [ ] 26.1 A group band may set `groupKey` to a function name. The
       return value is the key. `sort` defaults to true. `sort`
       false breaks only when consecutive keys differ. One level.
- [ ] 26.2 A detail band may set `pageBreak`. A true result starts a
       new page before that row. Fixed-height paging still applies.
- [ ] 26.3 A detail band may set `lineBreak`. A true result inserts
       one blank detail-height before that row. Newlines in a
       calculated value are preserved by the HTML and PDF emitters.
       The band height does not grow.
- [ ] 26.4 Calculated items on a group, page, or report header or
       footer receive the context in the User Lua table.
       `page.count` is filled after compose finishes the page list,
       before those formulas run.
- [ ] 26.5 Fixture: group by the pack-code month letter, start a new
       page when `ctx.row.break_before` is true, and insert a blank
       line when `ctx.row.blank_before` is true. A group footer
       formula reads `ctx.group.key`. Andrew's compose call matches.
- [ ] 26.6 Designer: the group band can choose a function as its key,
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

## Later, not scheduled

Pull one of these into a numbered phase only by amending this file.

| Topic | Why it waits |
| --- | --- |
| Memos and shapes | Each is a `typeKey` with its own box behavior. Images are Phase 21. |
| Charts | A renderer beyond text, placed images, and barcodes. |
| Other barcode keys | Phase 22 places lookup 053 keys 43 and 45. Aggregate, calculated, and crosstab barcode keys stay hidden. |
| Crosstabs | A second layout engine. |
| Subreports and packages | Nested definitions and their own datasources. |
| Multiple group levels | v1 is one level. The key is a field, or one Lua function from Phase 26. |
| Related datasources | v1 is one list. A link between two lists is a new contract. |
| Stored blobs past the caps | A datasource over 256 KiB, or a PDF whose base64 JSON exceeds the 1 MiB result cap. Raising `ClientInvokeMaxResultBytes` is operator config. |
| Parameter form | The `parameters` column is reserved and empty. |
| Native XLS | CSV is Phase 19. A spreadsheet writer waits until that path is the boring one. The retired client loaded SheetJS in the browser. |
| Thumbnails | `thumbnail` stays null until a preview snapshot has a home. |
| Text measurement and stretching bands | Needs a font engine shared by compose and the designer. |
| Vanadium in PDF | The v1 PDF face is Helvetica. Embedding the product face needs the font bytes inside the sandbox, with `io` left off. |
| Live QueryRef datasources | A datasource `kind` of `query` that runs a QueryRef and uses the rows. |
| Mail and scheduled runs | Mail Relay and the script scheduler. The report plan only has to leave `Reports.Render` invokable. |

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
`H.system_token` and `H.http.post`. No new C. Phase 23 prefers `xo`
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
