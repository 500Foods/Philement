<!-- markdownlint-disable MD007 MD024 MD013 MD036 -->

# Reporting — Graphics

**Parent:** [`/docs/Li/plans/REPORTING_PLAN.md`](/docs/Li/plans/REPORTING_PLAN.md)
**Phases:** 50–67
**Stop when:** The author can place images, barcodes, and charts, and can export SVG and PNG.

The locks, the phase index, Phase 0, the backlog, and the Working Log
live in the parent. This file holds these phase bodies. Work one phase
per conversation. Write Status, Accomplished, Lessons learned, and
Handoff on the phase in this file, set the same Status in the parent's
phase table, and append the Working Log in the parent.

**Entry:** Phase 49 in [`/docs/Li/plans/reporting/05-formulas.md`](/docs/Li/plans/reporting/05-formulas.md) is complete. Phase 57 may run alongside earlier phases, as its entry gate says.
**Next file:** [`/docs/Li/plans/reporting/07-native-pdf.md`](/docs/Li/plans/reporting/07-native-pdf.md) after Phase 67.

## Phase 50 — Embedded image

### Goal

An embedded image shows in the designer and in the HTML preview.

### Entry gate

Phase 49 Status complete.

### Work items

- [ ] 50.1 Palette shows lookup 053 key 22 (Field Image). Static
       Image (key 2) is Phase 44c and is not added again. A field
       image uses the same `doc.images` registry, the same `image`
       hash, and the same placement: `dpi`, `fit`, `fitAlign`,
       `fitValign`, flip, rotation, opacity, grayscale, and invert.
       The field value supplies the bytes. Import conversion matches
       Phase 44c. JPEG with no alpha stays JPEG. PNG, GIF, WebP,
       ICO, BMP, and TIFF become PNG. SVG stays SVG. There is no
       second resample store and no downsample-cap property.
- [ ] 50.2 The image browser loads a local file or takes a clipboard
       paste. Import converts immediately and stores one registry
       entry. The panel shows the display pixel size from the box
       and the item dpi, at least 1 pixel on a side, and lets the
       author change `dpi`. It does not replace the canonical bytes
       with a display PNG. Display PNGs are built at preview into
       `params.rasters` and are not written into the definition or
       into history. Confirm commits the hash through report
       `history`. A step that moves the item stores the hash.
       Previous bytes stay only in the undo step that removed them.
       Clear drops history and drops an unreferenced registry entry.
- [ ] 50.3 `stage=html` contains each display PNG once, in one SVG
       `<defs>` image, and each placement references that id.
       DOMPurify keeps that reference. The script does not call
       `image_scale`. A headless render without `params.rasters`
       embeds the canonical bitmap and warns once.
- [ ] 50.4 Browser: the same PNG on two detail rows shows in both
       rows, at the box size, and the HTML contains that data URI
       once.

### Done means

The preview shows the same embedded PNG on two detail rows, and the HTML contains that bitmap once.

### Exit gate

The script update applied, lint, and the browser check.

### Status

**not started**

### Note (2026-11-05)

Field Image (lookup 053 key 22) is already on the Field toolbar.
The cell is a base64 data URI, `data:image/png;base64,...` or
`data:image/svg+xml;base64,...`. Preview HTML uses that URI.
Reports.Render.Images (`acuranzo_1476.lua` since the 2026-11-09
split) carries the render path.
The file browser, the `doc.images` hash, and the shared display
PNG in this phase stay unstarted. Do not mark this phase in
progress. Do not start it by writing its import pipeline.

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

- [ ] 52.1 `stage=pdf` draws each `params.rasters` entry once
       through `doc:image` and references that XObject for every
       placement. Cover is already cropped in that raster. Opacity,
       flip, and rotation are the draw. A supported SVG is one form
       XObject of paths. A headless render without `params.rasters`
       embeds the canonical bitmap once and warns once. The script
       does not call `image_scale`. There is no second embedder.
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

Make lua-zint-svg available to the Hydrogen Lua runtime with no
runtime LuaRocks dependency. Andrew rejected a `scripts` seed and
rejected raising DB2's CLOB limit. The library is packed in the
payload. luatz is packed the same way.

### Entry gate

Phase 0 Status complete. Re-check the next Acuranzo file number the
day this is written. Phases 1–52 may still be open. This sitting
also wrote Phase 54 because Andrew asked for the library and the
barcode together.

### Work items

- [x] 53.1 Pack `lua/zint` and `lua/luatz` under
       `elements/001-hydrogen/hydrogen/payloads/lua/`. The payload
       searcher is always installed at `package.searchers` index 2,
       before the file searcher. A payload file wins over a scripts
       row of the same name. `AllowDBModuleLoad` does not gate it.
       `require("zint")` loads `lua/zint/init.lua`.
       `require("zint.barcode")` loads `lua/zint/barcode.lua`, a
       one-line wrapper. `require("luatz.tzcache")` loads
       `lua/luatz/tzcache.lua`. Do not seed the library into
       `scripts`. Do not modify the lua-zint-svg repository.
       Pin: lua-zint-svg 0.1.0 commit
       `89a8231386682109312c29d8b84dc525c271eea3`. luatz is the
       patched tree already in `acuranzo_1452.lua`, including
       `tzcache.set_base_path`.
- [x] 53.2 The port writes a file when `opts.output` is set, and the
       sandbox leaves `io` nil. The probe leaves that field unset and
       uses the returned SVG string. A desktop Lua 5.5.1 probe did
       that for UPC-A, Code 128 `Ab-129`, and QR.
- [ ] 53.3 Rebuild the payload so `lua/zint` and `lua/luatz` are in
       the binary, then restart Hydrogen. `mkt` does not refresh the
       archive. Probe `require("zint.barcode")` and
       `require("luatz.tzcache")` with `AllowDBModuleLoad` off.
       Code 128 SVG contains the data. QR SVG is a path and need
       not contain the data characters. `io` stays off.

### Done means

Andrew reports a Code 128 SVG and a QR SVG from inside Hydrogen, with
`io` left off, after a binary that contains the payload libraries.

### Exit gate

Andrew's restart and the probe. The library does not need a
migration apply.

### Status

**in progress**

### Accomplished

- The payload searcher, the vendored trees, and
  `payload-generate.sh` 4.6.0 are written. Unity covers a dotted
  require with the database searcher off, a payload file winning
  over a scripts row, a compile error that is not swallowed, and a
  `.br` entry that is not compiled.
- luatz is the only other runtime library `Reports.Render`
  requires. Optional `syscall`, `unix`, `socket`, and `ngx` loads
  inside luatz stay unresolved. SchemaTool's `terminal` rock is a
  developer TUI and is not packed.
- Zoneinfo still falls back to UTC while `io` is off. Packing
  luatz does not fix that.

### Lessons learned

- A brotli pack of the zint sources does not fit DB2
  `queries.code` CLOB(100K). The payload cache is the place for
  the files.
- `require` returns the loader path as a second value. A test that
  reads the top of the stack after `return require(...)` sees nil.
  Capture the first result.
- The payload searcher has to sit at index 2. Appending it lets a
  system luatz shadow the packed one.

### Handoff

Rebuild the payload (`payload-generate.sh` or `mka`) and restart a
binary that contains `lua/zint/` and `lua/luatz/`. `mkt` does not
refresh the archive. Probe `require("zint.barcode")` and
`require("luatz.tzcache")` with `AllowDBModuleLoad` off. Leave
`opts.output` unset. `io` stays off. Do not restart the pintura
Hydrogen. Do not apply a migration for the library itself. Phase 54
in this file is the barcode UI and is also in progress. Do not start
Phase 55, Phase 30, or Phase 45. The 2026-11-09 renumber voids the
file numbers above: zint.Svg is `acuranzo_1461.lua`, and the barcode
SVG path is Reports.Render.Barcode (`acuranzo_1477.lua`). A Render
change follows REPORTING_PLAN.md Implementor workflow rule 12. The
next free Acuranzo file is **1481**.

---

## Phase 54 — Barcode SVG

### Goal

Static and field barcodes render as SVG in the HTML preview.

### Entry gate

Phase 25 Status complete. Phase 53 is in progress in this same
sitting. Andrew asked for the library and the barcode together.

### Work items

- [x] 54.1 Palette shows lookup 053 keys 43 (Static Barcode) and 45
       (Field Barcode). Static/Barcode sits immediately left of
       Image. A new barcode writes `dpi` `300`, symbology `upca`,
       and data `12345678901`. A field barcode stores `field` and
       symbology `upca`, with no static data. The stored symbology
       is one of `qr`, `code39`, `code128`, `upca`, `gs1-128`.
       Lookup 079 lists those five. Check digit `supplied` on UPC-A
       calls library id `upcachk`. Other stored names warn and skip
       the symbol.
- [x] 54.2 `Reports.Render` calls `require("zint.barcode")` with no
       `output` path and `filetype` SVG. The rectangle is free.
       The symbol keeps its aspect inside it: contain meets,
       cover slices, and none or a fitting scale-down uses the
       symbol's own pixel size. Stretch is not a fit. Bar height is the
       zint `height` option. Item rotation is CSS on the wrapper.
       The design canvas calls invokable `zint.Svg` for the same
       options. `stage=pdf` is not wired.
- [x] 54.3 Preview keeps barcode `<text>` that has `font-family`,
       an `@font-face` style, and a meet aspect. A bare `<text>`
       and a duotone `<style>` are still dropped.
- [ ] 54.4 Browser: drop a UPC, set properties, and see the symbol
       on the design canvas and in Preview HTML. Also a Code 128
       of `Ab-129`, a QR, and a field barcode. QR SVG need not
       contain the data characters. Code 128 does.

### Done means

The Code 128 and the QR survive the preview sanitizer, and the field
barcode follows the sample row.

### Exit gate

Applied script update, lint, and the browser check.

### Status

**in progress**

### Accomplished

- Static/Barcode is immediately left of Image. Field Barcode is on
  the field row. The box starts at 1.75 in by 0.7 in. The
  rectangle then takes any width and height. Fit is contain,
  cover, none, or scale-down. The symbol is not stretched.
  `acuranzo_1472.lua` teaches Preview HTML the same fit. Andrew
  applied it. `acuranzo_1474.lua` copies `item.fit` onto the
  symbol, and he applied that too.
- Each symbology has an example. A symbology change writes that
  example only when Data is empty or still an example. QR is
  `https://500courses.com`. Code 39 is `CODE-39`. Code 128 is
  `Ab-129`. UPC-A is `12345678901`. GS1-128 is
  `[01]09501101020917[10]AB-123`.
- A field barcode has no typed Data. Datasource and Field supply
  it. A new one takes the band's datasource, or the first
  datasource, and leaves Field empty. The canvas encodes the
  first sample row. `acuranzo_1473.lua` lists attribute 8 on
  lookup 053 key 45. `acuranzo_1474.lua` reads that pair. The
  iterating datasource uses the current row. Any other
  datasource uses its first row. He applied both files.
- Properties cover the library options the plan allows. Symbology
  is lookup 079. Numerics, colours, and flags are client rows.
  Check digit is `compute` or `supplied`. PDF Form XObject is a
  ready `filetype = "PDF"` result from `zint.Svg` and from
  `zint.barcode`. It is not a `stage=pdf` path.
- `acuranzo_1469.lua` inserts invokable `zint.Svg`.
  `acuranzo_1470.lua` adds lookup 079, lookup 054 key 52, and
  the attribute lists on lookup 053 keys 43 and 45.
  `acuranzo_1471.lua` amends Reports.Render. Andrew applied
  1469 through 1474. `acuranzo_1455.lua` through
  `acuranzo_1474.lua` were not edited in this sitting.
- Field Memo (23), Field Shape (26), and Field Image (22) sit on
  the Field row with Text, Number, Datetime, and Barcode. Each
  reads Datasource and Field the same way a field barcode reads
  Data. A field image cell is a PNG or SVG base64 data URI. A
  field shape cell replaces the designed kind when the value is
  a known kind code. `acuranzo_1475.lua` lists Datasource before
  Field. `acuranzo_1476.lua` amends Reports.Render. Neither file
  is applied.

### Lessons learned

- The detail band is 0.25 in. `resizeItem` clamps each side to the
  band. The rectangle stays free, and the symbol fits inside it.
- Preview `<text>` is kept only when it has `font-family`. That
  keeps zint digits and still drops a bare `<text>12</text>` on an
  image. A `<style>` element is kept only for one `@font-face`.

### Handoff

Since the 2026-11-09 renumber, `acuranzo_1465.lua` adds lookup 075
keys 13–16, `acuranzo_1464.lua` lists Datasource before Field, and
the render path is in the Reports.Render.* modules (`acuranzo_1467.lua`
through `acuranzo_1479.lua`). A Render change follows REPORTING_PLAN.md
Implementor workflow rule 12. A barcode
shows Barcode Placement, Barcode Text, Barcode Colors, and
Barcode Symbology between Object and Position. The Symbology
picker stays in Object. Check digit is in Barcode Symbology.
Code 39 and GS1-128 hide that section. The headers show before
1465 is applied. 1464 lists Datasource before Field on lookup
053 keys 20, 21, 22, 23, 24, and 26. Reports.Render reads that pair for a field memo, image, and shape.
A text, number, or datetime field uses the datasource when it is
set, and uses the current row when it is absent. The iterating
datasource uses the current row. Any other datasource uses its
first row. A field image cell is `data:image/png;base64,...` or
`data:image/svg+xml;base64,...`. An invalid URI warns once and
the picture is skipped. A field shape cell replaces the designed
kind when the value is a known kind code (`line`, `rectangle`,
`roundrect`, `ellipse`, `x`, `trapezoid`, `star`, `pentagon`,
`hexagon`, or a lookup 078 code `shape_source` accepts). Any
other string keeps the designed kind, with no extra warning. A
missing binding draws the designed kind and warns once. On
`https://lithium.500courses.com`, the Field row is Number,
Datetime, Text, Memo, Shape, Barcode, Image. Bind each one. A
memo prints the cell. An image paints the data URI. A shape
whose cell is `ellipse` draws an ellipse. A shape whose cell is
any other string keeps the designed rectangle. The barcode
rectangle stays free. Leave `opts.output` unset. Do not deploy.
Do not apply from this session. Do not restart the pintura
Hydrogen. Do not start Phase 55 or Phase 50. The next free
Acuranzo file is **1481**. QueryRef **175** stays unused. Next
free lookup id **080**. Next free lookup 054 key **53**.

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
       bar with an overlaid line, and one pie. These are new files.
       Phase 1 did not write them. The Phase 1 set is the static,
       detail, and grouped trio in
       [`/docs/Li/LITHIUM-RPT.md`](/docs/Li/LITHIUM-RPT.md).

### Done means

The chart contract in [`/docs/Li/plans/REPORTING_PLAN.md`](/docs/Li/plans/REPORTING_PLAN.md) is the one Andrew approved.

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

